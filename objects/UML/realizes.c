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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "arrows.h"
#include "properties.h"
#include "stereotype.h"
#include "uml.h"

#include "pixmaps/realizes.xpm"

typedef struct _Realizes Realizes;
typedef struct _RealizesState RealizesState;

struct _RealizesState {
  ObjectState obj_state;

  char *name;
  char *stereotype; 

};

struct _Realizes {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  char *name;
  char *stereotype; /* including << and >> */

};


#define REALIZES_WIDTH 0.1
#define REALIZES_TRIANGLESIZE 0.8
#define REALIZES_DASHLEN 0.4
#define REALIZES_FONTHEIGHT 0.8

static Font *realize_font = NULL;

static real realizes_distance_from(Realizes *realize, Point *point);
static void realizes_select(Realizes *realize, Point *clicked_point,
			      Renderer *interactive_renderer);
static void realizes_move_handle(Realizes *realize, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void realizes_move(Realizes *realize, Point *to);
static void realizes_draw(Realizes *realize, Renderer *renderer);
static Object *realizes_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void realizes_destroy(Realizes *realize);
static Object *realizes_copy(Realizes *realize);
static DiaMenu *realizes_get_object_menu(Realizes *realize,
					 Point *clickedpoint);

static RealizesState *realizes_get_state(Realizes *realize);
static void realizes_set_state(Realizes *realize,
			       RealizesState *state);
static PropDescription *realizes_describe_props(Realizes *realizes);
static void realizes_get_props(Realizes * realizes, Property *props, guint nprops);
static void realizes_set_props(Realizes * realizes, Property *props, guint nprops);

static void realizes_save(Realizes *realize, ObjectNode obj_node,
			  const char *filename);
static Object *realizes_load(ObjectNode obj_node, int version,
			     const char *filename);

static void realizes_update_data(Realizes *realize);

static ObjectTypeOps realizes_type_ops =
{
  (CreateFunc) realizes_create,
  (LoadFunc)   realizes_load,
  (SaveFunc)   realizes_save
};

ObjectType realizes_type =
{
  "UML - Realizes",   /* name */
  0,                      /* version */
  (char **) realizes_xpm,  /* pixmap */
  
  &realizes_type_ops       /* ops */
};

static ObjectOps realizes_ops = {
  (DestroyFunc)         realizes_destroy,
  (DrawFunc)            realizes_draw,
  (DistanceFunc)        realizes_distance_from,
  (SelectFunc)          realizes_select,
  (CopyFunc)            realizes_copy,
  (MoveFunc)            realizes_move,
  (MoveHandleFunc)      realizes_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      realizes_get_object_menu,
  (DescribePropsFunc)   realizes_describe_props,
  (GetPropsFunc)        realizes_get_props,
  (SetPropsFunc)        realizes_set_props
};

static PropDescription realizes_props[] = {
  OBJECT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype:"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
realizes_describe_props(Realizes *realizes)
{
  if (realizes_props[0].quark == 0)
    prop_desc_list_calculate_quarks(realizes_props);
  return realizes_props;
}

static PropOffset realizes_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Realizes, name) },
  /*{ "stereotype", PROP_TYPE_STRING, offsetof(Realizes, stereotype) },*/
  { NULL, 0, 0 }
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "stereotype" }
};

static void
realizes_get_props(Realizes * realizes, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets(&realizes->orth.object, 
                                    realizes_offsets, props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      if (strlen(realizes->stereotype) != 0)
	PROP_VALUE_STRING(props[i]) =
	  stereotype_to_string(realizes->stereotype);
      else
	PROP_VALUE_STRING(props[i]) = strdup("");	
    }
  }

}

static void
realizes_set_props(Realizes *realizes, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets(&realizes->orth.object, 
                                     realizes_offsets, props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
	quarks[i].q = g_quark_from_static_string(quarks[i].name);
    
    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);
      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_STRING) {
	g_free(realizes->stereotype);
	if (strlen(PROP_VALUE_STRING(props[i])) > 0)
	  realizes->stereotype =
	    string_to_stereotype(PROP_VALUE_STRING(props[i]));
	else 
	  realizes->stereotype = strdup("");
      }
    }
  }
  realizes_update_data(realizes);
}


static real
realizes_distance_from(Realizes *realize, Point *point)
{
  OrthConn *orth = &realize->orth;
  return orthconn_distance_from(orth, point, REALIZES_WIDTH);
}

static void
realizes_select(Realizes *realize, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&realize->orth);
}

static void
realizes_move_handle(Realizes *realize, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(realize!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&realize->orth, handle, to, reason);
  realizes_update_data(realize);
}

static void
realizes_move(Realizes *realize, Point *to)
{
  orthconn_move(&realize->orth, to);
  realizes_update_data(realize);
}

static void
realizes_draw(Realizes *realize, Renderer *renderer)
{
  OrthConn *orth = &realize->orth;
  Point *points;
  int n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, REALIZES_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_dashlength(renderer, REALIZES_DASHLEN);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  arrow_draw(renderer, ARROW_HOLLOW_TRIANGLE,
	     &points[0], &points[1],
	     REALIZES_TRIANGLESIZE, REALIZES_TRIANGLESIZE, REALIZES_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, realize_font, REALIZES_FONTHEIGHT);
  pos = realize->text_pos;
  
  if (strlen(realize->stereotype) != 0) {
    renderer->ops->draw_string(renderer,
			       realize->stereotype,
			       &pos, realize->text_align,
			       &color_black);

    pos.y += REALIZES_FONTHEIGHT;
  }
  
  if (strlen(realize->name) != 0) {
    renderer->ops->draw_string(renderer,
			       realize->name,
			       &pos, realize->text_align,
			       &color_black);
  }
  
}

static void
realizes_update_data(Realizes *realize)
{
  OrthConn *orth = &realize->orth;
  Object *obj = &orth->object;
  int num_segm, i;
  Point *points;
  Rectangle rect;
  OrthConnBBExtras *extra;

  orthconn_update_data(orth);
  
  realize->text_width = 0.0;

  realize->text_width =
    font_string_width(realize->name, realize_font, REALIZES_FONTHEIGHT);
  realize->text_width = MAX(realize->text_width,
			    font_string_width(realize->stereotype, realize_font, REALIZES_FONTHEIGHT));

  extra = &orth->extra_spacing;
  
  extra->start_trans = REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  extra->start_long = 
    extra->middle_trans = 
    extra->end_trans = 
    extra->end_long = REALIZES_WIDTH/2.0;

  orthconn_update_boundingbox(orth);
  
  /* Calc text pos: */
  num_segm = realize->orth.numpoints - 1;
  points = realize->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (realize->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (realize->orth.orientation[i]) {
  case HORIZONTAL:
    realize->text_align = ALIGN_CENTER;
    realize->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    realize->text_pos.y = points[i].y - font_descent(realize_font, REALIZES_FONTHEIGHT);
    break;
  case VERTICAL:
    realize->text_align = ALIGN_LEFT;
    realize->text_pos.x = points[i].x + 0.1;
    realize->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(realize_font, REALIZES_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = realize->text_pos.x;
  if (realize->text_align == ALIGN_CENTER)
    rect.left -= realize->text_width/2.0;
  rect.right = rect.left + realize->text_width;
  rect.top = realize->text_pos.y - font_ascent(realize_font, REALIZES_FONTHEIGHT);
  rect.bottom = rect.top + 2*REALIZES_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static ObjectChange *
realizes_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  realizes_update_data((Realizes *)obj);
  return change;
}

static ObjectChange *
realizes_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  realizes_update_data((Realizes *)obj);
  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), realizes_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), realizes_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Realizes",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
realizes_get_object_menu(Realizes *realize, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &realize->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}

static Object *
realizes_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Realizes *realize;
  OrthConn *orth;
  Object *obj;
  OrthConnBBExtras *extra;

  if (realize_font == NULL) {
    realize_font = font_getfont("Courier");
  }
  
  realize = g_malloc(sizeof(Realizes));
  orth = &realize->orth;
  obj = &orth->object;
  extra = &orth->extra_spacing;

  obj->type = &realizes_type;

  obj->ops = &realizes_ops;

  orthconn_init(orth, startpoint);
  
  realize->name = strdup("");
  realize->stereotype = strdup("");
  realize->text_width = 0;

  extra->start_trans = REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  extra->start_long = 
    extra->middle_trans = 
    extra->end_trans = 
    extra->end_long = REALIZES_WIDTH/2.0;

  realizes_update_data(realize);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &realize->orth.object;
}

static void
realizes_destroy(Realizes *realize)
{
  g_free(realize->name);
  g_free(realize->stereotype);
  orthconn_destroy(&realize->orth);
}

static Object *
realizes_copy(Realizes *realize)
{
  Realizes *newrealize;
  OrthConn *orth, *neworth;
  
  orth = &realize->orth;
  
  newrealize = g_malloc(sizeof(Realizes));
  neworth = &newrealize->orth;

  orthconn_copy(orth, neworth);

  newrealize->name = strdup(realize->name);
  newrealize->stereotype = strdup(realize->stereotype);
  newrealize->text_width = realize->text_width;
  
  realizes_update_data(newrealize);
  
  return &newrealize->orth.object;
}

static void
realizes_state_free(ObjectState *ostate)
{
  RealizesState *state = (RealizesState *)ostate;
  g_free(state->name);
  g_free(state->stereotype);
}

static RealizesState *
realizes_get_state(Realizes *realize)
{
  RealizesState *state = g_new(RealizesState, 1);

  state->obj_state.free = realizes_state_free;

  state->name = g_strdup(realize->name);
  state->stereotype = g_strdup(realize->stereotype);
  
  return state;
}

static void
realizes_set_state(Realizes *realize, RealizesState *state)
{
  g_free(realize->name);
  g_free(realize->stereotype);
  realize->name = state->name;
  realize->stereotype = state->stereotype;
  
  g_free(state);
  
  realizes_update_data(realize);
}

static void
realizes_save(Realizes *realize, ObjectNode obj_node, const char *filename)
{
  orthconn_save(&realize->orth, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  realize->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  realize->stereotype);
}

static Object *
realizes_load(ObjectNode obj_node, int version, const char *filename)
{
  Realizes *realize;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;
  OrthConnBBExtras *extra;

  if (realize_font == NULL) {
    realize_font = font_getfont("Courier");
  }

  realize = g_new(Realizes, 1);

  orth = &realize->orth;
  obj = &orth->object;
  extra = &orth->extra_spacing;

  obj->type = &realizes_type;
  obj->ops = &realizes_ops;

  orthconn_load(orth, obj_node);

  realize->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    realize->name = data_string(attribute_first_data(attr));
  else
    realize->name = strdup("");
  
  realize->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    realize->stereotype = data_string(attribute_first_data(attr));
  else
    realize->stereotype = strdup("");

  realize->text_width = 0.0;

  
  realizes_update_data(realize);

  return &realize->orth.object;
}
