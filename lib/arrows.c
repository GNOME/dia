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

#include <glib.h>
#include <math.h>

#ifdef G_OS_WIN32
#include <float.h>
#define finite(d) _finite(d)
#endif

#ifdef __EMX__
#define finite(d) isfinite(d)
#endif

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
  len = point_len(&delta);
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
draw_fill_ellipse(Renderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *fg_color,Color *bg_color)
{
  BezPoint bp[5];
  Point vl,vt;

  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (!bg_color) {
    /* no bg_color means filled ellipse ; we then compensate for the line width
     */
    length += linewidth;
    width += linewidth;
  }
  point_copy(&vl,from); point_sub(&vl,to);
  if (point_len(&vl) > 0)
    point_normalize(&vl);
  else {
    vl.x = 1.0; vl.y = 0.0;
  }
  if (!finite(vl.x)) {
    vl.x = 1.0; vl.y = 0.0;
  }
  point_get_perp(&vt,&vl);


  /* This pile of crap is quite well handled by gcc. */ 
  bp[0].type = BEZ_MOVE_TO;
  point_copy(&bp[0].p1,to);
  bp[1].type = bp[2].type = bp[3].type = bp[4].type = BEZ_CURVE_TO;
  point_copy(&bp[4].p3,&bp[0].p1);

  point_copy_add_scaled(&bp[2].p3,&bp[0].p1,&vl,length);
  point_copy_add_scaled(&bp[2].p2,&bp[2].p3,&vt,-width / 4.0);
  point_copy_add_scaled(&bp[3].p1,&bp[2].p3,&vt,width / 4.0);
  point_copy_add_scaled(&bp[1].p1,&bp[0].p1,&vt,-width / 4.0);
  point_copy_add_scaled(&bp[4].p2,&bp[0].p1,&vt,width / 4.0);
  point_copy_add_scaled(&bp[1].p3,&bp[0].p1,&vl,length / 2.0); /* temp */
  point_copy_add_scaled(&bp[3].p3,&bp[1].p3,&vt,width / 2.0);
  point_add_scaled(&bp[1].p3,&vt,-width / 2.0);
  point_copy_add_scaled(&bp[1].p2,&bp[1].p3,&vl,-length / 4.0);
  point_copy_add_scaled(&bp[4].p1,&bp[3].p3,&vl,-length / 4.0);
  point_copy_add_scaled(&bp[2].p1,&bp[1].p3,&vl,length / 4.0);
  point_copy_add_scaled(&bp[3].p2,&bp[3].p3,&vl,length / 4.0);
  if (bg_color) {
    renderer->ops->fill_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),bg_color);
    renderer->ops->draw_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  } else {
    renderer->ops->fill_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  }
}

/* Only draw the upper part of the arrow */
static void
draw_halfhead(Renderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (poly[0].y > poly[2].y) 
      poly[0] = poly[2];
    
  renderer->ops->draw_line(renderer, &poly[0], &poly[1], color);
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

static void
calculate_slashed_cross(Point *poly/*[6]*/, Point *to, Point *from,
			real length, real width)
{
  Point delta;
  Point orth_delta;
  real len;
  int i;
  
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

  for(i=0; i<6;i++)poly[i] = *to;

  point_add(&poly[1], &delta);

  point_add(&poly[2], &delta);
  point_add(&poly[2], &orth_delta);

  point_sub(&poly[3], &delta);
  point_sub(&poly[3], &orth_delta);

  point_add(&poly[4], &orth_delta);
  point_sub(&poly[5], &orth_delta);
}

static void
draw_slashed_cross(Renderer *renderer, Point *to, Point *from,
     real length, real width, real linewidth, Color *color)
{
  Point poly[6];
  
  calculate_slashed_cross(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);
  
  renderer->ops->draw_line(renderer, &poly[0],&poly[1], color);
  renderer->ops->draw_line(renderer, &poly[2],&poly[3], color);                   
  renderer->ops->draw_line(renderer, &poly[4],&poly[5], color);
}

static void 
calculate_double_arrow(Point *second_to, Point *second_from, 
                       Point *to, Point *from, real length)
{
  Point delta;
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

  point_scale(&delta, length/2);
  
  *second_to = *to;
  point_sub(second_to, &delta);
  point_sub(second_to, &delta);
  *second_from = *from;
  point_add(second_from, &delta);
  point_add(second_from, &delta);
}

static void 
draw_double_triangle(Renderer *renderer, Point *to, Point *from,
      real length, real width, real linewidth, Color *color)
{
  Point second_from, second_to;
  
  draw_triangle(renderer, to, from, length, width, linewidth, color);
  calculate_double_arrow(&second_to, &second_from, to, from, length+linewidth);
  draw_triangle(renderer, &second_to, &second_from, length, width, linewidth, color);
}

static void 
fill_double_triangle(Renderer *renderer, Point *to, Point *from,
       real length, real width, Color *color)
{
  Point second_from, second_to;
  
  fill_triangle(renderer, to, from, length, width, color);
  calculate_double_arrow(&second_to, &second_from, to, from, length);
  fill_triangle(renderer, &second_to, &second_from, length, width, color);
}

void
arrow_draw(Renderer *renderer, ArrowType type,
	   Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *fg_color, Color *bg_color)
{
  switch(type) {
  case ARROW_NONE:
    break;
  case ARROW_LINES:
    draw_lines(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_HALF_HEAD:
    draw_halfhead(renderer, to, from, length, width, linewidth, fg_color);
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
  case ARROW_SLASHED_CROSS:
    draw_slashed_cross(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_FILLED_ELLIPSE:
    draw_fill_ellipse(renderer,to,from,length,width,linewidth,
		      fg_color,NULL);
    break;
  case ARROW_HOLLOW_ELLIPSE:
    draw_fill_ellipse(renderer,to,from,length,width,linewidth,
		      fg_color,bg_color);
    break;
  case ARROW_DOUBLE_HOLLOW_TRIANGLE:
    fill_double_triangle(renderer, to, from, length+(linewidth/2), width, bg_color);
    draw_double_triangle(renderer, to, from, length, width, linewidth, fg_color);  
    break;
  case ARROW_DOUBLE_FILLED_TRIANGLE:
    fill_double_triangle(renderer, to, from, length, width, fg_color);
    break;
  }
  
}




