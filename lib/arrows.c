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

#include <config.h>

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
#include "diarenderer.h"
#include "intl.h"

struct menudesc arrow_types[] =
  {{N_("None"),ARROW_NONE},
   {N_("Lines"),ARROW_LINES},
   {N_("Hollow Triangle"),ARROW_HOLLOW_TRIANGLE},
   {N_("Filled Triangle"),ARROW_FILLED_TRIANGLE},
   {N_("Unfilled Triangle"),ARROW_UNFILLED_TRIANGLE},
   {N_("Filled Diamond"),ARROW_FILLED_DIAMOND},
   {N_("Half Head"),ARROW_HALF_HEAD},
   {N_("Slashed Cross"),ARROW_SLASHED_CROSS},
   {N_("Filled Ellipse"),ARROW_FILLED_ELLIPSE},
   {N_("Hollow Ellipse"),ARROW_HOLLOW_ELLIPSE},
   {N_("Filled Dot"),ARROW_FILLED_DOT},
   {N_("Dimension Origin"),ARROW_DIMENSION_ORIGIN},
   {N_("Blanked Dot"),ARROW_BLANKED_DOT},
   {N_("Double Hollow triangle"),ARROW_DOUBLE_HOLLOW_TRIANGLE},
   {N_("Double Filled triangle"),ARROW_DOUBLE_FILLED_TRIANGLE},
   {N_("Filled Box"),ARROW_FILLED_BOX},
   {N_("Blanked Box"),ARROW_BLANKED_BOX},
   {N_("Slashed"),ARROW_SLASH_ARROW},
   {N_("Integral Symbol"),ARROW_INTEGRAL_SYMBOL},
   {N_("Crow Foot"),ARROW_CROW_FOOT},
   {N_("Cross"),ARROW_CROSS},
   {N_("Filled Concave"),ARROW_FILLED_CONCAVE},
   {N_("Blanked Concave"),ARROW_BLANKED_CONCAVE},
   {NULL,0}};


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

void
calculate_arrow_point(const Arrow *arrow, const Point *to, const Point *from,
		      Point *move_arrow, Point *move_line,
		      const real linewidth)
{
  real add_len;
  real angle;
  Point tmp;

  switch (arrow->type) {
  case ARROW_LINES:
    if (arrow->width < 0.0000001) return;
    angle = atan(arrow->length/(arrow->width/2));
    add_len = .5*linewidth/cos(angle);
    
    *move_arrow = *to;
    point_sub(move_arrow, from);
    point_normalize(move_arrow);    
    point_scale(move_arrow, add_len);

    *move_line = *move_arrow;
    point_scale(move_line, 2.0);
    return;
  case ARROW_HOLLOW_TRIANGLE:
  case ARROW_UNFILLED_TRIANGLE:
    if (arrow->width < 0.0000001) return;
    angle = atan(arrow->length/(arrow->width/2));
    add_len = .5*linewidth/cos(angle);
    
    *move_arrow = *to;
    point_sub(move_arrow, from);
    point_normalize(move_arrow);
    point_scale(move_arrow, add_len);

    *move_line = *move_arrow;
    point_normalize(move_line);
    point_scale(move_line, arrow->length);
    point_add(move_line, move_arrow);
    return;
  case ARROW_FILLED_DIAMOND:
    *move_line = *to;
    point_sub(move_line, from);
    point_normalize(move_line);
    tmp = *move_line;
    point_scale(move_line, arrow->length);
    point_scale(&tmp, M_SQRT2*linewidth);
    point_sub(move_line, &tmp);
    return;
  case ARROW_FILLED_DOT:
  case ARROW_DIMENSION_ORIGIN:
  case ARROW_BLANKED_DOT:
  case ARROW_FILLED_BOX:
  case ARROW_BLANKED_BOX:
    move_arrow->x = 0.0;
    move_arrow->y = 0.0;
    *move_line = *to;
    point_sub(move_line, from);
    point_normalize(move_line);
    point_scale(move_line, .5*arrow->length);
    return;
  case ARROW_FILLED_CONCAVE:
  case ARROW_BLANKED_CONCAVE:
    if (arrow->width < 0.0000001) return;
    angle = atan(arrow->length/(arrow->width/2));
    add_len = .5*linewidth/cos(angle);
    
    *move_arrow = *to;
    point_sub(move_arrow, from);
    point_normalize(move_arrow);
    point_scale(move_arrow, add_len);

    *move_line = *move_arrow;
    point_normalize(move_line);
    point_scale(move_line, .75*arrow->length);
    point_add(move_line, move_arrow);
    return;
  case ARROW_FILLED_TRIANGLE:
  case ARROW_FILLED_ELLIPSE:
  case ARROW_HOLLOW_ELLIPSE:
    move_arrow->x = 0.0;
    move_arrow->y = 0.0;
    *move_line = *to;
    point_sub(move_line, from);
    point_normalize(move_line);
    point_scale(move_line, arrow->length);
    return;
  case ARROW_DOUBLE_HOLLOW_TRIANGLE:
    if (arrow->width < 0.0000001) return;
    angle = atan(arrow->length/(arrow->width/2));
    add_len = .5*linewidth/cos(angle);
    
    *move_arrow = *to;
    point_sub(move_arrow, from);
    point_normalize(move_arrow);
    point_scale(move_arrow, add_len);

    *move_line = *move_arrow;
    point_normalize(move_line);
    tmp = *move_line;
    point_scale(move_line, 2.0*arrow->length);
    point_add(move_line, move_arrow);
    point_scale(&tmp, linewidth);
    point_add(move_line, &tmp);
    return;
  case ARROW_DOUBLE_FILLED_TRIANGLE:
    move_arrow->x = 0.0;
    move_arrow->y = 0.0;
    *move_line = *to;
    point_sub(move_line, from);
    point_normalize(move_line);
    point_scale(move_line, 2*arrow->length);
    return;
  default: 
    move_arrow->x = 0.0;
    move_arrow->y = 0.0;
    move_line->x = 0.0;
    move_line->y = 0.0;
    return;
  }
}

static void
calculate_crow(Point *poly/*[3]*/, Point *to, Point *from,
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
  poly[1] = *to;
  point_sub(&poly[1], &orth_delta);
  poly[2] = *to;
  point_add(&poly[2], &orth_delta);
}

static void 
draw_crow_foot(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *fg_color,Color *bg_color)
{

  Point poly[3];

  calculate_crow(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);

  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer,&poly[0],&poly[1],fg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer,&poly[0],&poly[2],fg_color);
}

static void
draw_lines(DiaRenderer *renderer, Point *to, Point *from,
	              real length, real width, real linewidth,
	              Color *color)
{
      Point poly[3];
    
      calculate_arrow(poly, to, from, length, width);
    
      DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
      DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
      DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
      DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);
    
      DIA_RENDERER_GET_CLASS(renderer)->draw_polyline(renderer, poly, 3, color);
}

static void 
draw_fill_ellipse(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *fg_color,Color *bg_color)
{
  BezPoint bp[5];
  Point vl,vt;

  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

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
    DIA_RENDERER_GET_CLASS(renderer)->fill_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),bg_color);
    DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  } else {
    DIA_RENDERER_GET_CLASS(renderer)->fill_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  }
}

static void 
draw_fill_box(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *fg_color,Color *bg_color)
{
  Point vl,vt;
  Point bs,be;
  Point poly[4];
  real lw_factor,clength,cwidth;
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  if (fg_color == bg_color) {
    /* Filled dot */
    lw_factor = linewidth;
  } else {
    /* Hollow dot or dimension origin */
    lw_factor = 0.0;
  }
  clength = length + lw_factor;
  cwidth = width + lw_factor;

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

  point_copy_add_scaled(&bs,to,&vl,length/4);
  point_copy_add_scaled(&be,&bs,&vt,-width/2.0);
  point_add_scaled(&bs,&vt,width/2.0);
  
  point_copy(&poly[0],to);
  point_copy(&poly[1],&poly[0]);
  point_add_scaled(&poly[0],&vt,cwidth/4.0);
  point_add_scaled(&poly[1],&vt,-cwidth/4.0);
  point_copy_add_scaled(&poly[2],&poly[1],&vl,clength/2.0);
  point_copy_add_scaled(&poly[3],&poly[0],&vl,clength/2.0);

  if (fg_color == bg_color) {
    DIA_RENDERER_GET_CLASS(renderer)->fill_polygon(renderer, poly, 4, fg_color);
  } else {
    DIA_RENDERER_GET_CLASS(renderer)->fill_polygon(renderer, poly, 4, bg_color);
    DIA_RENDERER_GET_CLASS(renderer)->draw_polygon(renderer, poly, 4, fg_color);
  }
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer,&bs,&be,fg_color);
}

static void 
draw_fill_dot(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *fg_color,Color *bg_color)
{
  BezPoint bp[5];
  Point vl,vt;
  Point bs,be;
  real lw_factor,clength,cwidth;
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  if (fg_color == bg_color) {
    /* Filled dot */
    lw_factor = linewidth;
  } else {
    /* Hollow dot or dimension origin */
    lw_factor = 0.0;
  }
  clength = length + lw_factor;
  cwidth = width + lw_factor;

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

  point_copy_add_scaled(&bs,to,&vl,length/4);
  point_copy_add_scaled(&be,&bs,&vt,-width/2.0);
  point_add_scaled(&bs,&vt,width/2.0);
  
  /* This pile of crap is quite well handled by gcc. */ 
  bp[0].type = BEZ_MOVE_TO;
  point_copy(&bp[0].p1,to);
  bp[1].type = bp[2].type = bp[3].type = bp[4].type = BEZ_CURVE_TO;
  point_copy(&bp[4].p3,&bp[0].p1);

  point_copy_add_scaled(&bp[2].p3,&bp[0].p1,&vl,clength/2);
  point_copy_add_scaled(&bp[2].p2,&bp[2].p3,&vt,-cwidth / 8.0);
  point_copy_add_scaled(&bp[3].p1,&bp[2].p3,&vt,cwidth / 8.0);
  point_copy_add_scaled(&bp[1].p1,&bp[0].p1,&vt,-cwidth / 8.0);
  point_copy_add_scaled(&bp[4].p2,&bp[0].p1,&vt,cwidth / 8.0);
  point_copy_add_scaled(&bp[1].p3,&bp[0].p1,&vl,clength / 4.0); /* temp */
  point_copy_add_scaled(&bp[3].p3,&bp[1].p3,&vt,cwidth / 4.0);
  point_add_scaled(&bp[1].p3,&vt,-cwidth / 4.0);
  point_copy_add_scaled(&bp[1].p2,&bp[1].p3,&vl,-clength / 8.0);
  point_copy_add_scaled(&bp[4].p1,&bp[3].p3,&vl,-clength / 8.0);
  point_copy_add_scaled(&bp[2].p1,&bp[1].p3,&vl,clength / 8.0);
  point_copy_add_scaled(&bp[3].p2,&bp[3].p3,&vl,clength / 8.0);

  if (!bg_color) {
    /* Means dimension origin */
    Point dos,doe;

    point_copy_add_scaled(&doe,to,&vl,length);
    point_copy_add_scaled(&dos,to,&vl,length/2);
    
    DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer,&dos,&doe,fg_color);
  } else {
    DIA_RENDERER_GET_CLASS(renderer)->fill_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),bg_color);
  }
  if (fg_color != bg_color) {
    DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
  }
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer,&bs,&be,fg_color);
}

static void
draw_integral(DiaRenderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *fg_color, Color *bg_color)
{
  BezPoint bp[2];
  Point vl,vt;
  Point bs,be, bs2,be2;
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

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

  point_copy_add_scaled(&bs,to,&vl,length/2);
  point_copy_add_scaled(&be,&bs,&vt,-width/2.0);
  point_add_scaled(&bs,&vt,width/2.0);

  point_copy_add_scaled(&bs2,to,&vl,length/2);
  point_copy_add_scaled(&be2,&bs2,&vl,length/2);
  
  bp[0].type = BEZ_MOVE_TO;
  bp[1].type = BEZ_CURVE_TO;
  point_copy_add_scaled(&bp[0].p1,to,&vl,.1*length);
  point_add_scaled(&bp[0].p1,&vt,.4*width);
  point_copy_add_scaled(&bp[1].p3,to,&vl,.9*length);
  point_add_scaled(&bp[1].p3,&vt,-.4*width);
  point_copy_add_scaled(&bp[1].p1,&bp[0].p1,&vl,.35*length);
  point_copy_add_scaled(&bp[1].p2,&bp[1].p3,&vl,-.35*length);

  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, to, &bs2, bg_color); /* kludge */
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bs2, &be2, fg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bs, &be, fg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_bezier(renderer,bp,sizeof(bp)/sizeof(bp[0]),fg_color);
}

static void
draw_slashed(DiaRenderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *fg_color, Color *bg_color)
{
  Point vl,vt;
  Point bs,be, bs2,be2, bs3,be3;
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

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

  point_copy_add_scaled(&bs,to,&vl,length/2);
  point_copy_add_scaled(&be,&bs,&vt,-width/2.0);
  point_add_scaled(&bs,&vt,width/2.0);

  point_copy_add_scaled(&bs2,to,&vl,length/2);
  point_copy_add_scaled(&be2,&bs2,&vl,length/2);
  
  point_copy_add_scaled(&bs3,to,&vl,.1*length);
  point_add_scaled(&bs3,&vt,.4*width);
  point_copy_add_scaled(&be3,to,&vl,.9*length);
  point_add_scaled(&be3,&vt,-.4*width);

  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, to, &bs2, bg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bs2, &be2, fg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bs, &be, fg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &bs3, &be3, fg_color);
}

/* Only draw the upper part of the arrow */
static void
draw_halfhead(DiaRenderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  if (poly[0].y > poly[2].y) 
      poly[0] = poly[2];
    
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[0], &poly[1], color);
}

static void
draw_triangle(DiaRenderer *renderer, Point *to, Point *from,
		     real length, real width, real linewidth,
		     Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);

  DIA_RENDERER_GET_CLASS(renderer)->draw_polygon(renderer, poly, 3, color);
}

static void
fill_triangle(DiaRenderer *renderer, Point *to, Point *from,
		     real length, real width, Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_fillstyle(renderer, FILLSTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);

  DIA_RENDERER_GET_CLASS(renderer)->fill_polygon(renderer, poly, 3, color);
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
draw_diamond(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width, real linewidth,
	     Color *color)
{
  Point poly[4];

  calculate_diamond(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  DIA_RENDERER_GET_CLASS(renderer)->draw_polygon(renderer, poly, 4, color);
}

static void
fill_diamond(DiaRenderer *renderer, Point *to, Point *from,
	     real length, real width,
	     Color *color)
{
  Point poly[4];

  calculate_diamond(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  DIA_RENDERER_GET_CLASS(renderer)->fill_polygon(renderer, poly, 4, color);
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
draw_slashed_cross(DiaRenderer *renderer, Point *to, Point *from,
     real length, real width, real linewidth, Color *color)
{
  Point poly[6];
  
  calculate_slashed_cross(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);
  
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[0],&poly[1], color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[2],&poly[3], color);                   
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[4],&poly[5], color);
}

static void
draw_cross(DiaRenderer *renderer, Point *to, Point *from,
     real length, real width, real linewidth, Color *color)
{
  Point poly[6];
  
  calculate_arrow(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, linewidth);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);
  
  DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[0],&poly[2], color);
  /*DIA_RENDERER_GET_CLASS(renderer)->draw_line(renderer, &poly[4],&poly[5], color); */
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
draw_double_triangle(DiaRenderer *renderer, Point *to, Point *from,
      real length, real width, real linewidth, Color *color)
{
  Point second_from, second_to;
  
  draw_triangle(renderer, to, from, length, width, linewidth, color);
  calculate_double_arrow(&second_to, &second_from, to, from, length+linewidth);
  draw_triangle(renderer, &second_to, &second_from, length, width, linewidth, color);
}

static void 
fill_double_triangle(DiaRenderer *renderer, Point *to, Point *from,
       real length, real width, Color *color)
{
  Point second_from, second_to;
  
  fill_triangle(renderer, to, from, length, width, color);
  calculate_double_arrow(&second_to, &second_from, to, from, length);
  fill_triangle(renderer, &second_to, &second_from, length, width, color);
}

static void
calculate_concave(Point *poly/*[4]*/, Point *to, Point *from,
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

  point_scale(&delta, length/4.0);
  point_scale(&orth_delta, width/2.0);
  
  poly[0] = *to;
  poly[1] = *to;
  point_sub(&poly[1], &delta);
  point_sub(&poly[1], &delta);
  point_sub(&poly[1], &delta);
  point_sub(&poly[1], &delta);
  point_sub(&poly[1], &orth_delta);
  poly[2] = *to;
  point_sub(&poly[2], &delta);
  point_sub(&poly[2], &delta);
  point_sub(&poly[2], &delta);
  poly[3] = *to;
  point_add(&poly[3], &orth_delta);
  point_sub(&poly[3], &delta);
  point_sub(&poly[3], &delta);
  point_sub(&poly[3], &delta);
  point_sub(&poly[3], &delta);
}


static void
draw_concave_triangle(DiaRenderer *renderer, Point *to, Point *from,
		      real length, real width, real linewidth,
		      Color *fg_color, Color *bg_color)
{
  Point poly[4];

  calculate_concave(poly, to, from, length, width);
  
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_linecaps(renderer, LINECAPS_BUTT);

  if (fg_color == bg_color)
    DIA_RENDERER_GET_CLASS(renderer)->fill_polygon(renderer, poly, 4, bg_color);
  DIA_RENDERER_GET_CLASS(renderer)->draw_polygon(renderer, poly, 4, fg_color);
}

void
arrow_draw(DiaRenderer *renderer, ArrowType type,
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
  case ARROW_UNFILLED_TRIANGLE:
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
  case ARROW_FILLED_DOT:
    draw_fill_dot(renderer,to,from,length,width,linewidth,fg_color,fg_color);
    break;
  case ARROW_BLANKED_DOT:
    draw_fill_dot(renderer,to,from,length,width,linewidth,fg_color,bg_color);
    break;
  case ARROW_FILLED_BOX:
    draw_fill_box(renderer,to,from,length,width,linewidth,fg_color,fg_color);
    break;
  case ARROW_BLANKED_BOX:
    draw_fill_box(renderer,to,from,length,width,linewidth,fg_color,bg_color);
    break;
  case ARROW_DIMENSION_ORIGIN:
    draw_fill_dot(renderer,to,from,length,width,linewidth,fg_color,NULL);
    break;
  case ARROW_INTEGRAL_SYMBOL:
    draw_integral(renderer,to,from,length,width,linewidth,fg_color,bg_color);
    break;
  case ARROW_SLASH_ARROW:
    draw_slashed(renderer,to,from,length,width,linewidth,fg_color,bg_color);
    break;
  case ARROW_CROW_FOOT:
    draw_crow_foot(renderer,to,from,length,width,linewidth,fg_color,bg_color);
    break;
  case ARROW_CROSS:
    draw_cross(renderer, to, from, length, width, linewidth, fg_color);
    break;
  case ARROW_FILLED_CONCAVE:
    draw_concave_triangle(renderer, to, from, length, width, linewidth, fg_color, fg_color);
    break;
  case ARROW_BLANKED_CONCAVE:
    draw_concave_triangle(renderer, to, from, length, width, linewidth, fg_color, bg_color);
    break;
  } 
}
