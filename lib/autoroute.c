/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 2002 Alexander Larsson
 *
 * autoroute.c -- Automatic layout of connections.
 * Copyright (C) 2003 Lars Clausen
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

#include "object.h"
#include "connectionpoint.h"
#include "orth_conn.h"
#include "autoroute.h"

#define MAX_BADNESS 10000.0
/** Add badness if a line is shorter than this distance. */
#define MIN_DIST 1.0
/** The maximum badness that can be given for a line being too short. */
#define MAX_SMALL_BADNESS 10.0
/** The badness given for having extra segments. */
#define EXTRA_SEGMENT_BADNESS 10.0

static real calculate_badness(Point *ps, guint num_points);

static Point* autoroute_layout_parallel(Point *to, guint *num_points);
static Point* autoroute_layout_orthogonal(Point *to,
					int enddir, guint *num_points);
static Point* autoroute_layout_opposite(Point *to, guint *num_points);

static guint autolayout_normalize_points(guint startdir, guint enddir,
					 Point start, Point end,
					 Point *newend);
static Point *autolayout_unnormalize_points(guint dir,
					    Point start,
					    Point *points,
					    guint num_points);

/* Calculate a 'pleasing' route between two connection points.
 * If a good route is found, updates the given OrthConn with the values
 *   and returns TRUE.
 * Otherwise, the OrthConn is untouched, and the function returns FALSE.
 * Handles are not updated by this operation.
 */
gboolean
autoroute_layout_orthconn(OrthConn *conn)
{
  real min_badness = MAX_BADNESS;
  Point *best_layout = NULL;
  guint best_num_points = 0;
  int startdir, enddir;
  Object *obj = (Object*)conn;
  ConnectionPoint *from = obj->handles[0]->connected_to,
    *to = obj->handles[1]->connected_to;
  /* Don't do unconnected layout yet */
  if (from == NULL || to == NULL) {
    return FALSE;
  }

  for (startdir = DIR_NORTH; startdir <= DIR_WEST; startdir *= 2) {
    for (enddir = DIR_NORTH; enddir <= DIR_WEST; enddir *= 2) {
      if ((from->directions & startdir) &&
	  (to->directions & enddir)) {
	real this_badness;
	Point *this_layout = NULL;
	guint this_num_points;
	guint normal_enddir;
	Point endpoint;
	normal_enddir = autolayout_normalize_points(startdir, enddir,
						    from->pos, to->pos,
						    &endpoint);
	if (normal_enddir == DIR_NORTH ) {
	  this_layout = autoroute_layout_parallel(&endpoint,
						  &this_num_points);
	} else if (normal_enddir == DIR_SOUTH) {
	  this_layout = autoroute_layout_opposite(&endpoint, 
						  &this_num_points);
	} else {
	  this_layout = autoroute_layout_orthogonal(&endpoint,
						    normal_enddir,
						    &this_num_points);
	}
	if (this_layout != NULL) {
	  this_badness = calculate_badness(this_layout, this_num_points);
	  if (this_badness-min_badness < -0.00001) {
	    printf("Dir %d to %d badness %f < %f\n", startdir, enddir,
		   this_badness, min_badness);
	    min_badness = this_badness;
	    if (best_layout != NULL) g_free(best_layout);
	    best_layout = autolayout_unnormalize_points(startdir, from->pos,
							this_layout, 
							this_num_points);
	    best_num_points = this_num_points;
	  } else {
	      g_free(this_layout);
	  }
	}
      }
    }    
  }
  
  if (min_badness < MAX_BADNESS) {
    orthconn_set_points(conn, best_num_points, best_layout);
    return TRUE;
  } else {
    g_free(best_layout);
    return FALSE;
  }
}

static real
calculate_badness(Point *ps, guint num_points)
{
  real badness;
  int i;
  badness = (num_points-1)*EXTRA_SEGMENT_BADNESS;
  for (i = 0; i < num_points-1; i++) {
    real this_badness;
    real len = distance_point_point_manhattan(&ps[i], &ps[i+1]);
    if (len < MIN_DIST) {
      /* This should be zero at MIN_DIST and MAX_SMALL_BADNESS at 0 */
      this_badness = 2*MAX_SMALL_BADNESS/(1.0+len/MIN_DIST)
	-MAX_SMALL_BADNESS;
      /* Special case:  Small kinks are ok in the middle. */
      if (i > 0 && i < num_points-2) {
	/* However, tight turns aren't */
	if (fabs(ps[i].x - ps[i+1].x) > 0.0000001) {
	  /* Vertical test */
	  if ((ps[i].y-ps[i-1].y)*(ps[i+1].y-ps[i+2].y) < 0)
	    this_badness /= 4;
	} else {
	  if ((ps[i].x-ps[i-1].x)*(ps[i+1].x-ps[i+2].x) < 0)
	    this_badness /= 4;
	}
      }
    } else {
      this_badness = len-MIN_DIST;
    }

    badness += this_badness;
  }
  return badness;
}

static Point *
autoroute_layout_parallel(Point *to, guint *num_points)
{
  Point *ps = NULL;
  if (fabs(to->x) > MIN_DIST) {
    real top = MIN(-MIN_DIST, to->y-MIN_DIST);
    printf("Doing parallel layout: Wide\n");
    *num_points = 4;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].y = top;
    ps[2].x = to->x;
    ps[2].y = top;
    ps[3] = *to;
  } else if (to->y > 0) { /* Close together, end below */
    real top = -MIN_DIST;
    real off = to->x+MIN_DIST*(to->x>0?1.0:-1.0);
    real bottom = to->y-MIN_DIST;
    printf("Doing parallel layout: Narrow\n");
    *num_points = 6;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].y = top;
    ps[2].x = off;
    ps[2].y = top;
    ps[3].x = off;
    ps[3].y = bottom;
    ps[4].x = to->x;
    ps[4].y = bottom;
    ps[5] = *to;
  } else {
    real top = to->y-MIN_DIST;
    real off = MIN_DIST*(to->x>0?-1.0:1.0);
    real bottom = -MIN_DIST;
    printf("Doing parallel layout: Narrow\n");
    *num_points = 6;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].y = bottom;
    ps[2].x = off;
    ps[2].y = bottom;
    ps[3].x = off;
    ps[3].y = top;
    ps[4].x = to->x;
    ps[4].y = top;
    ps[5] = *to;
  }
  return ps;
}

static Point *
autoroute_layout_orthogonal(Point *to, int enddir, guint *num_points)
{
  /* This one doesn't consider enddir yet, not more complex layouts. */
  Point *ps = NULL;
  real dirmult = (enddir==DIR_WEST?1.0:-1.0);
  if (to->y < -MIN_DIST) {
    if (dirmult*to->x > MIN_DIST) {
      printf("Doing orthogonal layout: Three-way\n");
      *num_points = 3;
      ps = g_new0(Point, *num_points);
      /* points[0] is 0,0 */
      ps[1].y = to->y;
      ps[2] = *to;
    } else {
      real off;
      if (dirmult*to->x > 0) off = -dirmult*MIN_DIST;
      else off = -dirmult*(MIN_DIST+fabs(to->x));
      *num_points = 5;
      ps = g_new0(Point, *num_points);
      ps[1].y = -MIN_DIST;
      ps[2].x = off;
      ps[2].y = -MIN_DIST;
      ps[3].x = off;
      ps[3].y = to->y;
      ps[4] = *to;
    }
  } else {
    if (dirmult*to->x > 2*MIN_DIST) {
      real mid = to->x/2;
      *num_points = 5;
      ps = g_new0(Point, *num_points);
      ps[1].y = -MIN_DIST;
      ps[2].x = mid;
      ps[2].y = -MIN_DIST;
      ps[3].x = mid;
      ps[3].y = to->y;
      ps[4] = *to;
    } else {
      real off;
      if (dirmult*to->x > 0) off = -dirmult*MIN_DIST;
      else off = -dirmult*(MIN_DIST+fabs(to->x));
      *num_points = 5;
      ps = g_new0(Point, *num_points);
      ps[1].y = -MIN_DIST;
      ps[2].x = off;
      ps[2].y = -MIN_DIST;
      ps[3].x = off;
      ps[3].y = to->y;
      ps[4] = *to;
    }
  }
  printf("Doing orthogonal layout\n");
  
  return ps;
}

static Point *
autoroute_layout_opposite(Point *to,guint *num_points)
{
  Point *ps = NULL;
  if (to->y < -MIN_DIST) {
    /* If we could have a two-point zig-zag line, it'd go here. */
    real mid = to->y/2;
    printf("Doing opposite layout: Three-way\n");
    *num_points = 4;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].y = mid;
    ps[2].x = to->x;
    ps[2].y = mid;
    ps[3] = *to;
  } else if (fabs(to->x) > 2*MIN_DIST) {
    real mid = to->x/2;
    printf("Doing opposite layout: Doorhanger\n");
    *num_points = 6;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].y = -MIN_DIST;
    ps[2].x = mid;
    ps[2].y = -MIN_DIST;
    ps[3].x = mid;
    ps[3].y = to->y+MIN_DIST;
    ps[4].x = to->x;
    ps[4].y = to->y+MIN_DIST;
    ps[5] = *to;
  } else {
    real off = MIN_DIST*(to->x>0?-1.0:1.0);
    printf("Doing opposite layout: Overlap\n");
    *num_points = 6;
    ps = g_new0(Point, *num_points);
    ps[1].y = -MIN_DIST;
    ps[2].x = off;
    ps[2].y = -MIN_DIST;
    ps[3].x = off;
    ps[3].y = to->y+MIN_DIST;
    ps[4].x = to->x;
    ps[4].y = to->y+MIN_DIST;
    ps[5] = *to;
  }
  return ps;
}

static void
point_rotate_cw(Point *p) {
  real tmp = p->x;
  p->x = -p->y;
  p->y = tmp;
}

static void
point_rotate_ccw(Point *p) {
  real tmp = p->x;
  p->x = p->y;
  p->y = -tmp;
}

static void
point_rotate_180(Point *p) {
  p->x = -p->x;
  p->y = -p->y;
}

/** Normalizes the directions and points to make startdir be north and
 * the starting point be 0,0.
 * Normalized points are put in newstart and newend.
 * Returns the new enddir.
 */
static guint
autolayout_normalize_points(guint startdir, guint enddir,
			    Point start, Point end, Point *newend)
{
  newend->x = end.x-start.x;
  newend->y = end.y-start.y;
  if (startdir == DIR_NORTH) {
    return enddir;
  } else if (startdir == DIR_EAST) {
    point_rotate_ccw(newend);
    if (enddir == DIR_NORTH) return DIR_WEST;
    return enddir/2;
  } else if (startdir == DIR_WEST) {
    point_rotate_cw(newend);
    if (enddir == DIR_WEST) return DIR_NORTH;
    return enddir*2;
  } else { /* startdir == DIR_SOUTH */
    point_rotate_180(newend);
    if (enddir < DIR_SOUTH) return enddir*4;
    else return enddir/4;
  }
  /* Insert handling of other stuff here */
  return enddir;
}

/** Reverses the normalizing process of autolayout_normalize_points.
 * Returns the new array of points, freeing the old one if necessary.
 */
static Point *
autolayout_unnormalize_points(guint startdir,
			      Point start,
			      Point *points,
			      guint num_points)
{
  Point *newpoints = g_new(Point, num_points);
  int i;
  if (startdir == DIR_NORTH) {
    for (i = 0; i < num_points; i++) {
      newpoints[i] = points[i];
      point_add(&newpoints[i], &start);
    }
  } else if (startdir == DIR_WEST) {
    for (i = 0; i < num_points; i++) {
      newpoints[i] = points[i];
      point_rotate_ccw(&newpoints[i]);
      point_add(&newpoints[i], &start);
    }
  } else if (startdir == DIR_SOUTH) {
    for (i = 0; i < num_points; i++) {
      newpoints[i] = points[i];
      point_rotate_180(&newpoints[i]);
      point_add(&newpoints[i], &start);
    }
  } else if (startdir == DIR_EAST) {
    for (i = 0; i < num_points; i++) {
      newpoints[i] = points[i];
      point_rotate_cw(&newpoints[i]);
      point_add(&newpoints[i], &start);
    }
  }
  g_free(points);
  return newpoints;
}
