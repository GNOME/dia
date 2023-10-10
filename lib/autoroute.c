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

/*!
 * \defgroup Autorouting Built-in Autorouting
 * \brief Simple autorouting for \ref _OrthConn "orthogonal lines"
 * \ingroup ObjectParts
 */

#include "config.h"

#include <glib/gi18n-lib.h>

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

static real autoroute_layout_parallel(Point *to,
					guint *num_points, Point **points);
static real autoroute_layout_orthogonal(Point *to,
					  int enddir,
					  guint *num_points, Point **points);
static real autoroute_layout_opposite(Point *to,
					guint *num_points, Point **points);
static Point autolayout_adjust_for_gap(Point *pos, int dir, ConnectionPoint *cp);
static void autolayout_adjust_for_arrow(Point *pos, int dir, real adjust);
static guint autolayout_normalize_points(guint startdir, guint enddir,
					 Point start, Point end,
					 Point *newend);
static Point *autolayout_unnormalize_points(guint dir,
					    Point start,
					    Point *points,
					    guint num_points);

static int
autolayout_calc_intersects (const DiaRectangle *r1, const DiaRectangle *r2,
			    const Point *points, guint num_points)
{
  guint i, n = 0;

  /* ignoring the first and last line assuming a proper 'outer' algorithm */
  for (i = 1; i < num_points - 2; ++i) {
    DiaRectangle rt = { points[i].x, points[i].y, points[i+1].x, points[i+1].y };
    if (r1)
      n += (rectangle_intersects (r1, &rt) ? 1 : 0);
    if (r2)
      n += (rectangle_intersects (r2, &rt) ? 1 : 0);
  }

  return n;
}
/*!
 * \brief Apply a good route (or none) to the given _OrthConn
 *
 * Calculate a 'pleasing' route between two connection points.
 * If a good route is found, updates the given OrthConn with the values
 *   and returns TRUE.
 * Otherwise, the OrthConn is untouched, and the function returns FALSE.
 * Handles are not updated by this operation.
 * @param conn The orthconn object to autoroute for.
 * @param startconn The connectionpoint at the start of the orthconn,
 *                  or null if it is not connected there at the moment.
 * @param endconn The connectionpoint at the end (target) of the orthconn,
 *                or null if it is not connected there at the moment.
 * @return TRUE if the orthconn could be laid out reasonably, FALSE otherwise.
 *
 * \ingroup Autorouting
 *
 * \callgraph
 */
gboolean
autoroute_layout_orthconn(OrthConn *conn,
			  ConnectionPoint *startconn, ConnectionPoint *endconn)
{
  real min_badness = MAX_BADNESS;
  guint best_intersects = G_MAXINT;
  guint intersects;
  Point *best_layout = NULL;
  guint best_num_points = 0;
  int startdir, enddir;

  int fromdir, todir;
  Point frompos, topos;

  frompos = conn->points[0];
  topos = conn->points[conn->numpoints-1];
  if (startconn != NULL) {
    fromdir = startconn->directions;
    frompos = startconn->pos;
  }
  else fromdir = DIR_NORTH|DIR_EAST|DIR_SOUTH|DIR_WEST;
  if (endconn != NULL) {
    todir = endconn->directions;
    topos = endconn->pos;
  }
  else todir = DIR_NORTH|DIR_EAST|DIR_SOUTH|DIR_WEST;

  for (startdir = DIR_NORTH; startdir <= DIR_WEST; startdir *= 2) {
    for (enddir = DIR_NORTH; enddir <= DIR_WEST; enddir *= 2) {
      if ((fromdir & startdir) &&
	  (todir & enddir)) {
	real this_badness;
	Point *this_layout = NULL;
	guint this_num_points;
	guint normal_enddir;
	Point startpoint, endpoint;
	Point otherpoint;
	startpoint = autolayout_adjust_for_gap(&frompos, startdir, startconn);
	autolayout_adjust_for_arrow(&startpoint, startdir, conn->extra_spacing.start_trans);
	endpoint = autolayout_adjust_for_gap(&topos, enddir, endconn);
	autolayout_adjust_for_arrow(&endpoint, enddir, conn->extra_spacing.end_trans);
	/*
	printf("Startdir %d enddir %d orgstart %.2f, %.2f orgend %.2f, %.2f start %.2f, %.2f end %.2f, %.2f\n",
	       startdir, enddir,
	       frompos.x, frompos.y,
	       topos.x, topos.y,
	       startpoint.x, startpoint.y,
	       endpoint.x, endpoint.y);
	*/
	normal_enddir = autolayout_normalize_points(startdir, enddir,
						    startpoint, endpoint,
						    &otherpoint);
	if (normal_enddir == DIR_NORTH ) {
	  this_badness = autoroute_layout_parallel(&otherpoint,
						   &this_num_points,
						   &this_layout);
	} else if (normal_enddir == DIR_SOUTH) {
	  this_badness = autoroute_layout_opposite(&otherpoint,
						   &this_num_points,
						   &this_layout);
	} else {
	  this_badness = autoroute_layout_orthogonal(&otherpoint,
						     normal_enddir,
						     &this_num_points,
						     &this_layout);
	}
	if (this_layout != NULL) {
	  /* this_layout is eaten by unnormalize */
	  Point *unnormalized = autolayout_unnormalize_points(startdir, startpoint,
							       this_layout, this_num_points);

	  intersects = autolayout_calc_intersects (
	    startconn ? dia_object_get_bounding_box (startconn->object) : NULL,
	    endconn ? dia_object_get_bounding_box (endconn->object) : NULL,
	    unnormalized, this_num_points);

	  if (   intersects <= best_intersects
	      && this_badness-min_badness < -0.00001) {
	    /*
	    printf("Dir %d to %d badness %f < %f\n", startdir, enddir,
		   this_badness, min_badness);
	    */
	    min_badness = this_badness;
	    g_clear_pointer (&best_layout, g_free);
	    best_layout = unnormalized;
	    best_num_points = this_num_points;
            /* revert adjusting start and end point */
	    autolayout_adjust_for_arrow(&best_layout[0], startdir,
	                                -conn->extra_spacing.start_trans);
	    autolayout_adjust_for_arrow(&best_layout[best_num_points-1], enddir,
	                                -conn->extra_spacing.end_trans);
	    best_intersects = intersects;
	  } else {
	    g_clear_pointer (&unnormalized, g_free);
	  }
	}
      }
    }
  }

  if (min_badness < MAX_BADNESS) {
    orthconn_set_points(conn, best_num_points, best_layout);
    g_clear_pointer (&best_layout, g_free);
    return TRUE;
  } else {
    g_clear_pointer (&best_layout, g_free);
    return FALSE;
  }
}

/*!
 * Returns the basic badness of a length.  The best length is MIN_DIST,
 * anything shorter quickly becomes messy, longer segments are linearly worse.
 * @param len The length of an orthconn segment.
 * @return How bad this segment would be to have in the autorouting.
 *
 * \ingroup Autorouting
 */
static real
length_badness(real len)
{
  if (len < MIN_DIST) {
    /* This should be zero at MIN_DIST and MAX_SMALL_BADNESS at 0 */
    return 2*MAX_SMALL_BADNESS/(1.0+len/MIN_DIST) - MAX_SMALL_BADNESS;
  } else {
    return len-MIN_DIST;
  }
}

/*!
 * \brief Calculate badness of a point array
 *
 * Returns the accumulated badness of a layout.  At the moment, this is
 * calculated as the sum of the badnesses of the segments plus a badness for
 * each bend in the line.
 * @param ps An array of points.
 * @param num_points How many points in the array.
 * @return How bad the points would look as an orthconn layout.
 *
 * \ingroup Autorouting
 */
static real
calculate_badness(Point *ps, guint num_points)
{
  real badness;
  guint i;
  badness = (num_points-1)*EXTRA_SEGMENT_BADNESS;
  for (i = 0; i < num_points-1; i++) {
    real this_badness;
    real len = distance_point_point_manhattan(&ps[i], &ps[i+1]);
    this_badness = length_badness(len);
    badness += this_badness;
  }
  return badness;
}

/*!
 * \brief Gap adjustment for points and a connection point
 *
 * Adjust one end of an orthconn for gaps, if autogap is on for the connpoint.
 * @param pos Point of the end of the line.
 * @param dir Which of the four cardinal directions the line goes from pos.
 * @param cp The connectionpoint the line is connected to.
 * @return Where the line should end to be on the correct edge of the
 *          object, if cp has autogap on.
 *
 * \ingroup Autorouting
 */
static Point
autolayout_adjust_for_gap(Point *pos, int dir, ConnectionPoint *cp)
{
  DiaObject *object;
  Point dir_other;
  /* Do absolute gaps here, once it's defined */

  if (!cp || !connpoint_is_autogap(cp)) {
    return *pos;
  }

  object  = cp->object;

  dir_other.x = pos->x;
  dir_other.y = pos->y;
  switch (dir) {
  case DIR_NORTH:
    dir_other.y += 2 * (object->bounding_box.top - pos->y);
    break;
  case DIR_SOUTH:
    dir_other.y += 2 * (object->bounding_box.bottom - pos->y);
    break;
  case DIR_EAST:
    dir_other.x += 2 * (object->bounding_box.right - pos->x);
    break;
  case DIR_WEST:
    dir_other.x += 2 * (object->bounding_box.left - pos->x);
    break;
  default:
    g_warning("Impossible direction %d\n", dir);
  }
  return calculate_object_edge(pos, &dir_other, object);
}


/**
 * autolayout_adjust_for_arrow
 *
 * Adjust the original position to move away from a potential arrow
 *
 * We could do some similar by making MIN_DIST depend on the arrow size,
 * but this one is much more easy, not touchun the autolayout algorithm at
 * all. Needs to be called twice - second time with negative adjust - to
 * move the point back to where it was.
 */
static void
autolayout_adjust_for_arrow (Point *pos, int dir, real adjust)
{
  switch (dir) {
    case DIR_NORTH:
      pos->y -= adjust;
      break;
    case DIR_EAST:
      pos->x += adjust;
      break;
    case DIR_SOUTH:
      pos->y += adjust;
      break;
    case DIR_WEST:
      pos->x -= adjust;
      break;
    default:
      g_return_if_reached ();
  }
}


/*!
 * \brief Parallel layout
 *
 * Lay out autorouting where start and end lines are parallel pointing the
 * same direction.  This can either a simple up-right-down layout, or if the
 * to point is too close to origo, it will go up-right-down-left-down.
 * @param to Where to lay out to, coming from origo.
 * @param num_points Return value of how many points in the points array.
 * @param points The points in the layout.  Free the array after use.  The
 *               passed in is ignored and overwritten, so should be NULL.
 * @return The badness of this layout.
 *
 * \ingroup Autorouting
 */
static real
autoroute_layout_parallel(Point *to, guint *num_points, Point **points)
{
  Point *ps = NULL;
  if (fabs(to->x) > MIN_DIST) {
    real top = MIN(-MIN_DIST, to->y-MIN_DIST);
    /*
    printf("Doing parallel layout: Wide\n");
    */
    *num_points = 4;
    ps = g_new0(Point, *num_points);
    /* points[0] is 0,0 */
    ps[1].x = to->x/2;
    ps[1].y = top;
    ps[2].x = to->x/2;
    ps[2].y = top;
    ps[3] = *to;
  } else if (to->y > 0) { /* Close together, end below */
    real top = -MIN_DIST;
    real off = to->x+MIN_DIST*(to->x>0?1.0:-1.0);
    real bottom = to->y-MIN_DIST;
    /*
    printf("Doing parallel layout: Narrow\n");
    */
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
    /*
    printf("Doing parallel layout: Narrow\n");
    */
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
  *points = ps;
  return calculate_badness(ps, *num_points);
}

/*!
 * \brief Orthogonal layout
 *
 * Do layout for the case where the directions are orthogonal to each other.
 * If both x and y of to are far enough from origo, this will be a simple
 * bend, otherwise it will be a question-mark style line.
 * @param to Where to lay out to, coming from origo.
 * @param enddir What direction the endpoint goes, either east or west.
 * @param num_points Return value of how many points in the points array.
 * @param points The points in the layout.  Free the array after use.  The
 *               passed in is ignored and overwritten, so should be NULL.
 * @return The badness of this layout.
 *
 * \ingroup Autorouting
 */
static real
autoroute_layout_orthogonal(Point *to, int enddir,
			    guint *num_points, Point **points)
{
  /* This one doesn't consider enddir yet, not more complex layouts. */
  Point *ps = NULL;
  real dirmult = (enddir==DIR_WEST?1.0:-1.0);
  if (to->y < -MIN_DIST) {
    if (dirmult*to->x > MIN_DIST) {
      /*
      printf("Doing orthogonal layout: Three-way\n");
      */
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
  /*
  printf("Doing orthogonal layout\n");
  */
  *points = ps;
  return calculate_badness(ps, *num_points);
}

/*!
 * \brief Opposite layout
 *
 * Do layout for the case where the end directions are opposite.
 * This can be either a straight line, a zig-zag, a rotated s-shape or
 * a spiral.
 * @param to Where to lay out to, coming from origo.
 * @param num_points Return value of how many points in the points array.
 * @param points The points in the layout.  Free the array after use.  The
 *               passed in is ignored and overwritten, so should be NULL.
 * @return The badness of this layout.
 *
 * \ingroup Autorouting
 */
static real
autoroute_layout_opposite(Point *to, guint *num_points, Point **points)
{
  Point *ps = NULL;
  if (to->y < -MIN_DIST) {
    *num_points = 4;
    ps = g_new0(Point, *num_points);
    if (fabs(to->x) < 0.00000001) {
      ps[2] = ps[3] = *to;
      /* distribute y */
      ps[1].y = ps[2].y = to->y / 2;
      *points = ps;
      return length_badness(fabs(to->y))+2*EXTRA_SEGMENT_BADNESS;
    } else {
      real mid = to->y/2;
      /*
	printf("Doing opposite layout: Three-way\n");
      */
      /* points[0] is 0,0 */
      ps[1].y = mid;
      ps[2].x = to->x;
      ps[2].y = mid;
      ps[3] = *to;
      *points = ps;
      return 2*length_badness(fabs(mid))+2*EXTRA_SEGMENT_BADNESS;
    }
  } else if (fabs(to->x) > 2*MIN_DIST) {
    real mid = to->x/2;
    /*
    printf("Doing opposite layout: Doorhanger\n");
    */
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
    /*
    printf("Doing opposite layout: Overlap\n");
    */
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
  *points = ps;
  return calculate_badness(ps, *num_points);
}

/*!
 * \brief Rotate a point clockwise.
 * @param p The point to rotate.
 * \ingroup Autorouting
 */
static void
point_rotate_cw(Point *p)
{
  real tmp = p->x;
  p->x = -p->y;
  p->y = tmp;
}

/*!
 * \brief Rotate a point counterclockwise.
 * @param p The point to rotate.
 * \ingroup Autorouting
 */
static void
point_rotate_ccw(Point *p)
{
  real tmp = p->x;
  p->x = p->y;
  p->y = -tmp;
}

/*!
 * \brief Rotate a point 180 degrees.
 * @param p The point to rotate.
 * \ingroup Autorouting
 */
static void
point_rotate_180(Point *p)
{
  p->x = -p->x;
  p->y = -p->y;
}

/*!
 * \brief Autolayout normalization
 *
 * Normalizes the directions and points to make startdir be north and
 * the starting point be 0,0.
 * @param startdir The original startdir.
 * @param enddir The original enddir.
 * @param start The original start point.
 * @param end The original end point.
 * @param newend Return address for the normalized end point.
 * @return The normalized end direction.
 *
 * \ingroup Autorouting
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

/*!
 * \brief Reverse normalization
 *
 * Reverses the normalizing process of autolayout_normalize_points by
 * moving and rotating the points to start at `start' with the start direction
 * `startdir', instead of from origo going north.
 * Returns the new array of points, freeing the old one if necessary.
 * @param startdir The direction to use as a starting direction.
 * @param start The point to start at.
 * @param points A set of points laid out from origo northbound.  This array
 *               will be freed by calling this function.
 * @param num_points The number of points in the `points' array.
 * @return A newly allocated array of points starting at `start'.
 *
 * \ingroup Autorouting
 */
static Point *
autolayout_unnormalize_points(guint startdir,
			      Point start,
			      Point *points,
			      guint num_points)
{
  Point *newpoints = g_new(Point, num_points);
  guint i;
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
  g_clear_pointer (&points, g_free);
  return newpoints;
}
