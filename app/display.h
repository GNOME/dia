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

#include "geometry.h"
#include "diagram.h"

typedef struct _DiaDisplay DiaDisplay;

#include "grid.h"
#include "diarenderer.h"

G_BEGIN_DECLS

/** Defines the pixels per cm, default is 20 pixels = 1 cm */
/** This is close to, but not quite the same as, the physical display size
 * in most cases */

#define DDISPLAY_MAX_ZOOM 2000.0
#define DDISPLAY_NORMAL_ZOOM 20.0
#define DDISPLAY_MIN_ZOOM 0.2

#define DIA_DISPLAY_DATA_HACK "DiaDisplay"

/* The zoom amount should be uniform.  Pixels per cm should be defined by the
 * renderer alone.  But that'd take a lot of fiddling in renderers. */
/*
#define DDISPLAY_MAX_ZOOM 100.0
#define DDISPLAY_NORMAL_ZOOM 1.0
#define DDISPLAY_MIN_ZOOM 0.01
*/
struct _DiaDisplay {
  GObject parent;

  Diagram *diagram;                  /* pointer to the associated diagram */

  GtkWidget      *shell;               /* shell widget for this ddisplay    */
  GtkWidget      *canvas;              /* canvas widget for this ddisplay   */
  GtkWidget      *hsb, *vsb;           /* widgets for scroll bars           */
  GtkWidget      *hrule, *vrule;       /* widgets for rulers                */
  GtkWidget      *origin;              /* either decoration or menu button  */
  GtkWidget      *menu_bar;            /* widget for the menu bar           */
  GtkUIManager   *ui_manager;     /* ui manager used to create the menu bar */
  GtkActionGroup *actions;        

  /* menu bar widgets */
  GtkMenuItem *rulers;

  GtkWidget *zoom_status;
  GtkWidget *grid_status;
  GtkWidget *mainpoint_status;
  GtkWidget *modified_status;

  GtkAccelGroup *accel_group;
  
  GtkAdjustment *hsbdata;         /* horizontal data information       */
  GtkAdjustment *vsbdata;         /* vertical data information         */

  Point origo;                    /* The origo (lower left) position   */
  real zoom_factor;               /* zoom, 20.0 means 20 pixel == 1 cm */
  Rectangle visible;              /* The part visible in this display  */

  Grid grid;                      /* the grid in this display          */

  gboolean show_cx_pts;           /* Connection points toggle boolean  */
  gboolean autoscroll;
  gboolean mainpoint_magnetism;   /* Mainpoints snapped from entire obj*/

  int aa_renderer;
  DiaRenderer *renderer;
  
  GSList *update_areas;           /* Update areas list                 */

  GtkIMContext *im_context;

  /* Preedit String */
  gchar *preedit_string;
  PangoAttrList *preedit_attrs;
 
  /* Is there another case?  Like I see embedded-dia modules, do these do something
   * in addition??? */  
  gboolean   is_standalone_window;

  /* Points to Integrated UI Toolbar */
  GtkToolbar *common_toolbar;

  /* Points to widget containing the diagram if not standalone window */
  GtkWidget *container; 

  /* Private field, indicates if rulers are shown for this diagram. */
  gboolean rulers_are_showing;

  /* Private field, indicates which text, if any, is being edited */
  Focus *active_focus;
  
  /* Rember the last clicked point per display, but in diagram coordinates */
  Point clicked_position;
};

extern GdkCursor *default_cursor;

#define DIA_TYPE_DISPLAY (dia_display_get_type ())
G_DECLARE_FINAL_TYPE (DiaDisplay, dia_display, DIA, DISPLAY, GObject)

DiaDisplay *dia_display_new  (Diagram    *diagram);
DiaDisplay *dia_display_copy (DiaDisplay *self);

/* Normal destroy is done through shell widget destroy event. */
void dia_display_transform_coords_double(DiaDisplay *ddisp,
				      coord x, coord y,
				      double *xi, double *yi);
void dia_display_transform_coords(DiaDisplay *ddisp,
			       coord x, coord y,
			       int *xi, int *yi);
void dia_display_untransform_coords(DiaDisplay *ddisp,
				 int xi, int yi,
				 coord *x, coord *y);
real dia_display_transform_length(DiaDisplay *ddisp, real len);
real dia_display_untransform_length(DiaDisplay *ddisp, real len);
void dia_display_add_update_pixels(DiaDisplay *ddisp, Point *point,
				       int pixel_width, int pixel_height);
void dia_display_add_update_all(DiaDisplay *ddisp);
void dia_display_add_update_with_border(DiaDisplay *ddisp, const Rectangle *rect,
				     int pixel_border);
void dia_display_add_update(DiaDisplay *ddisp, const Rectangle *rect);
void dia_display_flush(DiaDisplay *ddisp);
void dia_display_update_scrollbars(DiaDisplay *ddisp);
void dia_display_set_origo(DiaDisplay *ddisp,
			coord x, coord y);
void dia_display_zoom(DiaDisplay *ddisp, Point *point,
		   real zoom_factor);
void dia_display_zoom_middle(DiaDisplay *ddisp, real magnify);

void dia_display_zoom_centered(DiaDisplay *ddisp, Point *point, real magnify);
void dia_display_set_snap_to_grid(DiaDisplay *ddisp, gboolean snap);
void dia_display_set_snap_to_objects(DiaDisplay *ddisp, gboolean magnetic);
void dia_display_set_renderer(DiaDisplay *ddisp, int aa_renderer);
void dia_display_resize_canvas(DiaDisplay *ddisp,
			    int width,
			    int height);

void dia_display_render_pixmap(DiaDisplay *ddisp, Rectangle *update);

DiaDisplay *dia_display_active(void);
Diagram *dia_display_active_diagram(void);

void dia_display_close(DiaDisplay *ddisp);

void dia_display_set_title(DiaDisplay *ddisp, char *title);
void dia_display_set_cursor(DiaDisplay *ddisp, GdkCursor *cursor);
void dia_display_set_all_cursor(GdkCursor *cursor);

void  dia_display_set_clicked_point(DiaDisplay *ddisp, int x, int y);
Point dia_display_get_clicked_position(DiaDisplay *ddisp);

gboolean display_get_rulers_showing(DiaDisplay *ddisp);
void display_rulers_show (DiaDisplay *ddisp);
void display_rulers_hide (DiaDisplay *ddisp);
void dia_display_update_rulers (DiaDisplay *ddisp, const Rectangle *extents, const Rectangle *visible);

gboolean dia_display_scroll(DiaDisplay *ddisp, Point *delta);
gboolean dia_display_autoscroll(DiaDisplay *ddisp, int x, int y);
void dia_display_scroll_up(DiaDisplay *ddisp);
void dia_display_scroll_down(DiaDisplay *ddisp);
void dia_display_scroll_left(DiaDisplay *ddisp);
void dia_display_scroll_right(DiaDisplay *ddisp);
gboolean dia_display_scroll_center_point(DiaDisplay *ddisp, Point *p);
gboolean dia_display_scroll_to_object(DiaDisplay *ddisp, DiaObject *obj);
gboolean dia_display_present_object(DiaDisplay *ddisp, DiaObject *obj);

void dia_display_show_all (DiaDisplay *ddisp);

void display_update_menu_state(DiaDisplay *ddisp);
void dia_display_update_statusbar(DiaDisplay *ddisp);
void dia_display_do_update_menu_sensitivity (DiaDisplay *ddisp);

void display_set_active(DiaDisplay *ddisp);

void dia_display_im_context_preedit_reset(DiaDisplay *ddisp, Focus *focus);

Focus *dia_display_active_focus(DiaDisplay *ddisp);
void dia_display_set_active_focus(DiaDisplay *ddisp, Focus *focus);

void diagram_add_display(Diagram *dia, DiaDisplay *ddisp);
void diagram_remove_display(Diagram *dia, DiaDisplay *ddisp);

G_END_DECLS

#endif /* DDISPLAY_H */

