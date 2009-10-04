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
#include "intl.h"
#include "message.h"
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
bezierconn_create_point_change(BezierConn *bez, enum change_type type,
			       BezPoint *point, BezCornerType corner_type,
			       int segment,
			       Handle *handle1, ConnectionPoint *connected_to1,
			       Handle *handle2, ConnectionPoint *connected_to2,
			       Handle *handle3, ConnectionPoint *connected_to3);
static ObjectChange *
bezierconn_create_corner_change(BezierConn *bez, Handle *handle,
				Point *point_left, Point *point_right,
				BezCornerType old_corner_type,
				BezCornerType new_corner_type);

/** Set up a handle for any part of a bezierconn
 * @param handle A handle to set up.
 * @param id Handle id (HANDLE_BEZMAJOR or HANDLE_BEZMINOER)
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
 * @param bez A bezierconn object with handles set up.
 * @param handle A handle object.
 * @returns The index in bex->object.handles of the handle object, or -1 if
 *          `handle' is not in the array.
 */
static int
get_handle_nr(BezierConn *bez, Handle *handle)
{
  int i = 0;
  for (i=0;i<bez->object.num_handles;i++) {
    if (bez->object.handles[i] == handle)
      return i;
  }
  return -1;
}

void new_handles(BezierConn *bez, int num_points);

#define get_comp_nr(hnum) (((int)(hnum)+2)/3)
#define get_major_nr(hnum) (((int)(hnum)+1)/3)

/** Function called to move one of the handles associated with the
 *  bezierconn. 
 * @param obj The object whose handle is being moved.
 * @param handle The handle being moved.
 * @param pos The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 * @param cp If non-NULL, the connectionpoint found at this position.
 *   If @a cp is NULL, there may or may not be a connectionpoint.
 * @param reason ignored
 * @param modifiers ignored
 * @return NULL
 */
ObjectChange*
bezierconn_move_handle(BezierConn *bez, Handle *handle,
		       Point *to, ConnectionPoint *cp,
		       HandleMoveReason reason, ModifierKeys modifiers)
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
      switch (bez->corner_types[comp_nr]) {
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
	if (point_len(&pt) > 0)
	  point_normalize(&pt);
	else { pt.x = 1.0; pt.y = 0.0; }	  
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
      switch (bez->corner_types[comp_nr-1]) {
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
	if (point_len(&pt) > 0)
	  point_normalize(&pt);
	else { pt.x = 1.0; pt.y = 0.0; }	  
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
  return NULL;
}


/** Function called to move the entire object.
 * @param obj The object being moved.
 * @param pos Where the object is being moved to.  This is the first point
 * of the points array.
 * @return NULL
 */
ObjectChange*
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
  return NULL;
}


/** Return the segment of the bezierconn closest to a given point.
 * @param bez The bezierconn object
 * @param point A point to find the closest segment to.
 * @param line_width Line width of the bezier line.
 * @returns The index of the segment closest to point.
 */
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

/** Return the handle closest to a given point.
 * @param bez A bezierconn object
 * @param point A point to find distances from
 * @return The handle on `bez' closest to `point'.
 * @bug Why isn't this just a function on object that scans the handles?
 */
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

/** Retrun the major handle for the control point with the handle closest to
 * a given point.
 * @param bez A bezier connection
 * @param point A point
 * @return The major (middle) handle of the bezier control that has the
 * handle closest to point.
 * @bug Don't we really want the major handle that's actually closest to
 * the point?  This is used in connection with object menus and could cause
 * some unexpected selection of handles if a different segment has a control
 * point close to the major handle.
 */
Handle *
bezierconn_closest_major_handle(BezierConn *bez, Point *point)
{
  Handle *closest = bezierconn_closest_handle(bez, point);

  return bez->object.handles[3*get_major_nr(get_handle_nr(bez, closest))];
}

/** Return the distance from a bezier to a point.
 * @param bez A bezierconn object.
 * @param point A point to compare with.
 * @param line_width The line width of the bezier line.
 * @returns The shortest distance from the point to any part of the bezier.
 */
real
bezierconn_distance_from(BezierConn *bez, Point *point, real line_width)
{
  return distance_bez_line_point(bez->points, bez->numpoints,
				 line_width, point);
}

/** Add a trio of handles to a bezier.
 * @param bez The bezierconn having handles added.
 * @param pos Where in the list of segments to add the handle
 * @param point The bezier point to add.  This should already be initialized
 *              with sensible positions.
 * @param corner_type What kind of corner this bezpoint should be.
 * @param handle1 The handle that will be put on the bezier line.
 * @param handle2 The handle that will be put before handle1
 * @param handle3 The handle that will be put after handle1
 * @bug check that the handle ordering is correctly described.
 */
static void
add_handles(BezierConn *bez, int pos, BezPoint *point,
	    BezCornerType corner_type, Handle *handle1,
	    Handle *handle2, Handle *handle3)
{
  int i;
  DiaObject *obj;

  g_assert(pos > 0);
  
  obj = (DiaObject *)bez;
  bez->numpoints++;
  bez->points = g_realloc(bez->points, bez->numpoints*sizeof(BezPoint));
  bez->corner_types = g_realloc(bez->corner_types,
				bez->numpoints * sizeof(BezCornerType));

  for (i = bez->numpoints-1; i > pos; i--) {
    bez->points[i] = bez->points[i-1];
    bez->corner_types[i] = bez->corner_types[i-1];
  }
  bez->points[pos] = *point; 
  bez->points[pos].p1 = bez->points[pos+1].p1;
  bez->points[pos+1].p1 = point->p1;
  bez->corner_types[pos] = corner_type;
  object_add_handle_at(obj, handle1, 3*pos-2);
  object_add_handle_at(obj, handle2, 3*pos-1);
  object_add_handle_at(obj, handle3, 3*pos);

  if (pos==bez->numpoints-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MINOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_BEZMAJOR;
  }
}

/** Remove a trio of handles from a bezierconn.
 * @param bez The bezierconn to remove handles from.
 * @param pos The position in the bezierpoint array to remove handles and
 *            bezpoint at.
 */
static void
remove_handles(BezierConn *bez, int pos)
{
  int i;
  DiaObject *obj;
  Handle *old_handle1, *old_handle2, *old_handle3;
  Point tmppoint;

  g_assert(pos > 0);

  obj = (DiaObject *)bez;

  if (pos==obj->num_handles-1) {
    obj->handles[obj->num_handles-4]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[obj->num_handles-4]->id = HANDLE_MOVE_ENDPOINT;
  }

  /* delete the points */
  bez->numpoints--;
  tmppoint = bez->points[pos].p1;
  for (i = pos; i < bez->numpoints; i++) {
    bez->points[i] = bez->points[i+1];
    bez->corner_types[i] = bez->corner_types[i+1];
  }
  bez->points[pos].p1 = tmppoint;
  bez->points = g_realloc(bez->points, bez->numpoints*sizeof(BezPoint));
  bez->corner_types = g_realloc(bez->corner_types,
				bez->numpoints * sizeof(BezCornerType));

  old_handle1 = obj->handles[3*pos-2];
  old_handle2 = obj->handles[3*pos-1];
  old_handle3 = obj->handles[3*pos];
  object_remove_handle(&bez->object, old_handle1);
  object_remove_handle(&bez->object, old_handle2);
  object_remove_handle(&bez->object, old_handle3);
}


/** Add a point by splitting segment into two, putting the new point at
 * 'point' or, if NULL, in the middle.  This function will attempt to come
 * up with reasonable placements for the control points.
 * @param bez The bezierconn to add the segment to.
 * @param segment Which segment to split.
 * @param point Where to put the new corner, or NULL if undetermined.
 * @returns An ObjectChange object with undo information for the split.
 */
ObjectChange *
bezierconn_add_segment(BezierConn *bez, int segment, Point *point)
{
  BezPoint realpoint;
  BezCornerType corner_type = BEZ_CORNER_SYMMETRIC;
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

  new_handle1 = g_new0(Handle,1);
  new_handle2 = g_new0(Handle,1);
  new_handle3 = g_new0(Handle,1);
  setup_handle(new_handle1, HANDLE_RIGHTCTRL);
  setup_handle(new_handle2, HANDLE_LEFTCTRL);
  setup_handle(new_handle3, HANDLE_BEZMAJOR);
  add_handles(bez, segment+1, &realpoint, corner_type,
	      new_handle1, new_handle2, new_handle3);
  return bezierconn_create_point_change(bez, TYPE_ADD_POINT,
					&realpoint, corner_type, segment+1,
					new_handle1, NULL,
					new_handle2, NULL,
					new_handle3, NULL);
}

/** Remove a segment from a bezierconn.
 * @param bez The bezierconn to remove a segment from.
 * @param pos The index of the segment to remove.
 * @returns Undo information for the segment removal.
 */
ObjectChange *
bezierconn_remove_segment(BezierConn *bez, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *cpt1, *cpt2, *cpt3;
  BezPoint old_point;
  BezCornerType old_ctype;

  g_assert(pos > 0);
  g_assert(bez->numpoints > 2);

  if (pos == bez->numpoints-1) pos--;

  old_handle1 = bez->object.handles[3*pos-2];
  old_handle2 = bez->object.handles[3*pos-1];
  old_handle3 = bez->object.handles[3*pos];
  old_point = bez->points[pos];
  old_ctype = bez->corner_types[pos];

  cpt1 = old_handle1->connected_to;
  cpt2 = old_handle2->connected_to;
  cpt3 = old_handle3->connected_to;
  
  object_unconnect((DiaObject *)bez, old_handle1);
  object_unconnect((DiaObject *)bez, old_handle2);
  object_unconnect((DiaObject *)bez, old_handle3);

  remove_handles(bez, pos);

  bezierconn_update_data(bez);
  
  return bezierconn_create_point_change(bez, TYPE_REMOVE_POINT,
					&old_point, old_ctype, pos,
					old_handle1, cpt1,
					old_handle2, cpt2,
					old_handle3, cpt3);
}

/** Update a corner to have less freedom in its control handles, arranging
 * the control points at some reasonable places.
 * @param bez A bezierconn to straighten a corner of
 * @param comp_nr The index into the corner_types array of the corner to
 *                straighten.
 * @bug what happens if we're going from symmetric to smooth?
 */
static void
bezierconn_straighten_corner(BezierConn *bez, int comp_nr)
{
  /* Neat thing would be to have the kind of straigthening depend on
     which handle was chosen:  Mid-handle does average, other leaves that
     handle where it is. */
  switch (bez->corner_types[comp_nr]) {
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
    bez->points[comp_nr+1].p1 = pt2;
    bezierconn_update_data(bez);
  }
    break;
  case BEZ_CORNER_CUSP:
    break;
  }
}

/** Change the corner type of a bezier line.
 * @param bez The bezierconn that has the corner
 * @param handle The handle whose corner should be set.
 * @param corner_type What type of corner the handle should have.
 * @returns Undo information about the corner change.
 */
ObjectChange *
bezierconn_set_corner_type(BezierConn *bez, Handle *handle,
			   BezCornerType corner_type)
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

  comp_nr = get_major_nr(handle_nr);

  old_type = bez->corner_types[comp_nr];
  old_left = bez->points[comp_nr].p2;
  old_right = bez->points[comp_nr+1].p1;

  bez->corner_types[comp_nr] = corner_type;

  bezierconn_straighten_corner(bez, comp_nr);

  return bezierconn_create_corner_change(bez, mid_handle, &old_left, &old_right,
					 old_type, corner_type);
}

/** Update handle array and handle positions after changes.
 * @param bez A bezierconn to update.
 */
void
bezierconn_update_data(BezierConn *bez)
{
  int i;
  DiaObject *obj = &bez->object;
  
  /* handle the case of whole points array update (via set_prop) */
  if (3*bez->numpoints-2 != obj->num_handles) {
    g_assert(0 == obj->num_connections);

    for (i = 0; i < obj->num_handles; i++)
      g_free(obj->handles[i]);
    g_free(obj->handles);

    obj->num_handles = 3*bez->numpoints-2;
    obj->handles = g_new(Handle*, obj->num_handles); 

    new_handles(bez, bez->numpoints);
  }

  /* Update handles: */
  bez->object.handles[0]->pos = bez->points[0].p1;
  for (i = 1; i < bez->numpoints; i++) {
    bez->object.handles[3*i-2]->pos = bez->points[i].p1;
    bez->object.handles[3*i-1]->pos = bez->points[i].p2;
    bez->object.handles[3*i]->pos   = bez->points[i].p3;
  }
}

/** Update the boundingbox of the connection.
 * @param bez A bezier line to update bounding box for.
 */
void
bezierconn_update_boundingbox(BezierConn *bez)
{
  g_assert(bez != NULL);

  polybezier_bbox(&bez->points[0],
                  bez->numpoints,
                  &bez->extra_spacing, FALSE,
                  &bez->object.bounding_box);
}

/** Draw the main line of a bezier conn.
 * Note that this sets the linestyle, linejoin and linecaps to hardcoded
 * values.
 * @param bez The bezier conn to draw.
 * @param renderer The renderer to draw with.
 * @param width The linewidth of the bezier.
 */
void
bezierconn_simple_draw(BezierConn *bez, DiaRenderer *renderer, real width)
{
  BezPoint *points;
  
  g_assert(bez != NULL);
  g_assert(renderer != NULL);

  points = &bez->points[0];
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, width);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_ROUND);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  if (DIA_RENDERER_GET_CLASS(renderer)->is_capable_to(renderer, RENDER_HOLES)) {
    DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer, points, bez->numpoints, &color_black);
  } else {
    int i, from = 0, len;
    
    do {
      for (i = from+1; i < bez->numpoints; ++i)
        if (points[i].type == BEZ_MOVE_TO)
          break;
      len = i - from;
      DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer, &points[from], len, &color_black);
      from += len;
    } while (from < bez->numpoints);
  }
}

/** Draw the control lines from the points of the bezier conn.
 * Note that the control lines are hardcoded to be dotted with dash length 1.
 * @param bez The bezier conn to draw control lines for.
 * @param renderer A renderer to draw with.
 */
void
bezierconn_draw_control_lines(BezierConn *bez, DiaRenderer *renderer)
{
  Color line_colour = {0.0, 0.0, 0.6};
  Point startpoint;
  int i;
  
  /* setup DiaRenderer ... */
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

/** Create all handles used by a bezier conn.
 * @param bez A bezierconn object initialized with room for 3*num_points-2
 * handles.
 * @param num_points The number of points of the bezierconn.
 */
void
new_handles(BezierConn *bez, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bez->object;

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

/** Initialize a bezierconn object with the given amount of points.
 * The points array of the bezierconn object should be previously 
 * initialized with appropriate positions.
 * This will set up handles and make all corners symmetric.
 * @param bez A newly allocated bezierconn object.
 * @param num_points The initial number of points on the curve.
 */
void
bezierconn_init(BezierConn *bez, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bez->object;

  object_init(obj, 3*num_points-2, 0);
  
  bez->numpoints = num_points;

  bez->points = g_new(BezPoint, num_points);
  bez->corner_types = g_new(BezCornerType, num_points);
  bez->points[0].type = BEZ_MOVE_TO;
  bez->corner_types[0] = BEZ_CORNER_SYMMETRIC;
  for (i = 1; i < num_points; i++) {
    bez->points[i].type = BEZ_CURVE_TO;
    bez->corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  new_handles(bez, num_points);

  bezierconn_update_data(bez);
}

/** Set a bezierconn to use the given array of points.
 * This function does *not* set up handles
 * @param bez A bezierconn to operate on
 * @param num_points The number of points in the `points' array.
 * @param points The new points that this bezier should be set to use.
 */
void
bezierconn_set_points(BezierConn *bez, int num_points, BezPoint *points)
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


/** Copy a bezierconn objects.  This function in turn invokes object_copy.
 * @param from The object to copy from.
 * @param to The object to copy to.
 */
void
bezierconn_copy(BezierConn *from, BezierConn *to)
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
 * @param bez An object to destroy.  This function in turn calls object_destroy
 * and frees structures allocated by this bezierconn, but not the object itself
 */
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
  g_free(bez->corner_types);
}


/** Save the data defined by a bezierconn object to XML.
 * @param bez The object to save.
 * @param obj_node The XML node to save it into
 * @bug shouldn't this have version?
 */
void
bezierconn_save(BezierConn *bez, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;

  object_save(&bez->object, obj_node);

  attr = new_attribute(obj_node, "bez_points");
  
  data_add_point(attr, &bez->points[0].p1);
  for (i = 1; i < bez->numpoints; i++) {
    if (BEZ_MOVE_TO == bez->points[i].type)
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
    data_add_point(attr, &bez->points[i].p1);
    data_add_point(attr, &bez->points[i].p2);
    data_add_point(attr, &bez->points[i].p3);
  }

  attr = new_attribute(obj_node, "corner_types");
  for (i = 0; i < bez->numpoints; i++)
    data_add_enum(attr, bez->corner_types[i]);
}

/** Load a bezierconn object from XML.
 * Does object_init() on the bezierconn object.
 * @param bez A newly allocated bezierconn object to load into.
 * @param obj_node The XML node to load from.
 * @bug shouldn't this have version?
 * @bug Couldn't this use the setup_handles function defined above?
 */
void
bezierconn_load(BezierConn *bez, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;
  DataNode data;
  
  DiaObject *obj = &bez->object;

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

  bez->corner_types = g_new(BezCornerType, bez->numpoints);

  attr = object_find_attribute(obj_node, "corner_types");
  /* if corner_types is missing or corrupt */
  if (!attr || attribute_num_data(attr) != bez->numpoints) {
    for (i = 0; i < bez->numpoints; i++) {
      bez->corner_types[i] = BEZ_CORNER_SYMMETRIC;
    }
  } else {
    data = attribute_first_data(attr);
    for (i = 0; i < bez->numpoints; i++) {
      bez->corner_types[i] = data_enum(data);
      data = data_next(data);
    }
  }

  obj->handles[0] = g_new0(Handle,1);
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  
  for (i = 1; i < bez->numpoints; i++) {
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

  bezierconn_update_data(bez);
}

/*** Undo support ***/

/** Free undo information about adding or removing points.
 * @param change The undo information to free.
 */
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

/** Apply a point addition/removal.
 * @param change The change to apply.
 * @param obj The object (must be a BezierConn) to apply the change to.
 */
static void
bezierconn_point_change_apply(struct PointChange *change, DiaObject *obj)
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
bezierconn_point_change_revert(struct PointChange *change, DiaObject *obj)
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
 * @param bez The object that the change applies to (ignored)
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
 * @bug Describe what the state of the point and handles should be at start.
 * @bug Can these handles be connected to anything at all?
 */
static ObjectChange *
bezierconn_create_point_change(BezierConn *bez, enum change_type type,
			       BezPoint *point, BezCornerType corner_type,
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
  BezierConn *bez = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  bezierconn_straighten_corner(bez, comp_nr);

  bez->corner_types[comp_nr] = change->new_type;

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
  BezierConn *bez = (BezierConn *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  bez->points[comp_nr].p2 = change->point_left;
  bez->points[comp_nr+1].p1 = change->point_right;
  bez->corner_types[comp_nr] = change->old_type;  

  change->applied = 0;
}

/** Create new undo information about a changing the type of a corner.
 * Note that the created ObjectChange object has nothing in it that needs
 * freeing.
 * @param bez The bezierconn object this applies to.
 * @param handle The handle of the corner being changed.
 * @param point_left The position of the left control handle.
 * @param point_right The position of the right control handle.
 * @param old_corner_type The corner type before applying.
 * @param new_corner_type The corner type being changed to.
 * @returns Newly allocated undo information.
 */
static ObjectChange *
bezierconn_create_corner_change(BezierConn *bez, Handle *handle,
				Point *point_left, Point *point_right,
				BezCornerType old_corner_type,
				BezCornerType new_corner_type)
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
