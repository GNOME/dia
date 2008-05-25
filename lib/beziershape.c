/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * beziershape.c - code to help implement bezier shapes
 * Copyright (C) 2000 James Henstridge
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

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy() */

#include "beziershape.h"
#include "message.h"
#include "diarenderer.h"

#define HANDLE_BEZMAJOR  (HANDLE_CUSTOM1)
#define HANDLE_LEFTCTRL  (HANDLE_CUSTOM2)
#define HANDLE_RIGHTCTRL (HANDLE_CUSTOM3)

enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};

/* Invariant:
   # of handles = 3*(numpoints-1)
   # of connections = 2*(numpoints-1) + 1 (main point)
   For historical reasons, the main point is the last cp.
 */
struct BezPointChange {
  ObjectChange obj_change;

  enum change_type type;
  int applied;
  
  BezPoint point;
  BezCornerType corner_type;
  int pos;

  /* owning ref when not applied for ADD_POINT
   * owning ref when applied for REMOVE_POINT */
  Handle *handle1, *handle2, *handle3;
  ConnectionPoint *cp1, *cp2;
};

struct CornerChange {
  ObjectChange obj_change;
  /* Only one kind of corner_change */
  int applied;

  Handle *handle;
  /* Old places when SET_CORNER_TYPE is applied */
  Point point_left, point_right;
  BezCornerType old_type, new_type;
};

static ObjectChange *
beziershape_create_point_change(BezierShape *bezier, enum change_type type,
			BezPoint *point, BezCornerType corner_type,
			int segment,
			Handle *handle1, Handle *handle2, Handle *handle3,
			ConnectionPoint *cp1, ConnectionPoint *cp2);
static ObjectChange *
beziershape_create_corner_change(BezierShape *bezier, Handle *handle,
			Point *point_left, Point *point_right,
			BezCornerType old_corner_type,
			BezCornerType new_corner_type);

static void new_handles_and_connections(BezierShape *bezier, int num_points);

static void setup_handle(Handle *handle, int handle_id)
{
  handle->id = handle_id;
  handle->type =
    (handle_id == HANDLE_BEZMAJOR) ?
    HANDLE_MAJOR_CONTROL :
    HANDLE_MINOR_CONTROL;
  handle->connect_type = HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}


static int get_handle_nr(BezierShape *bezier, Handle *handle)
{
  int i = 0;
  for (i = 0; i < bezier->object.num_handles; i++) {
    if (bezier->object.handles[i] == handle)
      return i;
  }
  return -1;
}

#define get_comp_nr(hnum) ((int)(hnum)/3+1)
#define get_major_nr(hnum) (((int)(hnum)+2)/3)

ObjectChange *
beziershape_move_handle(BezierShape *bezier, Handle *handle,
			Point *to, ConnectionPoint *cp,
			HandleMoveReason reason, ModifierKeys modifiers)
{
  int handle_nr, comp_nr, next_nr, prev_nr;
  Point delta, pt;
  
  delta = *to;
  point_sub(&delta, &handle->pos);

  handle_nr = get_handle_nr(bezier, handle);
  comp_nr = get_comp_nr(handle_nr);
  next_nr = comp_nr + 1;
  prev_nr = comp_nr - 1;
  if (comp_nr == bezier->numpoints - 1)
    next_nr = 1;
  if (comp_nr == 1)
    prev_nr = bezier->numpoints - 1;
  
  switch(handle->id) {
  case HANDLE_BEZMAJOR:
    if (comp_nr == bezier->numpoints - 1) {
      bezier->points[comp_nr].p3 = *to;
      bezier->points[0].p1 = bezier->points[0].p3 = *to;
      point_add(&bezier->points[comp_nr].p2, &delta);
      point_add(&bezier->points[1].p1, &delta);
    } else {
      bezier->points[comp_nr].p3 = *to;
      point_add(&bezier->points[comp_nr].p2, &delta);
      point_add(&bezier->points[comp_nr+1].p1, &delta);
    }
    break;
  case HANDLE_LEFTCTRL:
    bezier->points[comp_nr].p2 = *to;
    switch (bezier->corner_types[comp_nr]) {
    case BEZ_CORNER_SYMMETRIC:
      pt = bezier->points[comp_nr].p3;
      point_sub(&pt, &bezier->points[comp_nr].p2);
      point_add(&pt, &bezier->points[comp_nr].p3);
      bezier->points[next_nr].p1 = pt;
      break;
    case BEZ_CORNER_SMOOTH: {
      real len;

      pt = bezier->points[next_nr].p1;
      point_sub(&pt, &bezier->points[comp_nr].p3);
      len = point_len(&pt);

      pt = bezier->points[comp_nr].p3;
      point_sub(&pt, &bezier->points[comp_nr].p2);
      if (point_len(&pt) > 0)
	point_normalize(&pt);
      else {
	pt.x = 1.0; pt.y = 0.0;
      }
      point_scale(&pt, len);
      point_add(&pt, &bezier->points[comp_nr].p3);
      bezier->points[next_nr].p1 = pt;
      break;
    }
    case BEZ_CORNER_CUSP:
      /* no mirror point movement required */
      break;
    }
    break;
  case HANDLE_RIGHTCTRL:
    bezier->points[comp_nr].p1 = *to;
    switch (bezier->corner_types[prev_nr]) {
    case BEZ_CORNER_SYMMETRIC:
      pt = bezier->points[prev_nr].p3;
      point_sub(&pt, &bezier->points[comp_nr].p1);
      point_add(&pt, &bezier->points[prev_nr].p3);
      bezier->points[prev_nr].p2 = pt;
      break;
    case BEZ_CORNER_SMOOTH: {
      real len;

      pt = bezier->points[prev_nr].p2;
      point_sub(&pt, &bezier->points[prev_nr].p3);
      len = point_len(&pt);

      pt = bezier->points[prev_nr].p3;
      point_sub(&pt, &bezier->points[comp_nr].p1);
      if (point_len(&pt) > 0)
	point_normalize(&pt);
      else {
	pt.x = 1.0; pt.y = 0.0;
      }
      point_scale(&pt, len);
      point_add(&pt, &bezier->points[prev_nr].p3);
      bezier->points[prev_nr].p2 = pt;
      break;
    }
    case BEZ_CORNER_CUSP:
      /* no mirror point movement required */
      break;
    }
    break;
  default:
    message_error("Internal error in beziershape_move_handle.");
    break;
  }
  return NULL;
}

ObjectChange*
beziershape_move(BezierShape *bezier, Point *to)
{
  Point p;
  int i;
  
  p = *to;
  point_sub(&p, &bezier->points[0].p1);

  bezier->points[0].p1 = bezier->points[0].p3 = *to;
  for (i = 1; i < bezier->numpoints; i++) {
    point_add(&bezier->points[i].p1, &p);
    point_add(&bezier->points[i].p2, &p);
    point_add(&bezier->points[i].p3, &p);
  }

  return NULL;
}

int
beziershape_closest_segment(BezierShape *bezier, Point *point, real line_width)
{
  Point last;
  int i;
  real dist = G_MAXDOUBLE;
  int closest;

  closest = 0;
  last = bezier->points[0].p1;
  /* the first point is just move-to so there is no need to consider p2,p3 of it */
  for (i = 1; i < bezier->numpoints; i++) {
    real new_dist = distance_bez_seg_point(&last, &bezier->points[i].p1,
			&bezier->points[i].p2, &bezier->points[i].p3,
			line_width, point);
    if (new_dist < dist) {
      dist = new_dist;
      closest = i;
    }
    last = bezier->points[i].p3;
  }
  return closest;
}

Handle *
beziershape_closest_handle(BezierShape *bezier, Point *point)
{
  int i, hn;
  real dist = G_MAXDOUBLE;
  Handle *closest = NULL;
  
  for (i = 1, hn = 0; i < bezier->numpoints; i++, hn++) {
    real new_dist;

    new_dist = distance_point_point( point, &bezier->points[i].p1);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }
    hn++;

    new_dist = distance_point_point( point, &bezier->points[i].p2);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }

    hn++;
    new_dist = distance_point_point( point, &bezier->points[i].p3);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }
  }
  return closest;
}

Handle *
beziershape_closest_major_handle(BezierShape *bezier, Point *point)
{
  Handle *closest = beziershape_closest_handle(bezier, point);
  int pos = get_major_nr(get_handle_nr(bezier, closest));

  if (pos == 0)
    pos = bezier->numpoints - 1;
  return bezier->object.handles[3*pos - 1];
}

real
beziershape_distance_from(BezierShape *bezier, Point *point, real line_width)
{
  return distance_bez_shape_point(bezier->points, bezier->numpoints,
				  line_width, point);
}

static void
add_handles(BezierShape *bezier, int pos, BezPoint *point,
	    BezCornerType corner_type, Handle *handle1,
	    Handle *handle2, Handle *handle3,
	    ConnectionPoint *cp1, ConnectionPoint *cp2)
{
  int i, next;
  DiaObject *obj;

  g_assert(pos >= 1);
  g_assert(pos <= bezier->numpoints);

  obj = (DiaObject *)bezier;
  bezier->numpoints++;
  next = pos + 1;
  if (pos == bezier->numpoints - 1)
    next = 1;
  bezier->points = g_realloc(bezier->points,
			     bezier->numpoints * sizeof(BezPoint));
  bezier->corner_types = g_realloc(bezier->corner_types,
				   bezier->numpoints * sizeof(BezCornerType));

  for (i = bezier->numpoints - 1; i > pos; i--) {
    bezier->points[i] = bezier->points[i-1];
    bezier->corner_types[i] =bezier->corner_types[i-1];
  }
  bezier->points[pos] = *point;
  bezier->points[pos].p1 = bezier->points[next].p1;
  bezier->points[next].p1 = point->p1;
  if (pos == bezier->numpoints - 1)
    bezier->points[0].p1 = bezier->points[0].p3 = bezier->points[pos].p3;
  bezier->corner_types[pos] = corner_type;
  object_add_handle_at((DiaObject*)bezier, handle1, 3*pos-3);
  object_add_handle_at((DiaObject*)bezier, handle2, 3*pos-2);
  object_add_handle_at((DiaObject*)bezier, handle3, 3*pos-1);
  object_add_connectionpoint_at((DiaObject *)bezier, cp1, 2*pos-2);
  object_add_connectionpoint_at((DiaObject *)bezier, cp2, 2*pos-1);
}

static void
remove_handles(BezierShape *bezier, int pos)
{
  int i;
  DiaObject *obj;
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *old_cp1, *old_cp2;
  Point tmppoint;
  Point controlvector;
  controlvector.x=0;
  controlvector.y=0;

  g_assert(pos > 0);
  g_assert(pos < bezier->numpoints);

  obj = (DiaObject *)bezier;

  /* delete the points */
  bezier->numpoints--;
  tmppoint = bezier->points[pos].p1;
  if (pos == bezier->numpoints) {
    controlvector = bezier->points[pos-1].p3;
    point_sub(&controlvector, &bezier->points[pos].p1);
  }
  for (i = pos; i < bezier->numpoints; i++) {
    bezier->points[i] = bezier->points[i+1];
    bezier->corner_types[i] = bezier->corner_types[i+1];
  }
  bezier->points[pos].p1 = tmppoint;
  if (pos == bezier->numpoints) {
    /* If this was the last point, we also need to move points[0] and
       the control point in points[1]. */
    bezier->points[0].p1 = bezier->points[bezier->numpoints-1].p3;
    bezier->points[1].p1 = bezier->points[0].p1;
    point_sub(&bezier->points[1].p1, &controlvector);
  }
  bezier->points = g_realloc(bezier->points,
			     bezier->numpoints * sizeof(BezPoint));
  bezier->corner_types = g_realloc(bezier->corner_types,
				   bezier->numpoints * sizeof(BezCornerType));

  old_handle1 = obj->handles[3*pos-3];
  old_handle2 = obj->handles[3*pos-2];
  old_handle3 = obj->handles[3*pos-1];
  object_remove_handle(&bezier->object, old_handle1);
  object_remove_handle(&bezier->object, old_handle2);
  object_remove_handle(&bezier->object, old_handle3);
  old_cp1 = obj->connections[2*pos-2];
  old_cp2 = obj->connections[2*pos-1];
  object_remove_connectionpoint(&bezier->object, old_cp1);
  object_remove_connectionpoint(&bezier->object, old_cp2);
}


/* Add a point by splitting segment into two, putting the new point at
 'point' or, if NULL, in the middle */
ObjectChange *
beziershape_add_segment(BezierShape *bezier, int segment, Point *point)
{
  BezPoint realpoint;
  BezCornerType corner_type = BEZ_CORNER_SYMMETRIC;
  Handle *new_handle1, *new_handle2, *new_handle3;
  ConnectionPoint *new_cp1, *new_cp2;
  Point startpoint;
  Point other;

  if (segment != 1)
    startpoint = bezier->points[segment-1].p3;
  else 
    startpoint = bezier->points[0].p1;
  other = bezier->points[segment].p3;
  if (point == NULL) {
    realpoint.p1.x = (startpoint.x + other.x)/6;
    realpoint.p1.y = (startpoint.y + other.y)/6;
    realpoint.p2.x = (startpoint.x + other.x)/3;
    realpoint.p2.y = (startpoint.y + other.y)/3;
    realpoint.p3.x = (startpoint.x + other.x)/2;
    realpoint.p3.y = (startpoint.y + other.y)/2;
  } else {
    realpoint.p2.x = point->x+(startpoint.x-other.x)/6;
    realpoint.p2.y = point->y+(startpoint.y-other.y)/6;

    realpoint.p3 = *point;
    /* this really goes into the next segment ... */
    realpoint.p1.x = point->x-(startpoint.x-other.x)/6;
    realpoint.p1.y = point->y-(startpoint.y-other.y)/6;
  }
  realpoint.type = BEZ_CURVE_TO;

  new_handle1 = g_new(Handle, 1);
  new_handle2 = g_new(Handle, 1);
  new_handle3 = g_new(Handle, 1);
  setup_handle(new_handle1, HANDLE_RIGHTCTRL);
  setup_handle(new_handle2, HANDLE_LEFTCTRL);
  setup_handle(new_handle3, HANDLE_BEZMAJOR);
  new_cp1 = g_new0(ConnectionPoint, 1);
  new_cp2 = g_new0(ConnectionPoint, 1);
  new_cp1->object = &bezier->object;
  new_cp2->object = &bezier->object;
  add_handles(bezier, segment, &realpoint, corner_type,
	      new_handle1, new_handle2, new_handle3, new_cp1, new_cp2);
  return beziershape_create_point_change(bezier, TYPE_ADD_POINT,
					 &realpoint, corner_type, segment,
					 new_handle1, new_handle2, new_handle3,
					 new_cp1, new_cp2);
}

ObjectChange *
beziershape_remove_segment(BezierShape *bezier, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *old_cp1, *old_cp2;
  BezPoint old_point;
  BezCornerType old_ctype;
  int next = pos+1;

  g_assert(pos > 0);
  g_assert(bezier->numpoints > 2);
  g_assert(pos < bezier->numpoints);

  if (pos == bezier->numpoints - 1)
    next = 1;

  old_handle1 = bezier->object.handles[3*pos-3];
  old_handle2 = bezier->object.handles[3*pos-2];
  old_handle3 = bezier->object.handles[3*pos-1];
  old_point = bezier->points[pos];
  /* remember the old contro point of following bezpoint */
  old_point.p1 = bezier->points[next].p1;
  old_ctype = bezier->corner_types[pos];

  old_cp1 = bezier->object.connections[2*pos-2];
  old_cp2 = bezier->object.connections[2*pos-1];
  
  object_unconnect((DiaObject *)bezier, old_handle1);
  object_unconnect((DiaObject *)bezier, old_handle2);
  object_unconnect((DiaObject *)bezier, old_handle3);

  remove_handles(bezier, pos);

  beziershape_update_data(bezier);
  
  return beziershape_create_point_change(bezier, TYPE_REMOVE_POINT,
					 &old_point, old_ctype, pos,
					 old_handle1, old_handle2, old_handle3,
					 old_cp1, old_cp2);
}

static void
beziershape_straighten_corner(BezierShape *bez, int comp_nr) {
  int next_nr;

  if (comp_nr == 0) comp_nr = bez->numpoints - 1;
  next_nr = comp_nr + 1;
  if (comp_nr == bez->numpoints - 1)
    next_nr = 1;
  /* Neat thing would be to have the kind of straigthening depend on
     which handle was chosen:  Mid-handle does average, other leaves that
     handle where it is. */
  bez->points[0].p3 = bez->points[0].p1;
  switch (bez->corner_types[comp_nr]) {
  case BEZ_CORNER_SYMMETRIC: {
    Point pt1, pt2;

    pt1 = bez->points[comp_nr].p3;
    point_sub(&pt1, &bez->points[comp_nr].p2);
    pt2 = bez->points[comp_nr].p3;
    point_sub(&pt2, &bez->points[next_nr].p1);
    point_scale(&pt2, -1.0);
    point_add(&pt1, &pt2);
    point_scale(&pt1, 0.5);
    pt2 = pt1;
    point_scale(&pt1, -1.0);
    point_add(&pt1, &bez->points[comp_nr].p3);
    point_add(&pt2, &bez->points[comp_nr].p3);
    bez->points[comp_nr].p2 = pt1;
    bez->points[next_nr].p1 = pt2;
    beziershape_update_data(bez);
  }
  break;
  case BEZ_CORNER_SMOOTH: {
    Point pt1, pt2;
    real len1, len2;
    pt1 = bez->points[comp_nr].p3;
    point_sub(&pt1, &bez->points[comp_nr].p2);
    pt2 = bez->points[comp_nr].p3;
    point_sub(&pt2, &bez->points[next_nr].p1);
    len1 = point_len(&pt1);
    len2 = point_len(&pt2);
    point_scale(&pt2, -1.0);
    if (len1 > 0)
      point_normalize(&pt1);
    if (len2 > 0)
      point_normalize(&pt2);
    point_add(&pt1, &pt2);
    point_scale(&pt1, 0.5);
    pt2 = pt1;
    point_scale(&pt1, -len1);
    point_add(&pt1, &bez->points[comp_nr].p3);
    point_scale(&pt2, len2);
    point_add(&pt2, &bez->points[comp_nr].p3);
    bez->points[comp_nr].p2 = pt1;
    bez->points[next_nr].p1 = pt2;
    beziershape_update_data(bez);
  }
    break;
  case BEZ_CORNER_CUSP:
    break;
  }
  bez->points[0].p1 = bez->points[0].p3;
}

ObjectChange *
beziershape_set_corner_type(BezierShape *bez, Handle *handle,
			    BezCornerType corner_type)
{
  Handle *mid_handle = NULL;
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
    if (handle_nr == bez->object.num_handles) handle_nr = 0;
    mid_handle = bez->object.handles[handle_nr];
    break;
  case HANDLE_RIGHTCTRL:
    handle_nr--;
    if (handle_nr < 0) handle_nr = bez->object.num_handles - 1;
    mid_handle = bez->object.handles[handle_nr];
    break;
  default:
    g_assert_not_reached();
    break;
  }

  comp_nr = get_major_nr(handle_nr);

  old_type = bez->corner_types[comp_nr];
  old_left = bez->points[comp_nr].p2;
  if (comp_nr == bez->numpoints - 1)
    old_right = bez->points[1].p1;
  else
    old_right = bez->points[comp_nr+1].p1;

#if 0
  g_message("Setting corner type on segment %d to %s", comp_nr,
	    corner_type == BEZ_CORNER_SYMMETRIC ? "symmetric" :
	    corner_type == BEZ_CORNER_SMOOTH ? "smooth" : "cusp");
#endif
  bez->corner_types[comp_nr] = corner_type;
  if (comp_nr == 0)
    bez->corner_types[bez->numpoints-1] = corner_type;
  else if (comp_nr == bez->numpoints - 1)
    bez->corner_types[0] = corner_type;

  beziershape_straighten_corner(bez, comp_nr);

  return beziershape_create_corner_change(bez, mid_handle, &old_left,
					  &old_right, old_type, corner_type);
}

void
beziershape_update_data(BezierShape *bezier)
{
  int i;
  Point last;
  Point minp, maxp;
  
  DiaObject *obj = &bezier->object;
  
  /* handle the case of whole points array update (via set_prop) */
  if (3*(bezier->numpoints-1) != obj->num_handles ||
      2*(bezier->numpoints-1) + 1 != obj->num_connections) {
    object_unconnect_all(obj); /* too drastic ? */

    /* delete the old ones */
    for (i = 0; i < obj->num_handles; i++)
      g_free(obj->handles[i]);
    g_free(obj->handles);
    for (i = 0; i < obj->num_connections; i++);
      g_free(obj->connections[i]);
    g_free(obj->connections);

    obj->num_handles = 3*(bezier->numpoints-1);
    obj->handles = g_new(Handle*, obj->num_handles);
    obj->num_connections = 2*(bezier->numpoints-1) + 1;
    obj->connections = g_new(ConnectionPoint *, obj->num_connections);

    new_handles_and_connections(bezier, bezier->numpoints);

    bezier->corner_types = g_realloc(bezier->corner_types, bezier->numpoints*sizeof(BezCornerType));
    for (i = 0; i < bezier->numpoints; i++)
      bezier->corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  /* Update handles: */
  bezier->points[0].p3 = bezier->points[0].p1;
  for (i = 1; i < bezier->numpoints; i++) {
    obj->handles[3*i-3]->pos = bezier->points[i].p1;
    obj->handles[3*i-2]->pos = bezier->points[i].p2;
    obj->handles[3*i-1]->pos = bezier->points[i].p3;
  }

  /* Update connection points: */
  last = bezier->points[0].p1;
  for (i = 1; i < bezier->numpoints; i++) {
    Point slopepoint1, slopepoint2;
    slopepoint1 = bezier->points[i].p1;
    point_sub(&slopepoint1, &last);
    point_scale(&slopepoint1, .5);
    point_add(&slopepoint1, &last);
    slopepoint2 = bezier->points[i].p2;
    point_sub(&slopepoint2, &bezier->points[i].p3);
    point_scale(&slopepoint2, .5);
    point_add(&slopepoint2, &bezier->points[i].p3);

    obj->connections[2*i-2]->pos = last;
    obj->connections[2*i-2]->directions =
      find_slope_directions(last, bezier->points[i].p1);
    obj->connections[2*i-1]->pos.x =
      (last.x + 3*bezier->points[i].p1.x + 3*bezier->points[i].p2.x +
       bezier->points[i].p3.x)/8;
    obj->connections[2*i-1]->pos.y =
      (last.y + 3*bezier->points[i].p1.y + 3*bezier->points[i].p2.y +
       bezier->points[i].p3.y)/8;
    obj->connections[2*i-1]->directions = 
      find_slope_directions(slopepoint1, slopepoint2);
    last = bezier->points[i].p3;
  }
  
  /* Find the middle of the object (or some approximation at least) */
  minp = maxp = bezier->points[0].p1;
  for (i = 1; i < bezier->numpoints; i++) {
    Point p = bezier->points[i].p3;
    if (p.x < minp.x) minp.x = p.x;
    if (p.x > maxp.x) maxp.x = p.x;
    if (p.y < minp.y) minp.y = p.y;
    if (p.y > maxp.y) maxp.y = p.y;
  }
  obj->connections[obj->num_connections-1]->pos.x = (minp.x + maxp.x) / 2;
  obj->connections[obj->num_connections-1]->pos.y = (minp.y + maxp.y) / 2;
  obj->connections[obj->num_connections-1]->directions = DIR_ALL;
}

void
beziershape_update_boundingbox(BezierShape *bezier)
{
  ElementBBExtras *extra;
  PolyBBExtras pextra;

  g_assert(bezier != NULL);

  extra = &bezier->extra_spacing;
  pextra.start_trans = pextra.end_trans = 
    pextra.start_long = pextra.end_long = 0;
  pextra.middle_trans = extra->border_trans;

  polybezier_bbox(&bezier->points[0],
                  bezier->numpoints,
                  &pextra, TRUE,
                  &bezier->object.bounding_box);
}

void
beziershape_simple_draw(BezierShape *bezier, DiaRenderer *renderer, real width)
{
  BezPoint *points;
  
  g_assert(bezier != NULL);
  g_assert(renderer != NULL);

  points = &bezier->points[0];
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, width);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_ROUND);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  DIA_RENDERER_GET_CLASS(renderer)->fill_bezier(renderer, points, bezier->numpoints,&color_white);
  DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer, points, bezier->numpoints,&color_black);
}

void
beziershape_draw_control_lines(BezierShape *bez, DiaRenderer *renderer)
{
  Color line_colour = {0.0, 0.0, 0.6};
  Point startpoint;
  int i;
  
  /* setup renderer ... */
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, 0);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_DOTTED);
  DIA_RENDERER_GET_CLASS(renderer)->set_dashlength(renderer, 1);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  startpoint = bez->points[0].p1;
  for (i = 1; i < bez->numpoints; i++) {
    DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &startpoint, &bez->points[i].p1,
                             &line_colour);
    DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bez->points[i].p2, &bez->points[i].p3,
                             &line_colour);
    startpoint = bez->points[i].p3;
  }
}

static void
new_handles_and_connections(BezierShape *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  for (i = 0; i < num_points-1; i++) {
    obj->handles[3*i] = g_new(Handle, 1);
    obj->handles[3*i+1] = g_new(Handle, 1);
    obj->handles[3*i+2] = g_new(Handle, 1);
  
    obj->handles[3*i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3*i]->connected_to = NULL;
    obj->handles[3*i]->type = HANDLE_MINOR_CONTROL;
    obj->handles[3*i]->id = HANDLE_RIGHTCTRL;

    obj->handles[3*i+1]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3*i+1]->connected_to = NULL;
    obj->handles[3*i+1]->type = HANDLE_MINOR_CONTROL;
    obj->handles[3*i+1]->id = HANDLE_LEFTCTRL;
  
    obj->handles[3*i+2]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3*i+2]->connected_to = NULL;
    obj->handles[3*i+2]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[3*i+2]->id = HANDLE_BEZMAJOR;

    obj->connections[2*i] = g_new0(ConnectionPoint, 1);
    obj->connections[2*i+1] = g_new0(ConnectionPoint, 1);
    obj->connections[2*i]->object = obj;
    obj->connections[2*i+1]->object = obj;
    obj->connections[2*i]->flags = 0;
    obj->connections[2*i+1]->flags = 0;
  }

  /** Main point */
  obj->connections[obj->num_connections-1] = g_new0(ConnectionPoint, 1);
  obj->connections[obj->num_connections-1]->object = obj;
  obj->connections[obj->num_connections-1]->flags = CP_FLAGS_MAIN;
}

void
beziershape_init(BezierShape *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  object_init(obj, 3*(num_points-1), 2*(num_points-1) + 1);
  
  bezier->numpoints = num_points;

  bezier->points = g_new(BezPoint, num_points);
  bezier->points[0].type = BEZ_MOVE_TO;
  bezier->corner_types = g_new(BezCornerType, num_points);
  for (i = 1; i < num_points; i++) {
    bezier->points[i].type = BEZ_CURVE_TO;
    bezier->corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  new_handles_and_connections(bezier, num_points);

  /* The points are not assigned at this point, so don't try to use
     them */
  /*  beziershape_update_data(bezier);*/
}


/** This function does *not* set up handles */
void
beziershape_set_points(BezierShape *bez, int num_points, BezPoint *points)
{
  int i;

  bez->numpoints = num_points;

  if (bez->points)
    g_free(bez->points);

  bez->points = g_malloc((bez->numpoints)*sizeof(BezPoint));

  for (i=0;i<bez->numpoints;i++) {
    bez->points[i] = points[i];
  }
}

void
beziershape_copy(BezierShape *from, BezierShape *to)
{
  int i;
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  to->numpoints = from->numpoints;

  to->points = g_new(BezPoint, to->numpoints);
  to->corner_types = g_new(BezCornerType, to->numpoints);

  for (i = 0; i < to->numpoints; i++) {
    to->points[i] = from->points[i];
    to->corner_types[i] = from->corner_types[i];
  }

  for (i = 0; i < toobj->num_handles; i++) {
    toobj->handles[i] = g_new(Handle, 1);
    setup_handle(toobj->handles[i], fromobj->handles[i]->id);
  }
  for (i = 0; i < toobj->num_connections; i++) {
    toobj->connections[i] = g_new0(ConnectionPoint, 1);
    toobj->connections[i]->object = &to->object;
    toobj->connections[i]->flags = fromobj->connections[i]->flags;
  }

  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));  
  beziershape_update_data(to);
}

void
beziershape_destroy(BezierShape *bezier)
{
  int i;
  Handle **temp_handles;
  ConnectionPoint **temp_cps;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  temp_handles = g_new(Handle *, bezier->object.num_handles);
  for (i = 0; i < bezier->object.num_handles; i++)
    temp_handles[i] = bezier->object.handles[i];

  temp_cps = g_new(ConnectionPoint *, bezier->object.num_connections);
  for (i = 0; i < bezier->object.num_connections; i++)
    temp_cps[i] = bezier->object.connections[i];
  
  object_destroy(&bezier->object);

  for (i = 0; i < bezier->object.num_handles; i++)
    g_free(temp_handles[i]);
  g_free(temp_handles);

  for (i = 0; i < bezier->object.num_connections; i++)
    g_free(temp_cps[i]);
  g_free(temp_cps);
  
  g_free(bezier->points);
  g_free(bezier->corner_types);
}


void
beziershape_save(BezierShape *bezier, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;

  object_save(&bezier->object, obj_node);

  attr = new_attribute(obj_node, "bez_points");

  data_add_point(attr, &bezier->points[0].p1);
  for (i = 1; i < bezier->numpoints; i++) {
    data_add_point(attr, &bezier->points[i].p1);
    data_add_point(attr, &bezier->points[i].p2);
    if (i < bezier->numpoints - 1)
      data_add_point(attr, &bezier->points[i].p3);
  }

  attr = new_attribute(obj_node, "corner_types");
  for (i = 0; i < bezier->numpoints; i++)
    data_add_enum(attr, bezier->corner_types[i]);
}

void
beziershape_load(BezierShape *bezier, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;
  DataNode data;
  
  DiaObject *obj = &bezier->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "bez_points");

  if (attr != NULL)
    bezier->numpoints = attribute_num_data(attr) / 3 + 1;
  else
    bezier->numpoints = 0;

  object_init(obj, 3 * (bezier->numpoints - 1),
	      2 * (bezier->numpoints - 1) + 1);

  data = attribute_first_data(attr);
  if (bezier->numpoints != 0) {
    bezier->points = g_new(BezPoint, bezier->numpoints);
    bezier->points[0].type = BEZ_MOVE_TO;
    data_point(data, &bezier->points[0].p1);
    bezier->points[0].p3 = bezier->points[0].p1;
    data = data_next(data);

    for (i = 1; i < bezier->numpoints; i++) {
      bezier->points[i].type = BEZ_CURVE_TO;
      data_point(data, &bezier->points[i].p1);
      data = data_next(data);
      data_point(data, &bezier->points[i].p2);
      data = data_next(data);
      if (i < bezier->numpoints - 1) {
	data_point(data, &bezier->points[i].p3);
	data = data_next(data);
      } else
	bezier->points[i].p3 = bezier->points[0].p1;
    }
  }

  bezier->corner_types = g_new(BezCornerType, bezier->numpoints);
  attr = object_find_attribute(obj_node, "corner_types");
  if (!attr || attribute_num_data(attr) != bezier->numpoints) {
    for (i = 0; i < bezier->numpoints; i++)
      bezier->corner_types[i] = BEZ_CORNER_SYMMETRIC;
  } else {
    data = attribute_first_data(attr);
    for (i = 0; i < bezier->numpoints; i++) {
      bezier->corner_types[i] = data_enum(data);
      data = data_next(data);
    }
  }

  for (i = 0; i < bezier->numpoints - 1; i++) {
    obj->handles[3*i] = g_new(Handle, 1);
    obj->handles[3*i+1] = g_new(Handle, 1);
    obj->handles[3*i+2]   = g_new(Handle, 1);

    setup_handle(obj->handles[3*i], HANDLE_RIGHTCTRL);
    setup_handle(obj->handles[3*i+1], HANDLE_LEFTCTRL);
    setup_handle(obj->handles[3*i+2],   HANDLE_BEZMAJOR);
  }
  for (i = 0; i < obj->num_connections; i++) {
    obj->connections[i] = g_new0(ConnectionPoint, 1);
    obj->connections[i]->object = obj;
  }
  obj->connections[obj->num_connections-1]->flags = CP_FLAGS_MAIN;

  beziershape_update_data(bezier);
}

static void
beziershape_point_change_free(struct BezPointChange *change)
{
  if ( (change->type==TYPE_ADD_POINT && !change->applied) ||
       (change->type==TYPE_REMOVE_POINT && change->applied) ){
    g_free(change->handle1);
    g_free(change->handle2);
    g_free(change->handle3);
    g_free(change->cp1);
    g_free(change->cp2);
    change->handle1 = NULL;
    change->handle2 = NULL;
    change->handle3 = NULL;
    change->cp1 = NULL;
    change->cp2 = NULL;
  }
}

static void
beziershape_point_change_apply(struct BezPointChange *change, DiaObject *obj)
{
  change->applied = 1;
  switch (change->type) {
  case TYPE_ADD_POINT:
    add_handles((BezierShape *)obj, change->pos, &change->point,
		change->corner_type,
		change->handle1, change->handle2, change->handle3,
		change->cp1, change->cp2);
    break;
  case TYPE_REMOVE_POINT:
    object_unconnect(obj, change->handle1);
    object_unconnect(obj, change->handle2);
    object_unconnect(obj, change->handle3);
    remove_handles((BezierShape *)obj, change->pos);
    break;
  }
}

static void
beziershape_point_change_revert(struct BezPointChange *change, DiaObject *obj)
{
  switch (change->type) {
  case TYPE_ADD_POINT:
    remove_handles((BezierShape *)obj, change->pos);
    break;
  case TYPE_REMOVE_POINT:
    add_handles((BezierShape *)obj, change->pos, &change->point,
		change->corner_type,
		change->handle1, change->handle2, change->handle3,
		change->cp1, change->cp2);
    break;
  }
  change->applied = 0;
}

static ObjectChange *
beziershape_create_point_change(BezierShape *bezier, enum change_type type,
				BezPoint *point, BezCornerType corner_type,
				int pos,
				Handle *handle1, Handle *handle2,
				Handle *handle3,
				ConnectionPoint *cp1, ConnectionPoint *cp2)
{
  struct BezPointChange *change;

  change = g_new(struct BezPointChange, 1);

  change->obj_change.apply =
    (ObjectChangeApplyFunc)beziershape_point_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc)beziershape_point_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc)beziershape_point_change_free;

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->corner_type = corner_type;
  change->pos = pos;
  change->handle1 = handle1;
  change->handle2 = handle2;
  change->handle3 = handle3;
  change->cp1 = cp1;
  change->cp2 = cp2;

  return (ObjectChange *)change;
}

static void
beziershape_corner_change_apply(struct CornerChange *change, DiaObject *obj)
{
  BezierShape *bez = (BezierShape *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  beziershape_straighten_corner(bez, comp_nr);

  bez->corner_types[comp_nr] = change->new_type;
  if (comp_nr == 0)
    bez->corner_types[bez->numpoints-1] = change->new_type;
  if (comp_nr == bez->numpoints - 1)
    bez->corner_types[0] = change->new_type;

  change->applied = 1;
}

static void
beziershape_corner_change_revert(struct CornerChange *change, DiaObject *obj)
{
  BezierShape *bez = (BezierShape *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  bez->points[comp_nr].p2 = change->point_left;
  if (comp_nr == bez->numpoints - 1)
    bez->points[1].p1 = change->point_right;
  else
    bez->points[comp_nr+1].p1 = change->point_right;
  bez->corner_types[comp_nr] = change->old_type;  
  if (comp_nr == 0)
    bez->corner_types[bez->numpoints-1] = change->new_type;
  if (comp_nr == bez->numpoints - 1)
    bez->corner_types[0] = change->new_type;

  change->applied = 0;
}

static ObjectChange *
beziershape_create_corner_change(BezierShape *bez, Handle *handle,
				 Point *point_left, Point *point_right,
				 BezCornerType old_corner_type,
				 BezCornerType new_corner_type)
{
  struct CornerChange *change;

  change = g_new(struct CornerChange, 1);

  change->obj_change.apply =
    (ObjectChangeApplyFunc)beziershape_corner_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc)beziershape_corner_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc)NULL;

  change->old_type = old_corner_type;
  change->new_type = new_corner_type;
  change->applied = 1;

  change->handle = handle;
  change->point_left = *point_left;
  change->point_right = *point_right;

  return (ObjectChange *)change;
}
