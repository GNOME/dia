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

#include "color_area.h"
#include "attributes.h"
#include "persistence.h"

#define FORE_AREA 0
#define BACK_AREA 1
#define SWAP_AREA 2
#define DEF_AREA  3

#define FOREGROUND 0
#define BACKGROUND 1

/*  Global variables  */
int active_color = 0;

/*  Static variables  */
GtkWidget *color_area;
static GdkPixbuf *default_pixmap = NULL;
static GdkPixbuf *swap_pixmap = NULL;

static GtkWidget *color_select = NULL;
static int color_select_active = 0;
static int edit_color;
static GdkRGBA stored_foreground;
static GdkRGBA stored_background;


static void
color_selection_ok (GtkWidget               *w,
                    GtkColorSelectionDialog *cs);
static void
color_selection_cancel (GtkWidget               *w,
			GtkColorSelectionDialog *cs);
static void
color_selection_cancel (GtkWidget               *w,
			GtkColorSelectionDialog *cs);
static gint
color_selection_delete (GtkWidget               *w,
			GdkEvent                *event,
			GtkColorSelectionDialog *cs);
static gint
color_selection_destroy (GtkWidget               *w,
			 GtkColorSelectionDialog *cs);
static void
color_selection_changed (GtkWidget *w,
                         GtkColorSelectionDialog *cs);

/*  Local functions  */
static int
color_area_target (int x,
                   int y)
{
  gint rect_w, rect_h;
  gint width, height;

  width = gtk_widget_get_allocated_width (color_area);
  height = gtk_widget_get_allocated_height (color_area);

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

static void
color_area_draw (GtkWidget *self, cairo_t *color_area_ctx)
{
  GdkRGBA *win_bg;
  GdkRGBA fg, bg;
  gint rect_w, rect_h;
  gint width, height;
  gint img_width, img_height;
  GtkStyle *style;
  GtkStyleContext *style_ctx;

  width = gtk_widget_get_allocated_width (self);
  height = gtk_widget_get_allocated_height (self);

  style = gtk_widget_get_style (self);
  style_ctx = gtk_widget_get_style_context (self);
  fg = attributes_get_foreground();
  bg = attributes_get_background();

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  gtk_render_background (style_ctx, color_area_ctx, 0, 0, width, height);

  gdk_cairo_set_source_rgba (color_area_ctx, &bg);

  cairo_rectangle (color_area_ctx,
                   (width - rect_w), (height - rect_h), rect_w, rect_h);
  cairo_fill (color_area_ctx);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (style, color_area_ctx, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      self, NULL,
		      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);
  else
    gtk_paint_shadow (style, color_area_ctx, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      self, NULL,
		      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);

  gdk_cairo_set_source_rgba (color_area_ctx, &fg);
  cairo_rectangle (color_area_ctx, 0, 0, rect_w, rect_h);
  cairo_fill (color_area_ctx);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (style, color_area_ctx, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      self, NULL,
                      0, 0,
                      rect_w, rect_h);
  else
    gtk_paint_shadow (style, color_area_ctx, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      self, NULL,
                      0, 0,
                      rect_w, rect_h);

  /*  draw the default colours pixmap  */
  img_width = gdk_pixbuf_get_width (default_pixmap);
  img_height = gdk_pixbuf_get_height (default_pixmap);
  gdk_cairo_set_source_pixbuf (color_area_ctx, default_pixmap, 0, height - img_height);
  cairo_rectangle (color_area_ctx, 0, height - img_height, img_width, img_height);
  cairo_fill (color_area_ctx);

  /*  draw the swap pixmap  */
  img_width = gdk_pixbuf_get_width (swap_pixmap);
  img_height = gdk_pixbuf_get_height (swap_pixmap);
  gdk_cairo_set_source_pixbuf (color_area_ctx, swap_pixmap, width - img_width, 0);
  cairo_rectangle (color_area_ctx, width - img_width, 0, img_width, img_height);
  cairo_fill (color_area_ctx);
}

static void
color_selection_ok (GtkWidget               *w,
                    GtkColorSelectionDialog *cs)
{
  GtkColorSelection *colorsel;
  GdkRGBA color;

  colorsel=GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(cs));

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (colorsel), &color);

  if (edit_color == FOREGROUND) {
    attributes_set_foreground(&color);
  } else {
    attributes_set_background(&color);
  }
  /* Trigger redraw */
  gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);

  /*  gtk_color_selection_set_currentcolor(colorsel,&color);*/

  gtk_widget_hide(color_select);
  color_select_active = 0;
}

static void
color_selection_cancel (GtkWidget               *w,
			GtkColorSelectionDialog *cs)
{
  if (color_select != NULL)
    gtk_widget_hide(color_select);
  color_select_active = 0;
  attributes_set_foreground(&stored_foreground);
  attributes_set_background(&stored_background);
  
  /* Trigger redraw */
  gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);
}

static gint
color_selection_delete (GtkWidget               *w,
			GdkEvent                *event,
			GtkColorSelectionDialog *cs)
{
  color_selection_cancel(w,cs);
  return TRUE;
}

static gint
color_selection_destroy (GtkWidget               *w,
			 GtkColorSelectionDialog *cs)
{
  color_select = NULL;
  color_selection_cancel(w,cs);
  return TRUE;
}

static void
color_selection_changed (GtkWidget *w,
                         GtkColorSelectionDialog *cs)
{
  GtkColorSelection *colorsel;
  GdkRGBA color;
  guint alpha;

  colorsel=GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(cs));

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (colorsel), &color);
  gtk_color_selection_get_current_rgba (colorsel,&color);

  if (edit_color == FOREGROUND) {
    attributes_set_foreground(&color);
  } else {
    attributes_set_background(&color);
  }
  /* Trigger redraw */
  gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);
}

static void
color_area_edit (void)
{
  GtkWidget *window;
  GdkRGBA color;
  GtkColorSelectionDialog *csd;
  GtkColorSelection *colorsel;

  if (!color_select_active) {
    stored_foreground  = attributes_get_foreground();
    stored_background  = attributes_get_background();
  }
  
  if (active_color == FOREGROUND) {
    color = attributes_get_foreground();
    edit_color = FOREGROUND;
  } else {
    color = attributes_get_background();
    edit_color = BACKGROUND;
  }

  if (color_select) {
    window = color_select;
    csd = GTK_COLOR_SELECTION_DIALOG (window);
    colorsel = GTK_COLOR_SELECTION (
		   gtk_color_selection_dialog_get_color_selection (csd));

    gtk_window_set_title(GTK_WINDOW(color_select),
			 edit_color==FOREGROUND?
			 _("Select foreground color"):
			 _("Select background color"));
    if (! color_select_active) {
      gtk_widget_show (color_select);
    }
  } else {
    GtkButton *button;

    window = color_select = 
      gtk_color_selection_dialog_new(edit_color==FOREGROUND?
				     _("Select foreground color"):
				     _("Select background color"));
    csd = GTK_COLOR_SELECTION_DIALOG (window);
    colorsel = GTK_COLOR_SELECTION (
		   gtk_color_selection_dialog_get_color_selection (csd));

    color_select_active = 1;
    gtk_color_selection_set_has_opacity_control (colorsel, TRUE);

    gtk_color_selection_set_has_palette (colorsel, TRUE);

    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    
    g_signal_connect (G_OBJECT (window), "delete_event",
		      G_CALLBACK(color_selection_delete),
		      window);
    
    g_signal_connect (G_OBJECT (window), "destroy",
		      G_CALLBACK(color_selection_destroy),
		      window);
    
    g_signal_connect (G_OBJECT (colorsel),
		      "color_changed",
		      G_CALLBACK(color_selection_changed),
		      window);

    g_object_get (G_OBJECT (window), "ok-button", &button, NULL);
    g_signal_connect (button, "clicked",
		      G_CALLBACK(color_selection_ok),
		      window);

    g_object_get (G_OBJECT (window), "cancel-button", &button, NULL);
    g_signal_connect (button, "clicked",
		      G_CALLBACK(color_selection_cancel),
		      window);

    /* Make sure window is shown before setting its colors: */
    gtk_widget_show_now (color_select);      
  }

  gtk_color_selection_set_current_rgba(colorsel, &color);
}

static gint
color_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  int target;
  
  switch (event->type) {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;

      if (bevent->button == 1) {
        switch ((target = color_area_target (bevent->x, bevent->y))) {
          case FORE_AREA:
          case BACK_AREA:
            if (target == active_color) {
              color_area_edit ();
            } else {
              active_color = target;
              /* Trigger redraw */
              gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);
            }
            break;
          case SWAP_AREA:
            attributes_swap_fgbg();
            /* Trigger redraw */
            gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);
            break;
          case DEF_AREA:
            attributes_default_fgbg();
            /* Trigger redraw */
            gdk_window_invalidate_rect (gtk_widget_get_window (color_area), NULL, TRUE);
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

GtkWidget *
color_area_create (int width,
                   int height)
{
  GtkWidget *event_box;

  default_pixmap =
    gdk_pixbuf_new_from_xpm_data (default_xpm);
  swap_pixmap =
    gdk_pixbuf_new_from_xpm_data (swap_xpm);

  attributes_set_foreground(persistence_register_color("fg_color", &color_black));
  attributes_set_background(persistence_register_color("bg_color", &color_white));

  event_box = gtk_event_box_new();
  color_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (color_area, width, height);
  gtk_widget_set_events (color_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (color_area), "event",
		       G_CALLBACK(color_area_events),
		       NULL);
  g_signal_connect (G_OBJECT (color_area), "draw",
                    G_CALLBACK (color_area_draw), NULL);

  gtk_widget_show(color_area);
  gtk_container_add(GTK_CONTAINER(event_box), color_area);
  return event_box;
}
