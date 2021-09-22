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
 */

#include "dia-arrow-preview.h"
#include "renderer/diacairo.h"

struct _DiaArrowPreview {
  GtkMisc misc;
  ArrowType atype;
  gboolean left;
};

G_DEFINE_TYPE (DiaArrowPreview, dia_arrow_preview, GTK_TYPE_MISC)


/**
 * dia_arrow_preview_expose:
 * @widget: The widget to display.
 * @event: The event that caused the call.
 *
 * Expose handle for the arrow preview widget.
 *
 * The expose handler gets called when the Arrow needs to be drawn.
 *
 * Returns: %TRUE always.
 *
 * Since: dawn-of-time
 */
static int
dia_arrow_preview_expose (GtkWidget *widget, GdkEventExpose *event)
{
  if (gtk_widget_is_drawable (widget)) {
    Point from, to;
    Point move_arrow, move_line, arrow_head;
    DiaCairoRenderer *renderer;
    DiaArrowPreview *arrow = DIA_ARROW_PREVIEW (widget);
    Arrow arrow_type;
    GtkMisc *misc = GTK_MISC (widget);
    int width, height;
    int x, y;
    GdkWindow *win;
    int linewidth = 2;
    cairo_surface_t *surface;
    cairo_t *ctx;
    GtkAllocation alloc;
    int xpad, ypad;

    gtk_widget_get_allocation (widget, &alloc);
    gtk_misc_get_padding (misc, &xpad, &ypad);

    width = alloc.width - xpad * 2;
    height = alloc.height - ypad * 2;
    x = (alloc.x + xpad);
    y = (alloc.y + ypad);

    win = gtk_widget_get_window (widget);

    to.y = from.y = height / 2;
    if (arrow->left) {
      from.x = width - linewidth;
      to.x = 0;
    } else {
      from.x = 0;
      to.x = width - linewidth;
    }

    /* here we must do some acrobaticts and construct Arrow type
     * variable
     */
    arrow_type.type = arrow->atype;
    arrow_type.length = .75 * ((double) height - linewidth);
    arrow_type.width = .75 * ((double) height - linewidth);

    /* and here we calculate new arrow start and end of line points */
    calculate_arrow_point (&arrow_type,
                           &from,
                           &to,
                           &move_arrow,
                           &move_line,
                           linewidth);
    arrow_head = to;
    point_add (&arrow_head, &move_arrow);
    point_add (&to, &move_line);

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

    renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);
    renderer->with_alpha = TRUE;
    renderer->surface = cairo_surface_reference (surface);

    dia_renderer_begin_render (DIA_RENDERER (renderer), NULL);
    dia_renderer_set_linewidth (DIA_RENDERER (renderer), linewidth);
    {
      Color colour_bg, colour_fg;
      GtkStyle *style = gtk_widget_get_style (widget);
      /* the text colors are the best approximation to what we had */
      GdkColor bg = style->base[gtk_widget_get_state (widget)];
      GdkColor fg = style->text[gtk_widget_get_state (widget)];

      GDK_COLOR_TO_DIA (bg, colour_bg);
      GDK_COLOR_TO_DIA (fg, colour_fg);

      dia_renderer_draw_line (DIA_RENDERER (renderer), &from, &to, &colour_fg);
      dia_arrow_draw (&arrow_type,
                      DIA_RENDERER (renderer),
                      &arrow_head,
                      &from,
                      linewidth,
                      &colour_fg,
                      &colour_bg);
    }
    dia_renderer_end_render (DIA_RENDERER (renderer));
    g_clear_object (&renderer);

    ctx = gdk_cairo_create (win);
    cairo_set_source_surface (ctx, surface, x, y);
    cairo_paint (ctx);
  }

  return TRUE;
}


static void
dia_arrow_preview_class_init (DiaArrowPreviewClass *class)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  widget_class->expose_event = dia_arrow_preview_expose;
}


static void
dia_arrow_preview_init (DiaArrowPreview *arrow)
{
  int xpad, ypad;

  gtk_widget_set_has_window (GTK_WIDGET (arrow), FALSE);

  gtk_misc_get_padding (GTK_MISC (arrow), &xpad, &ypad);

  gtk_widget_set_size_request (GTK_WIDGET (arrow),
                               40 + xpad * 2,
                               20 + ypad * 2);

  arrow->atype = ARROW_NONE;
  arrow->left = TRUE;
}


/**
 * dia_arrow_preview_new:
 * @atype: The type of arrow to start out selected with.
 * @left: If %TRUE, this preview will point to the left.
 *
 * Create a new arrow preview widget.
 *
 * Returns: A new widget.
 */
GtkWidget *
dia_arrow_preview_new (ArrowType atype, gboolean left)
{
  DiaArrowPreview *arrow = g_object_new (DIA_TYPE_ARROW_PREVIEW, NULL);

  arrow->atype = atype;
  arrow->left = left;

  return GTK_WIDGET (arrow);
}


/**
 * dia_arrow_preview_set_arrow:
 * @arrow: Preview widget to change.
 * @atype: New arrow type to use.
 * @left: If %TRUE, the preview should point to the left.
 *
 * Set the values shown by an arrow preview widget.
 *
 * Since: dawn-of-time
 */
void
dia_arrow_preview_set_arrow (DiaArrowPreview *arrow,
                             ArrowType        atype,
                             gboolean         left)
{
  if (arrow->atype != atype || arrow->left != left) {
    arrow->atype = atype;
    arrow->left = left;
    if (gtk_widget_is_drawable (GTK_WIDGET (arrow))) {
      gtk_widget_queue_draw (GTK_WIDGET (arrow));
    }
  }
}


ArrowType
dia_arrow_preview_get_arrow (DiaArrowPreview *self)
{
  g_return_val_if_fail (self, ARROW_NONE);

  return self->atype;
}
