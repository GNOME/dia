/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * Bezier object  Copyright (C) 1999 Lars R. Clausen
 * Conversion to use BezierConn by James Henstridge.
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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "bezier_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/bezier.xpm"

#define DEFAULT_WIDTH 0.15

typedef struct _Bezierline Bezierline;

struct _Bezierline {
  BezierConn bez;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
};


static void bezierline_move_handle(Bezierline *bezierline, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void bezierline_move(Bezierline *bezierline, Point *to);
static void bezierline_select(Bezierline *bezierline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void bezierline_draw(Bezierline *bezierline, Renderer *renderer);
static Object *bezierline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real bezierline_distance_from(Bezierline *bezierline, Point *point);
static void bezierline_update_data(Bezierline *bezierline);
static void bezierline_destroy(Bezierline *bezierline);
static Object *bezierline_copy(Bezierline *bezierline);

static PropDescription *bezierline_describe_props(Bezierline *bezierline);
static void bezierline_get_props(Bezierline *bezierline,
				 Property *props, guint nprops);
static void bezierline_set_props(Bezierline *bezierline,
				 Property *props, guint nprops);

static void bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
			  const char *filename);
static Object *bezierline_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint);
/* static GtkWidget *bezierline_get_defaults();
   static void bezierline_apply_defaults(); */

static ObjectTypeOps bezierline_type_ops =
{
  (CreateFunc)bezierline_create,   /* create */
  (LoadFunc)  bezierline_load,     /* load */
  (SaveFunc)  bezierline_save,      /* save */
  (GetDefaultsFunc)   NULL /*bezierline_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*bezierline_apply_defaults*/
};

static ObjectType bezierline_type =
{
  "Standard - BezierLine",   /* name */
  0,                         /* version */
  (char **) bezier_xpm,      /* pixmap */
  
  &bezierline_type_ops       /* ops */
};

ObjectType *_bezierline_type = (ObjectType *) &bezierline_type;


static ObjectOps bezierline_ops = {
  (DestroyFunc)         bezierline_destroy,
  (DrawFunc)            bezierline_draw,
  (DistanceFunc)        bezierline_distance_from,
  (SelectFunc)          bezierline_select,
  (CopyFunc)            bezierline_copy,
  (MoveFunc)            bezierline_move,
  (MoveHandleFunc)      bezierline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      bezierline_get_object_menu,
  (DescribePropsFunc)   bezierline_describe_props,
  (GetPropsFunc)        bezierline_get_props,
  (SetPropsFunc)        bezierline_set_props,
};

static PropDescription bezierline_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_DESC_END
};

static PropDescription *
bezierline_describe_props(Bezierline *bezierline)
{
  if (bezierline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(bezierline_props);
  return bezierline_props;
}

static PropOffset bezierline_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Bezierline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Bezierline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Bezierline, line_style), offsetof(Bezierline, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Bezierline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Bezierline, end_arrow) },
  { NULL, 0, 0 }
};

static void
bezierline_get_props(Bezierline *bezierline, Property *props, guint nprops)
{
  object_get_props_from_offsets((Object *)bezierline, bezierline_offsets,
				props, nprops);
}

static void
bezierline_set_props(Bezierline *bezierline, Property *props, guint nprops)
{
  object_set_props_from_offsets((Object *)bezierline, bezierline_offsets,
				props, nprops);
  bezierline_update_data(bezierline);
}

static real
bezierline_distance_from(Bezierline *bezierline, Point *point)
{
  BezierConn *bez = &bezierline->bez;
  return bezierconn_distance_from(bez, point, bezierline->line_width);
}

static Handle *bezierline_closest_handle(Bezierline *bezierline, Point *point) {
  return bezierconn_closest_handle(&bezierline->bez, point);
}

static int bezierline_closest_segment(Bezierline *bezierline, Point *point) {
  BezierConn *bez = &bezierline->bez;
  return bezierconn_closest_segment(bez, point, bezierline->line_width);
}

static void
bezierline_select(Bezierline *bezierline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  bezierconn_update_data(&bezierline->bez);
}

static void
bezierline_move_handle(Bezierline *bezierline, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(bezierline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (reason == HANDLE_MOVE_CREATE || reason == HANDLE_MOVE_CREATE_FINAL) {
    /* During creation, change the control points */
    BezierConn *bez = &bezierline->bez;
    Point dist = bez->points[0].p1;

    point_sub(&dist, to);
    dist.y = 0;
    point_scale(&dist, 0.332);

    bezierconn_move_handle(bez, handle, to, reason);
    
    bez->points[1].p1 = bez->points[0].p1;
    point_sub(&bez->points[1].p1, &dist);
    bez->points[1].p2 = *to;
    point_add(&bez->points[1].p2, &dist);
  } else {
    bezierconn_move_handle(&bezierline->bez, handle, to, reason);
  }

  bezierline_update_data(bezierline);
}


static void
bezierline_move(Bezierline *bezierline, Point *to)
{
  bezierconn_move(&bezierline->bez, to);
  bezierline_update_data(bezierline);
}

static void
bezierline_draw(Bezierline *bezierline, Renderer *renderer)
{
  BezierConn *bez = &bezierline->bez;
  
  renderer->ops->set_linewidth(renderer, bezierline->line_width);
  renderer->ops->set_linestyle(renderer, bezierline->line_style);
  renderer->ops->set_dashlength(renderer, bezierline->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_bezier(renderer, bez->points, bez->numpoints,
			     &bezierline->line_color);

  if (bezierline->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->start_arrow.type,
	       &bez->points[0].p1, &bez->points[1].p1,
	       bezierline->start_arrow.length, bezierline->start_arrow.width,
	       bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }
  if (bezierline->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->end_arrow.type,
	       &bez->points[bez->numpoints-1].p3,
	       &bez->points[bez->numpoints-1].p2,
	       bezierline->end_arrow.length, bezierline->end_arrow.width,
	       bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }

  /* these lines should only be displayed when object is selected.
   * Unfortunately the draw function is not aware of the selected
   * state.  This is a compromise until I fix this properly. */
  if (renderer->is_interactive) {
    bezierconn_draw_control_lines(&bezierline->bez, renderer);
  }
}

static Object *
bezierline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Bezierline *bezierline;
  BezierConn *bez;
  Object *obj;
  Point defaultlen = { .3, .3 };

  /*bezierline_init_defaults();*/
  bezierline = g_new(Bezierline, 1);
  bez = &bezierline->bez;
  obj = (Object *) bezierline;
  
  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  bezierconn_init(bez);
  
  bez->points[0].p1 = *startpoint;
  bez->points[1].p1 = *startpoint;
  point_add(&bez->points[1].p1, &defaultlen);
  bez->points[1].p2 = bez->points[1].p1;
  point_add(&bez->points[1].p2, &defaultlen);
  bez->points[1].p3 = bez->points[1].p2;
  point_add(&bez->points[1].p3, &defaultlen);

  bezierline_update_data(bezierline);

  bezierline->line_width =  attributes_get_default_linewidth();
  bezierline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&bezierline->line_style,
				    &bezierline->dashlength);
  bezierline->start_arrow = attributes_get_default_start_arrow();
  bezierline->end_arrow = attributes_get_default_end_arrow();

  *handle1 = bez->object.handles[0];
  *handle2 = bez->object.handles[3];
  return (Object *)bezierline;
}

static void
bezierline_destroy(Bezierline *bezierline)
{
  bezierconn_destroy(&bezierline->bez);
}

static Object *
bezierline_copy(Bezierline *bezierline)
{
  Bezierline *newbezierline;
  BezierConn *bez, *newbez;
  Object *newobj;
  
  bez = &bezierline->bez;
 
  newbezierline = g_new(Bezierline, 1);
  newbez = &newbezierline->bez;
  newobj = (Object *) newbezierline;

  bezierconn_copy(bez, newbez);

  newbezierline->line_color = bezierline->line_color;
  newbezierline->line_width = bezierline->line_width;
  newbezierline->line_style = bezierline->line_style;
  newbezierline->dashlength = bezierline->dashlength;
  newbezierline->start_arrow = bezierline->start_arrow;
  newbezierline->end_arrow = bezierline->end_arrow;

  return (Object *)newbezierline;
}


static void
bezierline_update_data(Bezierline *bezierline)
{
  BezierConn *bez = &bezierline->bez;
  Object *obj = (Object *) bezierline;

  bezierconn_update_data(bez);
    
  bezierconn_update_boundingbox(bez);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= bezierline->line_width/2;
  obj->bounding_box.left -= bezierline->line_width/2;
  obj->bounding_box.bottom += bezierline->line_width/2;
  obj->bounding_box.right += bezierline->line_width/2;

  /* Fix boundingbox for arrowheads */
  if (bezierline->start_arrow.type != ARROW_NONE ||
      bezierline->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (bezierline->start_arrow.type != ARROW_NONE)
      arrow_width = bezierline->start_arrow.width;
    if (bezierline->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, bezierline->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }

  obj->position = bez->points[0].p1;
}

static void
bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
	      const char *filename)
{
  bezierconn_save(&bezierline->bez, obj_node);

  if (!color_equals(&bezierline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &bezierline->line_color);
  
  if (bezierline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  bezierline->line_width);
  
  if (bezierline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  bezierline->line_style);

  if (bezierline->line_style != LINESTYLE_SOLID &&
      bezierline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  bezierline->dashlength);
  
  if (bezierline->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  bezierline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  bezierline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  bezierline->start_arrow.width);
  }
  
  if (bezierline->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  bezierline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  bezierline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  bezierline->end_arrow.width);
  }
}

static Object *
bezierline_load(ObjectNode obj_node, int version, const char *filename)
{
  Bezierline *bezierline;
  BezierConn *bez;
  Object *obj;
  AttributeNode attr;

  bezierline = g_new(Bezierline, 1);

  bez = &bezierline->bez;
  obj = (Object *) bezierline;
  
  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  bezierconn_load(bez, obj_node);

  bezierline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &bezierline->line_color);

  bezierline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    bezierline->line_width = data_real(attribute_first_data(attr));

  bezierline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    bezierline->line_style = data_enum(attribute_first_data(attr));

  bezierline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    bezierline->dashlength = data_real(attribute_first_data(attr));

  bezierline->start_arrow.type = ARROW_NONE;
  bezierline->start_arrow.length = 0.8;
  bezierline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    bezierline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    bezierline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    bezierline->start_arrow.width = data_real(attribute_first_data(attr));

  bezierline->end_arrow.type = ARROW_NONE;
  bezierline->end_arrow.length = 0.8;
  bezierline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    bezierline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    bezierline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    bezierline->end_arrow.width = data_real(attribute_first_data(attr));

  bezierline_update_data(bezierline);

  return (Object *)bezierline;
}

static ObjectChange *
bezierline_add_segment_callback (Object *obj, Point *clicked, gpointer data)
{
  Bezierline *bezierline = (Bezierline*) obj;
  int segment;
  ObjectChange *change;

  segment = bezierline_closest_segment(bezierline, clicked);
  change = bezierconn_add_segment(&bezierline->bez, segment, clicked);
  bezierline_update_data(bezierline);
  return change;
}

static ObjectChange *
bezierline_delete_segment_callback (Object *obj, Point *clicked, gpointer data)
{
  int seg_nr;
  Bezierline *bezierline = (Bezierline*) obj;
  ObjectChange *change;
  
  seg_nr = bezierconn_closest_segment(&bezierline->bez, clicked,
				      bezierline->line_width);

  change = bezierconn_remove_segment(&bezierline->bez, seg_nr+1);
  bezierline_update_data(bezierline);
  return change;
}

static ObjectChange *
bezierline_set_corner_type_callback (Object *obj, Point *clicked, gpointer data)
{
  Handle *closest;
  Bezierline *bezierline = (Bezierline*) obj;
  ObjectChange *change;
  
  closest = bezierconn_closest_major_handle(&bezierline->bez, clicked);
  change = bezierconn_set_corner_type(&bezierline->bez, closest, 
				      GPOINTER_TO_INT(data));
  
  bezierline_update_data(bezierline);
  return change;
}

static DiaMenuItem bezierline_menu_items[] = {
  { N_("Add Segment"), bezierline_add_segment_callback, NULL, 1 },
  { N_("Delete Segment"), bezierline_delete_segment_callback, NULL, 1 },
  { NULL, NULL, NULL, 1 },
  { N_("Symmetric control"), bezierline_set_corner_type_callback, 
    GINT_TO_POINTER(BEZ_CORNER_SYMMETRIC), 1 },
  { N_("Smooth control"), bezierline_set_corner_type_callback, 
    GINT_TO_POINTER(BEZ_CORNER_SMOOTH), 1 },
  { N_("Cusp control"), bezierline_set_corner_type_callback,
    GINT_TO_POINTER(BEZ_CORNER_CUSP), 1 },
};

static DiaMenu bezierline_menu = {
  "Bezierline",
  sizeof(bezierline_menu_items)/sizeof(DiaMenuItem),
  bezierline_menu_items,
  NULL
};

static DiaMenu *
bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint)
{
  Handle *closest;
  gboolean closest_is_endpoint;
  BezCornerType ctype = 42; /* nothing */
  gint i;

  closest = bezierconn_closest_major_handle(&bezierline->bez, clickedpoint);
  if (closest->id == HANDLE_MOVE_STARTPOINT ||
      closest->id == HANDLE_MOVE_ENDPOINT)
    closest_is_endpoint = TRUE;
  else
    closest_is_endpoint = FALSE;

  for (i = 0; i < bezierline->bez.numpoints; i++)
    if (bezierline->bez.object.handles[3*i] == closest) {
      ctype = bezierline->bez.corner_types[i];
      break;
    }

  /* Set entries sensitive/selected etc here */
  bezierline_menu_items[0].active = 1;
  bezierline_menu_items[1].active = bezierline->bez.numpoints > 2;
  bezierline_menu_items[3].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_SYMMETRIC);
  bezierline_menu_items[4].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_SMOOTH);
  bezierline_menu_items[5].active = !closest_is_endpoint &&
    (ctype != BEZ_CORNER_CUSP);
  return &bezierline_menu;
}
