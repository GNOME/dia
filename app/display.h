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
#ifndef DDISPLAY_H
#define DDISPLAY_H

#include <gtk/gtk.h>

typedef struct _DDisplay DDisplay;

#include "geometry.h"
#include "diagram.h"
#include "grid.h"
#include "render_gdk.h"
#include "render_libart.h"
#include "menus.h"

#define DDISPLAY_MAX_ZOOM 500.0
#define DDISPLAY_NORMAL_ZOOM 20.0
#define DDISPLAY_MIN_ZOOM 1.0

struct _DDisplay {
  Diagram *diagram;               /* pointer to the associated diagram */

  GtkWidget *shell;               /* shell widget for this ddisplay    */
  GtkWidget *canvas;              /* canvas widget for this ddisplay   */
  GtkWidget *hsb, *vsb;           /* widgets for scroll bars           */
  GtkWidget *hrule, *vrule;       /* widgets for rulers                */
  GtkWidget *origin;              /* widgets for rulers                */
  GtkWidget *menu_bar;            /* widget for the menu bar           */
  GtkItemFactory *mbar_item_factory; /* item factory used to create the menu bar */

  /* menu bar widgets */
  GtkWidget *rulers;
  GtkWidget *visible_grid;
  GtkWidget *snap_to_grid;
  GtkWidget *show_cx_pts_mitem;
#ifdef HAVE_LIBART
  GtkWidget *antialiased;
#endif
  UpdatableMenuItems updatable_menu_items;
    

  GtkWidget *snap_status;
  GtkWidget *zoom_status;         
  GtkWidget *modified_status;

  GtkAccelGroup *accel_group;
  
  GtkAdjustment *hsbdata;         /* horizontal data information       */
  GtkAdjustment *vsbdata;         /* vertical data information         */

  Point origo;                    /* The origo (lower left) position   */
  real zoom_factor;               /* zoom, 20.0 means 20 pixel == 1 cm */
  Rectangle visible;              /* The part visible in this display  */

  Grid grid;                      /* the grid in this display          */

  gboolean show_cx_pts;		  /* Connection points toggle boolean  */
  gboolean autoscroll;

  int aa_renderer;
  Renderer *renderer;
  
  GSList *update_areas;           /* Update areas list                 */
  GSList *display_areas;          /* Display areas list                */
  guint update_id;                /* idle handler ID for redraws       */

  /* input contexts */
  GdkIC *ic;
  GdkICAttr *ic_attr;
};

extern GdkCursor *default_cursor;

DDisplay *new_display(Diagram *dia);
/* Normal destroy is done through shell widget destroy event. */
void ddisplay_really_destroy(DDisplay *ddisp); 
void ddisplay_transform_coords_double(DDisplay *ddisp,
				      coord x, coord y,
				      double *xi, double *yi);
void ddisplay_transform_coords(DDisplay *ddisp,
			       coord x, coord y,
			       int *xi, int *yi);
void ddisplay_untransform_coords(DDisplay *ddisp,
				 int xi, int yi,
				 coord *x, coord *y);
real ddisplay_transform_length(DDisplay *ddisp, real len);
real ddisplay_untransform_length(DDisplay *ddisp, real len);
void ddisplay_add_update_pixels(DDisplay *ddisp, Point *point,
				       int pixel_width, int pixel_height);
void ddisplay_add_update_all(DDisplay *ddisp);
void ddisplay_add_update(DDisplay *ddisp, Rectangle *rect);
void ddisplay_add_display_area(DDisplay *ddisp,
			       int left, int top,
			       int right, int bottom);
void ddisplay_flush(DDisplay *ddisp);
void ddisplay_update_scrollbars(DDisplay *ddisp);
void ddisplay_set_origo(DDisplay *ddisp,
			coord x, coord y);
void ddisplay_zoom(DDisplay *ddisp, Point *point,
		   real zoom_factor);

void ddisplay_set_renderer(DDisplay *ddisp, int aa_renderer);
void ddisplay_resize_canvas(DDisplay *ddisp,
			    int width,
			    int height);

void ddisplay_render_pixmap(DDisplay *ddisp, Rectangle *update);

DDisplay *ddisplay_active(void);
Diagram *ddisplay_active_diagram(void);

void ddisplay_close(DDisplay *ddisp);

void ddisplay_set_title(DDisplay *ddisp, char *title);
void ddisplay_set_cursor(DDisplay *ddisp, GdkCursor *cursor);
void ddisplay_set_all_cursor(GdkCursor *cursor);

void ddisplay_scroll(DDisplay *ddisp, Point *delta);
gboolean ddisplay_autoscroll(DDisplay *ddisp, int x, int y);
void ddisplay_scroll_up(DDisplay *ddisp);
void ddisplay_scroll_down(DDisplay *ddisp);
void ddisplay_scroll_left(DDisplay *ddisp);
void ddisplay_scroll_right(DDisplay *ddisp);

void display_update_menu_state(DDisplay *ddisp);
void ddisplay_update_statusbar(DDisplay *ddisp);
void ddisplay_do_update_menu_sensitivity (DDisplay *ddisp);
/* Have to be called from interface.c */
GtkPixmap *snap_status_load_images(GdkWindow *window);

void display_set_active(DDisplay *ddisp);

#endif /* DDISPLAY_H */
