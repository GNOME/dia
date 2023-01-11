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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy() */

#include "beziershape.h"
#include "diarenderer.h"


#define HANDLE_BEZMAJOR  (HANDLE_CUSTOM1)
#define HANDLE_LEFTCTRL  (HANDLE_CUSTOM2)
#define HANDLE_RIGHTCTRL (HANDLE_CUSTOM3)

enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};


static DiaObjectChange *dia_bezier_shape_point_object_change_new  (BezierShape      *bezier,
                                                                   enum change_type  type,
                                                                   BezPoint         *point,
                                                                   BezCornerType     corner_type,
                                                                   int               segment,
                                                                   Handle           *handle1,
                                                                   Handle           *handle2,
                                                                   Handle           *handle3,
                                                                   ConnectionPoint  *cp1,
                                                                   ConnectionPoint  *cp2);
static DiaObjectChange *dia_bezier_shape_corner_object_change_new (BezierShape      *bezier,
                                                                   Handle           *handle,
                                                                   Point            *point_left,
                                                                   Point            *point_right,
                                                                   BezCornerType     old_corner_type,
                                                                   BezCornerType     new_corner_type);
static void             new_handles_and_connections               (BezierShape      *bezier,
                                                                   int               num_points);


/**
 * setup_handle:
 * @handle: A handle to set up.
 * @id: Handle id (%HANDLE_BEZMAJOR or %HANDLE_RIGHTCTRL or %HANDLE_LEFTCTRL)
 *
 * Set up a handle for any part of a bezier
 *
 * Since: dawn-of-time
 */
static void
setup_handle (Handle *handle, HandleId id)
{
  handle->id = id;
  handle->type =
    (id == HANDLE_BEZMAJOR) ?
    HANDLE_MAJOR_CONTROL :
    HANDLE_MINOR_CONTROL;
  handle->connect_type = HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}


/**
 * get_handle_nr:
 * @bezier: A bezier object with handles set up.
 * @handle: A handle object.
 *
 * Get the number in the array of handles that a given handle has.
 *
 * Returnss: The index in bezier->object.handles of the handle object, or -1
 *           if `handle' is not in the array.
 */
static int
get_handle_nr (BezierShape *bezier, Handle *handle)
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


/**
 * beziershape_move_handle:
 * @bezier: The object whose handle is being moved.
 * @handle: The handle being moved.
 * @to: The position it has been moved to (corrected for
 *      vertical/horizontal only movement).
 * @cp: If non-%NULL, the connectionpoint found at this position.
 *      If @cp is %NULL, there may or may not be a connectionpoint.
 * @reason: ignored
 * @modifiers: ignored
 *
 * Move one of the handles associated with the
 *
 * Returns: %NULL
 */
DiaObjectChange *
beziershape_move_handle (BezierShape      *bezier,
                         Handle           *handle,
                         Point            *to,
                         ConnectionPoint  *cp,
                         HandleMoveReason  reason,
                         ModifierKeys      modifiers)
{
  int handle_nr, comp_nr;
  int next_nr, prev_nr;
  Point delta, pt;

  delta = *to;
  point_sub (&delta, &handle->pos);

  handle_nr = get_handle_nr (bezier, handle);
  comp_nr = get_comp_nr (handle_nr);
  next_nr = comp_nr + 1;
  prev_nr = comp_nr - 1;
  if (comp_nr == bezier->bezier.num_points - 1)
    next_nr = 1;
  if (comp_nr == 1)
    prev_nr = bezier->bezier.num_points - 1;

  switch (handle->id) {
    case HANDLE_BEZMAJOR:
      if (comp_nr == bezier->bezier.num_points - 1) {
        bezier->bezier.points[comp_nr].p3 = *to;
        bezier->bezier.points[0].p1 = bezier->bezier.points[0].p3 = *to;
        point_add(&bezier->bezier.points[comp_nr].p2, &delta);
        point_add(&bezier->bezier.points[1].p1, &delta);
      } else {
        bezier->bezier.points[comp_nr].p3 = *to;
        point_add(&bezier->bezier.points[comp_nr].p2, &delta);
        point_add(&bezier->bezier.points[comp_nr+1].p1, &delta);
      }
      break;
    case HANDLE_LEFTCTRL:
      bezier->bezier.points[comp_nr].p2 = *to;
      switch (bezier->bezier.corner_types[comp_nr]) {
        case BEZ_CORNER_SYMMETRIC:
          pt = bezier->bezier.points[comp_nr].p3;
          point_sub (&pt, &bezier->bezier.points[comp_nr].p2);
          point_add (&pt, &bezier->bezier.points[comp_nr].p3);
          bezier->bezier.points[next_nr].p1 = pt;
          break;
        case BEZ_CORNER_SMOOTH: {
            double len;

            pt = bezier->bezier.points[next_nr].p1;
            point_sub (&pt, &bezier->bezier.points[comp_nr].p3);
            len = point_len (&pt);

            pt = bezier->bezier.points[comp_nr].p3;
            point_sub (&pt, &bezier->bezier.points[comp_nr].p2);
            if (point_len (&pt) > 0) {
              point_normalize (&pt);
            } else {
              pt.x = 1.0; pt.y = 0.0;
            }
            point_scale (&pt, len);
            point_add (&pt, &bezier->bezier.points[comp_nr].p3);
            bezier->bezier.points[next_nr].p1 = pt;
            break;
          }
        case BEZ_CORNER_CUSP:
          /* no mirror point movement required */
          break;
        default:
          g_return_val_if_reached (NULL);
      }
      break;
    case HANDLE_RIGHTCTRL:
      bezier->bezier.points[comp_nr].p1 = *to;
      switch (bezier->bezier.corner_types[prev_nr]) {
        case BEZ_CORNER_SYMMETRIC:
          pt = bezier->bezier.points[prev_nr].p3;
          point_sub (&pt, &bezier->bezier.points[comp_nr].p1);
          point_add (&pt, &bezier->bezier.points[prev_nr].p3);
          bezier->bezier.points[prev_nr].p2 = pt;
          break;
        case BEZ_CORNER_SMOOTH: {
            double len;

            pt = bezier->bezier.points[prev_nr].p2;
            point_sub (&pt, &bezier->bezier.points[prev_nr].p3);
            len = point_len (&pt);

            pt = bezier->bezier.points[prev_nr].p3;
            point_sub (&pt, &bezier->bezier.points[comp_nr].p1);
            if (point_len (&pt) > 0) {
              point_normalize(&pt);
            } else {
              pt.x = 1.0;
              pt.y = 0.0;
            }
            point_scale (&pt, len);
            point_add (&pt, &bezier->bezier.points[prev_nr].p3);
            bezier->bezier.points[prev_nr].p2 = pt;
            break;
          }
        case BEZ_CORNER_CUSP:
          /* no mirror point movement required */
          break;
        default:
          g_return_val_if_reached (NULL);
      }
      break;
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SE:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_NW:
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_warning ("Internal error in beziershape_move_handle.");
      break;
  }
  return NULL;
}


/**
 * beziershape_move:
 * @bezier: The object being moved.
 * @to: Where the object is being moved to. This is the first point of the
 *      points array.
 *
 * Move the entire object.
 *
 * Returns: %NULL
 */
DiaObjectChange *
beziershape_move (BezierShape *bezier, Point *to)
{
  Point p;
  int i;

  p = *to;
  point_sub(&p, &bezier->bezier.points[0].p1);

  bezier->bezier.points[0].p1 = bezier->bezier.points[0].p3 = *to;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    point_add(&bezier->bezier.points[i].p1, &p);
    point_add(&bezier->bezier.points[i].p2, &p);
    point_add(&bezier->bezier.points[i].p3, &p);
  }

  return NULL;
}


/**
 * beziershape_closest_handle:
 * @bezier: A bezier object
 * @point: A point to find distances from
 *
 * Return the handle closest to a given point.
 *
 * Returns: The handle on @bezier closest to @point
 */
Handle *
beziershape_closest_handle (BezierShape *bezier,
                            Point       *point)
{
  int i, hn;
  double dist = G_MAXDOUBLE;
  Handle *closest = NULL;

  for (i = 1, hn = 0; i < bezier->bezier.num_points; i++, hn++) {
    double new_dist;

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

Handle *
beziershape_closest_major_handle (BezierShape *bezier, Point *point)
{
  Handle *closest = beziershape_closest_handle(bezier, point);
  int pos = get_major_nr(get_handle_nr(bezier, closest));

  if (pos == 0)
    pos = bezier->bezier.num_points - 1;
  return bezier->object.handles[3*pos - 1];
}


/**
 * beziershape_distance_from:
 * @bezier: A bezier object.
 * @point: A point to compare with.
 * @line_width: The line width of the bezier line.
 *
 * Return the distance from a bezier to a point.
 *
 * Returns: The shortest distance from the point to any part of the bezier.
 *
 * Since: dawn-of-time
 */
double
beziershape_distance_from (BezierShape *bezier,
                           Point       *point,
                           double       line_width)
{
  return distance_bez_shape_point (bezier->bezier.points,
                                   bezier->bezier.num_points,
                                   line_width,
                                   point);
}


static void
add_handles (BezierShape *bezier,
	     int pos, BezPoint *point,
	     BezCornerType corner_type,
	     Handle *handle1, Handle *handle2, Handle *handle3,
	     ConnectionPoint *cp1, ConnectionPoint *cp2)
{
  int i, next;
  DiaObject *obj = &bezier->object;

  g_assert(pos >= 1);
  g_assert(pos <= bezier->bezier.num_points);

  bezier->bezier.num_points++;
  next = pos + 1;
  bezier->bezier.points = g_renew (BezPoint,
                                   bezier->bezier.points,
                                   bezier->bezier.num_points);
  if (pos == bezier->bezier.num_points - 1)
    next = 1;
  bezier->bezier.corner_types = g_renew (BezCornerType,
                                         bezier->bezier.corner_types,
                                         bezier->bezier.num_points);

  for (i = bezier->bezier.num_points - 1; i > pos; i--) {
    bezier->bezier.points[i] = bezier->bezier.points[i-1];
    bezier->bezier.corner_types[i] = bezier->bezier.corner_types[i-1];
  }
  bezier->bezier.points[pos] = *point;
  bezier->bezier.points[pos].p1 = bezier->bezier.points[next].p1;
  bezier->bezier.points[next].p1 = point->p1;
  if (pos == bezier->bezier.num_points - 1)
    bezier->bezier.points[0].p1 = bezier->bezier.points[0].p3 = bezier->bezier.points[pos].p3;
  bezier->bezier.corner_types[pos] = corner_type;
  object_add_handle_at(obj, handle1, 3*pos-3);
  object_add_handle_at(obj, handle2, 3*pos-2);
  object_add_handle_at(obj, handle3, 3*pos-1);
  object_add_connectionpoint_at(obj, cp1, 2*pos-2);
  object_add_connectionpoint_at(obj, cp2, 2*pos-1);
}

static void
remove_handles (BezierShape *bezier, int pos)
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
  g_assert(pos < bezier->bezier.num_points);

  obj = (DiaObject *)bezier;

  /* delete the points */
  bezier->bezier.num_points--;
  tmppoint = bezier->bezier.points[pos].p1;
  if (pos == bezier->bezier.num_points) {
    controlvector = bezier->bezier.points[pos-1].p3;
    point_sub(&controlvector, &bezier->bezier.points[pos].p1);
  }
  for (i = pos; i < bezier->bezier.num_points; i++) {
    bezier->bezier.points[i] = bezier->bezier.points[i+1];
    bezier->bezier.corner_types[i] = bezier->bezier.corner_types[i+1];
  }
  bezier->bezier.points[pos].p1 = tmppoint;
  if (pos == bezier->bezier.num_points) {
    /* If this was the last point, we also need to move points[0] and
       the control point in points[1]. */
    bezier->bezier.points[0].p1 = bezier->bezier.points[bezier->bezier.num_points-1].p3;
    bezier->bezier.points[1].p1 = bezier->bezier.points[0].p1;
    point_sub(&bezier->bezier.points[1].p1, &controlvector);
  }

  bezier->bezier.points = g_renew (BezPoint,
                                   bezier->bezier.points,
                                   bezier->bezier.num_points);
  bezier->bezier.corner_types = g_renew (BezCornerType,
                                         bezier->bezier.corner_types,
                                         bezier->bezier.num_points);

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
DiaObjectChange *
beziershape_add_segment (BezierShape *bezier,
                         int          segment,
                         Point       *point)
{
  BezPoint realpoint;
  BezCornerType corner_type = BEZ_CORNER_SYMMETRIC;
  Handle *new_handle1, *new_handle2, *new_handle3;
  ConnectionPoint *new_cp1, *new_cp2;
  Point startpoint;
  Point other;

  g_return_val_if_fail (segment >= 0 && segment < bezier->bezier.num_points, NULL);

  if (bezier->bezier.points[segment].type == BEZ_CURVE_TO) {
    startpoint = bezier->bezier.points[segment].p3;
  } else {
    startpoint = bezier->bezier.points[segment].p1;
  }

  other = bezier->bezier.points[segment+1].p3;
  if (point == NULL) {
    realpoint.p1.x = (startpoint.x + other.x) / 6;
    realpoint.p1.y = (startpoint.y + other.y) / 6;
    realpoint.p2.x = (startpoint.x + other.x) / 3;
    realpoint.p2.y = (startpoint.y + other.y) / 3;
    realpoint.p3.x = (startpoint.x + other.x) / 2;
    realpoint.p3.y = (startpoint.y + other.y) / 2;
  } else {
    realpoint.p2.x = point->x + (startpoint.x - other.x) / 6;
    realpoint.p2.y = point->y + (startpoint.y - other.y) / 6;

    realpoint.p3 = *point;
    /* this really goes into the next segment ... */
    realpoint.p1.x = point->x - (startpoint.x - other.x) / 6;
    realpoint.p1.y = point->y - (startpoint.y - other.y) / 6;
  }
  realpoint.type = BEZ_CURVE_TO;

  new_handle1 = g_new0 (Handle, 1);
  new_handle2 = g_new0 (Handle, 1);
  new_handle3 = g_new0 (Handle, 1);
  setup_handle (new_handle1, HANDLE_RIGHTCTRL);
  setup_handle (new_handle2, HANDLE_LEFTCTRL);
  setup_handle (new_handle3, HANDLE_BEZMAJOR);
  new_cp1 = g_new0 (ConnectionPoint, 1);
  new_cp2 = g_new0 (ConnectionPoint, 1);
  new_cp1->object = &bezier->object;
  new_cp2->object = &bezier->object;
  add_handles (bezier,
               segment + 1,
               &realpoint,
               corner_type,
               new_handle1,
               new_handle2,
               new_handle3,
               new_cp1,
               new_cp2);

  return dia_bezier_shape_point_object_change_new (bezier,
                                                   TYPE_ADD_POINT,
                                                   &realpoint,
                                                   corner_type,
                                                   segment + 1,
                                                   new_handle1,
                                                   new_handle2,
                                                   new_handle3,
                                                   new_cp1,
                                                   new_cp2);
}


/**
 * beziershape_remove_segment:
 * @bezier: The bezier to remove a segment from.
 * @pos: The index of the segment to remove.
 *
 * Remove a segment from a bezier.
 *
 * Returns: Undo information for the segment removal.
 */
DiaObjectChange *
beziershape_remove_segment (BezierShape *bezier, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *old_cp1, *old_cp2;
  BezPoint old_point;
  BezCornerType old_ctype;
  int next = pos+1;

  g_return_val_if_fail (pos > 0 && pos < bezier->bezier.num_points, NULL);
  g_assert(bezier->bezier.num_points > 2);

  if (pos == bezier->bezier.num_points - 1)
    next = 1;
  else if (next == 1)
    next = bezier->bezier.num_points - 1;

  old_handle1 = bezier->object.handles[3*pos-3];
  old_handle2 = bezier->object.handles[3*pos-2];
  old_handle3 = bezier->object.handles[3*pos-1];
  old_point = bezier->bezier.points[pos];
  /* remember the old control point of following bezpoint */
  old_point.p1 = bezier->bezier.points[next].p1;
  old_ctype = bezier->bezier.corner_types[pos];

  old_cp1 = bezier->object.connections[2*pos-2];
  old_cp2 = bezier->object.connections[2*pos-1];

  object_unconnect((DiaObject *)bezier, old_handle1);
  object_unconnect((DiaObject *)bezier, old_handle2);
  object_unconnect((DiaObject *)bezier, old_handle3);

  remove_handles(bezier, pos);

  beziershape_update_data(bezier);

  return dia_bezier_shape_point_object_change_new (bezier,
                                                   TYPE_REMOVE_POINT,
                                                   &old_point,
                                                   old_ctype,
                                                   pos,
                                                   old_handle1,
                                                   old_handle2,
                                                   old_handle3,
                                                   old_cp1,
                                                   old_cp2);
}


/**
 * beziershape_straighten_corner:
 * @bezier: A bezierconn to straighten a corner of
 * @comp_nr: The index into the corner_types array of the corner to straighten
 *
 * Limit movability of control handles
 *
 * Update a corner to have less freedom in its control handles, arranging
 * the control points at some reasonable places.
 */
static void
beziershape_straighten_corner (BezierShape *bezier, int comp_nr)
{
  int next_nr;

  if (comp_nr == 0) comp_nr = bezier->bezier.num_points - 1;
  next_nr = comp_nr + 1;
  if (comp_nr == bezier->bezier.num_points - 1)
    next_nr = 1;
  /* Neat thing would be to have the kind of straigthening depend on
     which handle was chosen:  Mid-handle does average, other leaves that
     handle where it is. */
  bezier->bezier.points[0].p3 = bezier->bezier.points[0].p1;

  switch (bezier->bezier.corner_types[comp_nr]) {
    case BEZ_CORNER_SYMMETRIC: {
        Point pt1, pt2;

        pt1 = bezier->bezier.points[comp_nr].p3;
        point_sub (&pt1, &bezier->bezier.points[comp_nr].p2);
        pt2 = bezier->bezier.points[comp_nr].p3;
        point_sub (&pt2, &bezier->bezier.points[next_nr].p1);
        point_scale (&pt2, -1.0);
        point_add (&pt1, &pt2);
        point_scale (&pt1, 0.5);
        pt2 = pt1;
        point_scale (&pt1, -1.0);
        point_add (&pt1, &bezier->bezier.points[comp_nr].p3);
        point_add (&pt2, &bezier->bezier.points[comp_nr].p3);
        bezier->bezier.points[comp_nr].p2 = pt1;
        bezier->bezier.points[next_nr].p1 = pt2;
        beziershape_update_data (bezier);
      }
      break;
    case BEZ_CORNER_SMOOTH: {
        Point pt1, pt2;
        double len1, len2;
        pt1 = bezier->bezier.points[comp_nr].p3;
        point_sub (&pt1, &bezier->bezier.points[comp_nr].p2);
        pt2 = bezier->bezier.points[comp_nr].p3;
        point_sub (&pt2, &bezier->bezier.points[next_nr].p1);
        len1 = point_len (&pt1);
        len2 = point_len (&pt2);
        point_scale (&pt2, -1.0);
        if (len1 > 0) {
          point_normalize (&pt1);
        }
        if (len2 > 0) {
          point_normalize (&pt2);
        }
        point_add (&pt1, &pt2);
        point_scale (&pt1, 0.5);
        pt2 = pt1;
        point_scale (&pt1, -len1);
        point_add (&pt1, &bezier->bezier.points[comp_nr].p3);
        point_scale (&pt2, len2);
        point_add (&pt2, &bezier->bezier.points[comp_nr].p3);
        bezier->bezier.points[comp_nr].p2 = pt1;
        bezier->bezier.points[next_nr].p1 = pt2;
        beziershape_update_data (bezier);
      }
      break;
    case BEZ_CORNER_CUSP:
      break;
    default:
      g_return_if_reached ();
  }

  bezier->bezier.points[0].p1 = bezier->bezier.points[0].p3;
}


DiaObjectChange *
beziershape_set_corner_type (BezierShape   *bezier,
                             Handle        *handle,
                             BezCornerType  corner_type)
{
  Handle *mid_handle = NULL;
  Point old_left, old_right;
  int old_type;
  int handle_nr, comp_nr;

  handle_nr = get_handle_nr (bezier, handle);

  switch (handle->id) {
    case HANDLE_BEZMAJOR:
      mid_handle = handle;
      break;
    case HANDLE_LEFTCTRL:
      handle_nr++;
      if (handle_nr == bezier->object.num_handles) handle_nr = 0;
      mid_handle = bezier->object.handles[handle_nr];
      break;
    case HANDLE_RIGHTCTRL:
      handle_nr--;
      if (handle_nr < 0) handle_nr = bezier->object.num_handles - 1;
      mid_handle = bezier->object.handles[handle_nr];
      break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SE:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_NW:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_assert_not_reached();
      break;
  }

  comp_nr = get_major_nr(handle_nr);

  old_type = bezier->bezier.corner_types[comp_nr];
  old_left = bezier->bezier.points[comp_nr].p2;
  if (comp_nr == bezier->bezier.num_points - 1)
    old_right = bezier->bezier.points[1].p1;
  else
    old_right = bezier->bezier.points[comp_nr+1].p1;

#if 0
  g_message("Setting corner type on segment %d to %s", comp_nr,
	    corner_type == BEZ_CORNER_SYMMETRIC ? "symmetric" :
	    corner_type == BEZ_CORNER_SMOOTH ? "smooth" : "cusp");
#endif
  bezier->bezier.corner_types[comp_nr] = corner_type;
  if (comp_nr == 0)
    bezier->bezier.corner_types[bezier->bezier.num_points-1] = corner_type;
  else if (comp_nr == bezier->bezier.num_points - 1)
    bezier->bezier.corner_types[0] = corner_type;

  beziershape_straighten_corner(bezier, comp_nr);

  return dia_bezier_shape_corner_object_change_new (bezier,
                                                    mid_handle,
                                                    &old_left,
                                                    &old_right,
                                                    old_type,
                                                    corner_type);
}


void
beziershape_update_data (BezierShape *bezier)
{
  int i;
  Point last;
  Point minp, maxp;

  DiaObject *obj = &bezier->object;

  /* handle the case of whole points array update (via set_prop) */
  if (3*(bezier->bezier.num_points-1) != obj->num_handles ||
      2*(bezier->bezier.num_points-1) + 1 != obj->num_connections) {
    object_unconnect_all(obj); /* too drastic ? */

    /* delete the old ones */
    for (i = 0; i < obj->num_handles; i++)
      g_clear_pointer (&obj->handles[i], g_free);
    g_clear_pointer (&obj->handles, g_free);
    for (i = 0; i < obj->num_connections; i++)
      g_clear_pointer (&obj->connections[i], g_free);
    g_clear_pointer (&obj->connections, g_free);

    obj->num_handles = 3*(bezier->bezier.num_points-1);
    obj->handles = g_new(Handle*, obj->num_handles);
    obj->num_connections = 2*(bezier->bezier.num_points-1) + 1;
    obj->connections = g_new(ConnectionPoint *, obj->num_connections);

    new_handles_and_connections(bezier, bezier->bezier.num_points);

    bezier->bezier.corner_types = g_renew (BezCornerType,
                                           bezier->bezier.corner_types,
                                           bezier->bezier.num_points);
    for (i = 0; i < bezier->bezier.num_points; i++)
      bezier->bezier.corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  /* Update handles: */
  bezier->bezier.points[0].p3 = bezier->bezier.points[0].p1;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    obj->handles[3*i-3]->pos = bezier->bezier.points[i].p1;
    obj->handles[3*i-2]->pos = bezier->bezier.points[i].p2;
    obj->handles[3*i-1]->pos = bezier->bezier.points[i].p3;
  }

  /* Update connection points: */
  last = bezier->bezier.points[0].p1;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    Point slopepoint1, slopepoint2;
    slopepoint1 = bezier->bezier.points[i].p1;
    point_sub(&slopepoint1, &last);
    point_scale(&slopepoint1, .5);
    point_add(&slopepoint1, &last);
    slopepoint2 = bezier->bezier.points[i].p2;
    point_sub(&slopepoint2, &bezier->bezier.points[i].p3);
    point_scale(&slopepoint2, .5);
    point_add(&slopepoint2, &bezier->bezier.points[i].p3);

    obj->connections[2*i-2]->pos = last;
    obj->connections[2*i-2]->directions =
      find_slope_directions(last, bezier->bezier.points[i].p1);
    if (bezier->bezier.points[i].type == BEZ_CURVE_TO) {
      obj->connections[2*i-1]->pos.x =
        (last.x + 3*bezier->bezier.points[i].p1.x + 3*bezier->bezier.points[i].p2.x +
         bezier->bezier.points[i].p3.x)/8;
      obj->connections[2*i-1]->pos.y =
        (last.y + 3*bezier->bezier.points[i].p1.y + 3*bezier->bezier.points[i].p2.y +
         bezier->bezier.points[i].p3.y)/8;
    } else {
      obj->connections[2*i-1]->pos.x = (last.x + bezier->bezier.points[i].p1.x) / 2;
      obj->connections[2*i-1]->pos.y = (last.y + bezier->bezier.points[i].p1.y) / 2;
    }
    obj->connections[2*i-1]->directions =
      find_slope_directions(slopepoint1, slopepoint2);
    if (bezier->bezier.points[i].type == BEZ_CURVE_TO)
      last = bezier->bezier.points[i].p3;
    else
      last = bezier->bezier.points[i].p1;
  }

  /* Find the middle of the object (or some approximation at least) */
  minp = maxp = bezier->bezier.points[0].p1;
  for (i = 1; i < bezier->bezier.num_points; i++) {
    Point p = bezier->bezier.points[i].p3;
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
beziershape_update_boundingbox (BezierShape *bezier)
{
  ElementBBExtras *extra;
  PolyBBExtras pextra;

  g_assert(bezier != NULL);

  extra = &bezier->extra_spacing;
  pextra.start_trans = pextra.end_trans =
    pextra.start_long = pextra.end_long = 0;
  pextra.middle_trans = extra->border_trans;

  polybezier_bbox(&bezier->bezier.points[0],
                  bezier->bezier.num_points,
                  &pextra, TRUE,
                  &bezier->object.bounding_box);
}


static void
new_handles_and_connections (BezierShape *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  for (i = 0; i < num_points-1; i++) {
    obj->handles[3 * i] = g_new0 (Handle, 1);
    obj->handles[3 * i + 1] = g_new0 (Handle, 1);
    obj->handles[3 * i + 2] = g_new0 (Handle, 1);

    obj->handles[3 * i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3 * i]->connected_to = NULL;
    obj->handles[3 * i]->type = HANDLE_MINOR_CONTROL;
    obj->handles[3 * i]->id = HANDLE_RIGHTCTRL;

    obj->handles[3 * i + 1]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3 * i + 1]->connected_to = NULL;
    obj->handles[3 * i + 1]->type = HANDLE_MINOR_CONTROL;
    obj->handles[3 * i + 1]->id = HANDLE_LEFTCTRL;

    obj->handles[3 * i + 2]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[3 * i + 2]->connected_to = NULL;
    obj->handles[3 * i + 2]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[3 * i + 2]->id = HANDLE_BEZMAJOR;

    obj->connections[2 * i] = g_new0 (ConnectionPoint, 1);
    obj->connections[2 * i + 1] = g_new0 (ConnectionPoint, 1);
    obj->connections[2 * i]->object = obj;
    obj->connections[2 * i + 1]->object = obj;
    obj->connections[2 * i]->flags = 0;
    obj->connections[2 * i + 1]->flags = 0;
  }

  /** Main point */
  obj->connections[obj->num_connections - 1] = g_new0 (ConnectionPoint, 1);
  obj->connections[obj->num_connections - 1]->object = obj;
  obj->connections[obj->num_connections - 1]->flags = CP_FLAGS_MAIN;
}


/**
 * beziershape_init:
 * @bezier: A newly allocated beziershape object.
 * @num_points: The initial number of points on the curve.
 *
 * Initialize a bezier object with the given amount of points.
 * The points array of the bezier object might not be previously
 * initialized with appropriate positions.
 * This will set up handles and make all corners symmetric.
 *
 * Since: dawn-of-time
 */
void
beziershape_init (BezierShape *bezier, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &bezier->object;

  object_init (obj, 3 * (num_points - 1), 2 * (num_points - 1) + 1);

  bezier->bezier.num_points = num_points;

  bezier->bezier.points = g_new (BezPoint, num_points);
  bezier->bezier.corner_types = g_new (BezCornerType, num_points);
  bezier->bezier.points[0].type = BEZ_MOVE_TO;
  bezier->bezier.corner_types[0] = BEZ_CORNER_SYMMETRIC;
  for (i = 1; i < num_points; i++) {
    bezier->bezier.points[i].type = BEZ_CURVE_TO;
    bezier->bezier.corner_types[i] = BEZ_CORNER_SYMMETRIC;
  }

  new_handles_and_connections (bezier, num_points);

  /* The points might not be assigned at this point,
   * so don't try to use them */
  /*  beziershape_update_data(bezier);*/
}


/**
 * beziershape_copy:
 * @from: The object to copy from.
 * @to: The object to copy to.
 *
 * Copy a beziershape objects. This function in turn invokes object_copy().
 *
 * Since: dawn-of-time
 */
void
beziershape_copy (BezierShape *from, BezierShape *to)
{
  int i;
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy (fromobj, toobj);

  beziercommon_copy (&from->bezier, &to->bezier);

  for (i = 0; i < toobj->num_handles; i++) {
    toobj->handles[i] = g_new0 (Handle, 1);
    setup_handle (toobj->handles[i], fromobj->handles[i]->id);
  }

  for (i = 0; i < toobj->num_connections; i++) {
    toobj->connections[i] = g_new0 (ConnectionPoint, 1);
    toobj->connections[i]->object = &to->object;
    toobj->connections[i]->flags = fromobj->connections[i]->flags;
  }

  memcpy (&to->extra_spacing,
          &from->extra_spacing,
          sizeof (to->extra_spacing));
  beziershape_update_data (to);
}


void
beziershape_destroy (BezierShape *bezier)
{
  int i, nh;
  Handle **temp_handles;
  ConnectionPoint **temp_cps;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  nh = bezier->object.num_handles;
  temp_handles = g_new0 (Handle *, nh);
  for (i = 0; i < nh; i++) {
    temp_handles[i] = bezier->object.handles[i];
  }

  temp_cps = g_new0 (ConnectionPoint *, bezier->object.num_connections);
  for (i = 0; i < bezier->object.num_connections; i++) {
    temp_cps[i] = bezier->object.connections[i];
  }

  object_destroy (&bezier->object);

  for (i = 0; i < nh; i++) {
    g_clear_pointer (&temp_handles[i], g_free);
  }
  g_clear_pointer (&temp_handles, g_free);

  for (i = 0; i < bezier->object.num_connections; i++) {
    g_clear_pointer (&temp_cps[i], g_free);
  }
  g_clear_pointer (&temp_cps, g_free);

  g_clear_pointer (&bezier->bezier.points, g_free);
  g_clear_pointer (&bezier->bezier.corner_types, g_free);
}


/**
 * beziershape_save:
 * @bezier: The object to save.
 * @obj_node: The XML node to save it into
 * @ctx: The context to transport error information.
 *
 * Save the data defined by a beziershape object to XML.
 *
 * Since: dawn-of-time
 */
void
beziershape_save (BezierShape *bezier,
                  ObjectNode   obj_node,
                  DiaContext  *ctx)
{
  int i;
  AttributeNode attr;

  object_save (&bezier->object, obj_node, ctx);

  attr = new_attribute (obj_node, "bez_points");

  data_add_point (attr, &bezier->bezier.points[0].p1, ctx);
  for (i = 1; i < bezier->bezier.num_points; i++) {
    if (BEZ_MOVE_TO == bezier->bezier.points[i].type) {
      g_warning ("only first BezPoint can be a BEZ_MOVE_TO");
    }
    data_add_point (attr, &bezier->bezier.points[i].p1, ctx);
    data_add_point (attr, &bezier->bezier.points[i].p2, ctx);
    if (i < bezier->bezier.num_points - 1) {
      data_add_point (attr, &bezier->bezier.points[i].p3, ctx);
    }
  }

  attr = new_attribute (obj_node, "corner_types");
  for (i = 0; i < bezier->bezier.num_points; i++) {
    data_add_enum (attr, bezier->bezier.corner_types[i], ctx);
  }
}


/**
 * beziershape_load:
 * @bezier: A newly allocated bezierconn object to load into.
 * @obj_node: The XML node to load from.
 * @ctx: The context in which this function is called
 *
 * Load a beziershape object from XML.
 *
 * Does object_init() on the bezierconn object.
 *
 * Since: dawn-of-time
 */
void
beziershape_load (BezierShape *bezier,
                  ObjectNode   obj_node,
                  DiaContext  *ctx)
{
  int i;
  AttributeNode attr;
  DataNode data;

  DiaObject *obj = &bezier->object;

  object_load (obj, obj_node, ctx);

  attr = object_find_attribute (obj_node, "bez_points");

  if (attr != NULL) {
    bezier->bezier.num_points = attribute_num_data (attr) / 3 + 1;
  } else {
    bezier->bezier.num_points = 0;
  }

  object_init (obj,
               3 * (bezier->bezier.num_points - 1),
               2 * (bezier->bezier.num_points - 1) + 1);

  data = attribute_first_data (attr);
  if (bezier->bezier.num_points != 0) {
    bezier->bezier.points = g_new (BezPoint, bezier->bezier.num_points);
    bezier->bezier.points[0].type = BEZ_MOVE_TO;
    data_point (data, &bezier->bezier.points[0].p1, ctx);
    bezier->bezier.points[0].p3 = bezier->bezier.points[0].p1;
    data = data_next (data);

    for (i = 1; i < bezier->bezier.num_points; i++) {
      bezier->bezier.points[i].type = BEZ_CURVE_TO;
      data_point (data, &bezier->bezier.points[i].p1, ctx);
      data = data_next (data);
      data_point (data, &bezier->bezier.points[i].p2, ctx);
      data = data_next (data);
      if (i < bezier->bezier.num_points - 1) {
        data_point (data, &bezier->bezier.points[i].p3, ctx);
        data = data_next (data);
      } else {
        bezier->bezier.points[i].p3 = bezier->bezier.points[0].p1;
      }
    }
  }

  bezier->bezier.corner_types = g_new (BezCornerType,
                                       bezier->bezier.num_points);
  attr = object_find_attribute (obj_node, "corner_types");
  /* if corner_types is missing or corrupt */
  if (!attr || attribute_num_data (attr) != bezier->bezier.num_points) {
    for (i = 0; i < bezier->bezier.num_points; i++)
      bezier->bezier.corner_types[i] = BEZ_CORNER_SYMMETRIC;
  } else {
    data = attribute_first_data (attr);
    for (i = 0; i < bezier->bezier.num_points; i++) {
      bezier->bezier.corner_types[i] = data_enum (data, ctx);
      data = data_next (data);
    }
  }

  for (i = 0; i < bezier->bezier.num_points - 1; i++) {
    obj->handles[3 * i]     = g_new0 (Handle, 1);
    obj->handles[3 * i + 1] = g_new0 (Handle, 1);
    obj->handles[3 * i + 2] = g_new0 (Handle, 1);

    setup_handle (obj->handles[3 * i], HANDLE_RIGHTCTRL);
    setup_handle (obj->handles[3 * i + 1], HANDLE_LEFTCTRL);
    setup_handle (obj->handles[3 * i + 2], HANDLE_BEZMAJOR);
  }
  for (i = 0; i < obj->num_connections; i++) {
    obj->connections[i] = g_new0 (ConnectionPoint, 1);
    obj->connections[i]->object = obj;
  }
  obj->connections[obj->num_connections - 1]->flags = CP_FLAGS_MAIN;

  beziershape_update_data (bezier);
}


/* Undo support */


/* Invariant:
   # of handles = 3*(numpoints-1)
   # of connections = 2*(numpoints-1) + 1 (main point)
   For historical reasons, the main point is the last cp.
 */
struct _DiaBezierShapePointObjectChange {
  DiaObjectChange obj_change;

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


DIA_DEFINE_OBJECT_CHANGE (DiaBezierShapePointObjectChange,
                          dia_bezier_shape_point_object_change)


/**
 * dia_bezier_shape_point_object_change_free:
 * @self: The undo information to free.
 *
 * Free undo information about adding or removing points.
 *
 * Since: 0.98
 */
static void
dia_bezier_shape_point_object_change_free (DiaObjectChange *self)
{
  DiaBezierShapePointObjectChange *change;

  change = DIA_BEZIER_SHAPE_POINT_OBJECT_CHANGE (self);

  if ((change->type == TYPE_ADD_POINT && !change->applied) ||
      (change->type == TYPE_REMOVE_POINT && change->applied)) {
    g_clear_pointer (&change->handle1, g_free);
    g_clear_pointer (&change->handle2, g_free);
    g_clear_pointer (&change->handle3, g_free);
    g_clear_pointer (&change->cp1, g_free);
    g_clear_pointer (&change->cp2, g_free);
  }
}


/**
 * dia_bezier_shape_point_object_change_apply:
 * @self: The change to apply.
 * @obj: The object (must be a BezierShape) to apply the change to.
 *
 * Apply a point addition/removal.
 */
static void
dia_bezier_shape_point_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaBezierShapePointObjectChange *change;

  change = DIA_BEZIER_SHAPE_POINT_OBJECT_CHANGE (self);

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_POINT:
      add_handles ((BezierShape *) obj, change->pos, &change->point,
      change->corner_type,
      change->handle1, change->handle2, change->handle3,
      change->cp1, change->cp2);
      break;
    case TYPE_REMOVE_POINT:
      object_unconnect (obj, change->handle1);
      object_unconnect (obj, change->handle2);
      object_unconnect (obj, change->handle3);
      remove_handles ((BezierShape *) obj, change->pos);
      break;
    default:
      g_return_if_reached ();
  }
}


/**
 * dia_bezier_shape_point_object_change_revert:
 * @self: The change to revert.
 * @obj: The object (must be a BezierShape) to revert the change of.
 *
 * Revert (unapply) a point addition/removal.
 */
static void
dia_bezier_shape_point_object_change_revert (DiaObjectChange *self,
                                             DiaObject       *obj)
{
  DiaBezierShapePointObjectChange *change;

  change = DIA_BEZIER_SHAPE_POINT_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      remove_handles ((BezierShape *) obj, change->pos);
      break;
    case TYPE_REMOVE_POINT:
      add_handles ((BezierShape *) obj, change->pos, &change->point,
      change->corner_type,
      change->handle1, change->handle2, change->handle3,
      change->cp1, change->cp2);
      break;
    default:
      g_return_if_reached ();
  }

  change->applied = 0;
}


static DiaObjectChange *
dia_bezier_shape_point_object_change_new (BezierShape      *bezier,
                                          enum change_type  type,
                                          BezPoint         *point,
                                          BezCornerType     corner_type,
                                          int               pos,
                                          Handle           *handle1,
                                          Handle           *handle2,
                                          Handle           *handle3,
                                          ConnectionPoint  *cp1,
                                          ConnectionPoint  *cp2)
{
  DiaBezierShapePointObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_BEZIER_SHAPE_POINT_OBJECT_CHANGE);

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

  return DIA_OBJECT_CHANGE (change);
}


struct _DiaBezierShapeCornerObjectChange {
  DiaObjectChange obj_change;
  /* Only one kind of corner_change */
  int applied;

  Handle *handle;
  /* Old places when SET_CORNER_TYPE is applied */
  Point point_left, point_right;
  BezCornerType old_type, new_type;
};


DIA_DEFINE_OBJECT_CHANGE (DiaBezierShapeCornerObjectChange,
                          dia_bezier_shape_corner_object_change)


static void
dia_bezier_shape_corner_object_change_free (DiaObjectChange *self)
{
}


/**
 * dia_bezier_shape_corner_object_change_apply:
 * @self: The undo information to apply.
 * @obj: The object to apply the undo information too.
 *
 * Apply a change of corner type. This may change the position of the
 * control handles by calling beziershape_straighten_corner().
 *
 * Since: 0.98
 */
static void
dia_bezier_shape_corner_object_change_apply (DiaObjectChange *self,
                                             DiaObject       *obj)
{
  DiaBezierShapeCornerObjectChange *change = DIA_BEZIER_SHAPE_CORNER_OBJECT_CHANGE (self);
  BezierShape *bezier = (BezierShape *) obj;
  int handle_nr = get_handle_nr (bezier, change->handle);
  int comp_nr = get_major_nr (handle_nr);

  beziershape_straighten_corner (bezier, comp_nr);

  bezier->bezier.corner_types[comp_nr] = change->new_type;
  if (comp_nr == 0) {
    bezier->bezier.corner_types[bezier->bezier.num_points-1] = change->new_type;
  }
  if (comp_nr == bezier->bezier.num_points - 1) {
    bezier->bezier.corner_types[0] = change->new_type;
  }

  change->applied = 1;
}


/**
 * dia_bezier_shape_corner_object_change_revert:
 * @self: Undo information to apply.
 * @obj: The beziershape object to apply the change to.
 *
 * Revert (unapply) a change of corner type. This may move the position
 * of the control handles to what they were before applying.
 *
 * Since: 0.98
 */
static void
dia_bezier_shape_corner_object_change_revert (DiaObjectChange *self,
                                              DiaObject       *obj)
{
  DiaBezierShapeCornerObjectChange *change = DIA_BEZIER_SHAPE_CORNER_OBJECT_CHANGE (self);
  BezierShape *bezier = (BezierShape *)obj;
  int handle_nr = get_handle_nr (bezier, change->handle);
  int comp_nr = get_major_nr (handle_nr);

  bezier->bezier.points[comp_nr].p2 = change->point_left;
  if (comp_nr == bezier->bezier.num_points - 1) {
    bezier->bezier.points[1].p1 = change->point_right;
  } else {
    bezier->bezier.points[comp_nr + 1].p1 = change->point_right;
  }
  bezier->bezier.corner_types[comp_nr] = change->old_type;
  if (comp_nr == 0) {
    bezier->bezier.corner_types[bezier->bezier.num_points - 1] = change->new_type;
  }
  if (comp_nr == bezier->bezier.num_points - 1) {
    bezier->bezier.corner_types[0] = change->new_type;
  }

  change->applied = 0;
}


/**
 * dia_bezier_shape_corner_object_change_new:
 * @bezier: The bezier object this applies to.
 * @handle: The handle of the corner being changed.
 * @point_left: The position of the left control handle.
 * @point_right: The position of the right control handle.
 * @old_corner_type: The corner type before applying.
 * @new_corner_type: The corner type being changed to.
 *
 * Create new undo information about a changing the type of a corner.
 * Note that the created #DiaObjectChange object has nothing in it that needs
 * freeing.
 *
 * Returns: Newly allocated undo information.
 *
 * Since: 0.98
 */
static DiaObjectChange *
dia_bezier_shape_corner_object_change_new (BezierShape   *bezier,
                                           Handle        *handle,
                                           Point         *point_left,
                                           Point         *point_right,
                                           BezCornerType  old_corner_type,
                                           BezCornerType  new_corner_type)
{
  DiaBezierShapeCornerObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_BEZIER_SHAPE_CORNER_OBJECT_CHANGE);

  change->old_type = old_corner_type;
  change->new_type = new_corner_type;
  change->applied = 1;

  change->handle = handle;
  change->point_left = *point_left;
  change->point_right = *point_right;

  return DIA_OBJECT_CHANGE (change);
}
