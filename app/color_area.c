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

#include "../lib/color.h"
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
static GdkGC *color_area_gc = NULL;
static GdkGC *mask_gc       = NULL;
static GdkPixmap *color_area_pixmap = NULL;
static GdkBitmap *color_area_mask   = NULL;
static GdkPixmap *default_pixmap = NULL;
static GdkBitmap *default_mask      = NULL;
static GdkPixmap *swap_pixmap = NULL;
static GdkBitmap *swap_mask         = NULL;

static GtkWidget *color_select = NULL;
static int color_select_active = 0;
static int edit_color;
static Color stored_foreground;
static Color stored_background;


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

  gdk_drawable_get_size (color_area_pixmap, &width, &height);

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
color_area_draw ()
{
  Color col;
  GdkColor *win_bg;
  GdkColor fg, bg;
  gint rect_w, rect_h;
  gint width, height;
  gint def_width, def_height;
  gint swap_width, swap_height;
  GdkColor  mask_pattern;
  GtkStyle *style;

  /* Check we haven't gotten initial expose yet,
   * no point in drawing anything
   */
  if (!color_area_pixmap || !color_area_gc)
    return;

  gdk_drawable_get_size (color_area_pixmap, &width, &height);

  style = gtk_widget_get_style(color_area);
  win_bg = &(style->bg[GTK_STATE_NORMAL]);
  col = attributes_get_foreground();
  color_convert(&col, &fg);
  col = attributes_get_background();
  color_convert(&col, &bg);

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  /*  initialize the mask to transparent  */
  mask_pattern.pixel = 0;
  gdk_gc_set_foreground (mask_gc, &mask_pattern);
  gdk_draw_rectangle (color_area_mask, mask_gc, TRUE, 0, 0, -1, -1);

  /*  set the mask's gc to opaque  */
  mask_pattern.pixel = 1;
  gdk_gc_set_foreground (mask_gc, &mask_pattern);

  gdk_gc_set_foreground (color_area_gc, win_bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, width, height);

  gdk_gc_set_foreground (color_area_gc, &bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      (width - rect_w), (height - rect_h), rect_w, rect_h);
  gdk_draw_rectangle (color_area_mask, mask_gc, TRUE,
		      (width - rect_w), (height - rect_h), rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (style, color_area_pixmap, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL, color_area, NULL,
		      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);
  else
    gtk_paint_shadow (style, color_area_pixmap, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      NULL, color_area, NULL,
		      (width - rect_w), (height - rect_h),
                      rect_w, rect_h);

  gdk_gc_set_foreground (color_area_gc, &fg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, rect_w, rect_h);
  gdk_draw_rectangle (color_area_mask, mask_gc, TRUE,
		      0, 0, rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_paint_shadow (style, color_area_pixmap, GTK_STATE_NORMAL,
                      GTK_SHADOW_IN,
                      NULL, color_area, NULL,
                      0, 0,
                      rect_w, rect_h);
  else
    gtk_paint_shadow (style, color_area_pixmap, GTK_STATE_NORMAL,
                      GTK_SHADOW_OUT,
                      NULL, color_area, NULL,
                      0, 0,
                      rect_w, rect_h);

  /*  draw the default pixmap  */
  gdk_drawable_get_size (default_pixmap, &def_width, &def_height);
  gdk_draw_drawable (color_area_pixmap, color_area_gc, default_pixmap,
		     0, 0, 0, height - def_height, def_width, def_height);
  gdk_draw_drawable (color_area_mask, mask_gc, default_mask,
		   0, 0, 0, height - def_height, def_width, def_height);

  /*  draw the swap pixmap  */
  gdk_drawable_get_size (swap_pixmap, &swap_width, &swap_height);
  gdk_draw_drawable (color_area_pixmap, color_area_gc, swap_pixmap,
		     0, 0, width - swap_width, 0, swap_width, swap_height);
  gdk_draw_drawable (color_area_mask, mask_gc, swap_mask,
		   0, 0, width - swap_width, 0, swap_width, swap_height);

  /*  draw the widget  */
  gdk_gc_set_clip_mask (color_area_gc, color_area_mask);
  gdk_gc_set_clip_origin (color_area_gc, 0, 0);
  gdk_draw_drawable (gtk_widget_get_window(color_area), color_area_gc, color_area_pixmap,
		   0, 0, 0, 0, width, height);

  /*  reset the clip mask  */
  gdk_gc_set_clip_mask (color_area_gc, NULL);
}

static void
color_selection_ok (GtkWidget               *w,
                    GtkColorSelectionDialog *cs)
{
  GtkColorSelection *colorsel;
  GdkColor color;
  guint alpha;
  Color col;

  colorsel=GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(cs));

  gtk_color_selection_get_current_color(colorsel,&color);
  GDK_COLOR_TO_DIA(color, col);

  alpha = gtk_color_selection_get_current_alpha(colorsel);
  col.alpha = alpha / 65535.0;

  if (edit_color == FOREGROUND) {
    attributes_set_foreground(&col);
  } else {
    attributes_set_background(&col);
  }
  color_area_draw ();

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
  
  color_area_draw ();
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
  GdkColor color;
  guint alpha;
  Color col;

  colorsel=GTK_COLOR_SELECTION(gtk_color_selection_dialog_get_color_selection(cs));

  gtk_color_selection_get_current_color(colorsel,&color);
  GDK_COLOR_TO_DIA(color, col);

  alpha = gtk_color_selection_get_current_alpha(colorsel);
  col.alpha = alpha / 65535.0;

  if (edit_color == FOREGROUND) {
    attributes_set_foreground(&col);
  } else {
    attributes_set_background(&col);
  }
  color_area_draw ();
}

static void
color_area_edit (void)
{
  Color col;
  GtkWidget *window;
  GdkColor color;
  GtkColorSelectionDialog *csd;
  GtkColorSelection *colorsel;

  if (!color_select_active) {
    stored_foreground  = attributes_get_foreground();
    stored_background  = attributes_get_background();
  }
  
  if (active_color == FOREGROUND) {
    col = attributes_get_foreground();
    edit_color = FOREGROUND;
  } else {
    col = attributes_get_background();
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
  DIA_COLOR_TO_GDK(col, color);

  gtk_color_selection_set_current_color(colorsel, &color);
  gtk_color_selection_set_current_alpha(colorsel, (guint)(col.alpha * 65535.0));
}

static gint
color_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  int target;
  
  switch (event->type)
    {
    case GDK_CONFIGURE:
      if (color_area_pixmap)
	{
	  g_object_unref (color_area_pixmap);
	  g_object_unref (color_area_mask);
	}

	color_area_pixmap = gdk_pixmap_new (gtk_widget_get_window(color_area),
					  color_area->allocation.width,
					  color_area->allocation.height, -1);
        color_area_mask = gdk_pixmap_new (gtk_widget_get_window(color_area),
					color_area->allocation.width,
					color_area->allocation.height, 1);
      break;
    case GDK_EXPOSE:
#if GTK_CHECK_VERSION(2,18,0)
      if (gtk_widget_is_drawable(color_area))
#else
      if (GTK_WIDGET_DRAWABLE (color_area))
#endif
	{
	  if (!color_area_gc)
	    {
	      color_area_gc = gdk_gc_new (gtk_widget_get_window(color_area));
	      mask_gc = gdk_gc_new (color_area_mask);
	    }

	  color_area_draw ();
	}
      break;
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
	      color_area_draw();
	  }
	  break;
	case SWAP_AREA:
	  attributes_swap_fgbg();
	  color_area_draw();
	  break;
	case DEF_AREA:
	  attributes_default_fgbg();
	  color_area_draw();
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
color_area_create (int        width,
		   int        height,
		   GtkWidget *parent,
		   GtkStyle  *style)
{
  GtkWidget *event_box;

  default_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &default_mask, 
		&style->bg[GTK_STATE_NORMAL], default_xpm);
  swap_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &swap_mask, 
		&style->bg[GTK_STATE_NORMAL], swap_xpm);

  attributes_set_foreground(persistence_register_color("fg_color", &color_black));
  attributes_set_background(persistence_register_color("bg_color", &color_white));

  event_box = gtk_event_box_new();
  color_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (color_area, width, height);
  gtk_widget_set_events (color_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (color_area), "event",
		       G_CALLBACK(color_area_events),
		       NULL);

  gtk_widget_show(color_area);
  gtk_container_add(GTK_CONTAINER(event_box), color_area);
  return event_box;
}
