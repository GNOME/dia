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


static void orthconn_try_remove_segments(OrthConn *orth, int segment);

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

void
orthconn_move_handle(OrthConn *orth, Handle *handle,
		     Point *to, HandleMoveReason reason)
{
  int n,i;
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
  

  object_remove_handle(&orth->object, orth->midpoint_handles[segment]);
  g_free(orth->midpoint_handles[segment]);
			 
  /* delete the points */
  orth->numpoints = orth->numpoints - 1;
  for (i=segment; i < orth->numpoints; i++) {
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

/* Removes segment nr 'segment' and point nr 'segment+1' */
static void
remove_segment_after(OrthConn *orth, int segment)
{
  int i;
  
  object_remove_handle(&orth->object, orth->midpoint_handles[segment]);
  g_free(orth->midpoint_handles[segment]);
			 
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

/* Removes very small segments on both sides of a segment */
static void
orthconn_try_remove_segments(OrthConn *orth, int segment)
{
  real len;

  if (orth->numpoints == 3)
    return; /* Cant remove any more */
  
  /* Earlier segment: */
  if (segment>0) {
    len = fabs(orth->points[segment-1].x - orth->points[segment].x) +
      fabs(orth->points[segment-1].y - orth->points[segment].y);

    if (len<0.08) {
      switch(orth->orientation[segment]) {
      case HORIZONTAL:
	orth->points[segment+1].y = orth->points[segment-1].y;
	break;
      case VERTICAL:
	orth->points[segment+1].x = orth->points[segment-1].x;
	break;
      }
      if (segment == 1) {
	orth->points[segment] = orth->points[segment-1];
	remove_segment_before(orth, segment-1);
	segment--;
      } else {
	remove_segment_before(orth, segment-1);
	remove_segment_before(orth, segment-1);
	segment -= 2;
      }
    }
  }

  if (orth->numpoints == 3)
    return; /* Cant remove any more */

  /* Later segment: */
  if (segment < (orth->numpoints-2)) {
    len = fabs(orth->points[segment+1].x - orth->points[segment+2].x) +
      fabs(orth->points[segment+1].y - orth->points[segment+2].y);

    if (len<0.08) {
      switch(orth->orientation[segment]) {
      case HORIZONTAL:
	orth->points[segment].y = orth->points[segment+2].y;
	break;
      case VERTICAL:
	orth->points[segment].x = orth->points[segment+2].x;
	break;
      }
      if (segment == (orth->numpoints-3)) {
	orth->points[segment+1] = orth->points[segment+2];
	remove_segment_after(orth, segment+1);
	segment--;
      } else {
	remove_segment_after(orth, segment);
	remove_segment_after(orth, segment);
	segment -= 2;
      }
    }
    
  }
}

void
orthconn_update_data(OrthConn *orth)
{
  int i;
  
  /* Update handles: */
  orth->endpoint_handles[0].pos = orth->points[0];
  orth->endpoint_handles[1].pos = orth->points[orth->numpoints-1];

  for (i=0;i<orth->numpoints-1;i++) {
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


/* Needs to have at least 4 handles. 
   The first 4 are used. Handles are dynamicaly moved created/deleted,
   so don't rely on them having a special index.
*/
void
orthconn_init(OrthConn *orth, Point *startpoint)
{
  Object *obj;

  obj = &orth->object;

  object_init(obj, 4, 0);
  
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
  
  orth->midpoint_handles[0] = g_new(Handle,1);
  setup_midpoint_handle(orth->midpoint_handles[0]);
  obj->handles[2] = orth->midpoint_handles[0];

  orth->midpoint_handles[1] = g_new(Handle,1);
  setup_midpoint_handle(orth->midpoint_handles[1]);
  obj->handles[3] = orth->midpoint_handles[1];

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

  for (i=0;i<to->numpoints-1;i++) {
    to->orientation[i] = from->orientation[i];
    to->midpoint_handles[i] = g_new(Handle,1);
    setup_midpoint_handle(to->midpoint_handles[i]);
    to->midpoint_handles[i]->pos = from->midpoint_handles[i]->pos;
    toobj->handles[i+2] = to->midpoint_handles[i];
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
  
  Object *obj = &orth->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "orth_points");

  if (attr != NULL)
    orth->numpoints = attribute_num_data(attr);
  else
    orth->numpoints = 0;

  object_init(obj, 2 + orth->numpoints-1, 0);

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

  for (i=0;i<orth->numpoints-1;i++) {
    orth->midpoint_handles[i] = g_new(Handle, 1);
    setup_midpoint_handle(orth->midpoint_handles[i]);
    obj->handles[i+2] = orth->midpoint_handles[i];
  }

  orthconn_update_data(orth);
}

Handle*
orthconn_get_middle_handle( OrthConn *orth )
{
  int n = orth->numpoints - 1 ;
  return orth->midpoint_handles[ n/2 ] ;
}

void
orthconn_state_get(OrthConnState *state, OrthConn *orth)
{
  state->numpoints = orth->numpoints;
  state->points = g_new(Point, state->numpoints);
  memcpy(state->points, orth->points,
	 state->numpoints*sizeof(Point));
  state->orientation = g_new(Orientation, state->numpoints-1);
  memcpy(state->orientation, orth->orientation,
	 (state->numpoints-1)*sizeof(Orientation));
}

void
orthconn_state_set(OrthConnState *state, OrthConn *orth)
{
  if (orth->points)
    g_free(orth->points);
  if (orth->orientation)
    g_free(orth->orientation);
  
  orth->numpoints = state->numpoints;
  orth->points = g_new(Point, orth->numpoints);
  memcpy(orth->points, state->points,
	 orth->numpoints*sizeof(Point));
  orth->orientation = g_new(Orientation, orth->numpoints-1);
  memcpy(orth->orientation, state->orientation,
	 (orth->numpoints-1)*sizeof(Orientation));
  
  orthconn_update_data(orth);
}

void
orthconn_state_free(OrthConnState *state)
{
  if (state->points)
    g_free(state->points);
  
  if (state->orientation)
    g_free(state->orientation);
}

