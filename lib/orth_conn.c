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
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "orth_conn.h"
#include "message.h"
#include "diamenu.h"
#include "handle.h"

enum change_type {
  TYPE_ADD_SEGMENT,
  TYPE_REMOVE_SEGMENT
};

struct SegmentChange {
  ObjectChange obj_change;

  enum change_type type;
  int applied;
  
  int segment;
  Point point1;
  Point point2;
  
  Handle *handle1;
  Handle *handle2;
  ConnectionPoint *cp;
};

static ObjectChange *
orthconn_create_change(OrthConn *orth, enum change_type type,
		       int segment, Point *point1, Point *point2, 
		       Handle *handle1, Handle *handle2, 
		       ConnectionPoint *cp);

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

static int get_handle_nr(OrthConn *orth, Handle *handle)
{
  int i = 0;
  for (i=0;i<orth->numpoints-1;i++) {
    if (orth->midpoint_handles[i] == handle)
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
    printf("i: %d\n", i);
    tmp_dist = distance_line_point(&orth->points[i], &orth->points[i+1], 0, point);
    if (tmp_dist < distance) {
      segment = i;
      distance = tmp_dist;
    }
  }

  printf("distance: %f, segment: %d\n", distance, segment);
  if (distance < max_dist)
    return segment;
  
  return -1;
}


void
orthconn_move_handle(OrthConn *orth, Handle *handle,
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
    message_error("Internal error in orthconn_move_handle.\n");
    break;
  }
}

void
orthconn_move(OrthConn *orth, Point *to)
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
orthconn_distance_from(OrthConn *orth, Point *point, real line_width)
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

/* Removes segment nr 'segment' and point nr 'segment' */
static void
remove_segment_before(OrthConn *orth, int segment)
{
  int i;
  

  if (orth->midpoint_handles[segment]) {
    object_remove_handle(&orth->object, orth->midpoint_handles[segment]);
  }
			 
  /* delete the points */
  orth->numpoints = orth->numpoints - 1;
  for (i=segment; i < orth->numpoints; i++) {
    orth->points[i] = orth->points[i+1];
  }
  orth->points = realloc(orth->points, orth->numpoints*sizeof(Point));

  /* Remove the segment and handle: */
  for (i=segment; i < orth->numpoints-1; i++) {
    orth->midpoint_handles[i] = orth->midpoint_handles[i+1];
    orth->orientation[i] = orth->orientation[i+1];
  }

  orth->orientation = realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->midpoint_handles = realloc(orth->midpoint_handles,
				   (orth->numpoints-1)*sizeof(Handle *));
}

/* Removes segment nr 'segment' and point nr 'segment+1' */
static void
remove_segment_after(OrthConn *orth, int segment)
{
  int i;
  
  if (orth->midpoint_handles[segment]) {
    object_remove_handle(&orth->object, orth->midpoint_handles[segment]);
  }
			 
  /* delete the points */
  orth->numpoints = orth->numpoints - 1;
  for (i=segment+1; i < orth->numpoints; i++) {
    orth->points[i] = orth->points[i+1];
  }
  orth->points = realloc(orth->points, orth->numpoints*sizeof(Point));

  /* Remove the segment: */
  for (i=segment; i < orth->numpoints-1; i++) {
    orth->midpoint_handles[i] = orth->midpoint_handles[i+1];
    orth->orientation[i] = orth->orientation[i+1];
  }

  orth->orientation = realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->midpoint_handles = realloc(orth->midpoint_handles,
				   (orth->numpoints-1)*sizeof(Handle *));
}


void
orthconn_update_data(OrthConn *orth)
{
  int i;
  Object *obj = (Object *)orth;
  
  /* Update handles: */
  orth->endpoint_handles[0].pos = orth->points[0];
  orth->endpoint_handles[1].pos = orth->points[orth->numpoints-1];

  obj->position = orth->points[0];

  for (i=1;i<orth->numpoints-2;i++) {
    set_midpoint(&orth->midpoint_handles[i]->pos, orth, i);
  }
}

void
orthconn_update_boundingbox(OrthConn *orth)
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
orthconn_simple_draw(OrthConn *orth, Renderer *renderer, real width)
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
orthconn_can_delete_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment;
  
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


void delete_segment(OrthConn *orth, int segment,
		    Handle **handle1, Handle **handle2)
{
  *handle1 = NULL;
  *handle2 = NULL;

  printf("deleting segment %d\n", segment);
  
  if (segment==0) {
    remove_segment_before(orth, 0);
    object_remove_handle(&orth->object, orth->midpoint_handles[0]);
    *handle1 = orth->midpoint_handles[0];
    orth->midpoint_handles[0] = NULL;
  } else if (segment == orth->numpoints-2) {
    object_remove_handle(&orth->object, orth->midpoint_handles[segment-1]);
    *handle1 = orth->midpoint_handles[segment-1];
    orth->midpoint_handles[segment-1] = NULL;
    remove_segment_after(orth, segment);
  } else if (segment > 0) {
    if (orth->numpoints == 4)
      return;
    if (segment == orth->numpoints-3) { /* next last segment */
      *handle1 = orth->midpoint_handles[segment-1];
      remove_segment_after(orth, segment-1);
      *handle2 = orth->midpoint_handles[segment-1];
      remove_segment_after(orth, segment-1);
      switch(orth->orientation[segment-2]) {
      case HORIZONTAL:
	orth->points[segment-1].x = orth->points[segment].x;
	break;
      case VERTICAL:
	orth->points[segment-1].y = orth->points[segment].y;
	break;
      }
    } else {
      *handle1 = orth->midpoint_handles[segment];
      remove_segment_before(orth, segment);
      *handle2 = orth->midpoint_handles[segment];
      remove_segment_before(orth, segment);
      switch(orth->orientation[segment]) {
      case HORIZONTAL:
	orth->points[segment].x = orth->points[segment-1].x;
	break;
      case VERTICAL:
	orth->points[segment].y = orth->points[segment-1].y;
	break;
      }
    }
  }
}


ObjectChange *
orthconn_delete_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment;
  Handle *handle1, *handle2;
  Point point1,  point2;
  
  if (orth->numpoints==3)
    return NULL;
  
  segment = get_segment_nr(orth, clickedpoint, 1.0);
  if (segment < 0)
    return NULL;

  point1 = orth->points[segment];
  point2 = orth->points[segment+1];
  
  printf("deleting segment %d, point1:%f, %f point1:%f, %f\n",
	 segment, point1.x, point1.y,point2.x, point2.y);
  
  delete_segment(orth, segment, &handle1, &handle2);
  return orthconn_create_change(orth, TYPE_REMOVE_SEGMENT,
				segment, &point1, &point2,
				handle1, handle2, NULL);
}

int
orthconn_can_add_segment(OrthConn *orth, Point *clickedpoint)
{
  return 1; /* TODO: Fix. */
}


static void
add_first_segment(OrthConn *orth, Handle *new_handle,
		  Point *point1, Point *point2)
{
  int i;
  Object *obj = (Object *) orth;
  
  orth->numpoints++;
  /* Add an extra point first: */
  orth->points = realloc(orth->points, orth->numpoints*sizeof(Point));
  for (i=orth->numpoints-1;i>=1;i--) {
    orth->points[i] = orth->points[i-1];
  }
  /* Add extra line-segment first: */
  orth->orientation = realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->midpoint_handles = realloc(orth->midpoint_handles,
				   (orth->numpoints-1)*sizeof(Handle *));
  for (i=orth->numpoints-2;i>0;i--) {
    orth->midpoint_handles[i] = orth->midpoint_handles[i-1];
    orth->orientation[i] = orth->orientation[i-1];
  }
  orth->orientation[0] = FLIP_ORIENT(orth->orientation[1]);
  orth->midpoint_handles[0] = NULL;
  orth->midpoint_handles[1] = new_handle;
  setup_midpoint_handle(orth->midpoint_handles[1]);

  if (point1) {
    orth->points[0] = *point1;
  } else {
    orth->points[0] = orth->points[1];
  }
  if (point2) {
    orth->points[1] = *point2;
  }
  
  set_midpoint(&orth->midpoint_handles[1]->pos, orth, 1);

  object_add_handle(&orth->object, orth->midpoint_handles[1]);

  obj->position = orth->points[0];
}

static void
add_last_segment(OrthConn *orth, Handle *new_handle,
		 Point *point1, Point *point2)
{
  int n;
  
  orth->numpoints++;

  n = orth->numpoints - 1; /* last point */
  /* Add an extra point last: */
  orth->points = realloc(orth->points, orth->numpoints*sizeof(Point));
  orth->points[n] = orth->points[n-1];
  /* Add extra line-segment last: */
  orth->orientation = realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->midpoint_handles = realloc(orth->midpoint_handles,
				   (orth->numpoints-1)*sizeof(Handle *));
  orth->orientation[n-1] = FLIP_ORIENT(orth->orientation[n-2]);
  orth->midpoint_handles[n-1] = NULL;
  orth->midpoint_handles[n-2] = new_handle;
  setup_midpoint_handle(orth->midpoint_handles[n-2]);

  orth->points[n-1].x = orth->points[n].x;
  orth->points[n-1].y = orth->points[n].y;
  
  if (point1) {
    orth->points[n-1] = *point1;
  }

  if (point2) {
    orth->points[n] = *point2;
  }

  set_midpoint(&orth->midpoint_handles[n-2]->pos, orth, n-2);
  
  object_add_handle(&orth->object, orth->midpoint_handles[n-2]);
}

static void
add_middle_segment(OrthConn *orth,
		   int segment, Point *point1, Point *point2,
		   Handle *handle1, Handle *handle2 )
{
  int i;
  
  orth->numpoints += 2;

  /* Add two extra points after segment: */
  orth->points = realloc(orth->points, orth->numpoints*sizeof(Point));
  for (i=orth->numpoints-1;i>segment+2;i--) {
    orth->points[i] = orth->points[i-2];
    
  }

  /* Add extra line-segment first: */
  orth->orientation = realloc(orth->orientation,
			      (orth->numpoints-1)*sizeof(Orientation));
  orth->midpoint_handles = realloc(orth->midpoint_handles,
				   (orth->numpoints-1)*sizeof(Handle *));
  for (i=orth->numpoints-2;i>segment+2;i--) {
    orth->midpoint_handles[i] = orth->midpoint_handles[i-2];
    orth->orientation[i] = orth->orientation[i-2];
  }
  orth->orientation[segment+1] = FLIP_ORIENT(orth->orientation[segment]);
  orth->orientation[segment+2] = orth->orientation[segment];
  orth->midpoint_handles[segment+1] = handle1;
  setup_midpoint_handle(orth->midpoint_handles[segment+1]);
  orth->midpoint_handles[segment+2] = handle2;
  setup_midpoint_handle(orth->midpoint_handles[segment+2]);

  switch (orth->orientation[segment]) {
  case HORIZONTAL:
    orth->points[segment+1].x = point1->x;
    orth->points[segment+1].y = orth->points[segment].y;
    orth->points[segment+2].x = point1->x;
    orth->points[segment+2].y = orth->points[segment].y;
    break;
  case VERTICAL:
    orth->points[segment+1].x = orth->points[segment].x;
    orth->points[segment+1].y = point1->y;
    orth->points[segment+2].x = orth->points[segment].x;
    orth->points[segment+2].y = point1->y;
    break;
  }

  if (point1) {
    orth->points[segment+1] = *point1;
  }
  if (point2) {
    orth->points[segment+2] = *point2;
  }
  
  if (orth->midpoint_handles[segment])
    set_midpoint(&orth->midpoint_handles[segment]->pos, orth, 0);
  set_midpoint(&orth->midpoint_handles[segment+1]->pos, orth, 0);
  set_midpoint(&orth->midpoint_handles[segment+2]->pos, orth, 0);

  object_add_handle(&orth->object, orth->midpoint_handles[segment+1]);
  object_add_handle(&orth->object, orth->midpoint_handles[segment+2]);
}

ObjectChange *
orthconn_add_segment(OrthConn *orth, Point *clickedpoint)
{
  int segment;
  Handle *handle1, *handle2;

  segment = get_segment_nr(orth, clickedpoint, 1000000.0);

  handle1 = handle2 = NULL;
  
  if (segment==0) {
    handle1 = g_new(Handle, 1);
    add_first_segment(orth, handle1, NULL, NULL);
  } else if (segment == orth->numpoints-2) {
    handle1 = g_new(Handle, 1);
    add_last_segment(orth, handle1, NULL, NULL);
  } else if (segment > 0) {
    handle1 = g_new(Handle, 1);
    handle2 = g_new(Handle, 1);
    add_middle_segment(orth, segment, clickedpoint, NULL,
		       handle1, handle2);
  }
  
  return orthconn_create_change(orth, TYPE_ADD_SEGMENT,
				segment, clickedpoint, NULL,
				handle1, handle2, NULL);
}


/* Needs to have at least 2 handles. 
   The first 2 are used. Handles are dynamicaly moved created/deleted,
   so don't rely on them having a special index.
   The midpoint handles for the end segments are NULL. */
void
orthconn_init(OrthConn *orth, Point *startpoint)
{
  Object *obj;

  obj = &orth->object;

  object_init(obj, 2, 0);
  
  orth->numpoints = 3;

  orth->points = g_malloc(3*sizeof(Point));

  orth->orientation = g_malloc(2*sizeof(Orientation));

  obj->handles[0] = &orth->endpoint_handles[0];
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  
  obj->handles[1] = &orth->endpoint_handles[1];
  obj->handles[1]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[1]->connected_to = NULL;
  obj->handles[1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[1]->id = HANDLE_MOVE_ENDPOINT;

  orth->midpoint_handles = g_malloc(2*sizeof(Handle *));
  
  orth->midpoint_handles[0] = NULL;
  orth->midpoint_handles[1] = NULL;

  /* Just so we have some position: */
  orth->points[0] = *startpoint;
  orth->points[1].x = startpoint->x;
  orth->points[1].y = startpoint->y + 1.0;
  orth->points[2].x = startpoint->x + 1.0;
  orth->points[2].y = startpoint->y + 1.0;

  orthconn_update_data(orth);

  orth->orientation[0] = VERTICAL;
  orth->orientation[1] = HORIZONTAL;
}

void
orthconn_copy(OrthConn *from, OrthConn *to)
{
  int i;
  int handle_nr;
  Object *toobj, *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  to->numpoints = from->numpoints;

  to->points = g_malloc((to->numpoints)*sizeof(Point));

  for (i=0;i<to->numpoints;i++) {
    to->points[i] = from->points[i];
  }

  for (i=0;i<2;i++) {
    to->endpoint_handles[i] = from->endpoint_handles[i];
    to->endpoint_handles[i].connected_to = NULL;
    toobj->handles[i] = &to->endpoint_handles[i];
  }
  
  to->orientation = g_malloc((to->numpoints-1)*sizeof(Orientation));
  to->midpoint_handles = g_malloc((to->numpoints-1)*sizeof(Handle *));

  handle_nr = 2;
  for (i=0;i<to->numpoints-1;i++) {
    to->orientation[i] = from->orientation[i];
    if (from->midpoint_handles[i] != NULL) {
      to->midpoint_handles[i] = g_new(Handle,1);
      setup_midpoint_handle(to->midpoint_handles[i]);
      to->midpoint_handles[i]->pos = from->midpoint_handles[i]->pos;
      toobj->handles[handle_nr] = to->midpoint_handles[i];
      handle_nr++;
    } else {
      to->midpoint_handles[i] = NULL;
    }
  }
}

void
orthconn_destroy(OrthConn *orth)
{
  int i;

  object_destroy(&orth->object);
  
  g_free(orth->points);
  g_free(orth->orientation);

  for (i=0;i<orth->numpoints-1;i++)
    if (orth->midpoint_handles[i])
      g_free(orth->midpoint_handles[i]);
  
  g_free(orth->midpoint_handles);
}


void
orthconn_save(OrthConn *orth, ObjectNode obj_node)
{
  int i;
  AttributeNode attr;

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
orthconn_load(OrthConn *orth, ObjectNode obj_node) /* NOTE: Does object_init() */
{
  int i;
  AttributeNode attr;
  DataNode data;
  int handle_nr;
  
  Object *obj = &orth->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "orth_points");

  if (attr != NULL)
    orth->numpoints = attribute_num_data(attr);
  else
    orth->numpoints = 0;

  object_init(obj, 2 + orth->numpoints-3, 0);

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

  obj->handles[0] = &orth->endpoint_handles[0];
  obj->handles[0]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_MOVE_STARTPOINT;
  obj->handles[0]->pos = orth->points[0];
  
  obj->handles[1] = &orth->endpoint_handles[1];
  obj->handles[1]->connect_type = HANDLE_CONNECTABLE;
  obj->handles[1]->connected_to = NULL;
  obj->handles[1]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[1]->id = HANDLE_MOVE_ENDPOINT;
  obj->handles[1]->pos = orth->points[orth->numpoints-1];

  orth->midpoint_handles = g_malloc((orth->numpoints-1)*sizeof(Handle *));

  orth->midpoint_handles[0] = NULL;
  orth->midpoint_handles[1] = NULL;
  handle_nr = 2;
  for (i=1;i<orth->numpoints-2;i++) {
    orth->midpoint_handles[i] = g_new(Handle, 1);
    setup_midpoint_handle(orth->midpoint_handles[i]);
    obj->handles[handle_nr++] = orth->midpoint_handles[i];
  }

  orthconn_update_data(orth);
}

Handle*
orthconn_get_middle_handle( OrthConn *orth )
{
  int n = orth->numpoints - 1 ;
  return orth->midpoint_handles[ n/2 ] ;
}



static void
orthconn_change_free(struct SegmentChange *change)
{
  if ( (change->type==TYPE_ADD_SEGMENT && !change->applied) ||
       (change->type==TYPE_REMOVE_SEGMENT && change->applied) ){
    if (change->handle1)
      g_free(change->handle1);
    change->handle1 = NULL;
    if (change->handle2)
      g_free(change->handle2);
    change->handle2 = NULL;
  }

}

static void
orthconn_change_apply(struct SegmentChange *change, Object *obj)
{
  Handle *h1, *h2;
  OrthConn *orth = (OrthConn *)obj;
  change->applied = 1;
  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    if (change->segment==0) {
      add_first_segment(orth, change->handle1, NULL, NULL);
    } else if (change->segment == orth->numpoints-2) {
      add_last_segment(orth, change->handle1, NULL, NULL);
    } else if (change->segment > 0) {
      add_middle_segment(orth, change->segment, &change->point1, NULL,
			 change->handle1, change->handle2);
    }
    break;
  case TYPE_REMOVE_SEGMENT:
    delete_segment(orth, change->segment, &h1, &h2);
    break;
  }
}

static void
orthconn_change_revert(struct SegmentChange *change, Object *obj)
{
  Handle *h1, *h2;
  OrthConn *orth = (OrthConn *)obj;
  
  switch (change->type) {
  case TYPE_ADD_SEGMENT:
    if (change->segment==0)
      delete_segment(orth, 0, &h1, &h2);
    else
      delete_segment(orth, change->segment+1, &h1, &h2);
    break;
  case TYPE_REMOVE_SEGMENT:
    printf("Undoing remove segment %d\n", change->segment);
    if (change->segment==0) {
      add_first_segment(orth, change->handle1,
			&change->point1, &change->point2);
    } else if (change->segment == orth->numpoints-2) {
      add_last_segment(orth, change->handle1,
		       &change->point1, &change->point2);
    } else if (change->segment > 0) {
      add_middle_segment(orth, change->segment,
			 &change->point1, &change->point2,
			 change->handle1, change->handle2);
    }
    break;
  }
  change->applied = 0;
}

static ObjectChange *
orthconn_create_change(OrthConn *orth, enum change_type type,
		       int segment, Point *point1, Point *point2,
		       Handle *handle1, Handle *handle2, 
		       ConnectionPoint *cp)
{
  struct SegmentChange *change;

  change = g_new(struct SegmentChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) orthconn_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) orthconn_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) orthconn_change_free;

  change->type = type;
  change->applied = 1;
  change->segment = segment;
  change->point1 = *point1;
  change->point2 = *point2;
  change->handle1 = handle1;
  change->handle2 = handle2;
  change->cp = cp;

  return (ObjectChange *)change;
}
