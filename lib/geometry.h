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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "diatypes.h"

#include <glib.h>
#include <math.h>

/* Solaris 2.4, 2.6, probably 2.5.x, and possibly others prototype
   finite() in ieeefp.h instead of math.h.  finite() might not be
   available at all on some HP-UX configurations (in which case,
   you're on your own). */
#ifdef HAVE_IEEEFP_H
#include <ieeefp.h>
#endif
#ifndef HAVE_ISINF
#  ifndef isinf
#    define isinf(a) (!finite(a))
#  endif
#endif

#ifdef _MSC_VER 
/* #ifdef G_OS_WIN32  apparently _MSC_VER and mingw */
   /* there are some things more in the gcc headers */
#  include <float.h>
#  define finite(a) _finite(a)
#  ifndef isnan
#    define isnan(a) _isnan(a)
#  endif
#endif
#ifdef G_OS_WIN32
#  define M_PI      3.14159265358979323846
#  define M_SQRT2	1.41421356237309504880	/* sqrt(2) */
#  define M_SQRT1_2 0.70710678118654752440	/* 1/sqrt(2) */
#endif

/* gcc -std=c89 doesn't have it either */
#ifndef M_PI
#define M_PI G_PI
#endif
#ifndef M_SQRT2
#define M_SQRT2 G_SQRT2
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2 (1.0/G_SQRT2)
#endif

G_BEGIN_DECLS


/*
  Coordinate system used:
   +---> x
   |
   |
   V  y
 */
typedef real coord;

/*! \brief A two dimensional position */
struct _Point {
  coord x; /*!< horizontal */
  coord y; /*!< vertical */
};

/*! \brief A rectangle given by upper left and lower right corner */
struct _Rectangle {
  coord left; /*!< x1 */
  coord top; /*!< y1 */
  coord right; /*!< x2 */
  coord bottom; /*!< y2 */
};

/*! \brief A rectangle for fixed point e.g. pixel coordinates */
struct _IntRectangle {
  int left; /*!< x1 */
  int top; /*!< y1 */
  int right; /*!< x2 */
  int bottom; /*!< y2 */
};

/*! 
 * \brief BezPoint is a bezier point forming _Bezierline or _Beziergon
 * \ingroup ObjectParts
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

/*!
 * \brief DiaMatrix used for affine transformation
 *
 * The struct is intentionally binary compatible with cairo_matrix_t.
 *
 * \ingroup ObjectParts
 */
struct _DiaMatrix {
  real xx; real yx;
  real xy; real yy;
  real x0; real y0;
};

gboolean dia_matrix_is_identity (const DiaMatrix *matix);
gboolean dia_matrix_get_angle_and_scales (const DiaMatrix *m,
                                          real            *a,
					  real            *sx,
					  real            *sy);
void dia_matrix_set_angle_and_scales (DiaMatrix *m,
                                      real       a,
				      real       sx,
				      real       sy);
void dia_matrix_multiply (DiaMatrix *result, const DiaMatrix *a, const DiaMatrix *b);
gboolean dia_matrix_is_invertible (const DiaMatrix *matrix);
void dia_matrix_set_rotate_around (DiaMatrix *result, real angle, const Point *around);

#define ROUND(x) ((int) floor((x)+0.5))

/* inline these functions if the platform supports it */

G_INLINE_FUNC void point_add(Point *p1, const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_add(Point *p1, const Point *p2)
{
  p1->x += p2->x;
  p1->y += p2->y;
}
#endif

G_INLINE_FUNC void point_sub(Point *p1, const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_sub(Point *p1, const Point *p2)
{
  p1->x -= p2->x;
  p1->y -= p2->y;
}
#endif

G_INLINE_FUNC real point_dot(const Point *p1, const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
point_dot(const Point *p1, const Point *p2)
{
  return p1->x*p2->x + p1->y*p2->y;
}
#endif

G_INLINE_FUNC real point_len(const Point *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
point_len(const Point *p)
{
  return sqrt(p->x*p->x + p->y*p->y);
}
#endif

G_INLINE_FUNC void point_scale(Point *p, real alpha);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_scale(Point *p, real alpha)
{
  p->x *= alpha;
  p->y *= alpha;
}
#endif

G_INLINE_FUNC void point_normalize(Point *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
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
#endif

G_INLINE_FUNC void point_rotate(Point *p1, const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_rotate(Point *p1, const Point *p2)
{
  p1->x = p1->x*p2->x - p1->y*p2->y;
  p1->y = p1->x*p2->y + p1->y*p2->x;
}
#endif

G_INLINE_FUNC void point_get_normed(Point *dst, const Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_get_normed(Point *dst, const Point *src)
{
  real len;

  len = sqrt(src->x*src->x + src->y*src->y);
  
  dst->x = src->x / len;
  dst->y = src->y / len;
}
#endif

G_INLINE_FUNC void point_get_perp(Point *dst, const Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_get_perp(Point *dst, const Point *src)
{
  /* dst = the src vector, rotated 90deg counter clowkwise. src *must* be 
     normalized before. */
  dst->y = src->x;
  dst->x = -src->y;
}
#endif

G_INLINE_FUNC void point_copy(Point *dst, const Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
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
#endif

G_INLINE_FUNC void point_add_scaled(Point *dst, const Point *src, real alpha);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
point_add_scaled(Point *dst, const Point *src, real alpha)
{
  /* especially useful if src is a normed vector... */
  dst->x += alpha * src->x;
  dst->y += alpha * src->y;
}
#endif

G_INLINE_FUNC void point_copy_add_scaled(Point *dst, const Point *src, 
                                         const Point *vct,
					 real alpha);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
point_copy_add_scaled(Point *dst, const Point *src, 
                      const Point *vct, real alpha)
{
  /* especially useful if vct is a normed vector... */
  dst->x = src->x + (alpha * vct->x);
  dst->y = src->y + (alpha * vct->y);
}
#endif

void point_convex(Point *dst, const Point *src1, const Point *src2, real alpha);

void rectangle_union(Rectangle *r1, const Rectangle *r2);
void int_rectangle_union(IntRectangle *r1, const IntRectangle *r2);
void rectangle_intersection(Rectangle *r1, const Rectangle *r2);
int rectangle_intersects(const Rectangle *r1, const Rectangle *r2);
int point_in_rectangle(const Rectangle* r, const Point *p);
int rectangle_in_rectangle(const Rectangle* outer, const Rectangle *inner);
void rectangle_add_point(Rectangle *r, const Point *p);

G_INLINE_FUNC gboolean rectangle_equals(const Rectangle *old_extents, 
                                         const Rectangle *new_extents);
#ifdef G_CAN_INLINE
G_INLINE_FUNC gboolean 
rectangle_equals(const Rectangle *r1, const Rectangle *r2)
{
  return ( (r2->left == r1->left) &&
           (r2->right == r1->right) &&
           (r2->top == r1->top) &&
           (r2->bottom == r1->bottom) );  
}
#endif

G_INLINE_FUNC real distance_point_point(const Point *p1, const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
distance_point_point(const Point *p1, const Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return sqrt(dx*dx + dy*dy);
}
#endif

G_INLINE_FUNC real distance_point_point_manhattan(const Point *p1, 
                                                  const Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
distance_point_point_manhattan(const Point *p1, const Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return ABS(dx) + ABS(dy);
}
#endif

real distance_rectangle_point(const Rectangle *rect, const Point *point);
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
