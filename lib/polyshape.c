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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy() */

#include "polyshape.h"
#include "message.h"
#include "diarenderer.h"

#define NUM_CONNECTIONS(poly) ((poly)->numpoints * 2 + 1)

enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};


struct _DiaPolyShapeObjectChange {
  DiaObjectChange obj_change;

  enum change_type type;
  int applied;

  Point point;
  int pos;

  Handle *handle; /* owning ref when not applied for ADD_POINT
		     owning ref when applied for REMOVE_POINT */
  ConnectionPoint *cp1, *cp2;
};


DIA_DEFINE_OBJECT_CHANGE (DiaPolyShapeObjectChange, dia_poly_shape_object_change)


static DiaObjectChange *polyshape_create_change (PolyShape        *poly,
                                                 enum change_type  type,
                                                 Point            *point,
                                                 int               segment,
                                                 Handle           *handle,
                                                 ConnectionPoint  *cp1,
                                                 ConnectionPoint  *cp2);


static void
setup_handle (Handle *handle)
{
  handle->id = HANDLE_CORNER;
  handle->type = HANDLE_MAJOR_CONTROL;
  handle->connect_type = HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}


static int get_handle_nr(PolyShape *poly, Handle *handle)
{
  int i = 0;
  for (i=0;i<poly->numpoints;i++) {
    if (poly->object.handles[i] == handle)
      return i;
  }
  return -1;
}


DiaObjectChange *
polyshape_move_handle (PolyShape        *poly,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  int handle_nr;

  handle_nr = get_handle_nr(poly, handle);
  poly->points[handle_nr] = *to;

  return NULL;
}


DiaObjectChange *
polyshape_move (PolyShape *poly, Point *to)
{
  Point p;
  int i;

  p = *to;
  point_sub(&p, &poly->points[0]);

  poly->points[0] = *to;
  for (i=1;i<poly->numpoints;i++) {
    point_add(&poly->points[i], &p);
  }

  return NULL;
}

int
polyshape_closest_segment(PolyShape *poly, Point *point, real line_width)
{
  int i;
  real dist;
  int closest;

  dist = distance_line_point( &poly->points[poly->numpoints-1], &poly->points[0],
			      line_width, point);
  closest = poly->numpoints-1;
  for (i=0;i<poly->numpoints-1;i++) {
    real new_dist =
      distance_line_point( &poly->points[i], &poly->points[i+1],
			   line_width, point);
    if (new_dist < dist) {
      dist = new_dist;
      closest = i;
    }
  }
  return closest;
}

Handle *
polyshape_closest_handle(PolyShape *poly, Point *point)
{
  int i;
  real dist;
  Handle *closest;

  closest = poly->object.handles[0];
  dist = distance_point_point( point, &closest->pos);
  for (i=1;i<poly->numpoints;i++) {
    real new_dist;
    new_dist = distance_point_point( point, &poly->points[i]);
    if (new_dist < dist) {
      dist = new_dist;
      closest = poly->object.handles[i];
    }
  }
  return closest;
}

real
polyshape_distance_from(PolyShape *poly, Point *point, real line_width)
{
  return distance_polygon_point(poly->points, poly->numpoints,
				line_width, point);
}


static void
add_handle (PolyShape       *poly,
            int              pos,
            Point           *point,
            Handle          *handle,
            ConnectionPoint *cp1,
            ConnectionPoint *cp2)
{
  DiaObject *obj = &poly->object;

  poly->numpoints++;
  poly->points = g_renew (Point, poly->points, poly->numpoints);

  for (int i = poly->numpoints - 1; i > pos; i--) {
    poly->points[i] = poly->points[i-1];
  }

  poly->points[pos] = *point;

  object_add_handle_at (obj, handle, pos);
  object_add_connectionpoint_at (obj, cp1, 2 * pos);
  object_add_connectionpoint_at (obj, cp2, 2 * pos + 1);
}


static void
remove_handle (PolyShape *poly, int pos)
{
  DiaObject *obj;
  Handle *old_handle;
  ConnectionPoint *old_cp1, *old_cp2;

  obj = (DiaObject *)poly;

  /* delete the points */
  poly->numpoints--;
  for (int i = pos; i < poly->numpoints; i++) {
    poly->points[i] = poly->points[i+1];
  }

  poly->points = g_renew (Point, poly->points, poly->numpoints);

  old_handle = obj->handles[pos];
  old_cp1 = obj->connections[2*pos];
  old_cp2 = obj->connections[2*pos+1];

  object_remove_handle (&poly->object, old_handle);
  object_remove_connectionpoint (obj, old_cp1);
  object_remove_connectionpoint (obj, old_cp2);
}


/* Add a point by splitting segment into two, putting the new point at
 'point' or, if NULL, in the middle */
DiaObjectChange *
polyshape_add_point (PolyShape *poly, int segment, Point *point)
{
  Point realpoint;
  Handle *new_handle;
  ConnectionPoint *new_cp1, *new_cp2;

  if (point == NULL) {
    realpoint.x = (poly->points[segment].x + poly->points[segment + 1].x) / 2;
    realpoint.y = (poly->points[segment].y + poly->points[segment + 1].y) / 2;
  } else {
    realpoint = *point;
  }

  new_handle = g_new (Handle, 1);
  new_cp1 = g_new0 (ConnectionPoint, 1);
  new_cp1->object = &poly->object;
  new_cp2 = g_new0 (ConnectionPoint, 1);
  new_cp2->object = &poly->object;

  setup_handle (new_handle);
  add_handle (poly, segment + 1, &realpoint, new_handle, new_cp1, new_cp2);

  return polyshape_create_change (poly,
                                  TYPE_ADD_POINT,
                                  &realpoint,
                                  segment + 1,
                                  new_handle,
                                  new_cp1,
                                  new_cp2);
}


DiaObjectChange *
polyshape_remove_point (PolyShape *poly, int pos)
{
  Handle *old_handle;
  ConnectionPoint *old_cp1, *old_cp2;
  Point old_point;

  old_handle = poly->object.handles[pos];
  old_point = poly->points[pos];
  old_cp1 = poly->object.connections[2 * pos];
  old_cp2 = poly->object.connections[2 * pos + 1];

  object_unconnect (DIA_OBJECT (poly), old_handle);

  remove_handle (poly, pos);

  polyshape_update_data (poly);

  return polyshape_create_change (poly,
                                  TYPE_REMOVE_POINT,
                                  &old_point,
                                  pos,
                                  old_handle,
                                  old_cp1,
                                  old_cp2);
}


/** Returns the first clockwise direction in dirs
 * (as returned from find_slope_directions) */
static gint
first_direction(gint dirs) {
  switch (dirs) {
  case DIR_NORTHEAST: return DIR_NORTH;
  case DIR_SOUTHEAST: return DIR_EAST;
  case DIR_NORTHWEST: return DIR_WEST;
  case DIR_SOUTHWEST: return DIR_SOUTH;
  default: return dirs;
  }
}

/** Returns the last clockwise direction in dirs
 * (as returned from find_slope_directions) */
static gint
last_direction(gint dirs) {
  switch (dirs) {
  case DIR_NORTHEAST: return DIR_EAST;
  case DIR_SOUTHEAST: return DIR_SOUTH;
  case DIR_NORTHWEST: return DIR_NORTH;
  case DIR_SOUTHWEST: return DIR_WEST;
  default: return dirs;
  }
}

/** Returns the available directions for a corner */
static gint
find_tip_directions(Point prev, Point this, Point next)
{
  gint startdirs = find_slope_directions(prev, this);
  gint enddirs = find_slope_directions(this, next);
  gint firstdir = first_direction(startdirs);
  gint lastdir = last_direction(enddirs);
  gint dir, dirs = 0;

  dir = firstdir;
  while (dir != lastdir) {
    dirs |= dir;
    dir = dir * 2;
    if (dir == 16) dir = 1;
  }
  dirs |= dir;

  return dirs;
}


void
polyshape_update_data (PolyShape *poly)
{
  Point next;
  int i;
  DiaObject *obj = &poly->object;
  Point minp, maxp;

  /* handle the case of whole points array update (via set_prop) */
  if (poly->numpoints != obj->num_handles ||
      NUM_CONNECTIONS(poly) != obj->num_connections) {
    object_unconnect_all(obj); /* too drastic ? */

    obj->handles = g_renew (Handle *, obj->handles, poly->numpoints);
    obj->num_handles = poly->numpoints;
    for (i = 0; i < poly->numpoints; i++) {
      obj->handles[i] = g_new0 (Handle, 1);
      setup_handle (obj->handles[i]);
    }

    obj->connections = g_renew (ConnectionPoint *,
                                obj->connections,
                                NUM_CONNECTIONS (poly));

    for (i = 0; i < NUM_CONNECTIONS(poly); i++) {
      obj->connections[i] = g_new0(ConnectionPoint, 1);
      obj->connections[i]->object = obj;
    }
    obj->num_connections = NUM_CONNECTIONS(poly);
  }

  /* Update handles: */
  minp = maxp = poly->points[0];
  for (i = 0; i < poly->numpoints; i++) {
    gint thisdir, nextdir;
    Point prev;
    obj->handles[i]->pos = poly->points[i];

    if (i == 0)
      prev = poly->points[poly->numpoints-1];
    else
      prev = poly->points[i-1];
    if (i == poly->numpoints - 1)
      next = poly->points[0];
    else
      next = poly->points[i+1];
    point_add(&next, &poly->points[i]);
    point_scale(&next, 0.5);

    thisdir = find_tip_directions(prev, poly->points[i], next);
    nextdir = find_slope_directions(poly->points[i], next);

    obj->connections[2*i]->pos = poly->points[i];
    obj->connections[2*i]->directions = thisdir;
    obj->connections[2*i+1]->pos = next;
    obj->connections[2*i+1]->directions = nextdir;

    if (poly->points[i].x < minp.x) minp.x = poly->points[i].x;
    if (poly->points[i].x > maxp.x) maxp.x = poly->points[i].x;
    if (poly->points[i].y < minp.y) minp.y = poly->points[i].y;
    if (poly->points[i].y > maxp.y) maxp.y = poly->points[i].y;
  }

  obj->connections[obj->num_connections-1]->pos.x = (minp.x + maxp.x) / 2;
  obj->connections[obj->num_connections-1]->pos.y = (minp.y + maxp.y) / 2;
  obj->connections[obj->num_connections-1]->directions = DIR_ALL;
}


void
polyshape_update_boundingbox (PolyShape *poly)
{
  ElementBBExtras *extra;
  PolyBBExtras pextra;

  g_return_if_fail (poly != NULL);

  extra = &poly->extra_spacing;
  pextra.start_trans = pextra.end_trans =
    pextra.start_long = pextra.end_long = 0;
  pextra.middle_trans = extra->border_trans;

  polyline_bbox (&poly->points[0],
                 poly->numpoints,
                 &pextra,
                 TRUE,
                 &poly->object.bounding_box);
}


void
polyshape_init(PolyShape *poly, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &poly->object;

  object_init(obj, num_points, 2 * num_points + 1);

  poly->numpoints = num_points;

  poly->points = g_new0 (Point, num_points);

  for (i = 0; i < num_points; i++) {
    obj->handles[i] = g_new(Handle, 1);

    obj->handles[i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[i]->connected_to = NULL;
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[i]->id = HANDLE_CORNER;
  }

  for (i = 0; i < NUM_CONNECTIONS(poly); i++) {
    obj->connections[i] = g_new0(ConnectionPoint, 1);
    obj->connections[i]->object = &poly->object;
    obj->connections[i]->flags = 0;
  }
  obj->connections[obj->num_connections - 1]->flags = CP_FLAGS_MAIN;


  /* Since the points aren't set yet, and update_data only deals with
     the points, don't call it (Thanks, valgrind!) */
  /*  polyshape_update_data(poly);*/
}

void
polyshape_set_points(PolyShape *poly, int num_points, Point *points)
{
  int i;

  poly->numpoints = num_points;

  g_clear_pointer (&poly->points, g_free);
  poly->points = g_new(Point, num_points);

  for (i = 0; i < num_points; i++) {
    poly->points[i] = points[i];
  }
}

void
polyshape_copy(PolyShape *from, PolyShape *to)
{
  int i;
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  polyshape_set_points(to, from->numpoints, from->points);

  for (i=0;i<to->numpoints;i++) {
    toobj->handles[i] = g_new(Handle, 1);
    setup_handle(toobj->handles[i]);
    toobj->connections[2*i] = g_new0(ConnectionPoint, 1);
    toobj->connections[2*i]->object = &to->object;
    toobj->connections[2*i+1] = g_new0(ConnectionPoint, 1);
    toobj->connections[2*i+1]->object = &to->object;
  }
  toobj->connections[toobj->num_connections - 1] = g_new0(ConnectionPoint, 1);
  toobj->connections[toobj->num_connections - 1]->object = &to->object;

  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));
  polyshape_update_data(to);
}

void
polyshape_destroy (PolyShape *poly)
{
  int i;
  Handle **temp_handles;
  ConnectionPoint **temp_cps;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  temp_handles = g_new0 (Handle *, poly->numpoints);
  for (i = 0; i < poly->numpoints; i++) {
    temp_handles[i] = poly->object.handles[i];
  }

  temp_cps = g_new0 (ConnectionPoint *, NUM_CONNECTIONS (poly));
  for (i = 0; i < NUM_CONNECTIONS (poly); i++) {
    temp_cps[i] = poly->object.connections[i];
  }

  object_destroy (&poly->object);

  for (i = 0; i < poly->numpoints;i++) {
    g_clear_pointer (&temp_handles[i], g_free);
  }
  g_clear_pointer (&temp_handles, g_free);

  for (i = 0; i < NUM_CONNECTIONS (poly); i++) {
    g_clear_pointer (&temp_cps[i], g_free);
  }
  g_clear_pointer (&temp_cps, g_free);

  g_clear_pointer (&poly->points, g_free);
}


void
polyshape_save(PolyShape *poly, ObjectNode obj_node, DiaContext *ctx)
{
  int i;
  AttributeNode attr;

  object_save(&poly->object, obj_node, ctx);

  attr = new_attribute(obj_node, "poly_points");

  for (i=0;i<poly->numpoints;i++) {
    data_add_point(attr, &poly->points[i], ctx);
  }
}

void
polyshape_load(PolyShape *poly, ObjectNode obj_node, DiaContext *ctx) /* NOTE: Does object_init() */
{
  int i;
  AttributeNode attr;
  DataNode data;

  DiaObject *obj = &poly->object;

  object_load(obj, obj_node, ctx);

  attr = object_find_attribute(obj_node, "poly_points");

  if (attr != NULL)
    poly->numpoints = attribute_num_data(attr);
  else
    poly->numpoints = 0;

  object_init(obj, poly->numpoints, NUM_CONNECTIONS(poly));

  data = attribute_first_data(attr);
  poly->points = g_new(Point, poly->numpoints);
  for (i=0;i<poly->numpoints;i++) {
    data_point(data, &poly->points[i], ctx);
    data = data_next(data);
  }

  for (i=0;i<poly->numpoints;i++) {
    obj->handles[i] = g_new(Handle, 1);
    setup_handle(obj->handles[i]);
  }
  for (i = 0; i < NUM_CONNECTIONS(poly); i++) {
    obj->connections[i] = g_new0(ConnectionPoint, 1);
    obj->connections[i]->object = obj;
  }
  obj->connections[obj->num_connections - 1]->flags = CP_FLAGS_MAIN;

  polyshape_update_data(poly);
}


static void
dia_poly_shape_object_change_free (DiaObjectChange *self)
{
  DiaPolyShapeObjectChange *change = DIA_POLY_SHAPE_OBJECT_CHANGE (self);

  if ((change->type == TYPE_ADD_POINT && !change->applied) ||
      (change->type == TYPE_REMOVE_POINT && change->applied) ){
    g_clear_pointer (&change->handle, g_free);
    g_clear_pointer (&change->cp1, g_free);
    g_clear_pointer (&change->cp2, g_free);
  }
}


static void
dia_poly_shape_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaPolyShapeObjectChange *change = DIA_POLY_SHAPE_OBJECT_CHANGE (self);

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_POINT:
      add_handle ((PolyShape *) obj, change->pos, &change->point,
                  change->handle, change->cp1, change->cp2);
      break;
    case TYPE_REMOVE_POINT:
      object_unconnect (obj, change->handle);
      remove_handle ((PolyShape *) obj, change->pos);
      break;
    default:
      g_return_if_reached ();
  }
}


static void
dia_poly_shape_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaPolyShapeObjectChange *change = DIA_POLY_SHAPE_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      remove_handle ((PolyShape *)obj, change->pos);
      break;
    case TYPE_REMOVE_POINT:
      add_handle ((PolyShape *) obj, change->pos, &change->point,
                  change->handle, change->cp1, change->cp2);
      break;
    default:
      g_return_if_reached ();
  }

  change->applied = 0;
}


static DiaObjectChange *
polyshape_create_change (PolyShape        *poly,
                         enum change_type  type,
                         Point            *point,
                         int               pos,
                         Handle           *handle,
                         ConnectionPoint  *cp1,
                         ConnectionPoint  *cp2)
{
  DiaPolyShapeObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_POLY_SHAPE_OBJECT_CHANGE);

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->pos = pos;
  change->handle = handle;
  change->cp1 = cp1;
  change->cp2 = cp2;

  return DIA_OBJECT_CHANGE (change);
}
