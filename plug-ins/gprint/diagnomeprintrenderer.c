/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * render_gnomeprint.[ch] -- gnome-print renderer for dia
 * Copyright (C) 1999 James Henstridge
 *
 * diagnomeprintrenderer.[ch] - resurrection as plug-in and porting to
 *                              DiaRenderer interface
 * Copyright (C) 2004 Hans Breuer
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

#include <stdio.h>
#include <string.h>
#include <math.h>

#define G_LOG_DOMAIN "DiaGnomePrint"
#include <glib.h>

#include "intl.h"
#include "message.h"
#include "diagramdata.h"
#include "dia_image.h"

#ifndef P_
/* FIXME: how ? */
#define P_(s) s
#endif

#include "diagnomeprintrenderer.h"
#include <libgnomeprint/gnome-print-pango.h>

/* FIXME: I don't get it ;) */
static const real magic = 72.0 / 2.54;
 
/* DiaRenderer implementation */
static void
begin_render(DiaRenderer *self)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  renderer->font = dia_font_new ("sans", 0, 0.8);
}

static void
end_render(DiaRenderer *self)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  if (renderer->font)
    g_object_unref(G_OBJECT(renderer->font));
  renderer->font = NULL;
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  /* 0 == hairline **/
  gnome_print_setlinewidth(renderer->ctx, linewidth);
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int ps_mode;
  
  switch(mode) {
  case LINECAPS_BUTT:
    ps_mode = 0;
    break;
  case LINECAPS_ROUND:
    ps_mode = 1;
    break;
  case LINECAPS_PROJECTING:
    ps_mode = 2;
    break;
  default:
    ps_mode = 0;
  }

  gnome_print_setlinecap(renderer->ctx, ps_mode);
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int ps_mode;
  
  switch(mode) {
  case LINEJOIN_MITER:
    ps_mode = 0;
    break;
  case LINEJOIN_ROUND:
    ps_mode = 1;
    break;
  case LINEJOIN_BEVEL:
    ps_mode = 2;
    break;
  default:
    ps_mode = 0;
  }

  gnome_print_setlinejoin(renderer->ctx, ps_mode);
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  real hole_width;
  double dashes[6];

  renderer->saved_line_style = mode;

  switch(mode) {
  case LINESTYLE_SOLID:
    gnome_print_setdash(renderer->ctx, 0, dashes, 0);
    break;
  case LINESTYLE_DASHED:
    dashes[0] = renderer->dash_length;
    gnome_print_setdash(renderer->ctx, 1, dashes, 0);
    break;
  case LINESTYLE_DASH_DOT:
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
    dashes[0] = renderer->dash_length;
    dashes[1] = hole_width;
    dashes[2] = renderer->dot_length;
    dashes[3] = hole_width;
    gnome_print_setdash(renderer->ctx, 4, dashes, 0);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    hole_width = (renderer->dash_length - 2.0*renderer->dot_length) / 3.0;
    dashes[0] = renderer->dash_length;
    dashes[1] = hole_width;
    dashes[2] = renderer->dot_length;
    dashes[3] = hole_width;
    dashes[4] = renderer->dot_length;
    dashes[5] = hole_width;
    gnome_print_setdash(renderer->ctx, 6, dashes, 0);
    break;
  case LINESTYLE_DOTTED:
    dashes[0] = renderer->dot_length;
    gnome_print_setdash(renderer->ctx, 1, dashes, 0);
    break;
  }
}

static void
set_dashlength(DiaRenderer *self, real length)
{  
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  /* dot = 20% of len */
  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = magic * length;
  renderer->dot_length = magic * length*0.2;
  
  set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("%s\nUnsupported fill mode specified!\n",
                  G_OBJECT_CLASS_NAME(G_OBJECT_GET_CLASS(renderer)));
  }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  if (renderer->font)
    g_object_unref(G_OBJECT(renderer->font));
  renderer->font = dia_font_copy (font);
  dia_font_set_height (renderer->font, height);
}
 
static void
draw_line(DiaRenderer *self, 
          Point *start, Point *end, 
          Color *line_color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  gnome_print_setrgbcolor(renderer->ctx,
			  (double) line_color->red,
			  (double) line_color->green,
			  (double) line_color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, start->x, start->y);
  gnome_print_lineto(renderer->ctx, end->x, end->y);
  gnome_print_stroke(renderer->ctx);
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int i;
  
  gnome_print_setrgbcolor(renderer->ctx,
			  (double) line_color->red,
			  (double) line_color->green,
			  (double) line_color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    gnome_print_lineto(renderer->ctx, points[i].x, points[i].y);
  }

  gnome_print_stroke(renderer->ctx);
}

static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int i;
  
  gnome_print_setrgbcolor(renderer->ctx,
			  (double) line_color->red,
			  (double) line_color->green,
			  (double) line_color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    gnome_print_lineto(renderer->ctx, points[i].x, points[i].y);
  }

  gnome_print_closepath(renderer->ctx);
  gnome_print_stroke(renderer->ctx);
}

static void
fill_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int i;
  
  gnome_print_setrgbcolor(renderer->ctx,
			  (double) line_color->red,
			  (double) line_color->green,
			  (double) line_color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, points[0].x, points[0].y);

  for (i=1;i<num_points;i++) {
    gnome_print_lineto(renderer->ctx, points[i].x, points[i].y);
  }

  gnome_print_fill(renderer->ctx);
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, ul_corner->x, ul_corner->y);
  gnome_print_lineto(renderer->ctx, ul_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, ul_corner->y);
  gnome_print_closepath(renderer->ctx);
  gnome_print_stroke(renderer->ctx);
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, ul_corner->x, ul_corner->y);
  gnome_print_lineto(renderer->ctx, ul_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, ul_corner->y);
  gnome_print_fill(renderer->ctx);
}

#define SEG_ANGLE (M_PI/4.0)

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  real arcangle;
  int nsegs, i;
  real theta, theta_2, sin_theta_2, cos_theta_2, x1, y1, rangle1, rangle2;
  real w_2 = width/2, h_2 = height/2;
  
  if (angle2 < angle1) angle2 += 360;
  rangle1 = (360 - angle2) * M_PI / 180.0;
  rangle2 = (360 - angle1) * M_PI / 180.0;
  arcangle = rangle2 - rangle1;
  nsegs = (int)ceil(arcangle / SEG_ANGLE); /* how many bezier segments? */
  theta = arcangle / nsegs;
  theta_2 = theta / 2;
  sin_theta_2 = sin(theta_2);
  cos_theta_2 = cos(theta_2);
  x1 = (4 - cos_theta_2)/3;   /* coordinates of control point */
  y1 = (1 - cos_theta_2)*(cos_theta_2 - 3)/(3*sin_theta_2);

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, center->x + w_2 * cos(rangle1),
		     center->y + h_2 * sin(rangle1));
  for (i = 0; i < nsegs; i++) {
    real phi = arcangle * i / nsegs + theta_2 + rangle1;
    real sin_phi = sin(phi), cos_phi = cos(phi);

    gnome_print_curveto(renderer->ctx,
		w_2*(cos_phi*x1 - sin_phi*y1) + center->x,
		h_2*(sin_phi*x1 + cos_phi*y1) + center->y,
		w_2*(cos_phi*x1 + sin_phi*y1) + center->x,
		h_2*(sin_phi*x1 - cos_phi*y1) + center->y,
		w_2*(cos_phi*cos_theta_2 - sin_phi*sin_theta_2) + center->x,
		h_2*(sin_phi*cos_theta_2 + cos_phi*sin_theta_2) + center->y);
  }
  gnome_print_stroke(renderer->ctx);
}

static void
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  real arcangle;
  int nsegs, i;
  real theta, theta_2, sin_theta_2, cos_theta_2, x1, y1, rangle1, rangle2;
  real w_2 = width/2, h_2 = height/2;
  
  if (angle2 < angle1) angle2 += 360;
  rangle1 = (360 - angle2) * M_PI / 180.0;
  rangle2 = (360 - angle1) * M_PI / 180.0;
  arcangle = rangle2 - rangle1;
  nsegs = (int)ceil(arcangle / SEG_ANGLE); /* how many bezier segments? */
  theta = arcangle / nsegs;
  theta_2 = theta / 2;
  sin_theta_2 = sin(theta_2);
  cos_theta_2 = cos(theta_2);
  x1 = (4 - cos_theta_2)/3;   /* coordinates of control point */
  y1 = (1 - cos_theta_2)*(cos_theta_2 - 3)/(3*sin_theta_2);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, center->x, center->y);
  gnome_print_lineto(renderer->ctx, center->x + w_2 * cos(rangle1),
		     center->y + h_2 * sin(rangle1));
  for (i = 0; i < nsegs; i++) {
    real phi = arcangle * i / nsegs + theta_2 + rangle1;
    real sin_phi = sin(phi), cos_phi = cos(phi);

    gnome_print_curveto(renderer->ctx,
		w_2*(cos_phi*x1 - sin_phi*y1) + center->x,
		h_2*(sin_phi*x1 + cos_phi*y1) + center->y,
		w_2*(cos_phi*x1 + sin_phi*y1) + center->x,
		h_2*(sin_phi*x1 - cos_phi*y1) + center->y,
		w_2*(cos_phi*cos_theta_2 - sin_phi*sin_theta_2) + center->x,
		h_2*(sin_phi*cos_theta_2 + cos_phi*sin_theta_2) + center->y);
  }
  gnome_print_fill(renderer->ctx);
}

/* This constant is equal to sqrt(2)/3*(8*cos(pi/8) - 4 - 1/sqrt(2)) - 1.
 * Its use should be quite obvious.
 */
#define ELLIPSE_CTRL1 0.26521648984
/* this constant is equal to 1/sqrt(2)*(1-ELLIPSE_CTRL1).
 * Its use should also be quite obvious.
 */
#define ELLIPSE_CTRL2 0.519570402739
#define ELLIPSE_CTRL3 M_SQRT1_2
/* this constant is equal to 1/sqrt(2)*(1+ELLIPSE_CTRL1).
 * Its use should also be quite obvious.
 */
#define ELLIPSE_CTRL4 0.894643159635

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  real x1 = center->x - width/2,  x2 = center->x + width/2;
  real y1 = center->y - height/2, y2 = center->y + height/2;
  real cw1 = ELLIPSE_CTRL1 * width / 2, cw2 = ELLIPSE_CTRL2 * width / 2;
  real cw3 = ELLIPSE_CTRL3 * width / 2, cw4 = ELLIPSE_CTRL4 * width / 2;
  real ch1 = ELLIPSE_CTRL1 * height / 2, ch2 = ELLIPSE_CTRL2 * height / 2;
  real ch3 = ELLIPSE_CTRL3 * height / 2, ch4 = ELLIPSE_CTRL4 * height / 2;

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, x1, center->y);
  gnome_print_curveto(renderer->ctx,
		      x1,              center->y - ch1,
		      center->x - cw4, center->y - ch2,
		      center->x - cw3, center->y - ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw2, center->y - ch4,
		      center->x - cw1, y1,
		      center->x,       y1);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw1, y1,
		      center->x + cw2, center->y - ch4,
		      center->x + cw3, center->y - ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw4, center->y - ch2,
		      x2,              center->y - ch1,
		      x2,              center->y);
  gnome_print_curveto(renderer->ctx,
		      x2,              center->y + ch1,
		      center->x + cw4, center->y + ch2,
		      center->x + cw3, center->y + ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw2, center->y + ch4,
		      center->x + cw1, y2,
		      center->x,       y2);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw1, y2,
		      center->x - cw2, center->y + ch4,
		      center->x - cw3, center->y + ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw4, center->y + ch2,
		      x1,              center->y + ch1,
		      x1,              center->y);
  gnome_print_closepath(renderer->ctx);
  gnome_print_stroke(renderer->ctx);
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  real x1 = center->x - width/2,  x2 = center->x + width/2;
  real y1 = center->y - height/2, y2 = center->y + height/2;
  real cw1 = ELLIPSE_CTRL1 * width / 2, cw2 = ELLIPSE_CTRL2 * width / 2;
  real cw3 = ELLIPSE_CTRL3 * width / 2, cw4 = ELLIPSE_CTRL4 * width / 2;
  real ch1 = ELLIPSE_CTRL1 * height / 2, ch2 = ELLIPSE_CTRL2 * height / 2;
  real ch3 = ELLIPSE_CTRL3 * height / 2, ch4 = ELLIPSE_CTRL4 * height / 2;

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, x1, center->y);
  gnome_print_curveto(renderer->ctx,
		      x1,              center->y - ch1,
		      center->x - cw4, center->y - ch2,
		      center->x - cw3, center->y - ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw2, center->y - ch4,
		      center->x - cw1, y1,
		      center->x,       y1);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw1, y1,
		      center->x + cw2, center->y - ch4,
		      center->x + cw3, center->y - ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw4, center->y - ch2,
		      x2,              center->y - ch1,
		      x2,              center->y);
  gnome_print_curveto(renderer->ctx,
		      x2,              center->y + ch1,
		      center->x + cw4, center->y + ch2,
		      center->x + cw3, center->y + ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x + cw2, center->y + ch4,
		      center->x + cw1, y2,
		      center->x,       y2);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw1, y2,
		      center->x - cw2, center->y + ch4,
		      center->x - cw3, center->y + ch3);
  gnome_print_curveto(renderer->ctx,
		      center->x - cw4, center->y + ch2,
		      x1,              center->y + ch1,
		      x1,              center->y);
  gnome_print_fill(renderer->ctx);
}

static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int i;

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, points[0].p1.x, points[0].p1.y);
  
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      gnome_print_lineto(renderer->ctx, points[i].p1.x, points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      gnome_print_curveto(renderer->ctx,
			  points[i].p1.x, points[i].p1.y,
			  points[i].p2.x, points[i].p2.y,
			  points[i].p3.x, points[i].p3.y);
      break;
    }

  gnome_print_stroke(renderer->ctx);
}

static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int i;

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, points[0].p1.x, points[0].p1.y);
  
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      break;
    case BEZ_LINE_TO:
      gnome_print_lineto(renderer->ctx, points[i].p1.x, points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      gnome_print_curveto(renderer->ctx,
			  points[i].p1.x, points[i].p1.y,
			  points[i].p2.x, points[i].p2.y,
			  points[i].p3.x, points[i].p3.y);
      break;
    }

  gnome_print_fill(renderer->ctx);
}

static void
draw_string(DiaRenderer *self,
	    const gchar *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  PangoLayout *layout;
  int pango_width, pango_height;
  PangoAttrList *list;
  PangoAttribute *attr;
  int len; 
  real width, height;

  if (!text || strlen(text) < 1)
    return;

  len = strlen(text);
  layout = gnome_print_pango_create_layout (renderer->ctx);
  pango_layout_set_text (layout, text, len);

  list = pango_attr_list_new ();
  attr = pango_attr_font_desc_new (renderer->font->pfd);
  attr->start_index = 0;
  attr->end_index = len;
  pango_attr_list_insert (list, attr); /* eats attr */
  pango_layout_set_attributes (layout, list);
  pango_attr_list_unref (list);
  
  pango_layout_set_indent (layout, 0);
  pango_layout_set_justify (layout, FALSE);
  pango_layout_set_alignment (layout, PANGO_ALIGN_LEFT);
  
  pango_layout_get_size (layout, &pango_width, &pango_height);

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  width = ((double) pango_width / PANGO_SCALE) / magic;
  height = ((double) pango_height / PANGO_SCALE) / magic;
  
  switch (alignment) {
  case ALIGN_LEFT:
    gnome_print_moveto(renderer->ctx, pos->x, pos->y - height);
    break;
  case ALIGN_CENTER:
    gnome_print_moveto(renderer->ctx, pos->x - width / 2, pos->y - height);
    break;
  case ALIGN_RIGHT:
    gnome_print_moveto(renderer->ctx, pos->x - width, pos->y - height);
    break;
  }


  gnome_print_gsave(renderer->ctx);
  gnome_print_scale(renderer->ctx, 1/magic, -1/magic);
  
  gnome_print_pango_layout (renderer->ctx, layout);
  g_object_unref (layout);

  gnome_print_grestore(renderer->ctx);
}

static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (self);
  int w = dia_image_width(image);
  int h = dia_image_height(image);
  int rs = dia_image_rowstride(image);

  gnome_print_gsave(renderer->ctx);

  /* into unit square (0,0 - 1,1) in current coordinate system. */
  gnome_print_translate(renderer->ctx, point->x, point->y + height);
  gnome_print_scale(renderer->ctx, width, -height);
  if (dia_image_rgba_data (image))
    gnome_print_rgbaimage(renderer->ctx, dia_image_rgba_data (image), w, h, rs);
  else
    gnome_print_rgbimage(renderer->ctx, dia_image_rgb_data (image), w, h, rs);
  gnome_print_grestore(renderer->ctx);
}

/* 'advanced' gobject stuff */
/* allow to get and set properties */
enum {
  PROP_0,
  PROP_CONFIG,
  PROP_CONTEXT
};

static void
renderer_set_property (GObject      *object,
			     guint         prop_id,
			     const GValue *value,
			     GParamSpec   *pspec)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (object);

  switch (prop_id)
    {
    case PROP_CONFIG:
      renderer->config = g_value_get_object (value);
      break;
    case PROP_CONTEXT:
      renderer->ctx = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
renderer_get_property (GObject    *object,
			     guint       prop_id,
			     GValue     *value,
			     GParamSpec *pspec)
{
  DiaGnomePrintRenderer *renderer = DIA_GNOME_PRINT_RENDERER (object);

  switch (prop_id)
    {
    case PROP_CONFIG:
      g_value_set_object (value, renderer->config);
      break;
    case PROP_CONTEXT:
      g_value_set_object (value, renderer->ctx);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

/* common gobject stuff */ 
static void dia_gnome_print_renderer_init (DiaGnomePrintRenderer *renderer, void *p);
static void dia_gnome_print_renderer_class_init (DiaGnomePrintRendererClass *klass);

static gpointer parent_class = NULL;

GType
dia_gnome_print_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaGnomePrintRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_gnome_print_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaGnomePrintRenderer),
        0,              /* n_preallocs */
	  (GInstanceInitFunc)dia_gnome_print_renderer_init /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaGnomePrintRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
dia_gnome_print_renderer_init (DiaGnomePrintRenderer *renderer, void *p)
{
  renderer->ctx = NULL;
  renderer->font = NULL;

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
}

static void
dia_gnome_print_renderer_finalize (GObject *object)
{
  DiaGnomePrintRenderer *dia_gnome_print_renderer = DIA_GNOME_PRINT_RENDERER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_gnome_print_renderer_class_init (DiaGnomePrintRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = dia_gnome_print_renderer_finalize;
  gobject_class->set_property = renderer_set_property;
  gobject_class->get_property = renderer_get_property;

  /* install properties */
  g_object_class_install_property (gobject_class,
                                   PROP_CONFIG,
                                   g_param_spec_object ("config",
                                                        P_("GNOME Print config to be used"),
                                                        P_("FIXME: not sure if this makes any sense"),
                                                        GNOME_TYPE_PRINT_CONFIG,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));
  g_object_class_install_property (gobject_class,
                                   PROP_CONTEXT,
                                   g_param_spec_object ("context",
                                                        P_("GNOME Print contex to be used"),
                                                        P_("The context does determine the output file format"),
                                                        GNOME_TYPE_PRINT_CONTEXT,
                                                        G_PARAM_READABLE | G_PARAM_WRITABLE));


  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->set_font  = set_font;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->fill_bezier   = fill_bezier;

  /* highest level functions */
#if 0 /* maybe these should be implemented by pathes, but why not in diarenderer.c ? */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->fill_rounded_rect = fill_rounded_rect;
#endif
}
