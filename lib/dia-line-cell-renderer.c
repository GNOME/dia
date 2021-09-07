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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "dia-line-cell-renderer.h"
#include "renderer/diacairo.h"
#include "diarenderer.h"


#define LINEWIDTH 2

typedef struct _DiaLineCellRendererPrivate DiaLineCellRendererPrivate;
struct _DiaLineCellRendererPrivate {
  DiaLineStyle  line;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaLineCellRenderer, dia_line_cell_renderer, GTK_TYPE_CELL_RENDERER)


enum {
  PROP_0,
  PROP_LINE,
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
    case PROP_LINE:
      priv->line = g_value_get_enum (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
dia_line_cell_renderer_get_size (GtkCellRenderer *cell,
                                 GtkWidget       *widget,
                                 const GdkRectangle *cell_area,
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
                               cairo_t              *ctx,
                               GtkWidget            *widget,
                               const GdkRectangle   *background_area,
                               const GdkRectangle   *cell_area,
                               GtkCellRendererState  flags)
{
  DiaLineCellRenderer *self;
  DiaLineCellRendererPrivate *priv;
  DiaRenderer *renderer;
  Point from, to;
  Color colour_fg;
  GtkStyleContext *style = gtk_widget_get_style_context (widget);
  GdkRGBA fg;
  int x, y, width, height, xpad, ypad;

  gtk_style_context_get_color (style,
                               gtk_widget_get_state_flags (widget),
                               &fg);

  g_return_if_fail (DIA_IS_LINE_CELL_RENDERER (cell));

  self = DIA_LINE_CELL_RENDERER (cell);
  priv = dia_line_cell_renderer_get_instance_private (self);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  GDK_COLOR_TO_DIA (fg, colour_fg);

  x = cell_area->x + xpad;
  y = cell_area->y + ypad;
  width = cell_area->width - (xpad * 2);
  height = cell_area->height - (ypad * 2);

  to.y = from.y = y + (height / 2);
  from.x = x;
  to.x = x + width - LINEWIDTH;

  renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);

  DIA_CAIRO_RENDERER (renderer)->cr = cairo_reference (ctx);
  DIA_CAIRO_RENDERER (renderer)->with_alpha = TRUE;

  dia_renderer_begin_render (DIA_RENDERER (renderer), NULL);
  dia_renderer_set_linewidth (DIA_RENDERER (renderer),
                              LINEWIDTH);
  dia_renderer_set_linestyle (DIA_RENDERER (renderer),
                              priv->line,
                              20.0);

  dia_renderer_draw_line (DIA_RENDERER (renderer),
                          &from,
                          &to,
                          &colour_fg);

  dia_renderer_end_render (DIA_RENDERER (renderer));

  g_clear_object (&renderer);
}


static void
dia_line_cell_renderer_class_init (DiaLineCellRendererClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->set_property = dia_line_cell_renderer_set_property;
  object_class->get_property = dia_line_cell_renderer_get_property;

  cell_class->get_size = dia_line_cell_renderer_get_size;
  cell_class->render = dia_line_cell_renderer_render;

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
}


GtkCellRenderer *
dia_line_cell_renderer_new (void)
{
  return g_object_new (DIA_TYPE_LINE_CELL_RENDERER, NULL);
}
