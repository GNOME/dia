/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#include "poly_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"

#include "create.h"

#include "pixmaps/polyline.xpm"

#define DEFAULT_WIDTH 0.15

typedef struct _Polyline {
  PolyConn poly;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
} Polyline;


static void polyline_move_handle(Polyline *polyline, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void polyline_move(Polyline *polyline, Point *to);
static void polyline_select(Polyline *polyline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void polyline_draw(Polyline *polyline, Renderer *renderer);
static Object *polyline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real polyline_distance_from(Polyline *polyline, Point *point);
static void polyline_update_data(Polyline *polyline);
static void polyline_destroy(Polyline *polyline);
static Object *polyline_copy(Polyline *polyline);

static PropDescription *polyline_describe_props(Polyline *polyline);
static void polyline_get_props(Polyline *polyline, GPtrArray *props);
static void polyline_set_props(Polyline *polyline, GPtrArray *props);

static void polyline_save(Polyline *polyline, ObjectNode obj_node,
			  const char *filename);
static Object *polyline_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *polyline_get_object_menu(Polyline *polyline, Point *clickedpoint);

static ObjectTypeOps polyline_type_ops =
{
  (CreateFunc)polyline_create,   /* create */
  (LoadFunc)  polyline_load,     /* load */
  (SaveFunc)  polyline_save,      /* save */
  (GetDefaultsFunc)   NULL /*polyline_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*polyline_apply_defaults*/
};

static ObjectType polyline_type =
{
  "Standard - PolyLine",   /* name */
  0,                         /* version */
  (char **) polyline_xpm,      /* pixmap */
  
  &polyline_type_ops       /* ops */
};

ObjectType *_polyline_type = (ObjectType *) &polyline_type;


static ObjectOps polyline_ops = {
  (DestroyFunc)         polyline_destroy,
  (DrawFunc)            polyline_draw,
  (DistanceFunc)        polyline_distance_from,
  (SelectFunc)          polyline_select,
  (CopyFunc)            polyline_copy,
  (MoveFunc)            polyline_move,
  (MoveHandleFunc)      polyline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      polyline_get_object_menu,
  (DescribePropsFunc)   polyline_describe_props,
  (GetPropsFunc)        polyline_get_props,
  (SetPropsFunc)        polyline_set_props,
};

static PropDescription polyline_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_DESC_END
};

static PropDescription *
polyline_describe_props(Polyline *polyline)
{
  if (polyline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(polyline_props);
  return polyline_props;
}

static PropOffset polyline_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Polyline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Polyline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Polyline, line_style), offsetof(Polyline, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Polyline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Polyline, end_arrow) },
  { NULL, 0, 0 }
};

static void
polyline_get_props(Polyline *polyline, GPtrArray *props)
{
  object_get_props_from_offsets(&polyline->poly.object, polyline_offsets,
				props);
}

static void
polyline_set_props(Polyline *polyline, GPtrArray *props)
{
  object_set_props_from_offsets(&polyline->poly.object, polyline_offsets,
				props);
  polyline_update_data(polyline);
}

static real
polyline_distance_from(Polyline *polyline, Point *point)
{
  PolyConn *poly = &polyline->poly;
  return polyconn_distance_from(poly, point, polyline->line_width);
}

static Handle *polyline_closest_handle(Polyline *polyline, Point *point) {
  return polyconn_closest_handle(&polyline->poly, point);
}

static int polyline_closest_segment(Polyline *polyline, Point *point) {
  PolyConn *poly = &polyline->poly;
  return polyconn_closest_segment(poly, point, polyline->line_width);
}

static void
polyline_select(Polyline *polyline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  polyconn_update_data(&polyline->poly);
}

static void
polyline_move_handle(Polyline *polyline, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(polyline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  polyconn_move_handle(&polyline->poly, handle, to, reason);
  polyline_update_data(polyline);
}


static void
polyline_move(Polyline *polyline, Point *to)
{
  polyconn_move(&polyline->poly, to);
  polyline_update_data(polyline);
}

static void
polyline_draw(Polyline *polyline, Renderer *renderer)
{
  PolyConn *poly = &polyline->poly;
  Point *points;
  int n;
  
  points = &poly->points[0];
  n = poly->numpoints;

  renderer->ops->set_linewidth(renderer, polyline->line_width);
  renderer->ops->set_linestyle(renderer, polyline->line_style);
  renderer->ops->set_dashlength(renderer, polyline->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline_with_arrows(renderer,
					   points, n,
					   polyline->line_width,
					   &polyline->line_color,
					   &polyline->start_arrow,
					   &polyline->end_arrow);
}

/** user_data is a struct polyline_create_data, containing an array of 
    points and a count.
    If user_data is NULL, the startpoint is used and a 1x1 line is created.
    Otherwise, the startpoint is ignored.
*/
static Object *
polyline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Polyline *polyline;
  PolyConn *poly;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  /*polyline_init_defaults();*/
  polyline = g_malloc0(sizeof(Polyline));
  poly = &polyline->poly;
  obj = &poly->object;
  
  obj->type = &polyline_type;
  obj->ops = &polyline_ops;

  if (user_data == NULL) {
    polyconn_init(poly, 2);

    poly->points[0] = *startpoint;
    poly->points[1] = *startpoint;

    point_add(&poly->points[1], &defaultlen);

    *handle1 = poly->object.handles[0];
    *handle2 = poly->object.handles[1];
  } else {
    PolylineCreateData *pcd = (PolylineCreateData *)user_data;

    polyconn_init(poly, pcd->num_points);

    /* Handles are set up by polyconn_init and polyconn_update_data */
    polyconn_set_points(poly, pcd->num_points, pcd->points);

    *handle1 = poly->object.handles[0];
    *handle2 = poly->object.handles[pcd->num_points-1];
  }
  
  polyline_update_data(polyline);

  polyline->line_width =  attributes_get_default_linewidth();
  polyline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&polyline->line_style,
				    &polyline->dashlength);
  polyline->start_arrow = attributes_get_default_start_arrow();
  polyline->end_arrow = attributes_get_default_end_arrow();

  return &polyline->poly.object;
}

static void
polyline_destroy(Polyline *polyline)
{
  polyconn_destroy(&polyline->poly);
}

static Object *
polyline_copy(Polyline *polyline)
{
  Polyline *newpolyline;
  PolyConn *poly, *newpoly;
  Object *newobj;
  
  poly = &polyline->poly;
 
  newpolyline = g_malloc0(sizeof(Polyline));
  newpoly = &newpolyline->poly;
  newobj = &newpoly->object;

  polyconn_copy(poly, newpoly);

  newpolyline->line_color = polyline->line_color;
  newpolyline->line_width = polyline->line_width;
  newpolyline->line_style = polyline->line_style;
  newpolyline->dashlength = polyline->dashlength;
  newpolyline->start_arrow = polyline->start_arrow;
  newpolyline->end_arrow = polyline->end_arrow;

  return &newpolyline->poly.object;
}

static void
polyline_update_data(Polyline *polyline)
{
  PolyConn *poly = &polyline->poly;
  Object *obj = &poly->object;
  PolyBBExtras *extra = &poly->extra_spacing;

  polyconn_update_data(&polyline->poly);

  extra->start_trans =  (polyline->line_width / 2.0);
  extra->end_trans =     (polyline->line_width / 2.0);
  extra->middle_trans = (polyline->line_width / 2.0);
    
  if (polyline->start_arrow.type != ARROW_NONE) 
    extra->start_trans = MAX(extra->start_trans,polyline->start_arrow.width);
  if (polyline->end_arrow.type != ARROW_NONE) 
    extra->end_trans = MAX(extra->end_trans,polyline->end_arrow.width);

  extra->start_long = (polyline->line_width / 2.0);
  extra->end_long   = (polyline->line_width / 2.0);

  polyconn_update_boundingbox(poly);

  obj->position = poly->points[0];
}

static void
polyline_save(Polyline *polyline, ObjectNode obj_node,
	      const char *filename)
{
  polyconn_save(&polyline->poly, obj_node);

  if (!color_equals(&polyline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &polyline->line_color);
  
  if (polyline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  polyline->line_width);
  
  if (polyline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  polyline->line_style);

  if (polyline->line_style != LINESTYLE_SOLID &&
      polyline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  polyline->dashlength);
  
  if (polyline->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  polyline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  polyline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  polyline->start_arrow.width);
  }
  
  if (polyline->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  polyline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  polyline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  polyline->end_arrow.width);
  }
}

static Object *
polyline_load(ObjectNode obj_node, int version, const char *filename)
{
  Polyline *polyline;
  PolyConn *poly;
  Object *obj;
  AttributeNode attr;

  polyline = g_malloc0(sizeof(Polyline));

  poly = &polyline->poly;
  obj = &poly->object;
  
  obj->type = &polyline_type;
  obj->ops = &polyline_ops;

  polyconn_load(poly, obj_node);

  polyline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polyline->line_color);

  polyline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    polyline->line_width = data_real(attribute_first_data(attr));

  polyline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    polyline->line_style = data_enum(attribute_first_data(attr));

  polyline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    polyline->dashlength = data_real(attribute_first_data(attr));

  polyline->start_arrow.type = ARROW_NONE;
  polyline->start_arrow.length = 0.8;
  polyline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    polyline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    polyline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    polyline->start_arrow.width = data_real(attribute_first_data(attr));

  polyline->end_arrow.type = ARROW_NONE;
  polyline->end_arrow.length = 0.8;
  polyline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    polyline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    polyline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    polyline->end_arrow.width = data_real(attribute_first_data(attr));

  polyline_update_data(polyline);

  return &polyline->poly.object;
}

static ObjectChange *
polyline_add_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Polyline *poly = (Polyline*) obj;
  int segment;
  ObjectChange *change;

  segment = polyline_closest_segment(poly, clicked);
  change = polyconn_add_point(&poly->poly, segment, clicked);
  polyline_update_data(poly);
  return change;
}

static ObjectChange *
polyline_delete_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Handle *handle;
  int handle_nr, i;
  Polyline *poly = (Polyline*) obj;
  ObjectChange *change;
  
  handle = polyline_closest_handle(poly, clicked);

  for (i = 0; i < obj->num_handles; i++) {
    if (handle == obj->handles[i]) break;
  }
  handle_nr = i;
  change = polyconn_remove_point(&poly->poly, handle_nr);
  polyline_update_data(poly);
  return change;
}


static DiaMenuItem polyline_menu_items[] = {
  { N_("Add Corner"), polyline_add_corner_callback, NULL, 1 },
  { N_("Delete Corner"), polyline_delete_corner_callback, NULL, 1 },
};

static DiaMenu polyline_menu = {
  "Polyline",
  sizeof(polyline_menu_items)/sizeof(DiaMenuItem),
  polyline_menu_items,
  NULL
};

static DiaMenu *
polyline_get_object_menu(Polyline *polyline, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  polyline_menu_items[0].active = 1;
  polyline_menu_items[1].active = polyline->poly.numpoints > 2;
  return &polyline_menu;
}
