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

#include "pixmaps/dependency.xpm"

typedef struct _Dependency Dependency;

struct _Dependency {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  int draw_arrow;
  char *name;
  char *stereotype; /* including << and >> */

};


#define DEPENDENCY_WIDTH 0.1
#define DEPENDENCY_ARROWLEN 0.8
#define DEPENDENCY_ARROWWIDTH 0.5
#define DEPENDENCY_DASHLEN 0.4
#define DEPENDENCY_FONTHEIGHT 0.8

static Font *dep_font = NULL;

static real dependency_distance_from(Dependency *dep, Point *point);
static void dependency_select(Dependency *dep, Point *clicked_point,
			      Renderer *interactive_renderer);
static void dependency_move_handle(Dependency *dep, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void dependency_move(Dependency *dep, Point *to);
static void dependency_draw(Dependency *dep, Renderer *renderer);
static Object *dependency_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void dependency_destroy(Dependency *dep);
static Object *dependency_copy(Dependency *dep);
static DiaMenu *dependency_get_object_menu(Dependency *dep,
					   Point *clickedpoint);

static void dependency_save(Dependency *dep, ObjectNode obj_node,
			    const char *filename);
static Object *dependency_load(ObjectNode obj_node, int version,
			       const char *filename);
static PropDescription *dependency_describe_props(Dependency *dependency);
static void dependency_get_props(Dependency * dependency, Property *props, guint nprops);
static void dependency_set_props(Dependency * dependency, Property *props, guint nprops);

static void dependency_update_data(Dependency *dep);

static ObjectTypeOps dependency_type_ops =
{
  (CreateFunc) dependency_create,
  (LoadFunc)   dependency_load,
  (SaveFunc)   dependency_save
};

ObjectType dependency_type =
{
  "UML - Dependency",   /* name */
  0,                      /* version */
  (char **) dependency_xpm,  /* pixmap */
  
  &dependency_type_ops       /* ops */
};

static ObjectOps dependency_ops = {
  (DestroyFunc)         dependency_destroy,
  (DrawFunc)            dependency_draw,
  (DistanceFunc)        dependency_distance_from,
  (SelectFunc)          dependency_select,
  (CopyFunc)            dependency_copy,
  (MoveFunc)            dependency_move,
  (MoveHandleFunc)      dependency_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      dependency_get_object_menu,
  (DescribePropsFunc)   dependency_describe_props,
  (GetPropsFunc)        dependency_get_props,
  (SetPropsFunc)        dependency_set_props
};

static PropDescription dependency_props[] = {
  OBJECT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype:"), NULL, NULL },
  { "show_arrow", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Show arrow:"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
dependency_describe_props(Dependency *dependency)
{
  if (dependency_props[0].quark == 0)
    prop_desc_list_calculate_quarks(dependency_props);
  return dependency_props;
}

static PropOffset dependency_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Dependency, name) },
  /*{ "stereotype", PROP_TYPE_STRING, offsetof(Dependency, stereotype) },*/
  { "show_arrow", PROP_TYPE_BOOL, offsetof(Dependency, draw_arrow) },
  { NULL, 0, 0 }
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "stereotype" }
};

static void
dependency_get_props(Dependency * dependency, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets(&dependency->orth.object, 
                                    dependency_offsets, props, nprops))
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
      if (dependency->stereotype && dependency->stereotype[0] != '\0')
	PROP_VALUE_STRING(props[i]) =
	  stereotype_to_string(dependency->stereotype);
      else
	PROP_VALUE_STRING(props[i]) = NULL;
    }
  }

}

static void
dependency_set_props(Dependency *dependency, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets(&dependency->orth.object, 
                                     dependency_offsets, props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
	quarks[i].q = g_quark_from_static_string(quarks[i].name);
    
    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);
      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_STRING) {
	g_free(dependency->stereotype);
	dependency->stereotype = NULL;
	if (PROP_VALUE_STRING(props[i]) &&
	    PROP_VALUE_STRING(props[i])[0] != '\0')
	  dependency->stereotype =
	    string_to_stereotype(PROP_VALUE_STRING(props[i]));
      }
    }
  }
  dependency_update_data(dependency);
}

static real
dependency_distance_from(Dependency *dep, Point *point)
{
  OrthConn *orth = &dep->orth;
  return orthconn_distance_from(orth, point, DEPENDENCY_WIDTH);
}

static void
dependency_select(Dependency *dep, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&dep->orth);
}

static void
dependency_move_handle(Dependency *dep, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(dep!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&dep->orth, handle, to, reason);
  dependency_update_data(dep);
}

static void
dependency_move(Dependency *dep, Point *to)
{
  orthconn_move(&dep->orth, to);
  dependency_update_data(dep);
}

static void
dependency_draw(Dependency *dep, Renderer *renderer)
{
  OrthConn *orth = &dep->orth;
  Point *points;
  int n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, DEPENDENCY_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_dashlength(renderer, DEPENDENCY_DASHLEN);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  if (dep->draw_arrow)
    arrow_draw(renderer, ARROW_LINES,
	       &points[n-1], &points[n-2],
	       DEPENDENCY_ARROWLEN, DEPENDENCY_ARROWWIDTH, DEPENDENCY_WIDTH,
	       &color_black, &color_white);

  renderer->ops->set_font(renderer, dep_font, DEPENDENCY_FONTHEIGHT);
  pos = dep->text_pos;
  
  if (dep->stereotype != NULL && dep->stereotype[0] != '\0') {
    renderer->ops->draw_string(renderer,
			       dep->stereotype,
			       &pos, dep->text_align,
			       &color_black);

    pos.y += DEPENDENCY_FONTHEIGHT;
  }
  
  if (dep->name != NULL && dep->name[0] != '\0') {
    renderer->ops->draw_string(renderer,
			       dep->name,
			       &pos, dep->text_align,
			       &color_black);
  }
  
}

static void
dependency_update_data(Dependency *dep)
{
  OrthConn *orth = &dep->orth;
  Object *obj = &orth->object;
  OrthConnBBExtras *extra = &orth->extra_spacing;

  int num_segm, i;
  Point *points;
  Rectangle rect;
  
  orthconn_update_data(orth);

  dep->text_width = 0.0;
  if (dep->name)
    dep->text_width = font_string_width(dep->name, dep_font,
					DEPENDENCY_FONTHEIGHT);
  if (dep->stereotype)
    dep->text_width = MAX(dep->text_width,
			  font_string_width(dep->stereotype, dep_font,
					    DEPENDENCY_FONTHEIGHT));
  
  extra->start_trans = 
    extra->start_long = 
    extra->middle_trans = DEPENDENCY_WIDTH/2.0;
  
  extra->end_trans = 
    extra->end_long = (dep->draw_arrow?
                       (DEPENDENCY_WIDTH + DEPENDENCY_ARROWLEN)/2.0:
                       DEPENDENCY_WIDTH/2.0);

  orthconn_update_boundingbox(orth);
  
  /* Calc text pos: */
  num_segm = dep->orth.numpoints - 1;
  points = dep->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (dep->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (dep->orth.orientation[i]) {
  case HORIZONTAL:
    dep->text_align = ALIGN_CENTER;
    dep->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    dep->text_pos.y = points[i].y - font_descent(dep_font, DEPENDENCY_FONTHEIGHT);
    break;
  case VERTICAL:
    dep->text_align = ALIGN_LEFT;
    dep->text_pos.x = points[i].x + 0.1;
    dep->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(dep_font, DEPENDENCY_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = dep->text_pos.x;
  if (dep->text_align == ALIGN_CENTER)
    rect.left -= dep->text_width/2.0;
  rect.right = rect.left + dep->text_width;
  rect.top = dep->text_pos.y - font_ascent(dep_font, DEPENDENCY_FONTHEIGHT);
  rect.bottom = rect.top + 2*DEPENDENCY_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static ObjectChange *
dependency_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  dependency_update_data((Dependency *)obj);
  return change;
}

static ObjectChange *
dependency_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  dependency_update_data((Dependency *)obj);
  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), dependency_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), dependency_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Dependency",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
dependency_get_object_menu(Dependency *dep, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &dep->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}

static Object *
dependency_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Dependency *dep;
  OrthConn *orth;
  Object *obj;

  if (dep_font == NULL) {
    dep_font = font_getfont("Courier");
  }
  
  dep = g_new0(Dependency, 1);
  orth = &dep->orth;
  obj = (Object *)dep;
  
  obj->type = &dependency_type;

  obj->ops = &dependency_ops;

  orthconn_init(orth, startpoint);

  dep->draw_arrow = TRUE;
  dep->name = NULL;
  dep->stereotype = NULL;
  dep->text_width = 0;

  dependency_update_data(dep);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return (Object *)dep;
}

static void
dependency_destroy(Dependency *dep)
{
  g_free(dep->name);
  g_free(dep->stereotype);
  
  orthconn_destroy(&dep->orth);
}

static Object *
dependency_copy(Dependency *dep)
{
  Dependency *newdep;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &dep->orth;
  
  newdep = g_new0(Dependency, 1);
  neworth = &newdep->orth;
  newobj = (Object *)newdep;

  orthconn_copy(orth, neworth);

  newdep->name = dep->name ? g_strdup(dep->name) : NULL;
  newdep->stereotype = dep->stereotype ? g_strdup(dep->stereotype) : NULL;
  newdep->draw_arrow = dep->draw_arrow;
  newdep->text_width = dep->text_width;
  
  dependency_update_data(newdep);
  
  return &newdep->orth.object;
}

static void
dependency_save(Dependency *dep, ObjectNode obj_node, const char *filename)
{
  orthconn_save(&dep->orth, obj_node);

  data_add_boolean(new_attribute(obj_node, "draw_arrow"),
		   dep->draw_arrow);
  data_add_string(new_attribute(obj_node, "name"),
		  dep->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  dep->stereotype);
}

static Object *
dependency_load(ObjectNode obj_node, int version, const char *filename)
{
  AttributeNode attr;
  Dependency *dep;
  OrthConn *orth;
  Object *obj;

  if (dep_font == NULL) {
    dep_font = font_getfont("Courier");
  }

  dep = g_new0(Dependency, 1);

  orth = &dep->orth;
  obj = (Object *)dep;

  obj->type = &dependency_type;
  obj->ops = &dependency_ops;

  orthconn_load(orth, obj_node);

  attr = object_find_attribute(obj_node, "draw_arrow");
  if (attr != NULL)
    dep->draw_arrow = data_boolean(attribute_first_data(attr));

  dep->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    dep->name = data_string(attribute_first_data(attr));
  
  dep->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    dep->stereotype = data_string(attribute_first_data(attr));

  dependency_update_data(dep);

  return &dep->orth.object;
}
