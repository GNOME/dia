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

#include <math.h>
#include <string.h> /* strlen */

#include "dialibartrenderer.h"

#include "message.h"
#include "color.h"
#include "font.h"
#include "intl.h"
#include "dia_image.h"
#include "object.h"
#include "textline.h"

#ifdef HAVE_LIBART

#ifdef HAVE_FREETYPE
#include <pango/pango.h>
#include <pango/pangoft2.h>
#elif defined G_OS_WIN32
/* ugly namespace clashes */
#define Rectangle Win32Rectangle
#include <pango/pango.h>
#undef Rectangle
#else
#include <pango/pango.h>
#include <gdk/gdk.h>
#endif

#include <libart_lgpl/art_point.h>
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_affine.h>
#include <libart_lgpl/art_svp_vpath.h>
#include <libart_lgpl/art_bpath.h>
#include <libart_lgpl/art_vpath_bpath.h>
#include <libart_lgpl/art_rgb.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_rgb_affine.h>
#include <libart_lgpl/art_rgb_rgba_affine.h>
#include <libart_lgpl/art_rgb_bitmap_affine.h>
#include <libart_lgpl/art_filterlevel.h>
#include <libart_lgpl/art_svp_intersect.h>

static inline guint32 
color_to_abgr(Color *col)
{
  int rgba;

  rgba = 0x0;
  rgba |= (guint)(0xFF*col->blue) << 16;
  rgba |= (guint)(0xFF*col->green) << 8;
  rgba |= (guint)(0xFF*col->red);
  
  return rgba;
}

static inline guint32 
color_to_rgba(DiaLibartRenderer *renderer, Color *col)
{
  int rgba;

  if (renderer->highlight_color != NULL) {
      rgba = 0xFF;
      rgba |= (guint)(0xFF*renderer->highlight_color->red) << 24;
      rgba |= (guint)(0xFF*renderer->highlight_color->green) << 16;
      rgba |= (guint)(0xFF*renderer->highlight_color->blue) << 8;
  } else {
    rgba = 0xFF;
    rgba |= (guint)(0xFF*col->red) << 24;
    rgba |= (guint)(0xFF*col->green) << 16;
    rgba |= (guint)(0xFF*col->blue) << 8;
  }

  return rgba;
}

static int 
get_width_pixels (DiaRenderer *self)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  return renderer->pixel_width;
}

static int 
get_height_pixels (DiaRenderer *self)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  return renderer->pixel_height;
}

static void
begin_render(DiaRenderer *self)
{
#ifdef HAVE_FREETYPE
  /* pango_ft2_get_context API docs :
   * ... Use of this function is discouraged, ...
   *
   * I've tried using the proper font_map stuff, but it kills font size:(
   */
  dia_font_push_context(pango_ft2_get_context(75, 75));
# define FONT_SCALE (1.0)
#elif defined G_OS_WIN32
  dia_font_push_context(gdk_pango_context_get());
  /* I shall never claim again to have understood Dias/Pangos font size
   * relations ;). This was (1.0/22.0) but nowadays 1.0 seems to be
   * fine with Pango/win32, too.  --hb
   */
# define FONT_SCALE (1.0)
#else
  dia_font_push_context (gdk_pango_context_get ());
# define FONT_SCALE (0.8)
#endif
}

static void
end_render(DiaRenderer *self)
{
  dia_font_pop_context();
}



static void
set_linewidth(DiaRenderer *self, real linewidth)
{  /* 0 == hairline **/
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  if (renderer->highlight_color != NULL) {
    /* 6 pixels wide -> 3 pixels beyond normal obj */
    real border = dia_untransform_length(renderer->transform, 6);
    linewidth += border;
  } 
  renderer->line_width =
    dia_transform_length(renderer->transform, linewidth);
  if (renderer->line_width<=0.5)
    renderer->line_width = 0.5; /* Minimum 0.5 pixel. */
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  if (renderer->highlight_color != NULL) {
    /* Can't tell that this does anything:( -LC */
    renderer->cap_style = ART_PATH_STROKE_CAP_ROUND;
  } else {
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
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  if (renderer->highlight_color != NULL) {
    /* Can't tell that this does anything:( -LC */
    renderer->join_style = ART_PATH_STROKE_JOIN_ROUND;
  } else {
    switch(mode) {
    case LINEJOIN_MITER:
      renderer->join_style = ART_PATH_STROKE_JOIN_MITER;
      break;
    case LINEJOIN_ROUND:
      renderer->join_style = ART_PATH_STROKE_JOIN_ROUND;
      break;
    case LINEJOIN_BEVEL:
      renderer->join_style = ART_PATH_STROKE_JOIN_BEVEL;
      break;
    }
  }
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
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
set_dashlength(DiaRenderer *self, real length)
{  /* dot = 10% of len */
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  real ddisp_len;

  ddisp_len =
    dia_transform_length(renderer->transform, length);
  
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
  set_linestyle(self, renderer->saved_line_style);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  switch(mode) {
  case FILLSTYLE_SOLID:
    break;
  default:
    message_error(_("gdk_renderer: Unsupported fill mode specified!\n"));
  }
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  self->font_height = height * FONT_SCALE;

  dia_font_ref(font);
  if (self->font)
    dia_font_unref(self->font);
  self->font = font;
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;

  rgba = color_to_rgba(renderer, line_color);
  
  vpath = art_new (ArtVpath, 3);
  
  dia_transform_coords_double(renderer->transform, start->x, start->y, &x, &y);
  vpath[0].code = ART_MOVETO;
  vpath[0].x = x;
  vpath[0].y = y;
  
  dia_transform_coords_double(renderer->transform, end->x, end->y, &x, &y);
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(renderer, line_color);
  
  vpath = art_new (ArtVpath, num_points+1);

  for (i=0;i<num_points;i++) {
    dia_transform_coords_double(renderer->transform,
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;
  
  rgba = color_to_rgba(renderer, line_color);
  
  vpath = art_new (ArtVpath, num_points+2);

  for (i=0;i<num_points;i++) {
    dia_transform_coords_double(renderer->transform,
                                points[i].x, points[i].y,
                                &x, &y);
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x;
    vpath[i].y = y;
  }
  dia_transform_coords_double(renderer->transform,
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath;
  ArtSVP *svp, *temp;
  guint32 rgba;
  double x,y;
  int i;
  ArtSvpWriter *swr;
  
  rgba = color_to_rgba(renderer, color);
  
  vpath = art_new (ArtVpath, num_points+2);

  for (i=0;i<num_points;i++) {
    dia_transform_coords_double(renderer->transform,
                                points[i].x, points[i].y,
                                &x, &y);
    vpath[i].code = (i==0)?ART_MOVETO:ART_LINETO;
    vpath[i].x = x;
    vpath[i].y = y;
  }
  dia_transform_coords_double(renderer->transform,
                              points[0].x, points[0].y,
                              &x, &y);
  vpath[i].code = ART_LINETO;
  vpath[i].x = x;
  vpath[i].y = y;
  vpath[i+1].code = ART_END;
  vpath[i+1].x = 0;
  vpath[i+1].y = 0;
  
  temp = art_svp_from_vpath (vpath);
  
  art_free( vpath );
  
  /** We always use odd-even wind rule */
  swr = art_svp_writer_rewind_new(ART_WIND_RULE_ODDEVEN);

  art_svp_intersector(temp, swr);
  svp = art_svp_writer_rewind_reap(swr);
  art_svp_free(temp);

  art_rgb_svp_alpha (svp,
		     0, 0, 
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  double top, bottom, left, right;
    
  dia_transform_coords_double(renderer->transform,
                              ul_corner->x, ul_corner->y, &left, &top);
  dia_transform_coords_double(renderer->transform,
                              lr_corner->x, lr_corner->y, &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  rgba = color_to_rgba(renderer, color);
  
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_rect(DiaRenderer *self, 
	  Point *ul_corner, Point *lr_corner,
	  Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath;
  ArtSVP *svp;
  guint32 rgba;
  double top, bottom, left, right;
    
  dia_transform_coords_double(renderer->transform,
                              ul_corner->x, ul_corner->y, &left, &top);
  dia_transform_coords_double(renderer->transform,
                              lr_corner->x, lr_corner->y, &right, &bottom);
  
  if ((left>right) || (top>bottom))
    return;

  rgba = color_to_rgba(renderer, color);
  
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *line_color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtSVP *svp;
  guint32 rgba;
  real dangle;
  real circ;
  double x,y;
  int num_points;
  double theta, dtheta;
  int i;
  
  width = dia_transform_length(renderer->transform, width);
  height = dia_transform_length(renderer->transform, height);
  dia_transform_coords_double(renderer->transform, 
                              center->x, center->y, &x, &y);

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

  rgba = color_to_rgba(renderer, line_color);
  
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath;
  ArtSVP *svp;
  guint32 rgba;
  real dangle;
  real circ;
  double x,y;
  int num_points;
  double theta, dtheta;
  int i;
  
  width = dia_transform_length(renderer->transform, width);
  height = dia_transform_length(renderer->transform, height);
  dia_transform_coords_double(renderer->transform, 
                              center->x, center->y, &x, &y);

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

  rgba = color_to_rgba(renderer, color);
  
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  draw_arc(self, center, width, height, 0.0, 360.0, color); 
}

static void
fill_ellipse(DiaRenderer *self, 
	     Point *center,
	     real width, real height,
	     Color *color)
{
  fill_arc(self, center, width, height, 0.0, 360.0, color); 
}

static void
draw_bezier(DiaRenderer *self, 
	    BezPoint *points,
	    int numpoints, 
	    Color *line_color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath, *vpath_dashed;
  ArtBpath *bpath;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(renderer, line_color);
  
  bpath = art_new (ArtBpath, numpoints+1);

  for (i=0;i<numpoints;i++) {
    switch(points[i].type) {
    case BEZ_MOVE_TO:
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].code = ART_MOVETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_LINE_TO:
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].code = ART_LINETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_CURVE_TO:
      bpath[i].code = ART_CURVETO;
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].x1 = x;
      bpath[i].y1 = y;
      dia_transform_coords_double(renderer->transform,
                                  points[i].p2.x, points[i].p2.y,
                                  &x, &y);
      bpath[i].x2 = x;
      bpath[i].y2 = y;
      dia_transform_coords_double(renderer->transform,
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
fill_bezier(DiaRenderer *self, 
	    BezPoint *points, /* Last point must be same as first point */
	    int numpoints, 
	    Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  ArtVpath *vpath;
  ArtBpath *bpath;
  ArtSVP *svp;
  guint32 rgba;
  double x,y;
  int i;

  rgba = color_to_rgba(renderer, color);
  
  bpath = art_new (ArtBpath, numpoints+1);

  for (i=0;i<numpoints;i++) {
    switch(points[i].type) {
    case BEZ_MOVE_TO:
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].code = ART_MOVETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_LINE_TO:
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].code = ART_LINETO;
      bpath[i].x3 = x;
      bpath[i].y3 = y;
      break;
    case BEZ_CURVE_TO:
      bpath[i].code = ART_CURVETO;
      dia_transform_coords_double(renderer->transform,
                                  points[i].p1.x, points[i].p1.y,
                                  &x, &y);
      bpath[i].x1 = x;
      bpath[i].y1 = y;
      dia_transform_coords_double(renderer->transform,
                                  points[i].p2.x, points[i].p2.y,
                                  &x, &y);
      bpath[i].x2 = x;
      bpath[i].y2 = y;
      dia_transform_coords_double(renderer->transform,
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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}


/* Draw a highlighted version of a string.
 */
static void
draw_highlighted_string(DiaLibartRenderer *renderer,
			PangoLayout *layout,
			real x, real y,
			guint32 rgba)
{
  ArtVpath *vpath;
  ArtSVP *svp;
  int width, height;
  double top, bottom, left, right;
    
  pango_layout_get_pixel_size(layout, &width, &height);
  dia_transform_coords_double(renderer->transform,
                              x, y, &left, &top);
  left -= 3;
  right = left+width+6;
  bottom = top+height;
  
  if ((left>right) || (top>bottom))
    return;

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
		     renderer->pixel_width,
		     renderer->pixel_height,
		     rgba,
		     renderer->rgb_buffer, renderer->pixel_width*3,
		     NULL);

  art_svp_free( svp );
}

static void
draw_text_line(DiaRenderer *self, TextLine *text_line,
	       Point *pos,  Alignment alignment, Color *color)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);
  /* Not working with Pango */
  guint8 *bitmap = NULL;
  double x,y;
  Point start_pos;
  PangoLayout* layout;
  int width, height;
  int i, j;
  double affine[6], tmpaffine[6];
  guint32 rgba;
  int rowstride;
  gchar *text = text_line_get_string(text_line);
  real scale = dia_transform_length(renderer->transform, 1.0);

  point_copy(&start_pos,pos);

  rgba = color_to_rgba(renderer, color);

  /* Something's screwed up with the zooming for fonts.
     It's like it zooms double.  Yet removing some of the zooms
     don't seem to work, either.
  */
  /*
  old_zoom = ddisp->zoom_factor;
  ddisp->zoom_factor = ddisp->zoom_factor;
  */
 
  start_pos.x -= text_line_get_alignment_adjustment (text_line, alignment);
  start_pos.y -= text_line_get_ascent(text_line);

  dia_transform_coords_double(renderer->transform, 
                              start_pos.x, start_pos.y, &x, &y);

  layout = dia_font_build_layout(text, text_line->font, 
				 dia_transform_length(renderer->transform,
						      text_line_get_height(text_line)) / 20.0);

  text_line_adjust_layout_line(text_line, pango_layout_get_line(layout, 0),
			       scale/20.0);

  if (renderer->highlight_color != NULL) {
    draw_highlighted_string(renderer, layout, start_pos.x, start_pos.y, rgba);
    g_object_unref(G_OBJECT(layout));
    return;
  }

  /*
  ddisp->zoom_factor = old_zoom;
  */
  pango_layout_get_pixel_size(layout, &width, &height);
  /* Pango doesn't have a 'render to raw bits' function, so we have
   * to render based on what other engines are available.
   */
#define DEPTH 4
#ifdef HAVE_FREETYPE
  /* Freetype version */
 {
   FT_Bitmap ftbitmap;
   guint8 *graybitmap;

   rowstride = 32*((width+31)/31);
   
   graybitmap = (guint8*)g_new0(guint8, height*rowstride);

   ftbitmap.rows = height;
   ftbitmap.width = width;
   ftbitmap.pitch = rowstride;
   ftbitmap.buffer = graybitmap;
   ftbitmap.num_grays = 256;
   ftbitmap.pixel_mode = ft_pixel_mode_grays;
   ftbitmap.palette_mode = 0;
   ftbitmap.palette = 0;
   pango_ft2_render_layout(&ftbitmap, layout, 0, 0);
   bitmap = (guint8*)g_new0(guint8, height*rowstride*DEPTH);
   for (i = 0; i < height; i++) {
     for (j = 0; j < width; j++) {
       bitmap[DEPTH*(i*rowstride+j)] = color->red*255;
       bitmap[DEPTH*(i*rowstride+j)+1] = color->green*255;
       bitmap[DEPTH*(i*rowstride+j)+2] = color->blue*255;
       bitmap[DEPTH*(i*rowstride+j)+3] = graybitmap[i*rowstride+j];
     }
   }
   g_free(graybitmap);
 }
#else
 /* gdk does not like 0x0 sized (it does not make much sense with the others as well,
  * but they don't complain ;) */
 if (width * height > 0)
 {
   GdkPixmap *pixmap = gdk_pixmap_new (NULL, width, height, 24);
   GdkGC     *gc = gdk_gc_new (pixmap);
   GdkImage  *image;

   rowstride = 32*((width+31)/31);
#if 1 /* with 8 bit pixmap we would probably need to set the whole gray palette */
   gdk_gc_set_foreground (gc, &color_gdk_black);
   gdk_gc_set_background (gc, &color_gdk_white);
   gdk_draw_rectangle (GDK_DRAWABLE (pixmap), gc, TRUE, 0, 0, width, height); 
#endif
   gdk_gc_set_foreground (gc, &color_gdk_white);
   gdk_gc_set_background (gc, &color_gdk_black);
   gdk_draw_layout (GDK_DRAWABLE (pixmap), gc, 0, 0, layout);
   image = gdk_drawable_get_image (GDK_DRAWABLE (pixmap), 0, 0, width, height);
   g_object_unref (G_OBJECT (gc));
   g_object_unref (G_OBJECT (pixmap));
   bitmap = (guint8*)g_new0(guint8, height*rowstride*DEPTH);
   for (i = 0; i < height; i++) {
     for (j = 0; j < width; j++) {
       bitmap[DEPTH*(i*rowstride+j)] = color->red*255;
       bitmap[DEPTH*(i*rowstride+j)+1] = color->green*255;
       bitmap[DEPTH*(i*rowstride+j)+2] = color->blue*255;
       bitmap[DEPTH*(i*rowstride+j)+3] = gdk_image_get_pixel (image, j, i) & 0xFF;
     }
   }
   g_object_unref (G_OBJECT (image));
 }
#endif

  /* abuse_layout_object(layout,text); */
  
  g_object_unref(G_OBJECT(layout));

  art_affine_identity(affine);
  art_affine_translate(tmpaffine, x, y);
  art_affine_multiply(affine, affine, tmpaffine);
  
  if (bitmap != NULL)
    art_rgb_rgba_affine (renderer->rgb_buffer,
		    0, 0,
		    renderer->pixel_width,
		    renderer->pixel_height,
		    renderer->pixel_width * 3,
		    bitmap,
		    width,
		    height,
		    rowstride*DEPTH,
		    affine,
		    ART_FILTER_NEAREST, NULL);

  g_free(bitmap);
#undef DEPTH
}

static void
draw_string (DiaRenderer *self,
	     const gchar *text,
	     Point *pos, Alignment alignment,
	     Color *color)
{
  TextLine *text_line = text_line_new(text, self->font, self->font_height);
  draw_text_line(self, text_line, pos, alignment, color);
  text_line_destroy(text_line);
}


/* Get the width of the given text in cm */
static real
get_text_width(DiaRenderer *object,
               const gchar *text, int length)
{
  real result;
  TextLine *text_line;

  if (length != g_utf8_strlen(text, -1)) {
    char *othertx;
    int ulen;
    /* A couple UTF8-chars: æblegrød Š Ť Ž ę ć ń уфхц�?ОПРЄ є �? Њ Ћ �? */
    ulen = g_utf8_offset_to_pointer(text, length)-text;
    if (!g_utf8_validate(text, ulen, NULL)) {
      g_warning ("Text at char %d not valid\n", length);
    }
    othertx = g_strndup(text, ulen);
    text_line = text_line_new(othertx, object->font, object->font_height);
  } else {
    text_line = text_line_new(text, object->font, object->font_height);
  }
  result = text_line_get_width(text_line);
  text_line_destroy(text_line);
  return result;
}


static void
draw_image(DiaRenderer *self,
	   Point *point,
	   real width, real height,
	   DiaImage image)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (self);

  /* Todo: Handle some kind of clipping! */
  
  if (renderer->highlight_color != NULL) {
    Point lr;
    DiaRendererClass *self_class = DIA_RENDERER_GET_CLASS (self);

    lr = *point;
    lr.x += width;
    lr.y += height;
    self_class->fill_rect(self, point, &lr, renderer->highlight_color);
  } else {
    double real_width, real_height;
    double x,y;
    int src_width, src_height;
    double affine[6];
    int rowstride;
    real_width = dia_transform_length(renderer->transform, width);
    real_height = dia_transform_length(renderer->transform, height);
    dia_transform_coords_double(renderer->transform, 
				point->x, point->y, &x, &y);

    src_width = dia_image_width(image);
    src_height = dia_image_height(image);
    rowstride = dia_image_rowstride(image);

    affine[0] = real_width/(double)src_width;
    affine[1] = 0;
    affine[2] = 0;
    affine[3] = real_height/(double)src_height;
    affine[4] = x;
    affine[5] = y;

    if (dia_image_rgba_data(image)) {
      /* If there is an alpha channel, we can use it directly. */
      const guint8 *img_data = dia_image_rgba_data(image);
      art_rgb_rgba_affine(renderer->rgb_buffer,
			  0, 0,
			  renderer->pixel_width,
			  renderer->pixel_height,
			  renderer->pixel_width*3,
			  img_data, src_width, src_height, 
			  rowstride,
			  affine, ART_FILTER_NEAREST, NULL);
      /* Note that dia_image_rgba_data doesn't copy */
    } else {
      guint8 *img_data = dia_image_rgb_data(image);

      art_rgb_affine(renderer->rgb_buffer,
		     0, 0,
		     renderer->pixel_width,
		     renderer->pixel_height,
		     renderer->pixel_width*3,
		     img_data, src_width, src_height, 
		     rowstride,
		     affine, ART_FILTER_NEAREST, NULL);
    
      g_free(img_data);
    }
  }
}


static void
draw_object (DiaRenderer *renderer, DiaObject *object)
{
  if (renderer->is_interactive &&
      object->highlight_color != NULL) {
    DiaLibartRenderer *libart_rend = DIA_LIBART_RENDERER(renderer);
    libart_rend->highlight_color = object->highlight_color;
    object->ops->draw(object, renderer);
    libart_rend->highlight_color = NULL;
  }
  object->ops->draw(object, renderer);
}

static void
renderer_init (DiaLibartRenderer *renderer, gpointer g_class)
{
  renderer->rgb_buffer = NULL;

  renderer->line_width = 1.0;
  renderer->cap_style = ART_PATH_STROKE_CAP_BUTT;
  renderer->join_style = ART_PATH_STROKE_JOIN_MITER;
  
  renderer->saved_line_style = LINESTYLE_SOLID;
  renderer->dash_enabled = 0;
  renderer->dash_length = 10;
  renderer->dot_length = 1;

  renderer->highlight_color = NULL;

  renderer->parent_instance.font = NULL;
}

static void dia_libart_renderer_class_init (DiaLibartRendererClass *klass);
static gpointer parent_class = NULL;

static void
renderer_finalize (GObject *object)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (object);

  if (renderer->rgb_buffer != NULL)
    g_free(renderer->rgb_buffer);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

GType
dia_libart_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaLibartRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) dia_libart_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaLibartRenderer),
        0,              /* n_preallocs */
        (GInstanceInitFunc)renderer_init /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaLibartRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

enum {
  PROP_0,
  PROP_TRANSFORM
};

static void
dia_libart_interactive_renderer_set_property (GObject         *object,
			 guint            prop_id,
			 const GValue    *value,
			 GParamSpec      *pspec)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (object);

  switch (prop_id) {
    case PROP_TRANSFORM:
      renderer->transform = g_value_get_pointer(value);
      break;
    default:
      break;
    }
}

static void
dia_libart_interactive_renderer_get_property (GObject         *object,
			 guint            prop_id,
			 GValue          *value,
			 GParamSpec      *pspec)
{
  DiaLibartRenderer *renderer = DIA_LIBART_RENDERER (object);
  
  switch (prop_id) {
    case PROP_TRANSFORM:
      g_value_set_pointer (value, renderer->transform);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
dia_libart_renderer_class_init (DiaLibartRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  gobject_class->finalize = renderer_finalize;
	
  gobject_class->set_property = dia_libart_interactive_renderer_set_property;
  gobject_class->get_property = dia_libart_interactive_renderer_get_property;

  g_object_class_install_property (gobject_class,
				   PROP_TRANSFORM,
				   g_param_spec_pointer ("transform",
 							_("Renderer transformation"),
							_("Transform pointer"),
							G_PARAM_READWRITE));

  /* Here we set the functions that we define for this renderer. */
  renderer_class->get_width_pixels = get_width_pixels;
  renderer_class->get_height_pixels = get_height_pixels;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;

  renderer_class->set_linewidth = set_linewidth;
  renderer_class->set_linecaps = set_linecaps;
  renderer_class->set_linejoin = set_linejoin;
  renderer_class->set_linestyle = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle = set_fillstyle;
  renderer_class->set_font = set_font;
  
  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;
  
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->fill_polygon = fill_polygon;

  renderer_class->draw_rect = draw_rect;
  renderer_class->fill_rect = fill_rect;

  renderer_class->draw_arc = draw_arc;
  renderer_class->fill_arc = fill_arc;

  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_bezier = draw_bezier;
  renderer_class->fill_bezier = fill_bezier;

  renderer_class->draw_string = draw_string;
  renderer_class->draw_text_line = draw_text_line;

  renderer_class->draw_image = draw_image;

  renderer_class->draw_object = draw_object;

  /* Interactive functions */
  renderer_class->get_text_width = get_text_width;
}

#endif
