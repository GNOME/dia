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
static void draw_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real rounding);
static void fill_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real rounding);
static void draw_bezier_with_arrows(Renderer *renderer, 
				    BezPoint *points,
				    int num_points,
				    real line_width,
				    Color *color,
				    Arrow *start_arrow,
				    Arrow *end_arrow);

static RenderOps AbstractRenderOps = {
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
  draw_image,

  /* Later additions, down here for binary combatability */
  draw_rounded_rect,
  fill_rounded_rect,

  draw_bezier_with_arrows,
};

RenderOps *
create_renderops_table() {
  RenderOps *table = g_new(RenderOps, 1);
  *table = AbstractRenderOps;
  return table;
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
/* Interim draw_polyline.  Doesn't draw nice corners. */
static void draw_polyline(Renderer *renderer, 
			  Point *points, int num_points, 
			  Color *line_color) {
  int i;
  for (i = 0; i < num_points-1; i++) {
    renderer->ops->draw_line(renderer, &points[i], &points[i+1], line_color);
  }
}
/* Interim draw_polyline.  Doesn't draw nice corners. */
static void draw_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color) {
  int i;
  for (i = 0; i < num_points-1; i++) {
    renderer->ops->draw_line(renderer, &points[i], &points[i+1], line_color);
  }
  renderer->ops->draw_line(renderer, &points[i], &points[0], line_color);
}
static void fill_polygon(Renderer *renderer, 
			 Point *points, int num_points, 
			 Color *line_color){}
/* Simple draw_rect.  Draws, but loses info about rectangularity. */
static void draw_rect(Renderer *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color) {
  Point ur_corner, ll_corner;
  ur_corner.x = lr_corner->x;
  ur_corner.y = ul_corner->y;
  ur_corner.x = ul_corner->x;
  ur_corner.y = lr_corner->y;
  renderer->ops->draw_line(renderer, ul_corner, &ur_corner, color);
  renderer->ops->draw_line(renderer, &ur_corner, lr_corner, color);
  renderer->ops->draw_line(renderer, lr_corner, &ll_corner, color);
  renderer->ops->draw_line(renderer, &ll_corner, ul_corner, color);
}
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
static void draw_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius) {
  Point start, end, center;

  radius = MIN(radius, (lr_corner->x-ul_corner->x)/2);
  radius = MIN(radius, (lr_corner->y-ul_corner->y)/2);
  start.x = center.x = ul_corner->x+radius;
  end.x = lr_corner->x-radius;
  start.y = end.y = ul_corner->y;
  renderer->ops->draw_line(renderer, &start, &end, color);
  start.y = end.y = lr_corner->y;
  renderer->ops->draw_line(renderer, &start, &end, color);

  center.y = ul_corner->y+radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  90.0, 180.0, color);
  center.x = end.x;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  0.0, 90.0, color);

  start.y = ul_corner->y+radius;
  start.x = end.x = ul_corner->x;
  end.y = center.y = lr_corner->y-radius;
  renderer->ops->draw_line(renderer, &start, &end, color);
  start.x = end.x = lr_corner->x;
  renderer->ops->draw_line(renderer, &start, &end, color);

  center.y = lr_corner->y-radius;
  center.x = ul_corner->x+radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  180.0, 270.0, color);
  center.x = lr_corner->x-radius;
  renderer->ops->draw_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  270.0, 360.0, color);
}
static void fill_rounded_rect(Renderer *renderer, 
			      Point *ul_corner, Point *lr_corner,
			      Color *color, real radius) {
  Point start, end, center;

  radius = MIN(radius, (lr_corner->x-ul_corner->x)/2);
  radius = MIN(radius, (lr_corner->y-ul_corner->y)/2);
  start.x = center.x = ul_corner->x+radius;
  end.x = lr_corner->x-radius;
  start.y = ul_corner->y;
  end.y = lr_corner->y;
  renderer->ops->fill_rect(renderer, &start, &end, color);

  center.y = ul_corner->y+radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  90.0, 180.0, color);
  center.x = end.x;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  0.0, 90.0, color);

  start.x = ul_corner->x;
  start.y = ul_corner->y+radius;
  end.x = lr_corner->x;
  end.y = center.y = lr_corner->y-radius;
  renderer->ops->fill_rect(renderer, &start, &end, color);

  center.y = lr_corner->y-radius;
  center.x = ul_corner->x+radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  180.0, 270.0, color);
  center.x = lr_corner->x-radius;
  renderer->ops->fill_arc(renderer, &center, 
			  2.0*radius, 2.0*radius,
			  270.0, 360.0, color);
}

static void
draw_bezier_with_arrows(Renderer *renderer, 
			BezPoint *points,
			int num_points,
			real line_width,
			Color *color,
			Arrow *start_arrow,
			Arrow *end_arrow)
{
  Point startpoint, endpoint;

  startpoint = points[0].p1;
  endpoint = points[num_points-1].p3;

  if (start_arrow->type != ARROW_NONE) {
    Point move;
    Point arrow_head;
    calculate_arrow_point(&points[0].p1, &points[1].p1, &move,
			  start_arrow->length, start_arrow->width,
			  line_width);
    point_sub(&points[0].p1, &move);
    arrow_head = points[0].p1;
    point_sub(&points[0].p1, &move);
    arrow_draw(renderer, start_arrow->type,
	       &arrow_head, &points[1].p1,
	       start_arrow->length, start_arrow->width,
	       line_width,
	       &color_black, &color_white);
    renderer->ops->draw_line(renderer, &arrow_head, &points[0].p1,
			     &color_white);
    if (0) {
      Point line_start = points[0].p1;
      points[0].p1 = arrow_head;
      point_normalize(&move);
      point_scale(&move, start_arrow->length);
      point_sub(&points[0].p1, &move);
      renderer->ops->draw_line(renderer, &line_start, &points[0].p1, color);
    }
  }
  if (end_arrow->type != ARROW_NONE) {
  }
  renderer->ops->draw_bezier(renderer, points, num_points, color);

  points[0].p1 = startpoint;
  points[num_points-1].p3 = endpoint;
  
}
