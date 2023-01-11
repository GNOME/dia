/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Matrix code from GIMP:app/transform_tool.c
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
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

/* include glib.h first, so we don't pick up its inline functions as well */

#include <glib.h>
/* include normal versions of the inlined functions here ... */
#include "geometry.h"

#include "object.h"
#include "units.h"

void
rectangle_union (DiaRectangle *r1, const DiaRectangle *r2)
{
  r1->top = MIN( r1->top, r2->top );
  r1->bottom = MAX( r1->bottom, r2->bottom );
  r1->left = MIN( r1->left, r2->left );
  r1->right = MAX( r1->right, r2->right );
}


void
rectangle_intersection (DiaRectangle *r1, const DiaRectangle *r2)
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
rectangle_intersects (const DiaRectangle *r1, const DiaRectangle *r2)
{
  if ( (r1->right < r2->left) ||
       (r1->left > r2->right) ||
       (r1->top > r2->bottom) ||
       (r1->bottom < r2->top) )
    return FALSE;

  return TRUE;
}

int
point_in_rectangle (const DiaRectangle* r, const Point *p)
{
  if ( (p->x < r->left) ||
       (p->x > r->right) ||
       (p->y > r->bottom) ||
       (p->y < r->top) )
    return FALSE;

  return TRUE;
}

int
rectangle_in_rectangle (const DiaRectangle* outer, const DiaRectangle *inner)
{
  if ( (inner->left < outer->left) ||
       (inner->right > outer->right) ||
       (inner->top < outer->top) ||
       (inner->bottom > outer->bottom))
    return FALSE;

  return TRUE;
}

void
rectangle_add_point (DiaRectangle *r, const Point *p)
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
distance_rectangle_point (const DiaRectangle *rect, const Point *point)
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
distance_line_point(const Point *line_start, const Point *line_end,
		    real line_width, const Point *point)
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

/* returns 1 if the line crosses the ray from (-Inf, rayend.y) to rayend */
static int
line_crosses_ray(const Point *line_start,
                 const Point *line_end, const Point *rayend)
{
  if ((line_start->y <= rayend->y && line_end->y > rayend->y) || /* upward crossing */
      (line_start->y > rayend->y && line_end->y <= rayend->y)) { /* downward crossing */
    real vt = (rayend->y - line_start->y) / (line_end->y - line_start->y);
    if (rayend->x < line_start->x + vt * (line_end->x - line_start->x)) /* intersect */
      return 1;
  }
  return 0;
}

real
distance_polygon_point(const Point *poly, guint npoints, real line_width,
		       const Point *point)
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
bez_point_distance_and_ray_crosses(const Point *b1,
                                   const Point *b2, const Point *b3,
                                   const Point *b4,
				   real line_width, const Point *point,
                                   guint *cross)
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
distance_bez_seg_point(const Point *b1, const BezPoint *b2,
		       real line_width, const Point *point)
{
  if (b2->type == BEZ_CURVE_TO)
    return bez_point_distance_and_ray_crosses(b1, &b2->p1, &b2->p2, &b2->p3,
					      line_width, point, NULL);
  else
    return distance_line_point(b1, &b2->p1, line_width, point);
}


double
distance_bez_line_point (const BezPoint *b,
                         guint           npoints,
                         double          line_width,
                         const Point    *point)
{
  Point last;
  guint i;
  double line_dist = G_MAXFLOAT;

  g_return_val_if_fail (b[0].type == BEZ_MOVE_TO, -1);

  last = b[0].p1;

  for (i = 1; i < npoints; i++) {
    double dist;

    switch (b[i].type) {
      case BEZ_MOVE_TO:
        last = b[i].p1;
        break;
      case BEZ_LINE_TO:
        dist = distance_line_point (&last, &b[i].p1, line_width, point);
        line_dist = MIN (line_dist, dist);
        last = b[i].p1;
        break;
      case BEZ_CURVE_TO:
        dist = bez_point_distance_and_ray_crosses (&last,
                                                   &b[i].p1, &b[i].p2,
                                                   &b[i].p3, line_width,
                                                   point,
                                                   NULL);
        line_dist = MIN(line_dist, dist);
        last = b[i].p3;
        break;
      default:
        g_return_val_if_reached (G_MAXDOUBLE);
    }
  }
  return line_dist;
}


double
distance_bez_shape_point (const BezPoint *b,
                          guint           npoints,
                          double          line_width,
                          const Point    *point)
{
  Point last;
  const Point *close_to; /* path must be closed to calculate distance */
  guint i;
  double line_dist = G_MAXFLOAT;
  guint crossings = 0;

  g_return_val_if_fail (b[0].type == BEZ_MOVE_TO, -1);

  last = b[0].p1;
  close_to = &b[0].p1;

  for (i = 1; i < npoints; i++) {
    double dist;

    switch (b[i].type) {
      case BEZ_MOVE_TO:
        /* no complains, there are renderers capable to handle this */
        last = b[i].p1;
        close_to = &b[i].p1;
        break;
      case BEZ_LINE_TO:
        dist = distance_line_point (&last, &b[i].p1, line_width, point);
        crossings += line_crosses_ray (&last, &b[i].p1, point);
        line_dist = MIN (line_dist, dist);
        last = b[i].p1;
        if (close_to && close_to->x == last.x && close_to->y == last.y) {
          close_to = NULL;
        }
        break;
      case BEZ_CURVE_TO:
        dist = bez_point_distance_and_ray_crosses (&last, &b[i].p1, &b[i].p2,
                                                   &b[i].p3, line_width,
                                                   point, &crossings);
        line_dist = MIN(line_dist, dist);
        last = b[i].p3;
        if (close_to && close_to->x == last.x && close_to->y == last.y) {
          close_to = NULL;
        }
        break;
      default:
        g_return_val_if_reached (0.0);
    }
  }
  if (close_to) {
    /* final, implicit line-to */
    real dist = distance_line_point (&last, close_to, line_width, point);
    crossings += line_crosses_ray (&last, close_to, point);
    line_dist = MIN (line_dist, dist);
  }
  /* If there is an odd number of ray crossings, we are inside the polygon.
   * Otherwise, return the minimum distance from a line segment */
  if (crossings % 2 == 1) {
    return 0.0;
  } else {
    return line_dist;
  }
}


real
distance_ellipse_point(const Point *centre, real width, real height,
		       real line_width, const Point *point)
{
  /* A faster intersection method would be to scaled the ellipse and the
   * point to where the ellipse is a circle, intersect with the circle,
   * then scale back.
   */
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

  if (pt.x <= 0.0 && pt.y <= 0.0)
    return 0.0; /* instead of division by zero */
  scale = w2 * h2 / (4*h2*pt.x + 4*w2*pt.y);
  rad = sqrt((pt.x + pt.y)*scale) + line_width/2;

  dist = sqrt(pt.x + pt.y);

  if (dist <= rad)
    return 0.0;
  else
    return dist - rad;
}

void
transform_point (Point *pt, const DiaMatrix *m)
{
  real x, y;

  g_return_if_fail (pt != NULL && m != NULL);

  x = pt->x;
  y = pt->y;

  pt->x = x * m->xx + y * m->xy + m->x0;
  pt->y = x * m->yx + y * m->yy + m->y0;
}
void
transform_length (real *len, const DiaMatrix *m)
{
  Point pt = { *len, 0 };
  transform_point (&pt, m);
  /* not interested in the offset */
  pt.x -= m->x0;
  pt.y -= m->y0;
  *len = point_len (&pt);
}
void
transform_bezpoint (BezPoint *bpt, const DiaMatrix *m)
{
  transform_point (&bpt->p1, m);
  transform_point (&bpt->p2, m);
  transform_point (&bpt->p3, m);
}

void
point_convex(Point *dst, const Point *src1, const Point *src2, real alpha)
{
  /* Make convex combination of src1 and src2:
     dst = alpha * src1 + (1-alpha) * src2;
  */
  point_copy(dst,src1);
  point_scale(dst,alpha);
  point_add_scaled(dst,src2,1.0 - alpha);
}



/* The following functions were originally taken from Graphics Gems III,
   pg 193 and corresponding code pgs 496-499

   They were modified to work within the dia coordinate system
 */
/* return angle subtended by two vectors */
real dot2(Point *p1, Point *p2)
{
  real d, t;
  d = sqrt(((p1->x*p1->x)+(p1->y*p1->y)) *
           ((p2->x*p2->x)+(p2->y*p2->y)));
  if ( d != 0.0 )
  {
    t = (p1->x*p2->x+p1->y*p2->y)/d;
    return (dia_acos(t));
  }
  else
  {
    return 0.0;
  }
}

/* Find a,b,c in ax + by + c = 0 for line p1, p2 */
void line_coef(real *a, real *b, real *c, Point *p1, Point *p2)
{
  *c = ( p2->x * p1->y ) - ( p1->x * p2->y );
  *a = p2->y - p1->y;
  *b = p1->x - p2->x;
}

/* Return signed distance from line
   ax + by + c = 0 to point p.              */
real line_to_point(real a, real b , real c, Point *p) {
  real d, lp;
  d = sqrt((a*a)+(b*b));
  if ( d == 0.0 ) {
    lp = 0.0;
  } else {
    lp = (a*p->x+b*p->y+c)/d;
  }
  return (lp);
}

/* Given line l = ax + by + c = 0 and point p,
   compute x,y so point perp is perpendicular to line l. */
void point_perp(Point *p, real a, real b, real c, Point *perp) {
  real d, cp;
  perp->x = 0.0;
  perp->y = 0.0;
  d = a*a + b*b;
  cp = a*p->y-b*p->x;
  if ( d != 0.0 ) {
    perp->x = (-a*c-b*cp)/d;
    perp->y = (a*cp-b*c)/d;
  }
  return;
}

gboolean
line_line_intersection (Point *crossing,
			const Point *p1, const Point *p2,
			const Point *p3, const Point *p4)
{
  real d = (p1->x - p2->x) * (p3->y - p4->y) - (p1->y - p2->y) * (p3->x - p4->x);
  real a, b;

  if (fabs(d) < 0.0000001)
    return FALSE;
  a = p1->x * p2->y - p1->y * p2->x;
  b = p3->x * p4->y - p3->y * p4->x;
  crossing->x = (a * (p3->x - p4->x) - (p1->x - p2->x) * b) / d;
  crossing->y = (a * (p3->y - p4->y) - (p1->y - p2->y) * b) / d;
  return TRUE;
}

/* Compute a circular arc fillet between lines L1 (p1 to p2)
   and L2 (p3 to p4) with radius r.
   The circle center is c.
   The start angle is pa
   The end angle is aa,
   The points p1-p4 will be modified as necessary */
gboolean
fillet(Point *p1, Point *p2, Point *p3, Point *p4,
       real r, Point *c, real *pa, real *aa)
{
  real a1, b1, c1;  /* Coefficients for L1 */
  real a2, b2, c2;  /* Coefficients for L2 */
  real d, d1, d2;
  real c1p, c2p;
  real rr;
  real start_angle, stop_angle;
  Point mp,gv1,gv2;
  gboolean righthand = FALSE;

  line_coef(&a1,&b1,&c1,p1,p2);
  line_coef(&a2,&b2,&c2,p3,p4);

  if ( (a1*b2) == (a2*b1) ) /* Parallel or coincident lines */
    return FALSE;

  mp.x = (p3->x + p4->x) / 2.0;          /* Find midpoint of p3 p4 */
  mp.y = (p3->y + p4->y) / 2.0;
  d1 = line_to_point(a1, b1, c1, &mp);    /* Find distance p1 p2 to
                                             midpoint p3 p4 */
  if ( d1 == 0.0 ) return FALSE;          /* p1p2 to p3 */

  mp.x = (p1->x + p2->x) / 2.0;          /* Find midpoint of p1 p2 */
  mp.y = (p1->y + p2->y) / 2.0;
  d2 = line_to_point(a2, b2, c2, &mp);    /* Find distance p3 p4 to
                                             midpoint p1 p2 */
  if ( d2 == 0.0 ) return FALSE;

  rr = r;
  if ( d1 <= 0.0 ) rr = -rr;

  c1p = c1-rr*sqrt((a1*a1)+(b1*b1));     /* Line parallell l1 at d */
  rr = r;

  if ( d2 <= 0.0 ) rr = -rr;
  c2p = c2-rr*sqrt((a2*a2)+(b2*b2));     /* Line parallell l2 at d */
  d = a1*b2-a2*b1;

  c->x = ( c2p*b1-c1p*b2 ) / d;          /* Intersect constructed lines */
  c->y = ( c1p*a2-c2p*a1 ) / d;          /* to find center of arc */

  point_perp(c,a1,b1,c1,p2);             /* Clip or extend lines as required */
  point_perp(c,a2,b2,c2,p3);

  /* need to negate the y coords to calculate angles correctly */
  gv1.x = p2->x-c->x; gv1.y = -(p2->y-c->y);
  gv2.x = p3->x-c->x; gv2.y = -(p3->y-c->y);

  start_angle = atan2(gv1.y,gv1.x);   /* Beginning angle for arc */
  stop_angle = dot2(&gv1,&gv2);
  righthand = point_cross(&gv1,&gv2) < 0.0;
  /* now calculate the actual angles in a form that the draw_arc function
     of the renderer can use */
  start_angle = start_angle*180.0/G_PI;
  if (righthand)
    stop_angle = -stop_angle;
  stop_angle  = start_angle + stop_angle*180.0/G_PI;
  *pa = start_angle;
  *aa = stop_angle;
  return TRUE;
}

int
three_point_circle (const Point *p1, const Point *p2, const Point *p3,
                    Point* center, real* radius)
{
  const real epsilon = 1e-4;
  real x1 = p1->x;
  real y1 = p1->y;
  real x2 = p2->x;
  real y2 = p2->y;
  real x3 = p3->x;
  real y3 = p3->y;
  real ma, mb;
  if (fabs(x2 - x1) < epsilon)
    return 0;
  if (fabs(x3 - x2) < epsilon)
    return 0;
  ma = (y2 - y1) / (x2 - x1);
  mb = (y3 - y2) / (x3 - x2);
  if (fabs (mb - ma) < epsilon)
    return 0;
  center->x = (ma*mb*(y1-y3)+mb*(x1+x2)-ma*(x2+x3))/(2*(mb-ma));
  if (fabs(ma)>epsilon)
    center->y = -1/ma*(center->x - (x1+x2)/2.0) + (y1+y2)/2.0;
  else if (fabs(mb)>epsilon)
    center->y = -1/mb*(center->x - (x2+x3)/2.0) + (y2+y3)/2.0;
  else
    return 0;
  *radius = distance_point_point(center, p1);
  return 1;
}


/* moved this here since it is being reused by rounded polyline code*/
real
point_cross(Point *p1, Point *p2)
{
  return p1->x * p2->y - p2->x * p1->y;
}

/* moved this here since it is used by line and bezier */
/* Computes one point between end and objmid which
 * touches the edge of the object */
Point
calculate_object_edge(Point *objmid, Point *end, DiaObject *obj)
{
#define MAXITER 25
#ifdef TRACE_DIST
  Point trace[MAXITER];
  real disttrace[MAXITER];
#endif
  Point mid1, mid2, mid3;
  real dist;
  int i = 0;

  mid1 = *objmid;
  mid2.x = (objmid->x+end->x)/2;
  mid2.y = (objmid->y+end->y)/2;
  mid3 = *end;

  /* If the other end is inside the object */
  dist = dia_object_distance_from (obj, &mid3);
  if (dist < 0.001) return mid1;


  do {
    dist = dia_object_distance_from (obj, &mid2);
    if (dist < 0.0000001) {
      mid1 = mid2;
    } else {
      mid3 = mid2;
    }
    mid2.x = (mid1.x + mid3.x)/2;
    mid2.y = (mid1.y + mid3.y)/2;
#ifdef TRACE_DIST
    trace[i] = mid2;
    disttrace[i] = dist;
#endif
    i++;
  } while (i < MAXITER && (dist < 0.0000001 || dist > 0.001));

#ifdef TRACE_DIST
  if (i == MAXITER) {
    for (i = 0; i < MAXITER; i++) {
      g_printerr ("%d: %f, %f: %f\n", i, trace[i].x, trace[i].y, disttrace[i]);
    }
    g_printerr ("i = %d, dist = %f\n", i, dist);
  }
#endif

  return mid2;
}

/*!
 * \brief Check the matrix if it has any effect
 * @param matrix matrix to check
 * \extends _DiaMatrix
 */
gboolean
dia_matrix_is_identity (const DiaMatrix *matrix)
{
  const real epsilon = 1e-6;
  if (   fabs(matrix->xx - 1.0) < epsilon
      && fabs(matrix->yy - 1.0) < epsilon
      && fabs(matrix->xy) < epsilon
      && fabs(matrix->yx) < epsilon
      && fabs(matrix->x0) < epsilon
      && fabs(matrix->y0) < epsilon)
    return TRUE;

  return FALSE;
}

void
dia_matrix_multiply (DiaMatrix *result, const DiaMatrix *a, const DiaMatrix *b)
{
    DiaMatrix r;

    r.xx = a->xx * b->xx + a->yx * b->xy;
    r.yx = a->xx * b->yx + a->yx * b->yy;

    r.xy = a->xy * b->xx + a->yy * b->xy;
    r.yy = a->xy * b->yx + a->yy * b->yy;

    r.x0 = a->x0 * b->xx + a->y0 * b->xy + b->x0;
    r.y0 = a->x0 * b->yx + a->y0 * b->yy + b->y0;

    *result = r;
}

/*!
 * \brief Splitting the given matrix into angle and scales
 * @param m matrix
 * @param a angle in radians
 * @param sx horizontal scale
 * @param sy vertical scale
 *
 * \code
 * with     scale    rotate
 *   xx yx    sx 0     cos(x) sin(x)
 *   xy yy    0  sy   -sin(x) cos(x)
 *
 * rxx =  sx *  cos(a) + 0  * -sin(a)
 * ryx =  sx *  sin(a) + 0  *  cos(a)
 * rxy =  0  *  cos(a) + sy * -sin(a)
 * ryy =  0  * -sin(a) + sy *  cos(a)
 * \endcode
 * \extends _DiaMatrix
 */
gboolean
dia_matrix_get_angle_and_scales (const DiaMatrix *m,
                                 real            *a,
				 real            *sx,
				 real            *sy)
{
  const real epsilon = 1e-6;
  gboolean no_skew;
  real ratio; /* the ratio of the sx/sy */
  real rxx, ryx, rxy, ryy;
  real len1, len2;
  real angle;
  real c, s;

  ratio = m->xx / m->yy;
  /* correct for uniform scale */
  rxx = m->xx / ratio;
  ryx = m->yx / ratio;
  rxy = m->xy;
  ryy = m->yy;
  /* w/o scale it would be len==1 */
  len1 = sqrt(rxx * rxx + ryx * ryx);
  len2 = sqrt(rxy * rxy + ryy * ryy);
  no_skew = fabs(len1 - len2) < epsilon;

  angle = atan2(ryx, rxx);
  if (a)
    *a = angle;
  c = fabs(cos(angle));
  s = fabs(sin(angle));
  if (sx)
    *sx = fabs(c > s ? m->xx / c : m->yx / s);
  if (sy)
    *sy = fabs(s > c ? m->xy / s : m->yy / c);

  return no_skew;
}

/*!
 * \brief Scale in the coordinate system of the shape, afterwards rotate
 * @param m matrix
 * @param a angle in radians
 * @param sx horizontal scale
 * @param sy vertical scale
 * \extends _DiaMatrix
 */
void
dia_matrix_set_angle_and_scales (DiaMatrix *m,
                                 real       a,
				 real       sx,
				 real       sy)
{
  real dx = m->x0;
  real dy = m->y0;
  cairo_matrix_init_rotate ((cairo_matrix_t *)m, a);
  cairo_matrix_scale ((cairo_matrix_t *)m, sx, sy);
  m->x0 = dx;
  m->y0 = dy;
}

gboolean
dia_matrix_is_invertible (const DiaMatrix *matrix)
{
  double a, b, c, d;
  double det;

  a = matrix->xx; b = matrix->yx;
  c = matrix->xy; d = matrix->yy;
  det = a*d - b*c;

  return isfinite(det) && det != 0.0;
}

void
dia_matrix_set_rotate_around (DiaMatrix *result,
			      real angle,
			      const Point *around)
{
  DiaMatrix m = { 1.0, 0.0, 0.0, 1.0,  around->x,  around->y };
  DiaMatrix t = { 1.0, 0.0, 0.0, 1.0, -around->x, -around->y };

  dia_matrix_set_angle_and_scales (&m, angle, 1.0, 1.0);
  dia_matrix_multiply (result, &t, &m);
}
/*!
 * \brief asin wrapped to limit to valid result range
 *
 * Although clipping the value might hide some miscalculation
 * elsewhere this should still be used in all of Dia. Continuing
 * calculation with bogus values might final fall on our foots
 * because rendering libraries might not be graceful.
 * See https://bugzilla.gnome.org/show_bug.cgi?id=710818
 */
real
dia_asin (real x)
{
  /* clamp to valid range */
  if (x <= -1.0)
    return -M_PI/2;
  if (x >= 1.0)
    return M_PI/2;
  return asin (x);
}

/*!
 * \brief acos wrapped to limit to valid result range
 */
real
dia_acos (real x)
{
  /* clamp to valid range */
  if (x <= -1.0)
    return M_PI;
  if (x >= 1.0)
    return 0.0;
  return acos (x);
}
