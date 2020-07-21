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

#include "diagram.h"
#include "display.h"
#include "renderer/diacairo.h"

#include "navigation.h"


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


#define THUMBNAIL_MAX_SIZE 150 /*(pixels) this may be a preference*/


struct _NavigationWindow
{
  GtkWidget * popup_window;

  /*popup size (drawing_area)*/
  int width;
  int height;
  int max_size;

  gboolean is_first_expose;

  /*miniframe*/
  int frame_w;
  int frame_h;
  GdkCursor * cursor;

  /*factors to translate thumbnail coordinates to adjustement values*/
  gdouble hadj_coef;
  gdouble vadj_coef;

  /*diagram thumbnail's buffer*/
  cairo_surface_t *surface;

  /*display to navigate*/
  DDisplay * ddisp;
};

typedef struct _NavigationWindow NavigationWindow;

static NavigationWindow _nav;
static NavigationWindow * nav = &_nav;


#define DIAGRAM_OFFSET 1 /*(diagram's unit) so we can see the little green boxes :)*/
#define FRAME_THICKNESS 2 /*(pixels)*/
#define STD_CURSOR_MIN 16 /*(pixels)*/


static void on_button_navigation_popup_pressed  (GtkButton * button, gpointer _ddisp);
static void on_button_navigation_popup_released (GtkButton * button, gpointer unused);

static void reset_sc_adj (GtkAdjustment * adj, gdouble lower, gdouble upper, gdouble page);

static gboolean on_da_expose_event         (GtkWidget * widget, GdkEventExpose * event, gpointer unused);
static gboolean on_da_motion_notify_event  (GtkWidget * widget, GdkEventMotion * event, gpointer unused);
static gboolean on_da_button_release_event (GtkWidget * widget, GdkEventButton * event, gpointer popup_window);


static char * nav_xpm[] = {
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

GtkWidget *
navigation_popup_new (DDisplay *ddisp)
{
  GtkWidget * button;

  GtkWidget * image;
  GdkPixmap * pixmap;
  GdkBitmap * mask = NULL;
  GtkStyle  * style;

  button = gtk_button_new ();
  gtk_container_set_border_width (GTK_CONTAINER (button), 0);
  gtk_button_set_relief (GTK_BUTTON(button), GTK_RELIEF_NONE);
  g_signal_connect (G_OBJECT (button), "pressed",
                    G_CALLBACK (on_button_navigation_popup_pressed), ddisp);
  /*if you are fast, the button catches it before the drawing_area:*/
  g_signal_connect (G_OBJECT (button), "released",
                    G_CALLBACK (on_button_navigation_popup_released), NULL);

  style = gtk_widget_get_style (button);
  pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
                                                 gtk_widget_get_colormap(button),
                                                 &mask,
                                                 &(style->bg[GTK_STATE_NORMAL]),
                                                 nav_xpm);

  image = gtk_image_new_from_pixmap (pixmap, mask);
  g_clear_object (&pixmap);
  g_clear_object (&mask);

  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);

  return button;
}


static void
on_button_navigation_popup_pressed (GtkButton * button, gpointer _ddisp)
{
  GtkWidget * popup_window;
  GtkWidget * frame;

  GtkWidget * drawing_area;

  DiagramData * data;

  DiaRectangle rect;/*diagram's extents*/
  real zoom;/*zoom factor for thumbnail rendering*/

  DiaCairoRenderer *renderer;

  memset (nav, 0, sizeof(NavigationWindow));
  /*--Retrieve the diagram's data*/
  nav->ddisp  = (DDisplay *) _ddisp;
  data = nav->ddisp->diagram->data;

  /*--Calculate sizes*/
  {
    int canvas_width, canvas_height;/*pixels*/
    int diagram_width, diagram_height;/*pixels*/
    GtkAdjustment * adj;
    GtkAllocation alloc;

    nav->max_size = THUMBNAIL_MAX_SIZE;

    /*size: Diagram <--> thumbnail*/
    rect.top    = data->extents.top    - DIAGRAM_OFFSET;
    rect.left   = data->extents.left   - DIAGRAM_OFFSET;
    rect.bottom = data->extents.bottom + DIAGRAM_OFFSET + 1;
    rect.right  = data->extents.right  + DIAGRAM_OFFSET + 1;

    zoom = nav->max_size / MAX( (rect.right - rect.left) , (rect.bottom - rect.top) );

    nav->width  = MIN( nav->max_size, (rect.right  - rect.left) * zoom);
    nav->height = MIN( nav->max_size, (rect.bottom - rect.top)  * zoom);

    /*size: display canvas <--> frame cursor*/
    diagram_width  = (int) ddisplay_transform_length (nav->ddisp, (rect.right - rect.left));
    diagram_height = (int) ddisplay_transform_length (nav->ddisp, (rect.bottom - rect.top));

    if (diagram_width * diagram_height == 0)
      return; /* don't crash with no size, i.e. empty diagram */

    gtk_widget_get_allocation (nav->ddisp->canvas, &alloc);

    canvas_width   = alloc.width;
    canvas_height  = alloc.height;

    nav->frame_w = nav->width  * canvas_width  / diagram_width;
    nav->frame_h = nav->height * canvas_height / diagram_height;

    /*reset adjustements to diagram size,*/
    /*(dia allows to grow the canvas bigger than the diagram's actual size)    */
    /*and store the ratio thumbnail/adjustement(speedup on motion)*/
    adj = nav->ddisp->hsbdata;
    reset_sc_adj (adj, rect.left, rect.right, canvas_width / nav->ddisp->zoom_factor);
    nav->hadj_coef = (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) - gtk_adjustment_get_lower (adj)) / (nav->width - nav->frame_w);

    adj = nav->ddisp->vsbdata;
    reset_sc_adj (adj, rect.top, rect.bottom, canvas_height / nav->ddisp->zoom_factor);
    nav->vadj_coef = (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj) - gtk_adjustment_get_lower (adj)) / (nav->height - nav->frame_h);
  }

  /*--GUI*/
  /*popup window, and cute frame*/
  popup_window = gtk_window_new (GTK_WINDOW_POPUP);
  nav->popup_window = popup_window;
  gtk_window_set_position (GTK_WINDOW (popup_window), GTK_WIN_POS_MOUSE);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_OUT);

  /*drawing area*/
  drawing_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (drawing_area, nav->width, nav->height);

  gtk_widget_set_events (drawing_area, 0
                         | GDK_EXPOSURE_MASK
                         | GDK_POINTER_MOTION_MASK
                         | GDK_BUTTON_RELEASE_MASK
                         );

  g_signal_connect (G_OBJECT (drawing_area), "expose_event",
                    G_CALLBACK (on_da_expose_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",
                    G_CALLBACK (on_da_motion_notify_event), NULL);
  g_signal_connect (G_OBJECT (drawing_area), "button_release_event",
                    G_CALLBACK (on_da_button_release_event), NULL);

  /*packing*/
  gtk_container_add (GTK_CONTAINER (frame), drawing_area);
  gtk_container_add (GTK_CONTAINER (popup_window), frame);
  gtk_widget_show (drawing_area);
  gtk_widget_show (frame);
  gtk_widget_show (popup_window);

  /*cursor*/
  if (MIN(nav->frame_h, nav->frame_w) > STD_CURSOR_MIN) {
    nav->cursor = gdk_cursor_new (GDK_FLEUR);
  } else { /*the miniframe is very small, so we use a minimalist cursor*/
    gchar cursor_none_data[] = { 0x00 };
    GdkBitmap * bitmap;
    GdkColor fg = { 0, 65535, 65535, 65535};
    GdkColor bg = { 0, 0, 0, 0 };

    bitmap = gdk_bitmap_create_from_data(NULL, cursor_none_data, 1, 1);
    nav->cursor = gdk_cursor_new_from_pixmap(bitmap, bitmap, &fg, &bg, 1, 1);
    g_clear_object (&bitmap);
  }

  /*grab the pointer*/
  gdk_pointer_grab (gtk_widget_get_window (drawing_area), TRUE,
                    GDK_BUTTON_RELEASE_MASK | GDK_BUTTON_MOTION_MASK,
                    gtk_widget_get_window (drawing_area),
                    nav->cursor,
                    GDK_CURRENT_TIME);

  /* surface to draw the thumbnail on */
  nav->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                            nav->width, nav->height);

  renderer = g_object_new (DIA_CAIRO_TYPE_RENDERER, NULL);
  renderer->scale = zoom;
  renderer->surface = cairo_surface_reference (nav->surface);

  /*render the data*/
  data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

  g_clear_object (&renderer);

  nav->is_first_expose = TRUE;/*set to request to draw the miniframe*/
}


/* resets adjustement to diagram size */
static void
reset_sc_adj (GtkAdjustment * adj, gdouble lower, gdouble upper, gdouble page)
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


static gboolean
on_da_expose_event (GtkWidget * widget, GdkEventExpose * event, gpointer unused)
{
  GtkAdjustment * adj;
  int x, y;
  cairo_t *ctx;

  ctx = gdk_cairo_create (gtk_widget_get_window (widget));
  cairo_set_line_width (ctx, FRAME_THICKNESS);
  cairo_set_line_cap (ctx, CAIRO_LINE_CAP_BUTT);
  cairo_set_line_join (ctx, CAIRO_LINE_JOIN_MITER);

  /*refresh the part outdated by the event*/
  cairo_set_source_surface (ctx, nav->surface,
                            event->area.x, event->area.y);
  cairo_rectangle (ctx, event->area.x, event->area.y,
                        event->area.width, event->area.height);
  cairo_fill (ctx);

  adj = nav->ddisp->hsbdata;
  x = (gtk_adjustment_get_value (adj) - gtk_adjustment_get_lower (adj)) / (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) * (nav->width) +1;

  adj = nav->ddisp->vsbdata;
  y = (gtk_adjustment_get_value (adj) - gtk_adjustment_get_lower (adj)) / (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_lower (adj)) * (nav->height) +1;

  /*draw directly on the window, do not buffer the miniframe*/
  cairo_set_source_rgb (ctx, 0, 0, 0);
  cairo_rectangle (ctx, x, y, nav->frame_w, nav->frame_h);
  cairo_stroke (ctx);

  nav->is_first_expose = FALSE;

  return FALSE;
}


static gboolean
on_da_motion_notify_event (GtkWidget * drawing_area, GdkEventMotion * event, gpointer unused)
{
  GtkAdjustment * adj;
  gboolean value_changed;

  int w = nav->frame_w;
  int h = nav->frame_h;

  int x, y;/*top left of the miniframe*/

  /* Don't try to move if there's no room for it.*/
  if (w >= nav->width-1 && h >= nav->height-1) return FALSE;

  x = CLAMP (event->x - w/2 , 0, nav->width  - w);
  y = CLAMP (event->y - h/2 , 0, nav->height - h);

  adj = nav->ddisp->hsbdata;
  value_changed = FALSE;
  if (w/2 <= event->x && event->x <= (nav->width - w/2)){
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj) + x * nav->hadj_coef);
    value_changed = TRUE;
  }
  else if (x == 0 && gtk_adjustment_get_value (adj) != gtk_adjustment_get_lower (adj)){/*you've been too fast! :)*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
    value_changed = TRUE;
  }
  else if (x == (nav->width - w) && gtk_adjustment_get_value (adj) != (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj))){/*idem*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
    value_changed = TRUE;
  }
  if (value_changed) gtk_adjustment_value_changed(adj);

  adj = nav->ddisp->vsbdata;
  value_changed = FALSE;
  if (h/2 <= event->y && event->y <= (nav->height - h/2)){
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj) + y * nav->vadj_coef);
    value_changed = TRUE;
  }
  else if (y == 0 && gtk_adjustment_get_value (adj) != gtk_adjustment_get_lower (adj)){/*you've been too fast! :)*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_lower (adj));
    value_changed = TRUE;
  }
  else if (y == (nav->height - h) && gtk_adjustment_get_value (adj) != (gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj))){/*idem*/
    gtk_adjustment_set_value (adj, gtk_adjustment_get_upper (adj) - gtk_adjustment_get_page_size (adj));
    value_changed = TRUE;
  }
  if (value_changed) gtk_adjustment_value_changed(adj);

  /* Trigger redraw */
  gdk_window_invalidate_rect (gtk_widget_get_window (drawing_area), NULL, TRUE);

  return FALSE;
}


static gboolean
on_da_button_release_event (GtkWidget * widget, GdkEventButton * event, gpointer unused)
{
  /* Apparently there are circumstances where this is run twice for one popup
   * Protected calls to avoid crashing on second pass.
   */
  if (nav->cursor)
    gdk_cursor_unref (nav->cursor);
  nav->cursor = NULL;

  if (nav->popup_window)
    gtk_widget_destroy (nav->popup_window);
  nav->popup_window = NULL;

/*returns the focus on the canvas*/
  gtk_widget_grab_focus(nav->ddisp->canvas);
  return FALSE;
}

static void
on_button_navigation_popup_released (GtkButton * button, gpointer z)
{
  /* don't popdown before having drawn once */
  if (!nav->is_first_expose) /* needed for gtk+-2.6.x, but work for 2.6 too. */
    on_da_button_release_event (NULL, NULL, NULL);
}
