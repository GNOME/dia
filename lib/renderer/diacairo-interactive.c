/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacairo-interactive.c -- Cairo based interactive renderer
 * Copyright (C) 2006, Nguyen Thai Ngoc Duy
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "diacairo.h"

#include <gdk/gdk.h>

#include "color.h"
#include "diatransform.h"
#include "object.h"
#include "textline.h"
#include "diainteractiverenderer.h"

struct _DiaCairoInteractiveRenderer {
  DiaCairoRenderer parent_instance;

  /*< private >*/
  DiaRectangle *visible;
  real *zoom_factor;

  cairo_surface_t *surface;       /* The surface shown in this display  */
  guint32 width;                  /* The width of the surface in pixels */
  guint32 height;                 /* The height of the surface in pixels */
  cairo_region_t *clip_region;

  /* Selection box */
  gboolean has_selection;
  double selection_x;
  double selection_y;
  double selection_width;
  double selection_height;

  /** If non-NULL, this rendering is a highlighting with the given color. */
  Color *highlight_color;
};

static void dia_cairo_interactive_renderer_iface_init (DiaInteractiveRendererInterface* iface);

G_DEFINE_TYPE_WITH_CODE (DiaCairoInteractiveRenderer, dia_cairo_interactive_renderer, DIA_CAIRO_TYPE_RENDERER,
                         G_IMPLEMENT_INTERFACE (DIA_TYPE_INTERACTIVE_RENDERER, dia_cairo_interactive_renderer_iface_init))

enum {
  PROP_0,
  PROP_ZOOM,
  PROP_RECT
};

static void
dia_cairo_interactive_renderer_init (DiaCairoInteractiveRenderer *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  renderer->surface = NULL;

  renderer->highlight_color = NULL;
}

static void
dia_cairo_interactive_renderer_finalize (GObject *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (object);

  g_clear_pointer (&base_renderer->cr, cairo_destroy);
  g_clear_pointer (&renderer->surface, cairo_surface_destroy);

  G_OBJECT_CLASS (dia_cairo_interactive_renderer_parent_class)->finalize (object);
}

static void
dia_cairo_interactive_renderer_set_property (GObject      *object,
                                             guint         prop_id,
                                             const GValue *value,
                                             GParamSpec   *pspec)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  switch (prop_id) {
    case PROP_ZOOM:
      renderer->zoom_factor = g_value_get_pointer (value);
      break;
    case PROP_RECT:
      renderer->visible = g_value_get_pointer (value);
      break;
    default:
      break;
  }
}

static void
dia_cairo_interactive_renderer_get_property (GObject    *object,
                                             guint       prop_id,
                                             GValue     *value,
                                             GParamSpec *pspec)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  switch (prop_id) {
    case PROP_ZOOM:
      g_value_set_pointer (value, renderer->zoom_factor);
      break;
    case PROP_RECT:
      g_value_set_pointer (value, renderer->visible);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static int
dia_cairo_interactive_renderer_get_width_pixels (DiaInteractiveRenderer *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  return renderer->width;
}

static int
dia_cairo_interactive_renderer_get_height_pixels (DiaInteractiveRenderer *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  return renderer->height;
}

/*
 * Taken from gtk-3-24 as gtk2 gdk_cairo_region uses GdkRegion
 *
 * TODO: Use gtk3 implementation
 */
static void
_gdk_cairo_region (cairo_t              *cr,
                   const cairo_region_t *region)
{
  cairo_rectangle_int_t box;
  gint n_boxes, i;

  g_return_if_fail (cr != NULL);
  g_return_if_fail (region != NULL);

  n_boxes = cairo_region_num_rectangles (region);

  for (i = 0; i < n_boxes; i++)
    {
      cairo_region_get_rectangle (region, i, &box);
      cairo_rectangle (cr, box.x, box.y, box.width, box.height);
    }
}

static void
dia_cairo_interactive_renderer_begin_render (DiaRenderer        *self,
                                             const DiaRectangle *update)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (self);
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (self);

  g_return_if_fail (base_renderer->cr == NULL);

  g_clear_pointer (&base_renderer->surface, cairo_surface_destroy);
  base_renderer->cr = cairo_create (renderer->surface);

  /* Setup clipping for this sequence of render operations */
  /* Must be done before the scaling because the clip is in pixel coords */
  _gdk_cairo_region (base_renderer->cr, renderer->clip_region);
  cairo_clip (base_renderer->cr);

  cairo_scale (base_renderer->cr, *renderer->zoom_factor, *renderer->zoom_factor);
  cairo_translate (base_renderer->cr, -renderer->visible->left, -renderer->visible->top);

  /* second clipping */
  if (update) {
    real width = update->right - update->left;
    real height = update->bottom - update->top;
    cairo_rectangle (base_renderer->cr, update->left, update->top, width, height);
    cairo_clip (base_renderer->cr);
  }
  g_clear_object (&base_renderer->layout);
  base_renderer->layout = pango_cairo_create_layout (base_renderer->cr);

  cairo_set_fill_rule (base_renderer->cr, CAIRO_FILL_RULE_EVEN_ODD);

  /* should we set the background color? Or do nothing at all? */
  /* if this is drawn you can see 'clipping in action', outside of the clip it gets yellow ;) */
  cairo_set_source_rgba (base_renderer->cr, 1.0, 1.0, .8, 1.0);
  cairo_set_operator (base_renderer->cr, CAIRO_OPERATOR_OVER);
  cairo_rectangle (base_renderer->cr, 0, 0, renderer->width, renderer->height);
  cairo_fill (base_renderer->cr);
}

static void
dia_cairo_interactive_renderer_end_render (DiaRenderer *self)
{
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (self);

  cairo_show_page (base_renderer->cr);

  g_clear_pointer (&base_renderer->cr, cairo_destroy);
}

/* Get the width of the given text in cm */
static real
dia_cairo_interactive_renderer_get_text_width (DiaRenderer *object,
                                               const gchar *text,
                                               int          length)
{
  real result;
  TextLine *text_line;
  DiaFont *font;
  double font_height;

  font = dia_renderer_get_font (object, &font_height);

  if (length != g_utf8_strlen (text, -1)) {
    char *shorter;
    int ulen;
    ulen = g_utf8_offset_to_pointer (text, length) - text;
    if (!g_utf8_validate (text, ulen, NULL)) {
      g_warning ("Text at char %d not valid\n", length);
    }
    shorter = g_strndup (text, ulen);
    text_line = text_line_new (shorter, font, font_height);
    g_clear_pointer (&shorter, g_free);
  } else {
    text_line = text_line_new (text, font, font_height);
  }
  result = text_line_get_width (text_line);
  text_line_destroy (text_line);

  return result;
}

static real
calculate_relative_luminance (const Color *c)
{
  real R, G, B;

  R = (c->red <= 0.03928) ? c->red / 12.92 : pow ((c->red+0.055)/1.055, 2.4);
  G = (c->green <= 0.03928) ? c->green / 12.92 : pow ((c->green+0.055)/1.055, 2.4);
  B = (c->blue <= 0.03928) ? c->blue / 12.92 : pow ((c->blue+0.055)/1.055, 2.4);

  return 0.2126 * R + 0.7152 * G + 0.0722 * B;
}

static void
dia_cairo_interactive_renderer_draw_text_line (DiaRenderer  *self,
                                               TextLine     *text_line,
                                               Point        *pos,
                                               DiaAlignment  alignment,
                                               Color        *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (self);
  DiaCairoInteractiveRenderer *interactive = DIA_CAIRO_INTERACTIVE_RENDERER (self);

  if (interactive->highlight_color) {
    /* the high_light color is just taken as a hint, alternative needs
     * to have some contrast to cursor color (curently hard coded black)
     */
    static Color alternate_color = { 0.5, 0.5, 0.4 };
    real rl, cr1, cr2;

    /* just draw the box */
    real h = text_line_get_height (text_line);
    real w = text_line_get_width (text_line);
    real x = pos->x;
    real y = pos->y;

    y -= text_line_get_ascent (text_line);
    x -= text_line_get_alignment_adjustment (text_line, alignment);

    rl = calculate_relative_luminance (color) + 0.05;
    cr1 = calculate_relative_luminance (interactive->highlight_color) + 0.05;
    cr1 = (cr1 > rl) ? cr1 / rl : rl / cr1;
    cr2 = calculate_relative_luminance (&alternate_color) + 0.05;
    cr2 = (cr2 > rl) ? cr2 / rl : rl / cr2;

    /* use color giving the better contrast ratio, if necessary
     * http://www.w3.org/TR/2008/REC-WCAG20-20081211/#visual-audio-contrast-contrast
     */
    if (cr1 > cr2) {
      cairo_set_source_rgba (renderer->cr,
                             interactive->highlight_color->red,
                             interactive->highlight_color->green,
                             interactive->highlight_color->blue,
                             1.0);
    } else {
      cairo_set_source_rgba (renderer->cr,
                             alternate_color.red,
                             alternate_color.green,
                             alternate_color.blue,
                             1.0);
    }

    cairo_rectangle (renderer->cr, x, y, w, h);
    cairo_fill (renderer->cr);
  }

  DIA_RENDERER_CLASS (dia_cairo_interactive_renderer_parent_class)->draw_text_line (self, text_line, pos, alignment, color);
}

static void
dia_cairo_interactive_renderer_class_init (DiaCairoInteractiveRendererClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  gobject_class->finalize = dia_cairo_interactive_renderer_finalize;

  gobject_class->set_property = dia_cairo_interactive_renderer_set_property;
  gobject_class->get_property = dia_cairo_interactive_renderer_get_property;

  g_object_class_install_property (gobject_class,
                                   PROP_ZOOM,
                                   g_param_spec_pointer ("zoom",
                                                         _("Zoom pointer"),
                                                         _("Zoom pointer"),
                                                         G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class,
                                   PROP_RECT,
                                   g_param_spec_pointer ("rect",
                                                         _("Visible rect pointer"),
                                                         _("Visible rect pointer"),
                                                         G_PARAM_READWRITE));

  /* renderer members */
  renderer_class->begin_render = dia_cairo_interactive_renderer_begin_render;
  renderer_class->end_render   = dia_cairo_interactive_renderer_end_render;

  /* mostly for cursor placement */
  renderer_class->get_text_width = dia_cairo_interactive_renderer_get_text_width;
  /* highlight for text editing is special */
  renderer_class->draw_text_line = dia_cairo_interactive_renderer_draw_text_line;
}

static void
dia_cairo_interactive_renderer_clip_region_clear (DiaInteractiveRenderer *object)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);

  if (renderer->clip_region != NULL) {
    cairo_region_destroy (renderer->clip_region);
  }

  renderer->clip_region = cairo_region_create();
}

static void
dia_cairo_interactive_renderer_clip_region_add_rect (DiaInteractiveRenderer *object,
                                                     DiaRectangle           *rect)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  cairo_rectangle_int_t clip_rect;
  int x1,y1;
  int x2,y2;

  DiaTransform *transform;        /* Our link to the display settings */

  transform = dia_transform_new (renderer->visible,renderer->zoom_factor);
  dia_transform_coords (transform, rect->left, rect->top, &x1, &y1);
  dia_transform_coords (transform, rect->right, rect->bottom, &x2, &y2);
  g_clear_object (&transform);

  clip_rect.x = x1;
  clip_rect.y = y1;
  clip_rect.width = x2 - x1 + 1;
  clip_rect.height = y2 - y1 + 1;

  cairo_region_union_rectangle (renderer->clip_region, &clip_rect);
}

static void
dia_cairo_interactive_renderer_draw_pixel_line (DiaInteractiveRenderer *object,
                                                int                     x1,
                                                int                     y1,
                                                int                     x2,
                                                int                     y2,
                                                Color                  *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x1 + .5, y1u = y1 + .5, x2u = x2 + .5, y2u = y2 + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;

  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_move_to (renderer->cr, x1u, y1u);
  cairo_line_to (renderer->cr, x2u, y2u);
  cairo_stroke (renderer->cr);
}

static void
dia_cairo_interactive_renderer_draw_pixel_rect (DiaInteractiveRenderer *object,
                                                int                     x,
                                                int                     y,
                                                int                     width,
                                                int                     height,
                                                Color                  *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x + .5, y1u = y + .5, x2u = x + width + .5, y2u = y + height + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;

  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_rectangle (renderer->cr, x1u, y1u, x2u - x1u, y2u - y1u);
  cairo_stroke (renderer->cr);
}

static void
dia_cairo_interactive_renderer_fill_pixel_rect (DiaInteractiveRenderer *object,
                                                int                     x,
                                                int                     y,
                                                int                     width,
                                                int                     height,
                                                Color                  *color)
{
  DiaCairoRenderer *renderer = DIA_CAIRO_RENDERER (object);
  double x1u = x + .5, y1u = y + .5, x2u = x + width + .5, y2u = y + height + .5;
  double lw[2];
  lw[0] = 1; lw[1] = 0;

  cairo_device_to_user_distance (renderer->cr, &lw[0], &lw[1]);
  cairo_set_line_width (renderer->cr, lw[0]);

  cairo_device_to_user (renderer->cr, &x1u, &y1u);
  cairo_device_to_user (renderer->cr, &x2u, &y2u);

  cairo_set_source_rgba (renderer->cr, color->red, color->green, color->blue, color->alpha);
  cairo_rectangle (renderer->cr, x1u, y1u, x2u - x1u, y2u - y1u);
  cairo_fill (renderer->cr);
}

static void
dia_cairo_interactive_renderer_paint (DiaInteractiveRenderer *object,
                                      cairo_t                *ctx,
                                      int                     width,
                                      int                     height)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  double dashes[1] = {3};

  cairo_save (ctx);
  cairo_set_source_surface (ctx, renderer->surface, 0.0, 0.0);
  cairo_rectangle (ctx, 0, 0, width > 0 ? width : -width, height > 0 ? height : -height);
  cairo_clip (ctx);
  cairo_paint (ctx);

  /* If there should be a selection rectange */
  if (renderer->has_selection) {
    /* Use a dark gray */
    cairo_set_source_rgba (ctx, 0.1, 0.1, 0.1, 0.8);
    cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);
    cairo_set_line_width (ctx, 1);
    cairo_set_dash (ctx, dashes, 1, 0);

    /* Set the selected area */
    cairo_rectangle (ctx,
                     renderer->selection_x,
                     renderer->selection_y,
                     renderer->selection_width,
                     renderer->selection_height);
    /* Add a dashed gray outline */
    cairo_stroke_preserve (ctx);
    /* Very light blue tint fill */
    cairo_set_source_rgba (ctx, 0, 0, 0.85, 0.05);
    cairo_fill (ctx);
  }

  cairo_restore (ctx);
}

static void
dia_cairo_interactive_renderer_set_size (DiaInteractiveRenderer *object,
                                         gpointer                window,
                                         int                     width,
                                         int                      height)
{
  DiaCairoInteractiveRenderer *renderer = DIA_CAIRO_INTERACTIVE_RENDERER (object);
  DiaCairoRenderer *base_renderer = DIA_CAIRO_RENDERER (object);

  renderer->width = width;
  renderer->height = height;
  g_clear_pointer (&renderer->surface, cairo_surface_destroy);
  renderer->surface = gdk_window_create_similar_surface (GDK_WINDOW (window),
                                                         CAIRO_CONTENT_COLOR,
                                                         width, height);

  g_clear_pointer (&base_renderer->surface, cairo_surface_destroy);
}

/** Used as background? for text editing */
static Color text_edit_color = {1.0, 1.0, 0.7 };

static void
dia_cairo_interactive_renderer_draw_object_highlighted (DiaInteractiveRenderer *self,
                                                        DiaObject              *object,
                                                        DiaHighlightType        type)
{
  DiaCairoInteractiveRenderer *interactive = DIA_CAIRO_INTERACTIVE_RENDERER (self);

  switch (type) {
    case DIA_HIGHLIGHT_TEXT_EDIT:
      interactive->highlight_color = &text_edit_color;
      break;
    case DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN:
    case DIA_HIGHLIGHT_CONNECTIONPOINT:
    case DIA_HIGHLIGHT_NONE:
      interactive->highlight_color = NULL;
      break;
    default:
      g_return_if_reached ();
  }

  /* usually this method would need to draw the object twice,
   * once with highlight and once without. But due to our
   * draw_text_line implementation we only need one run */
  dia_object_draw (object, DIA_RENDERER (self));
  /* always reset when done with this object */
  interactive->highlight_color = NULL;
}

static void
dia_cairo_interactive_renderer_set_selection (DiaInteractiveRenderer *renderer,
                                              gboolean                has_selection,
                                              double                  x,
                                              double                  y,
                                              double                  width,
                                              double                  height)
{
  DiaCairoInteractiveRenderer *self = DIA_CAIRO_INTERACTIVE_RENDERER (renderer);

  self->has_selection = has_selection;
  self->selection_x = x;
  self->selection_y = y;
  self->selection_width = width;
  self->selection_height = height;
}

static void
dia_cairo_interactive_renderer_iface_init (DiaInteractiveRendererInterface* iface)
{
  iface->get_width_pixels        = dia_cairo_interactive_renderer_get_width_pixels;
  iface->get_height_pixels       = dia_cairo_interactive_renderer_get_height_pixels;
  iface->clip_region_clear       = dia_cairo_interactive_renderer_clip_region_clear;
  iface->clip_region_add_rect    = dia_cairo_interactive_renderer_clip_region_add_rect;
  iface->draw_pixel_line         = dia_cairo_interactive_renderer_draw_pixel_line;
  iface->draw_pixel_rect         = dia_cairo_interactive_renderer_draw_pixel_rect;
  iface->fill_pixel_rect         = dia_cairo_interactive_renderer_fill_pixel_rect;
  iface->paint                   = dia_cairo_interactive_renderer_paint;
  iface->set_size                = dia_cairo_interactive_renderer_set_size;
  iface->draw_object_highlighted = dia_cairo_interactive_renderer_draw_object_highlighted;
  iface->set_selection           = dia_cairo_interactive_renderer_set_selection;
}


DiaRenderer *
dia_cairo_interactive_renderer_new (void)
{
  return g_object_new (DIA_CAIRO_TYPE_INTERACTIVE_RENDERER, NULL);
}
