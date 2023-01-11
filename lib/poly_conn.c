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

#include "poly_conn.h"
#include "diarenderer.h"


enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};


struct _DiaPolyConnObjectChange {
  DiaObjectChange obj_change;

  enum change_type type;
  int applied;

  Point point;
  int pos;

  Handle *handle; /* owning ref when not applied for ADD_POINT
                     owning ref when applied for REMOVE_POINT */
  ConnectionPoint *connected_to; /* NULL if not connected */
};


DIA_DEFINE_OBJECT_CHANGE (DiaPolyConnObjectChange, dia_poly_conn_object_change)


static DiaObjectChange *polyconn_create_change (PolyConn         *poly,
                                                enum change_type  type,
                                                Point            *point,
                                                int               segment,
                                                Handle           *handle,
                                                ConnectionPoint  *connected_to);

typedef enum
{
  PC_HANDLE_START,
  PC_HANDLE_CORNER,
  PC_HANDLE_END
} PolyConnHandleType;

static void
setup_handle(Handle *handle, PolyConnHandleType t)
{
  handle->id = (PC_HANDLE_CORNER == t ? HANDLE_CORNER
                                      : (PC_HANDLE_END == t ? HANDLE_MOVE_ENDPOINT
                                                            : HANDLE_MOVE_STARTPOINT));
  handle->type = (PC_HANDLE_CORNER == t ? HANDLE_MINOR_CONTROL : HANDLE_MAJOR_CONTROL);
  handle->connect_type = HANDLE_CONNECTABLE;
  handle->connected_to = NULL;
}


static int get_handle_nr(PolyConn *poly, Handle *handle)
{
  int i = 0;
  for (i=0;i<poly->numpoints;i++) {
    if (poly->object.handles[i] == handle)
      return i;
  }
  return -1;
}


DiaObjectChange *
polyconn_move_handle (PolyConn         *poly,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  int handle_nr;

  handle_nr = get_handle_nr (poly, handle);
  switch (handle->id) {
    case HANDLE_MOVE_STARTPOINT:
      poly->points[0] = *to;
      break;
    case HANDLE_MOVE_ENDPOINT:
      poly->points[poly->numpoints-1] = *to;
      break;
    case HANDLE_CORNER:
      poly->points[handle_nr] = *to;
      break;
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_warning ("Internal error in polyconn_move_handle.\n");
      break;
  }

  return NULL;
}


DiaObjectChange *
polyconn_move (PolyConn *poly, Point *to)
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
polyconn_closest_segment(PolyConn *poly, Point *point, real line_width)
{
  int i;
  real dist;
  int closest;

  dist = distance_line_point( &poly->points[0], &poly->points[1],
			      line_width, point);
  closest = 0;
  for (i=1;i<poly->numpoints-1;i++) {
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
polyconn_closest_handle(PolyConn *poly, Point *point)
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
polyconn_distance_from(PolyConn *poly, Point *point, real line_width)
{
  int i;
  real dist;

  dist = distance_line_point( &poly->points[0], &poly->points[1],
			      line_width, point);
  for (i=1;i<poly->numpoints-1;i++) {
    dist = MIN(dist,
	       distance_line_point( &poly->points[i], &poly->points[i+1],
				    line_width, point));
  }
  return dist;
}

static void
add_handle(PolyConn *poly, int pos, Point *point, Handle *handle)
{
  int i;
  DiaObject *obj;

  poly->numpoints++;
  poly->points = g_renew (Point, poly->points, poly->numpoints);

  for (i=poly->numpoints-1; i > pos; i--) {
    poly->points[i] = poly->points[i-1];
  }
  poly->points[pos] = *point;
  object_add_handle_at((DiaObject*)poly, handle, pos);

  obj = (DiaObject *)poly;
  if (pos==0) {
    obj->handles[1]->type = HANDLE_MINOR_CONTROL;
    obj->handles[1]->id = HANDLE_CORNER;
  }
  if (pos==obj->num_handles-1) {
    obj->handles[obj->num_handles-2]->type = HANDLE_MINOR_CONTROL;
    obj->handles[obj->num_handles-2]->id = HANDLE_CORNER;
  }
}

static void
remove_handle(PolyConn *poly, int pos)
{
  int i;
  DiaObject *obj;
  Handle *old_handle;

  obj = (DiaObject *)poly;

  if (pos==0) {
    obj->handles[1]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[1]->id = HANDLE_MOVE_STARTPOINT;
  }
  if (pos==obj->num_handles-1) {
    obj->handles[obj->num_handles-2]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[obj->num_handles-2]->id = HANDLE_MOVE_ENDPOINT;
  }

  /* delete the points */
  poly->numpoints--;
  for (i=pos; i < poly->numpoints; i++) {
    poly->points[i] = poly->points[i+1];
  }
  poly->points = g_renew (Point, poly->points, poly->numpoints);

  old_handle = obj->handles[pos];
  object_remove_handle(&poly->object, old_handle);
}


/* Add a point by splitting segment into two, putting the new point at
 'point' or, if NULL, in the middle */
DiaObjectChange *
polyconn_add_point (PolyConn *poly, int segment, Point *point)
{
  Point realpoint;
  Handle *new_handle;

  if (point == NULL) {
    realpoint.x = (poly->points[segment].x+poly->points[segment+1].x)/2;
    realpoint.y = (poly->points[segment].y+poly->points[segment+1].y)/2;
  } else {
    realpoint = *point;
  }

  new_handle = g_new0 (Handle, 1);
  setup_handle(new_handle, PC_HANDLE_CORNER);
  add_handle(poly, segment+1, &realpoint, new_handle);
  return polyconn_create_change(poly, TYPE_ADD_POINT,
				&realpoint, segment+1, new_handle,
				NULL);
}


DiaObjectChange *
polyconn_remove_point (PolyConn *poly, int pos)
{
  Handle *old_handle;
  ConnectionPoint *connectionpoint;
  Point old_point;

  old_handle = poly->object.handles[pos];
  old_point = poly->points[pos];
  connectionpoint = old_handle->connected_to;

  object_unconnect((DiaObject *)poly, old_handle);

  remove_handle(poly, pos);

  polyconn_update_data(poly);

  return polyconn_create_change(poly, TYPE_REMOVE_POINT,
				&old_point, pos, old_handle,
				connectionpoint);
}

void
polyconn_update_data(PolyConn *poly)
{
  int i;
  DiaObject *obj = &poly->object;

  /* handle the case of whole points array update (via set_prop) */
  if (poly->numpoints != obj->num_handles) {
    g_assert(0 == obj->num_connections);

    obj->handles = g_renew (Handle *, obj->handles, poly->numpoints);
    obj->num_handles = poly->numpoints;
    for (i=0;i<poly->numpoints;i++) {
      obj->handles[i] = g_new0 (Handle, 1);
      if (0 == i)
        setup_handle(obj->handles[i], PC_HANDLE_START);
      else if (i == poly->numpoints-1)
        setup_handle(obj->handles[i], PC_HANDLE_END);
      else
        setup_handle(obj->handles[i], PC_HANDLE_CORNER);
    }
  }

  /* Update handles: */
  for (i=0;i<poly->numpoints;i++) {
    obj->handles[i]->pos = poly->points[i];
  }
}


void
polyconn_update_boundingbox (PolyConn *poly)
{
  g_return_if_fail (poly != NULL);

  polyline_bbox (&poly->points[0],
                 poly->numpoints,
                 &poly->extra_spacing, FALSE,
                 &poly->object.bounding_box);
}


void
polyconn_init(PolyConn *poly, int num_points)
{
  DiaObject *obj;
  int i;

  obj = &poly->object;

  object_init(obj, num_points, 0);

  poly->numpoints = num_points;

  poly->points = g_new0 (Point, num_points);

  for (i=0;i<num_points;i++) {
    obj->handles[i] = g_new0 (Handle, 1);
    if (0 == i)
      setup_handle(obj->handles[i], PC_HANDLE_START);
    else if (i == num_points-1)
      setup_handle(obj->handles[i], PC_HANDLE_END);
    else
      setup_handle(obj->handles[i], PC_HANDLE_CORNER);
  }

  polyconn_update_data(poly);
}

/** This function does *not* set up handles */
void
polyconn_set_points(PolyConn *poly, int num_points, Point *points)
{
  int i;

  poly->numpoints = num_points;

  g_clear_pointer (&poly->points, g_free);

  poly->points = g_new0 (Point, poly->numpoints);

  for (i=0;i<poly->numpoints;i++) {
    poly->points[i] = points[i];
  }
}


void
polyconn_copy (PolyConn *from, PolyConn *to)
{
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy (fromobj, toobj);

  to->object.handles[0] = g_new0 (Handle,1);
  *to->object.handles[0] = *from->object.handles[0];

  for (int i = 1; i < toobj->num_handles - 1; i++) {
    to->object.handles[i] = g_new0 (Handle, 1);
    setup_handle (to->object.handles[i], PC_HANDLE_CORNER);
  }

  to->object.handles[toobj->num_handles-1] = g_new0 (Handle,1);
  *to->object.handles[toobj->num_handles-1] =
      *from->object.handles[toobj->num_handles-1];
  polyconn_set_points (to, from->numpoints, from->points);

  memcpy (&to->extra_spacing,
          &from->extra_spacing,
          sizeof(to->extra_spacing));
  polyconn_update_data (to);
}


void
polyconn_destroy (PolyConn *poly)
{
  int i;
  Handle **temp_handles;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  temp_handles = g_new0 (Handle *, poly->numpoints);
  for (i = 0; i <poly->numpoints;i++) {
    temp_handles[i] = poly->object.handles[i];
  }

  object_destroy (&poly->object);

  for (i = 0; i < poly->numpoints; i++) {
    g_clear_pointer (&temp_handles[i], g_free);
  }
  g_clear_pointer (&temp_handles, g_free);

  g_clear_pointer (&poly->points, g_free);
}


void
polyconn_save(PolyConn *poly, ObjectNode obj_node, DiaContext *ctx)
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
polyconn_load (PolyConn *poly, ObjectNode obj_node, DiaContext *ctx) /* NOTE: Does object_init() */
{
  AttributeNode attr;
  DataNode data;

  DiaObject *obj = &poly->object;

  object_load (obj, obj_node, ctx);

  attr = object_find_attribute (obj_node, "poly_points");

  if (attr != NULL) {
    poly->numpoints = attribute_num_data (attr);
  } else {
    poly->numpoints = 0;
  }

  object_init (obj, poly->numpoints, 0);

  data = attribute_first_data (attr);
  poly->points = g_new0 (Point, poly->numpoints);
  for (int i = 0; i < poly->numpoints; i++) {
    data_point (data, &poly->points[i], ctx);
    data = data_next (data);
  }

  obj->handles[0] = g_new0 (Handle, 1);
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;

  obj->handles[poly->numpoints-1] = g_new0 (Handle, 1);
  obj->handles[poly->numpoints-1]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[poly->numpoints-1]->connected_to = NULL;
  obj->handles[poly->numpoints-1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[poly->numpoints-1]->id = HANDLE_MOVE_ENDPOINT;

  for (int i = 1; i < poly->numpoints - 1; i++) {
    obj->handles[i] = g_new0 (Handle, 1);
    setup_handle (obj->handles[i], PC_HANDLE_CORNER);
  }

  polyconn_update_data (poly);
}


static void
dia_poly_conn_object_change_free (DiaObjectChange *self)
{
  DiaPolyConnObjectChange *change = DIA_POLY_CONN_OBJECT_CHANGE (self);

  if ((change->type == TYPE_ADD_POINT && !change->applied) ||
      (change->type == TYPE_REMOVE_POINT && change->applied) ){
    g_clear_pointer (&change->handle, g_free);
  }
}


static void
dia_poly_conn_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaPolyConnObjectChange *change = DIA_POLY_CONN_OBJECT_CHANGE (self);

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_POINT:
      add_handle ((PolyConn *) obj,
                  change->pos,
                  &change->point,
                  change->handle);
      break;
    case TYPE_REMOVE_POINT:
      object_unconnect (obj, change->handle);
      remove_handle ((PolyConn *) obj, change->pos);
      break;
    default:
      g_return_if_reached ();
  }
}


static void
dia_poly_conn_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaPolyConnObjectChange *change = DIA_POLY_CONN_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      remove_handle ((PolyConn *) obj, change->pos);
      break;
    case TYPE_REMOVE_POINT:
      add_handle ((PolyConn *) obj,
                  change->pos,
                  &change->point,
                  change->handle);

      if (change->connected_to) {
        object_connect (obj, change->handle, change->connected_to);
      }

      break;
    default:
      g_return_if_reached ();
  }

  change->applied = 0;
}


static DiaObjectChange *
polyconn_create_change (PolyConn         *poly,
                        enum change_type  type,
                        Point            *point,
                        int               pos,
                        Handle           *handle,
                        ConnectionPoint  *connected_to)
{
  DiaPolyConnObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_POLY_CONN_OBJECT_CHANGE);

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->pos = pos;
  change->handle = handle;
  change->connected_to = connected_to;

  return DIA_OBJECT_CHANGE (change);
}
