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
#include <gdk/gdk.h>

#include "render_libart.h"
#include "message.h"

#ifdef HAVE_LIBART

#include <libart_lgpl/art_point.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_rgb.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_rgb_affine.h>
#include "libart_lgpl/art_rgb_bitmap_affine.h"
#include <libart_lgpl/art_filterlevel.h>

static void begin_render(RendererLibart *renderer, DiagramData *data);
static void end_render(RendererLibart *renderer);
static void set_linewidth(RendererLibart *renderer, real linewidth);
static void set_linecaps(RendererLibart *renderer, LineCaps mode);
static void set_linejoin(RendererLibart *renderer, LineJoin mode);
static void set_linestyle(RendererLibart *renderer, LineStyle mode);
static void set_dashlength(RendererLibart *renderer, real length);
static void set_fillstyle(RendererLibart *renderer, FillStyle mode);
static void set_font(RendererLibart *renderer, Font *font, real height);
static void draw_line(RendererLibart *renderer, 
		      Point *start, Point *end, 
		      Color *line_color);
static void draw_polyline(RendererLibart *renderer, 
			  Point *points, int num_points, 
			  Color *line_color);
static void draw_polygon(RendererLibart *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void fill_polygon(RendererLibart *renderer, 
			 Point *points, int num_points, 
			 Color *line_color);
static void draw_rect(RendererLibart *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void fill_rect(RendererLibart *renderer, 
		      Point *ul_corner, Point *lr_corner,
		      Color *color);
static void draw_arc(RendererLibart *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void fill_arc(RendererLibart *renderer, 
		     Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     Color *color);
static void draw_ellipse(RendererLibart *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void fill_ellipse(RendererLibart *renderer, 
			 Point *center,
			 real width, real height,
			 Color *color);
static void draw_bezier(RendererLibart *renderer, 
			BezPoint *points,
			int numpoints,
			Color *color);
static void fill_bezier(RendererLibart *renderer, 
			BezPoint *points, /* Last point must be same as first point */
			int numpoints,
			Color *color);
static void draw_string(RendererLibart *renderer,
			const char *text,
			Point *pos, Alignment alignment,
			Color *color);
static void draw_image(RendererLibart *renderer,
		       Point *point,
		       real width, real height,
		       DiaImage image);

static real get_text_width(RendererLibart *renderer,
			   const char *text, int length);

static void clip_region_clear(RendererLibart *renderer);
static void clip_region_add_rect(RendererLibart *renderer,
				 Rectangle *rect);

static void draw_pixel_line(RendererLibart *renderer,
			    int x1, int y1,
			    int x2, int y2,
			    Color *color);
static void draw_pixel_rect(RendererLibart *renderer,
				 int x, int y,
				 int width, int height,
				 Color *color);
static void fill_pixel_rect(RendererLibart *renderer,
				 int x, int y,
				 int width, int height,
				 Color *color);

static RenderOps LibartRenderOps = {
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

static InteractiveRenderOps LibartInteractiveRenderOps = {
  (GetTextWidthFunc) get_text_width,

  (ClipRegionClearFunc) clip_region_clear,
  (ClipRegionAddRectangleFunc) clip_region_add_rect,

  (DrawPixelLineFunc) draw_pixel_line,
  (DrawPixelRectangleFunc) draw_pixel_rect,
  (FillPixelRectangleFunc) fill_pixel_rect,
};

RendererLibart *
new_libart_renderer(DDisplay *ddisp, int interactive)
{
  RendererLibart *renderer;

  renderer = g_new(RendererLibart, 1);
  renderer->renderer.ops = &LibartRenderOps;
  renderer->renderer.is_interactive = interactive;
  renderer->renderer.interactive_ops = &LibartInteractiveRenderOps;
  renderer->ddisp = ddisp;

  renderer->rgb_buffer = NULL;
  renderer->renderer.pixel_width = 0;
  renderer->renderer.pixel_height = 0;

  renderer->line_width = 1.0;
  renderer->cap_style = GDK_CAP_BUTT;
  renderer->join_style = GDK_JOIN_MITER;
  
  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_enabled = 0;
  renderer->dash_length = 10;
  renderer->dot_length = 1;

  renderer->gdk_font = NULL;
  renderer->suck_font = NULL;

  return renderer;
}

void
destroy_libart_renderer(RendererLibart *renderer)
{
  if (renderer->rgb_buffer != NULL)
    g_free(renderer->rgb_buffer);

  g_free(renderer);
}

void
libart_renderer_set_size(RendererLibart *renderer, GdkWindow *window,
			 int width, int height)
{
  int i;
  
  if ( (renderer->renderer.pixel_width==width) &&
       (renderer->renderer.pixel_height==height) )
    return;
  
  if (renderer->rgb_buffer != NULL) {
    g_free(renderer->rgb_buffer);
  }

  renderer->rgb_buffer = g_new (guint8, width * height * 3);
  for (i=0;i<width * height * 3;i++)
    renderer->rgb_buffer[i] = 0xff;
  renderer->renderer.pixel_width = width;
  renderer->renderer.pixel_height = height;
}

extern void
renderer_libart_copy_to_window(RendererLibart *renderer, GdkWindow *window,
			       int x, int y, int width, int height)
{
  static GdkGC *copy_gc = NULL;
  int w;

  if (copy_gc == NULL) {
    copy_gc = gdk_gc_new(window);
  }

  w = renderer->renderer.pixel_width;
  
  gdk_draw_rgb_image(window,
		     copy_gc,
		     x,y,
		     width, height,
		     GDK_RGB_DITHER_NONE,
		     renderer->rgb_buffer+x*3+y*3*w,
		     w*3);
}

static guint32 color_to_rgba(Color *col)
{
  int rgba;

  rgba = 0xFF;
  rgba |= (guint)(0xFF*col->red) << 24;
  rgba |= (guint)(0xFF*col->green) << 16;
  rgba |= (guint)(0xFF*col->blue) << 8;
  
  return rgba;
}

static void
begin_render(RendererLibart *renderer, DiagramData *data)
{
}

static void
end_render(RendererLibart *renderer)
{
}



static void
set_linewidth(RendererLibart *renderer, real linewidth)
{  /* 0 == hairline **/
  renderer->line_width =
    ddisplay_transform_length(renderer->ddisp, linewidth);

  if (renderer->line_width<=0.5)
    renderer->line_width = 0.5; /* Minimum 0.5 pixel. */
}

static void
set_linecaps(RendererLibart *renderer, LineCaps mode)
{
  switch(mode) {
  case LINECAPS_BUTT:
    renderer->cap_style = ART_PATH_STROKE_CAP_BUTT;
    break;
  case LINECAPS_ROUND:
    renderer->cap_style = ART_PATH_STROKE_CAP_ROUND;
    break;
  case LINECAPS_PROJECTING:
    renderer->cap_style = ART_PATH_STROKE_CAP_SQUARE;
    break;
  }
}

static void
set_linejoin(RendererLibart *renderer, LineJoin mode)
{
  switch(mode) {
  case LINEJOIN_MITER:
    renderer->cap_style = ART_PATH_STROKE_JOIN_MITER;
    break;
  case LINEJOIN_ROUND:
    renderer->cap_style = ART_PATH_STROKE_JOIN_ROUND;
    break;
  case LINEJOIN_BEVEL:
    renderer->cap_style = ART_PATH_STROKE_JOIN_BEVEL;
    break;
  }
}

static void
set_linestyle(RendererLibart *renderer, LineStyle mode)
{
  static double dash[10];
  double hole_width;
  
  renderer->saved_line_style = mode;
  switch(mode) {
  case LINESTYLE_SOLID:
    renderer->dash_enabled = 0;
    break;
  case LINESTYLE_DASHED:
    renderer->dash_enabled = 1;
    renderer->dash.offset = 0.0;
    renderer->dash.n_dash = 2;
    renderer->dash.dash = dash;
    dash[0] = renderer->dash_length;
    dash[1] = renderer->dash_length;
    break;
  case LINESTYLE_DASH_DOT:
    renderer->dash_enabled = 1;
    renderer->dash.offset = 0.0;
    renderer->dash.n_dash = 4;
    renderer->dash.dash = dash;
    hole_width = (renderer->dash_length - renderer->dot_length) / 2.0;
    if (hole_width<1.0)
      hole_width = 1.0;
    dash[0] = renderer->dash_length;
    dash[1] = hole_width;
    dash[2] = renderer->dot_length;
    dash[3] = hole_width;
    break;
  case LINESTYLE_DASH_DOT_DOT:
    renderer->dash_enabled = 1;
    renderer->dash.offset = 0.0;
    renderer->dash.n_dash = 6;
    renderer->dash.dash = dash;
    hole_width = (renderer->dash_length - 2*renderer->dot_length) / 3;
    if (hole_width<1.0)
      hole_width = 1.0;
    dash[0] = renderer->dash_length;
    dash[1] = hole_width;
    dash[2] = renderer->dot_length;
    dash[3] = hole_width;
    dash[4] = renderer->dot_length;
    dash[5] = hole_width;
    break;
  case LINESTYLE_DOTTED:
    renderer->dash_enabled = 1;
    renderer->dash.offset = 0.0;
    renderer->dash.n_dash = 2;
    renderer->dash.dash = dash;
    dash[0] = renderer->dot_length;
    dash[1] = renderer->dot_length;
    break;
  }
}

static void
set_dashlength(RendererLibart *renderer, real length)
{  /* dot = 10% of len */
  real ddisp_len;

  ddisp_len =
    ddisplay_transform_length(renderer->ddisp, length);
  
  renderer->dash_length = ddisp_len;
  renderer->dot_length = ddisp_len*0.1;
  
  if (renderer->dash_length<1.0)
    renderer->dash_length = 1.0;
  if (renderer->dash_length>255.0)
    renderer->dash_length = 255.0;
  if (renderer->dot_length<1.0)
    renderer->dot_length = 1.0;
  if (renderer->dot_length>255.0)
    renderer->dot_length = 255.0;
  set_linestyle(renderer, renderer->saved_line_style);
}

static void
set_fillstyle(RendererLibart *renderer, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error("gdk_renderer: Unsupported fill mode specified!\n");
  }
}

static void
set_font(RendererLibart *renderer, Font *font, real height)
{
  renderer->font_height =
    ddisplay_transform_length(renderer->ddisp, height);

  renderer->gdk_font = font_get_gdkfont(font, renderer->font_height);
  renderer->suck_font = font_get_suckfont(font, renderer->font_height);
}

static void
draw_line(RendererLibart *renderer, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;

  rgba = color_to_rgba(line_color);
  
  vpath = art_new (ArtVpath, 3);
  
  ddisplay_transform_coords_double(ddisp, start->x, start->y, &x, &y);
  vpath[0].code = ART_MOVETO;
  vpath[0].x = x;
  vpath[0].y = y;
  
  ddisplay_transform_coords_double(ddisp, end->x, end->y, &x, &y);
  vpath[1].code = ART_LINETO;
  vpath[1].x = x;
  vpath[1].y = y;
  
  vpath[2].code = ART_END;
  vpath[2].x = 0;
  vpath[2].y = 0;

  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_polyline(RendererLibart *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(line_color);
  
  vpath = art_new (ArtVpath, num_points+1);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords_double(ddisp,
				     points[i].x, points[i].y,
				     &x, &y);
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x;
    vpath[i].y = y;
  }
  vpath[i].code = ART_END;
  vpath[i].x = 0;
  vpath[i].y = 0;
  
  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_polygon(RendererLibart *renderer, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;
  
  rgba = color_to_rgba(line_color);
  
  vpath = art_new (ArtVpath, num_points+2);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords_double(ddisp,
				     points[i].x, points[i].y,
				     &x, &y);
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x;
    vpath[i].y = y;
  }
  ddisplay_transform_coords_double(ddisp,
				   points[0].x, points[0].y,
				   &x, &y);
  vpath[i].code = ART_LINETO;
  vpath[i].x = x;
  vpath[i].y = y;
  vpath[i+1].code = ART_END;
  vpath[i+1].x = 0;
  vpath[i+1].y = 0;
  
  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_polygon(RendererLibart *renderer, 
	      Point *points, int num_points, 
	      Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;
  
  rgba = color_to_rgba(color);
  
  vpath = art_new (ArtVpath, num_points+2);

  for (i=0;i<num_points;i++) {
    ddisplay_transform_coords_double(ddisp,
				     points[i].x, points[i].y,
				     &x, &y);
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x;
    vpath[i].y = y;
  }
  ddisplay_transform_coords_double(ddisp,
				   points[0].x, points[0].y,
				   &x, &y);
  vpath[i].code = ART_LINETO;
  vpath[i].x = x;
  vpath[i].y = y;
  vpath[i+1].code = ART_END;
  vpath[i+1].x = 0;
  vpath[i+1].y = 0;
  
  svp = art_svp_from_vpath (vpath);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_rect(RendererLibart *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double top, bottom, left, right;
    
  ddisplay_transform_coords_double(ddisp, ul_corner->x, ul_corner->y,
				   &left, &top);
  ddisplay_transform_coords_double(ddisp, lr_corner->x, lr_corner->y,
				   &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  rgba = color_to_rgba(color);
  
  vpath = art_new (ArtVpath, 6);

  vpath[0].code = ART_MOVETO;
  vpath[0].x = left;
  vpath[0].y = top;
  vpath[1].code = ART_LINETO;
  vpath[1].x = right;
  vpath[1].y = top;
  vpath[2].code = ART_LINETO;
  vpath[2].x = right;
  vpath[2].y = bottom;
  vpath[3].code = ART_LINETO;
  vpath[3].x = left;
  vpath[3].y = bottom;
  vpath[4].code = ART_LINETO;
  vpath[4].x = left;
  vpath[4].y = top;
  vpath[5].code = ART_END;
  vpath[5].x = 0;
  vpath[5].y = 0;
  
  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_rect(RendererLibart *renderer, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath;
  ArtSVP *svp;
  guint32 rgba;
  double top, bottom, left, right;
    
  ddisplay_transform_coords_double(ddisp, ul_corner->x, ul_corner->y,
				   &left, &top);
  ddisplay_transform_coords_double(ddisp, lr_corner->x, lr_corner->y,
				   &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  rgba = color_to_rgba(color);
  
  vpath = art_new (ArtVpath, 6);

  vpath[0].code = ART_MOVETO;
  vpath[0].x = left;
  vpath[0].y = top;
  vpath[1].code = ART_LINETO;
  vpath[1].x = right;
  vpath[1].y = top;
  vpath[2].code = ART_LINETO;
  vpath[2].x = right;
  vpath[2].y = bottom;
  vpath[3].code = ART_LINETO;
  vpath[3].x = left;
  vpath[3].y = bottom;
  vpath[4].code = ART_LINETO;
  vpath[4].x = left;
  vpath[4].y = top;
  vpath[5].code = ART_END;
  vpath[5].x = 0;
  vpath[5].y = 0;
  
  svp = art_svp_from_vpath (vpath);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_arc(RendererLibart *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  real dangle;
  real circ;
  double x,y;
  int num_points;
  double theta, dtheta;
  int i;
  
  width = ddisplay_transform_length(renderer->ddisp, width);
  height = ddisplay_transform_length(renderer->ddisp, height);
  ddisplay_transform_coords_double(ddisp, center->x, center->y, &x, &y);


  if ((width<0.0) || (height<0.0))
    return;
  
  dangle = angle2-angle1;
  if (dangle<0)
    dangle += 360.0;

  /* Over-approximate the circumference */
  if (width>height)
    circ = M_PI*width;
  else
    circ = M_PI*height;

  circ *= dangle/360.0;
  
#define LEN_PER_SEGMENT 3.0

  num_points = circ/LEN_PER_SEGMENT;
  if (num_points<5) /* Don't be too coarse */
    num_points = 5;

  rgba = color_to_rgba(line_color);
  
  vpath = art_new (ArtVpath, num_points+1);

  theta = M_PI*angle1/180.0;
  dtheta = (M_PI*dangle/180.0)/(num_points-1);
  for (i=0;i<num_points;i++) {
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x + width/2.0*cos(theta);
    vpath[i].y = y - height/2.0*sin(theta);
    theta += dtheta;
  }
  vpath[i].code = ART_END;
  vpath[i].x = 0;
  vpath[i].y = 0;
  
  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_arc(RendererLibart *renderer, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath;
  ArtSVP *svp;
  guint32 rgba;
  real dangle;
  real circ;
  double x,y;
  int num_points;
  double theta, dtheta;
  int i;
  
  width = ddisplay_transform_length(renderer->ddisp, width);
  height = ddisplay_transform_length(renderer->ddisp, height);
  ddisplay_transform_coords_double(ddisp, center->x, center->y, &x, &y);

  if ((width<0.0) || (height<0.0))
    return;

  dangle = angle2-angle1;
  if (dangle<0)
    dangle += 360.0;

  /* Over-approximate the circumference */
  if (width>height)
    circ = M_PI*width;
  else
    circ = M_PI*height;

  circ *= dangle/360.0;
  
#define LEN_PER_SEGMENT 3.0

  num_points = circ/LEN_PER_SEGMENT;
  if (num_points<5) /* Don't be too coarse */
    num_points = 5;

  rgba = color_to_rgba(color);
  
  vpath = art_new (ArtVpath, num_points+2+1);

  vpath[0].code = ART_MOVETO;
  vpath[0].x = x;
  vpath[0].y = y;
  theta = M_PI*angle1/180.0;
  dtheta = (M_PI*dangle/180.0)/(num_points-1);
  for (i=0;i<num_points;i++) {
    vpath[i+1].code = ART_LINETO;
    vpath[i+1].x = x + width/2.0*cos(theta);
    vpath[i+1].y = y - height/2.0*sin(theta);
    theta += dtheta;
  }
  vpath[i+1].code = ART_LINETO;
  vpath[i+1].x = x;
  vpath[i+1].y = y;
  vpath[i+2].code = ART_END;
  vpath[i+2].x = 0;
  vpath[i+2].y = 0;
  
  svp = art_svp_from_vpath (vpath);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_ellipse(RendererLibart *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  draw_arc(renderer, center, width, height, 0.0, 360.0, color); 
}

static void
fill_ellipse(RendererLibart *renderer, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  fill_arc(renderer, center, width, height, 0.0, 360.0, color); 
}

static void
draw_bezier(RendererLibart *renderer, 
	    BezPoint *points,
	    int numpoints, 
	    Color *line_color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath, *vpath_dashed;
  ArtBpath *bpath;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(line_color);
  
  bpath = art_new (ArtBpath, numpoints+1);

  for (i=0;i<numpoints;i++) {
    switch(points[i].type) {
    case BEZ_MOVE_TO:
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].code = ART_MOVETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_LINE_TO:
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].code = ART_LINETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_CURVE_TO:
      bpath[i].code = ART_CURVETO;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].x1 = x;
      bpath[i].y1 = y;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p2.x, points[i].p2.y,
				       &x, &y);
      bpath[i].x2 = x;
      bpath[i].y2 = y;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p3.x, points[i].p3.y,
				       &x, &y);
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    }
  }
  bpath[i].code = ART_END;
  bpath[i].x1 = 0;
  bpath[i].y1 = 0;

  vpath = art_bez_path_to_vec(bpath, 0.25);
  art_free(bpath);
  
  if (renderer->dash_enabled) {
    vpath_dashed = art_vpath_dash(vpath, &renderer->dash);
    art_free( vpath );
    vpath = vpath_dashed;
  }

  svp = art_svp_vpath_stroke (vpath,
			      renderer->join_style,
			      renderer->cap_style,
			      renderer->line_width,
			      4,
			      0.25);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_bezier(RendererLibart *renderer, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints, 
	    Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  ArtVpath *vpath;
  ArtBpath *bpath;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(color);
  
  bpath = art_new (ArtBpath, numpoints+1);

  for (i=0;i<numpoints;i++) {
    switch(points[i].type) {
    case BEZ_MOVE_TO:
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].code = ART_MOVETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_LINE_TO:
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].code = ART_LINETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_CURVE_TO:
      bpath[i].code = ART_CURVETO;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p1.x, points[i].p1.y,
				       &x, &y);
      bpath[i].x1 = x;
      bpath[i].y1 = y;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p2.x, points[i].p2.y,
				       &x, &y);
      bpath[i].x2 = x;
      bpath[i].y2 = y;
      ddisplay_transform_coords_double(ddisp,
				       points[i].p3.x, points[i].p3.y,
				       &x, &y);
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    }
  }
  bpath[i].code = ART_END;
  bpath[i].x1 = 0;
  bpath[i].y1 = 0;

  vpath = art_bez_path_to_vec(bpath, 0.25);
  art_free(bpath);

  svp = art_svp_from_vpath (vpath);
  
  art_free( vpath );
  
  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->renderer.pixel_width,
		     renderer->renderer.pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->renderer.pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_string(RendererLibart *renderer,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *color)
{
  DDisplay *ddisp = renderer->ddisp;
  int x, y, i, dx;
  int iwidth;
  int len;
  double affine[6];
  SuckFont *suckfont;
  double xpos, ypos;
  guint32 rgba;

  ddisplay_transform_coords(ddisp, pos->x, pos->y,
			    &x, &y);
  
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

  suckfont = renderer->suck_font;

  xpos = (double) x + 1; 
  ypos = (double) (y - suckfont->ascent);
  
  rgba = color_to_rgba(color);
  
  len = strlen(text);
  for (i = 0; i < len; i++) {
    SuckChar *ch;
    
    ch = &suckfont->chars[(unsigned char)text[i]];
    art_affine_translate(affine, xpos, ypos);
    art_rgb_bitmap_affine(renderer->rgb_buffer,
			  0, 0,
			  renderer->renderer.pixel_width,
			  renderer->renderer.pixel_height,
			  renderer->renderer.pixel_width*3,
			   suckfont->bitmap + (ch->bitmap_offset >> 3),
			   ch->width,
			   suckfont->bitmap_height,
			   suckfont->bitmap_width >> 3,
			   rgba,
			   affine,
			   ART_FILTER_NEAREST, NULL);
    
    dx = ch->left_sb + ch->width + ch->right_sb;
    xpos += dx;
  }

  
  
}

static void
draw_image(RendererLibart *renderer,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  double real_width, real_height;
  double x,y;
  int src_width, src_height;
  guint8 *img_data;
  double affine[6];

  /* Todo: Handle some kind of clipping! */
  
  real_width = ddisplay_transform_length(renderer->ddisp, width);
  real_height = ddisplay_transform_length(renderer->ddisp, height);
  ddisplay_transform_coords_double(renderer->ddisp, point->x, point->y,
			    &x, &y);

  img_data = dia_image_rgb_data(image);
  src_width = dia_image_width(image);
  src_height = dia_image_height(image);
  
  affine[0] = real_width/(double)src_width;
  affine[1] = 0;
  affine[2] = 0;
  affine[3] = real_height/(double)src_height;
  affine[4] = x;
  affine[5] = y;

  art_rgb_affine(renderer->rgb_buffer,
		 0, 0,
		 renderer->renderer.pixel_width,
		 renderer->renderer.pixel_height,
		 renderer->renderer.pixel_width*3,
		 img_data, src_width, src_height, src_width*3,
		 affine, ART_FILTER_NEAREST, NULL);
		 
		 
  /*  dia_image_draw(image,  renderer->pixmap, real_x, real_y,
      real_width, real_height);*/
}

static real
get_text_width(RendererLibart *renderer,
	       const char *text, int length)
{
  int iwidth;
  
  iwidth = gdk_text_width(renderer->gdk_font, text, length);

  return ddisplay_untransform_length(renderer->ddisp, (real) iwidth);
}


static void
clip_region_clear(RendererLibart *renderer)
{
  renderer->clip_rect_empty = 1;
  renderer->clip_rect.top = 0;
  renderer->clip_rect.bottom = 0;
  renderer->clip_rect.left = 0;
  renderer->clip_rect.right = 0;
}

static void
clip_region_add_rect(RendererLibart *renderer,
		     Rectangle *rect)
{
  DDisplay *ddisp = renderer->ddisp;
  int x1,y1;
  int x2,y2;
  IntRectangle r;

  ddisplay_transform_coords(ddisp, rect->left, rect->top,  &x1, &y1);
  ddisplay_transform_coords(ddisp, rect->right, rect->bottom,  &x2, &y2);

  if (x1 < 0)
    x1 = 0;
  if (y1 < 0)
    y1 = 0;
  if (x2 >= renderer->renderer.pixel_width)
    x2 = renderer->renderer.pixel_width - 1;
  if (y2 >= renderer->renderer.pixel_height)
    y2 = renderer->renderer.pixel_height - 1;
  
  r.top = y1;
  r.bottom = y2;
  r.left = x1;
  r.right = x2;
  
  if (renderer->clip_rect_empty) {
    renderer->clip_rect = r;
    renderer->clip_rect_empty = 0;
  } else {
    int_rectangle_union(&renderer->clip_rect, &r);
  }
}


/* BIG FAT WARNING:
 * This code is used to draw pixel based stuff in the RGB buffer.
 * This code is *NOT* as efficient as it could be!
 */

/* All lines and rectangles specifies the coordinates inclusive.
 * This means that a line from x1 to x2 renders both points.
 * If a length is specified the line x to (inclusive) x+width is rendered.
 *
 * The boundaries of the clipping rectangle *are* rendered.
 * so min=5 and max=10 means point 5 and 10 might be rendered to.
 */

/* If the start-end interval is totaly outside the min-max,
   then the returned clipped values can have len<0! */
#define CLIP_1D_LEN(min, max, start, len) \
   if ((start) < (min)) {                 \
      (len) -= (min) - (start);           \
      (start) = (min);                    \
   }                                      \
   if ((start)+(len) > (max)) {           \
      (len) = (max) - (start);            \
   }

/* Does no clipping! */
static void
draw_hline(RendererLibart *renderer,
	   int x, int y, int length,
	   guint8 r, guint8 g, guint8 b)
{
  int stride;
  guint8 *ptr;

  stride = renderer->renderer.pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  if (length>=0)
    art_rgb_fill_run(ptr, r, g, b, length+1);
}

/* Does no clipping! */
static void
draw_vline(RendererLibart *renderer,
	   int x, int y, int height,
	   guint8 r, guint8 g, guint8 b)
{
  int stride;
  guint8 *ptr;

  stride = renderer->renderer.pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  height+=y;
  while (y<=height) {
    *ptr++ = r;
    *ptr++ = g;
    *ptr++ = b;
    ptr += stride - 3;
    y++;
  }
}

static void
draw_pixel_line(RendererLibart *renderer,
		int x1, int y1,
		int x2, int y2,
		Color *color)
{
  guint8 r,g,b;
  guint8 *ptr;
  int start, len;
  int stride;
  int i;
  int x, y;
  int dx, dy, adx, ady;
  int incx, incy;
  int incx_ptr, incy_ptr;
  int frac_pos;
  IntRectangle *clip_rect;


  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;

  if (y1==y2) { /* Horizontal line */
    start = x1;
    len = x2-x1;
    CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, start, len);
    
    /* top line */
    if ( (y1>=renderer->clip_rect.top) &&
	 (y1<=renderer->clip_rect.bottom) ) {
      draw_hline(renderer, start, y1, len, r, g, b);
    }
    return;
  }
  
  if (x1==x2) { /* Vertical line */
    start = y1;
    len = y2-y1;
    CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, start, len);

    /* left line */
    if ( (x1>=renderer->clip_rect.left) &&
	 (x1<=renderer->clip_rect.right) ) {
      draw_vline(renderer, x1, start, len, r, g, b);
    }
    return;
  }

  /* Ugh, kill me slowly for writing this line-drawer.
   * It is actually a standard bresenham, but not very optimized.
   * It is also not very well tested.
   */
  
  stride = renderer->renderer.pixel_width*3;
  clip_rect = &renderer->clip_rect;
  
  dx = x2-x1;
  dy = y2-y1;
  adx = (dx>=0)?dx:-dx;
  ady = (dy>=0)?dy:-dy;

  x = x1; y = y1;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  
  if (adx>=ady) { /* x-major */
    if (dx>0) {
      incx = 1;
      incx_ptr = 0;
    } else {
      incx = -1;
      incx_ptr = -6;
    }
    if (dy>0) {
      incy = 1;
      incy_ptr = stride;
    } else {
      incy = -1;
      incy_ptr = -stride;
    }
    frac_pos = adx;
    
    for (i=0;i<=adx;i++) {
      /* Amazing... He does the clipping in the inner loop!
	 It must be horribly inefficient! */
      if ( (x>=clip_rect->left) &&
	   (x<=clip_rect->right) &&
	   (y>=clip_rect->top) &&
	   (y<=clip_rect->bottom) ) {
	*ptr++ = r;
	*ptr++ = g;
	*ptr++ = b;
      } else {
	ptr += 3;
      }
      x += incx;
      ptr += incx_ptr;
      frac_pos += ady*2;
      if ((frac_pos > 2*adx) || ((dy>0)&&(frac_pos == 2*adx))) {
	y += incy;
	ptr += incy_ptr;
	frac_pos -= 2*adx;
      }
    }
  } else { /* y-major */
    if (dx>0) {
      incx = 1;
      incx_ptr = 0;
    } else {
      incx = -1;
      incx_ptr = -6;
    }
    if (dy>0) {
      incy = 1;
      incy_ptr = stride;
    } else {
      incy = -1;
      incy_ptr = -stride;
    }
    frac_pos = ady;
    
    for (i=0;i<=ady;i++) {
      /* Amazing... He does the clipping in the inner loop!
	 It must be horribly inefficient! */
      if ( (x>=clip_rect->left) &&
	   (x<=clip_rect->right) &&
	   (y>=clip_rect->top) &&
	   (y<=clip_rect->bottom) ) {
	*ptr++ = r;
	*ptr++ = g;
	*ptr++ = b;
      } else {
	ptr += 3;
      }
      y += incy;
      ptr += incy_ptr;
      frac_pos += adx*2;
      if ((frac_pos > 2*ady) || ((dx>0)&&(frac_pos == 2*ady))) {
	x += incx;
	ptr += incx_ptr;
	frac_pos -= 2*ady;
      }
    }
  }

    
  
  
}

static void
draw_pixel_rect(RendererLibart *renderer,
		int x, int y,
		int width, int height,
		Color *color)
{
  guint8 r,g,b;
  int start, len;
  int stride;

  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;

  stride = renderer->renderer.pixel_width*3;
  
  /* clip in x */
  start = x;
  len = width;
  CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, start, len);

  /* top line */
  if ( (y>=renderer->clip_rect.top) &&
       (y<=renderer->clip_rect.bottom) ) {
    draw_hline(renderer, start, y, len, r, g, b);
  }

  /* bottom line */
  if ( (y+height>=renderer->clip_rect.top) &&
       (y+height<=renderer->clip_rect.bottom) ) {
    draw_hline(renderer, start, y+height, len, r, g, b);
  }

  /* clip in y */
  start = y;
  len = height;
  CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, start, len);

  /* left line */
  if ( (x>=renderer->clip_rect.left) &&
       (x<renderer->clip_rect.right) ) {
    draw_vline(renderer, x, start, len, r, g, b);
  }

  /* right line */
  if ( (x+width>=renderer->clip_rect.left) &&
       (x+width<renderer->clip_rect.right) ) {
    draw_vline(renderer, x+width, start, len, r, g, b);
  }
}

static void
fill_pixel_rect(RendererLibart *renderer,
		int x, int y,
		int width, int height,
		Color *color)
{
  guint8 r,g,b;
  guint8 *ptr;
  int i;
  int stride;


  CLIP_1D_LEN(renderer->clip_rect.left, renderer->clip_rect.right, x, width);
  if (width < 0)
    return;

  CLIP_1D_LEN(renderer->clip_rect.top, renderer->clip_rect.bottom, y, height);
  if (height < 0)
    return;
  
  r = color->red*0xff;
  g = color->green*0xff;
  b = color->blue*0xff;
  
  stride = renderer->renderer.pixel_width*3;
  ptr = renderer->rgb_buffer + x*3 + y*stride;
  for (i=0;i<=height;i++) {
    art_rgb_fill_run(ptr, r, g, b, width+1);
    ptr += stride;
  }
}

#else

RendererLibart *
new_libart_renderer(DDisplay *ddisp)
{
  return NULL;
}

void
destroy_libart_renderer(RendererLibart *renderer)
{
}

void
libart_renderer_set_size(RendererLibart *renderer, GdkWindow *window,
			 int width, int height)
{
}

extern void
renderer_libart_copy_to_window(RendererLibart *renderer, GdkWindow *window,
			       int x, int y, int width, int height)
{
}

#endif
