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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "beziershape.h"
#include "message.h"

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
beziershape_create_point_change(BezierShape *bezier, enum change_type type,
			BezPoint *point, BezCornerType corner_type,
			int segment,
			Handle *handle1, ConnectionPoint *connected_to1,
			Handle *handle2, ConnectionPoint *connected_to2,
			Handle *handle3, ConnectionPoint *connected_to3);
static ObjectChange *
beziershape_create_corner_change(BezierShape *bezier, Handle *handle,
			Point *point_left, Point *point_right,
			BezCornerType old_corner_type,
			BezCornerType new_corner_type);

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

#define get_comp_nr(hnum) (((int)(hnum)+2)/3)
#define get_major_nr(hnum) (((int)(hnum)+1)/3)

void
beziershape_move_handle(BezierShape *bezier, Handle *handle,
			Point *to, HandleMoveReason reason)
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
    if (comp_nr == 0) {
      bezier->points[0].p1 = bezier->points[0].p3 = *to;
      bezier->points[bezier->numpoints-1].p3 = *to;
      point_add(&bezier->points[bezier->numpoints-1].p2, &delta);
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
    switch (bezier->corner_types[comp_nr]) {
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
}

void
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
  for (i = 0; i < bezier->numpoints - 1; i++) {
    real new_dist = distance_bez_seg_point(&last, &bezier->points[i+1].p1,
			&bezier->points[i+1].p2, &bezier->points[i+1].p3,
			line_width, point);
    if (new_dist < dist) {
      dist = new_dist;
      closest = i;
    }
    last = bezier->points[i+1].p3;
  }
  return closest;
}

Handle *
beziershape_closest_handle(BezierShape *bezier, Point *point)
{
  int i, hn;
  real dist;
  Handle *closest;
  
  closest = bezier->object.handles[0];
  dist = distance_point_point( point, &closest->pos);
  for (i = 1, hn = 1; i < bezier->numpoints; i++, hn++) {
    real new_dist;

    new_dist = distance_point_point( point, &bezier->points[i].p1);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[i];
    }
    hn++;

    new_dist = distance_point_point( point, &bezier->points[i].p2);
    if (new_dist < dist) {
      dist = new_dist;
      closest = bezier->object.handles[i];
    }
    if (i < bezier->numpoints - 1) {
      hn++;
      new_dist = distance_point_point( point, &bezier->points[i].p3);
      if (new_dist < dist) {
	dist = new_dist;
	closest = bezier->object.handles[i];
      }
    }
  }
  return closest;
}

Handle *
beziershape_closest_major_handle(BezierShape *bezier, Point *point)
{
  Handle *closest = beziershape_closest_handle(bezier, point);
  int pos = get_major_nr(get_handle_nr(bezier, closest));

  if (pos == bezier->numpoints - 1)
    pos = 0;
  return bezier->object.handles[3*pos];
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
	    Handle *handle2, Handle *handle3)
{
  int i, next;
  Object *obj;

  assert(pos > 0);

  obj = (Object *)bezier;
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
  object_add_handle_at((Object*)bezier, handle1, 3*pos-2);
  object_add_handle_at((Object*)bezier, handle2, 3*pos-1);
  object_add_handle_at((Object*)bezier, handle3, 3*pos);
}

static void
remove_handles(BezierShape *bezier, int pos)
{
  int i;
  Object *obj;
  Handle *old_handle1, *old_handle2, *old_handle3;
  Point tmppoint;

  assert(pos > 0);

  obj = (Object *)bezier;

  /* delete the points */
  bezier->numpoints--;
  tmppoint = bezier->points[pos].p1;
  for (i = pos; i < bezier->numpoints - 1; i++) {
    bezier->points[i] = bezier->points[i+1];
    bezier->corner_types[i] = bezier->corner_types[i+1];
  }
  bezier->points[pos].p1 = tmppoint;
  bezier->points = g_realloc(bezier->points,
			     bezier->numpoints * sizeof(BezPoint));
  bezier->corner_types = g_realloc(bezier->corner_types,
				   bezier->numpoints * sizeof(BezCornerType));

  old_handle1 = obj->handles[3*pos-2];
  old_handle2 = obj->handles[3*pos-1];
  old_handle3 = obj->handles[3*pos];
  object_remove_handle(&bezier->object, old_handle1);
  object_remove_handle(&bezier->object, old_handle2);
  object_remove_handle(&bezier->object, old_handle3);
}


/* Add a point by splitting segment into two, putting the new point at
 'point' or, if NULL, in the middle */
ObjectChange *
beziershape_add_segment(BezierShape *bezier, int segment, Point *point)
{
  BezPoint realpoint;
  BezCornerType corner_type = BEZ_CORNER_SYMMETRIC;
  Handle *new_handle1, *new_handle2, *new_handle3;
  Point startpoint;

  startpoint = bezier->points[segment].p3;
  if (point == NULL) {
    realpoint.p1.x = (startpoint.x + bezier->points[segment+1].p3.x)/6;
    realpoint.p1.y = (startpoint.y + bezier->points[segment+1].p3.y)/6;
    realpoint.p2.x = (startpoint.x + bezier->points[segment+1].p3.x)/3;
    realpoint.p2.y = (startpoint.y + bezier->points[segment+1].p3.y)/3;
    realpoint.p3.x = (startpoint.x + bezier->points[segment+1].p3.x)/2;
    realpoint.p3.y = (startpoint.y + bezier->points[segment+1].p3.y)/2;
  } else {
    realpoint.p2.x = point->x+(startpoint.x-bezier->points[segment+1].p3.x)/6;
    realpoint.p2.y = point->y+(startpoint.y-bezier->points[segment+1].p3.y)/6;
    realpoint.p3 = *point;
    /* this really goes into the next segment ... */
    realpoint.p1.x = point->x-(startpoint.x-bezier->points[segment+1].p3.x)/6;
    realpoint.p1.y = point->y-(startpoint.y-bezier->points[segment+1].p3.y)/6;
  }
  realpoint.type = BEZ_CURVE_TO;

  new_handle1 = g_new(Handle, 1);
  new_handle2 = g_new(Handle, 1);
  new_handle3 = g_new(Handle, 1);
  setup_handle(new_handle1, HANDLE_RIGHTCTRL);
  setup_handle(new_handle2, HANDLE_LEFTCTRL);
  setup_handle(new_handle3, HANDLE_BEZMAJOR);
  add_handles(bezier, segment+1, &realpoint, corner_type,
	      new_handle1, new_handle2, new_handle3);
  return beziershape_create_point_change(bezier, TYPE_ADD_POINT,
					 &realpoint, corner_type, segment+1,
					 new_handle1, NULL,
					 new_handle2, NULL,
					 new_handle3, NULL);
}

ObjectChange *
beziershape_remove_segment(BezierShape *bezier, int pos)
{
  Handle *old_handle1, *old_handle2, *old_handle3;
  ConnectionPoint *cpt1, *cpt2, *cpt3;
  BezPoint old_point;
  BezCornerType old_ctype;

  assert(pos > 0);
  assert(bezier->numpoints > 2);

  if (pos == bezier->numpoints - 1) pos--;

  old_handle1 = bezier->object.handles[3*pos-2];
  old_handle2 = bezier->object.handles[3*pos-1];
  old_handle3 = bezier->object.handles[3*pos];
  old_point = bezier->points[pos];
  old_ctype = bezier->corner_types[pos];

  cpt1 = old_handle1->connected_to;
  cpt2 = old_handle2->connected_to;
  cpt3 = old_handle3->connected_to;
  
  object_unconnect((Object *)bezier, old_handle1);
  object_unconnect((Object *)bezier, old_handle2);
  object_unconnect((Object *)bezier, old_handle3);

  remove_handles(bezier, pos);

  beziershape_update_data(bezier);
  
  return beziershape_create_point_change(bezier, TYPE_REMOVE_POINT,
					 &old_point, old_ctype, pos,
					 old_handle1, cpt1,
					 old_handle2, cpt2,
					 old_handle3, cpt3);
}

static void
beziershape_straighten_corner(BezierShape *bez, int comp_nr) {
  int next_nr;

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
    mid_handle = bez->object.handles[handle_nr];
    break;
  case HANDLE_RIGHTCTRL:
    handle_nr--;
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

  bez->corner_types[comp_nr] = corner_type;

  beziershape_straighten_corner(bez, comp_nr);

  return beziershape_create_corner_change(bez, mid_handle, &old_left,
					  &old_right, old_type, corner_type);
}

void
beziershape_update_data(BezierShape *bezier)
{
  int i;
  
  /* Update handles: */
  bezier->object.handles[0]->pos = bezier->points[0].p1;
  bezier->points[0].p3 = bezier->points[0].p1;
  for (i = 1; i < bezier->numpoints; i++) {
    bezier->object.handles[3*i-2]->pos = bezier->points[i].p1;
    bezier->object.handles[3*i-1]->pos = bezier->points[i].p2;
    if (i < bezier->numpoints - 1)
      bezier->object.handles[3*i]->pos = bezier->points[i].p3;
  }
}

void
beziershape_update_boundingbox(BezierShape *bezier)
{
  Rectangle *bb;
  BezPoint *points;
  int i;
  
  assert(bezier != NULL);

  bb = &bezier->object.bounding_box;
  points = &bezier->points[0];

  bb->right = bb->left = points[0].p1.x;
  bb->top = bb->bottom = points[0].p1.y;
  for (i = 1; i < bezier->numpoints; i++) {
    if (points[i].p1.x < bb->left)
      bb->left = points[i].p1.x;
    if (points[i].p1.x > bb->right)
      bb->right = points[i].p1.x;
    if (points[i].p1.y < bb->top)
      bb->top = points[i].p1.y;
    if (points[i].p1.y > bb->bottom)
      bb->bottom = points[i].p1.y;
    if (points[i].type == BEZ_CURVE_TO) {
      if (points[i].p2.x < bb->left)
        bb->left = points[i].p2.x;
      if (points[i].p2.x > bb->right)
        bb->right = points[i].p2.x;
      if (points[i].p2.y < bb->top)
        bb->top = points[i].p2.y;
      if (points[i].p2.y > bb->bottom)
        bb->bottom = points[i].p2.y;
      if (points[i].p3.x < bb->left)
        bb->left = points[i].p3.x;
      if (points[i].p3.x > bb->right)
        bb->right = points[i].p3.x;
      if (points[i].p3.y < bb->top)
        bb->top = points[i].p3.y;
      if (points[i].p3.y > bb->bottom)
        bb->bottom = points[i].p3.y;
    }
  }
}

void
beziershape_simple_draw(BezierShape *bezier, Renderer *renderer, real width)
{
  BezPoint *points;
  
  assert(bezier != NULL);
  assert(renderer != NULL);

  points = &bezier->points[0];
  
  renderer->ops->set_linewidth(renderer, width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_ROUND);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->fill_bezier(renderer, points, bezier->numpoints,&color_white);
  renderer->ops->draw_bezier(renderer, points, bezier->numpoints,&color_black);
}

void
beziershape_draw_control_lines(BezierShape *bez, Renderer *renderer)
{
  Color line_colour = {0.0, 0.0, 0.6};
  Point startpoint;
  int i;
  
  /* setup renderer ... */
  renderer->ops->set_linewidth(renderer, 0);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DOTTED);
  renderer->ops->set_dashlength(renderer, 1);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  startpoint = bez->points[0].p1;
  for (i = 1; i < bez->numpoints; i++) {
    renderer->ops->draw_line(renderer, &startpoint, &bez->points[i].p1,
                             &line_colour);
    renderer->ops->draw_line(renderer, &bez->points[i].p2, &bez->points[i].p3,
                             &line_colour);
    startpoint = bez->points[i].p3;
  }
}

void
beziershape_init(BezierShape *bezier)
{
  Object *obj;

  obj = &bezier->object;

  object_init(obj, 6, 0);
  
  bezier->numpoints = 3;

  bezier->points = g_new(BezPoint, 3);
  bezier->points[0].type = BEZ_MOVE_TO;
  bezier->points[1].type = BEZ_CURVE_TO;
  bezier->points[2].type = BEZ_CURVE_TO;

  bezier->corner_types = g_new(BezCornerType, 3);
  bezier->corner_types[0] = BEZ_CORNER_SYMMETRIC;
  bezier->corner_types[1] = BEZ_CORNER_SYMMETRIC;
  bezier->corner_types[2] = BEZ_CORNER_SYMMETRIC;

  bezier->object.handles[0] = g_new(Handle, 1);
  bezier->object.handles[1] = g_new(Handle, 1);
  bezier->object.handles[2] = g_new(Handle, 1);
  bezier->object.handles[3] = g_new(Handle, 1);
  bezier->object.handles[4] = g_new(Handle, 1);
  bezier->object.handles[5] = g_new(Handle, 1);

  obj->handles[0]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[0]->connected_to = NULL;
  obj->handles[0]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[0]->id = HANDLE_CORNER;
  
  obj->handles[1]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[1]->connected_to = NULL;
  obj->handles[1]->type = HANDLE_MINOR_CONTROL;
  obj->handles[1]->id = HANDLE_RIGHTCTRL;

  obj->handles[2]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[2]->connected_to = NULL;
  obj->handles[2]->type = HANDLE_MINOR_CONTROL;
  obj->handles[2]->id = HANDLE_LEFTCTRL;

  obj->handles[3]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[3]->connected_to = NULL;
  obj->handles[3]->type = HANDLE_MAJOR_CONTROL;
  obj->handles[3]->id = HANDLE_CORNER;
  
  obj->handles[4]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[4]->connected_to = NULL;
  obj->handles[4]->type = HANDLE_MINOR_CONTROL;
  obj->handles[4]->id = HANDLE_RIGHTCTRL;

  obj->handles[5]->connect_type = HANDLE_NONCONNECTABLE;
  obj->handles[5]->connected_to = NULL;
  obj->handles[5]->type = HANDLE_MINOR_CONTROL;
  obj->handles[5]->id = HANDLE_LEFTCTRL;

  beziershape_update_data(bezier);
}

void
beziershape_copy(BezierShape *from, BezierShape *to)
{
  int i;
  Object *toobj, *fromobj;

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

  for (i = 0; i < to->object.num_handles; i++) {
    to->object.handles[i] = g_new(Handle, 1);
    setup_handle(to->object.handles[i], from->object.handles[i]->id);
  }

  beziershape_update_data(to);
}

void
beziershape_destroy(BezierShape *bezier)
{
  int i;
  Handle **temp_handles;

  /* Need to store these temporary since object.handles is
     freed by object_destroy() */
  temp_handles = g_new(Handle *, bezier->object.num_handles);
  for (i = 0; i < bezier->object.num_handles;i++)
    temp_handles[i] = bezier->object.handles[i];

  object_destroy(&bezier->object);

  for (i=0;i<bezier->numpoints;i++)
    g_free(temp_handles[i]);
  g_free(temp_handles);
  
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
  for (i = 0; i < bezier->numpoints; i++) {
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
  
  Object *obj = &bezier->object;

  object_load(obj, obj_node);

  attr = object_find_attribute(obj_node, "bez_points");

  if (attr != NULL)
    bezier->numpoints = attribute_num_data(attr) / 3 + 1;
  else
    bezier->numpoints = 0;

  object_init(obj, 3 * (bezier->numpoints - 1), 0);

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
    obj->handles[3*i-2] = g_new(Handle, 1);
    setup_handle(obj->handles[3*i],   HANDLE_BEZMAJOR);
    obj->handles[3*i-1] = g_new(Handle, 1);
    setup_handle(obj->handles[3*i+1], HANDLE_RIGHTCTRL);
    obj->handles[3*i]   = g_new(Handle, 1);
    setup_handle(obj->handles[3*i+2], HANDLE_LEFTCTRL);
  }

  beziershape_update_data(bezier);
}

static void
beziershape_point_change_free(struct PointChange *change)
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

static void
beziershape_point_change_apply(struct PointChange *change, Object *obj)
{
  change->applied = 1;
  switch (change->type) {
  case TYPE_ADD_POINT:
    add_handles((BezierShape *)obj, change->pos, &change->point,
		change->corner_type,
		change->handle1, change->handle2, change->handle3);
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
beziershape_point_change_revert(struct PointChange *change, Object *obj)
{
  switch (change->type) {
  case TYPE_ADD_POINT:
    remove_handles((BezierShape *)obj, change->pos);
    break;
  case TYPE_REMOVE_POINT:
    add_handles((BezierShape *)obj, change->pos, &change->point,
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

static ObjectChange *
beziershape_create_point_change(BezierShape *bezier, enum change_type type,
				BezPoint *point, BezCornerType corner_type,
				int pos,
				Handle *handle1,ConnectionPoint *connected_to1,
				Handle *handle2,ConnectionPoint *connected_to2,
				Handle *handle3,ConnectionPoint *connected_to3)
{
  struct PointChange *change;

  change = g_new(struct PointChange, 1);

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
  change->connected_to1 = connected_to1;
  change->handle2 = handle2;
  change->connected_to2 = connected_to2;
  change->handle3 = handle3;
  change->connected_to3 = connected_to3;

  return (ObjectChange *)change;
}

static void
beziershape_corner_change_apply(struct CornerChange *change, Object *obj)
{
  BezierShape *bez = (BezierShape *)obj;
  int handle_nr = get_handle_nr(bez, change->handle);
  int comp_nr = get_major_nr(handle_nr);

  beziershape_straighten_corner(bez, comp_nr);

  bez->corner_types[comp_nr] = change->new_type;

  change->applied = 1;
}

static void
beziershape_corner_change_revert(struct CornerChange *change, Object *obj)
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
