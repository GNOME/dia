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

#include <math.h>

#include "arrows.h"

static void
calculate_arrow(Point *poly/*[3]*/, Point *to, Point *from,
		real length, real width)
{
  Point delta;
  Point orth_delta;
  real len;
  
  delta = *to;
  point_sub(&delta, from);
  len = sqrt(point_dot(&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale(&delta, length);
  point_scale(&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub(&poly[0], &delta);
  point_sub(&poly[0], &orth_delta);
  poly[1] = *to;
  poly[2] = *to;
  point_sub(&poly[2], &delta);
  point_add(&poly[2], &orth_delta);
}

static void
draw_lines(Renderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, poly, 3, color);
}

static void
draw_triangle(Renderer *renderer, Point *to, Point *from,
		     real length, real width, real linewidth,
		     Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_polygon(renderer, poly, 3, color);
}

static void
fill_triangle(Renderer *renderer, Point *to, Point *from,
		     real length, real width, Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->fill_polygon(renderer, poly, 3, color);
}

static void
calculate_diamond(Point *poly/*[4]*/, Point *to, Point *from,
		  real length, real width)
{
  Point delta;
  Point orth_delta;
  real len;
  
  delta = *to;
  point_sub(&delta, from);
  len = sqrt(point_dot(&delta, &delta));
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale(&delta, length/2.0);
  point_scale(&orth_delta, width/2.0);
  
  poly[0] = *to;
  poly[1] = *to;
  point_sub(&poly[1], &delta);
  point_sub(&poly[1], &orth_delta);
  poly[2] = *to;
  point_sub(&poly[2], &delta);
  point_sub(&poly[2], &delta);
  poly[3] = *to;
  point_sub(&poly[3], &delta);
  point_add(&poly[3], &orth_delta);
}


static void
draw_diamond(Renderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *color)
{
  Point poly[4];

  calculate_diamond(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polygon(renderer, poly, 4, color);
}

static void
fill_diamond(Renderer *renderer, Point *to, Point *from,
	     real length, real width,
	     Color *color)
{
  Point poly[4];

  calculate_diamond(poly, to, from, length, width);
  
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->fill_polygon(renderer, poly, 4, color);
}

void
arrow_draw(Renderer *renderer, ArrowType type,
	   Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *fg_color, Color *bg_color)
{
  switch(type) {
  case ARROW_LINES:
    draw_lines(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_HOLLOW_TRIANGLE:
    fill_triangle(renderer, to, from, length, width, bg_color);
    draw_triangle(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_FILLED_TRIANGLE:
    fill_triangle(renderer, to, from, length, width, fg_color);
    break;
  case ARROW_HOLLOW_DIAMOND:
    fill_diamond(renderer, to, from, length, width, bg_color);
    draw_diamond(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_FILLED_DIAMOND:
    fill_diamond(renderer, to, from, length, width, fg_color);
    break;
  }
  
}

