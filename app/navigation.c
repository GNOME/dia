/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * $Id$
 *
 * navigation.c : a navigation popup window to browse large diagrams.
 * Copyright (C) 2003 Luc Pionchon
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
 */

#include <gtk/gtk.h>

#include <xpm-pixbuf.h>

#include "diagram.h"
#include "display.h"
#include "renderer/diacairo.h"
#include "navigation.h"

#define THUMBNAIL_MAX_SIZE 150 /*(pixels) this may be a preference*/
#define DIAGRAM_OFFSET 1 /*(diagram's unit) so we can see the little green boxes :)*/
#define FRAME_THICKNESS 2 /*(pixels)*/
#define STD_CURSOR_MIN 16 /*(pixels)*/

#define DIA_TYPE_NAVIGATION_WINDOW dia_navigation_window_get_type ()
G_DECLARE_FINAL_TYPE (DiaNavigationWindow, dia_navigation_window, DIA, NAVIGATION_WINDOW, GtkWindow)


struct _DiaNavigationWindow {
  GtkWindow parent;

  /*popup size (drawing_area)*/
  int width;
  int height;
  int max_size;

  gboolean is_first_expose;

  /*miniframe*/
  int frame_w;
  int frame_h;
  GdkCursor * cursor;

  /*factors to translate thumbnail coordinates to adjustment values*/
  double hadj_coef;
  double vadj_coef;

  /*diagram thumbnail's buffer*/
  cairo_surface_t *surface;

  /*display to navigate*/
  DDisplay * ddisp;
};

G_DEFINE_TYPE (DiaNavigationWindow, dia_navigation_window, GTK_TYPE_WINDOW)

enum {
  PROP_0,
  PROP_DISPLAY,
  LAST_PROP
};
static GParamSpec *pspecs[LAST_PROP] = { NULL, };

static void
dia_navigation_window_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (object);

  switch (property_id) {
    case PROP_DISPLAY:
      self->ddisp = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_navigation_window_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (object);

  switch (property_id) {
    case PROP_DISPLAY:
      g_value_set_pointer (value, self->ddisp);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

/* resets adjustement to diagram size */
static void
reset_sc_adj (GtkAdjustment *adj, double lower, double upper, double page)
{
  gtk_adjustment_set_page_size (adj, page);

  gtk_adjustment_set_lower (adj, lower);
  gtk_adjustment_set_upper (adj, upper);

  if (gtk_adjustment_get_value (adj) < lower) {
    gtk_adjustment_set_value (adj, lower);
  }

  if (gtk_adjustment_get_value (adj) > (upper - page)) {
    gtk_adjustment_set_value (adj, upper - page);
  }

  gtk_adjustment_changed (adj);
}


static void
dia_navigation_window_constructed (GObject *object)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (object);
  DiagramData * data;

  DiaRectangle rect;/*diagram's extents*/
  double zoom;/*zoom factor for thumbnail rendering*/

  DiaCairoRenderer *renderer;

  G_OBJECT_CLASS (dia_navigation_window_parent_class)->constructed (object);

  /*--Retrieve the diagram's data*/
  data = self->ddisp->diagram->data;

  /*--Calculate sizes*/
  {
    int canvas_width, canvas_height;/*pixels*/
    int diagram_width, diagram_height;/*pixels*/
    GtkAdjustment * adj;
    GtkAllocation alloc;

    self->max_size = THUMBNAIL_MAX_SIZE;

    /*size: Diagram <--> thumbnail*/
    rect.top    = data->extents.top    - DIAGRAM_OFFSET;
    rect.left   = data->extents.left   - DIAGRAM_OFFSET;
    rect.bottom = data->extents.bottom + DIAGRAM_OFFSET + 1;
    rect.right  = data->extents.right  + DIAGRAM_OFFSET + 1;

    zoom = self->max_size / MAX( (rect.right - rect.left) , (rect.bottom - rect.top) );

    self->width  = MIN (self->max_size, (rect.right  - rect.left) * zoom);
    self->height = MIN (self->max_size, (rect.bottom - rect.top)  * zoom);

    /*size: display canvas <--> frame cursor*/
    diagram_width  = (int) ddisplay_transform_length (self->ddisp, (rect.right - rect.left));
    diagram_height = (int) ddisplay_transform_length (self->ddisp, (rect.bottom - rect.top));

    if (diagram_width * diagram_height == 0)
      return; /* don't crash with no size, i.e. empty diagram */

    gtk_widget_get_allocation (self->ddisp->canvas, &alloc);

    canvas_width   = alloc.width;
    canvas_height  = alloc.height;

    self->frame_w = self->width  * canvas_width  / diagram_width;
    self->frame_h = self->height * canvas_height / diagram_height;

    /*reset adjustments to diagram size,*/
    /*(dia allows to grow the canvas bigger than the diagram's actual size)    */
    /*and store the ratio thumbnail/adjustment(speedup on motion)*/
    adj = self->ddisp->hsbdata;
    reset_sc_adj (adj, rect.left, rect.right, canvas_width / self->ddisp->zoom_factor);
    self->hadj_coef = (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) - gtk_adjustment_get_lower (adj)) /
                        (self->width - self->frame_w);

    adj = self->ddisp->vsbdata;
    reset_sc_adj (adj, rect.top, rect.bottom, canvas_height / self->ddisp->zoom_factor);
    self->vadj_coef = (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) - gtk_adjustment_get_lower (adj)) /
                        (self->height - self->frame_h);
  }

  gtk_widget_set_size_request (GTK_WIDGET (self), self->width, self->height);

  /* surface to draw the thumbnail on */
  self->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                             self->width,
                                             self->height);

  renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);
  renderer->scale = zoom;
  renderer->surface = cairo_surface_reference (self->surface);

  /*render the data*/
  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  self->is_first_expose = TRUE;/*set to request to draw the miniframe*/
}


static void
dia_navigation_window_dispose (GObject *object)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (object);

  g_clear_object (&self->cursor);
  g_clear_pointer (&self->surface, cairo_surface_destroy);

  G_OBJECT_CLASS (dia_navigation_window_parent_class)->dispose (object);
}


static gboolean
dia_navigation_window_draw (GtkWidget *widget, cairo_t *ctx)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (widget);
  GtkAdjustment * adj;
  int x, y;

  cairo_set_line_width (ctx, FRAME_THICKNESS);
  cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);

  /*refresh the part outdated by the event*/
  cairo_set_source_surface (ctx, self->surface, 0, 0);
  cairo_paint (ctx);

  adj = self->ddisp->hsbdata;
  x = (gtk_adjustment_get_value (adj) - gtk_adjustment_get_lower (adj)) /
          (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) *
              (self->width) +1;

  adj = self->ddisp->vsbdata;
  y = (gtk_adjustment_get_value (adj) - gtk_adjustment_get_lower (adj)) /
          (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) *
              (self->height) +1;

  /*draw directly on the window, do not buffer the miniframe*/
  cairo_set_source_rgb (ctx, 0, 0, 0);
  cairo_rectangle (ctx, x, y, self->frame_w, self->frame_h);
  cairo_stroke (ctx);

  self->is_first_expose = FALSE;

  return FALSE;
}


static gboolean
dia_navigation_window_motion_notify_event (GtkWidget      *widget,
                                           GdkEventMotion *event)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (widget);
  GtkAdjustment * adj;
  gboolean value_changed;

  int w = self->frame_w;
  int h = self->frame_h;

  int x, y;/*top left of the miniframe*/

  /* Don't try to move if there's no room for it.*/
  if (w >= self->width-1 && h >= self->height-1) return FALSE;

  x = CLAMP (event->x - w/2 , 0, self->width  - w);
  y = CLAMP (event->y - h/2 , 0, self->height - h);

  adj = self->ddisp->hsbdata;
  value_changed = FALSE;
  if (w/2 <= event->x && event->x <= (self->width - w/2)){
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj) + x * self->hadj_coef);
    value_changed = TRUE;
  }
  else if (x == 0 && gtk_adjustment_get_value (adj) != gtk_adjustment_get_lower (adj)){/*you've been too fast! :)*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
    value_changed = TRUE;
  }
  else if (x == (self->width - w) && gtk_adjustment_get_value (adj) != (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj))){/*idem*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
    value_changed = TRUE;
  }
  if (value_changed) gtk_adjustment_value_changed(adj);

  adj = self->ddisp->vsbdata;
  value_changed = FALSE;
  if (h/2 <= event->y && event->y <= (self->height - h/2)){
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj) + y * self->vadj_coef);
    value_changed = TRUE;
  }
  else if (y == 0 && gtk_adjustment_get_value (adj) != gtk_adjustment_get_lower (adj)){/*you've been too fast! :)*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
    value_changed = TRUE;
  }
  else if (y == (self->height - h) && gtk_adjustment_get_value (adj) != (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj))){/*idem*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
    value_changed = TRUE;
  }
  if (value_changed) {
    gtk_adjustment_value_changed (adj);
  }

  /* Trigger redraw */
  gtk_widget_queue_draw (widget);

  return FALSE;
}


static gboolean
dia_navigation_window_button_release_event (GtkWidget      *widget,
                                            GdkEventButton *event)
{
  DiaNavigationWindow *self = DIA_NAVIGATION_WINDOW (widget);

  g_object_ref (self);

  gtk_widget_destroy (widget);

  /* returns the focus on the canvas */
  gtk_widget_grab_focus (self->ddisp->canvas);

  g_object_unref (self);

  return FALSE;
}


static void
dia_navigation_window_class_init (DiaNavigationWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_navigation_window_set_property;
  object_class->get_property = dia_navigation_window_get_property;
  object_class->constructed = dia_navigation_window_constructed;
  object_class->dispose = dia_navigation_window_dispose;

  widget_class->draw = dia_navigation_window_draw;
  widget_class->motion_notify_event = dia_navigation_window_motion_notify_event;
  widget_class->button_release_event = dia_navigation_window_button_release_event;

  pspecs[PROP_DISPLAY] =
    g_param_spec_pointer ("display",
                          "Display",
                          "The DDisplay",
                          G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void
dia_navigation_window_init (DiaNavigationWindow *self)
{
  gtk_widget_add_events (GTK_WIDGET (self),
                         GDK_EXPOSURE_MASK | GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK);
}


static void
dia_navigation_window_popup (DiaNavigationWindow *self)
{
  gtk_widget_show (GTK_WIDGET (self));

  /*cursor*/
  if (MIN (self->frame_h, self->frame_w) > STD_CURSOR_MIN) {
    self->cursor = gdk_cursor_new (GDK_FLEUR);
  } else { /*the miniframe is very small, so we use a minimalist cursor*/
    self->cursor = gdk_cursor_new_from_name (gtk_widget_get_display (GTK_WIDGET (self)),
                                             "none");
  }
}


static const char *nav_xpm[] = {
  "10 10 2 1",
  "  c None",
  ". c #000000",
  "    ..    ",
  "   ....   ",
  "    ..    ",
  " .  ..  . ",
  "..........",
  "..........",
  " .  ..  . ",
  "    ..    ",
  "   ....   ",
  "    ..    "
};


static DiaNavigationWindow *current_popup;

static void
on_button_navigation_popup_pressed (GtkButton * button, gpointer _ddisp)
{
  /*--GUI*/
  /*popup window, and cute frame*/
  current_popup = g_object_new (DIA_TYPE_NAVIGATION_WINDOW,
                               "type", GTK_WINDOW_POPUP,
                               "window-position", GTK_WIN_POS_MOUSE,
                               "transient-for", gtk_widget_get_toplevel (GTK_WIDGET (button)),
                               "attached-to", button,
                               "display", _ddisp,
                               NULL);
  g_object_add_weak_pointer (G_OBJECT (current_popup), (gpointer *) &current_popup);

  dia_navigation_window_popup (current_popup);

  /*grab the pointer*/
  {
    GdkWindow *window = gtk_widget_get_window (GTK_WIDGET (button));
    GdkDisplay *display = window ? gdk_window_get_display (window) : gdk_display_get_default ();
    GdkSeat *seat = gdk_display_get_default_seat (display);
    GdkDevice *device = gdk_seat_get_pointer (seat);
    gdk_device_grab (device,
                     gtk_widget_get_window (GTK_WIDGET (button)),
                     GDK_OWNERSHIP_APPLICATION,
                     TRUE,
                     GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
                     current_popup->cursor,
                     GDK_CURRENT_TIME);
  }
}


static void
on_button_navigation_popup_released (GtkButton * button, gpointer z)
{
  /* don't popdown before having drawn once */
  if (current_popup && !current_popup->is_first_expose) {
    /* needed for gtk+-2.6.x, but work for 2.6 too. */
    gtk_widget_destroy (GTK_WIDGET (current_popup));
  }
}


/**
 * navigation_popup_new:
 * @ddisp: the #DDisplay to navigate through.
 *
 * A button which triggers a popup navigation window.
 *
 * The popup window is created when the button is "pressed",
 * and destroyed when the button is "released". In the meantime, the
 * popup window grabs the pointer/focus. Moving the mouse adjust the
 * scrollbars of the given #DDisplay accordingly.
 *
 * Returns: a new #GtkButton.
 *
 * Since: dawn-of-time
 */
GtkWidget *
navigation_popup_new (DDisplay *ddisp)
{
  GtkWidget *button;
  GtkWidget *image;
  GdkPixbuf *pixbuf;

  button = gtk_button_new ();
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  g_signal_connect (G_OBJECT (button),
                    "pressed", G_CALLBACK (on_button_navigation_popup_pressed),
                    ddisp);
  /*if you are fast, the button catches it before the drawing_area:*/
  g_signal_connect (G_OBJECT (button),
                    "released", G_CALLBACK (on_button_navigation_popup_released),
                    NULL);

  pixbuf = xpm_pixbuf_load (nav_xpm);

  image = gtk_image_new_from_pixbuf (pixbuf);

  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  g_clear_object (&pixbuf);

  return button;
}
