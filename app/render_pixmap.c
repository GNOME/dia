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

/* This renderer is similar to RenderGdk, except that it renders onto
 * its own little GdkPixmap, doesn't deal with the DDisplay at all, and
 * uses a 1-1 mapping between points and pixels.  It is used for rendering
 * arrows and lines onto selectors.  This way, any change in the arrow 
 * definitions are automatically reflected in the selectors.
 */

#include <config.h>

#include <math.h>
#include <stdlib.h>
#include <gdk/gdk.h>

#include "render_pixmap.h"
#include "message.h"

static void begin_render(RendererPixmap *renderer);
static void end_render(RendererPixmap *renderer);
static void set_linewidth(RendererPixmap *renderer, real linewidth);
static void set_linecaps(RendererPixmap *renderer, LineCaps mode);
static void set_linejoin(RendererPixmap *renderer, LineJoin mode);
static void set_linestyle(RendererPixmap *renderer, LineStyle mode);
static void set_dashlength(RendererPixmap *renderer, real length);
static void set_fillstyle(RendererPixmap *renderer, FillStyle mode);
static void set_font(RendererPixmap *renderer, DiaFont *font, real height);
static void draw_line(RendererPixmap *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererPixmap *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererPixmap *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererPixmap *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererPixmap *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererPixmap *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererPixmap *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererPixmap *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererPixmap *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererPixmap *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererPixmap *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererPixmap *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererPixmap *renderer,
			const gchar *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererPixmap *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static void init_pixmap_renderops();
static void renderer_pixmap_set_linestyle(RendererPixmap *renderer);

static RenderOps *PixmapRenderOps;

static void
init_pixmap_renderops() {
  PixmapRenderOps = create_renderops_table();

  PixmapRenderOps->begin_render = (BeginRenderFunc) begin_render;
  PixmapRenderOps->end_render = (EndRenderFunc) end_render;

  PixmapRenderOps->set_linewidth = (SetLineWidthFunc) set_linewidth;
  PixmapRenderOps->set_linecaps = (SetLineCapsFunc) set_linecaps;
  PixmapRenderOps->set_linejoin = (SetLineJoinFunc) set_linejoin;
  PixmapRenderOps->set_linestyle = (SetLineStyleFunc) set_linestyle;
  PixmapRenderOps->set_dashlength = (SetDashLengthFunc) set_dashlength;
  PixmapRenderOps->set_fillstyle = (SetFillStyleFunc) set_fillstyle;
  PixmapRenderOps->set_font = (SetFontFunc) set_font;
  
  PixmapRenderOps->draw_line = (DrawLineFunc) draw_line;
  PixmapRenderOps->draw_polyline = (DrawPolyLineFunc) draw_polyline;
  
  PixmapRenderOps->draw_polygon = (DrawPolygonFunc) draw_polygon;
  PixmapRenderOps->fill_polygon = (FillPolygonFunc) fill_polygon;

  PixmapRenderOps->draw_rect = (DrawRectangleFunc) draw_rect;
  PixmapRenderOps->fill_rect = (FillRectangleFunc) fill_rect;

  PixmapRenderOps->draw_arc = (DrawArcFunc) draw_arc;
  PixmapRenderOps->fill_arc = (FillArcFunc) fill_arc;

  PixmapRenderOps->draw_ellipse = (DrawEllipseFunc) draw_ellipse;
  PixmapRenderOps->fill_ellipse = (FillEllipseFunc) fill_ellipse;

  PixmapRenderOps->draw_bezier = (DrawBezierFunc) draw_bezier;
  PixmapRenderOps->fill_bezier = (FillBezierFunc) fill_bezier;

  PixmapRenderOps->draw_string = (DrawStringFunc) draw_string;

  PixmapRenderOps->draw_image = (DrawImageFunc) draw_image;
}

RendererPixmap *
new_pixmap_renderer(GdkWindow *window, int width, int height)
{
  RendererPixmap *renderer;
  GdkColor color;

  if (PixmapRenderOps == NULL)
    init_pixmap_renderops();

  renderer = g_new(RendererPixmap, 1);
  renderer->renderer.ops = PixmapRenderOps;
  renderer->renderer.is_interactive = 0;
  renderer->renderer.interactive_ops = NULL;

  renderer->pixmap = gdk_pixmap_new(window, width, height, -1);
  renderer->render_gc = gdk_gc_new(window);

  gdk_color_white(gdk_colormap_get_system(), &color);
  gdk_gc_set_foreground(renderer->render_gc, &color);
  gdk_draw_rectangle(renderer->pixmap, renderer->render_gc, 1,
		     0, 0, width, height);

  renderer->xoffset = renderer->yoffset = 0;
  renderer->renderer.pixel_width = width;
  renderer->renderer.pixel_height = height;

  renderer->line_width = 2;
  renderer->line_style = GDK_LINE_SOLID;
  renderer->cap_style = GDK_CAP_BUTT;
  renderer->join_style = GDK_JOIN_MITER;

  renderer_pixmap_set_linestyle(renderer);
  
  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_length = 10;
  renderer->dot_length = 2;
  
  return renderer;
}

void
destroy_pixmap_renderer(RendererPixmap *renderer)
{
  if (renderer->pixmap != NULL)
    gdk_pixmap_unref(renderer->pixmap);

  if (renderer->render_gc != NULL)
    gdk_gc_unref(renderer->render_gc);

  g_free(renderer);
}

void
renderer_pixmap_set_pixmap(RendererPixmap *renderer, GdkDrawable *drawable,
			   int xoffset, int yoffset, int width, int height)
{
  if (renderer->pixmap != NULL)
    gdk_pixmap_unref(renderer->pixmap);

  if (renderer->render_gc != NULL)
    gdk_gc_unref(renderer->render_gc);

  gdk_pixmap_ref(drawable);
  renderer->pixmap = drawable;
  renderer->render_gc = gdk_gc_new(drawable);
  
  renderer->xoffset = xoffset;
  renderer->yoffset = yoffset;

  renderer->renderer.pixel_width = width;
  renderer->renderer.pixel_height = height;
}

GdkPixmap *
renderer_pixmap_get_pixmap(RendererPixmap *renderer) {
  return renderer->pixmap;
}

static void
renderer_pixmap_set_linestyle(RendererPixmap *renderer)
{
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->line_width,
			     renderer->line_style,
			     renderer->cap_style,
			     renderer->join_style);
}

static void
begin_render(RendererPixmap *renderer)
{
  gdk_gc_get_values(renderer->render_gc, &renderer->gcvalues);
}

static void
end_render(RendererPixmap *renderer)
{
  gdk_gc_set_line_attributes(renderer->render_gc,
			     renderer->gcvalues.line_width,
			     renderer->gcvalues.line_style,
			     renderer->gcvalues.cap_style,
			     renderer->gcvalues.join_style);
}

static void
set_linewidth(RendererPixmap *renderer, real linewidth)
{  /* 0 == hairline **/
  renderer->line_width = (int)linewidth;

  if (renderer->line_width<=0)
    renderer->line_width = 1; /* Minimum 1 pixel. */

  renderer_pixmap_set_linestyle(renderer);
}

static void
set_linecaps(RendererPixmap *renderer, LineCaps mode)
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
 
}

static void
set_linejoin(RendererPixmap *renderer, LineJoin mode)
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
 
  renderer_pixmap_set_linestyle(renderer);
}

static void
set_linestyle(RendererPixmap *renderer, LineStyle mode)
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
  renderer_pixmap_set_linestyle(renderer);
}

static void
set_dashlength(RendererPixmap *renderer, real length)
{  /* dot = 10% of len */
  renderer->dash_length = (int)floor(length+0.5);
  renderer->dot_length = (int)floor(length*0.1+0.5);
  
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
set_fillstyle(RendererPixmap *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("pixmap_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererPixmap *renderer, DiaFont *font, real height)
{
  renderer->font_height = (int)height;

  renderer->gdk_font = font_get_gdkfont(font, renderer->font_height);
}

static void
draw_line(RendererPixmap *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  int x1,y1,x2,y2;
  
  x1 = start->x+renderer->xoffset; y1 = start->y+renderer->yoffset;
  x2 = end->x+renderer->xoffset; y2 = end->y+renderer->yoffset;
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_line(renderer->pixmap, gc,
		x1, y1,	x2, y2);
}

static void
draw_polyline(RendererPixmap *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i;

  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    gdk_points[i].x = (int)points[i].x+renderer->xoffset;
    gdk_points[i].y = (int)points[i].y+renderer->yoffset;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_lines(renderer->pixmap, gc, gdk_points, num_points);
  g_free(gdk_points);
}

static void
draw_polygon(RendererPixmap *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i;
  
  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    gdk_points[i].x = (int)points[i].x+renderer->xoffset;
    gdk_points[i].y = (int)points[i].y+renderer->yoffset;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_polygon(renderer->pixmap, gc, FALSE, gdk_points, num_points);
  g_free(gdk_points);
}

static void
fill_polygon(RendererPixmap *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor color;
  GdkPoint *gdk_points;
  int i;
  
  gdk_points = g_new(GdkPoint, num_points);

  for (i=0;i<num_points;i++) {
    gdk_points[i].x = (int)points[i].x+renderer->xoffset;
    gdk_points[i].y = (int)points[i].y+renderer->yoffset;
  }
  
  color_convert(line_color, &color);
  gdk_gc_set_foreground(gc, &color);
  
  gdk_draw_polygon(renderer->pixmap, gc, TRUE, gdk_points, num_points);
  g_free(gdk_points);
}

static void
draw_rect(RendererPixmap *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;

  left = (int)ul_corner->x+renderer->xoffset;
  right = (int)lr_corner->x+renderer->xoffset;
  top = (int)ul_corner->y+renderer->yoffset;
  bottom = (int)lr_corner->y+renderer->yoffset;
  
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
fill_rect(RendererPixmap *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, bottom, left, right;
    
  left = (int)ul_corner->x+renderer->xoffset;
  right = (int)lr_corner->x+renderer->xoffset;
  top = (int)ul_corner->y+renderer->yoffset;
  bottom = (int)lr_corner->y+renderer->yoffset;
  
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
draw_arc(RendererPixmap *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, left, bottom, right;
  real dangle;
  
  left = (int)center->x - width/2+renderer->xoffset;
  right = (int)center->x + width/2+renderer->xoffset;
  top = (int)center->y - width/2+renderer->yoffset;
  bottom = (int)center->y + width/2+renderer->yoffset;
  
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
fill_arc(RendererPixmap *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  gint top, left, bottom, right;
  real dangle;
  
  left = (int)center->x - width/2+renderer->xoffset;
  right = (int)center->x + width/2+renderer->xoffset;
  top = (int)center->y - width/2+renderer->yoffset;
  bottom = (int)center->y + width/2+renderer->yoffset;
  
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
draw_ellipse(RendererPixmap *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  draw_arc(renderer, center, width, height, 0.0, 360.0, color); 
}

static void
fill_ellipse(RendererPixmap *renderer, 
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
bezier_add_point(RendererPixmap *renderer,
		 struct bezier_curve *bezier,
		 Point *point)
{
  /* Grow if needed: */
  if (bezier->currpoint == bezier->numpoints) {
    bezier->numpoints += 40;
    bezier->gdk_points = g_realloc(bezier->gdk_points,
				   bezier->numpoints*sizeof(GdkPoint));
  }
  
  bezier->gdk_points[bezier->currpoint].x = (int)point->x+renderer->xoffset;
  bezier->gdk_points[bezier->currpoint].y = (int)point->y+renderer->yoffset;
  
  bezier->currpoint++;
}

static void
bezier_add_lines(RendererPixmap *renderer,
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
  delta = point_dot(&x,&x);
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
    delta = point_dot(&x,&x);
    if (delta < BEZIER_SUBDIVIDE_LIMIT_SQ) { /* Almost flat, draw a line */
      bezier_add_point(renderer, bezier, &points[3]);
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

  bezier_add_lines(renderer, r, bezier);
  bezier_add_lines(renderer, s, bezier);
}

static void
bezier_add_curve(RendererPixmap *renderer,
		 Point points[4],
		 struct bezier_curve *bezier)
{
  /* Is the bezier curve malformed? */
  if ( (distance_point_point(&points[0], &points[1]) < 0.00001) &&
       (distance_point_point(&points[2], &points[3]) < 0.00001) &&
       (distance_point_point(&points[0], &points[3]) < 0.00001)) {
    bezier_add_point(renderer, bezier, &points[3]);
  }
  
  bezier_add_lines(renderer, points, bezier);
}


static struct bezier_curve bezier = { NULL, 0, 0 };

static void
draw_bezier(RendererPixmap *renderer, 
	    BezPoint *points,
	    int numpoints,
	    Color *color)
{
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
  bezier_add_point(renderer, &bezier, &points[0].p1);
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      curve[3] = points[i].p1;
      break;
    case BEZ_LINE_TO:
      bezier_add_point(renderer, &bezier, &points[i].p1);
      curve[3] = points[i].p1;
      break;
    case BEZ_CURVE_TO:
      curve[0] = curve[3];
      curve[1] = points[i].p1;
      curve[2] = points[i].p2;
      curve[3] = points[i].p3;
      bezier_add_curve(renderer, curve, &bezier);
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

  renderer_pixmap_set_linestyle(renderer);
}

static void
fill_bezier(RendererPixmap *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints, /* numpoints = 4+3*n, n=>0 */
	    Color *color)
{
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
  bezier_add_point(renderer, &bezier, &points[0].p1);
  for (i = 1; i < numpoints; i++)
    switch (points[i].type) {
    case BEZ_MOVE_TO:
      g_warning("only first BezPoint can be a BEZ_MOVE_TO");
      curve[3] = points[i].p1;
      break;
    case BEZ_LINE_TO:
      bezier_add_point(renderer, &bezier, &points[i].p1);
      curve[3] = points[i].p1;
      break;
    case BEZ_CURVE_TO:
      curve[0] = curve[3];
      curve[1] = points[i].p1;
      curve[2] = points[i].p2;
      curve[3] = points[i].p3;
      bezier_add_curve(renderer, curve, &bezier);
      break;
    }
  
  color_convert(color, &gdk_color);
  gdk_gc_set_foreground(gc, &gdk_color);
  
  gdk_draw_polygon(renderer->pixmap, gc, TRUE,
		   bezier.gdk_points, bezier.currpoint);
}

struct pixmap_freetype_user_data {
  GdkPixmap *pixmap;
  GdkGC *gc;
};


static void
draw_string (RendererPixmap *renderer,
	     const gchar *text,
	     Point *pos, Alignment alignment,
	     Color *color)
{
  GdkGC *gc = renderer->render_gc;
  GdkColor gdkcolor;
  int x,y;
  int iwidth;
  GdkWChar *wcstr;
  gchar *str, *mbstr;
  int length, wclength;
  int i;
  
  x = (int)pos->x+renderer->xoffset;
  y = (int)pos->y+renderer->yoffset;

  iwidth = gdk_string_width(renderer->gdk_font, text);

  switch (alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    x -= iwidth/2;
    break;
  case ALIGN_RIGHT:
    x -= iwidth;
    break;
  }
  
  color_convert(color, &gdkcolor);
  gdk_gc_set_foreground(gc, &gdkcolor);

  gdk_draw_string(renderer->pixmap,
		  renderer->gdk_font, gc,
		  x,y, text);
}

static void
draw_image(RendererPixmap *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  int real_width, real_height, real_x, real_y;
  
  real_width = (int)width;
  real_height = (int)height;
  real_x = (int)point->x+renderer->xoffset;
  real_y = (int)point->y+renderer->yoffset;

  dia_image_draw(image,  renderer->pixmap, real_x, real_y,
		 real_width, real_height);
}
