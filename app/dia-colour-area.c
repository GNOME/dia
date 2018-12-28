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

#include <stdio.h>

#include "intl.h"

#include "dia-colour-area.h"
#include "attributes.h"
#include "persistence.h"

#define FORE_AREA 0
#define BACK_AREA 1
#define SWAP_AREA 2
#define DEF_AREA  3

#define FOREGROUND 0
#define BACKGROUND 1

G_DEFINE_TYPE (DiaColourArea, dia_colour_area, GTK_TYPE_EVENT_BOX)

/*  Local functions  */
static int
dia_colour_area_target (DiaColourArea *self,
                        int            x,
                        int            y)
{
  gint rect_w, rect_h;
  gint width, height;

  width = gtk_widget_get_allocated_width (GTK_WIDGET (self));
  height = gtk_widget_get_allocated_height (GTK_WIDGET (self));

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  /*  foreground active  */
  if (x > 0 && x < rect_w &&
      y > 0 && y < rect_h)
    return FORE_AREA;
  else if (x > (width - rect_w) && x < width &&
           y > (height - rect_h) && y < height)
    return BACK_AREA;
  else if (x > 0 && x < (width - rect_w) &&
           y > rect_h && y < height)
    return DEF_AREA;
  else if (x > rect_w && x < width &&
           y > 0 && y < (height - rect_h))
    return SWAP_AREA;
  else
    return -1;
}

static gboolean
dia_colour_area_draw (GtkWidget *self, cairo_t *ctx)
{
  GdkRGBA fg, bg;
  gint rect_w, rect_h;
  gint width, height;
  gint img_width, img_height;
  DiaColourArea *priv = DIA_COLOUR_AREA (self);

  width = gtk_widget_get_allocated_width (self);
  height = gtk_widget_get_allocated_height (self);

  fg = attributes_get_foreground();
  bg = attributes_get_background();

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  gdk_cairo_set_source_rgba (ctx, &bg);

  cairo_rectangle (ctx,
                   (width - rect_w), (height - rect_h), rect_w, rect_h);
  cairo_fill (ctx);

  gdk_cairo_set_source_rgba (ctx, &fg);
  cairo_rectangle (ctx, 0, 0, rect_w, rect_h);
  cairo_fill (ctx);

  /*  draw the default colours pixmap  */
  img_width = gdk_pixbuf_get_width (priv->reset);
  img_height = gdk_pixbuf_get_height (priv->reset);
  gdk_cairo_set_source_pixbuf (ctx, priv->reset, 0, height - img_height);
  cairo_rectangle (ctx, 0, height - img_height, img_width, img_height);
  cairo_fill (ctx);

  /*  draw the swap pixmap  */
  img_width = gdk_pixbuf_get_width (priv->swap);
  img_height = gdk_pixbuf_get_height (priv->swap);
  gdk_cairo_set_source_pixbuf (ctx, priv->swap, width - img_width, 0);
  cairo_rectangle (ctx, width - img_width, 0, img_width, img_height);
  cairo_fill (ctx);

  return FALSE;
}

static void
dia_colour_area_response (GtkDialog     *chooser,
                          gint           response,
                          DiaColourArea *self)
{
  if (response == GTK_RESPONSE_OK) {
    GdkRGBA color;

    gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (chooser), &color);

    if (self->edit_color == FOREGROUND) {
      attributes_set_foreground (&color);
    } else {
      attributes_set_background (&color);
    }
  } else {
    attributes_set_foreground(&self->stored_foreground);
    attributes_set_background(&self->stored_background);
  }

  gtk_widget_hide (self->color_select);
  self->color_select_active = 0;

  /* Trigger redraw */
  gtk_widget_queue_draw (GTK_WIDGET (self));
}

static void
dia_colour_area_edit (DiaColourArea *self)
{
  GtkWidget *window;
  GdkRGBA color;

  if (!self->color_select_active) {
    self->stored_foreground = attributes_get_foreground();
    self->stored_background = attributes_get_background();
  }
  
  if (self->active_color == FOREGROUND) {
    color = attributes_get_foreground();
    self->edit_color = FOREGROUND;
  } else {
    color = attributes_get_background();
    self->edit_color = BACKGROUND;
  }

  if (self->color_select) {
    window = self->color_select;

    gtk_window_set_title (GTK_WINDOW (self->color_select),
                          self->edit_color == FOREGROUND ?
                            _("Select foreground color") : _("Select background color"));

    if (!self->color_select_active) {
      gtk_widget_show (self->color_select);
    }
  } else {
    window = self->color_select = 
      gtk_color_chooser_dialog_new (self->edit_color == FOREGROUND ?
                                      _("Select foreground color") : _("Select background color"), 
                                      GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self))));

    self->color_select_active = 1;
    gtk_color_chooser_set_use_alpha (GTK_COLOR_CHOOSER (window), TRUE);

    g_signal_connect (G_OBJECT (window), "response",
                      G_CALLBACK (dia_colour_area_response), self);
    
    /* Make sure window is shown before setting its colors: */
    gtk_widget_show (self->color_select);
  }

  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->color_select), &color);
}

static gint
dia_colour_area_event (GtkWidget *widget,
                       GdkEvent  *event)
{
  DiaColourArea *self = DIA_COLOUR_AREA (widget);
  GdkEventButton *bevent;
  int target;
  
  switch (event->type) {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1) {
        switch ((target = dia_colour_area_target (self, bevent->x, bevent->y))) {
          case FORE_AREA:
          case BACK_AREA:
            if (target == self->active_color) {
              dia_colour_area_edit (self);
            } else {
              self->active_color = target;
              /* Trigger redraw */
              gtk_widget_queue_draw (GTK_WIDGET (self));
            }
            break;
          case SWAP_AREA:
            attributes_swap_fgbg();
            /* Trigger redraw */
            gtk_widget_queue_draw (GTK_WIDGET (self));
            break;
          case DEF_AREA:
            attributes_default_fgbg();
            /* Trigger redraw */
            gtk_widget_queue_draw (GTK_WIDGET (self));
            break;
        }
      }
      break;
    
    default:
      break;
  }

  return FALSE;
}

#include "pixmaps/swap.xpm"
#include "pixmaps/default.xpm"

static void
dia_colour_area_class_init (DiaColourAreaClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->draw = dia_colour_area_draw;
  widget_class->event = dia_colour_area_event;

  attributes_set_foreground (persistence_register_color ("fg_color", &color_black));
  attributes_set_background (persistence_register_color ("bg_color", &color_white));
}

static void
dia_colour_area_init (DiaColourArea *self)
{
  self->reset = gdk_pixbuf_new_from_xpm_data (default_xpm);
  self->swap = gdk_pixbuf_new_from_xpm_data (swap_xpm);

  self->active_color = 0;
    
  self->color_select = NULL;
  self->color_select_active = 0;

  gtk_widget_set_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK);

  gtk_widget_set_tooltip_text (GTK_WIDGET (self), 
      _("Foreground & background colors for new objects.  "
        "The small black and white squares reset colors.  "
        "The small arrows swap colors.  Double-click to "
        "change colors."));
}

GtkWidget *
dia_colour_area_new (int width,
                     int height)
{
  GtkWidget *event_box;

  event_box = g_object_new (DIA_TYPE_COLOUR_AREA, NULL);
  gtk_widget_set_size_request (event_box, width, height);

  gtk_widget_show (event_box);

  return event_box;
}
