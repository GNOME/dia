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

 /** \file bezier_conn.c Allows to construct object consisting of bezier lines */

#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy() */

#include "bezier_conn.h"
#include "diarenderer.h"

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
  BezCornerType corner_type;
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
  BezCornerType old_type, new_type;
};

static ObjectChange *
bezierconn_create_point_change (BezierConn *bezier,
				enum change_type type,
				BezPoint *point,
				BezCornerType corner_type,
				int segment,
				Handle *handle1,
				ConnectionPoint *connected_to1,
				Handle *handle2,
				ConnectionPoint *connected_to2,
				Handle *handle3,
				ConnectionPoint *connected_to3);
static ObjectChange *
bezierconn_create_corner_change(BezierConn *bezier,
				Handle *handle,
				Point *point_left,
				Point *point_right,
				BezCornerType old_corner_type,
				BezCornerType new_corner_type);

/*!
 * \brief Set up a handle for any part of a bezier
 * @param handle A handle to set up.
 * @param id Handle id (HANDLE_BEZMAJOR or HANDLE_RIGHTCTRL or HANDLE_LEFTCTRL)
 */
static void
setup_handle(Handle *handle, HandleId id)
{
  handle->id = id;
  handle->type = HANDLE_MINOR_CONTROL;
  handle->connect_type = (id == HANDLE_BEZMAJOR) ?
      HANDLE_CONNECTABLE : HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}

/** Get the number in the array of handles that a given handle has.
 * @param bezier A bezier object with handles set up.
 * @param handle A handle object.
 * @returns The index in bezier->object.handles of the handle object, or -1 if
 *          `handle' is not in the array.
 */
static int
get_handle_nr (BezierConn *bezier, Handle *handle)
{
  int i = 0;
  for (i = 0; i < bezier->object.num_handles; i++) {
    if (bezier->object.handles[i] == handle)
      return i;
  }
  return -1;
}

#define get_comp_nr(hnum) (((int)(hnum)+2)/3)
#define get_major_nr(hnum) (((int)(hnum)+1)/3)

void new_handles(BezierConn *bezier, int num_points);

/*!
 * \brief Move one of the handles associated with the
 * @param bezier The object whose handle is being moved.
 * @param handle The handle being moved.
 * @param to The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 * @param cp If non-NULL, the connectionpoint found at this position.
 *   If @a cp is NULL, there may or may not be a connectionpoint.
 * @param reason ignored
 * @param modifiers ignored
 * @return NULL
 * \memberof BezierConn
 */
ObjectChange*
bezierconn_move_handle (BezierConn *bezier,
			Handle *handle,
			Point *to,
			ConnectionPoint *cp,
			HandleMoveReason reason,
			ModifierKeys modifiers)
{
  int handle_nr, comp_nr;
  Point delta, pt;
  
  delta = *to;
  point_sub(&delta, &handle->pos);

  handle_nr = get_handle_nr(bezier, handle);
  comp_nr = get_comp_nr(handle_nr);
  switch(handle->id) {
  case HANDLE_MOVE_STARTPOINT:
    bezier->bezier.points[0].p1 = *to;
    /* shift adjacent point */
    point_add(&bezier->bezier.points[1].p1, &delta);
    break;
  case HANDLE_MOVE_ENDPOINT:
    bezier->bezier.points[bezier->bezier.num_points-1].p3 = *to;
    /* shift adjacent point */
    point_add(&bezier->bezier.points[bezier->bezier.num_points-1].p2, &delta);
    break;
  case HANDLE_BEZMAJOR:
    bezier->bezier.points[comp_nr].p3 = *to;
    /* shift adjacent point */
    point_add(&bezier->bezier.points[comp_nr].p2, &delta);
    point_add(&bezier->bezier.points[comp_nr+1].p1, &delta);
    break;
  case HANDLE_LEFTCTRL:
    bezier->bezier.points[comp_nr].p2 = *to;
    if (comp_nr < bezier->bezier.num_points - 1) {
      switch (bezier->bezier.corner_types[comp_nr]) {
      case BEZ_CORNER_SYMMETRIC:
	pt = bezier->bezier.points[comp_nr].p3;
	point_sub(&pt, &bezier->bezier.points[comp_nr].p2);
	point_add(&pt, &bezier->bezier.points[comp_nr].p3);
	bezier->bezier.points[comp_nr+1].p1 = pt;
	break;
      case BEZ_CORNER_SMOOTH: {
	real len;
	pt = bezier->bezier.points[comp_nr+1].p1;
	point_sub(&pt, &bezier->bezier.points[comp_nr].p3);
	len = point_len(&pt);
	pt = bezier->bezier.points[comp_nr].p2;
	point_sub(&pt, &bezier->bezier.points[comp_nr].p3);
	if (point_len(&pt) > 0)
	  point_normalize(&pt);
	else { pt.x = 1.0; pt.y = 0.0; }
	point_scale(&pt, -len);
	point_add(&pt, &bezier->bezier.points[comp_nr].p3);
	bezier->bezier.points[comp_nr+1].p1 = pt;
	break;
      }	
      case BEZ_CORNER_CUSP:
	/* Do nothing to the other guy */
	break;
      }
    }
    break;
  case HANDLE_RIGHTCTRL:
    bezier->bezier.points[comp_nr].p1 = *to;
    if (comp_nr > 1) {
      switch (bezier->bezier.corner_types[comp_nr-1]) {
      case BEZ_CORNER_SYMMETRIC:
	pt = bezier->bezier.points[comp_nr - 1].p3;
	point_sub(&pt, &bezier->bezier.points[comp_nr].p1);
	point_add(&pt, &bezier->bezier.points[comp_nr - 1].p3);
	bezier->bezier.points[comp_nr-1].p2 = pt;
	break;
      case BEZ_CORNER_SMOOTH: {
	real len;
	pt = bezier->bezier.points[comp_nr-1].p2;
	point_sub(&pt, &bezier->bezier.points[comp_nr-1].p3);
	len = point_len(&pt);
	pt = bezier->bezier.points[comp_nr].p1;
	point_sub(&pt, &bezier->bezier.points[comp_nr-1].p3);
	if (point_len(&pt) > 0)
	  point_normalize(&pt);
	else { pt.x = 1.0; pt.y = 0.0; }
	point_scale(&pt, -len);
	point_add(&pt, &bezier->bezier.points[comp_nr-1].p3);
	bezier->bezier.points[comp_nr-1].p2 = pt;
	break;
      }	
      case BEZ_CORNER_CUSP:
	/* Do nothing to the other guy */
	break;
      }
    }
    break;
  default:
    g_warning("Internal error in bezierconn_move_handle.\n");
    break;
  }
  return NULL;
}

/*!
 * \brief Move the entire object.
 * @param bezier The object being moved.
 * @param to Where the object is being moved to.  This is the first point
 * of the points array.
 * @return NULL
 * \memberof _BezierConn
 */
ObjectChange*
bezierconn_move (BezierConn *bezier, Point *to)
{
  Point p;
  int i;
  
  p = *to;
  point_sub(&p, &bezier->bezier.points[0].p1);

  bezier->bezier.points[0].p1 = *to;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    point_add(&bezier->bezier.points[i].p1, &p);
    point_add(&bezier->bezier.points[i].p2, &p);
    point_add(&bezier->bezier.points[i].p3, &p);
  }

  return NULL;
}

/*!
 * \brief Return the handle closest to a given point.
 * @param bezier A bezier object
 * @param point A point to find distances from
 * @return The handle on `bezier' closest to `point'.
 *
 * \memberof _BezierConn
 */
Handle *
bezierconn_closest_handle (BezierConn *bezier,
			   Point *point)
{
  int i, hn;
  real dist;
  Handle *closest;
  
  closest = bezier->object.handles[0];
  dist = distance_point_point( point, &closest->pos);
  for (i = 1, hn = 1; i < bezier->bezier.num_points; i++, hn++) {
    real new_dist;

    new_dist = distance_point_point(point, &bezier->bezier.points[i].p1);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }
    hn++;

    new_dist = distance_point_point(point, &bezier->bezier.points[i].p2);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }
    hn++;

    new_dist = distance_point_point(point, &bezier->bezier.points[i].p3);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[hn];
    }
  }
  return closest;
}

/*!
 * \brief Return the major handle for the control point with the handle closest to
 * a given point.
 * @param bezier A bezier connection
 * @param point A point
 * @return The major (middle) handle of the bezier control that has the
 * handle closest to point.
 * @bug Don't we really want the major handle that's actually closest to
 * the point?  This is used in connection with object menus and could cause
 * some unexpected selection of handles if a different segment has a control
 * point close to the major handle.
 */
Handle *
bezierconn_closest_major_handle (BezierConn *bezier, Point *point)
{
  Handle *closest = bezierconn_closest_handle(bezier, point);

  return bezier->object.handles[3*get_major_nr(get_handle_nr(bezier, closest))];
}

/*!
 * \brief Return the distance from a bezier to a point.
 * @param bezier A bezier object.
 * @param point A point to compare with.
 * @param line_width The line width of the bezier line.
 * @return The shortest distance from the point to any part of the bezier.
 * \memberof _BezierConn
 */
real
bezierconn_distance_from (BezierConn *bezier, Point *point, real line_width)
{
  return distance_bez_line_point(bezier->bezier.points, bezier->bezier.num_points,
				 line_width, point);
}

/** Add a trio of handles to a bezier.
 * @param bezier The bezierconn having handles added.
 * @param pos Where in the list of segments to add the handle
 * @param point The bezier point to add.  This should already be initialized
 *              with sensible positions.
 * @param corner_type What kind of corner this bezpoint should be.
 * @param handle1 The handle that will be put on the bezier line.
 * @param handle2 The handle that will be put before handle1
 * @param handle3 The handle that will be put after handle1
 */
static void
add_handles (BezierConn *bezier,
	     int pos, BezPoint *point,
	     BezCornerType corner_type,
	     Handle *handle1, Handle *handle2, Handle *handle3)
{
  int i, next;
  DiaObject *obj = &bezier->object;

  g_assert(pos > 0);

  bezier->bezier.num_points++;
  next = pos + 1;
  bezier->bezier.points = g_realloc(bezier->bezier.points, bezier->bezier.num_points*sizeof(BezPoint));
  bezier->bezier.corner_types = g_realloc(bezier->bezier.corner_types,
				   bezier->bezier.num_points * sizeof(BezCornerType));

  for (i = bezier->bezier.num_points - 1; i > pos; i--) {
    bezier->bezier.points[i] = bezier->bezier.points[i-1];
    bezier->bezier.corner_types[i] = bezier->bezier.corner_types[i-1];
  }
  bezier->bezier.points[pos] = *point;
  bezier->bezier.points[pos].p1 = bezier->bezier.points[next].p1;
  bezier->bezier.points[next].p1 = point->p1;
  bezier->bezier.corner_types[pos] = corner_type;
  object_add_handle_at(obj, handle1, 3*pos-2);
  object_add_handle_at(obj, handle2, 3*pos-1);
  object_add_handle_at(obj, handle3, 3*pos);

  if (pos==bezier->bezier.num_points-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MINOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_BEZMAJOR;
  }
}

/** Remove a trio of handles from a bezierconn.
 * @param bezier The bezierconn to remove handles from.
 * @param pos The position in the bezierpoint array to remove handles and
 *            bezpoint at.
 */
static void
remove_handles (BezierConn *bezier, int pos)
{
  int i;
  DiaObject *obj;
  Handle *old_handle1, *old_handle2, *old_handle3;
  Point tmppoint;

  g_assert(pos > 0);

  obj = (DiaObject *)bezier;

  if (pos==obj->num_handles-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_MOVE_ENDPOINT;
  }

  /* delete the points */
  bezier->bezier.num_points--;
  tmppoint = bezier->bezier.points[pos].p1;
  for (i = pos; i < bezier->bezier.num_points; i++) {
    bezier->bezier.points[i] = bezier->bezier.points[i+1];
    bezier->bezier.corner_types[i] = bezier->bezier.corner_types[i+1];
  }
  bezier->bezier.points[pos].p1 = tmppoint;
  bezier->bezier.points = g_realloc(bezier->bezier.points, bezier->bezier.num_points*sizeof(BezPoint));
  bezier->bezier.corner_types = g_realloc(bezier->bezier.corner_types,
				bezier->bezier.num_points * sizeof(BezCornerType));

  old_handle1 = obj->handles[3*pos-2];
  old_handle2 = obj->handles[3*pos-1];
  old_handle3 = obj->handles[3*pos];
  object_remove_handle(&bezier->object, old_handle1);
  object_remove_handle(&bezier->object, old_handle2);
  object_remove_handle(&bezier->object, old_handle3);
}


/** Add a point by splitting segment into two, putting the new point at
 * 'point' or, if NULL, in the middle.  This function will attempt to come
 * up with reasonable placements for the control points.
 * @param bezier The bezierconn to add the segment to.
 * @param segment Which segment to split.
 * @param point Where to put the new corner, or NULL if undetermined.
 * @returns An ObjectChange object with undo information for the split.
 */
ObjectChange *
bezierconn_add_segment (BezierConn *bezier,
			int segment, Point *point)
{
  BezPoint realpoint;
  BezCornerType corner_type = BEZ_CORNER_SYMMETRIC;
  Handle *new_handle1, *new_handle2, *new_handle3;
  Point startpoint;
  Point other;

  if (segment == 0)
    startpoint = bezier->bezier.points[0].p1;
  else
    startpoint = bezier->bezier.points[segment].p3;
  other = bezier->bezier.points[segment+1].p3;

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

  new_handle1 = g_new0(Handle,1);
  new_handle2 = g_new0(Handle,1);
  new_handle3 = g_new0(Handle,1);
  setup_handle(new_handle1, HANDLE_RIGHTCTRL);
  setup_handle(new_handle2, HANDLE_LEFTCTRL);
  setup_handle(new_handle3, HANDLE_BEZMAJOR);
  add_handles(bezier, segment+1, &realpoint, corner_type,
	      new_handle1, new_handle2, new_handle3);
  return bezierconn_create_point_change(bezier, TYPE_ADD_POINT,
					&realpoint, corner_type, segment+1,
					new_handle1, NULL,
					new_handle2, NULL,
					new_handle3, NULL);
}

/*!
 * \brief Remove a segment from a bezier.
 * @param bezier The bezier to remove a segment from.
 * @param pos The index of the segment to remove.
 * @returns Undo information for the segment removal.
 * \memberof _BezierConn
 */
ObjectChange *
bezierconn_remove_segment (BezierConn *bezier, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *cpt1, *cpt2, *cpt3;
  BezPoint old_point;
  BezCornerType old_ctype;
  int next;

  g_assert(pos > 0);
  g_assert(bezier->bezier.num_points > 2);

  if (pos == bezier->bezier.num_points-1)
    pos--;
  next = pos+1;

  old_handle1 = bezier->object.handles[3*pos-2];
  old_handle2 = bezier->object.handles[3*pos-1];
  old_handle3 = bezier->object.handles[3*pos];
  old_point = bezier->bezier.points[pos];
  /* remember the old control point of following bezpoint */
  old_point.p1 = bezier->bezier.points[next].p1;
  old_ctype = bezier->bezier.corner_types[pos];

  cpt1 = old_handle1->connected_to;
  cpt2 = old_handle2->connected_to;
  cpt3 = old_handle3->connected_to;
  
  object_unconnect((DiaObject *)bezier, old_handle1);
  object_unconnect((DiaObject *)bezier, old_handle2);
  object_unconnect((DiaObject *)bezier, old_handle3);

  remove_handles(bezier, pos);

  bezierconn_update_data(bezier);
  
  return bezierconn_create_point_change(bezier, TYPE_REMOVE_POINT,
					&old_point, old_ctype, pos,
					old_handle1, cpt1,
					old_handle2, cpt2,
					old_handle3, cpt3);
}

/*!
 * \brief Limit movability of control handles
 *
 * Update a corner to have less freedom in its control handles, arranging
 * the control points at some reasonable places.
 * @param bezier A bezierconn to straighten a corner of
 * @param comp_nr The index into the corner_types array of the corner to
 *                straighten.
 * \memberof _BezierConn
 */
static void
bezierconn_straighten_corner (BezierConn *bezier, int comp_nr)
{
  int next_nr = comp_nr+1;
  /* Neat thing would be to have the kind of straigthening depend on
     which handle was chosen:  Mid-handle does average, other leaves that
     handle where it is. */
  switch (bezier->bezier.corner_types[comp_nr]) {
  case BEZ_CORNER_SYMMETRIC: {
    Point pt1, pt2;

    pt1 = bezier->bezier.points[comp_nr].p3;
    point_sub(&pt1, &bezier->bezier.points[comp_nr].p2);
    pt2 = bezier->bezier.points[comp_nr].p3;
    point_sub(&pt2, &bezier->bezier.points[next_nr].p1);
    point_scale(&pt2, -1.0);
    point_add(&pt1, &pt2);
    point_scale(&pt1, 0.5);
    pt2 = pt1;
    point_scale(&pt1, -1.0);
    point_add(&pt1, &bezier->bezier.points[comp_nr].p3);
    point_add(&pt2, &bezier->bezier.points[comp_nr].p3);
    bezier->bezier.points[comp_nr].p2 = pt1;
    bezier->bezier.points[next_nr].p1 = pt2;
    bezierconn_update_data(bezier);
  }
  break;
  case BEZ_CORNER_SMOOTH: {
    Point pt1, pt2;
    real len1, len2;
    pt1 = bezier->bezier.points[comp_nr].p3;
    point_sub(&pt1, &bezier->bezier.points[comp_nr].p2);
    pt2 = bezier->bezier.points[comp_nr].p3;
    point_sub(&pt2, &bezier->bezier.points[next_nr].p1);
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
    point_add(&pt1, &bezier->bezier.points[comp_nr].p3);
    point_scale(&pt2, len2);
    point_add(&pt2, &bezier->bezier.points[comp_nr].p3);
    bezier->bezier.points[comp_nr].p2 = pt1;
    bezier->bezier.points[next_nr].p1 = pt2;
    bezierconn_update_data(bezier);
  }
    break;
  case BEZ_CORNER_CUSP:
    break;
  }
}

/** Change the corner type of a bezier line.
 * @param bezier The bezierconn that has the corner
 * @param handle The handle whose corner should be set.
 * @param corner_type What type of corner the handle should have.
 * @returns Undo information about the corner change.
 */
ObjectChange *
bezierconn_set_corner_type (BezierConn *bezier,
			    Handle *handle,
			    BezCornerType corner_type)
{
  Handle *mid_handle;
  Point old_left, old_right;
  int old_type;
  int handle_nr, comp_nr;

  handle_nr = get_handle_nr(bezier, handle);

  switch (handle->id) {
  case HANDLE_BEZMAJOR:
    mid_handle = handle;
    break;
  case HANDLE_LEFTCTRL:
    handle_nr++;
    mid_handle = bezier->object.handles[handle_nr];
    break;
  case HANDLE_RIGHTCTRL:
    handle_nr--;
    mid_handle = bezier->object.handles[handle_nr];
    break;
  default:
    g_warning("Internal error: Setting corner type of endpoint of bezier");
    return NULL;
  }

  comp_nr = get_major_nr(handle_nr);

  old_type = bezier->bezier.corner_types[comp_nr];
  old_left = bezier->bezier.points[comp_nr].p2;
  old_right = bezier->bezier.points[comp_nr+1].p1;

  bezier->bezier.corner_types[comp_nr] = corner_type;

  bezierconn_straighten_corner(bezier, comp_nr);

  return bezierconn_create_corner_change(bezier, mid_handle, &old_left, &old_right,
					 old_type, corner_type);
}

/** Update handle array and handle positions after changes.
 * @param bezier A bezierconn to update.
 */
void
bezierconn_update_data (BezierConn *bezier)
{
  int i;
  DiaObject *obj = &bezier->object;
  
  /* handle the case of whole points array update (via set_prop) */
  if (3*bezier->bezier.num_points-2 != obj->num_handles) {
    /* also maintain potential connections */
    ConnectionPoint *cps = bezier->object.handles[0]->connected_to;
    ConnectionPoint *cpe = bezier->object.handles[obj->num_handles-1]->connected_to;

    g_assert(0 == obj->num_connections);

    if (cps)
      object_unconnect (&bezier->object, bezier->object.handles[0]);
    if (cpe)
      object_unconnect (&bezier->object, bezier->object.handles[obj->num_handles-1]);

    /* delete the old ones */
    for (i = 0; i < obj->num_handles; i++)
      g_free(obj->handles[i]);
    g_free(obj->handles);

    obj->num_handles = 3*bezier->bezier.num_points-2;
    obj->handles = g_new(Handle*, obj->num_handles); 

    new_handles(bezier, bezier->bezier.num_points);
    /* we may assign NULL once more here */
    if (cps)
      object_connect(&bezier->object, bezier->object.handles[0], cps);
    if (cpe)
      object_connect(&bezier->object, bezier->object.handles[obj->num_handles-1], cpe);
  }

  /* Update handles: */
  bezier->object.handles[0]->pos = bezier->bezier.points[0].p1;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    bezier->object.handles[3*i-2]->pos = bezier->bezier.points[i].p1;
    bezier->object.handles[3*i-1]->pos = bezier->bezier.points[i].p2;
    bezier->object.handles[3*i]->pos   = bezier->bezier.points[i].p3;
  }
}

/** Update the boundingbox of the connection.
 * @param bezier A bezier line to update bounding box for.
 */
void
bezierconn_update_boundingbox (BezierConn *bezier)
{
  g_assert(bezier != NULL);

  polybezier_bbox(&bezier->bezier.points[0],
                  bezier->bezier.num_points,
                  &bezier->extra_spacing, FALSE,
                  &bezier->object.bounding_box);
}

/** Create all handles used by a bezier conn.
 * @param bezier A bezierconn object initialized with room for 3*num_points-2
 * handles.
 * @param num_points The number of points of the bezierconn.
 */
void
new_handles (BezierConn *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  obj->handles[0] = g_new0(Handle,1);
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;

  for (i = 1; i < num_points; i++) {
    obj->handles[3*i-2] = g_new0(Handle,1);
    obj->handles[3*i-1] = g_new0(Handle,1);
    obj->handles[3*i] = g_new0(Handle,1);
  
    setup_handle(obj->handles[3*i-2], HANDLE_RIGHTCTRL);
    setup_handle(obj->handles[3*i-1], HANDLE_LEFTCTRL);
  
    obj->handles[3*i]->connect_type = HANDLE_CONNECTABLE;
    obj->handles[3*i]->connected_to = NULL;
    obj->handles[3*i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[3*i]->id = HANDLE_MOVE_ENDPOINT;
  }
}

/** Initialize a bezier object with the given amount of points.
 * The points array of the bezier object might not be previously 
 * initialized with appropriate positions.
 * This will set up handles and make all corners symmetric.
 * @param bezier A newly allocated bezierconn object.
 * @param num_points The initial number of points on the curve.
 */
void
bezierconn_init (BezierConn *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  object_init(obj, 3*num_points-2, 0);
  
  bezier->bezier.num_points = num_points;

  bezier->bezier.points = g_new(BezPoint, num_points);
  bezier->bezier.corner_types = g_new(BezCornerType, num_points);
  bezier->bezier.points[0].type = BEZ_MOVE_TO;
  bezier->bezier.corner_types[0] = BEZ_CORNER_SYMMETRIC;
  for (i = 1; i < num_points; i++) {
    bezier->bezier.points[i].type = BEZ_CURVE_TO;
    bezier->bezier.corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  new_handles(bezier, num_points);

  /* The points might not be assigned at this point,
   * so don't try to use them */
  /* bezierconn_update_data(bezier); */
}

/** Copy a bezierconn objects.  This function in turn invokes object_copy.
 * @param from The object to copy from.
 * @param to The object to copy to.
 */
void
bezierconn_copy (BezierConn *from, BezierConn *to)
{
  int i;
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  beziercommon_copy (&from->bezier, &to->bezier);

  to->object.handles[0] = g_new0(Handle,1);
  *to->object.handles[0] = *from->object.handles[0];
  for (i = 1; i < to->object.num_handles - 1; i++) {
    to->object.handles[i] = g_new0(Handle,1);
    setup_handle(to->object.handles[i], from->object.handles[i]->id);
  }
  to->object.handles[to->object.num_handles-1] = g_new0(Handle,1);
  *to->object.handles[to->object.num_handles-1] =
    *from->object.handles[to->object.num_handles-1];

  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));
  bezierconn_update_data(to);
}

/** Destroy a bezierconn object.
 * @param bezier An object to destroy.  This function in turn calls object_destroy
 * and frees structures allocated by this bezierconn, but not the object itself
 */
void
bezierconn_destroy (BezierConn *bezier)
{
  int i, nh;
  Handle **temp_handles;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  nh = bezier->object.num_handles;
  temp_handles = g_new(Handle *, nh);
  for (i = 0; i < nh; i++)
    temp_handles[i] = bezier->object.handles[i];

  object_destroy(&bezier->object);

  for (i = 0; i < nh; i++)
    g_free(temp_handles[i]);
  g_free(temp_handles);
  
  g_free(bezier->bezier.points);
  g_free(bezier->bezier.corner_types);
}


/** Save the data defined by a bezierconn object to XML.
 * @param bezier The object to save.
 * @param obj_node The XML node to save it into
 * @param ctx The context to transport error information.
 */
void
bezierconn_save (BezierConn *bezier,
		 ObjectNode obj_node,
		 DiaContext *ctx)
{
  int i;
  AttributeNode attr;

  object_save(&bezier->object, obj_node, ctx);

  attr = new_attribute(obj_node, "bez_points");

  data_add_point(attr, &bezier->bezier.points[0].p1, ctx);
  for (i = 1; i < bezier->bezier.num_points; i++) {
    if (BEZ_MOVE_TO == bezier->bezier.points[i].type)
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
    data_add_point(attr, &bezier->bezier.points[i].p1, ctx);
    data_add_point(attr, &bezier->bezier.points[i].p2, ctx);
    data_add_point(attr, &bezier->bezier.points[i].p3, ctx);
  }

  attr = new_attribute(obj_node, "corner_types");
  for (i = 0; i < bezier->bezier.num_points; i++)
    data_add_enum(attr, bezier->bezier.corner_types[i], ctx);
}

/** Load a bezierconn object from XML.
 * Does object_init() on the bezierconn object.
 * @param bezier A newly allocated bezierconn object to load into.
 * @param obj_node The XML node to load from.
 * @param ctx The context in which this function is called
 */
void
bezierconn_load (BezierConn *bezier,
		 ObjectNode obj_node,
		 DiaContext *ctx)
{
  int i;
  AttributeNode attr;
  DataNode data;
  
  DiaObject *obj = &bezier->object;

  object_load(obj, obj_node, ctx);

  attr = object_find_attribute(obj_node, "bez_points");

  if (attr != NULL)
    bezier->bezier.num_points = (attribute_num_data(attr) + 2)/3;
  else
    bezier->bezier.num_points = 0;

  object_init(obj, 3 * bezier->bezier.num_points - 2, 0);

  data = attribute_first_data(attr);
  if (bezier->bezier.num_points != 0) {
    bezier->bezier.points = g_new(BezPoint, bezier->bezier.num_points);
    bezier->bezier.points[0].type = BEZ_MOVE_TO;
    data_point(data, &bezier->bezier.points[0].p1, ctx);
    data = data_next(data);

    for (i = 1; i < bezier->bezier.num_points; i++) {
      bezier->bezier.points[i].type = BEZ_CURVE_TO;
      data_point(data, &bezier->bezier.points[i].p1, ctx);
      data = data_next(data);
      data_point(data, &bezier->bezier.points[i].p2, ctx);
      data = data_next(data);
      data_point(data, &bezier->bezier.points[i].p3, ctx);
      data = data_next(data);
    }
  }

  bezier->bezier.corner_types = g_new(BezCornerType, bezier->bezier.num_points);
  attr = object_find_attribute(obj_node, "corner_types");
  /* if corner_types is missing or corrupt */
  if (!attr || attribute_num_data(attr) != bezier->bezier.num_points) {
    for (i = 0; i < bezier->bezier.num_points; i++)
      bezier->bezier.corner_types[i] = BEZ_CORNER_SYMMETRIC;
  } else {
    data = attribute_first_data(attr);
    for (i = 0; i < bezier->bezier.num_points; i++) {
      bezier->bezier.corner_types[i] = data_enum(data, ctx);
      data = data_next(data);
    }
  }

  obj->handles[0] = g_new0(Handle,1);
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  
  for (i = 1; i < bezier->bezier.num_points; i++) {
    obj->handles[3*i-2] = g_new0(Handle,1);
    setup_handle(obj->handles[3*i-2], HANDLE_RIGHTCTRL);
    obj->handles[3*i-1] = g_new0(Handle,1);
    setup_handle(obj->handles[3*i-1], HANDLE_LEFTCTRL);
    obj->handles[3*i]   = g_new0(Handle,1);
    setup_handle(obj->handles[3*i],   HANDLE_BEZMAJOR);
  }

  obj->handles[obj->num_handles-1]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[obj->num_handles-1]->connected_to = NULL;
  obj->handles[obj->num_handles-1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[obj->num_handles-1]->id = HANDLE_MOVE_ENDPOINT;

  bezierconn_update_data(bezier);
}

/*** Undo support ***/

/** Free undo information about adding or removing points.
 * @param change The undo information to free.
 */
static void
bezierconn_point_change_free (struct PointChange *change)
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

/** Apply a point addition/removal.
 * @param change The change to apply.
 * @param obj The object (must be a BezierConn) to apply the change to.
 */
static void
bezierconn_point_change_apply (struct PointChange *change, DiaObject *obj)
{
  change->applied = 1;
  switch (change->type) {
  case TYPE_ADD_POINT:
    add_handles((BezierConn *)obj, change->pos, &change->point,
		change->corner_type,
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

/** Revert (unapply) a point addition/removal.
 * @param change The change to revert.
 * @param obj The object (must be a BezierConn) to revert the change of.
 */
static void
bezierconn_point_change_revert (struct PointChange *change, DiaObject *obj)
{
  switch (change->type) {
  case TYPE_ADD_POINT:
    remove_handles((BezierConn *)obj, change->pos);
    break;
  case TYPE_REMOVE_POINT:
    add_handles((BezierConn *)obj, change->pos, &change->point,
		change->corner_type,
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

/** Create undo information about a point being added or removed.
 * @param bezier The object that the change applies to (ignored)
 * @param type The type of change (either TYPE_ADD_POINT or TYPE_REMOVE_POINT)
 * @param point The point being added or removed.
 * @param corner_type Which type of corner is at the point.
 * @param pos The position of the point.
 * @param handle1 The first (central) handle.
 * @param connected_to1 What the first handle is connected to.
 * @param handle2 The second (left-hand) handle.
 * @param connected_to2 What the second handle is connected to.
 * @param handle3 The third (right-hand) handle.
 * @param connected_to3 What the third handle is connected to.
 * @return Newly created undo information.
 */
static ObjectChange *
bezierconn_create_point_change(BezierConn *bezier,
			       enum change_type type,
			       BezPoint *point,
			       BezCornerType corner_type,
			       int pos,
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
  change->corner_type = corner_type;
  change->pos = pos;
  change->handle1 = handle1;
  change->connected_to1 = connected_to1;
  change->handle2 = handle2;
  change->connected_to2 = connected_to2;
  change->handle3 = handle3;
  change->connected_to3 = connected_to3;

  return (ObjectChange *)change;
}

/** Apply a change of corner type.  This may change the position of the
 * control handles by calling bezierconn_straighten_corner.
 * @param change The undo information to apply.
 * @param obj The object to apply the undo information too.
 */
static void
bezierconn_corner_change_apply(struct CornerChange *change,
			       DiaObject *obj)
{
  BezierConn *bezier = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bezier, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  bezierconn_straighten_corner(bezier, comp_nr);

  bezier->bezier.corner_types[comp_nr] = change->new_type;

  change->applied = 1;
}

/** Revert (unapply) a change of corner type.  This may move the position
 * of the control handles to what they were before applying.
 * @param change Undo information to apply.
 * @param obj The bezierconn object to apply the change to.
 */
static void
bezierconn_corner_change_revert(struct CornerChange *change,
				DiaObject *obj)
{
  BezierConn *bezier = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bezier, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  bezier->bezier.points[comp_nr].p2 = change->point_left;
  bezier->bezier.points[comp_nr+1].p1 = change->point_right;
  bezier->bezier.corner_types[comp_nr] = change->old_type;  

  change->applied = 0;
}

/** Create new undo information about a changing the type of a corner.
 * Note that the created ObjectChange object has nothing in it that needs
 * freeing.
 * @param bezier The bezier object this applies to.
 * @param handle The handle of the corner being changed.
 * @param point_left The position of the left control handle.
 * @param point_right The position of the right control handle.
 * @param old_corner_type The corner type before applying.
 * @param new_corner_type The corner type being changed to.
 * @returns Newly allocated undo information.
 */
static ObjectChange *
bezierconn_create_corner_change (BezierConn *bezier,
				 Handle *handle,
				 Point *point_left,
				 Point *point_right,
				 BezCornerType old_corner_type,
				 BezCornerType new_corner_type)
{
  struct CornerChange *change;

  change = g_new(struct CornerChange, 1);

  change->obj_change.apply =
	(ObjectChangeApplyFunc) bezierconn_corner_change_apply;
  change->obj_change.revert =
	(ObjectChangeRevertFunc) bezierconn_corner_change_revert;
  change->obj_change.free =
	(ObjectChangeFreeFunc) NULL;

  change->old_type = old_corner_type;
  change->new_type = new_corner_type;
  change->applied = 1;

  change->handle = handle;
  change->point_left = *point_left;
  change->point_right = *point_right;

  return (ObjectChange *)change;
}
