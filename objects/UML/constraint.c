/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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
#include <string.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"
#include "stereotype.h"
#include "attributes.h"

#include "pixmaps/constraint.xpm"

typedef struct _Constraint Constraint;

struct _Constraint {
  Connection connection;

  Handle text_handle;

  char *text;
  char *brtext;
  Point text_pos;
  real text_width;

  Color text_color;
  Color line_color;

  DiaFont *font;
  real     font_height;
  real     line_width;
};


#define CONSTRAINT_DASHLEN 0.4
#define CONSTRAINT_ARROWLEN (constraint->font_height)
#define CONSTRAINT_ARROWWIDTH (constraint->font_height*5./8.)

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static DiaObjectChange* constraint_move_handle(Constraint *constraint, Handle *handle,
					    Point *to, ConnectionPoint *cp,
					    HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* constraint_move(Constraint *constraint, Point *to);
static void constraint_select(Constraint *constraint, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void constraint_draw(Constraint *constraint, DiaRenderer *renderer);
static DiaObject *constraint_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real constraint_distance_from(Constraint *constraint, Point *point);
static void constraint_update_data(Constraint *constraint);
static void constraint_destroy(Constraint *constraint);

static PropDescription *constraint_describe_props(Constraint *constraint);
static void constraint_get_props(Constraint * constraint, GPtrArray *props);
static void constraint_set_props(Constraint * constraint, GPtrArray *props);

static DiaObject *constraint_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps constraint_type_ops =
{
  (CreateFunc) constraint_create,
  (LoadFunc)   constraint_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType constraint_type =
{
  "UML - Constraint",  /* name */
  0,                   /* version */
  constraint_xpm,      /* pixmap */
  &constraint_type_ops /* ops */
};

static ObjectOps constraint_ops = {
  (DestroyFunc)         constraint_destroy,
  (DrawFunc)            constraint_draw,
  (DistanceFunc)        constraint_distance_from,
  (SelectFunc)          constraint_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            constraint_move,
  (MoveHandleFunc)      constraint_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   constraint_describe_props,
  (GetPropsFunc)        constraint_get_props,
  (SetPropsFunc)        constraint_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription constraint_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "constraint", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Constraint:"), NULL, NULL },
  { "text_pos", PROP_TYPE_POINT, 0, NULL, NULL, NULL},
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
constraint_describe_props(Constraint *constraint)
{
  if (constraint_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(constraint_props);
  }
  return constraint_props;
}

static PropOffset constraint_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "constraint", PROP_TYPE_STRING, offsetof(Constraint, text) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Constraint, text_pos) },
  { "text_font", PROP_TYPE_FONT, offsetof(Constraint, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Constraint, font_height) },
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Constraint, text_color)},
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Constraint, line_width) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Constraint, line_color)},
  { NULL, 0, 0 }
};

static void
constraint_get_props(Constraint * constraint, GPtrArray *props)
{
  object_get_props_from_offsets(&constraint->connection.object,
                                constraint_offsets, props);
}

static void
constraint_set_props(Constraint *constraint, GPtrArray *props)
{
  object_set_props_from_offsets(&constraint->connection.object,
                                constraint_offsets, props);
  g_clear_pointer (&constraint->brtext, g_free);
  constraint_update_data(constraint);
}

static real
constraint_distance_from(Constraint *constraint, Point *point)
{
  Point *endpoints;
  real dist;

  endpoints = &constraint->connection.endpoints[0];

  dist = distance_line_point(&endpoints[0], &endpoints[1],
                             constraint->line_width, point);
  return dist;
}

static void
constraint_select(Constraint *constraint, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&constraint->connection);
}


static DiaObjectChange *
constraint_move_handle (Constraint       *constraint,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (constraint != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    constraint->text_pos = *to;
  } else  {
    endpoints = &constraint->connection.endpoints[0];
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle (&constraint->connection,
                            handle->id,
                            to,
                            cp,
                            reason,
                            modifiers);
    connection_adjust_for_autogap (&constraint->connection);
    p2.x = 0.5 * (endpoints[0].x + endpoints[1].x);
    p2.y = 0.5 * (endpoints[0].y + endpoints[1].y);
    point_sub (&p2, &p1);
    point_add (&constraint->text_pos, &p2);
  }

  constraint_update_data (constraint);

  return NULL;
}


static DiaObjectChange*
constraint_move(Constraint *constraint, Point *to)
{
  Point start_to_end;
  Point *endpoints = &constraint->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&constraint->text_pos, &delta);

  constraint_update_data(constraint);

  return NULL;
}

static void
constraint_draw (Constraint *constraint, DiaRenderer *renderer)
{
  Point *endpoints;
  Arrow arrow;

  g_return_if_fail (constraint != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &constraint->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, constraint->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, CONSTRAINT_DASHLEN);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  arrow.type = ARROW_LINES;
  arrow.length = CONSTRAINT_ARROWLEN;
  arrow.width = CONSTRAINT_ARROWWIDTH;

  dia_renderer_draw_line_with_arrows (renderer,
                                      &endpoints[0],
                                      &endpoints[1],
                                      constraint->line_width,
                                      &constraint->line_color,
                                      NULL,
                                      &arrow);

  dia_renderer_set_font (renderer,
                         constraint->font,
                         constraint->font_height);
  dia_renderer_draw_string (renderer,
                            constraint->brtext,
                            &constraint->text_pos,
                            DIA_ALIGN_LEFT,
                            &constraint->text_color);
}

static DiaObject *
constraint_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Constraint *constraint;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  constraint = g_new0 (Constraint, 1);

  /* old defaults */
  constraint->font_height = 0.8;
  constraint->font =
      dia_font_new_from_style (DIA_FONT_MONOSPACE, constraint->font_height);
  constraint->line_width = 0.1;

  conn = &constraint->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;

  obj->type = &constraint_type;
  obj->ops = &constraint_ops;

  connection_init(conn, 3, 0);

  constraint->text_color = color_black;
  constraint->line_color = attributes_get_foreground();
  constraint->text = g_strdup("");
  constraint->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  constraint->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y) - 0.2;

  constraint->text_handle.id = HANDLE_MOVE_TEXT;
  constraint->text_handle.type = HANDLE_MINOR_CONTROL;
  constraint->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  constraint->text_handle.connected_to = NULL;
  obj->handles[2] = &constraint->text_handle;

  constraint->brtext = NULL;
  constraint_update_data(constraint);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &constraint->connection.object;
}


static void
constraint_destroy (Constraint *constraint)
{
  connection_destroy (&constraint->connection);
  g_clear_object (&constraint->font);
  g_clear_pointer (&constraint->brtext, g_free);
  g_clear_pointer (&constraint->text, g_free);
}

static void
constraint_update_data(Constraint *constraint)
{
  Connection *conn = &constraint->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;
  LineBBExtras *extra;

  if ((constraint->text) && (constraint->text[0] == '{')) {
    /* we might have a string loaded from an older dia. Clean it up. */
    g_clear_pointer (&constraint->brtext, g_free);
    constraint->brtext = constraint->text;
    constraint->text = bracketted_to_string(constraint->text,"{","}");
  } else if (!constraint->brtext) {
    constraint->brtext = string_to_bracketted(constraint->text, "{", "}");
  }

  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  obj->position = conn->endpoints[0];

  constraint->text_width = dia_font_string_width(constraint->brtext,
                                                 constraint->font,
                                                 constraint->font_height);

  constraint->text_handle.pos = constraint->text_pos;

  connection_update_handles(conn);

  /* Boundingbox: */
  extra = &conn->extra_spacing;

  extra->start_long =
    extra->start_trans =
    extra->end_long = constraint->line_width/2.0;
  extra->end_trans = MAX(constraint->line_width,CONSTRAINT_ARROWLEN)/2.0;

  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = constraint->text_pos.x;
  rect.right = rect.left + constraint->text_width;
  rect.top = constraint->text_pos.y -
      dia_font_ascent(constraint->brtext,
                      constraint->font, constraint->font_height);
  rect.bottom = rect.top + constraint->font_height;
  rectangle_union(&obj->bounding_box, &rect);
}


static DiaObject *
constraint_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&constraint_type,
                                      obj_node,version,ctx);
}
