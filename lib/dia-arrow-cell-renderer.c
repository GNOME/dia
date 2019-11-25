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
#include "dia-arrow-cell-renderer.h"
#include "renderer/diacairo.h"
#include "diarenderer.h"
#include "arrows.h"

#define ARROW_LINEWIDTH 2

typedef struct _DiaArrowCellRendererPrivate DiaArrowCellRendererPrivate;
struct _DiaArrowCellRendererPrivate {
  DiaRenderer *renderer;
  Arrow       *arrow;
  gboolean     point_left;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaArrowCellRenderer, dia_arrow_cell_renderer, GTK_TYPE_CELL_RENDERER)


enum {
  PROP_0,
  PROP_ARROW,
  PROP_RENDERER,
  PROP_POINT_LEFT,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_arrow_cell_renderer_get_property (GObject    *object,
                                      guint       param_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DiaArrowCellRenderer *self = DIA_ARROW_CELL_RENDERER (object);
  DiaArrowCellRendererPrivate *priv = dia_arrow_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_RENDERER:
      g_value_set_object (value, priv->renderer);
      break;

    case PROP_ARROW:
      g_value_set_boxed (value, priv->arrow);
      break;

    case PROP_POINT_LEFT:
      g_value_set_boolean (value, priv->point_left);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}


static void
dia_arrow_cell_renderer_set_property (GObject      *object,
                                          guint         param_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  DiaArrowCellRenderer *self = DIA_ARROW_CELL_RENDERER (object);
  DiaArrowCellRendererPrivate *priv = dia_arrow_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_RENDERER:
      g_clear_object (&priv->renderer);
      priv->renderer = g_value_dup_object (value);
      break;

    case PROP_ARROW:
      g_clear_pointer (&priv->arrow, dia_arrow_free);
      priv->arrow = g_value_dup_boxed (value);
      break;

    case PROP_POINT_LEFT:
      priv->point_left = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
dia_arrow_cell_renderer_finalize (GObject *object)
{
  DiaArrowCellRenderer *self = DIA_ARROW_CELL_RENDERER (object);
  DiaArrowCellRendererPrivate *priv = dia_arrow_cell_renderer_get_instance_private (self);

  g_clear_object (&priv->renderer);
  g_clear_pointer (&priv->arrow, dia_arrow_free);

  G_OBJECT_CLASS (dia_arrow_cell_renderer_parent_class)->finalize (object);
}


static void
dia_arrow_cell_renderer_get_size (GtkCellRenderer *cell,
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
dia_arrow_cell_renderer_render (GtkCellRenderer      *cell,
                                GdkWindow            *window,
                                GtkWidget            *widget,
                                GdkRectangle         *background_area,
                                GdkRectangle         *cell_area,
                                GdkRectangle         *expose_area,
                                GtkCellRendererState  flags)
{
  DiaArrowCellRenderer *self = DIA_ARROW_CELL_RENDERER (cell);
  DiaArrowCellRendererPrivate *priv = dia_arrow_cell_renderer_get_instance_private (self);
  Point from, to;
  Point move_arrow, move_line, arrow_head;
  gint width, height;
  gint x, y;
  cairo_t *ctx;
  int xpad, ypad;
  Arrow tmp_arrow;
  Color colour_fg;
  Color colour_bg = { 0.0, 0.0, 0.0, 0.0 };
  GtkStyle *style = gtk_widget_get_style (widget);
  GdkColor fg = style->text[gtk_widget_get_state(widget)];

  GDK_COLOR_TO_DIA (fg, colour_fg);

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  ctx = gdk_cairo_create (GDK_DRAWABLE (window));

  g_return_if_fail (DIA_CAIRO_IS_RENDERER (priv->renderer));

  width = cell_area->width - xpad * 2;
  height = cell_area->height - ypad * 2;
  x = (cell_area->x + xpad);
  y = (cell_area->y + ypad);

  to.y = from.y = height/2;
  if (priv->point_left) {
    from.x = width - ARROW_LINEWIDTH;
    to.x = 0;
  } else {
    from.x = 0;
    to.x = width - ARROW_LINEWIDTH;
  }

  /* here we must do some acrobaticts and construct Arrow type
    * variable
    */
  tmp_arrow.type = priv->arrow->type;
  tmp_arrow.length = .75 * ((double) height - ARROW_LINEWIDTH);
  tmp_arrow.width = .75 * ((double) height - ARROW_LINEWIDTH);

  /* and here we calculate new arrow start and end of line points */
  calculate_arrow_point (&tmp_arrow,
                         &from, &to,
                         &move_arrow,
                         &move_line,
                         ARROW_LINEWIDTH);
  arrow_head = to;
  point_add (&arrow_head, &move_arrow);
  point_add (&to, &move_line);

  dia_renderer_begin_render (DIA_RENDERER (priv->renderer), NULL);
  dia_renderer_set_linewidth (DIA_RENDERER (priv->renderer),
                              ARROW_LINEWIDTH);

  dia_renderer_draw_line (DIA_RENDERER (priv->renderer),
                          &from,
                          &to,
                          &colour_fg);
  arrow_draw (DIA_RENDERER (priv->renderer),
              tmp_arrow.type,
              &arrow_head,
              &from,
              tmp_arrow.length,
              tmp_arrow.width,
              ARROW_LINEWIDTH,
              &colour_fg,
              &colour_bg);

  dia_renderer_end_render (DIA_RENDERER (priv->renderer));

  cairo_set_source_surface (ctx, DIA_CAIRO_RENDERER (priv->renderer)->surface, x, y);
  cairo_paint (ctx);
}


static void
dia_arrow_cell_renderer_class_init (DiaArrowCellRendererClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->set_property = dia_arrow_cell_renderer_set_property;
  object_class->get_property = dia_arrow_cell_renderer_get_property;
  object_class->finalize = dia_arrow_cell_renderer_finalize;

  cell_class->get_size = dia_arrow_cell_renderer_get_size;
  cell_class->render = dia_arrow_cell_renderer_render;

  /**
   * DiaArrowCellRenderer:renderer:
   *
   * Since: 0.98
   */
  pspecs[PROP_RENDERER] =
    g_param_spec_object ("renderer",
                         "Renderer",
                         "Renderer to draw arrows",
                         DIA_TYPE_RENDERER,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  /**
   * DiaArrowCellRenderer:arrow:
   *
   * Since: 0.98
   */
  pspecs[PROP_ARROW] =
    g_param_spec_boxed ("arrow",
                        "Arrow",
                        "Arrow to draw",
                        DIA_TYPE_ARROW,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  /**
   * DiaArrowCellRenderer:point-left:
   *
   * Since: 0.98
   */
  pspecs[PROP_POINT_LEFT] =
    g_param_spec_boolean ("point-left",
                          "Point Left",
                          "Arrow to should be pointing to the left",
                          FALSE,
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_arrow_cell_renderer_init (DiaArrowCellRenderer *self)
{
  DiaArrowCellRendererPrivate *priv = dia_arrow_cell_renderer_get_instance_private (self);

  priv->point_left = FALSE;

  priv->renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);

  DIA_CAIRO_RENDERER (priv->renderer)->surface = cairo_recording_surface_create (CAIRO_CONTENT_COLOR_ALPHA, NULL);
  DIA_CAIRO_RENDERER (priv->renderer)->with_alpha = TRUE;
}


GtkCellRenderer *
dia_arrow_cell_renderer_new (void)
{
  return g_object_new (DIA_TYPE_ARROW_CELL_RENDERER, NULL);
}
