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
#include <string.h>
#include <time.h>
#include <math.h>
#include <unistd.h>

#include "config.h"
#include "intl.h"
#include "render_gnomeprint.h"
#include "message.h"
#include "diagramdata.h"

static void begin_render(RendererGPrint *renderer, DiagramData *data);
static void end_render(RendererGPrint *renderer);
static void set_linewidth(RendererGPrint *renderer, real linewidth);
static void set_linecaps(RendererGPrint *renderer, LineCaps mode);
static void set_linejoin(RendererGPrint *renderer, LineJoin mode);
static void set_linestyle(RendererGPrint *renderer, LineStyle mode);
static void set_dashlength(RendererGPrint *renderer, real length);
static void set_fillstyle(RendererGPrint *renderer, FillStyle mode);
static void set_font(RendererGPrint *renderer, Font *font, real height);
static void draw_line(RendererGPrint *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererGPrint *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererGPrint *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererGPrint *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererGPrint *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererGPrint *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererGPrint *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererGPrint *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererGPrint *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererGPrint *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererGPrint *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererGPrint *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererGPrint *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererGPrint *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static RenderOps GPrintRenderOps = {
  (BeginRenderFunc) begin_render,
  (EndRenderFunc) end_render,

  (SetLineWidthFunc) set_linewidth,
  (SetLineCapsFunc) set_linecaps,
  (SetLineJoinFunc) set_linejoin,
  (SetLineStyleFunc) set_linestyle,
  (SetDashLengthFunc) set_dashlength,
  (SetFillStyleFunc) set_fillstyle,
  (SetFontFunc) set_font,
  
  (DrawLineFunc) draw_line,
  (DrawPolyLineFunc) draw_polyline,
  
  (DrawPolygonFunc) draw_polygon,
  (FillPolygonFunc) fill_polygon,

  (DrawRectangleFunc) draw_rect,
  (FillRectangleFunc) fill_rect,

  (DrawArcFunc) draw_arc,
  (FillArcFunc) fill_arc,

  (DrawEllipseFunc) draw_ellipse,
  (FillEllipseFunc) fill_ellipse,

  (DrawBezierFunc) draw_bezier,
  (FillBezierFunc) fill_bezier,

  (DrawStringFunc) draw_string,

  (DrawImageFunc) draw_image,
};

RendererGPrint *
new_gnomeprint_renderer(Diagram *dia, GnomePrintContext *ctx)
{
  RendererGPrint *renderer;
  time_t time_now;
  Rectangle *extent;
  char *name;
 
  renderer = g_new(RendererGPrint, 1);
  renderer->renderer.ops = &GPrintRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->ctx = ctx;
  renderer->font = NULL;

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;
  
  return renderer;
}

static void
begin_render(RendererGPrint *renderer, DiagramData *data)
{
  /*gnome_print_gsave(renderer->ctx);
    gnome_print_scale(renderer->ctx, 28.346, -28.346);
    gnome_print_translate(renderer->ctx, -left, -bottom);*/
}

static void
end_render(RendererGPrint *renderer)
{
  if (renderer->font)
    gtk_object_unref(GTK_OBJECT(renderer->font));
  renderer->font = NULL;
  /*gnome_print_showpage(renderer->ctx);
    gnome_print_grestore(renderer->ctx);*/
}

static void
set_linewidth(RendererGPrint *renderer, real linewidth)
{  /* 0 == hairline **/

  gnome_print_setlinewidth(renderer->ctx, linewidth);
}

static void
set_linecaps(RendererGPrint *renderer, LineCaps mode)
{
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
set_linejoin(RendererGPrint *renderer, LineJoin mode)
{
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
set_linestyle(RendererGPrint *renderer, LineStyle mode)
{
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
set_dashlength(RendererGPrint *renderer, real length)
{  /* dot = 20% of len */
  if (length<0.001)
    length = 0.001;
  
  renderer->dash_length = length;
  renderer->dot_length = length*0.2;
  
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererGPrint *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("eps_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererGPrint *renderer, Font *font, real height)
{
  GnomeFont *gfont;

  if (renderer->font)
    gtk_object_unref(GTK_OBJECT(renderer->font));
  gfont = gnome_font_new(font_get_psfontname(font), height);
  if (!gfont)
    gfont = gnome_font_new("Courier", height);
  gnome_print_setfont(renderer->ctx, gfont);
  renderer->font = gfont;
}

static void
draw_line(RendererGPrint *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
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
draw_polyline(RendererGPrint *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
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
draw_polygon(RendererGPrint *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
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
fill_polygon(RendererGPrint *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
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
draw_rect(RendererGPrint *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
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
fill_rect(RendererGPrint *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  gnome_print_newpath(renderer->ctx);
  gnome_print_moveto(renderer->ctx, ul_corner->x, ul_corner->y);
  gnome_print_lineto(renderer->ctx, ul_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, lr_corner->y);
  gnome_print_lineto(renderer->ctx, lr_corner->x, ul_corner->y);
  gnome_print_fill(renderer->ctx);
}

static void
draw_arc(RendererGPrint *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  /*fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f %f %f ellipse s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 ); */
}

static void
fill_arc(RendererGPrint *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  /*fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f m %f %f %f %f %f %f ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0,
	  (double) 360.0 - angle2, (double) 360.0 - angle1 ); */
}

static void
draw_ellipse(RendererGPrint *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  /*fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse cp s\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 ); */
}

static void
fill_ellipse(RendererGPrint *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  /*fprintf(renderer->file, "%f %f %f srgb\n",
	  (double) color->red, (double) color->green, (double) color->blue);

  fprintf(renderer->file, "n %f %f %f %f 0 360 ellipse f\n",
	  (double) center->x, (double) center->y,
	  (double) width/2.0, (double) height/2.0 ); */
}

static void
draw_bezier(RendererGPrint *renderer, 
	    BezPoint *points,
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
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
fill_bezier(RendererGPrint *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints,
	    Color *color)
{
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
draw_string(RendererGPrint *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  const char *str;
  int len;

  /* TODO: Use latin-1 encoding */

  gnome_print_setrgbcolor(renderer->ctx, (double)color->red,
			  (double)color->green, (double)color->blue);

  switch (alignment) {
  case ALIGN_LEFT:
    gnome_print_moveto(renderer->ctx, pos->x, pos->y);
    break;
  case ALIGN_CENTER:
    gnome_print_moveto(renderer->ctx, pos->x -
		       gnome_font_get_width_string(renderer->font, text) / 2,
		       pos->y);
    break;
  case ALIGN_RIGHT:
    gnome_print_moveto(renderer->ctx, pos->x -
		       gnome_font_get_width_string(renderer->font, text),
		       pos->y);
    break;
  }

  gnome_print_gsave(renderer->ctx);
  gnome_print_scale(renderer->ctx, 1, -1);
  gnome_print_show(renderer->ctx, text);
  gnome_print_grestore(renderer->ctx);
}

static void
draw_image(RendererGPrint *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  int img_width, img_height;
  int v;
  int                 x, y;
  real ratio;
  guint8 *rgb_data;

  img_width = dia_image_width(image);
  img_height = dia_image_height(image);

  rgb_data = dia_image_rgb_data(image);
  
  ratio = height/width;

  gnome_print_gsave(renderer->ctx);
  if (1) { /* Color output */
    gnome_print_translate(renderer->ctx, point->x, point->y);
    gnome_print_scale(renderer->ctx, width, height);
    gnome_print_rgbimage(renderer->ctx, rgb_data, img_width, img_height,
			 img_width * 3);
  }
  gnome_print_grestore(renderer->ctx);
}

