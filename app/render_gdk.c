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

#define PANGO_ENABLE_ENGINE
#include "pango/pango-engine.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>

#include "render_gdk.h"
#include "message.h"

#include <pango/pango.h>

static void begin_render(RendererGdk *renderer, DiagramData *data);
static void end_render(RendererGdk *renderer);
static void set_linewidth(RendererGdk *renderer, real linewidth);
static void set_linecaps(RendererGdk *renderer, LineCaps mode);
static void set_linejoin(RendererGdk *renderer, LineJoin mode);
static void set_linestyle(RendererGdk *renderer, LineStyle mode);
static void set_dashlength(RendererGdk *renderer, real length);
static void set_fillstyle(RendererGdk *renderer, FillStyle mode);
static void set_font(RendererGdk *renderer, DiaFont *font, real height);
static void draw_line(RendererGdk *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererGdk *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererGdk *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererGdk *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererGdk *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererGdk *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererGdk *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererGdk *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererGdk *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererGdk *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererGdk *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererGdk *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererGdk *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererGdk *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static real get_text_width(RendererGdk *renderer,
			   const char *text, int length);

static void clip_region_clear(RendererGdk *renderer);
static void clip_region_add_rect(RendererGdk *renderer,
				 Rectangle *rect);

static void draw_pixel_line(RendererGdk *renderer,
			    int x1, int y1,
			    int x2, int y2,
			    Color *color);
static void draw_pixel_rect(RendererGdk *renderer,
				 int x, int y,
				 int width, int height,
				 Color *color);
static void fill_pixel_rect(RendererGdk *renderer,
				 int x, int y,
				 int width, int height,
				 Color *color);

static void init_gdk_renderops();

static InteractiveRenderOps GdkInteractiveRenderOps = {
  (GetTextWidthFunc) get_text_width,

  (ClipRegionClearFunc) clip_region_clear,
  (ClipRegionAddRectangleFunc) clip_region_add_rect,

  (DrawPixelLineFunc) draw_pixel_line,
  (DrawPixelRectangleFunc) draw_pixel_rect,
  (FillPixelRectangleFunc) fill_pixel_rect,
};

static RenderOps *GdkRenderOps;

static void
init_gdk_renderops() {
  GdkRenderOps = create_renderops_table();

  GdkRenderOps->begin_render = (BeginRenderFunc) begin_render;
  GdkRenderOps->end_render = (EndRenderFunc) end_render;

  GdkRenderOps->set_linewidth = (SetLineWidthFunc) set_linewidth;
  GdkRenderOps->set_linecaps = (SetLineCapsFunc) set_linecaps;
  GdkRenderOps->set_linejoin = (SetLineJoinFunc) set_linejoin;
  GdkRenderOps->set_linestyle = (SetLineStyleFunc) set_linestyle;
  GdkRenderOps->set_dashlength = (SetDashLengthFunc) set_dashlength;
  GdkRenderOps->set_fillstyle = (SetFillStyleFunc) set_fillstyle;
  GdkRenderOps->set_font = (SetFontFunc) set_font;
  
  GdkRenderOps->draw_line = (DrawLineFunc) draw_line;
  GdkRenderOps->draw_polyline = (DrawPolyLineFunc) draw_polyline;
  
  GdkRenderOps->draw_polygon = (DrawPolygonFunc) draw_polygon;
  GdkRenderOps->fill_polygon = (FillPolygonFunc) fill_polygon;

  GdkRenderOps->draw_rect = (DrawRectangleFunc) draw_rect;
  GdkRenderOps->fill_rect = (FillRectangleFunc) fill_rect;

  GdkRenderOps->draw_arc = (DrawArcFunc) draw_arc;
  GdkRenderOps->fill_arc = (FillArcFunc) fill_arc;

  GdkRenderOps->draw_ellipse = (DrawEllipseFunc) draw_ellipse;
  GdkRenderOps->fill_ellipse = (FillEllipseFunc) fill_ellipse;

  GdkRenderOps->draw_bezier = (DrawBezierFunc) draw_bezier;
  GdkRenderOps->fill_bezier = (FillBezierFunc) fill_bezier;

  GdkRenderOps->draw_string = (DrawStringFunc) draw_string;

  GdkRenderOps->draw_image = (DrawImageFunc) draw_image;
}

RendererGdk *
new_gdk_renderer(DDisplay *ddisp)
{
  RendererGdk *renderer;

  if (GdkRenderOps == NULL)
    init_gdk_renderops();

  renderer = g_new(RendererGdk, 1);
  renderer->renderer.ops = GdkRenderOps;
  renderer->renderer.is_interactive = 1;
  renderer->renderer.interactive_ops = &GdkInteractiveRenderOps;
  renderer->ddisp = ddisp;
  renderer->render_gc = NULL;

  renderer->pixmap = NULL;
  renderer->renderer.pixel_width = 0;
  renderer->renderer.pixel_height = 0;
  renderer->clip_region = NULL;

  renderer->line_width = 1;
  renderer->line_style = GDK_LINE_SOLID;
  renderer->cap_style = GDK_CAP_BUTT;
  renderer->join_style = GDK_JOIN_MITER;
  
  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_length = 10;
  renderer->dot_length = 2;
  renderer->font = NULL;
  
  return renderer;
}

void
destroy_gdk_renderer(RendererGdk *renderer)
{
  if (renderer->font)
    dia_font_unref(renderer->font);

  if (renderer->pixmap != NULL)
    gdk_pixmap_unref(renderer->pixmap);

  if (renderer->render_gc != NULL)
    gdk_gc_unref(renderer->render_gc);

  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  g_free(renderer);
}

void
gdk_renderer_set_size(RendererGdk *renderer, GdkWindow *window,
		      int width, int height)
{
  
  if (renderer->pixmap != NULL) {
    gdk_pixmap_unref(renderer->pixmap);
  }

  renderer->pixmap = gdk_pixmap_new(window,  width, height, -1);
  renderer->renderer.pixel_width = width;
  renderer->renderer.pixel_height = height;

  if (renderer->render_gc == NULL) {
    renderer->render_gc = gdk_gc_new(renderer->pixmap);

    gdk_gc_set_line_attributes(renderer->render_gc,
			       renderer->line_width,
			       renderer->line_style,
			       renderer->cap_style,
			       renderer->join_style);
  }
}

extern void
renderer_gdk_copy_to_window(RendererGdk *renderer, GdkWindow *window,
			    int x, int y, int width, int height)
{
  static GdkGC *copy_gc = NULL;

  if (copy_gc == NULL) {
    copy_gc = gdk_gc_new(window);
  }
  
  gdk_draw_pixmap(window,
		  copy_gc,
		  renderer->pixmap,
		  x, y,
		  x, y,
		  width, height);
}

static void
begin_render(RendererGdk *renderer, DiagramData *data)
{
}

static void
end_render(RendererGdk *renderer)
{
}



static void
set_linewidth(RendererGdk *renderer, real linewidth)
{  /* 0 == hairline **/
  renderer->line_width =
    ddisplay_transform_length(renderer->ddisp, linewidth);

  if (renderer->line_width<=0)
    renderer->line_width = 1; /* Minimum 1 pixel. */

  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
set_linecaps(RendererGdk *renderer, LineCaps mode)
{
  switch(mode) {
  case LINECAPS_BUTT:
    renderer->cap_style = GDK_CAP_BUTT;
    break;
  case LINECAPS_ROUND:
    renderer->cap_style = GDK_CAP_ROUND;
    break;
  case LINECAPS_PROJECTING:
    renderer->cap_style = GDK_CAP_PROJECTING;
    break;
  }
 
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
set_linejoin(RendererGdk *renderer, LineJoin mode)
{
  switch(mode) {
  case LINEJOIN_MITER:
    renderer->cap_style = GDK_JOIN_MITER;
    break;
  case LINEJOIN_ROUND:
    renderer->cap_style = GDK_JOIN_ROUND;
    break;
  case LINEJOIN_BEVEL:
    renderer->cap_style = GDK_JOIN_BEVEL;
    break;
  }
 
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
set_linestyle(RendererGdk *renderer, LineStyle mode)
{
  char dash_list[6];
  int hole_width;
  
  renderer->saved_line_style = mode;
  switch(mode) {
  case LINESTYLE_SOLID:
    renderer->line_style = GDK_LINE_SOLID;
    break;
  case LINESTYLE_DASHED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = renderer->dash_length;
    gdk_gc_set_dashes(renderer->render_gc, 0, dash_list, 2);
    break;
  case LINESTYLE_DASH_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    hole_width = (renderer->dash_length - renderer->dot_length) / 2;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    gdk_gc_set_dashes(renderer->render_gc, 0, dash_list, 4);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    hole_width = (renderer->dash_length - 2*renderer->dot_length) / 3;
    if (hole_width==0)
      hole_width = 1;
    dash_list[0] = renderer->dash_length;
    dash_list[1] = hole_width;
    dash_list[2] = renderer->dot_length;
    dash_list[3] = hole_width;
    dash_list[4] = renderer->dot_length;
    dash_list[5] = hole_width;
    gdk_gc_set_dashes(renderer->render_gc, 0, dash_list, 6);
    break;
  case LINESTYLE_DOTTED:
    renderer->line_style = GDK_LINE_ON_OFF_DASH;
    dash_list[0] = renderer->dot_length;
    dash_list[1] = renderer->dot_length;
    gdk_gc_set_dashes(renderer->render_gc, 0, dash_list, 2);
    break;
  }
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
set_dashlength(RendererGdk *renderer, real length)
{  /* dot = 10% of len */
  real ddisp_len;

  ddisp_len =
    ddisplay_transform_length(renderer->ddisp, length);
  
  renderer->dash_length = (int)floor(ddisp_len+0.5);
  renderer->dot_length = (int)floor(ddisp_len*0.1+0.5);
  
  if (renderer->dash_length<=0)
    renderer->dash_length = 1;
  if (renderer->dash_length>255)
    renderer->dash_length = 255;
  if (renderer->dot_length<=0)
    renderer->dot_length = 1;
  if (renderer->dot_length>255)
    renderer->dot_length = 255;
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererGdk *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("gdk_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererGdk *renderer, DiaFont *font, real height)
{
  renderer->font_height = height;
  
  if (renderer->font)
    dia_font_unref(renderer->font);
  renderer->font = dia_font_ref(font);
}

static void
draw_line(RendererGdk *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  int x1,y1,x2,y2;
  
  ddisplay_transform_coords(ddisp, start->x, start->y, &x1, &y1);
  ddisplay_transform_coords(ddisp, end->x, end->y, &x2, &y2);
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_line(renderer->pixmap, gc,
		x1, y1,	x2, y2);
}

static void
draw_polyline(RendererGdk *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i,x,y;

  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords(ddisp, points[i].x, points[i].y, &x, &y);
    gdk_points[i].x = x;
    gdk_points[i].y = y;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_lines(renderer->pixmap, gc, gdk_points, num_points);
  g_free(gdk_points);
}

static void
draw_polygon(RendererGdk *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i,x,y;
  
  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords(ddisp, points[i].x, points[i].y, &x, &y);
    gdk_points[i].x = x;
    gdk_points[i].y = y;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_polygon(renderer->pixmap, gc, FALSE, gdk_points, num_points);
  g_free(gdk_points);
}

static void
fill_polygon(RendererGdk *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i,x,y;
  
  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords(ddisp, points[i].x, points[i].y, &x, &y);
    gdk_points[i].x = x;
    gdk_points[i].y = y;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_polygon(renderer->pixmap, gc, TRUE, gdk_points, num_points);
  g_free(gdk_points);
}

static void
draw_rect(RendererGdk *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;
    
  ddisplay_transform_coords(ddisp, ul_corner->x, ul_corner->y,
			    &left, &top);
  ddisplay_transform_coords(ddisp, lr_corner->x, lr_corner->y,
			    &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap,
		      gc, FALSE,
		      left, top,
		      right-left,
		      bottom-top);
}

static void
fill_rect(RendererGdk *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;
    
  ddisplay_transform_coords(ddisp, ul_corner->x, ul_corner->y,
			    &left, &top);
  ddisplay_transform_coords(ddisp, lr_corner->x, lr_corner->y,
			    &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;
  
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap,
		      gc, TRUE,
		      left, top,
		      right-left,
		      bottom-top);
}

static void
draw_arc(RendererGdk *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, left, bottom, right;
  real dangle;
  
  ddisplay_transform_coords(ddisp,
			    center->x - width/2, center->y - height/2,
			    &left, &top);
  ddisplay_transform_coords(ddisp,
			    center->x + width/2, center->y + height/2,
			    &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  dangle = angle2-angle1;
  if (dangle<0)
    dangle += 360.0;
  
  gdk_draw_arc(renderer->pixmap,
	       gc, FALSE,
	       left, top, right-left, bottom-top,
	       (int) (angle1*64.0), (int) (dangle*64.0));
}

static void
fill_arc(RendererGdk *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, left, bottom, right;
  real dangle;
  
  ddisplay_transform_coords(ddisp,
			    center->x - width/2, center->y - height/2,
			    &left, &top);
  ddisplay_transform_coords(ddisp,
			    center->x + width/2, center->y + height/2,
			    &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);
  
  dangle = angle2-angle1;
  if (dangle<0)
    dangle += 360.0;
  
  gdk_draw_arc(renderer->pixmap,
	       gc, TRUE,
	       left, top, right-left, bottom-top,
	       (int) (angle1*64), (int) (dangle*64));
}

static void
draw_ellipse(RendererGdk *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  draw_arc(renderer, center, width, height, 0.0, 360.0, color); 
}

static void
fill_ellipse(RendererGdk *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  fill_arc(renderer, center, width, height, 0.0, 360.0, color); 
}

struct bezier_curve {
  GdkPoint *gdk_points;
  int numpoints;
  int currpoint;
};

#define BEZIER_SUBDIVIDE_LIMIT 0.03
#define BEZIER_SUBDIVIDE_LIMIT_SQ (BEZIER_SUBDIVIDE_LIMIT*BEZIER_SUBDIVIDE_LIMIT)

static void
bezier_add_point(DDisplay *ddisp,
		 struct bezier_curve *bezier,
		 Point *point)
{
  int x,y;
  
  /* Grow if needed: */
  if (bezier->currpoint == bezier->numpoints) {
    bezier->numpoints += 40;
    bezier->gdk_points = g_realloc(bezier->gdk_points,
				   bezier->numpoints*sizeof(GdkPoint));
  }
  
  ddisplay_transform_coords(ddisp, point->x, point->y, &x, &y);
  
  bezier->gdk_points[bezier->currpoint].x = x;
  bezier->gdk_points[bezier->currpoint].y = y;
  
  bezier->currpoint++;
}

static void
bezier_add_lines(DDisplay *ddisp,
		 Point points[4],
		 struct bezier_curve *bezier)
{
  Point u, v, x, y;
  Point r[4];
  Point s[4];
  Point middle;
  coord delta;
  real v_len_sq;

  /* Check if almost flat: */
  u = points[1];
  point_sub(&u, &points[0]);
  v = points[3];
  point_sub(&v, &points[0]);
  y = v;
  v_len_sq = point_dot(&v,&v);
  if (isnan(v_len_sq)) {
    g_warning("v_len_sq is NaN while calculating bezier curve!");
    return;
  }
  if (v_len_sq < 0.000001)
    v_len_sq = 0.000001;
  point_scale(&y, point_dot(&u,&v)/v_len_sq);
  x = u;
  point_sub(&x,&y);
  delta = ddisplay_transform_length(ddisp, point_dot(&x,&x));
  if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) {
    u = points[2];
    point_sub(&u, &points[3]);
    v = points[0];
    point_sub(&v, &points[3]);
    y = v;
    v_len_sq = point_dot(&v,&v);
    if (v_len_sq < 0.000001)
      v_len_sq = 0.000001;
    point_scale(&y, point_dot(&u,&v)/v_len_sq);
    x = u;
    point_sub(&x,&y);
    delta = ddisplay_transform_length(ddisp, point_dot(&x,&x));
    if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) { /* Almost flat, draw a line */
      bezier_add_point(ddisp, bezier, &points[3]);
      return;
    }
  }
  /* Subdivide into two bezier curves: */

  middle = points[1];
  point_add(&middle, &points[2]);
  point_scale(&middle, 0.5);
  
  r[0] = points[0];
  
  r[1] = points[0];
  point_add(&r[1], &points[1]);
  point_scale(&r[1], 0.5);

  r[2] = r[1];
  point_add(&r[2], &middle);
  point_scale(&r[2], 0.5);

  s[3] = points[3];

  s[2] = points[2];
  point_add(&s[2], &points[3]);
  point_scale(&s[2], 0.5);
  
  s[1] = s[2];
  point_add(&s[1], &middle);
  point_scale(&s[1], 0.5);

  r[3] = r[2];
  point_add(&r[3], &s[1]);
  point_scale(&r[3], 0.5);
  
  s[0] = r[3];

  bezier_add_lines(ddisp, r, bezier);
  bezier_add_lines(ddisp, s, bezier);
}

static void
bezier_add_curve(DDisplay *ddisp,
		 Point points[4],
		 struct bezier_curve *bezier)
{
  /* Is the bezier curve malformed? */
  if ( (distance_point_point(&points[0], &points[1]) < 0.00001) &&
       (distance_point_point(&points[2], &points[3]) < 0.00001) &&
       (distance_point_point(&points[0], &points[3]) < 0.00001)) {
    bezier_add_point(ddisp, bezier, &points[3]);
  }
  
  bezier_add_lines(ddisp, points, bezier);
}


static struct bezier_curve bezier = { NULL, 0, 0 };

static void
draw_bezier(RendererGdk *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdk_color;
  Point curve[4];
  int i;
  
  if (bezier.gdk_points == NULL) {
    bezier.numpoints = 30;
    bezier.gdk_points = g_malloc(bezier.numpoints*sizeof(GdkPoint));
  }

  bezier.currpoint = 0;

  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");
  curve[3] = points[0].p1;
  bezier_add_point(renderer->ddisp, &bezier, &points[0].p1);
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      curve[3] = points[i].p1;
      break;
    case BEZ_LINE_TO:
      bezier_add_point(renderer->ddisp, &bezier, &points[i].p1);
      curve[3] = points[i].p1;
      break;
    case BEZ_CURVE_TO:
      curve[0] = curve[3];
      curve[1] = points[i].p1;
      curve[2] = points[i].p2;
      curve[3] = points[i].p3;
      bezier_add_curve(ddisp, curve, &bezier);
      break;
    }
  
  color_convert(color, &gdk_color);
  gdk_gc_set_foreground(gc, &gdk_color);
  
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     GDK_JOIN_ROUND);

  gdk_draw_lines(renderer->pixmap, gc, bezier.gdk_points, bezier.currpoint);

  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
fill_bezier(RendererGdk *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdk_color;
  Point curve[4];
  int i;
  
  if (bezier.gdk_points == NULL) {
    bezier.numpoints = 30;
    bezier.gdk_points = g_malloc(bezier.numpoints*sizeof(GdkPoint));
  }

  bezier.currpoint = 0;
  
  if (points[0].type != BEZ_MOVE_TO)
    g_warning("first BezPoint must be a BEZ_MOVE_TO");
  curve[3] = points[0].p1;
  bezier_add_point(renderer->ddisp, &bezier, &points[0].p1);
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      curve[3] = points[i].p1;
      break;
    case BEZ_LINE_TO:
      bezier_add_point(renderer->ddisp, &bezier, &points[i].p1);
      curve[3] = points[i].p1;
      break;
    case BEZ_CURVE_TO:
      curve[0] = curve[3];
      curve[1] = points[i].p1;
      curve[2] = points[i].p2;
      curve[3] = points[i].p3;
      bezier_add_curve(ddisp, curve, &bezier);
      break;
    }
  
  color_convert(color, &gdk_color);
  gdk_gc_set_foreground(gc, &gdk_color);
  
  gdk_draw_polygon(renderer->pixmap, gc, TRUE,
		   bezier.gdk_points, bezier.currpoint);
}


static void
abuse_layout_object(PangoLayout* layout, const char* text)
{
    PangoLayoutIter* iter;
    guint i = 0;

    g_message("*** START OF abuse_layout_object for %s",text);
    iter = pango_layout_get_iter(layout);

    
    do {
        PangoLayoutRun* run = pango_layout_iter_get_run(iter);
        PangoFontDescription* pfd = NULL;
        gchar *fname = NULL;
        gchar* gs = NULL;
        gint j = 0;
        
        g_message("Run #%u. Got %p",i++,run);
        if (!run) continue;

        if (run->item->analysis.font) {
            pfd = pango_font_describe(run->item->analysis.font);
            fname = pango_font_description_to_string(pfd);
        };
        g_message("   Analysis: shape_engine:%s/%s; lang_engine:%s/%s "
                  "font:%s level:%u lang:%s",
                  run->item->analysis.shape_engine ?
                  run->item->analysis.shape_engine->engine.id : "NULL",
                  run->item->analysis.shape_engine ?
                  run->item->analysis.shape_engine->engine.type : "NULL",

                  run->item->analysis.lang_engine ?
                  run->item->analysis.lang_engine->engine.id : "NULL",
                  run->item->analysis.lang_engine ?
                  run->item->analysis.lang_engine->engine.type : "NULL",

                  fname ? fname : "NULL",
                  run->item->analysis.level,

                  run->item->analysis.language ?
                  pango_language_to_string(run->item->analysis.language):
                  "NULL");

        gs = g_strdup_printf("Glyphs: num_glyphs=%u ",run->glyphs->num_glyphs);
        
        for (j = 0; j < run->glyphs->num_glyphs; ++j) {
            char *tmp = g_strdup_printf("%s (%08x %d)", gs,
                                        run->glyphs->glyphs[j].glyph,
                                        run->glyphs->glyphs[j].geometry.width);
            g_free(gs);
            gs = tmp;
        }
        g_message(gs);
        
        g_free(fname);
        g_free(gs);
                  
    } while (pango_layout_iter_next_run(iter));


    pango_layout_iter_free(iter);
    
    g_message("*** END OF abuse_layout_object for %s",text);
}

static gint get_layout_first_baseline(PangoLayout* layout) {
    PangoLayoutIter* iter = pango_layout_get_iter(layout);
    gint result = pango_layout_iter_get_baseline(iter) / PANGO_SCALE;
    pango_layout_iter_free(iter);
    return result;
}

static void
draw_string (RendererGdk *renderer,
	    const char *text,
	     Point *pos, Alignment alignment,
	     Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;

  int x,y;
  Point start_pos;
  PangoLayout* layout;
  
  point_copy(&start_pos,pos);

  switch (alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    start_pos.x -= dia_font_scaled_string_width(text, renderer->font,
                                                 renderer->font_height,
                                                 ddisp->zoom_factor)/2;
    break;
  case ALIGN_RIGHT:
    start_pos.x -= dia_font_scaled_string_width(text, renderer->font,
                                                 renderer->font_height,
                                                 ddisp->zoom_factor);
    break;
  }
  ddisplay_transform_coords(ddisp, start_pos.x, start_pos.y, &x, &y);

  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  layout = dia_font_scaled_build_layout(text, renderer->font,
                                        renderer->font_height,
                                        ddisp->zoom_factor);
  y -= get_layout_first_baseline(layout);  
  gdk_draw_layout(renderer->pixmap,gc,x,y,layout);

      /* abuse_layout_object(layout,text); */
  
  g_object_unref(G_OBJECT(layout));

#if 0
  {
     static Color red = {1.0,0.0,0.0};
     static Color blue = {0.0,0.0,1.0};
      
     draw_pixel_line(renderer,x-5,y+5,x+5,y-5,&red);
     draw_pixel_line(renderer,x-5,y-5,x+5,y+5,&red);
  
     ddisplay_transform_coords(ddisp, pos->x, pos->y, &x, &y);
     draw_pixel_line(renderer,x-5,y+5,x+5,y-5,&blue);
     draw_pixel_line(renderer,x-5,y-5,x+5,y+5,&blue);
  }
#endif
}

/* Get the width of the given text in cm */
static real
get_text_width(RendererGdk *renderer,
               const gchar *text, int length)
{
    if (length != strlen(text)) {
        char *othertx;
        real result;
        
        othertx = g_strndup(text,length);
        result = dia_font_scaled_string_width(othertx,renderer->font,
                                              renderer->font_height,
                                              renderer->ddisp->zoom_factor);
        g_free(othertx);
        return result;
    }
    
    return dia_font_scaled_string_width(text,renderer->font,
                                        renderer->font_height,
                                        renderer->ddisp->zoom_factor);
}

static void
draw_image(RendererGdk *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  int real_width, real_height, real_x, real_y;
  
  real_width = ddisplay_transform_length(renderer->ddisp, width);
  real_height = ddisplay_transform_length(renderer->ddisp, height);
  ddisplay_transform_coords(renderer->ddisp, point->x, point->y,
			    &real_x, &real_y);

  dia_image_draw(image,  renderer->pixmap, real_x, real_y,
		 real_width, real_height);
}

static void
clip_region_clear(RendererGdk *renderer)
{
  if (renderer->clip_region != NULL)
    gdk_region_destroy(renderer->clip_region);

  renderer->clip_region =  gdk_region_new();

  gdk_gc_set_clip_region(renderer->render_gc, renderer->clip_region);
}

static void
clip_region_add_rect(RendererGdk *renderer,
		     Rectangle *rect)
{
  DDisplay *ddisp = renderer->ddisp;
  GdkRectangle clip_rect;
  int x1,y1;
  int x2,y2;

  ddisplay_transform_coords(ddisp, rect->left, rect->top,  &x1, &y1);
  ddisplay_transform_coords(ddisp, rect->right, rect->bottom,  &x2, &y2);
  
  clip_rect.x = x1;
  clip_rect.y = y1;
  clip_rect.width = x2 - x1 + 1;
  clip_rect.height = y2 - y1 + 1;

  gdk_region_union_with_rect( renderer->clip_region, &clip_rect );
  
  gdk_gc_set_clip_region(renderer->render_gc, renderer->clip_region);
}

static void
draw_pixel_line(RendererGdk *renderer,
		int x1, int y1,
		int x2, int y2,
		Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);
  
  gdk_draw_line(renderer->pixmap, gc, x1, y1, x2, y2);
}

static void
draw_pixel_rect(RendererGdk *renderer,
		int x, int y,
		int width, int height,
		Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
    
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap, gc, FALSE,
		      x, y,  width, height);
}

static void
fill_pixel_rect(RendererGdk *renderer,
		int x, int y,
		int width, int height,
		Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
    
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_rectangle (renderer->pixmap, gc, TRUE,
		      x, y,  width, height);
}

