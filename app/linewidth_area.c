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
#include "linewidth_area.h"
#include "attributes.h"

#define BASE_WIDTH 0.05
#define PIXELS_BETWEEN_LINES 6
#define NUMLINES 5

#define X_OFFSET(i) (PIXELS_BETWEEN_LINES*(i)+((i)-1)*(i)/2)

#define AREA_WIDTH X_OFFSET(NUMLINES+1)
#define AREA_HEIGHT 42


static int active_linewidth = 2;
static GdkGC *linewidth_area_gc = NULL;
static GdkPixmap *linewidth_area_pixmap = NULL;


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
  GdkColor *win_bg;
  GdkColor line;
  int width, height;
  int i;
  int x_offs;
  
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

  gdk_window_get_size (linewidth_area_pixmap, &width, &height);

  win_bg = &(linewidth_area->style->bg[GTK_STATE_NORMAL]);
  line = color_gdk_black;

  gdk_gc_set_foreground (linewidth_area_gc, win_bg);
  gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 1,
		      0, 0, width, height);

  gdk_gc_set_foreground (linewidth_area_gc, &line);
  
  for (i=0;i<=NUMLINES;i++) {
    x_offs = X_OFFSET(i);

    gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 1,
			x_offs, 2, i, height-4);
    
  }
  
  gdk_draw_rectangle (linewidth_area_pixmap, linewidth_area_gc, 0,
		      X_OFFSET(active_linewidth)-2, 0,
		      active_linewidth+4, height-1);

  gdk_draw_pixmap (linewidth_area->window, linewidth_area_gc, linewidth_area_pixmap,
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
      if (cevent->width != 1) {
	linewidth_area_pixmap = gdk_pixmap_new (widget->window,
						cevent->width,
						cevent->height, -1);
      }
      break;
    case GDK_EXPOSE:
      linewidth_area_draw (widget);
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      if (bevent->button == 1) {
	target = linewidth_area_target (bevent->x, bevent->y);
	
	if (target != 0) {
	  active_linewidth = target;
	  linewidth_area_draw(widget);
	  attributes_set_default_linewidth(BASE_WIDTH*target);
	}
      }
      break;

    default:
      break;
    }

  return FALSE;
}


GtkWidget *
linewidth_area_create (void)
{
  GtkWidget *linewidth_area;

  linewidth_area = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (linewidth_area),
			 AREA_WIDTH, AREA_HEIGHT);
  gtk_widget_set_events (linewidth_area, GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect (GTK_OBJECT (linewidth_area), "event",
		      (GtkSignalFunc) linewidth_area_events,
		      NULL);

  attributes_set_default_linewidth(BASE_WIDTH*active_linewidth);

  return linewidth_area;
}


