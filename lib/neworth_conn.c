/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This file forked ; added a connection point in the middle of each 
 * segment. C.Chepelov, january 2000.
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

#include "neworth_conn.h"
#include "connectionpoint.h"
#include "message.h"
#include "diamenu.h"
#include "handle.h"

enum change_type {
  TYPE_ADD_SEGMENT,
  TYPE_REMOVE_SEGMENT
};

static ObjectChange *
midsegment_create_change(NewOrthConn *orth, enum change_type type,
			 int segment,
			 Point *point1, Point *point2,
			 Handle *handle1, Handle *handle2);

struct MidSegmentChange {
  ObjectChange obj_change;

  /* All additions and deletions of segments in the middle
   * of the NewOrthConn must delete/add two segments to keep
   * the horizontal/vertical balance.
   *
   * None of the end segments must be removed by this change.
   */
  
  enum change_type type;
  int applied;
  
  int segment;
  Point points[2]; 
  Handle *handles[2]; /* These handles cannot be connected */
  ConnectionPoint *conn; /* ? */
  ObjectChange *cplchange[2];
};

static ObjectChange *
endsegment_create_change(NewOrthConn *orth, enum change_type type,
			 int segment, Point *point,
			 Handle *handle);

struct EndSegmentChange {
  ObjectChange obj_change;

  /* Additions and deletions of segments of at the endpoints
   * of the NewOrthConn.
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
  ObjectChange *cplchange;
};


static void set_midpoint(Point *point, NewOrthConn *orth, int segment)
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

static int get_handle_nr(NewOrthConn *orth, Handle *handle)
{
  int i = 0;
  for (i=0;i<orth->numpoints-1;i++) {
    if (orth->handles[i] == handle)
      return i;
  }
  return -1;
}

static int get_segment_nr(NewOrthConn *orth, Point *point, real max_dist)
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


void
neworthconn_move_handle(NewOrthConn *orth, Handle *handle,
		     Point *to, HandleMoveReason reason)
{
  int n;
  int handle_nr;
  
  switch(handle->id) {
  case HANDLE_MOVE_STARTPOINT:
    orth->points[0] = *to;
    switch (orth->orientation[0]) {
    case HORIZONTAL:
      orth->points[1].y = to->y;
      break;
    case VERTICAL:
      orth->points[1].x = to->x;
      break;
    } 
    break;
  case HANDLE_MOVE_ENDPOINT:
    n = orth->numpoints - 1;
    orth->points[n] = *to;
    switch (orth->orientation[n-1]) {
    case HORIZONTAL:
      orth->points[n-1].y = to->y;
      break;
    case VERTICAL:
      orth->points[n-1].x = to->x;
      break;
    } 
    break;
  case HANDLE_MIDPOINT:
    n = orth->numpoints - 1;
    handle_nr = get_handle_nr(orth, handle);

    switch (orth->orientation[handle_nr]) {
    case HORIZONTAL:
      orth->points[handle_nr].y = to->y;
      orth->points[handle_nr+1].y = to->y;
      break;
    case VERTICAL:
      orth->points[handle_nr].x = to->x;
      orth->points[handle_nr+1].x = to->x;
      break;
    } 
    break;
  default:
    message_error("Internal error in neworthconn_move_handle.\n");
    break;
  }
}

void
neworthconn_move(NewOrthConn *orth, Point *to)
{
  Point p;
  int i;
  
  p = *to;
  point_sub(&p, &orth->points[0]);

  orth->points[0] = *to;
  for (i=1;i<orth->numpoints;i++) {
    point_add(&orth->points[i], &p);
  }
}

real
neworthconn_distance_from(NewOrthConn *orth, Point *point, real line_width)
{
  int i;
  real dist;
  
  dist = distance_line_point( &orth->points[0], &orth->points[1],
			      line_width, point);
  for (i=1;i<orth->numpoints-1;i++) {
    dist = MIN(dist,
	       distance_line_point( &orth->points[i], &orth->points[i+1],
				    line_width, point));
  }
  return dist;
}

static void
neworthconn_update_midpoints(NewOrthConn *orth)
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

void
neworthconn_update_data(NewOrthConn *orth)
{
  int i;
  Object *obj = (Object *)orth;
  
  obj->position = orth->points[0];

  /* 
     connpointline_update(orth->midpoints,orth->points[0],
                          orth->points[orth->numpoints-1]); 
  */
  /* Update handles : */
  orth->handles[0]->pos = orth->points[0];
  orth->handles[orth->numpoints-2]->pos = orth->points[orth->numpoints-1];

  for (i=1;i<orth->numpoints-2;i++) {
    set_midpoint(&orth->handles[i]->pos, orth, i);
  }
  neworthconn_update_midpoints(orth);
}

void
neworthconn_update_boundingbox(NewOrthConn *orth)
{
  Rectangle *bb;
  Point *points;
  int i;
  
  assert(orth != NULL);

  bb = &orth->object.bounding_box;
  points = &orth->points[0];

  bb->right = bb->left = points[0].x;
  bb->top = bb->bottom = points[0].y;
  for (i=1;i<orth->numpoints;i++) {
    if (points[i].x < bb->left)
      bb->left = points[i].x;
    if (points[i].x > bb->right)
      bb->right = points[i].x;
    if (points[i].y < bb->top)
      bb->top = points[i].y;
    if (points[i].y > bb->bottom)
      bb->bottom = points[i].y;
  }
}

void
neworthconn_simple_draw(NewOrthConn *orth, Renderer *renderer, real width)
{
  Point *points;
  
  assert(orth != NULL);
  assert(renderer != NULL);

  points = &orth->points[0];
  
  renderer->ops->set_linewidth(renderer, width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, orth->numpoints,
			       &color_black);
}


int
neworthconn_can_delete_segment(NewOrthConn *orth, Point *clickedpoint)
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
neworthconn_can_add_segment(NewOrthConn *orth, Point *clickedpoint)
{
  int segment = get_segment_nr(orth, clickedpoint, 1000000.0);

  if (segment<0)
    return 0;
  
  return 1;
}

/* Needs to have at least 2 handles. 
   The handles are stored in order in the NewOrthConn, but need
   not be stored in order in the Object.handles array. This
   is so that derived object can do what they want with
   Object.handles. */

void
neworthconn_init(NewOrthConn *orth, Point *startpoint)
{
  Object *obj;

  obj = &orth->object;

  object_init(obj, 3, 0);
  
  orth->numpoints = 4;

  orth->points = g_malloc(4*sizeof(Point));

  orth->orientation = g_malloc(3*sizeof(Orientation));

  orth->handles = g_malloc(3*sizeof(Handle *));

  orth->handles[0] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
  obj->handles[0] = orth->handles[0];
  
  orth->handles[1] = g_new(Handle, 1);
  setup_midpoint_handle(orth->handles[1]);
  obj->handles[1] = orth->handles[1];
  
  orth->handles[2] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[2], HANDLE_MOVE_ENDPOINT);
  obj->handles[2] = orth->handles[2];

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
 
  neworthconn_update_data(orth);
}

void
neworthconn_copy(NewOrthConn *from, NewOrthConn *to)
{
  int i,rcc;
  Object *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  to->numpoints = from->numpoints;

  to->points = g_malloc((to->numpoints)*sizeof(Point));

  for (i=0;i<to->numpoints;i++) {
    to->points[i] = from->points[i];
  }

  to->orientation = g_malloc((to->numpoints-1)*sizeof(Orientation));
  to->handles = g_malloc((to->numpoints-1)*sizeof(Handle *));

  for (i=0;i<to->numpoints-1;i++) {
    to->orientation[i] = from->orientation[i];
    to->handles[i] = g_new(Handle,1);
    *to->handles[i] = *from->handles[i];
    to->handles[i]->connected_to = NULL;
    toobj->handles[i] = to->handles[i];
  }
  rcc = 0;
  to->midpoints = connpointline_copy(toobj,from->midpoints,&rcc);
}


void
neworthconn_destroy(NewOrthConn *orth)
{
  int i;

  connpointline_destroy(orth->midpoints);
  object_destroy(&orth->object);
  
  g_free(orth->points);
  g_free(orth->orientation);

  for (i=0;i<orth->numpoints-1;i++) {
    g_free(orth->handles[i]);
  }
  g_free(orth->handles);
}

static void 
place_handle_by_swapping(NewOrthConn *orth, int index, Handle *handle)
{
  Object *obj;
  Handle *tmp;
  int j;

  obj = (Object *)orth;
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
neworthconn_save(NewOrthConn *orth, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;

  /* Make sure start-handle is first and end-handle is second. */
  place_handle_by_swapping(orth, 0, orth->handles[0]);
  place_handle_by_swapping(orth, 1, orth->handles[orth->numpoints-2]);
  
  object_save(&orth->object, obj_node);

  attr = new_attribute(obj_node, "orth_points");
  
  for (i=0;i<orth->numpoints;i++) {
    data_add_point(attr, &orth->points[i]);
  }

  attr = new_attribute(obj_node, "orth_orient");
  for (i=0;i<orth->numpoints-1;i++) {
    data_add_enum(attr, orth->orientation[i]);
  }
}

void
neworthconn_load(NewOrthConn *orth, ObjectNode obj_node) /* NOTE: Does object_init() */
{
  int i;
  AttributeNode attr;
  DataNode data;
  int n;
  
  Object *obj = &orth->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "orth_points");

  if (attr != NULL)
    orth->numpoints = attribute_num_data(attr);
  else
    orth->numpoints = 0;

  object_init(obj, orth->numpoints-1,0);

  data = attribute_first_data(attr);
  orth->points = g_malloc((orth->numpoints)*sizeof(Point));
  for (i=0;i<orth->numpoints;i++) {
    data_point(data, &orth->points[i]);
    data = data_next(data);
  }

  attr = object_find_attribute(obj_node, "orth_orient");

  data = attribute_first_data(attr);
  orth->orientation = g_malloc((orth->numpoints-1)*sizeof(Orientation));
  for (i=0;i<orth->numpoints-1;i++) {
    orth->orientation[i] = data_enum(data);
    data = data_next(data);
  }

  orth->handles = g_malloc((orth->numpoints-1)*sizeof(Handle *));

  orth->handles[0] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
  orth->handles[0]->pos = orth->points[0];
  obj->handles[0] = orth->handles[0];

  n = orth->numpoints-2;
  orth->handles[n] = g_new(Handle, 1);
  setup_endpoint_handle(orth->handles[n], HANDLE_MOVE_ENDPOINT);
  orth->handles[n]->pos = orth->points[orth->numpoints-1];
  obj->handles[1] = orth->handles[n];

  for (i=1; i<orth->numpoints-2; i++) {
    orth->handles[i] = g_new(Handle, 1);
    setup_midpoint_handle(orth->handles[i]);
    obj->handles[i+1] = orth->handles[i];
  }

  orth->midpoints = connpointline_create(obj,orth->numpoints-1);

  neworthconn_update_data(orth);
}

Handle*
neworthconn_get_middle_handle( NewOrthConn *orth )
{
  int n = orth->numpoints - 1 ;
  return orth->handles[ n/2 ] ;
}

ObjectChange *
neworthconn_delete_segment(NewOrthConn *orth, Point *clickedpoint)
{
  int segment;
  ObjectChange *change = NULL;
  
  if (orth->numpoints==3)
    return NULL;
  
  segment = get_segment_nr(orth, clickedpoint, 1.0);
  if (segment < 0)
    return NULL;

  if (segment==0) {
    change = endsegment_create_change(orth, TYPE_REMOVE_SEGMENT, segment,
				      &orth->points[segment],
				      orth->handles[segment]);
  } else if (segment == orth->numpoints-2) {
    change = endsegment_create_change(orth, TYPE_REMOVE_SEGMENT, segment,
				      &orth->points[segment+1],
				      orth->handles[segment]);
  } else if (segment > 0) {
    /* Don't delete the last midpoint segment.
     * That would delete also the endpoint segment after it.
     */
    if (segment == orth->numpoints-3)
      segment--; 
    
    change = midsegment_create_change(orth, TYPE_REMOVE_SEGMENT, segment,
				      &orth->points[segment],
				      &orth->points[segment+1],
				      orth->handles[segment],
				      orth->handles[segment+1]);
  }

  change->apply(change, (Object *)orth);
  
  return change;
}

ObjectChange *
neworthconn_add_segment(NewOrthConn *orth, Point *clickedpoint)
{
  Handle *handle1, *handle2;
  ObjectChange *change = NULL;
  int segment;
  Point newpoint;
  
  segment = get_segment_nr(orth, clickedpoint, 1.0);
  if (segment < 0)
    return NULL;

  if (segment==0) { /* First segment */
    handle1 = g_new(Handle, 1);
    setup_endpoint_handle(handle1, HANDLE_MOVE_STARTPOINT);
    change = endsegment_create_change(orth, TYPE_ADD_SEGMENT,
				      0, &orth->points[0],
				      handle1);
  } else if (segment == orth->numpoints-2) { /* Last segment */
    handle1 = g_new(Handle, 1);
    setup_endpoint_handle(handle1, HANDLE_MOVE_ENDPOINT);
    change = endsegment_create_change(orth, TYPE_ADD_SEGMENT,
				      segment+1, &orth->points[segment+1],
				      handle1);
  } else if (segment > 0) {
    handle1 = g_new(Handle, 1);
    setup_midpoint_handle(handle1);
    handle2 = g_new(Handle, 1);
    setup_midpoint_handle(handle2);
    newpoint = *clickedpoint;
    if (orth->orientation[segment]==HORIZONTAL)
      newpoint.y = orth->points[segment].y;
    else
      newpoint.x = orth->points[segment].x;
    
    change = midsegment_create_change(orth, TYPE_ADD_SEGMENT, segment,
				      &newpoint,
				      &newpoint,
				      handle1,
				      handle2);
  }

  change->apply(change, (Object *)orth);

  return change;
}

static void
delete_point(NewOrthConn *orth, int pos)
{
  int i;
  
  orth->numpoints--;

  for (i=pos;i<orth->numpoints;i++) {
    orth->points[i] = orth->points[i+1];
  }
  
  orth->points = g_realloc(orth->points, orth->numpoints*sizeof(Point));
}

/* Make sure numpoints have been decreased before calling this function.
 * ie. call delete_point first.
 */
static void
remove_handle(NewOrthConn *orth, int segment)
{
  int i;
  Handle *handle;

  handle = orth->handles[segment];
  
  for (i=segment; i < orth->numpoints-1; i++) {
    orth->handles[i] = orth->handles[i+1];
    orth->orientation[i] = orth->orientation[i+1];
  }

  orth->orientation = g_realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->handles = g_realloc(orth->handles,
			  (orth->numpoints-1)*sizeof(Handle *));
  
  object_remove_handle(&orth->object, handle);
}


static void
add_point(NewOrthConn *orth, int pos, Point *point)
{
  int i;
  
  orth->numpoints++;

  orth->points = g_realloc(orth->points, orth->numpoints*sizeof(Point));
  for (i=orth->numpoints-1;i>pos;i--) {
    orth->points[i] = orth->points[i-1];
  }
  orth->points[pos] = *point;
}

/* Make sure numpoints have been increased before calling this function.
 * ie. call add_point first.
 */
static void
insert_handle(NewOrthConn *orth, int segment,
	      Handle *handle, Orientation orient)
{
  int i;
  
  orth->orientation = g_realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->handles = g_realloc(orth->handles,
			  (orth->numpoints-1)*sizeof(Handle *));
  for (i=orth->numpoints-2;i>segment;i--) {
    orth->handles[i] = orth->handles[i-1];
    orth->orientation[i] = orth->orientation[i-1];
  }
  orth->handles[segment] = handle;
  orth->orientation[segment] = orient;
  
  object_add_handle(&orth->object, handle);
}



static void
endsegment_change_free(struct EndSegmentChange *change)
{
  if ( (change->type==TYPE_ADD_SEGMENT && !change->applied) ||
       (change->type==TYPE_REMOVE_SEGMENT && change->applied) ){
    if (change->handle)
      g_free(change->handle);
    change->handle = NULL;
  }
  if (change->cplchange) {
    if (change->cplchange->free) change->cplchange->free(change->cplchange);
    g_free(change->cplchange);
    change->cplchange = NULL;
  }
}

static void
endsegment_change_apply(struct EndSegmentChange *change, Object *obj)
{
  NewOrthConn *orth = (NewOrthConn *)obj;

  change->applied = 1;

  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    object_unconnect(obj, change->old_end_handle);
    if (change->segment==0) { /* first */
      add_point(orth, 0, &change->point);
      insert_handle(orth, change->segment,
		    change->handle, FLIP_ORIENT(orth->orientation[0]) );
      setup_midpoint_handle(orth->handles[1]);
      obj->position = orth->points[0];
      change->cplchange = connpointline_add_point(orth->midpoints,
						  &change->point);
    } else { /* last */
      add_point(orth, orth->numpoints, &change->point);
      insert_handle(orth, change->segment, change->handle,
		    FLIP_ORIENT(orth->orientation[orth->numpoints-3]) );
      setup_midpoint_handle(orth->handles[orth->numpoints-3]);
      change->cplchange = connpointline_add_point(orth->midpoints,
						  &orth->midpoints->end);      
    }
    break;
  case TYPE_REMOVE_SEGMENT:
    object_unconnect(obj, change->old_end_handle);
    change->cplchange = 
      connpointline_remove_point(orth->midpoints,
				 &orth->points[change->segment]);
    if (change->segment==0) { /* first */
      delete_point(orth, 0);
      remove_handle(orth, 0);
      setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
      obj->position = orth->points[0];
    } else { /* last */
      delete_point(orth, orth->numpoints-1);
      remove_handle(orth, change->segment);
      setup_endpoint_handle(orth->handles[orth->numpoints-2],
			    HANDLE_MOVE_ENDPOINT);
    }
    break;
  }
  neworthconn_update_midpoints(orth); /* useless ? */
}

static void
endsegment_change_revert(struct EndSegmentChange *change, Object *obj)
{
  NewOrthConn *orth = (NewOrthConn *)obj;

  change->cplchange->revert(change->cplchange,obj);    
  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    object_unconnect(obj, change->handle);
    if (change->segment==0) { /* first */
      delete_point(orth, 0);
      remove_handle(orth, 0);
      setup_endpoint_handle(orth->handles[0], HANDLE_MOVE_STARTPOINT);
      obj->position = orth->points[0];
    } else { /* last */
      delete_point(orth, orth->numpoints-1);
      remove_handle(orth, change->segment);
      setup_endpoint_handle(orth->handles[orth->numpoints-2],
			    HANDLE_MOVE_ENDPOINT);
    }
    break;
  case TYPE_REMOVE_SEGMENT:
    if (change->segment==0) { /* first */
      add_point(orth, 0, &change->point);
      insert_handle(orth, change->segment,
		    change->handle, FLIP_ORIENT(orth->orientation[0]) );
      setup_midpoint_handle(orth->handles[1]);
      obj->position = orth->points[0];
    } else { /* last */
      add_point(orth, orth->numpoints, &change->point);
      insert_handle(orth, change->segment, change->handle,
		    FLIP_ORIENT(orth->orientation[orth->numpoints-3]) );
      setup_midpoint_handle(orth->handles[orth->numpoints-3]);
    }
    break;
  }
  change->applied = 0;
  neworthconn_update_midpoints(orth); /* useless ? */
}

static ObjectChange *
endsegment_create_change(NewOrthConn *orth, enum change_type type,
			 int segment, Point *point,
			 Handle *handle)
{
  struct EndSegmentChange *change;

  change = g_new0(struct EndSegmentChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) endsegment_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) endsegment_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) endsegment_change_free;

  change->type = type;
  change->applied = 0;
  change->segment = segment;
  change->point = *point;
  change->handle = handle;
  if (segment == 0)
    change->old_end_handle = orth->handles[0];
  else
    change->old_end_handle = orth->handles[orth->numpoints-2];
  change->cp = change->old_end_handle->connected_to;
  return (ObjectChange *)change;
}


static void
midsegment_change_free(struct MidSegmentChange *change)
{
  if ( (change->type==TYPE_ADD_SEGMENT && !change->applied) ||
       (change->type==TYPE_REMOVE_SEGMENT && change->applied) ){
    if (change->handles[0])
      g_free(change->handles[0]);
    change->handles[0] = NULL;
    if (change->handles[1])
      g_free(change->handles[1]);
    change->handles[1] = NULL;
  }

  if (change->cplchange[0]) {
    if (change->cplchange[0]->free) 
      change->cplchange[0]->free(change->cplchange[0]);
    g_free(change->cplchange[0]);
    change->cplchange[0] = NULL;
  }
  if (change->cplchange[1]) {
    if (change->cplchange[1]->free) 
      change->cplchange[1]->free(change->cplchange[1]);
    g_free(change->cplchange[1]);
    change->cplchange[1] = NULL;
  }
}

static void
midsegment_change_apply(struct MidSegmentChange *change, Object *obj)
{
  NewOrthConn *orth = (NewOrthConn *)obj;
  int seg;

  change->applied = 1;

  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    add_point(orth, change->segment+1, &change->points[1]);
    add_point(orth, change->segment+1, &change->points[0]);
    insert_handle(orth, change->segment+1, change->handles[1],
		  orth->orientation[change->segment] );
    insert_handle(orth, change->segment+1, change->handles[0],
		  FLIP_ORIENT(orth->orientation[change->segment]) );
    change->cplchange[0] = 
      connpointline_add_point(orth->midpoints,&change->points[0]);
    change->cplchange[1] = 
      connpointline_add_point(orth->midpoints,&change->points[1]);
    break;
  case TYPE_REMOVE_SEGMENT:
    seg = change->segment?change->segment:1;
    change->cplchange[0] = 
      connpointline_remove_point(orth->midpoints,
				 &orth->points[seg-1]);
    change->cplchange[1] = 
      connpointline_remove_point(orth->midpoints,
				 &orth->points[seg]);
    delete_point(orth, change->segment);
    remove_handle(orth, change->segment);
    delete_point(orth, change->segment);
    remove_handle(orth, change->segment);
    if (orth->orientation[change->segment]==HORIZONTAL) {
      orth->points[change->segment].x = change->points[0].x;
    } else {
      orth->points[change->segment].y = change->points[0].y;
    }
    break;
  }
  neworthconn_update_midpoints(orth); /* useless ? */
}

static void
midsegment_change_revert(struct MidSegmentChange *change, Object *obj)
{
  NewOrthConn *orth = (NewOrthConn *)obj;
  
  change->cplchange[0]->revert(change->cplchange[0],obj);
  change->cplchange[1]->revert(change->cplchange[1],obj);

  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    delete_point(orth, change->segment+1);
    remove_handle(orth, change->segment+1);
    delete_point(orth, change->segment+1);
    remove_handle(orth, change->segment+1);
    break;
  case TYPE_REMOVE_SEGMENT:
    if (orth->orientation[change->segment]==HORIZONTAL) {
      orth->points[change->segment].x = change->points[1].x;
    } else {
      orth->points[change->segment].y = change->points[1].y;
    }
    add_point(orth, change->segment, &change->points[1]);
    add_point(orth, change->segment, &change->points[0]);
    insert_handle(orth, change->segment, change->handles[1],
		  orth->orientation[change->segment-1] );
    insert_handle(orth, change->segment, change->handles[0],
		  FLIP_ORIENT(orth->orientation[change->segment-1]) );
    break;
  }
  change->applied = 0;
}

static ObjectChange *
midsegment_create_change(NewOrthConn *orth, enum change_type type,
			 int segment,
			 Point *point1, Point *point2,
			 Handle *handle1, Handle *handle2)
{
  struct MidSegmentChange *change;

  change = g_new(struct MidSegmentChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) midsegment_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) midsegment_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) midsegment_change_free;

  change->type = type;
  change->applied = 0;
  change->segment = segment;
  change->points[0] = *point1;
  change->points[1] = *point2;
  change->handles[0] = handle1;
  change->handles[1] = handle2;

  return (ObjectChange *)change;
}




