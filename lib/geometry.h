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
#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <glib.h>
#include <math.h>

/*
  Coordinate system used:
   +---> x
   |
   |
   V  y
 */

typedef double real;
typedef real coord;

typedef struct _Point Point;
typedef struct _Rectangle Rectangle;
typedef struct _IntRectangle IntRectangle;
typedef struct _BezPoint BezPoint;

struct _Point {
  coord x;
  coord y;
};

struct _Rectangle {
  coord top;
  coord left;
  coord bottom;
  coord right;
};

struct _IntRectangle {
  int top;
  int left;
  int bottom;
  int right;
};

/* BezPoint types:
 *   BEZ_MOVE_TO: move to point p1
 *   BEZ_LINE_TO: line to point p1
 *   BEZ_CURVE_TO: curve to point p3 using p1 and p2 as control points.
 */
struct _BezPoint {
  enum {BEZ_MOVE_TO, BEZ_LINE_TO, BEZ_CURVE_TO} type;
  Point p1, p2, p3;
};


#define ROUND(x) ((int) floor((x)+0.5))

/* inline these functions if the platform supports it */

G_INLINE_FUNC void point_add(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_add(Point *p1, Point *p2)
{
  p1->x += p2->x;
  p1->y += p2->y;
}
#endif

G_INLINE_FUNC void point_sub(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_sub(Point *p1, Point *p2)
{
  p1->x -= p2->x;
  p1->y -= p2->y;
}
#endif

G_INLINE_FUNC real point_dot(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
point_dot(Point *p1, Point *p2)
{
  return p1->x*p2->x + p1->y*p2->y;
}
#endif

G_INLINE_FUNC real point_len(Point *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
point_len(Point *p)
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
  
  p->x /= len;
  p->y /= len;
}
#endif

G_INLINE_FUNC void point_rotate(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_rotate(Point *p1, Point *p2)
{
  p1->x = p1->x*p2->x - p1->y*p2->y;
  p1->y = p1->x*p2->y + p1->y*p2->x;
}
#endif

G_INLINE_FUNC void point_get_normed(Point *dst, Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_get_normed(Point *dst, Point *src)
{
  real len;

  len = sqrt(src->x*src->x + src->y*src->y);
  
  dst->x = src->x / len;
  dst->y = src->y / len;
}
#endif

G_INLINE_FUNC void point_get_perp(Point *dst, Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void
point_get_perp(Point *dst, Point *src)
{
  /* dst = the src vector, rotated 90<B0> counter clowkwise. src *must* be 
     normalized before. */
  dst->y = src->x;
  dst->x = -src->y;
}
#endif

G_INLINE_FUNC void point_copy(Point *dst, Point *src);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
point_copy(Point *dst, Point *src)
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

G_INLINE_FUNC void point_add_scaled(Point *dst, Point *src, real alpha);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
point_add_scaled(Point *dst, Point *src, real alpha)
{
  /* especially useful if src is a normed vector... */
  dst->x += alpha * src->x;
  dst->y += alpha * src->y;
}
#endif

G_INLINE_FUNC void point_copy_add_scaled(Point *dst, Point *src, Point *vct,
					 real alpha);
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
point_copy_add_scaled(Point *dst, Point *src, Point *vct, real alpha)
{
  /* especially useful if vct is a normed vector... */
  dst->x = src->x + (alpha * vct->x);
  dst->y = src->y + (alpha * vct->y);
}
#endif

G_INLINE_FUNC void check_bb_x( Rectangle *bb, real val, real check );
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
check_bb_x( Rectangle *bb, real val, real check )
{
  /* if x is outside the bounding box, and check is real, adjust the bb */
  if (finite(check)) { 
    if (bb->left > val) bb->left = val; 
    if (bb->right < val) bb->right = val; 
  }
}
#endif

G_INLINE_FUNC void check_bb_y( Rectangle *bb, real val, real check );
#ifdef G_CAN_INLINE
G_INLINE_FUNC void 
check_bb_y( Rectangle *bb, real val, real check )
{
  /* if y is outside the bounding box, and check is real, adjust the bb */
  if (finite(check)) { 
    if (bb->top > val) bb->top = val; 
    if (bb->bottom < val) bb->bottom = (val); 
  } 
}
#endif

void rectangle_union(Rectangle *r1, Rectangle *r2);
void int_rectangle_union(IntRectangle *r1, IntRectangle *r2);
void rectangle_intersection(Rectangle *r1, Rectangle *r2);
int rectangle_intersects(Rectangle *r1, Rectangle *r2);
int point_in_rectangle(Rectangle* r, Point *p);
int rectangle_in_rectangle(Rectangle* outer, Rectangle *inner);
void rectangle_add_point(Rectangle *r, Point *p);

G_INLINE_FUNC real distance_point_point(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
distance_point_point(Point *p1, Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return sqrt(dx*dx + dy*dy);
}
#endif

G_INLINE_FUNC real distance_point_point_manhattan(Point *p1, Point *p2);
#ifdef G_CAN_INLINE
G_INLINE_FUNC real
distance_point_point_manhattan(Point *p1, Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return ABS(dx) + ABS(dy);
}
#endif

real distance_rectangle_point(Rectangle *rect, Point *point);
real distance_line_point(Point *line_start, Point *line_end,
			 real line_width, Point *point);

real distance_polygon_point(Point *poly, guint npoints,
			    real line_width, Point *point);

/* bezier distance calculations */
real distance_bez_seg_point(Point *b1, Point *b2, Point *b3, Point *b4,
			    real line_width, Point *point);
real distance_bez_line_point(BezPoint *b, guint npoints,
			     real line_width, Point *point);
real distance_bez_shape_point(BezPoint *b, guint npoints,
			      real line_width, Point *point);

real distance_ellipse_point(Point *centre, real width, real height,
			    real line_width, Point *point);

#endif /* GEOMETRY_H */
