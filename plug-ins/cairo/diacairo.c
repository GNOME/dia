/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo.c -- Cairo based export plugin for dia
 * Copyright (C) 2004, Hans Breuer, <Hans@Breuer.Org>
 *   based on wpg.c 
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

#define G_LOG_DOMAIN "DiaCairo"
#include <glib.h>
#include <errno.h>

#include <cairo.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "dia_image.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"


/* --- the renderer --- */
G_BEGIN_DECLS

#define DIA_CAIRO_TYPE_RENDERER           (dia_cairo_renderer_get_type ())
#define DIA_CAIRO_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_CAIRO_TYPE_RENDERER, DiaCairoRenderer))
#define DIA_CAIRO_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_CAIRO_TYPE_RENDERER, DiaCairoRendererClass))
#define DIA_CAIRO_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_CAIRO_TYPE_RENDERER))
#define DIA_CAIRO_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_CAIRO_TYPE_RENDERER, DiaCairoRendererClass))

GType dia_cairo_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaCairoRenderer DiaCairoRenderer;
typedef struct _DiaCairoRendererClass DiaCairoRendererClass;

struct _DiaCairoRenderer
{
  DiaRenderer parent_instance;

  cairo_t *cr;
  cairo_surface_t *surface;
 
  double dash_length;
  DiagramData *dia;

  real scale;
};

struct _DiaCairoRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

//#define DEBUG_CAIRO

#ifdef DEBUG_CAIRO
#  define DIAG_NOTE(action) action
#  define DIAG_STATE(cr) { if (cairo_status (cr) != CAIRO_STATUS_SUCCESS) g_print ("%s:%d, %s\n", __FILE__, __LINE__, cairo_status_string (cr)); }
#else
#  define DIAG_NOTE(action)
#  define DIAG_STATE(cr)
#endif

/* 
 * render functions 
 */ 
static void
begin_render(DiaRenderer *self)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  renderer->cr = cairo_create ();
  cairo_scale (renderer->cr, renderer->scale, renderer->scale);
  cairo_translate (renderer->cr, -renderer->dia->extents.left, -renderer->dia->extents.top);

  cairo_set_target_surface (renderer->cr, renderer->surface);

  //FIXME: I'd like this to clear the alpha background, but it doesn't work
  //cairo_set_alpha (renderer->cr, 0.0);
  /* clear background */
  cairo_set_rgb_color (renderer->cr,
                       renderer->dia->bg_color.red, 
                       renderer->dia->bg_color.green, 
                       renderer->dia->bg_color.blue);
  cairo_rectangle (renderer->cr, 
                   renderer->dia->extents.left, renderer->dia->extents.top,
                   renderer->dia->extents.right, renderer->dia->extents.bottom);
  cairo_fill (renderer->cr);
  //FIXME : how is this supposed to work ?
  //cairo_set_operator (renderer->cr, DIA_CAIRO_OPERATOR_SRC);
  //cairo_set_alpha (renderer->cr, 1.0);

  DIAG_STATE(renderer->cr)
}

static void
end_render(DiaRenderer *self)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message( "end_render"));
 
  cairo_show_page (renderer->cr);
  DIAG_STATE(renderer->cr)
  cairo_surface_destroy (renderer->surface);
  DIAG_STATE(renderer->cr)
}

static void
set_linewidth(DiaRenderer *self, real linewidth)
{  
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("set_linewidth %f", linewidth));

  cairo_set_line_width (renderer->cr, linewidth);
  DIAG_STATE(renderer->cr)
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("set_linecaps %d", mode));

  switch(mode) {
  case LINECAPS_BUTT:
    cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_BUTT);
    break;
  case LINECAPS_ROUND:
    cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_ROUND);
    break;
  case LINECAPS_PROJECTING:
    cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_SQUARE); //??
    break;
  default:
    message_error("DiaCairoRenderer : Unsupported fill mode specified!\n");
  }
  DIAG_STATE(renderer->cr)
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("set_join %d", mode));

  switch(mode) {
  case LINEJOIN_MITER:
    cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_MITER);
    break;
  case LINEJOIN_ROUND:
    cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_ROUND);
    break;
  case LINEJOIN_BEVEL:
    cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_BEVEL);
    break;
  default:
    message_error("DiaCairoRenderer : Unsupported fill mode specified!\n");
  }
  DIAG_STATE(renderer->cr)
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  /* dot = 10% of len */
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  double dash[6];

  DIAG_NOTE(g_message("set_linestyle %d", mode));

  /* line type */
  switch (mode) {
  case LINESTYLE_SOLID:
    cairo_set_dash (renderer->cr, NULL, 0, 0);
    break;
  case LINESTYLE_DASHED:
    dash[0] = renderer->dash_length;
    dash[1] = renderer->dash_length;
    cairo_set_dash (renderer->cr, dash, 2, 0);
    break;
  case LINESTYLE_DASH_DOT:
    dash[0] = renderer->dash_length;
    dash[1] = renderer->dash_length * 0.45;
    dash[2] = renderer->dash_length * 0.1;
    dash[3] = renderer->dash_length * 0.45;
    cairo_set_dash (renderer->cr, dash, 4, 0);
    break;
  case LINESTYLE_DASH_DOT_DOT:
    dash[0] = renderer->dash_length;
    dash[1] = renderer->dash_length * (0.8/3);
    dash[2] = renderer->dash_length * 0.1;
    dash[3] = renderer->dash_length * (0.8/3);
    dash[4] = renderer->dash_length * 0.1;
    dash[5] = renderer->dash_length * (0.8/3);
    cairo_set_dash (renderer->cr, dash, 6, 0);
    break;
  case LINESTYLE_DOTTED:
    dash[0] = renderer->dash_length * 0.1;
    dash[1] = renderer->dash_length * 0.1;
    cairo_set_dash (renderer->cr, dash, 2, 0);
    break;
  default:
    message_error("DiaCairoRenderer : Unsupported fill mode specified!\n");
  }
  DIAG_STATE(renderer->cr)
}

static void
set_dashlength(DiaRenderer *self, real length)
{  
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("set_dashlength %f", length));

  renderer->dash_length = length;
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("set_fillstyle %d", mode));

  switch(mode) {
  case FILLSTYLE_SOLID:
    //FIXME: how to set _no_ pattern ?
    //cairo_set_pattern (renderer->cr, NULL);
    break;
  default:
    message_error("DiaCairoRenderer : Unsupported fill mode specified!\n");
  }
  DIAG_STATE(renderer->cr)
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  DiaFontStyle style = dia_font_get_style (font);

  const char *family_name;
  DIAG_NOTE(g_message("set_font %f %s", height, dia_font_get_family(font)));

  family_name = dia_font_get_family(font);

  cairo_select_font (renderer->cr,
                     family_name,
                     DIA_FONT_STYLE_GET_SLANT(style) == DIA_FONT_NORMAL ? CAIRO_FONT_SLANT_NORMAL : CAIRO_FONT_SLANT_ITALIC,
                     DIA_FONT_STYLE_GET_WEIGHT(style) < DIA_FONT_MEDIUM ? CAIRO_FONT_WEIGHT_NORMAL : CAIRO_FONT_WEIGHT_BOLD); 
  cairo_scale_font (renderer->cr, height * 0.7); //same magic factor as in lib/font.c

  DIAG_STATE(renderer->cr)
}

static void
draw_line(DiaRenderer *self, 
          Point *start, Point *end, 
          Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("draw_line %f,%f -> %f, %f", 
            start->x, start->y, end->x, end->y));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);
  cairo_move_to (renderer->cr, start->x, start->y);
  cairo_line_to (renderer->cr, end->x, end->y);
  cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
draw_polyline(DiaRenderer *self, 
              Point *points, int num_points, 
              Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE(g_message("draw_polyline n:%d %f,%f ...", 
            num_points, points->x, points->y));

  g_return_if_fail(1 < num_points);

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);

  cairo_new_path (renderer->cr);
  /* point data */
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  for (i = 1; i < num_points; i++)
    {
      cairo_line_to (renderer->cr, points[i].x, points[i].y);
    }
  cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
_polygon(DiaRenderer *self, 
         Point *points, int num_points, 
         Color *color,
         gboolean fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE(g_message("%s_polygon n:%d %f,%f ...",
            fill ? "fill" : "draw",
            num_points, points->x, points->y));

  g_return_if_fail(1 < num_points);

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);

  cairo_new_path (renderer->cr);
  /* point data */
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  for (i = 1; i < num_points; i++)
    {
      cairo_line_to (renderer->cr, points[i].x, points[i].y);
    }
  cairo_line_to (renderer->cr, points[0].x, points[0].y);
  cairo_close_path (renderer->cr);
  if (fill)
    cairo_fill (renderer->cr);
  else
    cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
draw_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color)
{
  _polygon (self, points, num_points, color, FALSE);
}

static void
fill_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color)
{
  _polygon (self, points, num_points, color, TRUE);
}

static void
_rect(DiaRenderer *self, 
      Point *ul_corner, Point *lr_corner,
      Color *color,
      gboolean fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE(g_message("%s_rect %f,%f -> %f,%f", 
            fill ? "fill" : "draw",
            ul_corner->x, ul_corner->y, lr_corner->x, lr_corner->y));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);
  
  cairo_rectangle (renderer->cr, 
                   ul_corner->x, ul_corner->y, 
                   lr_corner->x - ul_corner->x, lr_corner->y - ul_corner->y);

  if (fill)
    cairo_fill (renderer->cr);
  else
    cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
draw_rect(DiaRenderer *self, 
          Point *ul_corner, Point *lr_corner,
          Color *color)
{
  _rect (self, ul_corner, lr_corner, color, FALSE);
}

static void
fill_rect(DiaRenderer *self, 
          Point *ul_corner, Point *lr_corner,
          Color *color)
{
  _rect (self, ul_corner, lr_corner, color, TRUE);
}

static void
draw_arc(DiaRenderer *self, 
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  Point start;

  DIAG_NOTE(g_message("draw_arc %fx%f <%f,<%f", 
            width, height, angle1, angle2));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);

  //FIXME : the result is wrong, but maybe in cairo?
  cairo_new_path (renderer->cr);
  start.x = center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1);
  start.y = center->y + (height / 2.0) * sin((M_PI / 180.0) * angle1);
  cairo_move_to (renderer->cr, start.x, start.y);
#if 0 /* all bogus */
  if (angle1 < angle2)
    cairo_arc_negative (renderer->cr, center->x, center->y, 
               width > height ? height / 2.0 : width / 2.0, //FIXME 2nd radius
               (angle1 / 180) * G_PI, ((angle1 - angle2) / 180) * G_PI);
  else
#else
    cairo_arc (renderer->cr, center->x, center->y, 
               width > height ? height / 2.0 : width / 2.0, //FIXME 2nd radius
               (angle1 / 180) * G_PI, (angle2 / 180) * G_PI);
#endif
  cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
fill_arc(DiaRenderer *self, 
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  Point start;

  DIAG_NOTE(g_message("draw_arc %fx%f <%f,<%f", 
            width, height, angle1, angle2));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);
  
  cairo_new_path (renderer->cr);
  start.x = center->x + (width / 2.0)  * cos((M_PI / 180.0) * angle1);
  start.y = center->y - (height / 2.0) * sin((M_PI / 180.0) * angle1);
  cairo_move_to (renderer->cr, start.x, start.y);
  cairo_arc (renderer->cr, center->x, center->y, 
             width > height ? height / 2.0 : width / 2.0, //FIXME
             (angle1 / 180) * G_PI, (angle2 / 180) * G_PI);
  cairo_line_to (renderer->cr, center->x, center->y);
  // FIXME : does it really close the path ??
  // i.e. line_to from center to start of arc
  cairo_close_path (renderer->cr);
  cairo_fill (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
_ellipse(DiaRenderer *self, 
         Point *center,
         real width, real height,
         Color *color,
         gboolean fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  double co;

  DIAG_NOTE(g_message("%s_ellipse %fx%f center @ %f,%f", 
            fill ? "fill" : "draw", width, height, center->x, center->y));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);
  
  //FIXME: how to make a perfect ellipse from a bezier ?
  co = sqrt(pow(width,2)/4 + pow(height,2)/4);

  cairo_new_path (renderer->cr);
  cairo_move_to (renderer->cr,
                 center->x, center->y - height/2);
  cairo_curve_to (renderer->cr,
                  center->x + co, center->y - height/2,
                  center->x + co, center->y + height/2,
                  center->x, center->y + height/2);
  cairo_curve_to (renderer->cr,
                  center->x - co, center->y + height/2,
                  center->x - co, center->y - height/2,
                  center->x, center->y - height/2);
  if (fill)
    cairo_fill (renderer->cr);
  else
    cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
draw_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color)
{
  _ellipse (self, center, width, height, color, FALSE);
}

static void
fill_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color)
{
  _ellipse (self, center, width, height, color, TRUE);
}

static void
_bezier(DiaRenderer *self, 
        BezPoint *points,
        int numpoints,
        Color *color,
        gboolean fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE(g_message("%s_bezier n:%d %fx%f ...", 
            fill ? "fill" : "draw", numpoints, points->p1.x, points->p1.y));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);

  cairo_new_path (renderer->cr);
  for (i = 0; i < numpoints; i++)
  {
    switch (points[i].type)
    {
    case BEZ_MOVE_TO:
      cairo_move_to (renderer->cr, points[i].p1.x, points[i].p1.y);
      break;
    case BEZ_LINE_TO:
      cairo_line_to (renderer->cr, points[i].p1.x, points[i].p1.y);
      break;
    case BEZ_CURVE_TO:
      cairo_curve_to (renderer->cr, 
                      points[i].p1.x, points[i].p1.y,
                      points[i].p2.x, points[i].p2.y,
                      points[i].p3.x, points[i].p3.y);
      break;
    default :
      g_assert_not_reached ();
    }
  }

  if (fill)
    cairo_fill (renderer->cr);
  else
    cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void
draw_bezier(DiaRenderer *self, 
            BezPoint *points,
            int numpoints,
            Color *color)
{
  _bezier (self, points, numpoints, color, FALSE);
}

static void
fill_bezier(DiaRenderer *self, 
            BezPoint *points,
            int numpoints,
            Color *color)
{
  _bezier (self, points, numpoints, color, TRUE);
}

static void
draw_string(DiaRenderer *self,
            const char *text,
            Point *pos, Alignment alignment,
            Color *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  cairo_text_extents_t extents;
  double x = 0, y = 0;
  int len = strlen(text);

  DIAG_NOTE(g_message("draw_string(%d) %f,%f %s", 
            len, pos->x, pos->y, text));

  if (len < 1) return; /* shouldn't this be handled by Dia's core ? */

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);
  cairo_text_extents (renderer->cr,
                      text,
                      &extents);

  y = pos->y; //??

  switch (alignment) {
  case ALIGN_LEFT:
    x = pos->x;
    break;
  case ALIGN_CENTER:
    x = pos->x - extents.width / 2;
    break;
  case ALIGN_RIGHT:
    x = pos->x - extents.width / 2;
    break;
  }

  cairo_move_to (renderer->cr, x, y);
  cairo_show_text (renderer->cr, text);
  DIAG_STATE(renderer->cr)
}

static void
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage image)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  cairo_surface_t *surface;
  guint8 *data;
  int w = dia_image_width(image);
  int h = dia_image_height(image);
  int rs = dia_image_rowstride(image);

  DIAG_NOTE(g_message("draw_image %fx%f [%d(%d),%d] @%f,%f", 
            width, height, w, rs, h, point->x, point->y));

  if (dia_image_rgba_data (image))
    {
      const guint32 *p1 = (const guint32*)dia_image_rgba_data (image);
      /* we need to make a copy to rearrange channels 
       * (also need to use malloc, cause Cairo insists to free() it)
       */
      guint8 *p2 = data = malloc (h * rs);
      int i;

      for (i = 0; i < (h * rs) / 4; i++)
        {
          p2[0] = p1[3];
          p2[1] = p1[0];
          p2[2] = p1[1];
          p2[3] = p1[2];
          p1+=4;
          p2+=4;
        }

      surface = cairo_surface_create_for_image (data, 
                                                CAIRO_FORMAT_ARGB32,
                                                w, h, rs);
      /* DON'T it's owned by cairo/pixman now
      free (data);
       */
    }
  else
    {
      guint8 *p, *p2;
      guint8 *p1 = data = dia_image_rgb_data (image);
      /* need to copy to be owned by cairo/pixman, urgh.
       * Also cairo wants RGB24 32 bit aligned
       */
      int x, y;

      p = p2 = malloc(h*w*4);
      for (y = 0; y < h; y++)
        {
          for (x = 0; x < w; x++)
            {
              /* apparently BGR is required */
              p2[x*4  ] = p1[x*3+2];
              p2[x*4+1] = p1[x*3+1];
              p2[x*4+2] = p1[x*3  ];
              p2[x*4+3] = 0xFF; /* should not matter */
            }
          p2 += (w*4);
          p1 += rs;
        }
      surface = cairo_surface_create_for_image (p, 
                                                CAIRO_FORMAT_RGB24,
                                                w, h, w*4);
      g_free (data);
    }
  cairo_save (renderer->cr);
  cairo_translate (renderer->cr, point->x, point->y);
  cairo_scale (renderer->cr, width/w, height/h);
  cairo_surface_set_filter (renderer->surface, CAIRO_FILTER_BEST);
  cairo_show_surface (renderer->cr, surface, w, h);
  cairo_restore (renderer->cr);
  cairo_surface_destroy (surface);

  DIAG_STATE(renderer->cr);
}

static void 
_rounded_rect (DiaRenderer *self,
               Point *topleft, Point *bottomright,
               Color *color, real radius,
               gboolean fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  real r2;

  radius = MIN(radius, (bottomright->x - topleft->x)/2);
  radius = MIN(radius, (bottomright->y - topleft->y)/2);
  r2 = radius/2;

  DIAG_NOTE(g_message("%s_rounded_rect %f,%f -> %f,%f, %f", 
            fill ? "fill" : "draw",
            topleft->x, topleft->y, bottomright->x, bottomright->y, radius));

  cairo_set_rgb_color (renderer->cr, color->red, color->green, color->blue);

  cairo_new_path (renderer->cr);
  cairo_move_to (renderer->cr,
                 topleft->x + radius, topleft->y);

  /* corners are _not_ control points, but halfway */
  cairo_line_to  (renderer->cr,
                  bottomright->x - radius, topleft->y);
  cairo_curve_to (renderer->cr,
                  bottomright->x - r2, topleft->y, bottomright->x, topleft->y + r2,
                  bottomright->x, topleft->y + radius);
  cairo_line_to  (renderer->cr,
                  bottomright->x, bottomright->y - radius);
  cairo_curve_to (renderer->cr,
                  bottomright->x, bottomright->y - r2, bottomright->x - r2, bottomright->y,
                  bottomright->x - radius, bottomright->y);
  cairo_line_to  (renderer->cr,
                  topleft->x + radius, bottomright->y);
  cairo_curve_to (renderer->cr,
                  topleft->x + r2, bottomright->y, topleft->x, bottomright->y - r2,
                  topleft->x, bottomright->y - radius);
  cairo_line_to  (renderer->cr,
                  topleft->x, topleft->y + radius); 
  cairo_curve_to (renderer->cr,
                  topleft->x, topleft->y + r2, topleft->x + r2, topleft->y,
                  topleft->x + radius, topleft->y);
  if (fill)
    cairo_fill (renderer->cr);
  else
    cairo_stroke (renderer->cr);
  DIAG_STATE(renderer->cr)
}

static void 
draw_rounded_rect (DiaRenderer *renderer,
                   Point *topleft, Point *bottomright,
                   Color *color, real radius)
{
  _rounded_rect (renderer, topleft, bottomright, color, radius, FALSE);
}

static void 
fill_rounded_rect (DiaRenderer *renderer,
                   Point *topleft, Point *bottomright,
                   Color *color, real radius)
{
  _rounded_rect (renderer, topleft, bottomright, color, radius, TRUE);
}

/* gobject boiler plate */
static void cairo_renderer_class_init (DiaCairoRendererClass *klass);

static gpointer parent_class = NULL;

GType
dia_cairo_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DiaCairoRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) cairo_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DiaCairoRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DiaCairoRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
cairo_renderer_finalize (GObject *object)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);

  cairo_destroy (renderer->cr);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cairo_renderer_class_init (DiaCairoRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = cairo_renderer_finalize;

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
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->fill_rounded_rect = fill_rounded_rect;
}

typedef enum OutputKind
{
  OUTPUT_PS = 1,
  OUTPUT_PNG,
} OutputKind;

/* dia export funtion */
static void
export_data(DiagramData *data, const gchar *filename, 
            const gchar *diafilename, void* user_data)
{
  DiaCairoRenderer *renderer;
  FILE *file;
  real width, height;
  OutputKind kind = (OutputKind)user_data;

  file = fopen(filename, "wb"); /* "wb" for binary! */

  if (file == NULL) {
    message_error(_("Can't open output file %s: %s\n"), filename, strerror(errno));
    return;
  }

  renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);
  renderer->dia = data; //FIXME: not sure if this a good idea

  switch (kind) {
#ifdef CAIRO_HAS_PS_SURFACE
  case OUTPUT_PS :
    width  = data->paper.width / 2.54;
    height = data->paper.height / 2.54;
    renderer->scale = data->paper.scaling;
    DIAG_NOTE(g_message ("PS Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_ps_surface_create (file,
                                                 width, height, // inches
                                                 75, 75); // pixels per inch
    break;
#endif  
#ifdef CAIRO_HAS_PNG_SURFACE
  case OUTPUT_PNG :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale;
    height = (data->extents.bottom - data->extents.top) * renderer->scale;

    DIAG_NOTE(g_message ("PNG Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_png_surface_create (file,
                                                  CAIRO_FORMAT_ARGB32,
                                                  (int)width, (int)height);
    /* although it is slow enough with out this prefer quality over speed */
    cairo_surface_set_filter (renderer->surface, CAIRO_FILTER_BEST);
    break;
#endif
  default :
    /* quite arbitrary, but consistent with ../pixbuf ;-) */
    renderer->scale = 20.0 * data->paper.scaling; 
    width  = (data->extents.right - data->extents.left) * renderer->scale;
    height = (data->extents.bottom - data->extents.top) * renderer->scale;
    DIAG_NOTE(g_message ("Image Surface %dx%d\n", (int)width, (int)height)); 
    renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_A8, (int)width, (int)height);
  }

  /* use extents */
  DIAG_NOTE(g_message("export_data extents %f,%f -> %f,%f", 
            data->extents.left, data->extents.top, data->extents.right, data->extents.bottom));

  data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
  fclose (file);
  g_object_unref(renderer);
}

static const gchar *ps_extensions[] = { "ps", NULL };
static DiaExportFilter ps_export_filter = {
    N_("Cairo PostScript"),
    ps_extensions,
    export_data,
    (void*)OUTPUT_PS
};

static const gchar *png_extensions[] = { "png", NULL };
static DiaExportFilter png_export_filter = {
    N_("Cairo PNG"),
    png_extensions,
    export_data,
    (void*)OUTPUT_PNG
};

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Cairo",
                            _("Cairo based Rendering"),
                            NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

#ifdef CAIRO_HAS_PS_SURFACE
  filter_register_export(&ps_export_filter);
#endif
#ifdef CAIRO_HAS_PNG_SURFACE
  filter_register_export(&png_export_filter);
#endif

  return DIA_PLUGIN_INIT_OK;
}
