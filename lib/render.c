/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2002 Lars Clausen
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

/* This file defines the abstract superclass for renderers.  It defines 
 * a bunch of functions to render higher-level structures with lower-level
 * primitives.
 */

#include "config.h"

#include "render.h"

static void begin_render(Renderer *renderer);
static void end_render(Renderer *renderer);
static void set_linewidth(Renderer *renderer, real linewidth);
static void set_linecaps(Renderer *renderer, LineCaps mode);
static void set_linejoin(Renderer *renderer, LineJoin mode);
static void set_linestyle(Renderer *renderer, LineStyle mode);
static void set_dashlength(Renderer *renderer, real length);
static void set_fillstyle(Renderer *renderer, FillStyle mode);
static void set_font(Renderer *renderer, DiaFont *font, real height);
static void draw_line(Renderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(Renderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(Renderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(Renderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(Renderer *renderer,
			const utfchar *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(Renderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);


RenderOps AbstractRenderOps = {
  begin_render,
  end_render,
  
  /* Line attributes: */
  set_linewidth,
  set_linecaps,
  set_linejoin,
  set_linestyle,
  set_dashlength,
  /* Fill attributes: */
  set_fillstyle,
  /* DiaFont stuff: */
  set_font,
  
  /* Lines: */
  draw_line,
  draw_polyline,

  /* Polygons: */
  draw_polygon,
  fill_polygon,

  /* Rectangles: */
  draw_rect,
  fill_rect,
  
  /* Arcs: */
  draw_arc,
  fill_arc,
  
  /* Ellipses: */
  draw_ellipse,
  fill_ellipse,

  /* Bezier curves: */
  draw_bezier,
  fill_bezier,
  
  /* Text: */
  draw_string,

  /* Images: */
  draw_image
};

void
inherit_renderer(RenderOps *child_ops) {
  if (child_ops->begin_render != NULL)
    child_ops->begin_render = AbstractRenderOps.begin_render;
  if (child_ops->end_render != NULL)
    child_ops->end_render = AbstractRenderOps.end_render;
  
  /* Line attributes: */
  if (child_ops->set_linewidth != NULL)
    child_ops->set_linewidth = AbstractRenderOps.set_linewidth;
  if (child_ops->set_linecaps != NULL)
    child_ops->set_linecaps = AbstractRenderOps.set_linecaps;
  if (child_ops->set_linejoin != NULL)
    child_ops->set_linejoin = AbstractRenderOps.set_linejoin;
  if (child_ops->set_linestyle != NULL)
    child_ops->set_linestyle = AbstractRenderOps.set_linestyle;
  if (child_ops->set_dashlength != NULL)
    child_ops->set_dashlength = AbstractRenderOps.set_dashlength;
  /* Fill attributes: */
  if (child_ops->set_fillstyle != NULL)
    child_ops->set_fillstyle = AbstractRenderOps.set_fillstyle;
  /* DiaFont stuff: */
  if (child_ops->set_font != NULL)
    child_ops->set_font = AbstractRenderOps.set_font;
  
  /* Lines: */
  if (child_ops->draw_line != NULL)
    child_ops->draw_line = AbstractRenderOps.draw_line;
  if (child_ops->draw_polyline != NULL)
    child_ops->draw_polyline = AbstractRenderOps.draw_polyline;

  /* Polygons: */
  if (child_ops->draw_polygon != NULL)
    child_ops->draw_polygon = AbstractRenderOps.draw_polygon;
  if (child_ops->fill_polygon != NULL)
    child_ops->fill_polygon = AbstractRenderOps.fill_polygon;

  /* Rectangles: */
  if (child_ops->draw_rect != NULL)
    child_ops->draw_rect = AbstractRenderOps.draw_rect;
  if (child_ops->fill_rect != NULL)
    child_ops->fill_rect = AbstractRenderOps.fill_rect;
  
  /* Arcs: */
  if (child_ops->draw_arc != NULL)
    child_ops->draw_arc = AbstractRenderOps.draw_arc;
  if (child_ops->fill_arc != NULL)
    child_ops->fill_arc = AbstractRenderOps.fill_arc;
  
  /* Ellipses: */
  if (child_ops->draw_ellipse != NULL)
    child_ops->draw_ellipse = AbstractRenderOps.draw_ellipse;
  if (child_ops->fill_ellipse != NULL)
    child_ops->fill_ellipse = AbstractRenderOps.fill_ellipse;

  /* Bezier curves: */
  if (child_ops->draw_bezier != NULL)
    child_ops->draw_bezier = AbstractRenderOps.draw_bezier;
  if (child_ops->fill_bezier != NULL)
    child_ops->fill_bezier = AbstractRenderOps.fill_bezier;
  
  /* Text: */
  if (child_ops->draw_string != NULL)
    child_ops->draw_string = AbstractRenderOps.draw_string;

  /* Images: */
  if (child_ops->draw_image != NULL)
    child_ops->draw_image = AbstractRenderOps.draw_image;
}

static void begin_render(Renderer *renderer){}
static void end_render(Renderer *renderer){}
static void set_linewidth(Renderer *renderer, real linewidth){}
static void set_linecaps(Renderer *renderer, LineCaps mode){}
static void set_linejoin(Renderer *renderer, LineJoin mode){}
static void set_linestyle(Renderer *renderer, LineStyle mode){}
static void set_dashlength(Renderer *renderer, real length){}
static void set_fillstyle(Renderer *renderer, FillStyle mode){}
static void set_font(Renderer *renderer, DiaFont *font, real height){}
static void draw_line(Renderer *renderer, 
		      Point *start, Point *end, 
		      Color *line_color){}
static void draw_polyline(Renderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color){}/* Call draw_line */
static void draw_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color){}/* Call draw_line */
static void fill_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color){}
static void draw_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color){}/* Call draw_line */
static void fill_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color){}
static void draw_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color){}
static void fill_arc(Renderer *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color){}
static void draw_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color){}
static void fill_ellipse(Renderer *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color){}
static void draw_bezier(Renderer *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color){}
static void fill_bezier(Renderer *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color){}
static void draw_string(Renderer *renderer,
			const utfchar *text,
			Point *pos, Alignment alignment,
			Color *color){}
static void draw_image(Renderer *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image){}
