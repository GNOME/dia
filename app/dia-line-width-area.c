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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "dia-line-width-area.h"
#include "attributes.h"
#include "persistence.h"
#include "intl.h"

#if !defined(rint)
# include <math.h>
# define rint(x) floor ((x) + 0.5)
#endif

#define BASE_WIDTH 0.05
#define PIXELS_BETWEEN_LINES 6
#define NUMLINES 5

#define X_OFFSET(i) (PIXELS_BETWEEN_LINES*(i)+((i)-1)*(i)/2)

#define AREA_WIDTH X_OFFSET(NUMLINES+1)
#define AREA_HEIGHT 42

G_DEFINE_TYPE (DiaLineWidthArea, dia_line_width_area, GTK_TYPE_EVENT_BOX)

static int
linewidth_area_target (int x, int y)
{
  int i;
  int x_offs;
  for (i=1;i<=NUMLINES;i++) {
    x_offs = X_OFFSET(i);
    if ((x>=x_offs-PIXELS_BETWEEN_LINES/2) &&
        (x<x_offs+i+PIXELS_BETWEEN_LINES/2))
      return i;
  }
  return 0;
}

static int
linewidth_number_from_width (real width)
{
  if (fabs(width/BASE_WIDTH-rint(width/BASE_WIDTH)) > 0.0005 ||
      (width/BASE_WIDTH > NUMLINES)) {
    return 0;
  } else {
    return width/BASE_WIDTH+1.0005;
  }
}

static void
dia_line_width_area_dialog_respond (GtkWidget        *widget,
                                    gint              response_id,
                                    DiaLineWidthArea *self)
{
  if (response_id == GTK_RESPONSE_OK) {
    float newvalue = gtk_spin_button_get_value (GTK_SPIN_BUTTON (self->button));
    self->active = linewidth_number_from_width (newvalue);
    /* Trigger redraw */
    gtk_widget_queue_draw (self);
    attributes_set_default_linewidth (newvalue);
  }
  gtk_widget_hide(self->dialog);
}

static void
dia_line_width_area_dialog_ok (GtkWidget *widget, DiaLineWidthArea *self)
{
  gtk_dialog_response (GTK_DIALOG (self->dialog), GTK_RESPONSE_OK);
}

/* Crashes with gtk_widget_destroyed, so use this instead */
static void
dialog_destroyed(GtkWidget *widget, gpointer data)
{
  GtkWidget **wid = (GtkWidget**)data;
  if (wid) *wid = NULL;
}

static void
dia_line_width_area_create_dialog (DiaLineWidthArea *self,
                                   GtkWindow        *toplevel)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkAdjustment *adj;

  self->dialog = gtk_dialog_new_with_buttons (_("Line width"), toplevel, 0,
                                              _("Cancel"), GTK_RESPONSE_CANCEL,
                                              _("Okay"), GTK_RESPONSE_OK, 
                                              NULL);
  
  gtk_dialog_set_default_response (GTK_DIALOG(self->dialog), GTK_RESPONSE_OK);
  gtk_window_set_role (GTK_WINDOW (self->dialog), "linewidth_window");
  gtk_window_set_resizable (GTK_WINDOW (self->dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (self->dialog), 2);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
  label = gtk_label_new(_("Line width:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);
  adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.05, 0.0);
  self->button = gtk_spin_button_new(adj, attributes_get_default_linewidth(), 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(self->button), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(self->button), TRUE);
  gtk_box_pack_start(GTK_BOX (hbox), self->button, TRUE, TRUE, 0);
  gtk_widget_show (self->button);
  gtk_widget_show(hbox);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG (self->dialog))), hbox, TRUE, TRUE, 0);
  
  gtk_widget_show (self->button);

  g_signal_connect(G_OBJECT (self->dialog), "response",
                   G_CALLBACK (dia_line_width_area_dialog_respond), self);
  g_signal_connect_after (G_OBJECT (self->button), "activate",
                   G_CALLBACK (dia_line_width_area_dialog_ok), self);

  g_signal_connect (G_OBJECT (self->dialog), "delete_event",
		    G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect (G_OBJECT (self->dialog), "destroy",
		    G_CALLBACK(dialog_destroyed), &self->dialog);

  persistence_register_window (GTK_WINDOW (self->dialog));
}

static gboolean
dia_line_width_area_draw (GtkWidget *self, cairo_t *ctx)
{
  GdkRGBA fg;
  int width, height;
  int i;
  int x_offs;
  GtkStyle *style;
  double dashes[] = { 3 };
  DiaLineWidthArea *priv = DIA_LINE_WIDTH_AREA (self);

  cairo_set_line_width (ctx, 1);
  cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);
  cairo_set_dash (ctx, dashes, 1, 0);

  width = gtk_widget_get_allocated_width (self);
  height = gtk_widget_get_allocated_height (self);

  style = gtk_widget_get_style (self);

  gtk_style_context_get_color (gtk_widget_get_style_context (self),
                               gtk_widget_get_state_flags (self),
                               &fg);

  gdk_cairo_set_source_rgba (ctx, &fg);
  
  for (i = 0; i <= NUMLINES; i++) {
    x_offs = X_OFFSET(i);

    cairo_rectangle (ctx, x_offs, 2, i, height - 4);
    cairo_fill (ctx);
  }
  
  if (priv->active != 0) {
    cairo_rectangle (ctx, X_OFFSET(priv->active) - 2, 0,
                          priv->active + 4, height - 1);
    cairo_stroke (ctx);
  }

  return FALSE;
}

static gint
dia_line_width_area_event (GtkWidget *self,
                           GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventConfigure *cevent;
  int target;
  DiaLineWidthArea *priv = DIA_LINE_WIDTH_AREA (self);
  
  switch (event->type)
    {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      if (bevent->button == 1) {
        target = linewidth_area_target (bevent->x, bevent->y);
        if (target != 0) {
          priv->active = target;
          /* Trigger redraw */
          gtk_widget_queue_draw (self);
          attributes_set_default_linewidth(BASE_WIDTH*(target-1));
        }
      }
      break;

    case GDK_2BUTTON_PRESS:
      if (priv->dialog == NULL)
        dia_line_width_area_create_dialog (priv, GTK_WINDOW (gtk_widget_get_toplevel (self)));
      else
        gtk_widget_grab_focus (priv->button);
      gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->button), attributes_get_default_linewidth ());

      gtk_widget_show (priv->dialog);
      break;

    default:
      break;
    }

  return FALSE;
}

static void
dia_line_width_area_class_init (DiaLineWidthAreaClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->draw = dia_line_width_area_draw;
  widget_class->event = dia_line_width_area_event;

  attributes_set_default_linewidth (persistence_register_real ("linewidth", 0.1));
}

static void
dia_line_width_area_init (DiaLineWidthArea *self)
{
  self->active = linewidth_number_from_width (attributes_get_default_linewidth ());

  gtk_widget_set_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK);
  gtk_widget_set_tooltip_text (GTK_WIDGET (self), _("Line widths.  Click on a line to set the default line width for new objects.  Double-click to set the line width more precisely."));
}


GtkWidget *
dia_line_width_area_new ()
{
  GtkWidget *event_box;

  event_box = g_object_new (DIA_TYPE_LINE_WIDTH_AREA, NULL);
  gtk_widget_set_size_request (event_box, AREA_WIDTH, AREA_HEIGHT);

  gtk_widget_show (event_box);

  return event_box;
}