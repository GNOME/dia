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

/* include glib.h first, so we don't pick up its inline functions as well */
#include <config.h>

#include <glib.h>
/* include normal versions of the inlined functions here ... */
#undef G_INLINE_FUNC
#define G_INLINE_FUNC extern
#define G_CAN_INLINE 1
#include "geometry.h"

#include "object.h"
#include "units.h"

void
rectangle_union(Rectangle *r1, const Rectangle *r2)
{
  r1->top = MIN( r1->top, r2->top );
  r1->bottom = MAX( r1->bottom, r2->bottom );
  r1->left = MIN( r1->left, r2->left );
  r1->right = MAX( r1->right, r2->right );
}

void
int_rectangle_union(IntRectangle *r1, const IntRectangle *r2)
{
  r1->top = MIN( r1->top, r2->top );
  r1->bottom = MAX( r1->bottom, r2->bottom );
  r1->left = MIN( r1->left, r2->left );
  r1->right = MAX( r1->right, r2->right );
}

void
rectangle_intersection(Rectangle *r1, const Rectangle *r2)
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
rectangle_intersects(const Rectangle *r1, const Rectangle *r2)
{
  if ( (r1->right < r2->left) ||
       (r1->left > r2->right) ||
       (r1->top > r2->bottom) ||
       (r1->bottom < r2->top) )
    return FALSE;

  return TRUE;
}

int
point_in_rectangle(const Rectangle* r, const Point *p)
{
  if ( (p->x < r->left) ||
       (p->x > r->right) ||
       (p->y > r->bottom) ||
       (p->y < r->top) )
    return FALSE;

  return TRUE;
}

int
rectangle_in_rectangle(const Rectangle* outer, const Rectangle *inner)
{
  if ( (inner->left < outer->left) ||
       (inner->right > outer->right) ||
       (inner->top < outer->top) ||
       (inner->bottom > outer->bottom))
    return FALSE;
  
  return TRUE;
}

void
rectangle_add_point(Rectangle *r, const Point *p)
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
distance_rectangle_point(const Rectangle *rect, const Point *point)
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

/* returns 1 iff the line crosses the ray from (-Inf, rayend.y) to rayend */
static guint
line_crosses_ray(const Point *line_start, 
                 const Point *line_end, const Point *rayend)
{
  coord xpos;

  /* swap end points if necessary */
  if (line_start->y > line_end->y) {
    const Point *tmp;

    tmp = line_start;
    line_start = line_end;
    line_end = tmp;
  }
  /* if y coords of line do not include rayend.y */
  if (line_start->y > rayend->y || line_end->y < rayend->y)
    return 0;
  /* Avoid division by zero for horizontal case */
  if (line_end->y - line_start->y < 0.00000000001) {
    return (line_end->y - rayend->y < 0.00000000001);
  }
  xpos = line_start->x + (rayend->y - line_start->y) * 
    (line_end->x - line_start->x) / (line_end->y - line_start->y);
  return xpos <= rayend->x;
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
distance_bez_seg_point(const Point *b1, const Point *b2, 
                       const Point *b3, const Point *b4,
		       real line_width, const Point *point)
{
  return bez_point_distance_and_ray_crosses(b1, b2, b3, b4,
					    line_width, point, NULL);
}
			     
real
distance_bez_line_point(const BezPoint *b, guint npoints,
			real line_width, const Point *point)
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
distance_bez_shape_point(const BezPoint *b, guint npoints,
                         real line_width, const Point *point)
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

  scale = w2 * h2 / (4*h2*pt.x + 4*w2*pt.y);
  rad = sqrt((pt.x + pt.y)*scale) + line_width/2;

  dist = sqrt(pt.x + pt.y);

  if (dist <= rad)
    return 0.0;
  else
    return dist - rad;
}


void
transform_point (Matrix m, Point *src, Point *dest)
{
  real xx, yy, ww;

  xx = m[0][0] * src->x + m[0][1] * src->y + m[0][2];
  yy = m[1][0] * src->x + m[1][1] * src->y + m[1][2];
  ww = m[2][0] * src->x + m[2][1] * src->y + m[2][2];

  if (!ww)
    ww = 1.0;

  dest->x = xx / ww;
  dest->y = yy / ww;
}

void
mult_matrix (Matrix m1, Matrix m2)
{
  Matrix result;
  int i, j, k;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      {
	result [i][j] = 0.0;
	for (k = 0; k < 3; k++)
	  result [i][j] += m1 [i][k] * m2[k][j];
      }

  /*  copy the result into matrix 2  */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m2 [i][j] = result [i][j];
}

void
identity_matrix (Matrix m)
{
  int i, j;

  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      m[i][j] = (i == j) ? 1 : 0;

}

void
translate_matrix (Matrix m, real x, real y)
{
  Matrix trans;

  identity_matrix (trans);
  trans[0][2] = x;
  trans[1][2] = y;
  mult_matrix (trans, m);
}

void
scale_matrix (Matrix m, real x, real y)
{
  Matrix scale;

  identity_matrix (scale);
  scale[0][0] = x;
  scale[1][1] = y;
  mult_matrix (scale, m);
}

void
rotate_matrix (Matrix m, real theta)
{
  Matrix rotate;
  real cos_theta, sin_theta;

  cos_theta = cos (theta);
  sin_theta = sin (theta);

  identity_matrix (rotate);
  rotate[0][0] = cos_theta;
  rotate[0][1] = -sin_theta;
  rotate[1][0] = sin_theta;
  rotate[1][1] = cos_theta;
  mult_matrix (rotate, m);
}

void
xshear_matrix (Matrix m, real shear)
{
  Matrix shear_m;

  identity_matrix (shear_m);
  shear_m[0][1] = shear;
  mult_matrix (shear_m, m);
}

void
yshear_matrix (Matrix m, real shear)
{
  Matrix shear_m;

  identity_matrix (shear_m);
  shear_m[1][0] = shear;
  mult_matrix (shear_m, m);
}

/*  find the determinate for a 3x3 matrix  */
static real
determinate (Matrix m)
{
  int i;
  double det = 0;

  for (i = 0; i < 3; i ++)
    {
      det += m[0][i] * m[1][(i+1)%3] * m[2][(i+2)%3];
      det -= m[2][i] * m[1][(i+1)%3] * m[0][(i+2)%3];
    }

  return det;
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
    return (acos(t));
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

/* Compute a circular arc fillet between lines L1 (p1 to p2)
   and L2 (p3 to p4) with radius r.
   The circle center is c.
   The start angle is pa
   The end angle is aa,
   The points p1-p4 will be modified as necessary */
void fillet(Point *p1, Point *p2, Point *p3, Point *p4,
                   real r, Point *c, real *pa, real *aa) {
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
  {
    return;
  }

  mp.x = (p3->x + p4->x) / 2.0;          /* Find midpoint of p3 p4 */
  mp.y = (p3->y + p4->y) / 2.0;
  d1 = line_to_point(a1, b1, c1, &mp);    /* Find distance p1 p2 to
                                             midpoint p3 p4 */
  if ( d1 == 0.0 ) return;                /* p1p2 to p3 */

  mp.x = (p1->x + p2->x) / 2.0;          /* Find midpoint of p1 p2 */
  mp.y = (p1->y + p2->y) / 2.0;
  d2 = line_to_point(a2, b2, c2, &mp);    /* Find distance p3 p4 to
                                             midpoint p1 p2 */
  if ( d2 == 0.0 ) return;

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
  if ( point_cross(&gv1,&gv2) < 0.0 ) {
    stop_angle = -stop_angle; /* Angle subtended by arc */
    /* also this means we need to swap our angles later on */
    righthand = TRUE;
  }
  /* now calculate the actual angles in a form that the draw arc function
     of the renderer can use */
  start_angle = start_angle*180.0/G_PI;
  stop_angle  = start_angle + stop_angle*180.0/G_PI;
  while (start_angle < 0.0) start_angle += 360.0;
  while (stop_angle < 0.0)  stop_angle += 360.0;
  /* swap the start and stop if we had to negate the cross product */
  if ( righthand ) {
    real tmp = start_angle;
    start_angle = stop_angle;
    stop_angle = tmp;
  }
  *pa = start_angle;
  *aa = stop_angle;
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
  dist = obj->ops->distance_from(obj, &mid3);
  if (dist < 0.001) return mid1;


  do {
    dist = obj->ops->distance_from(obj, &mid2);
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
      printf("%d: %f, %f: %f\n", i, trace[i].x, trace[i].y, disttrace[i]);
    }
    printf("i = %d, dist = %f\n", i, dist);
  }
#endif

  return mid2;
}

