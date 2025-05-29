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

#define G_LOG_DOMAIN "DiaCairo"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

#include <pango/pangocairo.h>

#include <cairo.h>
/* some backend headers, win32 missing in official Cairo */
#include <cairo-svg.h>
#ifdef CAIRO_HAS_PS_SURFACE
#include <cairo-ps.h>
#endif
#ifdef CAIRO_HAS_PDF_SURFACE
#include <cairo-pdf.h>
#endif

#include "message.h"
#include "geometry.h"
#include "dia_image.h"
#include "diarenderer.h"
#include "filter.h"
#include "plug-ins.h"
#include "object.h" /* only for object->ops->draw */
#include "pattern.h"

#include "diacairo.h"

G_DEFINE_TYPE (DiaCairoRenderer, dia_cairo_renderer, DIA_TYPE_RENDERER)

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


static void ensure_minimum_one_device_unit (DiaCairoRenderer *renderer,
                                            real             *value);

/*
 * render functions
 */
static void
dia_cairo_renderer_begin_render (DiaRenderer *self, const DiaRectangle *update)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  real onedu = 0.0;
  real lmargin = 0.0, tmargin = 0.0;
  gboolean paginated = renderer->surface && /* only with our own pagination, not GtkPrint */
    cairo_surface_get_type (renderer->surface) == CAIRO_SURFACE_TYPE_PDF && !renderer->skip_show_page;
  Color background = color_white;

  if (renderer->surface && !renderer->cr) {
    renderer->cr = cairo_create (renderer->surface);
  } else {
    g_assert (renderer->cr);
  }

  /* remember current state, so we can start from new with every page */
  cairo_save (renderer->cr);

  if (paginated && renderer->dia) {
    DiagramData *data = renderer->dia;
    /* Dia's paper.width already contains the scale, cairo needs it without
     * Similar for margins, Dia's without, but cairo wants them?
     */
    real width = (data->paper.lmargin + data->paper.width * data->paper.scaling + data->paper.rmargin)
          * (72.0 / 2.54) + 0.5;
    real height = (data->paper.tmargin + data->paper.height * data->paper.scaling + data->paper.bmargin)
           * (72.0 / 2.54) + 0.5;
    /* "Changes the size of a PDF surface for the current (and
     * subsequent) pages." Pagination setup? */
    cairo_pdf_surface_set_size (renderer->surface, width, height);
    lmargin = data->paper.lmargin / data->paper.scaling;
    tmargin = data->paper.tmargin / data->paper.scaling;
  }

  cairo_scale (renderer->cr, renderer->scale, renderer->scale);
  /* to ensure no clipping at top/left we need some extra gymnastics,
   * otherwise a box with a line witdh one one pixel might loose the
   * top/left border as in bug #147386 */
  ensure_minimum_one_device_unit (renderer, &onedu);

  if (update && paginated) {
    cairo_rectangle (renderer->cr, lmargin, tmargin,
                     update->right - update->left, update->bottom - update->top);
    cairo_clip (renderer->cr);
    cairo_translate (renderer->cr, -update->left + lmargin, -update->top + tmargin);
  } else {
    if (renderer->dia) {
      cairo_translate (renderer->cr, -renderer->dia->extents.left + onedu, -renderer->dia->extents.top + onedu);
    }
  }
  cairo_set_antialias (renderer->cr, CAIRO_ANTIALIAS_BEST);

  /* clear background */
  if (renderer->dia) {
    background = renderer->dia->bg_color;
  }
  if (renderer->with_alpha) {
    cairo_set_source_rgba (renderer->cr,
                           background.red,
                           background.green,
                           background.blue,
                           0.0);
  } else {
    cairo_set_source_rgba (renderer->cr,
                           background.red,
                           background.green,
                           background.blue,
                           1.0);
  }
  cairo_paint (renderer->cr);
  if (renderer->with_alpha) {
    /* restore to default drawing */
    cairo_set_operator (renderer->cr, CAIRO_OPERATOR_OVER);
    cairo_set_source_rgba (renderer->cr,
                           background.red,
                           background.green,
                           background.blue,
                           1.0);
  }
  if (!renderer->layout) {
    renderer->layout = pango_cairo_create_layout (renderer->cr);
  }

  cairo_set_fill_rule (renderer->cr, CAIRO_FILL_RULE_EVEN_ODD);

#if 0 /* try to work around bug #341481 - no luck */
  {
    cairo_font_options_t *fo = cairo_font_options_create ();
    cairo_get_font_options (renderer->cr, fo);
    /* try to switch off kerning */
    cairo_font_options_set_hint_style (fo, CAIRO_HINT_STYLE_NONE);
    cairo_font_options_set_hint_metrics (fo, CAIRO_HINT_METRICS_OFF);

    cairo_set_font_options (renderer->cr, fo);
    cairo_font_options_destroy (fo);
#ifdef HAVE_PANGOCAIRO_H
    pango_cairo_update_context (renderer->cr, pango_layout_get_context (renderer->layout));
    pango_layout_context_changed (renderer->layout);
#endif
  }
#endif

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_end_render (DiaRenderer *self)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("end_render"));

  if (!renderer->skip_show_page) {
    cairo_show_page (renderer->cr);
  }
  /* pop current state, so we can start from new with every page */
  cairo_restore (renderer->cr);

  DIAG_STATE (renderer->cr)
}

/*!
 * \brief Advertize renderers capabilities
 *
 * Especially with cairo this should be 'all' so this function
 * is complaining if it will return FALSE
 * \memberof _DiaCairoRenderer
 */
static gboolean
dia_cairo_renderer_is_capable_to (DiaRenderer      *renderer,
                                  RenderCapability  cap)
{
  static RenderCapability warned = RENDER_HOLES;

  if (RENDER_HOLES == cap) {
    return TRUE;
  } else if (RENDER_ALPHA == cap) {
    return TRUE;
  } else if (RENDER_AFFINE == cap) {
    return TRUE;
  } else if (RENDER_PATTERN == cap) {
    return TRUE;
  }

  if (cap != warned) {
    g_warning ("New capability not supported by cairo??");
  }

  warned = cap;

  return FALSE;
}


/*!
 * \brief Remember the given pattern to use for consecutive fill
 * @param self explicit this pointer
 * @param pattern linear or radial gradient
 */
static void
dia_cairo_renderer_set_pattern (DiaRenderer *self,
                                DiaPattern  *pattern)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  g_set_object (&renderer->pattern, pattern);
}


static gboolean
_add_color_stop (real ofs, const Color *col, gpointer user_data)
{
  cairo_pattern_t *pat = (cairo_pattern_t *) user_data;

  cairo_pattern_add_color_stop_rgba (pat,
                                     ofs,
                                     col->red,
                                     col->green,
                                     col->blue,
                                     col->alpha);

  return TRUE;
}

static cairo_pattern_t *
_pattern_build_for_cairo (DiaPattern *pattern, const DiaRectangle *ext)
{
  cairo_pattern_t *pat;
  DiaPatternType type;
  guint flags;
  Point p1, p2;
  real r;

  g_return_val_if_fail (pattern != NULL, NULL);

  dia_pattern_get_settings (pattern, &type, &flags);
  dia_pattern_get_points (pattern, &p1, &p2);
  dia_pattern_get_radius (pattern, &r);

  switch (type) {
    case DIA_LINEAR_GRADIENT :
      pat = cairo_pattern_create_linear (p1.x, p1.y, p2.x, p2.y);
      break;
    case DIA_RADIAL_GRADIENT :
      pat = cairo_pattern_create_radial (p2.x, p2.y, 0.0, p1.x, p1.y, r);
      break;
    default :
      g_warning ("_pattern_build_for_cairo non such.");
      return NULL;
  }
  /* this must only be optionally done */
  if ((flags & DIA_PATTERN_USER_SPACE) == 0) {
    cairo_matrix_t matrix;
    real w = ext->right - ext->left;
    real h = ext->bottom - ext->top;
    cairo_matrix_init (&matrix, w, 0.0, 0.0, h, ext->left, ext->top);
    cairo_matrix_invert (&matrix);
    cairo_pattern_set_matrix (pat, &matrix);
  }

  if (flags & DIA_PATTERN_EXTEND_PAD) {
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_PAD);
  } else if (flags & DIA_PATTERN_EXTEND_REPEAT) {
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REPEAT);
  } else if (flags & DIA_PATTERN_EXTEND_REFLECT) {
    cairo_pattern_set_extend (pat, CAIRO_EXTEND_REFLECT);
  }

  dia_pattern_foreach (pattern, _add_color_stop, pat);

  return pat;
}

/*!
 * \brief Make use of the pattern if any
 */
static void
_dia_cairo_fill (DiaCairoRenderer *renderer, gboolean preserve)
{
  if (!renderer->pattern) {
    if (preserve) {
      cairo_fill_preserve (renderer->cr);
    } else {
      cairo_fill (renderer->cr);
    }
  } else {
    /* maybe we should cache the cairo-pattern */
    cairo_pattern_t *pat;
    DiaRectangle fe;

    /* Using the extents to scale the pattern is probably not correct */
    cairo_fill_extents (renderer->cr,
                        &fe.left,
                        &fe.top,
                        &fe.right,
                        &fe.bottom);

    pat = _pattern_build_for_cairo (renderer->pattern, &fe);
    cairo_set_source (renderer->cr, pat);
    if (preserve) {
      cairo_fill_preserve (renderer->cr);
    } else {
      cairo_fill (renderer->cr);
    }
    cairo_pattern_destroy (pat);
  }
}

/*!
 * \brief Render the given object optionally transformed by matrix
 * @param self explicit this pointer
 * @param object the _DiaObject to draw
 * @param matrix the transformation matrix to use or NULL
 */
static void
dia_cairo_renderer_draw_object (DiaRenderer *self,
                                DiaObject   *object,
                                DiaMatrix   *matrix)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  cairo_matrix_t before;

  if (matrix) {
    /* at least in SVG the intent of an invalid matrix is not rendering */
    if (!dia_matrix_is_invertible (matrix)) {
      return;
    }
    cairo_get_matrix (renderer->cr, &before);
    g_assert (sizeof (cairo_matrix_t) == sizeof (DiaMatrix));
    cairo_transform (renderer->cr, (cairo_matrix_t *) matrix);
  }
  dia_object_draw (object, DIA_RENDERER (renderer));
  if (matrix) {
    cairo_set_matrix (renderer->cr, &before);
  }
}

/*!
 * \brief Ensure a minimum of one device unit
 * Dia as well as many other drawing applications/libraries is using a
 * line with 0f 0.0 tho mean hairline. Cairo doe not have this capability
 * so this functions should be used to get the thinnest line possible.
 * \protected \memberof _DiaCairoRenderer
 */
static void
ensure_minimum_one_device_unit (DiaCairoRenderer *renderer, real *value)
{
  double ax = 1., ay = 1.;

  cairo_device_to_user_distance (renderer->cr, &ax, &ay);

  ax = MAX(ax, ay);
  if (*value < ax) {
    *value = ax;
  }
}

static void
dia_cairo_renderer_set_linewidth (DiaRenderer *self, real linewidth)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("set_linewidth %f", linewidth));

  /* make hairline? Everythnig below one device unit get the same width,
   * otherwise 0.0 may end up thicker than 0.0+epsilon
   */
  ensure_minimum_one_device_unit (renderer, &linewidth);

  cairo_set_line_width (renderer->cr, linewidth);

  DIAG_STATE (renderer->cr)
}


static void
dia_cairo_renderer_set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("set_linecaps %d", mode));

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_BUTT);
      break;
    case DIA_LINE_CAPS_ROUND:
      cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_ROUND);
      break;
    case DIA_LINE_CAPS_PROJECTING:
      cairo_set_line_cap (renderer->cr, CAIRO_LINE_CAP_SQUARE); /* ?? */
      break;
    default:
      g_warning ("DiaCairoRenderer : Unsupported caps mode specified!\n");
  }

  DIAG_STATE (renderer->cr)
}


static void
dia_cairo_renderer_set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("set_join %d", mode));

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_MITER);
      break;
    case DIA_LINE_JOIN_ROUND:
      cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_ROUND);
      break;
    case DIA_LINE_JOIN_BEVEL:
      cairo_set_line_join (renderer->cr, CAIRO_LINE_JOIN_BEVEL);
      break;
    default:
      g_warning ("DiaCairoRenderer : Unsupported join mode specified!\n");
  }

  DIAG_STATE (renderer->cr)
}


static void
dia_cairo_renderer_set_linestyle (DiaRenderer  *self,
                                  DiaLineStyle  mode,
                                  double        dash_length)
{
  /* dot = 10% of len */
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  double dash[6];

  DIAG_NOTE (g_message ("set_linestyle %d", mode));

  ensure_minimum_one_device_unit (renderer, &dash_length);

  /* line type */
  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      cairo_set_dash (renderer->cr, NULL, 0, 0);
      break;
    case DIA_LINE_STYLE_DASHED:
      dash[0] = dash_length;
      dash[1] = dash_length;
      cairo_set_dash (renderer->cr, dash, 2, 0);
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      dash[0] = dash_length;
      dash[1] = dash_length * 0.45;
      dash[2] = dash_length * 0.1;
      dash[3] = dash_length * 0.45;
      cairo_set_dash (renderer->cr, dash, 4, 0);
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      dash[0] = dash_length;
      dash[1] = dash_length * (0.8/3);
      dash[2] = dash_length * 0.1;
      dash[3] = dash_length * (0.8/3);
      dash[4] = dash_length * 0.1;
      dash[5] = dash_length * (0.8/3);
      cairo_set_dash (renderer->cr, dash, 6, 0);
      break;
    case DIA_LINE_STYLE_DOTTED:
      dash[0] = dash_length * 0.1;
      dash[1] = dash_length * 0.1;
      cairo_set_dash (renderer->cr, dash, 2, 0);
      break;
    default:
      g_warning ("DiaCairoRenderer : Unsupported line style specified!\n");
  }

  DIAG_STATE (renderer->cr)
}

/*!
 * \brief Set the fill style
 * The fill style is one of the areas, where the cairo library offers a lot
 * more the Dia currently uses. Cairo can render repeating patterns as well
 * as linear and radial gradients. As of this writing Dia just uses solid
 * color fill.
 * \memberof _DiaCairoRenderer
 */
static void
dia_cairo_renderer_set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  DIAG_NOTE (g_message ("set_fillstyle %d", mode));

  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      /* FIXME: how to set _no_ pattern ?
        * cairo_set_pattern (renderer->cr, NULL);
        */
      break;
    default:
      g_warning ("DiaCairoRenderer : Unsupported fill mode specified!\n");
  }

  DIAG_STATE (DIA_CAIRO_RENDERER (self)->cr)
}

/* There is a recurring bug with pangocairo related to kerning and font scaling.
 * See: https://bugzilla.gnome.org/buglist.cgi?quicksearch=341481+573261+700592
 * Rather than waiting for another fix let's try to implement the ultimate work
 * around. With Pango-1.32 and HarfBuzz the kludge in Pango is gone and apparently
 * substituted with a precision problem. If we now use huge fonts when talking
 * to Pango and downscale these with cairo it should work with all Pango versions.
 */
#define FONT_SIZE_TWEAK (72.0)

static void
dia_cairo_renderer_set_font (DiaRenderer *self, DiaFont *font, real height)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  /* pango/cairo wants the font size, not the (line-) height */
  real size = dia_font_get_size (font) * (height / dia_font_get_height (font));

  PangoFontDescription *pfd = pango_font_description_copy (dia_font_get_description (font));

  DIAG_NOTE (g_message ("set_font %f %s", height, dia_font_get_family (font)));

  /* select font and size */
  pango_font_description_set_absolute_size (pfd,
                                            (int) (size * FONT_SIZE_TWEAK * PANGO_SCALE));
  pango_layout_set_font_description (renderer->layout, pfd);
  pango_font_description_free (pfd);

  g_object_ref (font);
  g_clear_object (&renderer->font);

  renderer->font = font;
  renderer->font_height = height;
}

static void
dia_cairo_renderer_draw_line (DiaRenderer *self,
                              Point       *start,
                              Point       *end,
                              Color       *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("draw_line %f,%f -> %f, %f",
                        start->x,
                        start->y,
                        end->x,
                        end->y));

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);
  if (!renderer->stroke_pending) {
    /* use current point from previous drawing command */
    cairo_move_to (renderer->cr, start->x, start->y);
  }
  cairo_line_to (renderer->cr, end->x, end->y);
  if (!renderer->stroke_pending) {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_draw_polyline (DiaRenderer *self,
                                  Point       *points,
                                  int          num_points,
                                  Color       *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE (g_message ("draw_polyline n:%d %f,%f ...",
                        num_points,
                        points->x,
                        points->y));

  g_return_if_fail (1 < num_points);

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_new_path (renderer->cr);
  /* point data */
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  for (i = 1; i < num_points; i++) {
    cairo_line_to (renderer->cr, points[i].x, points[i].y);
  }
  cairo_stroke (renderer->cr);

  DIAG_STATE (renderer->cr)
}

static void
_polygon (DiaRenderer *self,
          Point       *points,
          int          num_points,
          Color       *color,
          gboolean     fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE (g_message ("%s_polygon n:%d %f,%f ...",
                        fill ? "fill" : "draw",
                        num_points,
                        points->x,
                        points->y));

  g_return_if_fail (1 < num_points);

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_new_path (renderer->cr);
  /* point data */
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  for (i = 1; i < num_points; i++) {
    cairo_line_to (renderer->cr, points[i].x, points[i].y);
  }
  cairo_line_to (renderer->cr, points[0].x, points[0].y);
  cairo_close_path (renderer->cr);
  if (fill) {
    _dia_cairo_fill (renderer, FALSE);
  } else {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_draw_polygon (DiaRenderer *self,
                                 Point       *points,
                                 int          num_points,
                                 Color       *fill,
                                 Color       *stroke)
{
  if (fill) {
    _polygon (self, points, num_points, fill, TRUE);
  }
  if (stroke) {
    _polygon (self, points, num_points, stroke, FALSE);
  }
}

static void
_rect (DiaRenderer *self,
       Point       *ul_corner,
       Point       *lr_corner,
       Color       *color,
       gboolean     fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("%s_rect %f,%f -> %f,%f",
                        fill ? "fill" : "draw",
                        ul_corner->x,
                        ul_corner->y,
                        lr_corner->x,
                        lr_corner->y));

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_rectangle (renderer->cr,
                   ul_corner->x,
                   ul_corner->y,
                   lr_corner->x - ul_corner->x,
                   lr_corner->y - ul_corner->y);

  if (fill) {
    _dia_cairo_fill (renderer, FALSE);
  } else {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_draw_rect (DiaRenderer *self,
                              Point       *ul_corner,
                              Point       *lr_corner,
                              Color       *fill,
                              Color       *stroke)
{
  if (fill) {
    _rect (self, ul_corner, lr_corner, fill, TRUE);
  }
  if (stroke) {
    _rect (self, ul_corner, lr_corner, stroke, FALSE);
  }
}

static void
dia_cairo_renderer_draw_arc (DiaRenderer *self,
                             Point       *center,
                             real         width,
                             real         height,
                             real         angle1,
                             real         angle2,
                             Color       *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  Point start;
  double a1, a2;
  real onedu = 0.0;

  DIAG_NOTE (g_message ("draw_arc %fx%f <%f,<%f",
                        width,
                        height,
                        angle1,
                        angle2));

  g_return_if_fail (!isnan (angle1) && !isnan (angle2));

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  if (!renderer->stroke_pending) {
    cairo_new_path (renderer->cr);
  }
  start.x = center->x + (width / 2.0)  * cos ((M_PI / 180.0) * angle1);
  start.y = center->y - (height / 2.0) * sin ((M_PI / 180.0) * angle1);
  if (!renderer->stroke_pending) {
    /* when activated the first current point must be set */
    cairo_move_to (renderer->cr, start.x, start.y);
  }
  a1 = - (angle1 / 180.0) * G_PI;
  a2 = - (angle2 / 180.0) * G_PI;
  /* FIXME: to handle width != height some cairo_scale/cairo_translate would be needed */
  ensure_minimum_one_device_unit (renderer, &onedu);
  /* FIXME2: with too small arcs cairo goes into an endless loop */
  if (height/2.0 > onedu && width/2.0 > onedu) {
    if (angle2 > angle1) {
      cairo_arc_negative (renderer->cr,
                          center->x, center->y,
                          width > height ? height / 2.0 : width / 2.0, /* FIXME 2nd radius */
                          a1,
                          a2);
    } else {
      cairo_arc (renderer->cr,
                 center->x,
                 center->y,
                 width > height ? height / 2.0 : width / 2.0, /* FIXME 2nd radius */
                 a1,
                 a2);
    }
  }
  if (!renderer->stroke_pending) {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_fill_arc (DiaRenderer *self,
                             Point       *center,
                             real         width,
                             real         height,
                             real         angle1,
                             real         angle2,
                             Color       *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  Point start;
  double a1, a2;

  DIAG_NOTE (g_message ("draw_arc %fx%f <%f,<%f",
                        width,
                        height,
                        angle1,
                        angle2));

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_new_path (renderer->cr);
  start.x = center->x + (width / 2.0)  * cos ((M_PI / 180.0) * angle1);
  start.y = center->y - (height / 2.0) * sin ((M_PI / 180.0) * angle1);
  cairo_move_to (renderer->cr, center->x, center->y);
  cairo_line_to (renderer->cr, start.x, start.y);
  a1 = - (angle1 / 180.0) * G_PI;
  a2 = - (angle2 / 180.0) * G_PI;
  /* FIXME: to handle width != height some cairo_scale/cairo_translate would be needed */
  if (angle2 > angle1) {
    cairo_arc_negative (renderer->cr,
                        center->x,
                        center->y,
                        width > height ? height / 2.0 : width / 2.0, /* XXX 2nd radius */
                        a1,
                        a2);
  } else {
    cairo_arc (renderer->cr,
               center->x,
               center->y,
               width > height ? height / 2.0 : width / 2.0, /* XXX 2nd radius */
               a1,
               a2);
  }
  cairo_line_to (renderer->cr, center->x, center->y);
  cairo_close_path (renderer->cr);
  _dia_cairo_fill (renderer, FALSE);

  DIAG_STATE (renderer->cr)
}

static void
_ellipse (DiaRenderer *self,
          Point       *center,
          real         width,
          real         height,
          Color       *color,
          gboolean     fill)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  DIAG_NOTE (g_message ("%s_ellipse %fx%f center @ %f,%f",
                        fill ? "fill" : "draw",
                        width,
                        height,
                        center->x,
                        center->y));

  /* avoid screwing cairo context - I'd say restore should fix it again, but it doesn't
   * (dia.exe:3152): DiaCairo-WARNING **: diacairo-renderer.c:254, invalid matrix (not invertible)
   */
  if (!(width > 0. && height > 0.)) {
    return;
  }

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_save (renderer->cr);
  /* don't create a line from the current point to the beginning
   * of the ellipse */
  cairo_new_sub_path (renderer->cr);
  /* copied straight from cairo's documentation, and fixed the bug there */
  cairo_translate (renderer->cr, center->x, center->y);
  cairo_scale (renderer->cr, width / 2., height / 2.);
  cairo_arc (renderer->cr, 0., 0., 1., 0., 2 * G_PI);
  cairo_restore (renderer->cr);

  if (fill) {
    _dia_cairo_fill (renderer, FALSE);
  } else {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_draw_ellipse (DiaRenderer *self,
                                 Point       *center,
                                 real         width,
                                 real         height,
                                 Color       *fill,
                                 Color       *stroke)
{
  if (fill) {
    _ellipse (self, center, width, height, fill, TRUE);
  }
  if (stroke) {
    _ellipse (self, center, width, height, stroke, FALSE);
  }
}

static void
_bezier (DiaRenderer *self,
         BezPoint    *points,
         int          numpoints,
         Color       *color,
         gboolean     fill,
         gboolean     closed)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int i;

  DIAG_NOTE (g_message ("%s_bezier n:%d %fx%f ...",
                        fill ? "fill" : "draw",
                        numpoints,
                        points->p1.x,
                        points->p1.y));

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);

  cairo_new_path (renderer->cr);
  for (i = 0; i < numpoints; i++) {
    switch (points[i].type) {
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
      default:
        g_assert_not_reached ();
    }
  }

  if (closed) {
    cairo_close_path(renderer->cr);
  }
  if (fill) {
    _dia_cairo_fill (renderer, FALSE);
  } else {
    cairo_stroke (renderer->cr);
  }

  DIAG_STATE(renderer->cr)
}

static void
dia_cairo_renderer_draw_bezier (DiaRenderer *self,
                                BezPoint    *points,
                                int          numpoints,
                                Color       *color)
{
  _bezier (self, points, numpoints, color, FALSE, FALSE);
}

static void
dia_cairo_renderer_draw_beziergon (DiaRenderer *self,
                                   BezPoint    *points,
                                   int          numpoints,
                                   Color       *fill,
                                   Color       *stroke)
{
  if (fill) {
    _bezier (self, points, numpoints, fill, TRUE, TRUE);
  }
  /* XXX: optimize if line_width is zero and fill==stroke */
  if (stroke) {
    _bezier (self, points, numpoints, stroke, FALSE, TRUE);
  }
}


static void
dia_cairo_renderer_draw_string (DiaRenderer  *self,
                                const char   *text,
                                Point        *pos,
                                DiaAlignment  alignment,
                                Color        *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int len = strlen (text);

  DIAG_NOTE (g_message ("draw_string(%d) %f,%f %s",
                        len, pos->x, pos->y, text));

  if (len < 1) {
    return;
    /* shouldn't this be handled by Dia's core ? */
  }

  cairo_set_source_rgba (renderer->cr,
                         color->red,
                         color->green,
                         color->blue,
                         color->alpha);
  cairo_save (renderer->cr);
  /* alignment calculation done by pangocairo? */
  pango_layout_set_alignment (renderer->layout, alignment == DIA_ALIGN_CENTRE ?
                                                  PANGO_ALIGN_CENTER : alignment == DIA_ALIGN_RIGHT ?
                                                    PANGO_ALIGN_RIGHT : PANGO_ALIGN_LEFT);
  pango_layout_set_text (renderer->layout, text, len);
  {
    PangoLayoutIter *iter = pango_layout_get_iter (renderer->layout);
    int bline = pango_layout_iter_get_baseline (iter);
    /* although we give the alignment above we need to adjust the start point */
    PangoRectangle extents;
    int shift;
    pango_layout_iter_get_line_extents (iter, NULL, &extents);
    shift = alignment == DIA_ALIGN_CENTRE ?
                            PANGO_RBEARING (extents) / 2: alignment == DIA_ALIGN_RIGHT ?
                              PANGO_RBEARING (extents) : 0;
    shift /= FONT_SIZE_TWEAK;
    bline /= FONT_SIZE_TWEAK;
    cairo_move_to (renderer->cr,
                   pos->x - (double) shift / PANGO_SCALE,
                   pos->y - (double) bline / PANGO_SCALE);
    pango_layout_iter_free (iter);
  }
  /* does this hide bug #341481? */
  cairo_scale (renderer->cr, 1.0 / FONT_SIZE_TWEAK, 1.0 / FONT_SIZE_TWEAK);
  pango_cairo_update_layout (renderer->cr, renderer->layout);

  pango_cairo_show_layout (renderer->cr, renderer->layout);
  /* restoring the previous scale */
  cairo_restore (renderer->cr);

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_draw_rotated_image (DiaRenderer *self,
                                       Point       *point,
                                       real         width,
                                       real         height,
                                       real         angle,
                                       DiaImage    *image)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  int w = dia_image_width (image);
  int h = dia_image_height (image);

  DIAG_NOTE (g_message ("draw_image %fx%f [%d(%d),%d] @%f,%f",
                        width, height, w, rs, h, point->x, point->y));

  cairo_save (renderer->cr);
  cairo_translate (renderer->cr, point->x, point->y);
  cairo_scale (renderer->cr, width / w, height / h);
  cairo_move_to (renderer->cr, 0.0, 0.0);
  cairo_set_source_surface (renderer->cr, dia_image_get_surface (image), 0.0, 0.0);

  if (angle != 0.0) {
    DiaMatrix rotate;
    Point center = { w / 2, h / 2  };

    dia_matrix_set_rotate_around (&rotate, -G_PI * angle / 180.0, &center);
    cairo_pattern_set_matrix (cairo_get_source (renderer->cr), (cairo_matrix_t *) &rotate);
  }
#if 0
  /*
   * CAIRO_FILTER_FAST: aka. CAIRO_FILTER_NEAREST
   * CAIRO_FILTER_GOOD: maybe bilinear, "reasonable-performance filter" (default?)
   * CAIRO_FILTER_BEST: "may not be suitable for interactive use"
   */
  cairo_pattern_set_filter (cairo_get_source (renderer->cr), CAIRO_FILTER_BILINEAR);
#endif
  cairo_paint (renderer->cr);
  cairo_restore (renderer->cr);

  DIAG_STATE (renderer->cr);
}

static void
dia_cairo_renderer_draw_image (DiaRenderer *self,
                               Point       *point,
                               real         width,
                               real         height,
                               DiaImage    *image)
{
  dia_cairo_renderer_draw_rotated_image (self,
                                         point,
                                         width,
                                         height,
                                         0.0,
                                         image);
}

/*!
 * \brief Fill and/or stroke a rectangle with rounded corner
 * Implemented to avoid seams between arcs and lines caused by the base class
 * working in real which than gets rounded independently to int here
 * \memberof _DiaCairoRenderer
 */
static void
dia_cairo_renderer_draw_rounded_rect (DiaRenderer *self,
                                      Point       *ul_corner,
                                      Point       *lr_corner,
                                      Color       *fill,
                                      Color       *stroke,
                                      real         radius)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  real r2 = (lr_corner->x - ul_corner->x) / 2.0;
  radius = MIN (r2, radius);
  r2 = (lr_corner->y - ul_corner->y) / 2.0;
  radius = MIN (r2, radius);
  if (radius < 0.0001) {
    dia_cairo_renderer_draw_rect (self, ul_corner, lr_corner, fill, stroke);
    return;
  }
  g_return_if_fail (stroke != NULL || fill != NULL);
  /* use base class implementation to create a path */
  cairo_new_path (renderer->cr);
  cairo_move_to (renderer->cr, ul_corner->x + radius, ul_corner->y);
  renderer->stroke_pending = TRUE;
  /* only stroke, no fill gives us the contour */
  DIA_RENDERER_CLASS (dia_cairo_renderer_parent_class)->draw_rounded_rect (self,
                                                                           ul_corner,
                                                                           lr_corner,
                                                                           NULL,
                                                                           stroke ? stroke : fill,
                                                                           radius);
  renderer->stroke_pending = FALSE;
  cairo_close_path (renderer->cr);
  if (fill) { /* if a stroke follows preserve the path */
    cairo_set_source_rgba (renderer->cr, fill->red, fill->green, fill->blue, fill->alpha);
    _dia_cairo_fill (renderer, stroke ? TRUE : FALSE);
  }
  if (stroke) {
    cairo_set_source_rgba (renderer->cr, stroke->red, stroke->green, stroke->blue, stroke->alpha);
    cairo_stroke (renderer->cr);
  }
}

static void
dia_cairo_renderer_draw_rounded_polyline (DiaRenderer *self,
                                          Point       *points,
                                          int          num_points,
                                          Color       *color,
                                          real         radius)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);

  cairo_new_path (renderer->cr);
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  /* use base class implementation */
  renderer->stroke_pending = TRUE;
  /* set the starting point to avoid move-to in between */
  cairo_move_to (renderer->cr, points[0].x, points[0].y);
  DIA_RENDERER_CLASS (dia_cairo_renderer_parent_class)->draw_rounded_polyline (self,
                                                                               points,
                                                                               num_points,
                                                                               color,
                                                                               radius);
  renderer->stroke_pending = FALSE;
  cairo_stroke (renderer->cr);

  DIAG_STATE (renderer->cr)
}

static void
dia_cairo_renderer_init (DiaCairoRenderer *renderer)
{
  renderer->scale = 1.0;
}

static void
dia_cairo_renderer_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  DiaCairoRenderer *self = DIA_CAIRO_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      dia_cairo_renderer_set_font (DIA_RENDERER (self),
                                   g_value_get_object (value),
                                   self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      dia_cairo_renderer_set_font (DIA_RENDERER (self),
                                   self->font,
                                   g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_cairo_renderer_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  DiaCairoRenderer *self = DIA_CAIRO_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_cairo_renderer_finalize (GObject *object)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);

  g_clear_pointer (&renderer->cr, cairo_destroy);
  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  g_clear_object (&renderer->layout);
  g_clear_object (&renderer->font);

  G_OBJECT_CLASS (dia_cairo_renderer_parent_class)->finalize (object);
}


static void
dia_cairo_renderer_class_init (DiaCairoRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  object_class->set_property = dia_cairo_renderer_set_property;
  object_class->get_property = dia_cairo_renderer_get_property;
  object_class->finalize = dia_cairo_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = dia_cairo_renderer_begin_render;
  renderer_class->end_render   = dia_cairo_renderer_end_render;
  renderer_class->draw_object  = dia_cairo_renderer_draw_object;

  renderer_class->set_linewidth  = dia_cairo_renderer_set_linewidth;
  renderer_class->set_linecaps   = dia_cairo_renderer_set_linecaps;
  renderer_class->set_linejoin   = dia_cairo_renderer_set_linejoin;
  renderer_class->set_linestyle  = dia_cairo_renderer_set_linestyle;
  renderer_class->set_fillstyle  = dia_cairo_renderer_set_fillstyle;

  renderer_class->draw_line    = dia_cairo_renderer_draw_line;
  renderer_class->draw_polygon = dia_cairo_renderer_draw_polygon;
  renderer_class->draw_arc     = dia_cairo_renderer_draw_arc;
  renderer_class->fill_arc     = dia_cairo_renderer_fill_arc;
  renderer_class->draw_ellipse = dia_cairo_renderer_draw_ellipse;

  renderer_class->draw_string  = dia_cairo_renderer_draw_string;
  renderer_class->draw_image   = dia_cairo_renderer_draw_image;

  /* medium level functions */
  renderer_class->draw_rect      = dia_cairo_renderer_draw_rect;
  renderer_class->draw_polyline  = dia_cairo_renderer_draw_polyline;

  renderer_class->draw_bezier    = dia_cairo_renderer_draw_bezier;
  renderer_class->draw_beziergon = dia_cairo_renderer_draw_beziergon;

  /* highest level functions */
  renderer_class->draw_rounded_rect     = dia_cairo_renderer_draw_rounded_rect;
  renderer_class->draw_rounded_polyline = dia_cairo_renderer_draw_rounded_polyline;
  renderer_class->draw_rotated_image    = dia_cairo_renderer_draw_rotated_image;

  /* other */
  renderer_class->is_capable_to = dia_cairo_renderer_is_capable_to;
  renderer_class->set_pattern   = dia_cairo_renderer_set_pattern;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}
