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

/* include glib.h first, so we don't pick up its inline functions as well */
#include <glib.h>
/* include normal versions of the inlined functions here ... */
#undef G_INLINE_FUNC
#define G_INLINE_FUNC extern
#define G_CAN_INLINE 1
#include "geometry.h"

void
rectangle_union(Rectangle *r1, Rectangle *r2)
{
  r1->top = MIN( r1->top, r2->top );
  r1->bottom = MAX( r1->bottom, r2->bottom );
  r1->left = MIN( r1->left, r2->left );
  r1->right = MAX( r1->right, r2->right );
}

void
int_rectangle_union(IntRectangle *r1, IntRectangle *r2)
{
  r1->top = MIN( r1->top, r2->top );
  r1->bottom = MAX( r1->bottom, r2->bottom );
  r1->left = MIN( r1->left, r2->left );
  r1->right = MAX( r1->right, r2->right );
}

void
rectangle_intersection(Rectangle *r1, Rectangle *r2)
{
  r1->top = MAX( r1->top, r2->top );
  r1->bottom = MIN( r1->bottom, r2->bottom );
  r1->left = MAX( r1->left, r2->left );
  r1->right = MIN( r1->right, r2->right );

  /* Is rectangle empty? */
  if ( (r1->top >= r1->bottom) ||
       (r1->left >= r1->right) ) {
    r1->top = r1->bottom = r1->left = r1->right = 0.0;
  }
}

int
rectangle_intersects(Rectangle *r1, Rectangle *r2)
{
  if ( (r1->right < r2->left) ||
       (r1->left > r2->right) ||
       (r1->top > r2->bottom) ||
       (r1->bottom < r2->top) )
    return FALSE;

  return TRUE;
}

int
point_in_rectangle(Rectangle* r, Point *p)
{
  if ( (p->x < r->left) ||
       (p->x > r->right) ||
       (p->y > r->bottom) ||
       (p->y < r->top) )
    return FALSE;

  return TRUE;
}

int
rectangle_in_rectangle(Rectangle* outer, Rectangle *inner)
{
  if ( (inner->left < outer->left) ||
       (inner->right > outer->right) ||
       (inner->top < outer->top) ||
       (inner->bottom > outer->bottom))
    return FALSE;
  
  return TRUE;
}

void
rectangle_add_point(Rectangle *r, Point *p)
{
  if (p->x < r->left)
    r->left = p->x;
  else if (p->x > r->right)
    r->right = p->x;

  if (p->y < r->top)
    r->top = p->y;
  else if (p->y > r->bottom)
    r->bottom = p->y;
}


/*
 * This function estimates the distance from a point to a rectangle.
 * If the point is in the rectangle, 0.0 is returned. Otherwise the
 * distance in a manhattan metric from the point to the nearest point
 * on the rectangle is returned.
 */
real
distance_rectangle_point(Rectangle *rect, Point *point)
{
  real dx = 0.0;
  real dy = 0.0;

  if (point->x < rect->left ) {
    dx = rect->left - point->x;
  } else if (point->x > rect->right ) {
    dx = point->x - rect->right;
  }

  if (point->y < rect->top ) {
    dy = rect->top - point->y;
  } else if (point->y > rect->bottom ) {
    dy = point->y - rect->bottom;
  }

  return dx + dy;
}

/*
 * This function estimates the distance from a point to a line segment
 * specified by two endpoints.
 * If the point is on the line segment, 0.0 is returned. Otherwise the
 * distance in the R^2 metric from the point to the nearest point
 * on the line segment is returned. Does one sqrt per call.
 * Philosophical bug: line_width is ignored iff point is beyond
 * end of line segment.
 */
real
distance_line_point(Point *line_start, Point *line_end,
		    real line_width, Point *point)
{
  Point v1, v2;
  real v1_lensq;
  real projlen;
  real perp_dist;

  v1 = *line_end;
  point_sub(&v1,line_start);

  v2 = *point;
  point_sub(&v2, line_start);

  v1_lensq = point_dot(&v1,&v1);

  if (v1_lensq<0.000001) {
    return sqrt(point_dot(&v2,&v2));
  }

  projlen = point_dot(&v1,&v2) / v1_lensq;
  
  if (projlen<0.0) {
    return sqrt(point_dot(&v2,&v2));
  }
  
  if (projlen>1.0) {
    Point v3 = *point;
    point_sub(&v3,line_end);
    return sqrt(point_dot(&v3,&v3));
  }

  point_scale(&v1, projlen);
  point_sub(&v1, &v2);

  perp_dist = sqrt(point_dot(&v1,&v1));

  perp_dist -= line_width / 2.0;
  if (perp_dist < 0.0) {
    perp_dist = 0.0;
  }
  
  return perp_dist;
}

/* returns 1 iff the line crosses the ray from (-Inf, rayend.y) to rayend */
static guint
line_crosses_ray(Point *line_start, Point *line_end, Point *rayend)
{
  coord xpos;

  /* swap end points if necessary */
  if (line_start->y > line_end->y) {
    Point *tmp;

    tmp = line_start;
    line_start = line_end;
    line_end = tmp;
  }
  /* if y coords of line do not include rayend.y */
  if (line_start->y > rayend->y || line_end->y < rayend->y)
    return 0;
  xpos = line_start->x + (rayend->y - line_start->y) * 
    (line_end->x - line_start->x) / (line_end->y - line_start->y);
  return xpos <= rayend->x;
}

real
distance_polygon_point(Point *poly, guint npoints, real line_width,
		       Point *point)
{
  guint i, last = npoints - 1;
  real line_dist = G_MAXFLOAT;
  guint crossings = 0;

  /* calculate ray crossings and line distances */
  for (i = 0; i < npoints; i++) {
    real dist;

    crossings += line_crosses_ray(&poly[last], &poly[i], point);
    dist = distance_line_point(&poly[last], &poly[i], line_width, point);
    line_dist = MIN(line_dist, dist);
    last = i;
  }
  /* If there is an odd number of ray crossings, we are inside the polygon.
   * Otherwise, return the minium distance from a line segment */
  if (crossings % 2 == 1)
    return 0.0;
  else
    return line_dist;
}

/* number of segments to use in bezier curve approximation */
#define NBEZ_SEGS 10

/* if cross is not NULL, it will be incremented for each ray crossing */
static real
bez_point_distance_and_ray_crosses(Point *b1, Point *b2, Point *b3, Point *b4,
				   real line_width, Point *point, guint *cross)
{
  static gboolean calculated_coeff = FALSE;
  static real coeff[NBEZ_SEGS+1][4];
  int i;
  real line_dist = G_MAXFLOAT;
  Point prev, pt;

  if (!calculated_coeff) {
    for (i = 0; i <= NBEZ_SEGS; i++) {
      real t1 = ((real)i)/NBEZ_SEGS, t2 = t1*t1, t3 = t1*t2;
      real it1 = 1-t1, it2 = it1*it1, it3 = it1*it2;

      coeff[i][0] = it3;
      coeff[i][1] = 3 * t1 * it2;
      coeff[i][2] = 3 * t2 * it1;
      coeff[i][3] = t3;
    }
  }
  calculated_coeff = TRUE;

  prev.x = coeff[0][0] * b1->x + coeff[0][1] * b2->x +
           coeff[0][2] * b3->x + coeff[0][3] * b4->x;
  prev.y = coeff[0][0] * b1->y + coeff[0][1] * b2->y +
           coeff[0][2] * b3->y + coeff[0][3] * b4->y;
  for (i = 1; i <= NBEZ_SEGS; i++) {
    real dist;

    pt.x = coeff[i][0] * b1->x + coeff[i][1] * b2->x +
           coeff[i][2] * b3->x + coeff[i][3] * b4->x;
    pt.y = coeff[i][0] * b1->y + coeff[i][1] * b2->y +
           coeff[i][2] * b3->y + coeff[i][3] * b4->y;

    dist = distance_line_point(&prev, &pt, line_width, point);
    line_dist = MIN(line_dist, dist);
    if (cross)
      *cross += line_crosses_ray(&prev, &pt, point);

    prev = pt;
  }
  return line_dist;
}

real
distance_bez_seg_point(Point *b1, Point *b2, Point *b3, Point *b4,
		       real line_width, Point *point)
{
  return bez_point_distance_and_ray_crosses(b1, b2, b3, b4,
					    line_width, point, NULL);
}
			     
real
distance_bez_line_point(BezPoint *b, guint npoints,
			real line_width, Point *point)
{
  Point last;
  guint i;
  real line_dist = G_MAXFLOAT;

  g_return_val_if_fail(b[0].type == BEZ_MOVE_TO, -1);

  last = b[0].p1;

  for (i = 1; i < npoints; i++) {
    real dist;

    switch (b[i].type) {
    case BEZ_MOVE_TO:
      /*g_assert_not_reached();*/
      g_warning("BEZ_MOVE_TO found half way through a bezier line");
      break;
    case BEZ_LINE_TO:
      dist = distance_line_point(&last, &b[i].p1, line_width, point);
      line_dist = MIN(line_dist, dist);
      last = b[i].p1;
      break;
    case BEZ_CURVE_TO:
      dist = bez_point_distance_and_ray_crosses(&last, &b[i].p1, &b[i].p2,
						&b[i].p3, line_width, point,
						NULL);
      line_dist = MIN(line_dist, dist);
      last = b[i].p3;
      break;
    }
  }
  return line_dist;
}

real
distance_bez_shape_point(BezPoint *b, guint npoints,
			real line_width, Point *point)
{
  Point last;
  guint i;
  real line_dist = G_MAXFLOAT;
  guint crossings = 0;

  g_return_val_if_fail(b[0].type == BEZ_MOVE_TO, -1);

  last = b[0].p1;

  for (i = 1; i < npoints; i++) {
    real dist;

    switch (b[i].type) {
    case BEZ_MOVE_TO:
      /*g_assert_not_reached();*/
      g_warning("BEZ_MOVE_TO found half way through a bezier shape");
      break;
    case BEZ_LINE_TO:
      dist = distance_line_point(&last, &b[i].p1, line_width, point);
      crossings += line_crosses_ray(&last, &b[i].p1, point);
      line_dist = MIN(line_dist, dist);
      last = b[i].p1;
      break;
    case BEZ_CURVE_TO:
      dist = bez_point_distance_and_ray_crosses(&last, &b[i].p1, &b[i].p2,
						&b[i].p3, line_width, point,
						&crossings);
      line_dist = MIN(line_dist, dist);
      last = b[i].p3;
      break;
    }
  }
  /* If there is an odd number of ray crossings, we are inside the polygon.
   * Otherwise, return the minium distance from a line segment */
  if (crossings % 2 == 1)
    return 0.0;
  else
    return line_dist;
}

real
distance_ellipse_point(Point *centre, real width, real height,
		       real line_width, Point *point)
{
  real w2 = width*width, h2 = height*height;
  real scale, rad, dist;
  Point pt;

  /* find the radius of the ellipse in the appropriate direction */

  /* find the point of intersection between line (x=cx+(px-cx)t; y=cy+(py-cy)t)
   * and ellipse ((x-cx)^2)/(w/2)^2 + ((y-cy)^2)/(h/2)^2 = 1 */
  /* radius along ray is sqrt((px-cx)^2 * t^2 + (py-cy)^2 * t^2) */

  pt = *point;
  point_sub(&pt, centre);
  pt.x *= pt.x;
  pt.y *= pt.y;  /* pt = (point - centre).^2 */

  scale = w2 * h2 / (4*h2*pt.x + 4*w2*pt.y);
  rad = sqrt((pt.x + pt.y)*scale) + line_width/2;

  dist = sqrt(pt.x + pt.y);

  if (dist <= rad)
    return 0.0;
  else
    return dist - rad;
}
