/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * BezierConn  Copyright (C) 1999 James Henstridge
 * BezierShape Copyright (C) 2000 James Henstridge
 *
 * Major restructuring to share more code
 * Copyright (C) 2012 Hans Breuer
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

#include "bezier-common.h"
#include "diarenderer.h"

/*!
 * \brief Calculate BezCornerType just from the _BezPoint
 *
 * The bezier line/shape is fully described just with the array of BezPoint.
 * For convenience and editing there also is an BezierConn::corner_types.
 * This function adjust the corner types in the given array to match
 * the bezier points.
 */
static void
bezier_calc_corner_types (BezierCommon *bezier)
{
  int i;
  int num = bezier->num_points;
  const real tolerance = 0.00001; /* EPSILON */

  g_return_if_fail (bezier->num_points > 1);

  bezier->corner_types = g_realloc (bezier->corner_types, bezier->num_points * sizeof(BezCornerType));
  bezier->corner_types[0] = BEZ_CORNER_CUSP;
  bezier->corner_types[num-1] = BEZ_CORNER_CUSP;
  
  for (i = 0; i < num - 2; ++i) {
    const Point *start = &bezier->points[i].p2;
    const Point *major = &bezier->points[i].p3;
    const Point *end   = &bezier->points[i+1].p2;

    if (bezier->points[i+1].type != BEZ_CURVE_TO)
      bezier->corner_types[i+1] = BEZ_CORNER_CUSP;
    else if (distance_line_point (start, end, 0, major) > tolerance)
      bezier->corner_types[i+1] = BEZ_CORNER_CUSP;
    else if (fabs (   distance_point_point (start, major) 
		   -  distance_point_point (end, major) > tolerance))
      bezier->corner_types[i+1] = BEZ_CORNER_SMOOTH;
    else
      bezier->corner_types[i+1] = BEZ_CORNER_SYMMETRIC;
  }
}

/** Set a bezier to use the given array of points.
 * This function does *not* set up handles
 * @param bezier A bezier to operate on
 * @param num_points The number of points in the `points' array.
 * @param points The new points that this bezier should be set to use.
 */
void
beziercommon_set_points (BezierCommon *bezier, int num_points, const BezPoint *points)
{
  int i;

  bezier->num_points = num_points;

  if (bezier->points)
    g_free(bezier->points);

  bezier->points = g_malloc((bezier->num_points)*sizeof(BezPoint));

  for (i=0;i<bezier->num_points;i++) {
    bezier->points[i] = points[i];
  }

  /* adjust our corner_types to what is possible with the points */
  bezier_calc_corner_types (bezier);
}

void
beziercommon_copy (BezierCommon *from, BezierCommon *to)
{
  int i;

  to->num_points = from->num_points;

  to->points = g_new(BezPoint, to->num_points);
  to->corner_types = g_new(BezCornerType, to->num_points);

  for (i = 0; i < to->num_points; i++) {
    to->points[i] = from->points[i];
    to->corner_types[i] = from->corner_types[i];
  }
}

/*!
 * \brief Return the segment of the bezier closest to a given point.
 * @param bezier The bezier object
 * @param point A point to find the closest segment to.
 * @param line_width Line width of the bezier line.
 * @return The index of the segment closest to point.
 * \memberof BezierCommon
 */
int
beziercommon_closest_segment (BezierCommon *bezier,
			      const Point  *point,
			      real          line_width)
{
  Point last;
  int i;
  real dist = G_MAXDOUBLE;
  int closest;

  closest = 0;
  last = bezier->points[0].p1;
  for (i = 0; i < bezier->num_points - 1; i++) {
    real new_dist = distance_bez_seg_point(&last, &bezier->points[i+1], line_width, point);
    if (new_dist < dist) {
      dist = new_dist;
      closest = i;
    }
    if (bezier->points[i+1].type == BEZ_CURVE_TO)
      last = bezier->points[i+1].p3;
    else
      last = bezier->points[i+1].p1;
  }
  return closest;
}

/*!
 * \brief Draw control lines of the given _BezPoint array
 */
void
bezier_draw_control_lines (int          num_points,
			   BezPoint    *points,
			   DiaRenderer *renderer)
{
  Color line_colour = { 0.0, 0.0, 0.6, 1.0 };
  Point startpoint;
  int i;
  
  /* setup renderer ... */
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, 0);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_DOTTED);
  DIA_RENDERER_GET_CLASS(renderer)->set_dashlength(renderer, 1);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  startpoint = points[0].p1;
  for (i = 1; i < num_points; i++) {
    DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &startpoint, &points[i].p1, &line_colour);
    if (points[i].type == BEZ_CURVE_TO) {
      DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &points[i].p2, &points[i].p3, &line_colour);
      startpoint = points[i].p3;
    } else {
      startpoint = points[i].p1;
    }
  }
}
