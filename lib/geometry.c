/* xxxxxx -- an diagram creation/manipulation program
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
#include <math.h>
#include <glib.h>

#include "geometry.h"

void
point_add(Point *p1, Point *p2)
{
  p1->x += p2->x;
  p1->y += p2->y;
}

void
point_sub(Point *p1, Point *p2)
{
  p1->x -= p2->x;
  p1->y -= p2->y;
}

real
point_dot(Point *p1, Point *p2)
{
  return p1->x*p2->x + p1->y*p2->y;
}
    
void
point_scale(Point *p, real alpha)
{
  p->x *= alpha;
  p->y *= alpha;
}

void
point_normalize(Point *p)
{
  real len;

  len = sqrt(p->x*p->x + p->y*p->y);
  
  p->x /= len;
  p->y /= len;
}

void
rectangle_union(Rectangle *r1, Rectangle *r2)
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

real
distance_point_point(Point *p1, Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return sqrt( dx*dx+dy*dy );
}

real
distance_point_point_manhattan(Point *p1, Point *p2)
{
  real dx = p1->x - p2->x;
  real dy = p1->y - p2->y;
  return ABS(dx) + ABS(dy);
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
 * If the point is on the line, 0.0 is returned. Otherwise the
 * distance in the R^2 metric from the point to the nearest point
 * on the line segment is returned. Does one sqrt per call.
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

