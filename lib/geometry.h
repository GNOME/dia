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

#define ROUND(x) ((int) floor((x)+0.5))

extern void point_add(Point *p1, Point *p2);
extern void point_sub(Point *p1, Point *p2);
extern real point_dot(Point *p1, Point *p2);
extern void point_scale(Point *p, real alpha);
extern void point_normalize(Point *p);

extern void rectangle_union(Rectangle *r1, Rectangle *r2);
extern void rectangle_intersection(Rectangle *r1, Rectangle *r2);
extern int rectangle_intersects(Rectangle *r1, Rectangle *r2);
extern int point_in_rectangle(Rectangle* r, Point *p);
extern int rectangle_in_rectangle(Rectangle* outer, Rectangle *inner);
extern void rectangle_add_point(Rectangle *r, Point *p);

extern real distance_point_point(Point *p1, Point *p2);
extern real distance_point_point_manhattan(Point *p1, Point *p2);
extern real distance_rectangle_point(Rectangle *rect, Point *point);
extern real distance_line_point(Point *line_start, Point *line_end,
				real line_width, Point *point);

#endif /* GEOMETRY_H */



