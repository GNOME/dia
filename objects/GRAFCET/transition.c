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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
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
#define TRANSITION_FONT "Helvetica-Bold"
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
  Point SD1,SD2,NU1,NU2;
  
  /* computed values : */
  Rectangle rceptbb; /* The bounding box of the receptivity */
  Point A,B,C,D,Z;
} Transition;

static void transition_move_handle(Transition *transition, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void transition_move(Transition *transition, Point *to);
static void transition_select(Transition *transition, Point *clicked_point,
			      Renderer *interactive_renderer);
static void transition_draw(Transition *transition, Renderer *renderer);
static Object *transition_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real transition_distance_from(Transition *transition, Point *point);
static void transition_update_data(Transition *transition);
static void transition_destroy(Transition *transition);

static Object *transition_load(ObjectNode obj_node, int version,
			       const char *filename);
static PropDescription *transition_describe_props(Transition *transition);
static void transition_get_props(Transition *transition, 
                                 GPtrArray *props);
static void transition_set_props(Transition *transition, 
                                 GPtrArray *props);

static ObjectTypeOps transition_type_ops =
{
  (CreateFunc)transition_create,   /* create */
  (LoadFunc)  transition_load /*using_properties*/,
  (SaveFunc)  object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType transition_type =
{
  "GRAFCET - Transition",   /* name */
  0,                         /* version */
  (char **) transition_xpm,      /* pixmap */
  
  &transition_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   transition_describe_props,
  (GetPropsFunc)        transition_get_props,
  (SetPropsFunc)        transition_set_props
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
transition_describe_props(Transition *transition) 
{
  if (transition_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(transition_props);
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
transition_get_props(Transition *transition, GPtrArray *props)
{  
  object_get_props_from_offsets(&transition->element.object,
                                transition_offsets,props);
}

static void
transition_set_props(Transition *transition, GPtrArray *props)
{
  object_set_props_from_offsets(&transition->element.object,
                                transition_offsets,props);

  boolequation_set_value(transition->receptivity,transition->rcep_value);
  transition->receptivity->font = transition->rcep_font;
  transition->receptivity->fontheight = transition->rcep_fontheight;
  transition->receptivity->color = transition->rcep_color;

  transition_update_data(transition);
}

static void
transition_update_data(Transition *transition)
{
  Element *elem = &transition->element;
  Object *obj = &elem->object;
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
    font_string_width(" ",transition->receptivity->font,
		      transition->receptivity->fontheight);

  for (p = &transition->A; p <= &transition->Z; p++) 
    point_add(p,&elem->corner);

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
  obj->connections[1]->pos = transition->B;


  element_update_boundingbox(elem);

  rectangle_add_point(&obj->bounding_box,&transition->north.pos);
  rectangle_add_point(&obj->bounding_box,&transition->south.pos);

  /* compute the rcept's width and bounding box, then merge. */
  boolequation_calc_boundingbox(transition->receptivity,&transition->rceptbb);
  rectangle_union(&obj->bounding_box,&transition->rceptbb);

  element_update_handles(elem);
}

static real
transition_distance_from(Transition *transition, Point *point)
{
  real dist;
  dist = distance_rectangle_point(&transition->rceptbb,point);
  dist = MIN(dist,distance_line_point(&transition->C,&transition->D,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->north.pos,&transition->NU1,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->NU1,&transition->NU2,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->NU2,&transition->SD1,
				      TRANSITION_LINE_WIDTH,point));
  /* A and B are on the [NU2; SD1] segment. */
  dist = MIN(dist,distance_line_point(&transition->SD1,&transition->SD2,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->SD2,&transition->south.pos,
				      TRANSITION_LINE_WIDTH,point));

  return dist;
}

static void
transition_select(Transition *transition, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  transition_update_data(transition);
}

static void
transition_move_handle(Transition *transition, Handle *handle,
		       Point *to, HandleMoveReason reason, 
		       ModifierKeys modifiers)
{
  g_assert(transition!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  switch(handle->id) {
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
  default:
    element_move_handle(&transition->element, handle->id, to, reason);
  }

  transition_update_data(transition);
}


static void
transition_move(Transition *transition, Point *to)
{
  Point delta = *to;
  Element *elem = &transition->element;
  point_sub(&delta,&transition->element.corner);
  transition->element.corner = *to;
  point_add(&transition->north.pos,&delta);
  point_add(&transition->south.pos,&delta);

  element_update_handles(elem);
  transition_update_data(transition);
}


static void 
transition_draw(Transition *transition, Renderer *renderer)
{
  Point pts[6];
  renderer->ops->set_linewidth(renderer, TRANSITION_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  pts[0] = transition->north.pos;
  pts[1] = transition->NU1;
  pts[2] = transition->NU2;
  pts[3] = transition->SD1;
  pts[4] = transition->SD2;
  pts[5] = transition->south.pos;
  renderer->ops->draw_polyline(renderer,pts,sizeof(pts)/sizeof(pts[0]),
			       &color_black);

  renderer->ops->draw_line(renderer,
			     &transition->C,&transition->D,
			     &color_black);
  
  boolequation_draw(transition->receptivity,renderer);
}

static Object *
transition_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Transition *transition;
  Object *obj;
  int i;
  Element *elem;
  DiaFont *default_font; 
  real default_fontheight;
  Color fg_color;

  transition = g_malloc0(sizeof(Transition));
  elem = &transition->element;
  obj = &elem->object;
  
  obj->type = &transition_type;
  obj->ops = &transition_ops;
  
  elem->corner = *startpoint;
  elem->width = TRANSITION_DECLAREDWIDTH;
  elem->height = TRANSITION_DECLAREDHEIGHT;

  element_init(elem, 10,2);

  attributes_get_default_font(&default_font,&default_fontheight);
  fg_color = attributes_get_foreground();
  
  transition->receptivity = 
    boolequation_create("",
                        default_font,
                        default_fontheight,
                        &fg_color);

  transition->rcep_value = g_strdup("");
  transition->rcep_font = default_font;
  transition->rcep_fontheight = default_fontheight;
  transition->rcep_color = fg_color;


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

  for (i=0;i<2;i++) {
    obj->connections[i] = &transition->connections[i];
    obj->connections[i]->object = obj;
    obj->connections[i]->connected = NULL;
  }

  transition_update_data(transition);

  *handle1 = NULL;
  *handle2 = obj->handles[0];  

  return &transition->element.object;
}

static void
transition_destroy(Transition *transition)
{
  boolequation_destroy(transition->receptivity);
  g_free(transition->rcep_value);
  element_destroy(&transition->element);
}

static Object *
transition_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&transition_type,
                                      obj_node,version,filename);
}








