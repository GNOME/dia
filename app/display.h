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
#include "diarenderer.h"

G_BEGIN_DECLS

/*
 * Defines the pixels per cm, default is 20 pixels = 1 cm
 * This is close to, but not quite the same as, the physical display size
 * in most cases
 */

#define DDISPLAY_MAX_ZOOM 2000.0
#define DDISPLAY_NORMAL_ZOOM 20.0
#define DDISPLAY_MIN_ZOOM 0.2

/* The zoom amount should be uniform.  Pixels per cm should be defined by the
 * renderer alone.  But that'd take a lot of fiddling in renderers. */
/*
#define DDISPLAY_MAX_ZOOM 100.0
#define DDISPLAY_NORMAL_ZOOM 1.0
#define DDISPLAY_MIN_ZOOM 0.01
*/
struct _DDisplay {
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
  GtkWidget *guide_snap_status;
  GtkWidget *modified_status;

  GtkAccelGroup *accel_group;

  GtkAdjustment *hsbdata;         /* horizontal data information       */
  GtkAdjustment *vsbdata;         /* vertical data information         */

  Point origo;                    /* The origo (lower left) position   */
  double zoom_factor;             /* zoom, 20.0 means 20 pixel == 1 cm */
  DiaRectangle visible;           /* The part visible in this display  */

  Grid grid;                      /* the grid in this display          */

  gboolean guides_visible;        /* Whether guides are visible. */
  gboolean guides_snap;           /* Whether to snap to guides. */

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

  /* Remember the last clicked point per display, but in diagram coordinates */
  Point clicked_position;

  /* For dragging a new guideline. */
  gboolean is_dragging_new_guideline;
  gdouble dragged_new_guideline_position;
  GtkOrientation dragged_new_guideline_orientation;
};

extern GdkCursor *default_cursor;

DDisplay *new_display(Diagram *dia);
DDisplay *copy_display(DDisplay *orig_ddisp);
/* Normal destroy is done through shell widget destroy event. */
void     ddisplay_really_destroy          (DDisplay *ddisp);
void     ddisplay_transform_coords_double (DDisplay *ddisp,
                                           double    x,
                                           double    y,
                                           double   *xi,
                                           double   *yi);
void     ddisplay_transform_coords        (DDisplay *ddisp,
                                           double    x,
                                           double    y,
                                           int      *xi,
                                           int      *yi);
void     ddisplay_untransform_coords      (DDisplay *ddisp,
                                           int       xi,
                                           int       yi,
                                           double   *x,
                                           double   *y);
double   ddisplay_transform_length        (DDisplay *ddisp,
                                           double    len);
double   ddisplay_untransform_length      (DDisplay *ddisp,
                                           double    len);
void ddisplay_add_update_pixels(DDisplay *ddisp, Point *point,
				       int pixel_width, int pixel_height);
void ddisplay_add_update_all(DDisplay *ddisp);
void ddisplay_add_update_with_border(DDisplay *ddisp, const DiaRectangle *rect,
				     int pixel_border);
void ddisplay_add_update(DDisplay *ddisp, const DiaRectangle *rect);
void ddisplay_flush(DDisplay *ddisp);
void ddisplay_update_scrollbars(DDisplay *ddisp);
void     ddisplay_set_origo               (DDisplay *ddisp,
                                           double    x,
                                           double    y);
void     ddisplay_zoom                    (DDisplay *ddisp,
                                           Point    *point,
                                           double    zoom_factor);
void     ddisplay_zoom_middle             (DDisplay *ddisp,
                                           double    magnify);
void     ddisplay_zoom_centered           (DDisplay *ddisp,
                                           Point    *point,
                                           double    magnify);
void ddisplay_set_snap_to_grid(DDisplay *ddisp, gboolean snap);
void ddisplay_set_snap_to_guides(DDisplay *ddisp, gboolean snap);
void ddisplay_set_snap_to_objects(DDisplay *ddisp, gboolean magnetic);
void ddisplay_set_renderer(DDisplay *ddisp, int aa_renderer);
void ddisplay_resize_canvas(DDisplay *ddisp,
			    int width,
			    int height);

void ddisplay_render_pixmap(DDisplay *ddisp, DiaRectangle *update);

DDisplay *ddisplay_active(void);
Diagram *ddisplay_active_diagram(void);

void ddisplay_close(DDisplay *ddisp);

void ddisplay_set_title(DDisplay *ddisp, char *title);
void ddisplay_set_cursor(DDisplay *ddisp, GdkCursor *cursor);
void ddisplay_set_all_cursor(GdkCursor *cursor);
void ddisplay_set_all_cursor_name (GdkDisplay  *disp,
                                   const gchar *cursor);
void  ddisplay_set_clicked_point(DDisplay *ddisp, int x, int y);
Point ddisplay_get_clicked_position(DDisplay *ddisp);

gboolean display_get_rulers_showing(DDisplay *ddisp);
void display_rulers_show (DDisplay *ddisp);
void display_rulers_hide (DDisplay *ddisp);
void ddisplay_update_rulers (DDisplay *ddisp, const DiaRectangle *extents, const DiaRectangle *visible);

gboolean ddisplay_scroll(DDisplay *ddisp, Point *delta);
gboolean ddisplay_autoscroll(DDisplay *ddisp, int x, int y);
void ddisplay_scroll_up(DDisplay *ddisp);
void ddisplay_scroll_down(DDisplay *ddisp);
void ddisplay_scroll_left(DDisplay *ddisp);
void ddisplay_scroll_right(DDisplay *ddisp);
gboolean ddisplay_scroll_center_point(DDisplay *ddisp, Point *p);
gboolean ddisplay_scroll_to_object(DDisplay *ddisp, DiaObject *obj);
gboolean ddisplay_present_object(DDisplay *ddisp, DiaObject *obj);

void ddisplay_show_all (DDisplay *ddisp);

void display_update_menu_state(DDisplay *ddisp);
void ddisplay_update_statusbar(DDisplay *ddisp);
void ddisplay_do_update_menu_sensitivity (DDisplay *ddisp);

void display_set_active(DDisplay *ddisp);

void ddisplay_im_context_preedit_reset(DDisplay *ddisp, Focus *focus);

Focus *ddisplay_active_focus(DDisplay *ddisp);
void ddisplay_set_active_focus(DDisplay *ddisp, Focus *focus);

void diagram_add_ddisplay(Diagram *dia, DDisplay *ddisp);
void diagram_remove_ddisplay(Diagram *dia, DDisplay *ddisp);

G_END_DECLS

#endif /* DDISPLAY_H */

