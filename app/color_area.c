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
#include <stdio.h>

#include "color_area.h"
#include "attributes.h"

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
static GdkPixmap *color_area_pixmap = NULL;
static GdkPixmap *default_pixmap = NULL;
static GdkPixmap *swap_pixmap = NULL;

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
static void
color_selection_changed (GtkWidget *w,
                         GtkColorSelectionDialog *cs);

/*  Local functions  */
static int
color_area_target (int x,
                   int y)
{
  int rect_w, rect_h;
  int width, height;

  gdk_window_get_size (color_area_pixmap, &width, &height);

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
  GdkColor fg, bg, bd;
  int rect_w, rect_h;
  int width, height;
  int def_width, def_height;
  int swap_width, swap_height;

  if (!color_area_pixmap)     /* we haven't gotten initial expose yet,
                               * no point in drawing anything */
     return;

  if (!color_area_gc)
    color_area_gc = gdk_gc_new (color_area_pixmap);

  gdk_window_get_size (color_area_pixmap, &width, &height);

  win_bg = &(color_area->style->bg[GTK_STATE_NORMAL]);
  col = attributes_get_foreground();
  color_convert(&col, &fg);
  col = attributes_get_background();
  color_convert(&col, &bg);
  bd = color_gdk_black;

  rect_w = width * 0.65;
  rect_h = height * 0.65;

  gdk_gc_set_foreground (color_area_gc, win_bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, width, height);

  gdk_gc_set_foreground (color_area_gc, &bg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      (width - rect_w), (height - rect_h), rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		     (width - rect_w), (height - rect_h), rect_w, rect_h);
  else
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     (width - rect_w), (height - rect_h), rect_w, rect_h);

  gdk_gc_set_foreground (color_area_gc, &fg);
  gdk_draw_rectangle (color_area_pixmap, color_area_gc, 1,
		      0, 0, rect_w, rect_h);

  if (active_color == FOREGROUND)
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_IN,
		     0, 0, rect_w, rect_h);
  else
    gtk_draw_shadow (color_area->style, color_area_pixmap, GTK_STATE_NORMAL, GTK_SHADOW_OUT,
		     0, 0, rect_w, rect_h);


  gdk_window_get_size (default_pixmap, &def_width, &def_height);
  gdk_draw_pixmap (color_area_pixmap, color_area_gc, default_pixmap,
		   0, 0, 0, height - def_height, def_width, def_height);

  gdk_window_get_size (swap_pixmap, &swap_width, &swap_height);
  gdk_draw_pixmap (color_area_pixmap, color_area_gc, swap_pixmap,
		   0, 0, width - swap_width, 0, swap_width, swap_height);

  gdk_draw_pixmap (color_area->window, color_area_gc, color_area_pixmap,
		   0, 0, 0, 0, width, height);
}

static void
color_selection_ok (GtkWidget               *w,
                    GtkColorSelectionDialog *cs)
{
  GtkColorSelection *colorsel;
  gdouble color[3];
  Color col;

  colorsel=GTK_COLOR_SELECTION(cs->colorsel);

  gtk_color_selection_get_color(colorsel,color);
  col.red = color[0];
  col.green= color[1];
  col.blue = color[2];

  if (edit_color == FOREGROUND) {
    attributes_set_foreground(&col);
  } else {
    attributes_set_background(&col);
  }
  color_area_draw ();

  gtk_color_selection_set_color(colorsel,color);

  gtk_widget_hide(color_select);
  color_select_active = 0;
}

static void
color_selection_cancel (GtkWidget               *w,
			GtkColorSelectionDialog *cs)
{
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

static void
color_selection_changed (GtkWidget *w,
                         GtkColorSelectionDialog *cs)
{
  GtkColorSelection *colorsel;
  gdouble color[3];
  Color col;

  colorsel=GTK_COLOR_SELECTION(cs->colorsel);

  gtk_color_selection_get_color(colorsel,color);
  col.red = color[0];
  col.green= color[1];
  col.blue = color[2];

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
  gdouble color[3];
  GtkWidget *window;

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

  if (! color_select) {
    window = color_select = gtk_color_selection_dialog_new("Select color");
    color_select_active = 1;
    
    gtk_color_selection_set_update_policy(
        GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (window)->colorsel),
	GTK_UPDATE_CONTINUOUS);
    
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
    
    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC(color_selection_delete),
			window);
    
    gtk_signal_connect (
	GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (window)->colorsel),
	"color_changed",
	GTK_SIGNAL_FUNC(color_selection_changed),
	window);
    
    gtk_signal_connect (
	GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (window)->ok_button),
	"clicked",
	GTK_SIGNAL_FUNC(color_selection_ok),
	window);

    gtk_signal_connect (
	GTK_OBJECT (GTK_COLOR_SELECTION_DIALOG (window)->cancel_button),
	"clicked",
	GTK_SIGNAL_FUNC(color_selection_cancel),
	window);


    /* Make sure window is shown before setting its colors: */
    gtk_widget_realize (window);
    gtk_widget_show (window);
    while ( gtk_events_pending() ) {
      gtk_main_iteration();
    }
      
    color[0] = col.red;
    color[1] = col.green;
    color[2] = col.blue;

    gtk_color_selection_set_color(
         GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (window)->colorsel),
   	 color);
    gtk_color_selection_set_color(
         GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (window)->colorsel),
	 color);
  } else {
    if (! color_select_active) {
      gtk_widget_show (color_select);
    }
    color[0] = col.red;
    color[1] = col.green;
    color[2] = col.blue;

    gtk_color_selection_set_color(
         GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (color_select)->colorsel),
   	 color);
    gtk_color_selection_set_color(
         GTK_COLOR_SELECTION (GTK_COLOR_SELECTION_DIALOG (color_select)->colorsel),
	 color);
  }
}

static gint
color_area_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventConfigure *cevent;
  int target;

  
  
  switch (event->type)
    {
    case GDK_CONFIGURE:
      cevent = (GdkEventConfigure *)  event;
      if (cevent->width != 1) {
	color_area_pixmap = gdk_pixmap_new (widget->window,
					    cevent->width,
					    cevent->height, -1);
      }
      break;
    case GDK_EXPOSE:
      if (color_area_pixmap)
	color_area_draw ();
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


GtkWidget *
color_area_create (int        width,
		   int        height,
		   GdkPixmap *default_pmap,
		   GdkPixmap *swap_pmap)
{
  color_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (color_area), width, height);
  gtk_widget_set_events (color_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (color_area), "event",
		      (GtkSignalFunc) color_area_events,
		      NULL);
  default_pixmap = default_pmap;
  swap_pixmap    = swap_pmap;

  return color_area;
}

