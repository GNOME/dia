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

/** \file geometry.h -- basic geometry classes and functions operationg on them */
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <config.h>

#include "diatypes.h"

#include <glib.h>
#include <math.h>

G_BEGIN_DECLS

#define DIA_RADIANS(degrees) ((degrees) * G_PI / 180.0)
#define DIA_DEGREES(radians) ((radians) * 180.0 / G_PI)


/*
  Coordinate system used:
   +---> x
   |
   |
   V  y
 */


/**
 * Point:
 * @x: horizontal
 * @y: vertical
 *
 * A two dimensional position
 *
 * Since: dawn-of-time
 */
struct _Point {
  double x;
  double y;
};


/**
 * DiaRectangle:
 * @left: top left x co-ord
 * @top: top left y co-ord
 * @right: bottom right x co-ord
 * @button: bottom right y co-ord
 *
 * A rectangle given by upper left and lower right corner
 *
 * Since: 0.98
 */
struct _DiaRectangle {
  double left;
  double top;
  double right;
  double bottom;
};


/**
 * BezPoint:
 * @BEZ_MOVE_TO: move to point @p1
 * @BEZ_LINE_TO: line to point @p1
 * @BEZ_CURVE_TO: curve to point @p3 using @p1 and @p2 as control points
 * @p1: main point in case of move or line-to, otherwise first control point
 * @p2: second control point
 * @p3: main point for 'true' bezier point
 *
 * #BezPoint is a bezier point forming #Bezierline or #Beziergon
 *
 * Since: dawn-of-time
 */
struct _BezPoint {
  enum {
    BEZ_MOVE_TO, /*!< move to point p1 */
    BEZ_LINE_TO, /*!< line to point p1 */
    BEZ_CURVE_TO /*!< curve to point p3 using p1 and p2 as control points */
  } type;
  Point p1; /*!< main point in case of move or line-to, otherwise first control point */
  Point p2; /*!< second control point  */
  Point p3; /*!< main point for 'true' bezier point */
};


/**
 * DiaMatrix:
 *
 * #DiaMatrix used for affine transformation
 *
 * The struct is intentionally binary compatible with #cairo_matrix_t.
 *
 * Since: dawn-of-time
 */
struct _DiaMatrix {
  double xx;
  double yx;
  double xy;
  double yy;
  double x0;
  double y0;
};


gboolean dia_matrix_is_identity          (const DiaMatrix *matix);
gboolean dia_matrix_get_angle_and_scales (const DiaMatrix *m,
                                          double          *a,
                                          double          *sx,
                                          double          *sy);
void     dia_matrix_set_angle_and_scales (DiaMatrix       *m,
                                          double           a,
                                          double           sx,
                                          double           sy);
void     dia_matrix_multiply             (DiaMatrix       *result,
                                          const DiaMatrix *a,
                                          const DiaMatrix *b);
gboolean dia_matrix_is_invertible        (const DiaMatrix *matrix);
void     dia_matrix_set_rotate_around    (DiaMatrix       *result,
                                          double           angle,
                                          const Point     *around);

#define ROUND(x) ((int) floor((x)+0.5))

/* inline these functions if the platform supports it */

static inline void
point_add(Point *p1, const Point *p2)
{
  p1->x += p2->x;
  p1->y += p2->y;
}

static inline void
point_sub(Point *p1, const Point *p2)
{
  p1->x -= p2->x;
  p1->y -= p2->y;
}

static inline real
point_dot(const Point *p1, const Point *p2)
{
  return p1->x*p2->x + p1->y*p2->y;
}

static inline real
point_len(const Point *p)
{
  return sqrt(p->x*p->x + p->y*p->y);
}

static inline void
point_scale(Point *p, real alpha)
{
  p->x *= alpha;
  p->y *= alpha;
}

static inline void
point_normalize(Point *p)
{
  real len;

  len = sqrt(p->x*p->x + p->y*p->y);

  /* One could call it a bug to try normalizing a vector with
   * len 0 and the result at least requires definition. But
   * this is what makes the beziergon bounding box calculation
   * work. What's the mathematical correct result of 0.0/0.0 ?
   */
  if (len > 0.0) {
    p->x /= len;
    p->y /= len;
  } else {
    p->x = 0.0;
    p->y = 0.0;
  }
}

static inline void
point_rotate(Point *p1, const Point *p2)
{
  p1->x = p1->x*p2->x - p1->y*p2->y;
  p1->y = p1->x*p2->y + p1->y*p2->x;
}

static inline void
point_get_normed(Point *dst, const Point *src)
{
  real len;

  len = sqrt(src->x*src->x + src->y*src->y);

  dst->x = src->x / len;
  dst->y = src->y / len;
}

static inline void
point_get_perp(Point *dst, const Point *src)
{
  /* dst = the src vector, rotated 90deg counter clowkwise. src *must* be
     normalized before. */
  dst->y = src->x;
  dst->x = -src->y;
}

static inline void
point_copy(Point *dst, const Point *src)
{
  /* Unfortunately, the compiler is not clever enough. And copying using
     ints is faster if we don't computer based on the copied values, but
     is slower if we have to make a FP reload afterwards.
     point_copy() is meant for the latter case : then, the compiler is
     able to shuffle and merge the FP loads. */
  dst->x = src->x;
  dst->y = src->y;
}

static inline void
point_add_scaled(Point *dst, const Point *src, real alpha)
{
  /* especially useful if src is a normed vector... */
  dst->x += alpha * src->x;
  dst->y += alpha * src->y;
}

static inline void
point_copy_add_scaled(Point *dst, const Point *src,
                      const Point *vct, real alpha)
{
  /* especially useful if vct is a normed vector... */
  dst->x = src->x + (alpha * vct->x);
  dst->y = src->y + (alpha * vct->y);
}

void point_convex(Point *dst, const Point *src1, const Point *src2, real alpha);

void rectangle_union(DiaRectangle *r1, const DiaRectangle *r2);
void rectangle_intersection(DiaRectangle *r1, const DiaRectangle *r2);
int rectangle_intersects(const DiaRectangle *r1, const DiaRectangle *r2);
int point_in_rectangle(const DiaRectangle* r, const Point *p);
int rectangle_in_rectangle(const DiaRectangle* outer, const DiaRectangle *inner);
void rectangle_add_point(DiaRectangle *r, const Point *p);

static inline gboolean
rectangle_equals (const DiaRectangle *r1, const DiaRectangle *r2)
{
  return ( (r2->left == r1->left) &&
           (r2->right == r1->right) &&
           (r2->top == r1->top) &&
           (r2->bottom == r1->bottom) );
}

static inline real
distance_point_point(const Point *p1, const Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return sqrt(dx*dx + dy*dy);
}

static inline real
distance_point_point_manhattan(const Point *p1, const Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return ABS(dx) + ABS(dy);
}

real distance_rectangle_point(const DiaRectangle *rect, const Point *point);
real distance_line_point(const Point *line_start, const Point *line_end,
			 real line_width, const Point *point);

real distance_polygon_point(const Point *poly, guint npoints,
			    real line_width, const Point *point);

/* bezier distance calculations */
real distance_bez_seg_point(const Point *b1, const BezPoint *b2,
			    real line_width, const Point *point);
real distance_bez_line_point(const BezPoint *b, guint npoints,
			     real line_width, const Point *point);
real distance_bez_shape_point(const BezPoint *b, guint npoints,
			      real line_width, const Point *point);

real distance_ellipse_point(const Point *centre, real width, real height,
			    real line_width, const Point *point);

void transform_length (real *length, const DiaMatrix *m);
void transform_point (Point *pt, const DiaMatrix *m);
void transform_bezpoint (BezPoint *bpt, const DiaMatrix *m);

real dot2(Point *p1, Point *p2);
void line_coef(real *a, real *b, real *c, Point *p1, Point *p2);
real line_to_point(real a, real b , real c, Point *p);
gboolean line_line_intersection (Point *crossing,
				 const Point *p1, const Point *p2,
				 const Point *p3, const Point *p4);
void point_perp(Point *p, real a, real b, real c, Point *perp);
gboolean fillet(Point *p1, Point *p2, Point *p3, Point *p4,
		real r, Point *c, real *pa, real *aa);
int  three_point_circle(const Point *p1, const Point *p2, const Point *p3,
                        Point* center, real* radius);
real point_cross(Point *p1, Point *p2);
Point calculate_object_edge(Point *objmid, Point *end, DiaObject *obj);

real dia_asin (real x);
real dia_acos (real x);

G_END_DECLS

#endif /* GEOMETRY_H */
