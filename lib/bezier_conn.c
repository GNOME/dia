/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * BezierConn  Copyright (C) 1999 James Henstridge
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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "bezier_conn.h"
#include "intl.h"
#include "message.h"

#define HANDLE_BEZMAJOR  (HANDLE_CUSTOM1)
#define HANDLE_LEFTCTRL  (HANDLE_CUSTOM2)
#define HANDLE_RIGHTCTRL (HANDLE_CUSTOM3)

enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};

struct PointChange {
  ObjectChange obj_change;

  enum change_type type;
  int applied;
  
  BezPoint point;
  int pos;
  /* owning ref when not applied for ADD_POINT
   * owning ref when applied for REMOVE_POINT */
  Handle *handle1, *handle2, *handle3;
  /* NULL if not connected */
  ConnectionPoint *connected_to1, *connected_to2, *connected_to3;
};

struct CornerChange {
  ObjectChange obj_change;
  /* Only one kind of corner_change */
  int applied;

  Handle *handle;
  /* Old places when SET_CORNER_TYPE is applied */
  Point point_left, point_right;
  int old_type, new_type;
};

static ObjectChange *
bezierconn_create_point_change(BezierConn *bez, enum change_type type,
			       BezPoint *point, int segment,
			       Handle *handle1, ConnectionPoint *connected_to1,
			       Handle *handle2, ConnectionPoint *connected_to2,
			       Handle *handle3, ConnectionPoint *connected_to3);
static ObjectChange *
bezierconn_create_corner_change(BezierConn *bez, Handle *handle,
				Point *point_left, Point *point_right,
				int old_corner_type, int new_corner_type);


static void setup_corner_handle(Handle *handle, HandleId id)
{
  handle->id = id;
  handle->type = HANDLE_MINOR_CONTROL;
  handle->connect_type = (id == HANDLE_BEZMAJOR) ?
      HANDLE_CONNECTABLE : HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}

static int get_handle_nr(BezierConn *bez, Handle *handle)
{
  int i = 0;
  for (i=0;i<bez->object.num_handles;i++) {
    if (bez->object.handles[i] == handle)
      return i;
  }
  return -1;
}

#define get_comp_nr(hnum) (((int)(hnum)+2)/3)

void
bezierconn_move_handle(BezierConn *bez, Handle *handle,
		     Point *to, HandleMoveReason reason)
{
  int handle_nr, comp_nr;
  Point delta, pt;
  
  delta = *to;
  point_sub(&delta, &handle->pos);

  handle_nr = get_handle_nr(bez, handle);
  comp_nr = get_comp_nr(handle_nr);
  switch(handle->id) {
  case HANDLE_MOVE_STARTPOINT:
    bez->points[0].p1 = *to;
    /* shift adjacent point */
    point_add(&bez->points[1].p1, &delta);
    break;
  case HANDLE_MOVE_ENDPOINT:
    bez->points[bez->numpoints-1].p3 = *to;
    /* shift adjacent point */
    point_add(&bez->points[bez->numpoints-1].p2, &delta);
    break;
  case HANDLE_BEZMAJOR:
    bez->points[comp_nr].p3 = *to;
    /* shift adjacent point */
    point_add(&bez->points[comp_nr].p2, &delta);
    point_add(&bez->points[comp_nr+1].p1, &delta);
    break;
  case HANDLE_LEFTCTRL:
    bez->points[comp_nr].p2 = *to;
    if (comp_nr < bez->numpoints - 1) {
      switch (bez->points[comp_nr].corner_type) {
      case BEZ_CORNER_SYMMETRIC:
	pt = bez->points[comp_nr].p3;
	point_sub(&pt, &bez->points[comp_nr].p2);
	point_add(&pt, &bez->points[comp_nr].p3);
	bez->points[comp_nr+1].p1 = pt;
	break;
      case BEZ_CORNER_SMOOTH: {
	real len;
	pt = bez->points[comp_nr+1].p1;
	point_sub(&pt, &bez->points[comp_nr].p3);
	len = point_len(&pt);
	pt = bez->points[comp_nr].p2;
	point_sub(&pt, &bez->points[comp_nr].p3);
	point_normalize(&pt);
	point_scale(&pt, -len);
	point_add(&pt, &bez->points[comp_nr].p3);
	bez->points[comp_nr+1].p1 = pt;
	break;
      }	
      case BEZ_CORNER_CUSP:
	/* Do nothing to the other guy */
	break;
      }
    }
    break;
  case HANDLE_RIGHTCTRL:
    bez->points[comp_nr].p1 = *to;
    if (comp_nr > 1) {
      switch (bez->points[comp_nr-1].corner_type) {
      case BEZ_CORNER_SYMMETRIC:
	pt = bez->points[comp_nr - 1].p3;
	point_sub(&pt, &bez->points[comp_nr].p1);
	point_add(&pt, &bez->points[comp_nr - 1].p3);
	bez->points[comp_nr-1].p2 = pt;
	break;
      case BEZ_CORNER_SMOOTH: {
	real len;
	pt = bez->points[comp_nr-1].p2;
	point_sub(&pt, &bez->points[comp_nr-1].p3);
	len = point_len(&pt);
	pt = bez->points[comp_nr].p1;
	point_sub(&pt, &bez->points[comp_nr-1].p3);
	point_normalize(&pt);
	point_scale(&pt, -len);
	point_add(&pt, &bez->points[comp_nr-1].p3);
	bez->points[comp_nr-1].p2 = pt;
	break;
      }	
      case BEZ_CORNER_CUSP:
	/* Do nothing to the other guy */
	break;
      }
    }
    break;
  default:
    message_error("Internal error in bezierconn_move_handle.\n");
    break;
  }
}

void
bezierconn_move(BezierConn *bez, Point *to)
{
  Point p;
  int i;
  
  p = *to;
  point_sub(&p, &bez->points[0].p1);

  bez->points[0].p1 = *to;
  for (i = 1; i < bez->numpoints; i++) {
    point_add(&bez->points[i].p1, &p);
    point_add(&bez->points[i].p2, &p);
    point_add(&bez->points[i].p3, &p);
  }
}

int
bezierconn_closest_segment(BezierConn *bez, Point *point, real line_width)
{
  Point last;
  int i;
  real dist = G_MAXDOUBLE;
  int closest;

  closest = 0;
  last = bez->points[0].p1;
  for (i = 0; i < bez->numpoints - 1; i++) {
    real new_dist = distance_bez_seg_point(&last, &bez->points[i+1].p1,
				&bez->points[i+1].p2, &bez->points[i+1].p3,
				line_width, point);
    if (new_dist < dist) {
      dist = new_dist;
      closest = i;
    }
    last = bez->points[i+1].p3;
  }
  return closest;
}

Handle *
bezierconn_closest_handle(BezierConn *bez, Point *point)
{
  int i, hn;
  real dist;
  Handle *closest;
  
  closest = bez->object.handles[0];
  dist = distance_point_point( point, &closest->pos);
  for (i = 1, hn = 1; i < bez->numpoints; i++, hn++) {
    real new_dist;

    new_dist = distance_point_point(point, &bez->points[i].p1);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bez->object.handles[hn];
    }
    hn++;

    new_dist = distance_point_point(point, &bez->points[i].p2);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bez->object.handles[hn];
    }
    hn++;

    new_dist = distance_point_point(point, &bez->points[i].p3);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bez->object.handles[hn];
    }
  }
  return closest;
}

real
bezierconn_distance_from(BezierConn *bez, Point *point, real line_width)
{
  return distance_bez_line_point(bez->points, bez->numpoints,
				 line_width, point);
}

static void
add_handles(BezierConn *bez, int pos, BezPoint *point, Handle *handle1,
	    Handle *handle2, Handle *handle3)
{
  int i;
  Object *obj;

  assert(pos > 0);
  
  obj = (Object *)bez;
  bez->numpoints++;
  bez->points = g_realloc(bez->points, bez->numpoints*sizeof(BezPoint));

  for (i = bez->numpoints-1; i > pos; i--) {
    bez->points[i] = bez->points[i-1];
  }
  bez->points[pos] = *point; 
  bez->points[pos].p1 = bez->points[pos+1].p1;
  bez->points[pos+1].p1 = point->p1;
  bez->points[pos].corner_type = BEZ_CORNER_SYMMETRIC;
  object_add_handle_at(obj, handle1, 3*pos-2);
  object_add_handle_at(obj, handle2, 3*pos-1);
  object_add_handle_at(obj, handle3, 3*pos);

  if (pos==bez->numpoints-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MINOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_BEZMAJOR;
  }
}

static void
remove_handles(BezierConn *bez, int pos)
{
  int i;
  Object *obj;
  Handle *old_handle1, *old_handle2, *old_handle3;
  Point tmppoint;

  assert(pos > 0);

  obj = (Object *)bez;

  if (pos==obj->num_handles-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_MOVE_ENDPOINT;
  }

  /* delete the points */
  bez->numpoints--;
  tmppoint = bez->points[pos].p1;
  for (i = pos; i < bez->numpoints; i++) {
    bez->points[i] = bez->points[i+1];
  }
  bez->points[pos].p1 = tmppoint;
  bez->points = g_realloc(bez->points, bez->numpoints*sizeof(BezPoint));

  old_handle1 = obj->handles[3*pos-2];
  old_handle2 = obj->handles[3*pos-1];
  old_handle3 = obj->handles[3*pos];
  object_remove_handle(&bez->object, old_handle1);
  object_remove_handle(&bez->object, old_handle2);
  object_remove_handle(&bez->object, old_handle3);
}


/* Add a point by splitting segment into two, putting the new point at
 'point' or, if NULL, in the middle */
ObjectChange *
bezierconn_add_segment(BezierConn *bez, int segment, Point *point)
{
  BezPoint realpoint;
  Handle *new_handle1, *new_handle2, *new_handle3;
  Point startpoint;

  if (segment == 0)
    startpoint = bez->points[0].p1;
  else
    startpoint = bez->points[segment].p3;

  if (point == NULL) {
    realpoint.p1.x = (startpoint.x + bez->points[segment+1].p3.x) / 6;
    realpoint.p1.y = (startpoint.y + bez->points[segment+1].p3.y) / 6;
    realpoint.p2.x = (startpoint.x + bez->points[segment+1].p3.x) / 3;
    realpoint.p2.y = (startpoint.y + bez->points[segment+1].p3.y) / 3;
    realpoint.p3.x = (startpoint.x + bez->points[segment+1].p3.x) / 2;
    realpoint.p3.y = (startpoint.y + bez->points[segment+1].p3.y) / 2;
  } else {
    realpoint.p2.x = point->x+(startpoint.x - bez->points[segment+1].p3.x)/6;
    realpoint.p2.y = point->y+(startpoint.y - bez->points[segment+1].p3.y)/6;
    realpoint.p3 = *point;
    /* this really goes into the next segment ... */
    realpoint.p1.x = point->x-(startpoint.x - bez->points[segment+1].p3.x)/6;
    realpoint.p1.y = point->y-(startpoint.y - bez->points[segment+1].p3.y)/6;
  }
  realpoint.type = BEZ_CURVE_TO;

  new_handle1 = g_malloc(sizeof(Handle));
  new_handle2 = g_malloc(sizeof(Handle));
  new_handle3 = g_malloc(sizeof(Handle));
  setup_corner_handle(new_handle1, HANDLE_RIGHTCTRL);
  setup_corner_handle(new_handle2, HANDLE_LEFTCTRL);
  setup_corner_handle(new_handle3, HANDLE_BEZMAJOR);
  add_handles(bez, segment+1, &realpoint, new_handle1,new_handle2,new_handle3);
  return bezierconn_create_point_change(bez, TYPE_ADD_POINT,
					&realpoint, segment+1,
					new_handle1, NULL,
					new_handle2, NULL,
					new_handle3, NULL);
}

ObjectChange *
bezierconn_remove_segment(BezierConn *bez, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *cpt1, *cpt2, *cpt3;
  BezPoint old_point;

  assert(pos > 0);

  old_handle1 = bez->object.handles[3*pos-2];
  old_handle2 = bez->object.handles[3*pos-1];
  old_handle3 = bez->object.handles[3*pos];
  old_point = bez->points[pos];

  cpt1 = old_handle1->connected_to;
  cpt2 = old_handle2->connected_to;
  cpt3 = old_handle3->connected_to;
  
  object_unconnect((Object *)bez, old_handle1);
  object_unconnect((Object *)bez, old_handle2);
  object_unconnect((Object *)bez, old_handle3);

  remove_handles(bez, pos);

  bezierconn_update_data(bez);
  
  return bezierconn_create_point_change(bez, TYPE_REMOVE_POINT,
					&old_point, pos,
					old_handle1, cpt1,
					old_handle2, cpt2,
					old_handle3, cpt3);
}

static void
bezierconn_straighten_corner(BezierConn *bez, int comp_nr) {
  /* Neat thing would be to have the kind of straigthening depend on
     which handle was chosen:  Mid-handle does average, other leaves that
     handle where it is. */
  message_warning("Segment %d type %d\n", comp_nr, bez->points[comp_nr].corner_type);
  switch (bez->points[comp_nr].corner_type) {
  case BEZ_CORNER_SYMMETRIC: {
    Point pt1, pt2;
    pt1 = bez->points[comp_nr].p3;
    point_sub(&pt1, &bez->points[comp_nr].p2);
    pt2 = bez->points[comp_nr].p3;
    point_sub(&pt2, &bez->points[comp_nr+1].p1);
    point_scale(&pt2, -1.0);
    point_add(&pt1, &pt2);
    point_scale(&pt1, 0.5);
    pt2 = pt1;
    point_scale(&pt1, -1.0);
    point_add(&pt1, &bez->points[comp_nr].p3);
    point_add(&pt2, &bez->points[comp_nr].p3);
    bez->points[comp_nr].p2 = pt1;
    bez->points[comp_nr+1].p1 = pt2;
    bezierconn_update_data(bez);
  }
  break;
  case BEZ_CORNER_SMOOTH: {
    Point pt1, pt2;
    real len1, len2;
    pt1 = bez->points[comp_nr].p3;
    point_sub(&pt1, &bez->points[comp_nr].p2);
    pt2 = bez->points[comp_nr].p3;
    point_sub(&pt2, &bez->points[comp_nr+1].p1);
    len1 = point_len(&pt1);
    len2 = point_len(&pt2);
    point_scale(&pt2, -1.0);
    point_normalize(&pt1);
    point_normalize(&pt2);
    point_add(&pt1, &pt2);
    point_scale(&pt1, 0.5);
    pt2 = pt1;
    point_scale(&pt1, -len1);
    point_add(&pt1, &bez->points[comp_nr].p3);
    point_scale(&pt2, len2);
    point_add(&pt2, &bez->points[comp_nr].p3);
    bez->points[comp_nr].p2 = pt1;
    bez->points[comp_nr+1].p1 = pt2;
    bezierconn_update_data(bez);
  }
    break;
  case BEZ_CORNER_CUSP:
    break;
  }
}

ObjectChange *
bezierconn_set_corner_type(BezierConn *bez, Handle *handle, int corner_type)
{
  Handle *mid_handle;
  Point old_left, old_right;
  int old_type;
  int handle_nr, comp_nr;

  handle_nr = get_handle_nr(bez, handle);

  switch (handle->id) {
  case HANDLE_BEZMAJOR:
    mid_handle = handle;
    break;
  case HANDLE_LEFTCTRL:
    handle_nr++;
    mid_handle = bez->object.handles[handle_nr];
    break;
  case HANDLE_RIGHTCTRL:
    handle_nr--;
    mid_handle = bez->object.handles[handle_nr];
    break;
  default:
    message_warning(_("Internal error: Setting corner type of endpoint of bezier"));
    return NULL;
  }

  comp_nr = get_comp_nr(handle_nr);

  old_type = bez->points[comp_nr].corner_type;
  old_left = bez->points[comp_nr].p2;
  old_right = bez->points[comp_nr+1].p1;

  bez->points[comp_nr].corner_type = corner_type;

  bezierconn_straighten_corner(bez, comp_nr);

  return bezierconn_create_corner_change(bez, mid_handle, &old_left, &old_right,
					 old_type, corner_type);
}

void
bezierconn_update_data(BezierConn *bez)
{
  int i;
  
  /* Update handles: */
  bez->object.handles[0]->pos = bez->points[0].p1;
  for (i = 1; i < bez->numpoints; i++) {
    bez->object.handles[3*i-2]->pos = bez->points[i].p1;
    bez->object.handles[3*i-1]->pos = bez->points[i].p2;
    bez->object.handles[3*i]->pos   = bez->points[i].p3;
  }
}

void
bezierconn_update_boundingbox(BezierConn *bez)
{
  Rectangle *bb;
  BezPoint *points;
  int i;
  
  assert(bez != NULL);

  bb = &bez->object.bounding_box;
  points = &bez->points[0];

  bb->right = bb->left = points[0].p1.x;
  bb->top = bb->bottom = points[0].p1.y;
  for (i = 1; i < bez->numpoints; i++) {
    if (points[i].p1.x < bb->left)
      bb->left = points[i].p1.x;
    if (points[i].p1.x > bb->right)
      bb->right = points[i].p1.x;
    if (points[i].p1.y < bb->top)
      bb->top = points[i].p1.y;
    if (points[i].p1.y > bb->bottom)
      bb->bottom = points[i].p1.y;
    if (points[i].type == BEZ_CURVE_TO) {
      if (points[i].p2.x < bb->left)
	bb->left = points[i].p2.x;
      if (points[i].p2.x > bb->right)
	bb->right = points[i].p2.x;
      if (points[i].p2.y < bb->top)
	bb->top = points[i].p2.y;
      if (points[i].p2.y > bb->bottom)
	bb->bottom = points[i].p2.y;
      if (points[i].p3.x < bb->left)
	bb->left = points[i].p3.x;
      if (points[i].p3.x > bb->right)
	bb->right = points[i].p3.x;
      if (points[i].p3.y < bb->top)
	bb->top = points[i].p3.y;
      if (points[i].p3.y > bb->bottom)
	bb->bottom = points[i].p3.y;
    }
  }
}

void
bezierconn_simple_draw(BezierConn *bez, Renderer *renderer, real width)
{
  BezPoint *points;
  
  assert(bez != NULL);
  assert(renderer != NULL);

  points = &bez->points[0];
  
  renderer->ops->set_linewidth(renderer, width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_ROUND);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_bezier(renderer, points, bez->numpoints, &color_black);
}

void
bezierconn_draw_control_lines(BezierConn *bez, Renderer *renderer)
{
  Color line_colour = {0.0, 0.0, 0.6};
  Point startpoint;
  int i;
  
  /* setup renderer ... */
  renderer->ops->set_linewidth(renderer, 0);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DOTTED);
  renderer->ops->set_dashlength(renderer, 1);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  startpoint = bez->points[0].p1;
  for (i = 1; i < bez->numpoints; i++) {
    renderer->ops->draw_line(renderer, &startpoint, &bez->points[i].p1,
			     &line_colour);
    renderer->ops->draw_line(renderer, &bez->points[i].p2, &bez->points[i].p3,
			     &line_colour);
    startpoint = bez->points[i].p3;
  }
}

void
bezierconn_init(BezierConn *bez)
{
  Object *obj;

  obj = &bez->object;

  object_init(obj, 4, 0);
  
  bez->numpoints = 2;

  bez->points = g_new(BezPoint, 2);
  bez->points[0].type = BEZ_MOVE_TO;
  bez->points[1].type = BEZ_CURVE_TO;

  bez->object.handles[0] = g_new(Handle, 1);
  bez->object.handles[1] = g_new(Handle, 1);
  bez->object.handles[2] = g_new(Handle, 1);
  bez->object.handles[3] = g_new(Handle, 1);

  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  
  obj->handles[1]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[1]->connected_to = NULL;
  obj->handles[1]->type = HANDLE_MINOR_CONTROL;
  obj->handles[1]->id = HANDLE_RIGHTCTRL;

  obj->handles[2]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[2]->connected_to = NULL;
  obj->handles[2]->type = HANDLE_MINOR_CONTROL;
  obj->handles[2]->id = HANDLE_LEFTCTRL;
  
  obj->handles[3]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[3]->connected_to = NULL;
  obj->handles[3]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[3]->id = HANDLE_MOVE_ENDPOINT;

  bezierconn_update_data(bez);
}

void
bezierconn_copy(BezierConn *from, BezierConn *to)
{
  int i;
  Object *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  to->numpoints = from->numpoints;

  to->points = g_new(BezPoint, to->numpoints);

  for (i = 0; i < to->numpoints; i++) {
    to->points[i] = from->points[i];
  }

  to->object.handles[0] = g_new(Handle,1);
  *to->object.handles[0] = *from->object.handles[0];
  for (i = 1; i < to->object.num_handles - 1; i++) {
    to->object.handles[i] = g_new(Handle, 1);
    setup_corner_handle(to->object.handles[i], from->object.handles[i]->id);
  }
  to->object.handles[to->object.num_handles-1] = g_new(Handle,1);
  *to->object.handles[to->object.num_handles-1] =
    *from->object.handles[to->object.num_handles-1];

  bezierconn_update_data(to);
}

void
bezierconn_destroy(BezierConn *bez)
{
  int i, nh;
  Handle **temp_handles;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  nh = bez->object.num_handles;
  temp_handles = g_new(Handle *, nh);
  for (i = 0; i < nh; i++)
    temp_handles[i] = bez->object.handles[i];

  object_destroy(&bez->object);

  for (i = 0; i < nh; i++)
    g_free(temp_handles[i]);
  g_free(temp_handles);
  
  g_free(bez->points);
}


void
bezierconn_save(BezierConn *bez, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;

  object_save(&bez->object, obj_node);

  attr = new_attribute(obj_node, "bez_points");
  
  data_add_point(attr, &bez->points[0].p1);
  for (i = 1; i < bez->numpoints; i++) {
    data_add_point(attr, &bez->points[i].p1);
    data_add_point(attr, &bez->points[i].p2);
    data_add_point(attr, &bez->points[i].p3);
  }
}

void
bezierconn_load(BezierConn *bez, ObjectNode obj_node) /* NOTE: Does object_init() */
{
  int i;
  AttributeNode attr;
  DataNode data;
  
  Object *obj = &bez->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "bez_points");

  if (attr != NULL)
    bez->numpoints = (attribute_num_data(attr) + 2)/3;
  else
    bez->numpoints = 0;

  object_init(obj, 3 * bez->numpoints - 2, 0);

  data = attribute_first_data(attr);
  if (bez->numpoints != 0) {
    bez->points = g_new(BezPoint, bez->numpoints);
    bez->points[0].type = BEZ_MOVE_TO;
    data_point(data, &bez->points[0].p1);
    data = data_next(data);

    for (i = 1; i < bez->numpoints; i++) {
      bez->points[i].type = BEZ_CURVE_TO;
      data_point(data, &bez->points[i].p1);
      data = data_next(data);
      data_point(data, &bez->points[i].p2);
      data = data_next(data);
      data_point(data, &bez->points[i].p3);
      data = data_next(data);
    }
  }

  obj->handles[0] = g_new(Handle, 1);
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  
  for (i = 1; i < bez->numpoints; i++) {
    obj->handles[3*i-2] = g_new(Handle, 1);
    setup_corner_handle(obj->handles[3*i-2], HANDLE_RIGHTCTRL);
    obj->handles[3*i-1] = g_new(Handle, 1);
    setup_corner_handle(obj->handles[3*i-1], HANDLE_LEFTCTRL);
    obj->handles[3*i]   = g_new(Handle, 1);
    setup_corner_handle(obj->handles[3*i],   HANDLE_BEZMAJOR);
  }

  obj->handles[obj->num_handles-1]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[obj->num_handles-1]->connected_to = NULL;
  obj->handles[obj->num_handles-1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[obj->num_handles-1]->id = HANDLE_MOVE_ENDPOINT;

  bezierconn_update_data(bez);
}

static void
bezierconn_point_change_free(struct PointChange *change)
{
  if ( (change->type==TYPE_ADD_POINT && !change->applied) ||
       (change->type==TYPE_REMOVE_POINT && change->applied) ){
    g_free(change->handle1);
    g_free(change->handle2);
    g_free(change->handle3);
    change->handle1 = NULL;
    change->handle2 = NULL;
    change->handle3 = NULL;
  }
}

static void
bezierconn_point_change_apply(struct PointChange *change, Object *obj)
{
  change->applied = 1;
  switch (change->type) {
  case TYPE_ADD_POINT:
    add_handles((BezierConn *)obj, change->pos, &change->point,
		change->handle1, change->handle2, change->handle3);
    break;
  case TYPE_REMOVE_POINT:
    object_unconnect(obj, change->handle1);
    object_unconnect(obj, change->handle2);
    object_unconnect(obj, change->handle3);
    remove_handles((BezierConn *)obj, change->pos);
    break;
  }
}

static void
bezierconn_point_change_revert(struct PointChange *change, Object *obj)
{
  switch (change->type) {
  case TYPE_ADD_POINT:
    remove_handles((BezierConn *)obj, change->pos);
    break;
  case TYPE_REMOVE_POINT:
    add_handles((BezierConn *)obj, change->pos, &change->point,
		change->handle1, change->handle2, change->handle3);

    if (change->connected_to1)
      object_connect(obj, change->handle1, change->connected_to1);
    if (change->connected_to2)
      object_connect(obj, change->handle2, change->connected_to2);
    if (change->connected_to3)
      object_connect(obj, change->handle3, change->connected_to3);
      
    break;
  }
  change->applied = 0;
}

static ObjectChange *
bezierconn_create_point_change(BezierConn *bez, enum change_type type,
			       BezPoint *point, int pos,
			       Handle *handle1, ConnectionPoint *connected_to1,
			       Handle *handle2, ConnectionPoint *connected_to2,
			       Handle *handle3, ConnectionPoint *connected_to3)
{
  struct PointChange *change;

  change = g_new(struct PointChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) bezierconn_point_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) bezierconn_point_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) bezierconn_point_change_free;

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->pos = pos;
  change->handle1 = handle1;
  change->connected_to1 = connected_to1;
  change->handle2 = handle2;
  change->connected_to2 = connected_to2;
  change->handle3 = handle3;
  change->connected_to3 = connected_to3;

  return (ObjectChange *)change;
}

static void
bezierconn_corner_change_apply(struct CornerChange *change,
			       Object *obj) {
  BezierConn *bez = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_comp_nr(handle_nr);

  bezierconn_straighten_corner(bez, comp_nr);

  bez->points[comp_nr].corner_type = change->new_type;

  change->applied = 1;
}

static void
bezierconn_corner_change_revert(struct CornerChange *change,
				Object *obj) {
  BezierConn *bez = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_comp_nr(handle_nr);

  bez->points[comp_nr].p2 = change->point_left;
  bez->points[comp_nr+1].p1 = change->point_right;
  bez->points[comp_nr].corner_type = change->old_type;  

  change->applied = 0;
}

static ObjectChange *
bezierconn_create_corner_change(BezierConn *bez, Handle *handle,
				Point *point_left, Point *point_right,
				int old_corner_type, int new_corner_type)
{
  struct CornerChange *change;

  change = g_new(struct CornerChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) bezierconn_corner_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) bezierconn_corner_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) NULL;

  change->old_type = old_corner_type;
  change->new_type = new_corner_type;
  change->applied = 1;

  change->handle = handle;
  change->point_left = *point_left;
  change->point_right = *point_right;

  return (ObjectChange *)change;
}
