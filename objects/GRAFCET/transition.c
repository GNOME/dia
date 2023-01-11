/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support
 * Copyright(C) 2000,2001 Cyrille Chepelov
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
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "color.h"
#include "properties.h"
#include "geometry.h"
#include "text.h"

#include "grafcet.h"
#include "boolequation.h"

#include "pixmaps/transition.xpm"

#define HANDLE_NORTH HANDLE_CUSTOM1
#define HANDLE_SOUTH HANDLE_CUSTOM2


#define TRANSITION_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH

#define TRANSITION_FONT "Arial"
#define TRANSITION_STYLE STYLE_BOLD
#define TRANSITION_FONT_HEIGHT 0.8
#define TRANSITION_DECLAREDHEIGHT 2.0
#define TRANSITION_DECLAREDWIDTH 2.0
#define TRANSITION_HEIGHT .5
#define TRANSITION_WIDTH 1.5


typedef struct _Transition {
  Element element;

  Boolequation *receptivity;
  DiaFont *rcep_font;
  real rcep_fontheight;
  Color rcep_color;
  char *rcep_value;

  ConnectionPoint connections[2];
  Handle north,south;
  Point SD1, SD2, NU1, NU2;

  /* computed values : */
  DiaRectangle rceptbb; /* The bounding box of the receptivity */
  Point A, B, C, D, Z;
} Transition;


static DiaObjectChange *transition_move_handle    (Transition        *transition,
                                                   Handle            *handle,
                                                   Point             *to,
                                                   ConnectionPoint   *cp,
                                                   HandleMoveReason   reason,
                                                   ModifierKeys       modifiers);
static DiaObjectChange *transition_move           (Transition        *transition,
                                                   Point             *to);
static void             transition_select         (Transition        *transition,
                                                   Point             *clicked_point,
                                                   DiaRenderer       *interactive_renderer);
static void             transition_draw           (Transition        *transition,
                                                   DiaRenderer       *renderer);
static DiaObject       *transition_create         (Point             *startpoint,
                                                   void              *user_data,
                                                   Handle           **handle1,
                                                   Handle           **handle2);
static real             transition_distance_from  (Transition        *transition,
                                                   Point             *point);
static void             transition_update_data    (Transition        *transition);
static void             transition_destroy        (Transition        *transition);
static DiaObject       *transition_load           (ObjectNode         obj_node,
                                                   int                version,
                                                   DiaContext        *ctx);
static PropDescription *transition_describe_props (Transition        *transition);
static void             transition_get_props      (Transition        *transition,
                                                   GPtrArray         *props);
static void             transition_set_props      (Transition        *transition,
                                                   GPtrArray         *props);


static ObjectTypeOps transition_type_ops = {
  (CreateFunc)transition_create,   /* create */
  (LoadFunc)  transition_load,
  (SaveFunc)  object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};


DiaObjectType transition_type =
{
  "GRAFCET - Transition", /* name */
  0,                   /* version */
  transition_xpm,       /* pixmap */
  &transition_type_ops     /* ops */
};


static ObjectOps transition_ops = {
  (DestroyFunc)         transition_destroy,
  (DrawFunc)            transition_draw,
  (DistanceFunc)        transition_distance_from,
  (SelectFunc)          transition_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            transition_move,
  (MoveHandleFunc)      transition_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   transition_describe_props,
  (GetPropsFunc)        transition_get_props,
  (SetPropsFunc)        transition_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription transition_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "receptivity",PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Receptivity"),N_("The boolean equation of the receptivity")},
  { "rcep_font",PROP_TYPE_FONT, PROP_FLAG_VISIBLE,
    N_("Font"),N_("The receptivity's font") },
  { "rcep_fontheight",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Font size"),N_("The receptivity's font size"),
    &prop_std_text_height_data},
  { "rcep_color",PROP_TYPE_COLOUR,PROP_FLAG_VISIBLE,
    N_("Color"),N_("The receptivity's color")},
  { "north_pos",PROP_TYPE_POINT,0,N_("North point"),NULL },
  { "south_pos",PROP_TYPE_POINT,0,N_("South point"),NULL },
  PROP_DESC_END
};


static PropDescription *
transition_describe_props (Transition *transition)
{
  if (transition_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (transition_props);
  }
  return transition_props;
}


static PropOffset transition_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"receptivity",PROP_TYPE_STRING,offsetof(Transition,rcep_value)},
  {"rcep_font",PROP_TYPE_FONT,offsetof(Transition,rcep_font)},
  {"rcep_fontheight",PROP_TYPE_REAL,offsetof(Transition,rcep_fontheight)},
  {"rcep_color",PROP_TYPE_COLOUR,offsetof(Transition,rcep_color)},
  {"north_pos",PROP_TYPE_POINT,offsetof(Transition,north.pos)},
  {"south_pos",PROP_TYPE_POINT,offsetof(Transition,south.pos)},
  { NULL,0,0 }
};


static void
transition_get_props (Transition *transition, GPtrArray *props)
{
  object_get_props_from_offsets (&transition->element.object,
                                 transition_offsets,props);
}


static void
transition_set_props (Transition *transition, GPtrArray *props)
{
  object_set_props_from_offsets (&transition->element.object,
                                 transition_offsets,
                                 props);

  boolequation_set_value (transition->receptivity,transition->rcep_value);
  g_clear_object (&transition->receptivity->font);
  transition->receptivity->font = g_object_ref (transition->rcep_font);
  transition->receptivity->fontheight = transition->rcep_fontheight;
  transition->receptivity->color = transition->rcep_color;

  transition_update_data (transition);
}


static void
transition_update_data (Transition *transition)
{
  Element *elem = &transition->element;
  DiaObject *obj = &elem->object;
  Point *p;

  transition->element.extra_spacing.border_trans = TRANSITION_LINE_WIDTH / 2.0;

  obj->position = elem->corner;
  elem->width = TRANSITION_DECLAREDWIDTH;
  elem->height = TRANSITION_DECLAREDWIDTH;

  /* compute the useful points' positions : */
  transition->A.x = transition->B.x = (TRANSITION_DECLAREDWIDTH / 2.0);
  transition->A.y = (TRANSITION_DECLAREDHEIGHT / 2.0)
    - (TRANSITION_HEIGHT / 2.0);
  transition->B.y = transition->A.y + TRANSITION_HEIGHT;
  transition->C.y = transition->D.y = (TRANSITION_DECLAREDHEIGHT / 2.0);
  transition->C.x =
    (TRANSITION_DECLAREDWIDTH / 2.0) - (TRANSITION_WIDTH / 2.0);
  transition->D.x = transition->C.x + TRANSITION_WIDTH;

  transition->Z.y = (TRANSITION_DECLAREDHEIGHT / 2.0)
    + (.3 * transition->receptivity->fontheight);

  transition->Z.x = transition->D.x +
    dia_font_string_width ("_", transition->receptivity->font,
                           transition->receptivity->fontheight);

  for (p = &transition->A; p <= &transition->Z; p++) {
    point_add (p,&elem->corner);
  }

  transition->receptivity->pos = transition->Z;


  /* Update handles: */
  if (transition->north.pos.x == -65536.0) {
    transition->north.pos = transition->A;
    transition->south.pos = transition->B;
  }
  transition->NU1.x = transition->north.pos.x;
  transition->NU2.x = transition->A.x;
  transition->NU1.y = transition->NU2.y =
    (transition->north.pos.y + transition->A.y) / 2.0;
  transition->SD1.x = transition->B.x;
  transition->SD2.x = transition->south.pos.x;
  transition->SD1.y = transition->SD2.y =
    (transition->south.pos.y + transition->B.y) / 2.0;

  obj->connections[0]->pos = transition->A;
  obj->connections[0]->directions = DIR_EAST|DIR_WEST;
  obj->connections[1]->pos = transition->B;
  obj->connections[1]->directions = DIR_EAST|DIR_WEST;


  element_update_boundingbox (elem);

  rectangle_add_point (&obj->bounding_box,&transition->north.pos);
  rectangle_add_point (&obj->bounding_box,&transition->south.pos);

  /* compute the rcept's width and bounding box, then merge. */
  boolequation_calc_boundingbox (transition->receptivity,
                                 &transition->rceptbb);
  rectangle_union (&obj->bounding_box, &transition->rceptbb);

  element_update_handles (elem);
}


static real
transition_distance_from (Transition *transition, Point *point)
{
  real dist;
  dist = distance_rectangle_point (&transition->rceptbb, point);
  dist = MIN (dist,
              distance_line_point (&transition->C, &transition->D,
                                   TRANSITION_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&transition->north.pos, &transition->NU1,
                                   TRANSITION_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&transition->NU1, &transition->NU2,
                                   TRANSITION_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&transition->NU2, &transition->SD1,
                                   TRANSITION_LINE_WIDTH, point));
  /* A and B are on the [NU2; SD1] segment. */
  dist = MIN (dist,
              distance_line_point (&transition->SD1, &transition->SD2,
                                   TRANSITION_LINE_WIDTH, point));
  dist = MIN (dist,
              distance_line_point (&transition->SD2, &transition->south.pos,
                                   TRANSITION_LINE_WIDTH, point));

  return dist;
}


static void
transition_select (Transition  *transition,
                   Point       *clicked_point,
                   DiaRenderer *interactive_renderer)
{
  transition_update_data (transition);
}


static DiaObjectChange *
transition_move_handle (Transition       *transition,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  g_return_val_if_fail (transition != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  switch (handle->id) {
    case HANDLE_NORTH:
      transition->north.pos = *to;
      if (transition->north.pos.y > transition->A.y)
        transition->north.pos.y = transition->A.y;
      break;
    case HANDLE_SOUTH:
      transition->south.pos = *to;
      if (transition->south.pos.y < transition->B.y)
        transition->south.pos.y = transition->B.y;
      break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      element_move_handle (&transition->element,
                           handle->id, to, cp,
                           reason, modifiers);
  }

  transition_update_data (transition);

  return NULL;
}


static DiaObjectChange *
transition_move (Transition *transition, Point *to)
{
  Point delta = *to;
  Element *elem = &transition->element;
  point_sub (&delta,&transition->element.corner);
  transition->element.corner = *to;
  point_add (&transition->north.pos,&delta);
  point_add (&transition->south.pos,&delta);

  element_update_handles (elem);
  transition_update_data (transition);

  return NULL;
}


static void
transition_draw (Transition *transition, DiaRenderer *renderer)
{
  Point pts[6];
  dia_renderer_set_linewidth (renderer, TRANSITION_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  pts[0] = transition->north.pos;
  pts[1] = transition->NU1;
  pts[2] = transition->NU2;
  pts[3] = transition->SD1;
  pts[4] = transition->SD2;
  pts[5] = transition->south.pos;
  dia_renderer_draw_polyline (renderer,
                              pts,
                              sizeof(pts)/sizeof(pts[0]),
                              &color_black);

  dia_renderer_draw_line (renderer,
                          &transition->C,
                          &transition->D,
                          &color_black);

  boolequation_draw (transition->receptivity,renderer);
}


static DiaObject *
transition_create (Point   *startpoint,
                   void    *user_data,
                   Handle **handle1,
                   Handle **handle2)
{
  Transition *transition;
  DiaObject *obj;
  int i;
  Element *elem;
  DiaFont *default_font = NULL;
  real default_fontheight;
  Color fg_color;

  transition = g_new0 (Transition, 1);
  elem = &transition->element;
  obj = &elem->object;

  obj->type = &transition_type;
  obj->ops = &transition_ops;

  elem->corner = *startpoint;
  elem->width = TRANSITION_DECLAREDWIDTH;
  elem->height = TRANSITION_DECLAREDHEIGHT;

  element_init (elem, 10, 2);

  attributes_get_default_font (&default_font, &default_fontheight);
  fg_color = attributes_get_foreground ();

  transition->receptivity =
    boolequation_create ("",
                         default_font,
                         default_fontheight,
                         &fg_color);

  transition->rcep_value = g_strdup ("");
  transition->rcep_font = g_object_ref (default_font);
  transition->rcep_fontheight = default_fontheight;
  transition->rcep_color = fg_color;

  g_clear_object (&default_font);


  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &transition->north;
  obj->handles[9] = &transition->south;
  transition->north.connect_type = HANDLE_CONNECTABLE;
  transition->north.type = HANDLE_MAJOR_CONTROL;
  transition->north.id = HANDLE_NORTH;
  transition->south.connect_type = HANDLE_CONNECTABLE;
  transition->south.type = HANDLE_MAJOR_CONTROL;
  transition->south.id = HANDLE_SOUTH;
  transition->north.pos.x = -65536.0; /* magic */

  for (i = 0; i < 2; i++) {
    obj->connections[i] = &transition->connections[i];
    obj->connections[i]->object = obj;
    obj->connections[i]->connected = NULL;
  }

  transition_update_data (transition);

  *handle1 = NULL;
  *handle2 = obj->handles[0];

  return DIA_OBJECT (transition);
}


static void
transition_destroy (Transition *transition)
{
  g_clear_object (&transition->rcep_font);
  boolequation_destroy (transition->receptivity);
  g_clear_pointer (&transition->rcep_value, g_free);
  element_destroy (&transition->element);
}


static DiaObject *
transition_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties (&transition_type,
                                       obj_node,version,ctx);
}
