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
#include <math.h>

#include "intl.h"
#include "object.h"
#include "polyshape.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/polygon.xpm"

#include "create.h"

/*
TODO:
Have connections be remembered across delete corner
Move connections correctly on delete corner
Add/remove connection points
Fix crashes:)
*/

#define DEFAULT_WIDTH 0.15

typedef struct _Polygon {
  PolyShape poly;

  Color line_color;
  LineStyle line_style;
  Color inner_color;
  gboolean show_background;
  real dashlength;
  real line_width;
} Polygon;

static struct _PolygonProperties {
  gboolean show_background;
} default_properties = { TRUE };

static ObjectChange* polygon_move_handle(Polygon *polygon, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* polygon_move(Polygon *polygon, Point *to);
static void polygon_select(Polygon *polygon, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void polygon_draw(Polygon *polygon, DiaRenderer *renderer);
static DiaObject *polygon_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real polygon_distance_from(Polygon *polygon, Point *point);
static void polygon_update_data(Polygon *polygon);
static void polygon_destroy(Polygon *polygon);
static DiaObject *polygon_copy(Polygon *polygon);

static PropDescription *polygon_describe_props(Polygon *polygon);
static void polygon_get_props(Polygon *polygon, GPtrArray *props);
static void polygon_set_props(Polygon *polygon, GPtrArray *props);

static void polygon_save(Polygon *polygon, ObjectNode obj_node,
			  const char *filename);
static DiaObject *polygon_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *polygon_get_object_menu(Polygon *polygon, Point *clickedpoint);

static ObjectTypeOps polygon_type_ops =
{
  (CreateFunc)polygon_create,   /* create */
  (LoadFunc)  polygon_load,     /* load */
  (SaveFunc)  polygon_save,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

static DiaObjectType polygon_type =
{
  "Standard - Polygon",   /* name */
  0,                         /* version */
  (char **) polygon_xpm,      /* pixmap */
  
  &polygon_type_ops,       /* ops */
  NULL, /* pixmap_file */
  0 /* default_user_data */
};

DiaObjectType *_polygon_type = (DiaObjectType *) &polygon_type;


static ObjectOps polygon_ops = {
  (DestroyFunc)         polygon_destroy,
  (DrawFunc)            polygon_draw,
  (DistanceFunc)        polygon_distance_from,
  (SelectFunc)          polygon_select,
  (CopyFunc)            polygon_copy,
  (MoveFunc)            polygon_move,
  (MoveHandleFunc)      polygon_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      polygon_get_object_menu,
  (DescribePropsFunc)   polygon_describe_props,
  (GetPropsFunc)        polygon_get_props,
  (SetPropsFunc)        polygon_set_props,
};

static PropDescription polygon_props[] = {
  POLYSHAPE_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_DESC_END
};

static PropDescription *
polygon_describe_props(Polygon *polygon)
{
  if (polygon_props[0].quark == 0)
    prop_desc_list_calculate_quarks(polygon_props);
  return polygon_props;
}

static PropOffset polygon_offsets[] = {
  POLYSHAPE_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Polygon, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Polygon, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Polygon, line_style), offsetof(Polygon, dashlength) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Polygon, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Polygon, show_background) },
  { NULL, 0, 0 }
};

static void
polygon_get_props(Polygon *polygon, GPtrArray *props)
{
  object_get_props_from_offsets(&polygon->poly.object, 
                                polygon_offsets, props);
}

static void
polygon_set_props(Polygon *polygon, GPtrArray *props)
{
  object_set_props_from_offsets(&polygon->poly.object, 
                                polygon_offsets, props);
  polygon_update_data(polygon);
}

static real
polygon_distance_from(Polygon *polygon, Point *point)
{
  return polyshape_distance_from(&polygon->poly, point, polygon->line_width);
}

static Handle *polygon_closest_handle(Polygon *polygon, Point *point) {
  return polyshape_closest_handle(&polygon->poly, point);
}

static int polygon_closest_segment(Polygon *polygon, Point *point) {
  return polyshape_closest_segment(&polygon->poly, point, polygon->line_width);
}

static void
polygon_select(Polygon *polygon, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  polyshape_update_data(&polygon->poly);
}

static ObjectChange*
polygon_move_handle(Polygon *polygon, Handle *handle,
		    Point *to, ConnectionPoint *cp,
		    HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(polygon!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  polyshape_move_handle(&polygon->poly, handle, to, cp, reason, modifiers);
  polygon_update_data(polygon);

  return NULL;
}


static ObjectChange*
polygon_move(Polygon *polygon, Point *to)
{
  polyshape_move(&polygon->poly, to);
  polygon_update_data(polygon);

  return NULL;
}

static void
polygon_draw(Polygon *polygon, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  PolyShape *poly = &polygon->poly;
  Point *points;
  int n;
  
  points = &poly->points[0];
  n = poly->numpoints;

  renderer_ops->set_linewidth(renderer, polygon->line_width);
  renderer_ops->set_linestyle(renderer, polygon->line_style);
  renderer_ops->set_dashlength(renderer, polygon->dashlength);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (polygon->show_background)
    renderer_ops->fill_polygon(renderer, points, n, &polygon->inner_color);

  renderer_ops->draw_polygon(renderer, points, n, &polygon->line_color);
}

static DiaObject *
polygon_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  Polygon *polygon;
  PolyShape *poly;
  DiaObject *obj;
  Point defaultx = { 1.0, 0.0 };
  Point defaulty = { 0.0, 1.0 };

  polygon = g_new0(Polygon, 1);
  poly = &polygon->poly;
  obj = &poly->object;

  obj->type = &polygon_type;
  obj->ops = &polygon_ops;

  if (user_data == NULL) {
    polyshape_init(poly, 3);

    poly->points[0] = *startpoint;
    poly->points[1] = *startpoint;
    point_add(&poly->points[1], &defaultx);
    poly->points[2] = *startpoint;
    point_add(&poly->points[2], &defaulty);
  } else {
    MultipointCreateData *pcd = (MultipointCreateData *)user_data;
    
    polyshape_init(poly, pcd->num_points);
    polyshape_set_points(poly, pcd->num_points, pcd->points);
  }

  polygon->line_width =  attributes_get_default_linewidth();
  polygon->line_color = attributes_get_foreground();
  polygon->inner_color = attributes_get_background();
  attributes_get_default_line_style(&polygon->line_style,
				    &polygon->dashlength);
  polygon->show_background = default_properties.show_background;

  polygon_update_data(polygon);

  *handle1 = poly->object.handles[0];
  *handle2 = poly->object.handles[2];
  return &polygon->poly.object;
}

static void
polygon_destroy(Polygon *polygon)
{
  polyshape_destroy(&polygon->poly);
}

static DiaObject *
polygon_copy(Polygon *polygon)
{
  Polygon *newpolygon;
  PolyShape *poly, *newpoly;
  DiaObject *newobj;

  poly = &polygon->poly;
 
  newpolygon = g_malloc0(sizeof(Polygon));
  newpoly = &newpolygon->poly;
  newobj = &newpoly->object;

  polyshape_copy(poly, newpoly);

  newpolygon->line_color = polygon->line_color;
  newpolygon->line_width = polygon->line_width;
  newpolygon->line_style = polygon->line_style;
  newpolygon->dashlength = polygon->dashlength;
  newpolygon->inner_color = polygon->inner_color;
  newpolygon->show_background = polygon->show_background;

  return &newpolygon->poly.object;
}


static void
polygon_update_data(Polygon *polygon)
{
  PolyShape *poly = &polygon->poly;
  DiaObject *obj = &poly->object;
  ElementBBExtras *extra = &poly->extra_spacing;

  polyshape_update_data(poly);
  
  extra->border_trans = polygon->line_width / 2.0;
  polyshape_update_boundingbox(poly);

  obj->position = poly->points[0];
}

static void
polygon_save(Polygon *polygon, ObjectNode obj_node,
	      const char *filename)
{
  polyshape_save(&polygon->poly, obj_node);

  if (!color_equals(&polygon->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &polygon->line_color);
  
  if (polygon->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  polygon->line_width);
  
  if (!color_equals(&polygon->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &polygon->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"),
		   polygon->show_background);

  if (polygon->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  polygon->line_style);

  if (polygon->line_style != LINESTYLE_SOLID &&
      polygon->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  polygon->dashlength);
  
}

static DiaObject *
polygon_load(ObjectNode obj_node, int version, const char *filename)
{
  Polygon *polygon;
  PolyShape *poly;
  DiaObject *obj;
  AttributeNode attr;

  polygon = g_malloc0(sizeof(Polygon));

  poly = &polygon->poly;
  obj = &poly->object;
  
  obj->type = &polygon_type;
  obj->ops = &polygon_ops;

  polyshape_load(poly, obj_node);

  polygon->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polygon->line_color);

  polygon->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    polygon->line_width = data_real(attribute_first_data(attr));

  polygon->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polygon->inner_color);
  
  polygon->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    polygon->show_background = data_boolean( attribute_first_data(attr) );

  polygon->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    polygon->line_style = data_enum(attribute_first_data(attr));

  polygon->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    polygon->dashlength = data_real(attribute_first_data(attr));

  polygon_update_data(polygon);

  return &polygon->poly.object;
}

static ObjectChange *
polygon_add_corner_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Polygon *poly = (Polygon*) obj;
  int segment;
  ObjectChange *change;
  
  segment = polygon_closest_segment(poly, clicked);
  change = polyshape_add_point(&poly->poly, segment, clicked);

  polygon_update_data(poly);
  return change;
}

static ObjectChange *
polygon_delete_corner_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Handle *handle;
  int handle_nr, i;
  Polygon *poly = (Polygon*) obj;
  ObjectChange *change;
  
  handle = polygon_closest_handle(poly, clicked);

  for (i = 0; i < obj->num_handles; i++) {
    if (handle == obj->handles[i]) break;
  }
  handle_nr = i;
  change = polyshape_remove_point(&poly->poly, handle_nr);

  polygon_update_data(poly);
  return change;
}


static DiaMenuItem polygon_menu_items[] = {
  { N_("Add Corner"), polygon_add_corner_callback, NULL, 1 },
  { N_("Delete Corner"), polygon_delete_corner_callback, NULL, 1 },
};

static DiaMenu polygon_menu = {
  "Polygon",
  sizeof(polygon_menu_items)/sizeof(DiaMenuItem),
  polygon_menu_items,
  NULL
};

static DiaMenu *
polygon_get_object_menu(Polygon *polygon, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  polygon_menu_items[0].active = 1;
  polygon_menu_items[1].active = polygon->poly.numpoints > 3;
  return &polygon_menu;
}
