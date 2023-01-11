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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h> /* memcpy() */
#include <glib.h>

#include "orth_conn.h"
#include "message.h"
#include "diamenu.h"
#include "handle.h"
#include "diarenderer.h"
#include "autoroute.h"


static void place_handle_by_swapping(OrthConn *orth,
                                     int index, Handle *handle);
static DiaObjectChange *orthconn_set_autorouting (OrthConn *orth, gboolean on);

enum change_type {
  TYPE_ADD_SEGMENT,
  TYPE_REMOVE_SEGMENT
};

static DiaObjectChange *
midsegment_create_change (OrthConn         *orth,
                          enum change_type  type,
                          int               segment,
                          Point            *point1,
                          Point            *point2,
                          Handle           *handle1,
                          Handle           *handle2);


struct _DiaOrthConnMidSegmentObjectChange {
  DiaObjectChange obj_change;

  /* All additions and deletions of segments in the middle
   * of the orthconn must delete/add two segments to keep
   * the horizontal/vertical balance.
   *
   * None of the end segments must be removed by this change.
   */

  enum change_type type;
  int applied;

  int segment;
  Point points[2];
  Handle *handles[2]; /* These handles cannot be connected */
  /* NewOrthConn additions */
  ConnectionPoint *conn; /* ? */
  DiaObjectChange *cplchange[2];
};


DIA_DEFINE_OBJECT_CHANGE (DiaOrthConnMidSegmentObjectChange,
                          dia_orth_conn_mid_segment_object_change)


static DiaObjectChange *
endsegment_create_change (OrthConn         *orth,
                          enum change_type  type,
                          int               segment,
                          Point            *point,
                          Handle           *handle);

static void
place_handle_by_swapping(OrthConn *orth, int index, Handle *handle);

struct _DiaOrthConnEndSegmentObjectChange {
  DiaObjectChange obj_change;

  /* Additions and deletions of segments of at the endpoints
   * of the orthconn.
   *
   * Addition of an endpoint segment need not store any point.
   * Deletion of an endpoint segment need to store the endpoint position
   * so that it can be reverted.
   * Deleted segments might be connected, so we must store the connection
   * point.
   */

  enum change_type type;
  int applied;

  int segment;
  Point point;
  Handle *handle;
  Handle *old_end_handle;
  ConnectionPoint *cp; /* NULL in add segment and if not connected in
			  remove segment */
  /* NewOrthConn additions */
  DiaObjectChange *cplchange;
};


DIA_DEFINE_OBJECT_CHANGE (DiaOrthConnEndSegmentObjectChange,
                          dia_orth_conn_end_segment_object_change)


static DiaObjectChange *autoroute_create_change (OrthConn *orth, gboolean on);


struct _DiaOrthConnAutorouteObjectChange {
  DiaObjectChange obj_change;
  gboolean on;
  int num_points;
  Point *points;
};


DIA_DEFINE_OBJECT_CHANGE (DiaOrthConnAutorouteObjectChange,
                          dia_orth_conn_autoroute_object_change)


static void set_midpoint(Point *point, OrthConn *orth, int segment)
{
  int i = segment;
  point->x = 0.5*(orth->points[i].x + orth->points[i+1].x);
  point->y = 0.5*(orth->points[i].y + orth->points[i+1].y);
}

static void setup_midpoint_handle(Handle *handle)
{
  handle->id = HANDLE_MIDPOINT;
  handle->type = HANDLE_MINOR_CONTROL;
  handle->connect_type = HANDLE_NONCONNECTABLE;
  handle->connected_to = NULL;
}

static void setup_endpoint_handle(Handle *handle, HandleId id )
{
  handle->id = id;
  handle->type = HANDLE_MAJOR_CONTROL;
  handle->connect_type = HANDLE_CONNECTABLE;
  handle->connected_to = NULL;
}

static int get_handle_nr(OrthConn *orth, Handle *handle)
{
  int i = 0;
  for (i=0;i<orth->numpoints-1;i++) {
    if (orth->handles[i] == handle)
      return i;
  }
  return -1;
}

static int get_segment_nr(OrthConn *orth, Point *point, real max_dist)
{
  int i;
  int segment;
  real distance, tmp_dist;

  segment = 0;
  distance = distance_line_point(&orth->points[0], &orth->points[1], 0, point);

  for (i=1;i<orth->numpoints-1;i++) {
    tmp_dist = distance_line_point(&orth->points[i], &orth->points[i+1], 0, point);
    if (tmp_dist < distance) {
      segment = i;
      distance = tmp_dist;
    }
  }

  if (distance < max_dist)
    return segment;

  return -1;
}


DiaObjectChange *
orthconn_move_handle (OrthConn         *orth,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  int n;
  int handle_nr;
  DiaObject *obj = (DiaObject *) orth;
  DiaObjectChange *change = NULL;

  switch (handle->id) {
    case HANDLE_MOVE_STARTPOINT:
      orth->points[0] = *to;
      if (orth->autorouting &&
          autoroute_layout_orthconn (orth, cp,
                                     obj->handles[1]->connected_to)) {
        break;
      }
      switch (orth->orientation[0]) {
        case HORIZONTAL:
          orth->points[1].y = to->y;
          break;
        case VERTICAL:
          orth->points[1].x = to->x;
          break;
        default:
          g_return_val_if_reached (NULL);
      }
      break;
    case HANDLE_MOVE_ENDPOINT:
      n = orth->numpoints - 1;
      orth->points[n] = *to;
      if (orth->autorouting &&
          autoroute_layout_orthconn (orth, obj->handles[0]->connected_to,
                                     cp)) {
        break;
      }
      switch (orth->orientation[n-1]) {
        case HORIZONTAL:
          orth->points[n-1].y = to->y;
          break;
        case VERTICAL:
          orth->points[n-1].x = to->x;
          break;
        default:
          g_return_val_if_reached (NULL);
      }
      break;
    case HANDLE_MIDPOINT:
      handle_nr = get_handle_nr (orth, handle);
      if (orth->autorouting) {
        change = orthconn_set_autorouting (orth, FALSE);
      }
      switch (orth->orientation[handle_nr]) {
        case HORIZONTAL:
          orth->points[handle_nr].y = to->y;
          orth->points[handle_nr+1].y = to->y;
          break;
        case VERTICAL:
          orth->points[handle_nr].x = to->x;
          orth->points[handle_nr+1].x = to->x;
          break;
        default:
          g_return_val_if_reached (NULL);
      }
      break;
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
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
      g_warning ("Internal error in orthconn_move_handle.\n");
      break;
  }

  return change;
}


DiaObjectChange *
orthconn_move (OrthConn *orth, Point *to)
{
  Point p;
  int i;

  p = *to;
  point_sub (&p, &orth->points[0]);

  orth->points[0] = *to;
  for (i = 1; i < orth->numpoints; i++) {
    point_add (&orth->points[i], &p);
  }

  return NULL;
}


double
orthconn_distance_from (OrthConn *orth, Point *point, double line_width)
{
  int i;
  double dist;

  dist = distance_line_point (&orth->points[0], &orth->points[1],
                              line_width, point);

  for (i = 1; i < orth->numpoints - 1; i++) {
    dist = MIN (dist,
                distance_line_point (&orth->points[i], &orth->points[i+1],
                                     line_width, point));
  }

  return dist;
}


static void
neworthconn_update_midpoints(OrthConn *orth)
{
  int i;
  GSList *elem;

  elem=orth->midpoints->connections;

  /* Update connection points, using the handles' positions where useful : */
  set_midpoint(&((ConnectionPoint *)(elem->data))->pos,orth,0);
  elem=g_slist_next(elem);
  for (i=1; i<orth->numpoints-2; i++) {
    ((ConnectionPoint *)(elem->data))->pos = orth->handles[i]->pos;
    elem = g_slist_next(elem);
  }
  set_midpoint(&(((ConnectionPoint *)(elem->data))->pos),orth,i);
}


static void
adjust_handle_count_to (OrthConn *orth, int count)
{
  /* This will shrink or expand orth->handles as necessary (so that
     orth->numhandles matches orth->numpoints-1, most probably), by adding or
     removing minor handles and keeping the endpoint handles at the
     extremities of the array. */

  if (orth->numhandles == count) {
    return;
  }

  if (orth->numhandles < count) {
    /* adding */
    orth->handles = g_renew (Handle *, orth->handles, count);
    orth->handles[count-1] = orth->handles[orth->numhandles-1];
    orth->handles[orth->numhandles-1] = NULL;
    for (int i = orth->numhandles - 1; i < count - 1; i++) {
      Handle *handle = g_new0 (Handle, 1);
      setup_midpoint_handle (handle);
      object_add_handle (&orth->object,handle);
      orth->handles[i] = handle;
    }
  } else {
    /* removing */
    for (int i = count - 1; i < orth->numhandles - 1; i++) {
      Handle *handle = orth->handles[i];
      object_remove_handle (&orth->object,handle);
      g_clear_pointer (&handle, g_free);
      orth->handles[i] = NULL;
    }
    orth->handles[count-1] = orth->handles[orth->numhandles-1];
    orth->handles[orth->numhandles-1] = NULL;
    orth->handles = g_renew (Handle *, orth->handles, count);
  }
  orth->numhandles = count;
  /* handles' positions will be set now */
}


void
orthconn_update_data(OrthConn *orth)
{
  int i;
  DiaObject *obj = (DiaObject *)orth;
  Point *points;
  ConnectionPoint *start_cp;
  ConnectionPoint *end_cp;

  obj->position = orth->points[0];

  /* During startup, handles may not have been setup yet, so do so
   * temporarily to be able to get the last handle connection.
   */
  adjust_handle_count_to(orth, orth->numpoints-1);

  start_cp = orth->handles[0]->connected_to;
  end_cp = orth->handles[orth->numpoints-2]->connected_to;

  if (!orth->points) {
    g_warning("very sick OrthConn object...");
    return;
  }

  points = orth->points;
  if (!orth->autorouting &&
      (connpoint_is_autogap(start_cp) ||
       connpoint_is_autogap(end_cp))) {
    Point* new_points = g_new(Point, orth->numpoints);
    for (i = 0; i < orth->numpoints; i++) {
      new_points[i] = points[i];
    }

    if (connpoint_is_autogap(start_cp)) {
      new_points[0] = calculate_object_edge(&start_cp->pos, &new_points[1],
					    start_cp->object);
      /*
      printf("Moved start to %f, %f\n",
	     new_points[0].x, new_points[0].y);
      */
    }
    if (connpoint_is_autogap(end_cp)) {
      new_points[orth->numpoints-1] =
	calculate_object_edge(&end_cp->pos, &new_points[orth->numpoints-2],
			      end_cp->object);
      /*
      printf("Moved end to %f, %f\n",
	     new_points[orth->numpoints-1].x, new_points[orth->numpoints-1].y);
      */
    }
    g_clear_pointer (&points, g_free);
    orth->points = new_points;
  }

  obj->position = orth->points[0];

  adjust_handle_count_to(orth, orth->numpoints-1);
  connpointline_adjust_count(orth->midpoints,orth->numpoints-1,NULL);

  /* Make sure start-handle is first and end-handle is second. */
  place_handle_by_swapping(orth, 0, orth->handles[0]);
  place_handle_by_swapping(orth, 1, orth->handles[orth->numpoints-2]);

  /* Update handles: */
  orth->handles[0]->pos = orth->points[0];
  orth->handles[orth->numpoints-2]->pos = orth->points[orth->numpoints-1];

  for (i=1;i<orth->numpoints-2;i++) {
    set_midpoint(&orth->handles[i]->pos, orth, i);
  }
  neworthconn_update_midpoints(orth);
}


void
orthconn_update_boundingbox (OrthConn *orth)
{
  g_return_if_fail (orth != NULL);

  polyline_bbox (&orth->points[0],
                 orth->numpoints,
                 &orth->extra_spacing,
                 FALSE,
                 &orth->object.bounding_box);
}


int
orthconn_can_delete_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment;

  /* Cannot delete any segments when there are only two left,
   * and not amy middle segment if there are only three segments.
   */

  if (orth->numpoints==3)
    return 0;

  segment = get_segment_nr(orth, clickedpoint, 1.0);

  if (segment<0)
    return 0;

  if ( (segment != 0) && (segment != orth->numpoints-2)) {
    /* middle segment */
    if (orth->numpoints==4)
      return 0;
  }

  return 1;
}

int
orthconn_can_add_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment = get_segment_nr(orth, clickedpoint, 1000000.0);

  if (segment<0)
    return 0;

  return 1;
}


/* Needs to have at least 2 handles.
   The handles are stored in order in the OrthConn, but need
   not be stored in order in the DiaObject.handles array. This
   is so that derived object can do what they want with
   DiaObject.handles. */

void
orthconn_init(OrthConn *orth, Point *startpoint)
{
  DiaObject *obj;

  obj = &orth->object;

  object_init(obj, 3, 0);

  orth->numpoints = 4;
  orth->numorient = orth->numpoints - 1;

  orth->points = g_new0 (Point, 4);

  orth->orientation = g_new0 (Orientation, 3);

  orth->numhandles = 3;
  orth->handles = g_new0(Handle *, 3);

  orth->handles[0] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
  obj->handles[0] = orth->handles[0];

  orth->handles[1] = g_new(Handle, 1);
  setup_midpoint_handle(orth->handles[1]);
  obj->handles[1] = orth->handles[1];

  orth->handles[2] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[2], HANDLE_MOVE_ENDPOINT);
  obj->handles[2] = orth->handles[2];

  orth->autorouting = TRUE;

  /* Just so we have some position: */
  orth->points[0] = *startpoint;
  orth->points[1].x = startpoint->x;
  orth->points[1].y = startpoint->y + 1.0;
  orth->points[2].x = startpoint->x + 1.0;
  orth->points[2].y = startpoint->y + 1.0;
  orth->points[3].x = startpoint->x + 2.0;
  orth->points[3].y = startpoint->y + 1.0;

  orth->orientation[0] = VERTICAL;
  orth->orientation[1] = HORIZONTAL;
  orth->orientation[2] = VERTICAL;

  orth->midpoints = connpointline_create(obj,3);

  orthconn_update_data(orth);
}

/*!
 * \brief Change the points of the OrthConn
 *
 * This function used to *not* set up handles, but that
 * was leaving the object in an inconsistent state especially
 * when auto-routing changed the number of points
 */
void
orthconn_set_points (OrthConn *orth, int num_points, Point *points)
{
  int i;
  gboolean horiz;

  orth->numpoints = num_points;

  g_clear_pointer (&orth->points, g_free);

  orth->points = g_new0 (Point, orth->numpoints);

  for (i = 0; i < orth->numpoints; i++) {
    orth->points[i] = points[i];
  }

  /* Set up the orientation array. */
  /* Maybe we could get rid of this array altogether? */
  orth->numorient = orth->numpoints-1;
  g_clear_pointer (&orth->orientation, g_free);
  orth->orientation = g_new(Orientation, orth->numorient);
  horiz = (fabs(orth->points[0].y-orth->points[1].y) < 0.00001);
  for (i = 0; i < orth->numorient; i++) {
    if (horiz) orth->orientation[i] = HORIZONTAL;
    else orth->orientation[i] = VERTICAL;
    horiz = !horiz;
  }
  /* keep the object sane regardless of it's previous comment */
  adjust_handle_count_to (orth, orth->numpoints-1);
}

void
orthconn_copy(OrthConn *from, OrthConn *to)
{
  int i, rcc;
  DiaObject *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  to->numpoints = from->numpoints;
  to->numorient = from->numorient;

  to->points = g_new0 (Point, to->numpoints);

  for (i=0;i<to->numpoints;i++) {
    to->points[i] = from->points[i];
  }

  to->autorouting = from->autorouting;
  to->orientation = g_new0 (Orientation, to->numpoints - 1);
  to->numhandles = from->numhandles;
  to->handles = g_new0 (Handle *, to->numpoints - 1);

  for (i=0;i<to->numpoints-1;i++) {
    to->orientation[i] = from->orientation[i];
    to->handles[i] = g_new(Handle,1);
    *to->handles[i] = *from->handles[i];
    to->handles[i]->connected_to = NULL;
    toobj->handles[i] = to->handles[i];
  }
  rcc = 0;
  to->midpoints = connpointline_copy(toobj,from->midpoints,&rcc);
  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));
}

void
orthconn_destroy(OrthConn *orth)
{
  int i;

  connpointline_destroy(orth->midpoints);
  object_destroy(&orth->object);

  g_clear_pointer (&orth->points, g_free);
  g_clear_pointer (&orth->orientation, g_free);

  for (i=0;i<orth->numpoints-1;i++)
    g_clear_pointer (&orth->handles[i], g_free);

  g_clear_pointer (&orth->handles, g_free);
}

static void
place_handle_by_swapping(OrthConn *orth, int index, Handle *handle)
{
  DiaObject *obj;
  Handle *tmp;
  int j;

  obj = (DiaObject *)orth;
  if (obj->handles[index] == handle)
    return; /* Nothing to do */

  for (j=0;j<obj->num_handles;j++) {
    if (obj->handles[j] == handle) {
      /* Swap handle j and index */
      tmp = obj->handles[j];
      obj->handles[j] = obj->handles[index];
      obj->handles[index] = tmp;

      return;
    }
  }
}

void
orthconn_save(OrthConn *orth, ObjectNode obj_node, DiaContext *ctx)
{
  int i;
  AttributeNode attr;

  /* Make sure start-handle is first and end-handle is second. */
  place_handle_by_swapping(orth, 0, orth->handles[0]);
  place_handle_by_swapping(orth, 1, orth->handles[orth->numpoints-2]);

  object_save(&orth->object, obj_node, ctx);

  attr = new_attribute(obj_node, "orth_points");

  for (i=0;i<orth->numpoints;i++) {
    data_add_point(attr, &orth->points[i], ctx);
  }

  attr = new_attribute(obj_node, "orth_orient");
  for (i=0;i<orth->numpoints-1;i++) {
    data_add_enum(attr, orth->orientation[i], ctx);
  }

  data_add_boolean(new_attribute(obj_node, "autorouting"), orth->autorouting, ctx);
}

void
orthconn_load(OrthConn *orth, ObjectNode obj_node,
	      DiaContext *ctx) /* NOTE: Does object_init() */
{
  int i;
  AttributeNode attr;
  DataNode data;
  int n;
  int version = 0;

  DiaObject *obj = &orth->object;

  object_load(obj, obj_node, ctx);

  attr = object_find_attribute(obj_node, "version");
  if (attr != NULL)
    version = attribute_num_data(attr);

  attr = object_find_attribute(obj_node, "orth_points");

  if (attr != NULL)
    orth->numpoints = attribute_num_data(attr);
  else
    orth->numpoints = 0;

  orth->numorient = orth->numpoints - 1;

  object_init(obj, orth->numpoints-1, 0);

  data = attribute_first_data(attr);
  orth->points = g_new0 (Point, orth->numpoints);
  for (i=0;i<orth->numpoints;i++) {
    data_point(data, &orth->points[i], ctx);
    data = data_next(data);
  }

  attr = object_find_attribute(obj_node, "orth_orient");

  data = attribute_first_data(attr);
  orth->orientation = g_new0 (Orientation, orth->numpoints - 1);
  for (i=0;i<orth->numpoints-1;i++) {
    orth->orientation[i] = data_enum(data, ctx);
    data = data_next(data);
  }

  orth->autorouting = TRUE;
  attr = object_find_attribute(obj_node, "autorouting");
  if (attr != NULL)
    orth->autorouting = data_boolean(attribute_first_data(attr), ctx);
  else if (version == 0) {
    /* Version 0 orthconns have no autorouting. */
    orth->autorouting = FALSE;
  }

  orth->handles = g_new0 (Handle *, orth->numpoints - 1);

  orth->handles[0] = g_new (Handle, 1);
  setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
  orth->handles[0]->pos = orth->points[0];
  obj->handles[0] = orth->handles[0];

  n = orth->numpoints-2;
  orth->handles[n] = g_new (Handle, 1);
  setup_endpoint_handle(orth->handles[n], HANDLE_MOVE_ENDPOINT);
  orth->handles[n]->pos = orth->points[orth->numpoints-1];
  obj->handles[1] = orth->handles[n];

  for (i=1; i<orth->numpoints-2; i++) {
    orth->handles[i] = g_new (Handle, 1);
    setup_midpoint_handle(orth->handles[i]);
    obj->handles[i+1] = orth->handles[i];
  }
  orth->numhandles = orth->numpoints-1;
  orth->midpoints = connpointline_create(obj,orth->numpoints-1);

  orthconn_update_data(orth);
}

Handle*
orthconn_get_middle_handle( OrthConn *orth )
{
  int n = orth->numpoints - 1 ;
  return orth->handles[ n/2 ] ;
}


DiaObjectChange *
orthconn_delete_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment;
  DiaObjectChange *change = NULL;

  if (!orthconn_can_delete_segment (orth, clickedpoint)) {
    return NULL;
  }

  segment = get_segment_nr(orth, clickedpoint, 1.0);
  if (segment < 0)
    return NULL;

  if (segment==0) {
    change = endsegment_create_change (orth,
                                       TYPE_REMOVE_SEGMENT,
                                       segment,
                                       &orth->points[segment],
                                       orth->handles[segment]);
  } else if (segment == orth->numpoints-2) {
    change = endsegment_create_change (orth,
                                       TYPE_REMOVE_SEGMENT,
                                       segment,
                                       &orth->points[segment + 1],
                                       orth->handles[segment]);
  } else if (segment > 0) {
    /* Don't delete the last midpoint segment.
     * That would delete also the endpoint segment after it.
     */
    if (segment == orth->numpoints-3) {
      segment--;
    }

    change = midsegment_create_change (orth,
                                       TYPE_REMOVE_SEGMENT,
                                       segment,
                                       &orth->points[segment],
                                       &orth->points[segment + 1],
                                       orth->handles[segment],
                                       orth->handles[segment + 1]);
  }

  dia_object_change_apply (change, DIA_OBJECT (orth));

  return change;
}


DiaObjectChange *
orthconn_add_segment (OrthConn *orth, Point *clickedpoint)
{
  Handle *handle1, *handle2;
  DiaObjectChange *change = NULL;
  int segment;
  Point newpoint;

  if (!orthconn_can_add_segment (orth, clickedpoint)) {
    return NULL;
  }

  segment = get_segment_nr (orth, clickedpoint, 1.0);
  if (segment < 0) {
    return NULL;
  }

  if (segment==0) { /* First segment */
    handle1 = g_new0 (Handle, 1);
    setup_endpoint_handle (handle1, HANDLE_MOVE_STARTPOINT);
    change = endsegment_create_change (orth,
                                       TYPE_ADD_SEGMENT,
                                       0,
                                       &orth->points[0],
                                       handle1);
  } else if (segment == orth->numpoints-2) { /* Last segment */
    handle1 = g_new0 (Handle, 1);
    setup_endpoint_handle (handle1, HANDLE_MOVE_ENDPOINT);
    change = endsegment_create_change (orth,
                                       TYPE_ADD_SEGMENT,
                                       segment + 1,
                                       &orth->points[segment + 1],
                                       handle1);
  } else if (segment > 0) {
    handle1 = g_new (Handle, 1);
    setup_midpoint_handle (handle1);
    handle2 = g_new (Handle, 1);
    setup_midpoint_handle (handle2);
    newpoint = *clickedpoint;
    if (orth->orientation[segment]==HORIZONTAL)
      newpoint.y = orth->points[segment].y;
    else
      newpoint.x = orth->points[segment].x;

    change = midsegment_create_change (orth,
                                       TYPE_ADD_SEGMENT,
                                       segment,
                                       &newpoint,
                                       &newpoint,
                                       handle1,
                                       handle2);
  }

  dia_object_change_apply (change, DIA_OBJECT (orth));

  return change;
}


/* Set autorouting on or off.  If setting on, try to autoroute and
 * return the changes from that.
 */
static DiaObjectChange *
orthconn_set_autorouting (OrthConn *conn, gboolean on)
{
  DiaObject *obj = DIA_OBJECT (conn);
  DiaObjectChange *change;

  change = autoroute_create_change (conn, on);
  dia_object_change_apply (change, obj);

  return change;
}


static void
delete_point(OrthConn *orth, int pos)
{
  orth->numpoints--;
  orth->numorient = orth->numpoints - 1;

  for (int i = pos; i < orth->numpoints; i++) {
    orth->points[i] = orth->points[i+1];
  }

  orth->points = g_renew (Point, orth->points, orth->numpoints);
}


/* Make sure numpoints have been decreased before calling this function.
 * ie. call delete_point first.
 */
static void
remove_handle(OrthConn *orth, int segment)
{
  int i;
  Handle *handle;

  handle = orth->handles[segment];

  for (i=segment; i < orth->numpoints-1; i++) {
    orth->handles[i] = orth->handles[i+1];
    orth->orientation[i] = orth->orientation[i+1];
  }

  orth->orientation = g_renew (Orientation,
                               orth->orientation,
                               orth->numpoints - 1);
  orth->handles = g_renew (Handle *, orth->handles, orth->numpoints - 1);

  object_remove_handle(&orth->object, handle);
  orth->numhandles = orth->numpoints-1;
}


static void
add_point (OrthConn *orth, int pos, Point *point)
{
  orth->numpoints++;
  orth->numorient = orth->numpoints-1;

  orth->points = g_renew (Point, orth->points, orth->numpoints);
  for (int i = orth->numpoints - 1; i > pos; i--) {
    orth->points[i] = orth->points[i-1];
  }
  orth->points[pos] = *point;
}


/* Make sure numpoints have been increased before calling this function.
 * ie. call add_point first.
 */
static void
insert_handle (OrthConn    *orth,
               int          segment,
               Handle      *handle,
               Orientation  orient)
{
  orth->orientation = g_renew (Orientation,
                               orth->orientation,
                               orth->numpoints - 1);
  orth->handles = g_renew (Handle *, orth->handles, orth->numpoints - 1);

  for (int i = orth->numpoints - 2; i > segment; i--) {
    orth->handles[i] = orth->handles[i-1];
    orth->orientation[i] = orth->orientation[i-1];
  }

  orth->handles[segment] = handle;
  orth->orientation[segment] = orient;

  object_add_handle(&orth->object, handle);
  orth->numhandles = orth->numpoints-1;
}


static void
dia_orth_conn_end_segment_object_change_free (DiaObjectChange *self)
{
  DiaOrthConnEndSegmentObjectChange *change = DIA_ORTH_CONN_END_SEGMENT_OBJECT_CHANGE (self);

  if ( (change->type==TYPE_ADD_SEGMENT && !change->applied) ||
       (change->type==TYPE_REMOVE_SEGMENT && change->applied) ){
    g_clear_pointer (&change->handle, g_free);
  }

  g_clear_pointer (&change->cplchange, dia_object_change_unref);
}


static void
dia_orth_conn_end_segment_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnEndSegmentObjectChange *change = DIA_ORTH_CONN_END_SEGMENT_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn *)obj;

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_SEGMENT:
      object_unconnect (obj, change->old_end_handle);
      if (change->segment==0) { /* first */
        add_point (orth, 0, &change->point);
        insert_handle (orth, change->segment,
                       change->handle, FLIP_ORIENT(orth->orientation[0]) );
        setup_midpoint_handle (orth->handles[1]);
        obj->position = orth->points[0];
        change->cplchange = connpointline_add_point (orth->midpoints,
                                                     &change->point);
      } else { /* last */
        add_point (orth, orth->numpoints, &change->point);
        insert_handle (orth, change->segment, change->handle,
                       FLIP_ORIENT(orth->orientation[orth->numpoints-3]) );
        setup_midpoint_handle (orth->handles[orth->numpoints-3]);
        change->cplchange = connpointline_add_point (orth->midpoints,
                                                     &orth->midpoints->end);
      }
      if (change->cp) {
        object_connect (obj, change->handle, change->cp);
      }
      break;
    case TYPE_REMOVE_SEGMENT:
      object_unconnect (obj, change->old_end_handle);
      change->cplchange =
        connpointline_remove_point (orth->midpoints,
                                    &orth->points[change->segment]);
      if (change->segment==0) { /* first */
        delete_point (orth, 0);
        remove_handle (orth, 0);
        setup_endpoint_handle (orth->handles[0], HANDLE_MOVE_STARTPOINT);
        obj->position = orth->points[0];
      } else { /* last */
        delete_point (orth, orth->numpoints-1);
        remove_handle (orth, change->segment);
        setup_endpoint_handle (orth->handles[orth->numpoints-2],
                               HANDLE_MOVE_ENDPOINT);
      }
      break;
    default:
      g_return_if_reached ();
  }
  neworthconn_update_midpoints (orth); /* useless ? */
}


static void
dia_orth_conn_end_segment_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnEndSegmentObjectChange *change = DIA_ORTH_CONN_END_SEGMENT_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn *) obj;

  dia_object_change_revert (change->cplchange, obj);

  switch (change->type) {
    case TYPE_ADD_SEGMENT:
      object_unconnect(obj, change->handle);
      if (change->segment==0) { /* first */
        delete_point (orth, 0);
        remove_handle (orth, 0);
        setup_endpoint_handle (orth->handles[0], HANDLE_MOVE_STARTPOINT);
        obj->position = orth->points[0];
      } else { /* last */
        delete_point (orth, orth->numpoints-1);
        remove_handle (orth, change->segment);
        setup_endpoint_handle (orth->handles[orth->numpoints-2],
                               HANDLE_MOVE_ENDPOINT);
      }
      if (change->cp) {
        object_connect (obj, change->old_end_handle, change->cp);
      }
      break;
    case TYPE_REMOVE_SEGMENT:
      if (change->segment==0) { /* first */
        add_point (orth, 0, &change->point);
        insert_handle (orth, change->segment,
                       change->handle, FLIP_ORIENT (orth->orientation[0]) );
        setup_midpoint_handle (orth->handles[1]);
        obj->position = orth->points[0];
      } else { /* last */
        add_point (orth, orth->numpoints, &change->point);
        insert_handle (orth, change->segment, change->handle,
                       FLIP_ORIENT (orth->orientation[orth->numpoints-3]) );
        setup_midpoint_handle (orth->handles[orth->numpoints-3]);
      }
      if (change->cp) {
        object_connect (obj, change->old_end_handle, change->cp);
      }
      break;
    default:
      g_return_if_reached ();
  }

  change->applied = 0;

  neworthconn_update_midpoints (orth); /* useless ? */
}


static DiaObjectChange *
endsegment_create_change (OrthConn         *orth,
                          enum change_type  type,
                          int               segment,
                          Point            *point,
                          Handle           *handle)
{
  DiaOrthConnEndSegmentObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_ORTH_CONN_END_SEGMENT_OBJECT_CHANGE);

  change->type = type;
  change->applied = 0;
  change->segment = segment;
  change->point = *point;
  change->handle = handle;
  if (segment == 0) {
    change->old_end_handle = orth->handles[0];
  } else {
    change->old_end_handle = orth->handles[orth->numpoints - 2];
  }
  change->cp = change->old_end_handle->connected_to;

  return DIA_OBJECT_CHANGE (change);
}


static void
dia_orth_conn_mid_segment_object_change_free (DiaObjectChange *self)
{
  DiaOrthConnMidSegmentObjectChange *change = DIA_ORTH_CONN_MID_SEGMENT_OBJECT_CHANGE (self);

  if ( (change->type==TYPE_ADD_SEGMENT && !change->applied) ||
       (change->type==TYPE_REMOVE_SEGMENT && change->applied) ){
    g_clear_pointer (&change->handles[0], g_free);
    g_clear_pointer (&change->handles[1], g_free);
  }

  g_clear_pointer (&change->cplchange[0], dia_object_change_unref);
  g_clear_pointer (&change->cplchange[1], dia_object_change_unref);
}


static void
dia_orth_conn_mid_segment_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnMidSegmentObjectChange *change = DIA_ORTH_CONN_MID_SEGMENT_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn *)obj;
  int seg;

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_SEGMENT:
      add_point (orth, change->segment+1, &change->points[1]);
      add_point (orth, change->segment+1, &change->points[0]);
      insert_handle (orth, change->segment+1, change->handles[1],
                     orth->orientation[change->segment] );
      insert_handle (orth, change->segment+1, change->handles[0],
                     FLIP_ORIENT (orth->orientation[change->segment]) );
      change->cplchange[0] =
        connpointline_add_point (orth->midpoints, &change->points[0]);
      change->cplchange[1] =
        connpointline_add_point (orth->midpoints, &change->points[1]);
      break;
    case TYPE_REMOVE_SEGMENT:
      seg = change->segment?change->segment:1;
      change->cplchange[0] =
        connpointline_remove_point (orth->midpoints,
          &orth->points[seg-1]);
      change->cplchange[1] =
        connpointline_remove_point (orth->midpoints,
          &orth->points[seg]);
      delete_point (orth, change->segment);
      remove_handle (orth, change->segment);
      delete_point (orth, change->segment);
      remove_handle (orth, change->segment);
      if (orth->orientation[change->segment]==HORIZONTAL) {
        orth->points[change->segment].x = change->points[0].x;
      } else {
        orth->points[change->segment].y = change->points[0].y;
      }
      break;
    default:
     g_return_if_reached ();
  }
  neworthconn_update_midpoints (orth); /* useless ? */
}


static void
dia_orth_conn_mid_segment_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnMidSegmentObjectChange *change = DIA_ORTH_CONN_MID_SEGMENT_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn *)obj;

  dia_object_change_revert (change->cplchange[0], obj);
  dia_object_change_revert (change->cplchange[1], obj);

  switch (change->type) {
    case TYPE_ADD_SEGMENT:
      delete_point (orth, change->segment+1);
      remove_handle (orth, change->segment+1);
      delete_point (orth, change->segment+1);
      remove_handle (orth, change->segment+1);
      break;
    case TYPE_REMOVE_SEGMENT:
      if (orth->orientation[change->segment]==HORIZONTAL) {
        orth->points[change->segment].x = change->points[1].x;
      } else {
        orth->points[change->segment].y = change->points[1].y;
      }
      add_point (orth, change->segment, &change->points[1]);
      add_point (orth, change->segment, &change->points[0]);
      insert_handle (orth, change->segment, change->handles[1],
                     orth->orientation[change->segment-1] );
      insert_handle (orth, change->segment, change->handles[0],
                     FLIP_ORIENT (orth->orientation[change->segment-1]) );
      break;
    default:
      g_return_if_reached ();
  }
  change->applied = 0;
}


static DiaObjectChange *
midsegment_create_change (OrthConn         *orth,
                          enum change_type  type,
                          int               segment,
                          Point            *point1,
                          Point            *point2,
                          Handle           *handle1,
                          Handle           *handle2)
{
  DiaOrthConnMidSegmentObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_ORTH_CONN_MID_SEGMENT_OBJECT_CHANGE);

  change->type = type;
  change->applied = 0;
  change->segment = segment;
  change->points[0] = *point1;
  change->points[1] = *point2;
  change->handles[0] = handle1;
  change->handles[1] = handle2;

  return DIA_OBJECT_CHANGE (change);
}


static void
dia_orth_conn_autoroute_object_change_free (DiaObjectChange *self)
{
  DiaOrthConnAutorouteObjectChange *change = DIA_ORTH_CONN_AUTOROUTE_OBJECT_CHANGE (self);

  g_clear_pointer (&change->points, g_free);
}


static void
dia_orth_conn_autoroute_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnAutorouteObjectChange *change = DIA_ORTH_CONN_AUTOROUTE_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn*)obj;

  if (change->on) {
    orth->autorouting = TRUE;
    autoroute_layout_orthconn(orth, obj->handles[0]->connected_to,
			      obj->handles[1]->connected_to);
  } else {
    orth->autorouting = FALSE;
    orthconn_set_points(orth, change->num_points, change->points);
    /* adjust the rest of the object? nope, see: object_change_revert() */
  }
}


static void
dia_orth_conn_autoroute_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaOrthConnAutorouteObjectChange *change = DIA_ORTH_CONN_AUTOROUTE_OBJECT_CHANGE (self);
  OrthConn *orth = (OrthConn*)obj;

  if (change->on) {
    orth->autorouting = FALSE;
    orthconn_set_points(orth, change->num_points, change->points);
  } else {
    orth->autorouting = TRUE;
    autoroute_layout_orthconn(orth, obj->handles[0]->connected_to,
			      obj->handles[1]->connected_to);
  }
}


static DiaObjectChange *
autoroute_create_change (OrthConn *orth, gboolean on)
{
  DiaOrthConnAutorouteObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_ORTH_CONN_AUTOROUTE_OBJECT_CHANGE);

  change->on = on;
  change->num_points = orth->numpoints;
  change->points = g_new0 (Point, orth->numpoints);

  for (int i = 0; i < orth->numpoints; i++) {
    change->points[i] = orth->points[i];
  }

  return DIA_OBJECT_CHANGE (change);
}


DiaObjectChange *
orthconn_toggle_autorouting_callback (DiaObject *obj,
                                      Point     *clicked,
                                      gpointer   data)
{
  DiaObjectChange *change;
  /* This is kinda hackish.  Since we can't see the menu item, we have to
   * assume that we're right about toggling and just send !orth->autorouting.
   */
  change = orthconn_set_autorouting ((OrthConn*) obj,
                                     !((OrthConn*) obj)->autorouting);
  orthconn_update_data ((OrthConn *) obj);

  return change;
}


void
orthconn_update_object_menu(OrthConn *orth, Point *clicked,
			    DiaMenuItem *object_menu_items)
{
  object_menu_items[0].active = DIAMENU_ACTIVE|DIAMENU_TOGGLE|
    (orth->autorouting?DIAMENU_TOGGLE_ON:0);
}
