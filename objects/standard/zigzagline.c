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
#include <math.h>

#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/zigzag.xpm"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
} Zigzagline;


static void zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void zigzagline_move(Zigzagline *zigzagline, Point *to);
static void zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void zigzagline_draw(Zigzagline *zigzagline, DiaRenderer *renderer);
static Object *zigzagline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real zigzagline_distance_from(Zigzagline *zigzagline, Point *point);
static void zigzagline_update_data(Zigzagline *zigzagline);
static void zigzagline_destroy(Zigzagline *zigzagline);
static Object *zigzagline_copy(Zigzagline *zigzagline);
static DiaMenu *zigzagline_get_object_menu(Zigzagline *zigzagline,
					   Point *clickedpoint);

static PropDescription *zigzagline_describe_props(Zigzagline *zigzagline);
static void zigzagline_get_props(Zigzagline *zigzagline, GPtrArray *props);
static void zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props);

static void zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
			    const char *filename);
static Object *zigzagline_load(ObjectNode obj_node, int version,
			       const char *filename);

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save,      /* save */
  (GetDefaultsFunc)   NULL,/*zigzagline_get_defaults*/
  (ApplyDefaultsFunc) NULL /*zigzagline_apply_defaults*/
};

static ObjectType zigzagline_type =
{
  "Standard - ZigZagLine",   /* name */
  0,                         /* version */
  (char **) zigzag_xpm,      /* pixmap */
  
  &zigzagline_type_ops       /* ops */
};

ObjectType *_zigzagline_type = (ObjectType *) &zigzagline_type;


static ObjectOps zigzagline_ops = {
  (DestroyFunc)         zigzagline_destroy,
  (DrawFunc)            zigzagline_draw,
  (DistanceFunc)        zigzagline_distance_from,
  (SelectFunc)          zigzagline_select,
  (CopyFunc)            zigzagline_copy,
  (MoveFunc)            zigzagline_move,
  (MoveHandleFunc)      zigzagline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      zigzagline_get_object_menu,
  (DescribePropsFunc)   zigzagline_describe_props,
  (GetPropsFunc)        zigzagline_get_props,
  (SetPropsFunc)        zigzagline_set_props,
};

static PropDescription zigzagline_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_DESC_END
};

static PropDescription *
zigzagline_describe_props(Zigzagline *zigzagline)
{
  if (zigzagline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(zigzagline_props);
  return zigzagline_props;
}

static PropOffset zigzagline_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Zigzagline, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Zigzagline, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Zigzagline, line_style), offsetof(Zigzagline, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Zigzagline, end_arrow) },
  { NULL, 0, 0 }
};

static void
zigzagline_get_props(Zigzagline *zigzagline, GPtrArray *props)
{
  object_get_props_from_offsets(&zigzagline->orth.object, zigzagline_offsets,
				props);
}

static void
zigzagline_set_props(Zigzagline *zigzagline, GPtrArray *props)
{
  object_set_props_from_offsets(&zigzagline->orth.object, zigzagline_offsets,
				props);
  zigzagline_update_data(zigzagline);
}

static real
zigzagline_distance_from(Zigzagline *zigzagline, Point *point)
{
  OrthConn *orth = &zigzagline->orth;
  return orthconn_distance_from(orth, point, zigzagline->line_width);
}

static void
zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&zigzagline->orth);
}

/** Returns TRUE if this connection point allows horizontal connection
 * from our direction.
 */
static gboolean
zigzagline_horizontal_endpoint_allowed(ConnectionPoint *p1) {
  if (p1 == NULL) return TRUE;
  return p1->directions & (DIR_EAST|DIR_WEST);
}

/** Returns TRUE if this connection point allows vertical connection
 * from our direction.
 */
static gboolean
zigzagline_vertical_endpoint_allowed(ConnectionPoint *p1) {
  if (p1 == NULL) return TRUE;
  return p1->directions & (DIR_NORTH|DIR_SOUTH);
}

/** Returns TRUE if this zigzagline would be better off horizontal */
static gboolean
zigzagline_check_orientation(ConnectionPoint *p1, ConnectionPoint *p2,
			     Point *pos1, Point *pos2)
{
  real horiz_dist, vert_dist;
  int dir1, dir2;

  horiz_dist = pos2->x-pos1->x;
  vert_dist = pos2->y-pos1->y;

  if (fabs(vert_dist) < 0.00000001) return TRUE;
  if (fabs(horiz_dist) < 0.00000001) return FALSE;

  if (p1 == NULL) {
    dir1 = DIR_NORTH|DIR_SOUTH|DIR_EAST|DIR_WEST;
  } else {
    dir1 = p1->directions;
  }
  if (p2 == NULL) {
    dir2 = DIR_NORTH|DIR_SOUTH|DIR_EAST|DIR_WEST;
  } else {
    dir2 = p2->directions;
  }

  if (fabs(horiz_dist) > fabs(vert_dist)) {
    if (horiz_dist > 0) { /* West-to-east */
      if ((dir1 & DIR_EAST) && (dir2 & DIR_WEST))
	return TRUE;
    } else {
      if ((dir1 & DIR_WEST) && (dir2 & DIR_EAST))
	return TRUE;
    }
  } else {
    if (vert_dist > 0) { /* North-to-south */
      if ((dir1 & DIR_SOUTH) && (dir2 & DIR_NORTH))
	return FALSE;
    } else {
      if ((dir1 & DIR_NORTH) && (dir2 & DIR_SOUTH))
	return FALSE;
    }
  }
  
  /* We didn't find the best direction.  Try the minor direction */
  if (fabs(horiz_dist) <= fabs(vert_dist)) {
    if (horiz_dist > 0) { /* West-to-east */
      if ((dir1 & DIR_EAST) && (dir2 & DIR_WEST))
	return TRUE;
    } else {
      if ((dir1 & DIR_WEST) && (dir2 & DIR_EAST))
	return TRUE;
    }
  } else {
    if (vert_dist > 0) { /* North-to-south */
      if ((dir1 & DIR_SOUTH) && (dir2 & DIR_NORTH))
	return FALSE;
    } else {
      if ((dir1 & DIR_NORTH) && (dir2 & DIR_SOUTH))
	return FALSE;
    }
  }
  /* It's going to suck to be a zigzagline */
  /* No good match found */
  return TRUE;
}

/** Returns TRUE if this endpoint must change direction to look good. 
 * @param zigzagline The line itself.
 * @param cp The connectionpoint being connected to, or NULL.
 * @param handlenum The number of the handle considered (0 or npoints-2)
 * @param p1 The point defining the other end of this line segment.
 */
static gboolean zigzagline_endpoint_must_change(Zigzagline *zigzagline,
						ConnectionPoint *cp,
						int handlenum,
						Point *p1) 
{
  OrthConn *orth = (OrthConn*)zigzagline;
  int dir;
  if (cp == NULL) return FALSE;
  if (orth->orientation[handlenum] == HORIZONTAL) {
    if (p1->x < cp->pos.x &&
	(cp->directions & DIR_WEST) == 0) return TRUE;
    if (p1->x >= cp->pos.x &&
	(cp->directions & DIR_EAST) == 0) return TRUE;
    return FALSE;
  }
  if (orth->orientation[handlenum] == VERTICAL) {
    if (p1->y < cp->pos.y &&
	(cp->directions & DIR_NORTH) == 0) return TRUE;
    if (p1->y >= cp->pos.y &&
	(cp->directions & DIR_SOUTH) == 0) return TRUE;
    return FALSE;
  }
  return FALSE;
}

static void
zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  OrthConn *orth = (OrthConn*)zigzagline;
  int handle_nr;
  gboolean change_start = FALSE, change_end = FALSE;

  assert(zigzagline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  orthconn_move_handle(orth, handle, to, reason);
#if 0
  handle_nr = get_handle_nr(orth, handle);
  if (handle_nr == 0 || handle_nr == orth->numpoints-2) {
    /* An ending handle, must adjust orientation */
    layer_find_closest_connectionpoint(dia_object_get_parent_layer((Object*)zigzagline), &cp1, &orth->points[handle_nr], (Object *)zigzagline);
    change_start = zigzagline_endpoint_must_change(zigzagline, cp1, 0);
    change_end = zigzagline_endpoint_must_change(zigzagline, cp1, orth->numpoints-2);

    /* To add:  When creating, favor three-segment lines */
    
  }
#endif
#if 1
  if (reason == HANDLE_MOVE_CREATE) {
    /* This only works for the creation stage, as we assume # of points */
    gboolean horizontal;
    ConnectionPoint *cp2 = NULL;

    /* The second connectionpoint is not updated yet, so we find it here */
    layer_find_closest_connectionpoint(dia_object_get_parent_layer((Object*)zigzagline), &cp2, &orth->points[3], (Object *)zigzagline);
    if (cp2 != NULL &&
	(cp2->pos.x - orth->points[3].x > 0.00000001 ||
	 cp2->pos.y - orth->points[3].y > 0.00000001))
	cp2 = NULL;
    horizontal = zigzagline_check_orientation(orth->object.handles[0]->connected_to,
					      cp2,
					      &orth->points[0], 
					      &orth->points[3]);
    if (horizontal) {
      real mid = (orth->points[0].x + orth->points[3].x)/2;
      orth->points[1].x = mid;
      orth->points[1].y = orth->points[0].y;
      orth->points[2].x = mid;
      orth->points[2].y = orth->points[3].y;
      orth->orientation[0] = HORIZONTAL;
      orth->orientation[1] = VERTICAL;
      orth->orientation[2] = HORIZONTAL;
    } else {
      real mid = (orth->points[0].y + orth->points[3].y)/2;
      orth->points[1].x = orth->points[0].x;
      orth->points[1].y = mid;
      orth->points[2].x = orth->points[3].x;
      orth->points[2].y = mid;
      orth->orientation[0] = VERTICAL;
      orth->orientation[1] = HORIZONTAL;
      orth->orientation[2] = VERTICAL;
    }
  }
#endif
  zigzagline_update_data(zigzagline);
}


static void
zigzagline_move(Zigzagline *zigzagline, Point *to)
{
  orthconn_move(&zigzagline->orth, to);
  zigzagline_update_data(zigzagline);
}

static void
zigzagline_draw(Zigzagline *zigzagline, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  OrthConn *orth = &zigzagline->orth;
  Point *points;
  int n;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer_ops->set_linewidth(renderer, zigzagline->line_width);
  renderer_ops->set_linestyle(renderer, zigzagline->line_style);
  renderer_ops->set_dashlength(renderer, zigzagline->dashlength);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer_ops->draw_polyline_with_arrows(renderer,
					   points, n,
					   zigzagline->line_width,
					   &zigzagline->line_color,
					   &zigzagline->start_arrow,
					   &zigzagline->end_arrow);
}

static Object *
zigzagline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  Object *obj;

  /*zigzagline_init_defaults();*/
  zigzagline = g_malloc0(sizeof(Zigzagline));
  orth = &zigzagline->orth;
  obj = &orth->object;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;
  
  orthconn_init(orth, startpoint);

  zigzagline_update_data(zigzagline);

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&zigzagline->line_style,
				    &zigzagline->dashlength);
  zigzagline->start_arrow = attributes_get_default_start_arrow();
  zigzagline->end_arrow = attributes_get_default_end_arrow();
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &zigzagline->orth.object;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
  orthconn_destroy(&zigzagline->orth);
}

static Object *
zigzagline_copy(Zigzagline *zigzagline)
{
  Zigzagline *newzigzagline;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &zigzagline->orth;
 
  newzigzagline = g_malloc0(sizeof(Zigzagline));
  neworth = &newzigzagline->orth;
  newobj = &neworth->object;

  orthconn_copy(orth, neworth);

  newzigzagline->line_color = zigzagline->line_color;
  newzigzagline->line_width = zigzagline->line_width;
  newzigzagline->line_style = zigzagline->line_style;
  newzigzagline->dashlength = zigzagline->dashlength;
  newzigzagline->start_arrow = zigzagline->start_arrow;
  newzigzagline->end_arrow = zigzagline->end_arrow;

  zigzagline_update_data(newzigzagline);

  return &newzigzagline->orth.object;
}

static void
zigzagline_update_data(Zigzagline *zigzagline)
{
  OrthConn *orth = &zigzagline->orth;
  PolyBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&zigzagline->orth);
    
  extra->start_long = 
    extra->end_long = 
    extra->middle_trans = zigzagline->line_width/2.0;
  extra->start_trans = (zigzagline->line_width / 2.0);
  extra->end_trans = (zigzagline->line_width / 2.0);

  if (zigzagline->start_arrow.type != ARROW_NONE) 
    extra->start_trans = MAX(extra->start_trans,zigzagline->start_arrow.width);
  if (zigzagline->end_arrow.type != ARROW_NONE) 
    extra->end_trans = MAX(extra->end_trans,zigzagline->end_arrow.width);
  
  orthconn_update_boundingbox(orth);
}

static ObjectChange *
zigzagline_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static ObjectChange *
zigzagline_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), zigzagline_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), zigzagline_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Zigzagline",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
zigzagline_get_object_menu(Zigzagline *zigzagline, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &zigzagline->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}


static void
zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
		const char *filename)
{
  orthconn_save(&zigzagline->orth, obj_node);

  if (!color_equals(&zigzagline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &zigzagline->line_color);
  
  if (zigzagline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  zigzagline->line_width);
  
  if (zigzagline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  zigzagline->line_style);
  
  if (zigzagline->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  zigzagline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  zigzagline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  zigzagline->start_arrow.width);
  }
  
  if (zigzagline->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  zigzagline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  zigzagline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  zigzagline->end_arrow.width);
  }

  if (zigzagline->line_style != LINESTYLE_SOLID && 
      zigzagline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	  data_add_real(new_attribute(obj_node, "dashlength"),
		  zigzagline->dashlength);
}

static Object *
zigzagline_load(ObjectNode obj_node, int version, const char *filename)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  Object *obj;
  AttributeNode attr;

  zigzagline = g_malloc0(sizeof(Zigzagline));

  orth = &zigzagline->orth;
  obj = &orth->object;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  orthconn_load(orth, obj_node);

  zigzagline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &zigzagline->line_color);

  zigzagline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    zigzagline->line_width = data_real(attribute_first_data(attr));

  zigzagline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    zigzagline->line_style = data_enum(attribute_first_data(attr));

  zigzagline->start_arrow.type = ARROW_NONE;
  zigzagline->start_arrow.length = 0.8;
  zigzagline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    zigzagline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    zigzagline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    zigzagline->start_arrow.width = data_real(attribute_first_data(attr));

  zigzagline->end_arrow.type = ARROW_NONE;
  zigzagline->end_arrow.length = 0.8;
  zigzagline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    zigzagline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    zigzagline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    zigzagline->end_arrow.width = data_real(attribute_first_data(attr));

  zigzagline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
	  zigzagline->dashlength = data_real(attribute_first_data(attr));
  
  zigzagline_update_data(zigzagline);

  return &zigzagline->orth.object;
}
