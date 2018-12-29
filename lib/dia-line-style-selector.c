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

#include <gtk/gtk.h>

#include "intl.h"
#include "widgets.h"
#include "diaoptionmenu.h"
#include "dia-line-style-selector.h"
#include "dia-line-style-selector-popover.h"

/************* DiaLinePreview: ***************/

G_DEFINE_TYPE (DiaLinePreview, dia_line_preview, GTK_TYPE_WIDGET)

static gint
dia_line_preview_draw (GtkWidget *widget, cairo_t *ctx)
{
  DiaLinePreview *line = DIA_LINE_PREVIEW (widget);
  GtkAllocation alloc;
  gint width, height;
  double dash[6];
  int length;
  GdkRGBA fg;
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget), &fg);
  gdk_cairo_set_source_rgba (ctx, &fg);

  gtk_widget_get_allocation (widget, &alloc);

  width = alloc.width;
  height = alloc.height;
  length = line->length * 20;

  cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);

  /* Adapted from DiaCairoRenderer, TODO: Avoid duplication */
  switch (line->lstyle) {
    case LINESTYLE_DEFAULT:
    case LINESTYLE_SOLID:
      cairo_set_dash (ctx, NULL, 0, 0);
      break;
    case LINESTYLE_DASHED:
      dash[0] = length;
      dash[1] = length;
      cairo_set_dash (ctx, dash, 2, 0);
      break;
    case LINESTYLE_DASH_DOT:
      dash[0] = length;
      dash[1] = length * 0.45;
      dash[2] = length * 0.1;
      dash[3] = length * 0.45;
      cairo_set_dash (ctx, dash, 4, 0);
      break;
    case LINESTYLE_DASH_DOT_DOT:
      dash[0] = length;
      dash[1] = length * (0.8/3);
      dash[2] = length * 0.1;
      dash[3] = length * (0.8/3);
      dash[4] = length * 0.1;
      dash[5] = length * (0.8/3);
      cairo_set_dash (ctx, dash, 6, 0);
      break;
    case LINESTYLE_DOTTED:
      dash[0] = length * 0.1;
      dash[1] = length * 0.1;
      cairo_set_dash (ctx, dash, 2, 0);
      break;
    default:
      g_warning("DiaLinePreview : Unsupported line style specified!");
  }

  cairo_move_to (ctx, 0, height / 2);
  cairo_line_to (ctx, width, height / 2);
  cairo_stroke (ctx);

  return FALSE;
}

static void
dia_line_preview_class_init (DiaLinePreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->draw = dia_line_preview_draw;
}

static void
dia_line_preview_init (DiaLinePreview *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 24);

  self->lstyle = LINESTYLE_SOLID;
  self->length = 1; /* For clarity we default quite big */
}

GtkWidget *
dia_line_preview_new (LineStyle lstyle)
{
  DiaLinePreview *line = g_object_new (DIA_TYPE_LINE_PREVIEW, NULL);

  line->lstyle = lstyle;

  return GTK_WIDGET (line);
}

void
dia_line_preview_set_line_style (DiaLinePreview *self,
                                 LineStyle       lstyle,
                                 gdouble         length)
{
  if (self->lstyle != lstyle || self->length != length) {
    self->lstyle = lstyle;
    self->length = length;
    gtk_widget_queue_draw (GTK_WIDGET (self));
  }
}

LineStyle
dia_line_preview_get_line_style (DiaLinePreview *self)
{
  return self->lstyle;
}

/************* DiaLineStyleSelector: ***************/

typedef struct _DiaLineStyleSelectorPrivate DiaLineStyleSelectorPrivate;

struct _DiaLineStyleSelectorPrivate {
  GtkWidget *popover;
  GtkWidget *preview;
};

G_DEFINE_TYPE_WITH_CODE (DiaLineStyleSelector, dia_line_style_selector, GTK_TYPE_MENU_BUTTON,
                         G_ADD_PRIVATE (DiaLineStyleSelector))

enum {
    DLS_VALUE_CHANGED,
    DLS_LAST_SIGNAL
};

static guint dls_signals[DLS_LAST_SIGNAL] = { 0 };

static void
dia_line_style_selector_class_init (DiaLineStyleSelectorClass *class)
{
  dls_signals[DLS_VALUE_CHANGED]
      = g_signal_new("value-changed",
		     G_TYPE_FROM_CLASS(class),
		     G_SIGNAL_RUN_FIRST,
		     0, NULL, NULL,
		     g_cclosure_marshal_VOID__VOID,
		     G_TYPE_NONE, 0);
}

static void
value_changed (GObject *src, DiaLineStyleSelector *self)
{
  DiaLineStyleSelectorPrivate *priv = dia_line_style_selector_get_instance_private (self);
  gdouble length;
  LineStyle style = dia_line_style_selector_popover_get_line_style (DIA_LINE_STYLE_SELECTOR_POPOVER (priv->popover),
                                                                    &length);
  dia_line_preview_set_line_style (DIA_LINE_PREVIEW (priv->preview), style, length);

  g_signal_emit (G_OBJECT (self),
                 dls_signals[DLS_VALUE_CHANGED], 0);
}

static void
dia_line_style_selector_init (DiaLineStyleSelector *self)
{
  GtkWidget *box;
  DiaLineStyleSelectorPrivate *priv = dia_line_style_selector_get_instance_private (self);

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 16);
  gtk_widget_show (box);
  gtk_container_add (GTK_CONTAINER (self), box);

  priv->preview = dia_line_preview_new (DEFAULT_LINESTYLE);
  gtk_widget_show (priv->preview);
  gtk_box_pack_start (GTK_BOX (box), priv->preview, TRUE, TRUE, 0);
 
  priv->popover = dia_line_style_selector_popover_new ();
  dia_line_style_selector_popover_set_line_style (DIA_LINE_STYLE_SELECTOR_POPOVER (priv->popover),
                                                  DEFAULT_LINESTYLE);
  g_signal_connect (G_OBJECT (priv->popover), "value-changed",
                    G_CALLBACK (value_changed), self);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self), priv->popover);
}

GtkWidget *
dia_line_style_selector_new ()
{
  return g_object_new (DIA_TYPE_LINE_STYLE_SELECTOR, NULL);
}

void 
dia_line_style_selector_get_line_style (DiaLineStyleSelector *self,
                                        LineStyle            *ls,
                                        gdouble              *dl)
{
  DiaLineStyleSelectorPrivate *priv = dia_line_style_selector_get_instance_private (self);

  *ls = dia_line_style_selector_popover_get_line_style (DIA_LINE_STYLE_SELECTOR_POPOVER (priv->popover),
                                                        (gdouble *) dl);
}

void
dia_line_style_selector_set_line_style (DiaLineStyleSelector *self,
                                        LineStyle             linestyle,
                                        gdouble               dashlength)
{
  DiaLineStyleSelectorPrivate *priv = dia_line_style_selector_get_instance_private (self);

  dia_line_style_selector_popover_set_line_style (DIA_LINE_STYLE_SELECTOR_POPOVER (priv->popover),
                                                  linestyle);
  dia_line_style_selector_popover_set_length (DIA_LINE_STYLE_SELECTOR_POPOVER (priv->popover),
                                              dashlength);
}
