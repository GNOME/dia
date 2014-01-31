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

#include "linewidth_area.h"
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

static void linewidth_create_dialog(GtkWindow *toplevel);

static int active_linewidth = 2;
static GdkGC *linewidth_area_gc = NULL;
static GdkPixmap *linewidth_area_pixmap = NULL;

static GtkWidget *linewidth_area_widget = NULL;
static GtkWidget *linewidth_dialog = NULL;
static GtkWidget *linewidth_button = NULL;

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

static void
linewidth_area_draw (GtkWidget *linewidth_area)
{
  GdkColor *win_bg, *win_fg;
  int width, height;
  int i;
  int x_offs;
  GtkStyle *style;
  
  if (!linewidth_area_pixmap)     /* we haven't gotten initial expose yet,
                               * no point in drawing anything */
    return;


  if (!linewidth_area_gc) {
    linewidth_area_gc = gdk_gc_new (linewidth_area_pixmap);
    gdk_gc_set_line_attributes(linewidth_area_gc, 1,
			       GDK_LINE_ON_OFF_DASH,
			       GDK_CAP_BUTT,
			       GDK_JOIN_MITER);
  }

  gdk_drawable_get_size (linewidth_area_pixmap, &width, &height);

  style = gtk_widget_get_style (linewidth_area);
  win_bg = &(style->bg[GTK_STATE_NORMAL]);
  win_fg = &(style->fg[GTK_STATE_NORMAL]);

  gdk_gc_set_foreground (linewidth_area_gc, win_bg);
  gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 1,
		      0, 0, width, height);

  gdk_gc_set_foreground (linewidth_area_gc, win_fg);
  
  for (i=0;i<=NUMLINES;i++) {
    x_offs = X_OFFSET(i);

    gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 1,
			x_offs, 2, i, height-4);
    
  }
  
  if (active_linewidth != 0) {
  gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 0,
		      X_OFFSET(active_linewidth)-2, 0,
		      active_linewidth+4, height-1);
  }

  gdk_draw_drawable (gtk_widget_get_window(linewidth_area), linewidth_area_gc, linewidth_area_pixmap,
		     0, 0, 0, 0, width, height);
}

static gint
linewidth_area_events (GtkWidget *widget,
		       GdkEvent  *event)
{
  GdkEventButton *bevent;
  GdkEventConfigure *cevent;
  int target;
  
  switch (event->type)
    {
    case GDK_CONFIGURE:
      cevent = (GdkEventConfigure *)  event;
      if (cevent->width > 1) {
	linewidth_area_pixmap = gdk_pixmap_new (gtk_widget_get_window(widget),
						cevent->width,
						cevent->height, -1);
      }
      break;
    case GDK_EXPOSE:
      linewidth_area_draw (linewidth_area_widget);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      if (bevent->button == 1) {
	target = linewidth_area_target (bevent->x, bevent->y);
	
	if (target != 0) {
	  active_linewidth = target;
	  linewidth_area_draw(linewidth_area_widget);
	  attributes_set_default_linewidth(BASE_WIDTH*(target-1));
	}
      }
      break;

    case GDK_2BUTTON_PRESS:
      if (linewidth_dialog == NULL)
        linewidth_create_dialog(GTK_WINDOW (gtk_widget_get_toplevel (widget)));
      else
        gtk_widget_grab_focus(linewidth_button);
      gtk_spin_button_set_value(GTK_SPIN_BUTTON(linewidth_button), attributes_get_default_linewidth());

      gtk_widget_show(linewidth_dialog);
      break;

    default:
      break;
    }

  return FALSE;
}


static int
linewidth_number_from_width(real width)
{
  if (fabs(width/BASE_WIDTH-rint(width/BASE_WIDTH)) > 0.0005 ||
      (width/BASE_WIDTH > NUMLINES)) {
    return 0;
  } else {
    return width/BASE_WIDTH+1.0005;
  }
}

GtkWidget *
linewidth_area_create (void)
{
  GtkWidget *linewidth_area;
  GtkWidget *event_box;

  attributes_set_default_linewidth(persistence_register_real("linewidth", 0.1));
  active_linewidth = linewidth_number_from_width(attributes_get_default_linewidth());

  event_box = gtk_event_box_new();
  linewidth_area = gtk_drawing_area_new ();
  gtk_widget_set_size_request (linewidth_area, AREA_WIDTH, AREA_HEIGHT);
  gtk_widget_set_events (linewidth_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  g_signal_connect (G_OBJECT (linewidth_area), "event",
		    G_CALLBACK(linewidth_area_events),
		      NULL);

  linewidth_area_widget = linewidth_area;

  gtk_container_add(GTK_CONTAINER(event_box), linewidth_area);
  gtk_widget_show(linewidth_area);
  return event_box;
}

static void 
get_current_line_width() 
{
  float newvalue = gtk_spin_button_get_value (GTK_SPIN_BUTTON (linewidth_button));
  active_linewidth = linewidth_number_from_width(newvalue);
  linewidth_area_draw(GTK_WIDGET(linewidth_area_widget));
  attributes_set_default_linewidth(newvalue);
}

static void
linewidth_dialog_respond(GtkWidget *widget, gint response_id, gpointer data)
{
  if (response_id == GTK_RESPONSE_OK) {
    get_current_line_width();
  }
  gtk_widget_hide(linewidth_dialog);
}

static void
linewidth_dialog_ok(GtkWidget *widget, gpointer data)
{
  gtk_dialog_response(GTK_DIALOG(linewidth_dialog), GTK_RESPONSE_OK);
}

/* Crashes with gtk_widget_destroyed, so use this instead */
static void
dialog_destroyed(GtkWidget *widget, gpointer data)
{
  GtkWidget **wid = (GtkWidget**)data;
  if (wid) *wid = NULL;
}

static void
linewidth_create_dialog(GtkWindow *toplevel)
{
  GtkWidget *hbox;
  GtkWidget *label;
  GtkAdjustment *adj;

  linewidth_dialog = gtk_dialog_new_with_buttons(
	_("Line width"), toplevel,
	0,
	GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
	GTK_STOCK_OK, GTK_RESPONSE_OK, 
	NULL);
  
  gtk_dialog_set_default_response (GTK_DIALOG(linewidth_dialog), GTK_RESPONSE_OK);
  gtk_window_set_role (GTK_WINDOW (linewidth_dialog), "linewidth_window");
  gtk_window_set_resizable (GTK_WINDOW (linewidth_dialog), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (linewidth_dialog), 2);

  hbox = gtk_hbox_new(FALSE, 5);
  label = gtk_label_new(_("Line width:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
  gtk_widget_show (label);
  adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.05, 0.0);
  linewidth_button = gtk_spin_button_new(adj, attributes_get_default_linewidth(), 2);
  gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(linewidth_button), TRUE);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(linewidth_button), TRUE);
  gtk_box_pack_start(GTK_BOX (hbox), linewidth_button, TRUE, TRUE, 0);
  gtk_widget_show (linewidth_button);
  gtk_widget_show(hbox);
  gtk_box_pack_start (GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG (linewidth_dialog))), hbox, TRUE, TRUE, 0);
  
  gtk_widget_show (linewidth_button);

  g_signal_connect(G_OBJECT (linewidth_dialog), "response",
                   G_CALLBACK (linewidth_dialog_respond), NULL);
  g_signal_connect_after(G_OBJECT (linewidth_button), "activate",
                   G_CALLBACK (linewidth_dialog_ok), NULL);

  g_signal_connect (G_OBJECT (linewidth_dialog), "delete_event",
		    G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect (G_OBJECT (linewidth_dialog), "destroy",
		    G_CALLBACK(dialog_destroyed), &linewidth_dialog);

  persistence_register_window (GTK_WINDOW (linewidth_dialog));
}
