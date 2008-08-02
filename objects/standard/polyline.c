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
#include "poly_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"

#include "create.h"

#include "tool-icons.h"

#define DEFAULT_WIDTH 0.15

typedef struct _Polyline {
  PolyConn poly;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  real corner_radius;
  Arrow start_arrow, end_arrow;
  real absolute_start_gap, absolute_end_gap;
} Polyline;


static ObjectChange* polyline_move_handle(Polyline *polyline, Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* polyline_move(Polyline *polyline, Point *to);
static void polyline_select(Polyline *polyline, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void polyline_draw(Polyline *polyline, DiaRenderer *renderer);
static DiaObject *polyline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real polyline_distance_from(Polyline *polyline, Point *point);
static void polyline_update_data(Polyline *polyline);
static void polyline_destroy(Polyline *polyline);
static DiaObject *polyline_copy(Polyline *polyline);

static PropDescription *polyline_describe_props(Polyline *polyline);
static void polyline_get_props(Polyline *polyline, GPtrArray *props);
static void polyline_set_props(Polyline *polyline, GPtrArray *props);

static void polyline_save(Polyline *polyline, ObjectNode obj_node,
			  const char *filename);
static DiaObject *polyline_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *polyline_get_object_menu(Polyline *polyline, Point *clickedpoint);
void polyline_calculate_gap_endpoints(Polyline *polyline, Point *gap_endpoints);
static void polyline_exchange_gap_points(Polyline *polyline,  Point *gap_points);

static ObjectTypeOps polyline_type_ops =
{
  (CreateFunc)polyline_create,   /* create */
  (LoadFunc)  polyline_load,     /* load */
  (SaveFunc)  polyline_save,      /* save */
  (GetDefaultsFunc)   NULL /*polyline_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*polyline_apply_defaults*/
};

static DiaObjectType polyline_type =
{
  "Standard - PolyLine",   /* name */
  0,                         /* version */
  (char **) polyline_icon,      /* pixmap */
  
  &polyline_type_ops,       /* ops */
  NULL, /* pixmap_file */
  0 /* default_user_data */
};

DiaObjectType *_polyline_type = (DiaObjectType *) &polyline_type;


static ObjectOps polyline_ops = {
  (DestroyFunc)         polyline_destroy,
  (DrawFunc)            polyline_draw,
  (DistanceFunc)        polyline_distance_from,
  (SelectFunc)          polyline_select,
  (CopyFunc)            polyline_copy,
  (MoveFunc)            polyline_move,
  (MoveHandleFunc)      polyline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      polyline_get_object_menu,
  (DescribePropsFunc)   polyline_describe_props,
  (GetPropsFunc)        polyline_get_props,
  (SetPropsFunc)        polyline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData polyline_corner_radius_data = { 0.0, 10.0, 0.1 };
static PropNumData gap_range = { -G_MAXFLOAT, G_MAXFLOAT, 0.1};

static PropDescription polyline_props[] = {
  POLYCONN_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &polyline_corner_radius_data },
  PROP_FRAME_BEGIN("gaps",0,N_("Line gaps")),
  { "absolute_start_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Absolute start gap"), NULL, &gap_range },
  { "absolute_end_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Absolute end gap"), NULL, &gap_range },
  PROP_FRAME_END("gaps",0),
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
  POLYCONN_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Polyline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Polyline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Polyline, line_style), offsetof(Polyline, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Polyline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Polyline, end_arrow) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Polyline, corner_radius) },
  PROP_OFFSET_FRAME_BEGIN("gaps"),
  { "absolute_start_gap", PROP_TYPE_REAL, offsetof(Polyline, absolute_start_gap) },
  { "absolute_end_gap", PROP_TYPE_REAL, offsetof(Polyline, absolute_end_gap) },
  PROP_OFFSET_FRAME_END("gaps"),
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
  real dist;
  Point gap_endpoints[2];
  polyline_calculate_gap_endpoints(polyline, gap_endpoints);
  polyline_exchange_gap_points(polyline, gap_endpoints);
  dist = polyconn_distance_from(poly, point, polyline->line_width);
  polyline_exchange_gap_points(polyline, gap_endpoints);
  return dist;
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
		  DiaRenderer *interactive_renderer)
{
  polyconn_update_data(&polyline->poly);
}

static ObjectChange*
polyline_move_handle(Polyline *polyline, Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(polyline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  polyconn_move_handle(&polyline->poly, handle, to, cp, reason, modifiers);
  polyline_update_data(polyline);

  return NULL;
}


static ObjectChange*
polyline_move(Polyline *polyline, Point *to)
{
  polyconn_move(&polyline->poly, to);
  polyline_update_data(polyline);

  return NULL;
}

void
polyline_calculate_gap_endpoints(Polyline *polyline, Point *gap_endpoints)
{
  Point  start_vec, end_vec;
  ConnectionPoint *start_cp, *end_cp;
  int n = polyline->poly.numpoints;

  gap_endpoints[0] = polyline->poly.points[0];
  gap_endpoints[1] = polyline->poly.points[n-1];

  start_cp = (polyline->poly.object.handles[0])->connected_to;
  end_cp = (polyline->poly.object.handles[polyline->poly.object.num_handles-1])->connected_to;

  if (connpoint_is_autogap(start_cp)) {
      gap_endpoints[0] = calculate_object_edge(&gap_endpoints[0],
					   &polyline->poly.points[1],
					   start_cp->object);
  }
  if (connpoint_is_autogap(end_cp)) {    
      gap_endpoints[1] = calculate_object_edge(&gap_endpoints[1],
					   &polyline->poly.points[n-2],
					   end_cp->object);
  }

  start_vec = gap_endpoints[0];
  point_sub(&start_vec, &polyline->poly.points[0]);
  point_normalize(&start_vec);

  end_vec = gap_endpoints[1];
  point_sub(&end_vec, &polyline->poly.points[n-1]);
  point_normalize(&end_vec);
  
  /* add absolute gap */
  point_add_scaled(&gap_endpoints[0], &start_vec, polyline->absolute_start_gap);
  point_add_scaled(&gap_endpoints[1], &end_vec, polyline->absolute_end_gap);
}
static void
polyline_exchange_gap_points(Polyline *polyline,  Point *gap_points)
{
        Point tmp[2];
        int n = polyline->poly.numpoints;
        tmp[0] = gap_points[0];
        tmp[1] = gap_points[1];
        gap_points[0] = polyline->poly.points[0];
        gap_points[1] = polyline->poly.points[n-1];
        polyline->poly.points[0] = tmp[0];
        polyline->poly.points[n-1] = tmp[1];
}

        
static void
polyline_draw(Polyline *polyline, DiaRenderer *renderer)
{
  Point gap_endpoints[2];
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  PolyConn *poly = &polyline->poly;
  Point *points;
  int n;
  
  points = &poly->points[0];
  n = poly->numpoints;
  renderer_ops->set_linewidth(renderer, polyline->line_width);
  renderer_ops->set_linestyle(renderer, polyline->line_style);
  renderer_ops->set_dashlength(renderer, polyline->dashlength);
  if (polyline->corner_radius > 0.0)
    renderer_ops->set_linejoin(renderer, LINEJOIN_ROUND);
  else
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  polyline_calculate_gap_endpoints(polyline, gap_endpoints);
  polyline_exchange_gap_points(polyline, gap_endpoints);
  renderer_ops->draw_rounded_polyline_with_arrows(renderer,
						  points, n,
						  polyline->line_width,
						  &polyline->line_color,
						  &polyline->start_arrow,
						  &polyline->end_arrow,
						  polyline->corner_radius);
  polyline_exchange_gap_points(polyline, gap_endpoints);
}

/** user_data is a struct polyline_create_data, containing an array of 
    points and a count.
    If user_data is NULL, the startpoint is used and a 1x1 line is created.
    Otherwise, the startpoint is ignored.
*/
static DiaObject *
polyline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Polyline *polyline;
  PolyConn *poly;
  DiaObject *obj;
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
    MultipointCreateData *pcd = (MultipointCreateData *)user_data;

    polyconn_init(poly, pcd->num_points);

    /* Handles are set up by polyconn_init and polyconn_update_data */
    polyconn_set_points(poly, pcd->num_points, pcd->points);

    *handle1 = poly->object.handles[0];
    *handle2 = poly->object.handles[pcd->num_points-1];
  }


  polyline->line_width =  attributes_get_default_linewidth();
  polyline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&polyline->line_style,
				    &polyline->dashlength);
  polyline->start_arrow = attributes_get_default_start_arrow();
  polyline->end_arrow = attributes_get_default_end_arrow();
  polyline->corner_radius = 0.0;

  polyline_update_data(polyline);

  return &polyline->poly.object;
}

static void
polyline_destroy(Polyline *polyline)
{
  polyconn_destroy(&polyline->poly);
}

static DiaObject *
polyline_copy(Polyline *polyline)
{
  Polyline *newpolyline;
  PolyConn *poly, *newpoly;
  DiaObject *newobj;
  
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
  newpolyline->corner_radius = polyline->corner_radius;
  newpolyline->absolute_start_gap = polyline->absolute_start_gap;
  newpolyline->absolute_end_gap = polyline->absolute_end_gap;

  polyline_update_data(newpolyline);

  return &newpolyline->poly.object;
}

static void
polyline_update_data(Polyline *polyline)
{
  PolyConn *poly = &polyline->poly;
  DiaObject *obj = &poly->object;
  PolyBBExtras *extra = &poly->extra_spacing;
  Point gap_endpoints[2];

  polyconn_update_data(&polyline->poly);

  extra->start_trans =  (polyline->line_width / 2.0);
  extra->end_trans =     (polyline->line_width / 2.0);
  extra->middle_trans = (polyline->line_width / 2.0);
  extra->start_long = (polyline->line_width / 2.0);
  extra->end_long   = (polyline->line_width / 2.0);

  polyline_calculate_gap_endpoints(polyline, gap_endpoints);
  polyline_exchange_gap_points(polyline, gap_endpoints);

  polyconn_update_boundingbox(poly);

  if (polyline->start_arrow.type != ARROW_NONE) {
    Rectangle bbox;
    Point move_arrow, move_line;
    Point to = gap_endpoints[0];
    Point from = poly->points[1];
    calculate_arrow_point(&polyline->start_arrow, &to, &from,
                          &move_arrow, &move_line, polyline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&polyline->start_arrow, polyline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
  if (polyline->end_arrow.type != ARROW_NONE) {
    Rectangle bbox;
    int n = polyline->poly.numpoints;
    Point move_arrow, move_line;
    Point to = gap_endpoints[1];
    Point from = poly->points[n-2];
    calculate_arrow_point(&polyline->start_arrow, &to, &from,
                          &move_arrow, &move_line, polyline->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&polyline->end_arrow, polyline->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }

  polyline_exchange_gap_points(polyline, gap_endpoints);

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
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  polyline->line_width);
  
  if (polyline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  polyline->line_style);

  if (polyline->line_style != LINESTYLE_SOLID &&
      polyline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  polyline->dashlength);
  
  if (polyline->start_arrow.type != ARROW_NONE) {
    save_arrow(obj_node, &polyline->start_arrow, "start_arrow",
	     "start_arrow_length", "start_arrow_width");
  }

  if (polyline->end_arrow.type != ARROW_NONE) {
    save_arrow(obj_node, &polyline->end_arrow, "end_arrow",
	     "end_arrow_length", "end_arrow_width");
  }

  if (polyline->absolute_start_gap)
    data_add_real(new_attribute(obj_node, "absolute_start_gap"),
                 polyline->absolute_start_gap);
  if (polyline->absolute_end_gap)
    data_add_real(new_attribute(obj_node, "absolute_end_gap"),
                 polyline->absolute_end_gap);

  if (polyline->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
                  polyline->corner_radius);
}

static DiaObject *
polyline_load(ObjectNode obj_node, int version, const char *filename)
{
  Polyline *polyline;
  PolyConn *poly;
  DiaObject *obj;
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
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
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

  load_arrow(obj_node, &polyline->start_arrow, "start_arrow",
	     "start_arrow_length", "start_arrow_width");

  load_arrow(obj_node, &polyline->end_arrow, "end_arrow",
	     "end_arrow_length", "end_arrow_width");

  polyline->absolute_start_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_start_gap");
  if (attr != NULL)
    polyline->absolute_start_gap =  data_real( attribute_first_data(attr) );
  polyline->absolute_end_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_end_gap");
  if (attr != NULL)
    polyline->absolute_end_gap =  data_real( attribute_first_data(attr) );


  polyline->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    polyline->corner_radius =  data_real( attribute_first_data(attr) );

  polyline_update_data(polyline);

  return &polyline->poly.object;
}

static ObjectChange *
polyline_add_corner_callback (DiaObject *obj, Point *clicked, gpointer data)
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
polyline_delete_corner_callback (DiaObject *obj, Point *clicked, gpointer data)
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
