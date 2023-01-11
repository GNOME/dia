/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support
 * Copyright (C) 2000,2001 Cyrille Chepelov
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "color.h"
#include "properties.h"
#include "geometry.h"
#include "text.h"

#include "grafcet.h"
#include "boolequation.h"

#include "pixmaps/condition.xpm"

#define CONDITION_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH
#define CONDITION_FONT BASIC_SANS_FONT
#define CONDITION_FONT_STYLE STYLE_BOLD
#define CONDITION_FONT_HEIGHT 0.8
#define CONDITION_LENGTH (1.5)
#define CONDITION_ARROW_SIZE 0.0 /* XXX The norm says there's no arrow head.
                                    a lot of people do put it, though. */


typedef struct _Condition {
  Connection connection;

  Boolequation *cond;

  gchar *cond_value;
  DiaFont *cond_font;
  real cond_fontheight;
  Color cond_color;

  /* computed values : */
  DiaRectangle labelbb;
} Condition;


static DiaObjectChange *condition_move_handle    (Condition         *condition,
                                                  Handle            *handle,
                                                  Point             *to,
                                                  ConnectionPoint   *cp,
                                                  HandleMoveReason   reason,
                                                  ModifierKeys       modifiers);
static DiaObjectChange *condition_move           (Condition         *condition,
                                                  Point             *to);
static void             condition_select         (Condition         *condition,
                                                  Point             *clicked_point,
                                                  DiaRenderer       *renderer);
static void             condition_draw           (Condition         *condition,
                                                  DiaRenderer       *renderer);
static DiaObject       *condition_create         (Point             *startpoint,
                                                  void              *user_data,
                                                  Handle           **handle1,
                                                  Handle           **handle2);
static real             condition_distance_from  (Condition         *condition,
                                                  Point             *point);
static void             condition_update_data    (Condition         *condition);
static void             condition_destroy        (Condition         *condition);
static DiaObject       *condition_load           (ObjectNode         obj_node,
                                                  int                version,
                                                  DiaContext        *context);
static PropDescription *condition_describe_props (Condition         *condition);
static void             condition_get_props      (Condition         *condition,
                                                  GPtrArray         *props);
static void             condition_set_props      (Condition         *condition,
                                                  GPtrArray         *props);


static ObjectTypeOps condition_type_ops = {
  (CreateFunc) condition_create,              /* create */
  (LoadFunc)   condition_load,                /* load */
  (SaveFunc)   object_save_using_properties,  /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};


DiaObjectType condition_type = {
  "GRAFCET - Condition",  /* name */
  0,                      /* version */
  condition_xpm,          /* pixmap */
  &condition_type_ops     /* ops */
};


static ObjectOps condition_ops = {
  (DestroyFunc)         condition_destroy,
  (DrawFunc)            condition_draw,
  (DistanceFunc)        condition_distance_from,
  (SelectFunc)          condition_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            condition_move,
  (MoveHandleFunc)      condition_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   condition_describe_props,
  (GetPropsFunc)        condition_get_props,
  (SetPropsFunc)        condition_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};


static PropDescription condition_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "condition",PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Condition"),N_("The boolean equation of the condition")},
  { "cond_font",PROP_TYPE_FONT, PROP_FLAG_VISIBLE,
    N_("Font"),N_("The condition's font") },
  { "cond_fontheight",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Font size"),N_("The condition's font size"),
    &prop_std_text_height_data},
  { "cond_color",PROP_TYPE_COLOUR,PROP_FLAG_VISIBLE,
    N_("Color"),N_("The condition's color")},
  PROP_DESC_END
};


static PropDescription *
condition_describe_props (Condition *condition)
{
  if (condition_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (condition_props);
  }
  return condition_props;
}


static PropOffset condition_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  {"condition", PROP_TYPE_STRING, offsetof (Condition, cond_value)},
  {"cond_font", PROP_TYPE_FONT, offsetof (Condition, cond_font)},
  {"cond_fontheight", PROP_TYPE_REAL, offsetof (Condition, cond_fontheight)},
  {"cond_color", PROP_TYPE_COLOUR, offsetof (Condition, cond_color)},
  { NULL,0,0 }
};


static void
condition_get_props (Condition *condition, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (condition),
                                 condition_offsets,props);
}


static void
condition_set_props (Condition *condition, GPtrArray *props)
{
  object_set_props_from_offsets (&condition->connection.object,
                                 condition_offsets,props);

  boolequation_set_value (condition->cond,condition->cond_value);
  g_clear_object (&condition->cond->font);
  condition->cond->font = g_object_ref (condition->cond_font);
  condition->cond->fontheight = condition->cond_fontheight;
  condition->cond->color = condition->cond_color;
  condition_update_data (condition);
}


static real
condition_distance_from (Condition *condition, Point *point)
{
  Connection *conn = &condition->connection;
  real dist;
  dist = distance_rectangle_point (&condition->labelbb, point);
  dist = MIN (dist,
              distance_line_point (&conn->endpoints[0],
                                   &conn->endpoints[1],
                                   CONDITION_LINE_WIDTH,
                                   point));
  return dist;
}


static void
condition_select (Condition   *condition,
                  Point       *clicked_point,
                  DiaRenderer *interactive_renderer)
{
  condition_update_data (condition);
}


static DiaObjectChange *
condition_move_handle (Condition        *condition,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  Point s,e,v;
  int horiz;

  g_return_val_if_fail (condition!= NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  switch (handle->id) {
    case HANDLE_MOVE_STARTPOINT:
      point_copy (&s, to);
      point_copy (&e, &condition->connection.endpoints[1]);
      point_copy (&v, &e);
      point_sub (&v, &s);

      horiz = ABS (v.x) > ABS (v.y);
      if (horiz) {
        v.y = 0.0;
      } else {
        v.x = 0.0;
      }

      point_copy (&s,&e);
      point_sub (&s,&v);

      /* XXX: fix e to make it look good (what's good ?) V is a good hint ? */
      connection_move_handle (&condition->connection,
                              HANDLE_MOVE_STARTPOINT,
                              &s,
                              cp,
                              reason,
                              modifiers);

      break;
    case HANDLE_MOVE_ENDPOINT:
      point_copy (&s, &condition->connection.endpoints[0]);
      point_copy (&e, &condition->connection.endpoints[1]);
      point_copy (&v, &e);
      point_sub (&v, &s);
      connection_move_handle (&condition->connection,
                              HANDLE_MOVE_ENDPOINT,
                              to,
                              cp,
                              reason,
                              modifiers);
      point_copy (&s,to);
      point_sub (&s,&v);
      connection_move_handle (&condition->connection,
                              HANDLE_MOVE_STARTPOINT,
                              &s,
                              NULL,
                              reason,
                              0);
      break;
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_return_val_if_reached (NULL);
      break;
  }

  condition_update_data (condition);

  return NULL;
}


static DiaObjectChange *
condition_move (Condition *condition, Point *to)
{
  Point start_to_end;
  Point *endpoints = &condition->connection.endpoints[0];

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);
  point_copy(&endpoints[0],to);
  point_copy(&endpoints[1],to);
  point_add(&endpoints[1], &start_to_end);

  condition_update_data(condition);

  return NULL;
}

static void
condition_update_data(Condition *condition)
{
  Connection *conn = &condition->connection;
  DiaObject *obj = &conn->object;

  obj->position = conn->endpoints[0];
  connection_update_boundingbox(conn);

  /* compute the label's width and bounding box */
  condition->cond->pos.x = conn->endpoints[0].x +
    (.5 * dia_font_string_width("a", condition->cond->font,
			    condition->cond->fontheight));
  condition->cond->pos.y = conn->endpoints[0].y + condition->cond->fontheight;

  boolequation_calc_boundingbox(condition->cond, &condition->labelbb);
  rectangle_union(&obj->bounding_box,&condition->labelbb);

  connection_update_handles(conn);
}


static void
condition_draw (Condition *condition, DiaRenderer *renderer)
{
  Connection *conn = &condition->connection;

  dia_renderer_set_linewidth (renderer, CONDITION_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (CONDITION_ARROW_SIZE > (CONDITION_LINE_WIDTH/2.0)) {
    Arrow arrow;
    arrow.type = ARROW_FILLED_TRIANGLE;
    arrow.width = CONDITION_ARROW_SIZE;
    arrow.length = CONDITION_ARROW_SIZE/2;
    dia_renderer_draw_line_with_arrows (renderer,
                                        &conn->endpoints[0],
                                        &conn->endpoints[1],
                                        CONDITION_LINE_WIDTH,
                                        &color_black,
                                        &arrow,
                                        NULL);
  } else {
    dia_renderer_draw_line (renderer,
                            &conn->endpoints[0],
                            &conn->endpoints[1],
                            &color_black);
  }

  boolequation_draw (condition->cond, renderer);
}

static DiaObject *
condition_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Condition *condition;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  Point defaultlen  = {0.0,CONDITION_ARROW_SIZE};

  DiaFont *default_font;
  real default_fontheight;
  Color fg_color;

  condition = g_new0 (Condition, 1);
  conn = &condition->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &condition_type;
  obj->ops = &condition_ops;

  point_copy(&conn->endpoints[0],startpoint);
  point_copy(&conn->endpoints[1],startpoint);
  point_add(&conn->endpoints[1], &defaultlen);

  connection_init(conn, 2,0);

  default_font = NULL;
  attributes_get_default_font(&default_font,&default_fontheight);
  fg_color = attributes_get_foreground();

  condition->cond = boolequation_create("",default_font,default_fontheight,
				 &fg_color);
  condition->cond_value = g_strdup("");
  condition->cond_font = g_object_ref (default_font);
  condition->cond_fontheight = default_fontheight;
  condition->cond_color = fg_color;

  extra->start_trans =
    extra->start_long =
    extra->end_long = CONDITION_LINE_WIDTH/2.0;
  extra->end_trans = MAX(CONDITION_LINE_WIDTH,CONDITION_ARROW_SIZE) / 2.0;

  condition_update_data(condition);

  conn->endpoint_handles[0].connect_type = HANDLE_NONCONNECTABLE;
  *handle1 = &conn->endpoint_handles[0];
  *handle2 = &conn->endpoint_handles[1];

  g_clear_object (&default_font);

  return &condition->connection.object;
}


static void
condition_destroy (Condition *condition)
{
  g_clear_object (&condition->cond_font);
  boolequation_destroy (condition->cond);
  g_clear_pointer (&condition->cond_value, g_free);
  connection_destroy (&condition->connection);
}


static DiaObject *
condition_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&condition_type,
                                      obj_node,version,ctx);
}









