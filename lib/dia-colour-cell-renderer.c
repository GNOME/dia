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

#include "dia-colour-cell-renderer.h"
#include "renderer/diacairo.h"
#include "diarenderer.h"


typedef struct _DiaColourCellRendererPrivate DiaColourCellRendererPrivate;
struct _DiaColourCellRendererPrivate {
  Color *colour;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaColourCellRenderer, dia_colour_cell_renderer, GTK_TYPE_CELL_RENDERER_TEXT)


enum {
  PROP_0,
  PROP_COLOUR,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_colour_cell_renderer_get_property (GObject    *object,
                                       guint       param_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  DiaColourCellRenderer *self = DIA_COLOUR_CELL_RENDERER (object);
  DiaColourCellRendererPrivate *priv = dia_colour_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_COLOUR:
      g_value_set_boxed (value, priv->colour);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
  }
}


static void
dia_colour_cell_renderer_set_property (GObject      *object,
                                       guint         param_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  DiaColourCellRenderer *self = DIA_COLOUR_CELL_RENDERER (object);
  DiaColourCellRendererPrivate *priv = dia_colour_cell_renderer_get_instance_private (self);

  switch (param_id) {
    case PROP_COLOUR:
      g_clear_pointer (&priv->colour, dia_colour_free);
      priv->colour = g_value_dup_boxed (value);
      if (priv->colour) {
        g_object_set (self,
                      "family", "monospace",
                      "weight", PANGO_WEIGHT_BOLD,
                      NULL);

        /* See http://web.umr.edu/~rhall/commentary/color_readability.htm for
        * explanation of this formula */
        if (((priv->colour->red * 255) * 299 +
             (priv->colour->green * 255) * 587 +
             (priv->colour->blue * 255) * 114) > 500 * 256 &&
            priv->colour->alpha > 0.4) {
          g_object_set (self,
                        "foreground", "#000000",
                        NULL);
        } else {
          g_object_set (self,
                        "foreground", "#FFFFFF",
                        NULL);
        }
      } else {
        g_object_set (self,
                      "family", NULL,
                      "foreground", NULL,
                      "weight", PANGO_WEIGHT_NORMAL,
                      NULL);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}


static void
dia_colour_cell_renderer_finalize (GObject *object)
{
  DiaColourCellRenderer *self = DIA_COLOUR_CELL_RENDERER (object);
  DiaColourCellRendererPrivate *priv = dia_colour_cell_renderer_get_instance_private (self);

  g_clear_pointer (&priv->colour, dia_colour_free);

  G_OBJECT_CLASS (dia_colour_cell_renderer_parent_class)->finalize (object);
}

// Taken from Gtk 3.24 (66f0bdee0adf422f23b5e0bb5addd6256958eb82)

static cairo_pattern_t *
get_checkered_pattern (void)
{
  /* need to respect pixman's stride being a multiple of 4 */
  static unsigned char data[8] = { 0xFF, 0x00, 0x00, 0x00,
                                   0x00, 0xFF, 0x00, 0x00 };
  static cairo_surface_t *checkered = NULL;
  cairo_pattern_t *pattern;

  if (checkered == NULL)
    checkered = cairo_image_surface_create_for_data (data,
                                                     CAIRO_FORMAT_A8,
                                                     2, 2, 4);

  pattern = cairo_pattern_create_for_surface (checkered);
  cairo_pattern_set_extend (pattern, CAIRO_EXTEND_REPEAT);
  cairo_pattern_set_filter (pattern, CAIRO_FILTER_NEAREST);

  return pattern;
}


static void
dia_colour_cell_renderer_render (GtkCellRenderer      *cell,
                                 cairo_t              *ctx,
                                 GtkWidget            *widget,
                                 const GdkRectangle   *background_area,
                                 const GdkRectangle   *cell_area,
                                 GtkCellRendererState  flags)
{
  DiaColourCellRenderer *self = DIA_COLOUR_CELL_RENDERER (cell);
  DiaColourCellRendererPrivate *priv = dia_colour_cell_renderer_get_instance_private (self);
  int width, height;
  int x, y;
  int xpad, ypad;

  // Ugly hack for the "More" and "Reset" rows
  if (!priv->colour) {
    GTK_CELL_RENDERER_CLASS (dia_colour_cell_renderer_parent_class)->render (cell,
                                                                             ctx,
                                                                             widget,
                                                                             background_area,
                                                                             cell_area,
                                                                             flags);

    return;
  }

  gtk_cell_renderer_get_padding (cell, &xpad, &ypad);

  width = cell_area->width - xpad * 2;
  height = cell_area->height - ypad * 2;
  x = (cell_area->x + xpad);
  y = (cell_area->y + ypad);

  cairo_rectangle (ctx, x, y, width, height);

  if (priv->colour->alpha < 0.99) {
    cairo_pattern_t *checks;
    cairo_matrix_t matrix;

    cairo_save (ctx);

    cairo_clip_preserve (ctx);

    cairo_set_source_rgb (ctx, 0.33, 0.33, 0.33);
    cairo_fill_preserve (ctx);

    checks = get_checkered_pattern ();
    cairo_matrix_init_scale (&matrix, 0.25, 0.25);
    cairo_pattern_set_matrix (checks, &matrix);

    cairo_set_source_rgb (ctx, 0.66, 0.66, 0.66);
    cairo_mask (ctx, checks);

    cairo_pattern_destroy (checks);

    cairo_restore (ctx);
  }

  cairo_set_source_rgba (ctx,
                         priv->colour->red,
                         priv->colour->green,
                         priv->colour->blue,
                         priv->colour->alpha);
  cairo_fill (ctx);

  GTK_CELL_RENDERER_CLASS (dia_colour_cell_renderer_parent_class)->render (cell,
                                                                           ctx,
                                                                           widget,
                                                                           background_area,
                                                                           cell_area,
                                                                           flags);
}


static void
dia_colour_cell_renderer_class_init (DiaColourCellRendererClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  object_class->set_property = dia_colour_cell_renderer_set_property;
  object_class->get_property = dia_colour_cell_renderer_get_property;
  object_class->finalize = dia_colour_cell_renderer_finalize;

  cell_class->render = dia_colour_cell_renderer_render;

  /**
   * DiaColourCellRenderer:colour:
   *
   * Since: 0.98
   */
  pspecs[PROP_COLOUR] =
    g_param_spec_boxed ("colour",
                        "Colour",
                        "Item colour",
                        DIA_TYPE_COLOUR,
                        G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_colour_cell_renderer_init (DiaColourCellRenderer *self)
{
}


GtkCellRenderer *
dia_colour_cell_renderer_new (void)
{
  return g_object_new (DIA_TYPE_COLOUR_CELL_RENDERER, NULL);
}
