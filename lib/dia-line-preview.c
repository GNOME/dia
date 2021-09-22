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

#include "dia-line-preview.h"

struct _DiaLinePreview {
  GtkMisc misc;
  LineStyle lstyle;
};

G_DEFINE_TYPE (DiaLinePreview, dia_line_preview, GTK_TYPE_MISC)


static int
dia_line_preview_expose (GtkWidget *widget, GdkEventExpose *event)
{
  DiaLinePreview *line = DIA_LINE_PREVIEW (widget);
  GtkMisc *misc = GTK_MISC(widget);
  int width, height;
  int x, y;
  GdkWindow *win;
  double dash_list[6];
  GtkStyle *style;
  GdkColor fg;
  cairo_t *ctx;

  if (gtk_widget_is_drawable (widget)) {
    GtkAllocation alloc;
    int xpad, ypad;

    gtk_widget_get_allocation (widget, &alloc);
    gtk_misc_get_padding (misc, &xpad, &ypad);

    width = alloc.width - xpad * 2;
    height = alloc.height - ypad * 2;
    x = (alloc.x + xpad);
    y = (alloc.y + ypad);

    win = gtk_widget_get_window (widget);
    style = gtk_widget_get_style (widget);
    fg = style->text[gtk_widget_get_state(widget)];

    ctx = gdk_cairo_create (win);
    gdk_cairo_set_source_color (ctx, &fg);
    cairo_set_line_width (ctx, 2);
    cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
    cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);

    switch (line->lstyle) {
      case LINESTYLE_DEFAULT:
      case LINESTYLE_SOLID:
        cairo_set_dash (ctx, dash_list, 0, 0);
        break;
      case LINESTYLE_DASHED:
        dash_list[0] = 10;
        dash_list[1] = 10;
        cairo_set_dash (ctx, dash_list, 2, 0);
        break;
      case LINESTYLE_DASH_DOT:
        dash_list[0] = 10;
        dash_list[1] = 4;
        dash_list[2] = 2;
        dash_list[3] = 4;
        cairo_set_dash (ctx, dash_list, 4, 0);
        break;
      case LINESTYLE_DASH_DOT_DOT:
        dash_list[0] = 10;
        dash_list[1] = 2;
        dash_list[2] = 2;
        dash_list[3] = 2;
        dash_list[4] = 2;
        dash_list[5] = 2;
        cairo_set_dash (ctx, dash_list, 6, 0);
        break;
      case LINESTYLE_DOTTED:
        dash_list[0] = 2;
        dash_list[1] = 2;
        cairo_set_dash (ctx, dash_list, 2, 0);
        break;
      default:
        g_return_val_if_reached (FALSE);
    }
    cairo_move_to (ctx, x, y + height / 2);
    cairo_line_to (ctx, x + width, y + height / 2);
    cairo_stroke (ctx);
  }
  return TRUE;
}


static void
dia_line_preview_class_init (DiaLinePreviewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->expose_event = dia_line_preview_expose;
}


static void
dia_line_preview_init (DiaLinePreview *line)
{
  int xpad, ypad;

  gtk_widget_set_has_window (GTK_WIDGET (line), FALSE);

  gtk_misc_get_padding (GTK_MISC (line), &xpad, &ypad);

  gtk_widget_set_size_request (GTK_WIDGET (line),
                               30 + xpad * 2,
                               15 + ypad * 2);

  line->lstyle = LINESTYLE_SOLID;
}


GtkWidget *
dia_line_preview_new (LineStyle lstyle)
{
  DiaLinePreview *line = g_object_new (DIA_TYPE_LINE_PREVIEW, NULL);

  line->lstyle = lstyle;

  return GTK_WIDGET (line);
}


void
dia_line_preview_set_style (DiaLinePreview *line, LineStyle lstyle)
{
  if (line->lstyle == lstyle) {
    return;
  }

  line->lstyle = lstyle;

  if (gtk_widget_is_drawable (GTK_WIDGET (line))) {
    gtk_widget_queue_draw (GTK_WIDGET (line));
  }
}

