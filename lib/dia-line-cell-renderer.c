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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 */

#include <config.h>
#include "intl.h"
#include <gtk/gtk.h>
#include "dia-line-cell-renderer.h"
#include "renderer/diacairo.h"
#include "diarenderer.h"


#define LINEWIDTH 2

typedef struct _DiaLineCellRendererPrivate DiaLineCellRendererPrivate;
struct _DiaLineCellRendererPrivate {
  DiaRenderer  *renderer;
  DiaLineStyle  line;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaLineCellRenderer, dia_line_cell_renderer, GTK_TYPE_CELL_RENDERER)


enum {
  PROP_0,
  PROP_LINE,
  PROP_RENDERER,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_line_cell_renderer_get_property (GObject    *object,
                                     guint       param_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  DiaLineCellRenderer *self = DIA_LINE_CELL_RENDERER (object);
  DiaLineCellRendererPrivate *priv = dia_line_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_RENDERER:
      g_value_set_object (value, priv->renderer);
      break;

    case PROP_LINE:
      g_value_set_enum (value, priv->line);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}


static void
dia_line_cell_renderer_set_property (GObject      *object,
                                     guint         param_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  DiaLineCellRenderer *self = DIA_LINE_CELL_RENDERER (object);
  DiaLineCellRendererPrivate *priv = dia_line_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_RENDERER:
      g_clear_object (&priv->renderer);
      priv->renderer = g_value_dup_object (value);
      break;

    case PROP_LINE:
      priv->line = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
dia_line_cell_renderer_finalize (GObject *object)
{
  DiaLineCellRenderer *self = DIA_LINE_CELL_RENDERER (object);
  DiaLineCellRendererPrivate *priv = dia_line_cell_renderer_get_instance_private (self);

  g_clear_object (&priv->renderer);

  G_OBJECT_CLASS (dia_line_cell_renderer_parent_class)->finalize (object);
}


static void
dia_line_cell_renderer_get_size (GtkCellRenderer *cell,
                                 GtkWidget       *widget,
                                 GdkRectangle    *cell_area,
                                 gint            *x_offset,
                                 gint            *y_offset,
                                 gint            *width,
                                 gint            *height)
{
  gint calc_width;
  gint calc_height;
  int  xpad;
  int  ypad;

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  calc_width  = (gint) xpad * 2 + 40;
  calc_height = (gint) ypad * 2 + 20;

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

  if (width)  *width  = calc_width;
  if (height) *height = calc_height;
}


static void
dia_line_cell_renderer_render (GtkCellRenderer      *cell,
                               GdkWindow            *window,
                               GtkWidget            *widget,
                               GdkRectangle         *background_area,
                               GdkRectangle         *cell_area,
                               GdkRectangle         *expose_area,
                               GtkCellRendererState  flags)
{
  DiaLineCellRenderer *self;
  DiaLineCellRendererPrivate *priv;
  Point from, to;
  Color colour_fg;
  GtkStyle *style = gtk_widget_get_style (widget);
  GdkColor fg = style->text[gtk_widget_get_state(widget)];
  GdkRectangle rect;
  int xpad, ypad;
  cairo_t *ctx;

  g_return_if_fail (DIA_IS_LINE_CELL_RENDERER (cell));

  self = DIA_LINE_CELL_RENDERER (cell);
  priv = dia_line_cell_renderer_get_instance_private (self);

  g_return_if_fail (DIA_CAIRO_IS_RENDERER (priv->renderer));

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  GDK_COLOR_TO_DIA (fg, colour_fg);

  gtk_cell_renderer_get_size (cell,
                              widget,
                              cell_area,
                              &rect.x,
                              &rect.y,
                              NULL,
                              NULL);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  ctx = gdk_cairo_create (GDK_DRAWABLE (window));

  rect.x += cell_area->x + xpad;
  rect.y += cell_area->y + ypad;
  rect.width = cell_area->width - (xpad * 2);
  rect.height = cell_area->height - (ypad * 2);

  ctx = gdk_cairo_create (GDK_DRAWABLE (window));

  to.y = from.y = rect.height / 2;
  from.x = 0;
  to.x = rect.width - LINEWIDTH;

  dia_renderer_begin_render (DIA_RENDERER (priv->renderer), NULL);
  dia_renderer_set_linewidth (DIA_RENDERER (priv->renderer),
                              LINEWIDTH);
  dia_renderer_set_linestyle (DIA_RENDERER (priv->renderer),
                              priv->line,
                              20.0);

  dia_renderer_draw_line (DIA_RENDERER (priv->renderer),
                          &from,
                          &to,
                          &colour_fg);

  dia_renderer_end_render (DIA_RENDERER (priv->renderer));

  cairo_set_source_surface (ctx, DIA_CAIRO_RENDERER (priv->renderer)->surface, rect.x, rect.y);
  gdk_cairo_rectangle (ctx, &rect);
  cairo_paint (ctx);

  cairo_destroy (ctx);
}


static void
dia_line_cell_renderer_class_init (DiaLineCellRendererClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->set_property = dia_line_cell_renderer_set_property;
  object_class->get_property = dia_line_cell_renderer_get_property;
  object_class->finalize = dia_line_cell_renderer_finalize;

  cell_class->get_size = dia_line_cell_renderer_get_size;
  cell_class->render = dia_line_cell_renderer_render;

  /**
   * DiaLineCellRenderer:renderer:
   *
   * Since: 0.98
   */
  pspecs[PROP_RENDERER] =
    g_param_spec_object ("renderer",
                         "Renderer",
                         "Renderer to draw lines",
                         DIA_TYPE_RENDERER,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  /**
   * DiaLineCellRenderer:line:
   *
   * Since: 0.98
   */
  pspecs[PROP_LINE] =
    g_param_spec_enum ("line",
                       "Line",
                       "Line style",
                       DIA_TYPE_LINE_STYLE,
                       DIA_LINE_STYLE_DEFAULT,
                       G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_line_cell_renderer_init (DiaLineCellRenderer *self)
{
  DiaLineCellRendererPrivate *priv = dia_line_cell_renderer_get_instance_private (self);

  priv->renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);

  DIA_CAIRO_RENDERER (priv->renderer)->surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
  DIA_CAIRO_RENDERER (priv->renderer)->with_alpha = TRUE;
}


GtkCellRenderer *
dia_line_cell_renderer_new (void)
{
  return g_object_new (DIA_TYPE_LINE_CELL_RENDERER, NULL);
}
