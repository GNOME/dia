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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include <stdio.h>
#include <math.h>

#include <gdk/gdkkeysyms.h>

#include "display.h"
#include "disp_callbacks.h"
#include "interface.h"
#include "focus.h"
#include "color.h"
#include "object.h"
#include "handle_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "cut_n_paste.h"
#include "message.h"
#include "preferences.h"
#include "app_procs.h"
#include "layer-editor/layer_dialog.h"
#include "load_save.h"
#include "dia-diagram-properties-dialog.h"
#include "renderer/diacairo.h"
#include "diatransform.h"
#include "recent_files.h"
#include "filedlg.h"
#include "dia-layer.h"
#include "exit_dialog.h"


static GdkCursor *current_cursor = NULL;

GdkCursor *default_cursor = NULL;

static DDisplay *active_display = NULL;

static void
update_zoom_status (DDisplay *ddisp)
{
  gchar* zoom_text;

  if (is_integrated_ui ()) {
    zoom_text = g_strdup_printf ("%.0f%%",
                                 ddisp->zoom_factor * 100.0 / DDISPLAY_NORMAL_ZOOM);

    integrated_ui_toolbar_set_zoom_text (ddisp->common_toolbar, zoom_text);
  } else {
    GtkWidget *zoomcombo;
    zoom_text = g_strdup_printf ("%.1f%%",
                                 ddisp->zoom_factor * 100.0 / DDISPLAY_NORMAL_ZOOM);
    zoomcombo = ddisp->zoom_status;
    gtk_entry_set_text (GTK_ENTRY (g_object_get_data (G_OBJECT (zoomcombo), "user_data")),
                        zoom_text);
  }

  g_clear_pointer (&zoom_text, g_free); /* Copied by gtk_entry_set_text */
}


static void
selection_changed (Diagram *dia, int n, DDisplay *ddisp)
{
  GtkStatusbar *statusbar;
  guint context_id;

  /* nothing to do if there is no display with the diagram anymore */
  if (g_slist_length (dia->displays) < 1) {
    return;
  }

  statusbar = GTK_STATUSBAR (ddisp->modified_status);
  context_id = gtk_statusbar_get_context_id (statusbar, "Selection");

  if (n > 1) {
    /* http://www.gnu.org/software/gettext/manual/html_chapter/gettext_10.html#SEC150
     * Although the single objects wont get triggered here some languages have variations on the other numbers
     */
    char *msg = g_strdup_printf (ngettext ("Selection of %d object",
                                           "Selection of %d objects",
                                           n),
                                 n);

    gtk_statusbar_pop (statusbar, context_id);
    gtk_statusbar_push (statusbar, context_id, msg);

    g_clear_pointer (&msg, g_free);
  } else if (n == 1) {
    /* find the selected objects name - and display it */
    DiaObject *object = (DiaObject *) ddisp->diagram->data->selected->data;
    char *name = object_get_displayname (object);
    char *msg = g_strdup_printf (_("Selected '%s'"), _(name));

    gtk_statusbar_pop (statusbar, context_id);
    gtk_statusbar_push (statusbar, context_id, msg);

    g_clear_pointer (&name, g_free);
    g_clear_pointer (&msg, g_free);
  } else {
    gtk_statusbar_pop (statusbar, context_id);
  }

  /* selection-changed signal can also be emitted from outside of the dia core */
  ddisplay_do_update_menu_sensitivity (ddisp);
}


/**
 * initialize_display_widgets:
 * @ddisp: A #DDisplay with all non-GTK/GDK items set.
 *
 * Initialize the various GTK-level thinks in a display after the internal
 * data has been set.
 *
 * Since: dawn-of-time
 */
static void
initialize_display_widgets (DDisplay *ddisp)
{
  Diagram *dia = ddisp->diagram;
  char *filename;

  g_return_if_fail (dia && dia->filename);

  ddisp->im_context = gtk_im_multicontext_new ();
  g_signal_connect (G_OBJECT (ddisp->im_context), "commit",
                    G_CALLBACK (ddisplay_im_context_commit), ddisp);
  ddisp->preedit_string = NULL;
  g_signal_connect (G_OBJECT (ddisp->im_context), "preedit_changed",
                    G_CALLBACK (ddisplay_im_context_preedit_changed),
                    ddisp);
  ddisp->preedit_attrs = NULL;

  filename = strrchr (dia->filename, G_DIR_SEPARATOR);
  if (filename==NULL) {
    filename = dia->filename;
  } else {
    filename++;
  }

  create_display_shell (ddisp,
                        prefs.new_view.width,
                        prefs.new_view.height,
                        filename,
                        prefs.new_view.use_menu_bar);

  ddisplay_update_statusbar (ddisp);

  ddisplay_set_cursor (ddisp, current_cursor);
}


/**
 * copy_display:
 * @orig_ddisp: the #DDisplay to copy
 *
 * Make a copy of an existing display. The original does not need to have
 * the various GTK-related fields initialized, and so can just have been read
 * from a savefile.
 *
 * Returns: A newly allocated #DDisplay, inserted into the diagram list, with
 *          same basic layout as the original.
 *
 * Since: dawn-of-time
 */
DDisplay *
copy_display (DDisplay *orig_ddisp)
{
  DDisplay *ddisp;
  Diagram *dia = orig_ddisp->diagram;

  ddisp = g_new0 (DDisplay,1);

  ddisp->diagram = orig_ddisp->diagram;
  /* Every display has its own reference */
  g_object_ref (dia);

  ddisp->grid = orig_ddisp->grid;

  ddisp->show_cx_pts = orig_ddisp->show_cx_pts;


  ddisp->autoscroll = orig_ddisp->autoscroll;
  ddisp->mainpoint_magnetism = orig_ddisp->mainpoint_magnetism;

  ddisp->aa_renderer = orig_ddisp->aa_renderer;

  ddisp->update_areas = orig_ddisp->update_areas;

  ddisp->clicked_position.x = ddisp->clicked_position.y = 0.0;

  diagram_add_ddisplay(dia, ddisp);
  g_signal_connect (dia, "selection_changed", G_CALLBACK(selection_changed), ddisp);
  ddisp->origo = orig_ddisp->origo;
  ddisp->zoom_factor = orig_ddisp->zoom_factor;
  ddisp->visible = orig_ddisp->visible;

  initialize_display_widgets(ddisp);
  return ddisp;  /*  set the user data  */
}


/**
 * new_display:
 * @dia: #Diagram the #DDisplay is for
 *
 * Create a new display for a diagram, using prefs settings.
 *
 * Returns: A newly created display.
 *
 * Since: dawn-of-time
 */
DDisplay *
new_display(Diagram *dia)
{
  DDisplay *ddisp;
  DiaRectangle visible;
  int preset;

  ddisp = g_new0 (DDisplay,1);

  /* Every display has it's own reference */
  ddisp->diagram = g_object_ref (dia);

  ddisp->grid.visible = prefs.grid.visible;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "show-grid"));
  if (preset != 0)
    ddisp->grid.visible = (preset > 0 ? TRUE : FALSE);
  ddisp->grid.snap = prefs.grid.snap;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "snap-to-grid"));
  if (preset != 0)
    ddisp->grid.snap = (preset > 0 ? TRUE : FALSE);

  ddisp->guides_visible = prefs.guides_visible;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "show-guides"));
  if (preset != 0) {
    ddisp->guides_visible = (preset > 0 ? TRUE : FALSE);
  }
  ddisp->guides_snap = prefs.guides_snap;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "snap-to-guides"));
  if (preset != 0) {
    ddisp->guides_snap = (preset > 0 ? TRUE : FALSE);
  }

  ddisp->show_cx_pts = prefs.show_cx_pts;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "show-connection-points"));
  if (preset != 0)
    ddisp->show_cx_pts = (preset > 0 ? TRUE : FALSE);

  ddisp->autoscroll = TRUE;
  ddisp->mainpoint_magnetism = prefs.snap_object;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "snap-to-object"));
  if (preset != 0)
    ddisp->mainpoint_magnetism = (preset > 0 ? TRUE : FALSE);

  ddisp->aa_renderer = prefs.view_antialiased;
  preset = GPOINTER_TO_INT (g_object_get_data (G_OBJECT(dia), "antialiased"));
  if (preset != 0)
    ddisp->aa_renderer = (preset > 0 ? TRUE : FALSE);

  ddisp->update_areas = NULL;

  ddisp->clicked_position.x = ddisp->clicked_position.y = 0.0;

  diagram_add_ddisplay (dia, ddisp);
  g_signal_connect (dia, "selection_changed", G_CALLBACK(selection_changed), ddisp);
  ddisp->origo.x = 0.0;
  ddisp->origo.y = 0.0;
  ddisp->zoom_factor = prefs.new_view.zoom/100.0*DDISPLAY_NORMAL_ZOOM;
  if ((ddisp->diagram) && (ddisp->diagram->data)) {
    DiaRectangle *extents = &ddisp->diagram->data->extents;

    visible.left = extents->left;
    visible.top = extents->top;
  } else {
    visible.left = 0.0;
    visible.top = 0.0;
  }
  visible.right = visible.left + prefs.new_view.width/ddisp->zoom_factor;
  visible.bottom = visible.top + prefs.new_view.height/ddisp->zoom_factor;

  ddisp->visible = visible;

  ddisp->is_dragging_new_guideline = FALSE;
  ddisp->dragged_new_guideline_position = 0;
  ddisp->dragged_new_guideline_orientation = GTK_ORIENTATION_HORIZONTAL;

  initialize_display_widgets(ddisp);
  return ddisp;  /*  set the user data  */
}

void
ddisplay_transform_coords_double (DDisplay *ddisp,
                                  double    x,
                                  double    y,
                                  double   *xi,
                                  double   *yi)
{
  DiaRectangle *visible = &ddisp->visible;
  double width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  double height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  *xi = (x - visible->left)  * (double) width / (visible->right - visible->left);
  *yi = (y - visible->top)  * (double) height / (visible->bottom - visible->top);
}


void
ddisplay_transform_coords (DDisplay *ddisp,
                           double    x,
                           double    y,
                           int      *xi,
                           int      *yi)
{
  DiaRectangle *visible = &ddisp->visible;
  int width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  int height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  *xi = ROUND ( (x - visible->left) * (double) width /
                (visible->right - visible->left) );
  *yi = ROUND ( (y - visible->top) * (double) height /
                (visible->bottom - visible->top) );
}


/* Takes real length and returns pixel length */
double
ddisplay_transform_length (DDisplay *ddisp, double len)
{
  return len * ddisp->zoom_factor;
}


/* Takes pixel length and returns real length */
double
ddisplay_untransform_length (DDisplay *ddisp, double len)
{
  return len / ddisp->zoom_factor;
}


void
ddisplay_untransform_coords (DDisplay *ddisp,
                             int       xi,
                             int       yi,
                             double   *x,
                             double   *y)
{
  DiaRectangle *visible = &ddisp->visible;
  int width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  int height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  *x = visible->left + xi*(visible->right - visible->left) / (double) width;
  *y = visible->top +  yi*(visible->bottom - visible->top) / (double) height;
}


void
ddisplay_add_update_pixels (DDisplay *ddisp,
                            Point    *point,
                            int       pixel_width,
                            int       pixel_height)
{
  DiaRectangle rect;
  double size_x, size_y;

  size_x = ddisplay_untransform_length (ddisp, pixel_width+1);
  size_y = ddisplay_untransform_length (ddisp, pixel_height+1);

  rect.left = point->x - size_x/2.0;
  rect.top = point->y - size_y/2.0;
  rect.right = point->x + size_x/2.0;
  rect.bottom = point->y + size_y/2.0;

  ddisplay_add_update (ddisp, &rect);
}


/** Free update_areas list */
static void
ddisplay_free_update_areas (DDisplay *ddisp)
{
  g_slist_free_full (ddisp->update_areas, g_free);
  ddisp->update_areas = NULL;
}


/**
 * ddisplay_add_update_all:
 * @ddisp: the #DDisplay
 *
 * Marks the entire visible area for update.
 * Throws out old updates, since everything will be updated anyway.
 *
 * Since: dawn-of-time
 */
void
ddisplay_add_update_all (DDisplay *ddisp)
{
  if (ddisp->update_areas != NULL) {
    ddisplay_free_update_areas (ddisp);
  }

  ddisplay_add_update (ddisp, &ddisp->visible);
}


/**
 * ddisplay_add_update_with_border:
 * @ddisp: the #DDisplay
 * @rect: the area to update
 * @pixel_border: the border size around the area
 *
 * Marks a rectangle for update, with a pixel border around it.
 */
void
ddisplay_add_update_with_border (DDisplay           *ddisp,
                                 const DiaRectangle *rect,
                                 int                 pixel_border)
{
  DiaRectangle r;
  double size = ddisplay_untransform_length (ddisp, pixel_border+1);

  r.left = rect->left-size;
  r.top = rect->top-size;
  r.right = rect->right+size;
  r.bottom = rect->bottom+size;

  ddisplay_add_update (ddisp, &r);
}

void
ddisplay_add_update (DDisplay *ddisp, const DiaRectangle *rect)
{
  DiaRectangle *r;
  // int width, height;

  if (!ddisp->renderer)
    return; /* can happen at creation time of the diagram */
  // width = dia_renderer_get_width_pixels (ddisp->renderer);
  // height = dia_renderer_get_height_pixels (ddisp->renderer);

  if (!rectangle_intersects(rect, &ddisp->visible))
    return;

  /* Temporarily just do a union of all rectangles: */
  if (ddisp->update_areas==NULL) {
    r = g_new(DiaRectangle, 1);
    *r = *rect;
    rectangle_intersection(r, &ddisp->visible);
    ddisp->update_areas = g_slist_prepend(ddisp->update_areas, r);
  } else {
    r = (DiaRectangle *) ddisp->update_areas->data;
    rectangle_union(r, rect);
    rectangle_intersection(r, &ddisp->visible);
  }

  gtk_widget_queue_draw (ddisp->canvas);
}

void
ddisplay_flush(DDisplay *ddisp)
{
  /* if no update is queued, queue update
   *
   * GTK+ handles resize with higher priority than redraw
   * http://developer.gnome.org/gtk/2.24/gtk-General.html#GTK-PRIORITY-REDRAW:CAPS
   * GDK_PRIORITY_REDRAW = (G_PRIORITY_HIGH_IDLE + 20) with gtk-2-22
   * GTK_PRIORITY_RESIZE = (G_PRIORITY_HIGH_IDLE + 10)
   * Dia's canvas rendering is in between
   */
  gtk_widget_queue_draw (ddisp->canvas);
}

static void
ddisplay_obj_render (DiaObject   *obj,
                     DiaRenderer *renderer,
                     int          active_layer,
                     gpointer     data)
{
  DDisplay *ddisp = (DDisplay *) data;
  DiaHighlightType hltype = data_object_get_highlight (DIA_DIAGRAM_DATA (ddisp->diagram), obj);

  if (hltype != DIA_HIGHLIGHT_NONE) {
    dia_interactive_renderer_draw_object_highlighted (DIA_INTERACTIVE_RENDERER (renderer),
                                                      obj,
                                                      hltype);
  } else {
    /* maybe the renderer does not support highlighting */
    dia_renderer_draw_object (renderer, obj, NULL);
  }

  if (ddisp->show_cx_pts &&
      obj->parent_layer != NULL && dia_layer_is_connectable (obj->parent_layer)) {
    object_draw_connectionpoints (obj, ddisp);
  }
}

void
ddisplay_render_pixmap (DDisplay     *ddisp,
                        DiaRectangle *update)
{
  GList *list;
  DiaObject *obj;
  int i;
#ifdef TRACES
  GTimer *timer;
#endif

  if (ddisp->renderer==NULL) {
    g_critical ("ERROR! Renderer was NULL!!");
    return;
  }

  /* Erase background */
  dia_renderer_begin_render (ddisp->renderer, update);
  if (update) {
    int x0, y0, x1, y1;

    ddisplay_transform_coords (ddisp, update->left, update->top, &x0, &y0);
    ddisplay_transform_coords (ddisp, update->right, update->bottom, &x1, &y1);
    dia_interactive_renderer_fill_pixel_rect (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                              x0,
                                              y0,
                                              x1 - x0,
                                              y1 - y0,
                                              &ddisp->diagram->data->bg_color);
  } else {
    dia_interactive_renderer_fill_pixel_rect (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                              0,
                                              0,
                                              dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer)),
                                              dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer)),
                                              &ddisp->diagram->data->bg_color);
  }

  /* Draw grid */
  grid_draw (ddisp, update);
  pagebreak_draw (ddisp, update);
  guidelines_draw (ddisp, update);

#ifdef TRACES
  timer = g_timer_new();
#endif
  data_render (ddisp->diagram->data,
               ddisp->renderer, update,
               ddisplay_obj_render,
               (gpointer) ddisp);
#ifdef TRACES
  g_printerr ("data_render(%g%%) took %g seconds\n", ddisp->zoom_factor * 5.0, g_timer_elapsed (timer, NULL));
  g_timer_destroy (timer);
#endif
  /* Draw handles for all selected objects */
  list = ddisp->diagram->data->selected;
  while (list!=NULL) {
    obj = (DiaObject *) list->data;

    for (i=0;i<obj->num_handles;i++) {
      handle_draw(obj->handles[i], ddisp);
    }
    list = g_list_next(list);
  }
  dia_renderer_end_render (ddisp->renderer);
}

void
ddisplay_update_scrollbars(DDisplay *ddisp)
{
  DiaRectangle *extents = &ddisp->diagram->data->extents;
  DiaRectangle *visible = &ddisp->visible;
  GtkAdjustment *hsbdata, *vsbdata;

  hsbdata = ddisp->hsbdata;
  /* Horizontal: */
  gtk_adjustment_set_lower (hsbdata, MIN(extents->left, visible->left));
  gtk_adjustment_set_upper (hsbdata, MAX(extents->right, visible->right));
  gtk_adjustment_set_page_size (hsbdata, visible->right - visible->left - 0.0001);
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  gtk_adjustment_set_page_increment (hsbdata, (visible->right - visible->left) / 2.0);
  gtk_adjustment_set_step_increment (hsbdata, (visible->right - visible->left) / 10.0);
  gtk_adjustment_set_value (hsbdata, visible->left);

  g_signal_emit_by_name (G_OBJECT (ddisp->hsbdata), "changed");

  /* Vertical: */
  vsbdata = ddisp->vsbdata;
  gtk_adjustment_set_lower (vsbdata, MIN(extents->top, visible->top));
  gtk_adjustment_set_upper (vsbdata, MAX(extents->bottom, visible->bottom));
  gtk_adjustment_set_page_size (vsbdata, visible->bottom - visible->top - 0.00001);
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  gtk_adjustment_set_page_increment (vsbdata, (visible->bottom - visible->top) / 2.0);
  gtk_adjustment_set_step_increment (vsbdata, (visible->bottom - visible->top) / 10.0);
  gtk_adjustment_set_value (vsbdata, visible->top);

  g_signal_emit_by_name (G_OBJECT (ddisp->vsbdata), "changed");
}


void
ddisplay_set_origo (DDisplay *ddisp, double x, double y)
{
  DiaRectangle *extents = &ddisp->diagram->data->extents;
  DiaRectangle *visible = &ddisp->visible;
  int width, height;

  g_return_if_fail (ddisp->renderer != NULL);

  /*  updaterar origo+visible+rulers */
  ddisp->origo.x = x;
  ddisp->origo.y = y;

  if (ddisp->zoom_factor < DDISPLAY_MIN_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MIN_ZOOM;

  if (ddisp->zoom_factor > DDISPLAY_MAX_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MAX_ZOOM;

  width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  visible->left = ddisp->origo.x;
  visible->top = ddisp->origo.y;
  visible->right = ddisp->origo.x + ddisplay_untransform_length (ddisp, width);
  visible->bottom = ddisp->origo.y + ddisplay_untransform_length (ddisp, height);

  ddisplay_update_rulers (ddisp, extents, visible);
}


void
ddisplay_zoom (DDisplay *ddisp, Point *point, double magnify)
{
  DiaRectangle *visible;
  double width, height, old_zoom;

  visible = &ddisp->visible;

  if ((ddisp->zoom_factor <= DDISPLAY_MIN_ZOOM) && (magnify<=1.0)) {
    return;
  }

  if ((ddisp->zoom_factor >= DDISPLAY_MAX_ZOOM) && (magnify>=1.0)) {
    return;
  }

  old_zoom = ddisp->zoom_factor;
  ddisp->zoom_factor = old_zoom * magnify;

  /* clip once more */
  if (ddisp->zoom_factor < DDISPLAY_MIN_ZOOM) {
    ddisp->zoom_factor = DDISPLAY_MIN_ZOOM;
  } else if (ddisp->zoom_factor > DDISPLAY_MAX_ZOOM) {
    ddisp->zoom_factor = DDISPLAY_MAX_ZOOM;
  }

  /* the real one used - after clipping */
  magnify = ddisp->zoom_factor / old_zoom;
  width = (visible->right - visible->left)/magnify;
  height = (visible->bottom - visible->top)/magnify;


  ddisplay_set_origo (ddisp, point->x - width/2.0, point->y - height/2.0);

  ddisplay_update_scrollbars (ddisp);
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);

  update_zoom_status (ddisp);
}


/**
 * ddisplay_zoom_middle:
 * @ddisp: the #DDisplay
 * @magnify: the zoom level
 *
 * Zoom around the middle point of the visible area
 */
void
ddisplay_zoom_middle (DDisplay *ddisp, double magnify)
{
  Point middle;
  DiaRectangle *visible;

  visible = &ddisp->visible;
  middle.x = visible->left*0.5 + visible->right*0.5;
  middle.y = visible->top*0.5 + visible->bottom*0.5;

  ddisplay_zoom (ddisp, &middle, magnify);
}


/*
   When using the mouse wheel button to zoom in and out, it is more
   intuitive to maintain the drawing zoom center-point based on the
   cursor position. This can help orientation and prevent the drawing
   from "jumping" around while zooming in and out.
 */
void
ddisplay_zoom_centered (DDisplay *ddisp, Point *point, double magnify)
{
  DiaRectangle *visible;
  double width, height;
  /* cursor position ratios */
  double rx, ry;

  if ((ddisp->zoom_factor <= DDISPLAY_MIN_ZOOM) && (magnify <= 1.0)) {
    return;
  }

  if ((ddisp->zoom_factor >= DDISPLAY_MAX_ZOOM) && (magnify >= 1.0)) {
    return;
  }

  visible = &ddisp->visible;

  /* calculate cursor position ratios */
  rx = (point->x-visible->left)/(visible->right - visible->left);
  ry = (point->y-visible->top)/(visible->bottom - visible->top);

  width = (visible->right - visible->left)/magnify;
  height = (visible->bottom - visible->top)/magnify;

  ddisp->zoom_factor *= magnify;

  /* set new origin based on the calculated ratios before zooming */
  ddisplay_set_origo (ddisp, point->x-(width*rx),point->y-(height*ry));

  ddisplay_update_scrollbars (ddisp);
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);

  update_zoom_status (ddisp);
}


/**
 * ddisplay_set_snap_to_grid:
 * @ddisp: the #DDisplay
 * @snap: should snap to grid be enabled
 *
 * Set the display's snap-to-grid setting, updating menu and button
 * in the process
 */
void
ddisplay_set_snap_to_grid (DDisplay *ddisp, gboolean snap)
{
  GtkToggleAction *snap_to_grid;
  ddisp->grid.snap = snap;

  snap_to_grid = GTK_TOGGLE_ACTION (menus_get_action ("ViewSnaptogrid"));
  if (is_integrated_ui ()) {
    integrated_ui_toolbar_grid_snap_synchronize_to_display (ddisp);
  }
  /* Currently, this can cause double emit, but that's a small problem. */
  gtk_toggle_action_set_active (snap_to_grid, ddisp->grid.snap);
  ddisplay_update_statusbar (ddisp);
}


/* Update the button showing whether snap-to-grid is on */
static void
update_snap_grid_status (DDisplay *ddisp)
{
  if (ddisp->grid_status) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ddisp->grid_status),
                                  ddisp->grid.snap);
  }
}


/**
 * ddisplay_set_snap_to_objects:
 * @ddisp: the #DDisplay
 * @magnetic: should snap to objects
 *
 * Set the display's mainpoint magnetism setting, updating menu and button
 * in the process
 */
void
ddisplay_set_snap_to_objects (DDisplay *ddisp, gboolean magnetic)
{
  GtkToggleAction *mainpoint_magnetism;
  ddisp->mainpoint_magnetism = magnetic;

  mainpoint_magnetism = GTK_TOGGLE_ACTION (menus_get_action ("ViewSnaptoobjects"));
  if (is_integrated_ui ()) {
    integrated_ui_toolbar_object_snap_synchronize_to_display (ddisp);
  }
  /* Currently, this can cause double emit, but that's a small problem. */
  gtk_toggle_action_set_active (mainpoint_magnetism, ddisp->mainpoint_magnetism);
  ddisplay_update_statusbar (ddisp);
}


/* Update the button showing whether mainpoint magnetism is on */
static void
update_mainpoint_status (DDisplay *ddisp)
{
  if (ddisp->mainpoint_status) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ddisp->mainpoint_status),
                                  ddisp->mainpoint_magnetism);
  }
}


/** Scroll display to where point x,y (window coords) is visible */
gboolean
ddisplay_autoscroll (DDisplay *ddisp, int x, int y)
{
  guint16 width, height;
  Point scroll;
  GtkAllocation alloc;

  if (! ddisp->autoscroll) {
    return FALSE;
  }

  scroll.x = scroll.y = 0;

  gtk_widget_get_allocation (ddisp->canvas, &alloc);

  width = alloc.width;
  height = alloc.height;

  if (x < 0) {
    scroll.x = x;
  } else if (x > width) {
    scroll.x = x - width;
  }

  if (y < 0) {
    scroll.y = y;
  } else if (y > height) {
    scroll.y = y - height;
  }

  if ((scroll.x != 0) || (scroll.y != 0)) {
    gboolean scrolled;

    scroll.x = ddisplay_untransform_length (ddisp, scroll.x);
    scroll.y = ddisplay_untransform_length (ddisp, scroll.y);

    scrolled = ddisplay_scroll (ddisp, &scroll);

    if (scrolled) {
      ddisplay_flush (ddisp);
      return TRUE;
    }
  }
  return FALSE;
}


/**
 * ddisplay_scroll:
 * @ddisp: the #DDisplay
 * @delta: about to move by
 *
 * Scroll the display by delta (diagram coords)
 */
gboolean
ddisplay_scroll (DDisplay *ddisp, Point *delta)
{
  DiaRectangle *visible = &ddisp->visible;
  double width = visible->right - visible->left;
  double height = visible->bottom - visible->top;

  DiaRectangle extents = ddisp->diagram->data->extents;
  double ex_width = extents.right - extents.left;
  double ex_height = extents.bottom - extents.top;

  Point new_origo = ddisp->origo;
  point_add(&new_origo, delta);

  rectangle_union(&extents, visible);

  if (new_origo.x < extents.left - ex_width) {
    new_origo.x = extents.left - ex_width;
  }

  if (new_origo.x+width > extents.right + ex_width) {
    new_origo.x = extents.right - width + ex_width;
  }

  if (new_origo.y < extents.top - ex_height) {
    new_origo.y = extents.top - ex_height;
  }

  if (new_origo.y+height > extents.bottom + ex_height) {
    new_origo.y = extents.bottom - height + ex_height;
  }

  if ( (new_origo.x != ddisp->origo.x) ||
       (new_origo.y != ddisp->origo.y) ) {
    ddisplay_set_origo (ddisp, new_origo.x, new_origo.y);
    ddisplay_update_scrollbars (ddisp);
    ddisplay_add_update_all (ddisp);
    return TRUE;
  }

  return FALSE;
}


void
ddisplay_scroll_up (DDisplay *ddisp)
{
  Point delta;

  delta.x = 0;
  delta.y = -(ddisp->visible.bottom - ddisp->visible.top)/4.0;

  ddisplay_scroll (ddisp, &delta);
}


void
ddisplay_scroll_down (DDisplay *ddisp)
{
  Point delta;

  delta.x = 0;
  delta.y = (ddisp->visible.bottom - ddisp->visible.top)/4.0;

  ddisplay_scroll (ddisp, &delta);
}


void
ddisplay_scroll_left (DDisplay *ddisp)
{
  Point delta;

  delta.x = -(ddisp->visible.right - ddisp->visible.left)/4.0;
  delta.y = 0;

  ddisplay_scroll (ddisp, &delta);
}


void
ddisplay_scroll_right (DDisplay *ddisp)
{
  Point delta;

  delta.x = (ddisp->visible.right - ddisp->visible.left)/4.0;
  delta.y = 0;

  ddisplay_scroll (ddisp, &delta);
}


/**
 * ddisplay_scroll_center_point:
 * @ddisp: the #DDisplay
 * @p: the #Point
 *
 * Scroll display to have the diagram point p at the center.
 *
 * Returns: %TRUE if anything changed.
 */
gboolean
ddisplay_scroll_center_point (DDisplay *ddisp, Point *p)
{
  Point center;

  g_return_val_if_fail (ddisp != NULL, FALSE);
  /* Find current center */
  center.x = (ddisp->visible.right+ddisp->visible.left)/2;
  center.y = (ddisp->visible.top+ddisp->visible.bottom)/2;

  point_sub (p, &center);

  return ddisplay_scroll (ddisp, p);
}


/**
 * ddisplay_scroll_to_object:
 * @ddisp: the #DDisplay
 * @obj: the #DiaObject
 *
 * Scroll display so that obj is centered.
 *
 * Returns: %TRUE if anything changed.
 */
gboolean
ddisplay_scroll_to_object (DDisplay *ddisp, DiaObject *obj)
{
  DiaRectangle r = obj->bounding_box;

  Point p;
  p.x = (r.left+r.right)/2;
  p.y = (r.top+r.bottom)/2;

  display_set_active (ddisp);

  return ddisplay_scroll_center_point (ddisp, &p);
}


/**
 * ddisplay_present_object:
 * @ddisp: the #DDisplay
 * @obj: the #DiaObject
 *
 * Ensure the object is visible but minimize scrolling
 */
gboolean
ddisplay_present_object (DDisplay *ddisp, DiaObject *obj)
{
  const DiaRectangle *r = dia_object_get_enclosing_box (obj);
  const DiaRectangle *v = &ddisp->visible;

  display_set_active (ddisp);
  if (!rectangle_in_rectangle (v, r)) {
    Point delta = { 0, 0 };

    if ((r->right - r->left) > (v->right - v->left)) /* object bigger than visible area */
      delta.x = (r->left - v->left + r->right - v->right) / 2;
    else if (r->left < v->left)
      delta.x = r->left - v->left;
    else if  (r->right > v->right)
      delta.x = r->right - v->right;

    if ((r->bottom - r->top) > (v->bottom - v->top)) /* object bigger than visible area */
      delta.y = (r->top - v->top + r->bottom - v->bottom) / 2;
    else if (r->top < v->top)
      delta.y = r->top - v->top;
    else if  (r->bottom > v->bottom)
      delta.y = r->bottom - v->bottom;

    ddisplay_scroll (ddisp, &delta);

    return TRUE;
  }

  return FALSE;
}


/**
 * ddisplay_set_clicked_point:
 * @ddisp: the #DDisplay
 * @x: the x position
 * @y: the y position
 *
 * Remember the last clicked point given in pixel coordinates
 *
 * Since: dawn-of-time
 */
void
ddisplay_set_clicked_point (DDisplay *ddisp, int x, int y)
{
  Point pt;

  ddisplay_untransform_coords (ddisp, x, y, &pt.x, &pt.y);

  ddisp->clicked_position = pt;
}


/**
 * ddisplay_get_clicked_position:
 * @ddisp: the #DDisplay
 *
 * Get the last clicked point in diagram coordinates
 */
Point
ddisplay_get_clicked_position (DDisplay *ddisp)
{
  return ddisp->clicked_position;
}


void
ddisplay_set_renderer (DDisplay *ddisp, int aa_renderer)
{
  int width, height;
  GdkWindow *window = gtk_widget_get_window (ddisp->canvas);
  GtkAllocation alloc;

  g_clear_object (&ddisp->renderer);

  ddisp->aa_renderer = aa_renderer;

  gtk_widget_get_allocation (ddisp->canvas, &alloc);

  width = alloc.width;
  height = alloc.height;

  if (!ddisp->aa_renderer){
    g_message ("Only antialias renderers supported");
  }
  ddisp->renderer = dia_cairo_interactive_renderer_new ();
  g_object_set (ddisp->renderer,
                "zoom", &ddisp->zoom_factor,
                "rect", &ddisp->visible,
                NULL);

  if (window) {
    dia_interactive_renderer_set_size (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                       window,
                                       width,
                                       height);
  }
}


void
ddisplay_resize_canvas (DDisplay *ddisp,
                        int       width,
                        int       height)
{
  if (ddisp->renderer == NULL) {
    if (!ddisp->aa_renderer){
      g_message ("Only antialias renderers supported");
    }
    ddisp->renderer = dia_cairo_interactive_renderer_new ();
    g_object_set (ddisp->renderer,
                  "zoom", &ddisp->zoom_factor,
                  "rect", &ddisp->visible,
                  NULL);
  }

  dia_interactive_renderer_set_size (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                     gtk_widget_get_window (ddisp->canvas),
                                     width,
                                     height);

  ddisplay_set_origo (ddisp, ddisp->origo.x, ddisp->origo.y);

  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);
}


DDisplay *
ddisplay_active (void)
{
  return active_display;
}


Diagram *
ddisplay_active_diagram (void)
{
  DDisplay *ddisp = ddisplay_active ();

  if (!ddisp) return NULL;

  return ddisp->diagram;
}


static void
ddisp_destroy (DDisplay *ddisp)
{
  g_signal_handlers_disconnect_by_func (ddisp->diagram, selection_changed, ddisp);

  g_clear_object (&ddisp->im_context);

  ddisplay_im_context_preedit_reset (ddisp, get_active_focus ((DiagramData *) ddisp->diagram));

  if (GTK_WINDOW (ddisp->shell) == gtk_window_get_transient_for (GTK_WINDOW (interface_get_toolbox_shell ()))) {
    /* we have to break the connection otherwise the toolbox will be closed */
    gtk_window_set_transient_for (GTK_WINDOW (interface_get_toolbox_shell ()), NULL);
  }

  /* This calls ddisplay_really_destroy */
  if (ddisp->is_standalone_window) {
    gtk_widget_destroy (ddisp->shell);
  } else {
    gtk_widget_destroy (ddisp->container);
    ddisplay_really_destroy (ddisp);
  }
}


void
ddisplay_close (DDisplay *ddisp)
{
  Diagram *dia;
  DiaExitDialog *dialog;
  DiaExitDialogResult res;
  GPtrArray *items = NULL;
  char *fname;
  char *path;
  GFile *file;
  gboolean close_ddisp = TRUE;

  g_return_if_fail (ddisp != NULL);

  dia = ddisp->diagram;

  if ((g_slist_length (dia->displays) > 1) || (!diagram_is_modified (dia))) {
    ddisp_destroy (ddisp);
    return;
  }

  file = dia_diagram_get_file (dia);

  if (file) {
    fname = g_file_get_basename (file);
    path = g_file_get_path (file);
  } else {
    fname = g_strdup (_("Unsaved Diagram"));
    path = NULL;
  }

  dialog = dia_exit_dialog_new (GTK_WINDOW (ddisp->shell));
  dia_exit_dialog_add_item (dialog, fname, path, dia);

  res = dia_exit_dialog_run (dialog, &items);

  g_clear_object (&dialog);
  g_clear_pointer (&items, g_ptr_array_unref);

  g_clear_pointer (&fname, g_free);
  g_clear_pointer (&path, g_free);

  switch (res) {
    case DIA_EXIT_DIALOG_SAVE:
      /* save changes */
      if (ddisp->diagram->unsaved) {
        if (file_save_as (ddisp->diagram, ddisp)) {
          ddisp_destroy (ddisp);
        }
        /* no way back */
        return;
      } else {
        DiaContext *ctx = dia_context_new (_("Save"));
        if (!diagram_save (ddisp->diagram, ddisp->diagram->filename, ctx)) {
          close_ddisp = FALSE;
        }
        dia_context_release (ctx);
      }

      if (close_ddisp) {
        /* saving succeeded */
        recent_file_history_add(ddisp->diagram->filename);
      }

    case DIA_EXIT_DIALOG_QUIT:
      if (close_ddisp) {
        ddisp_destroy (ddisp);
      }
    case DIA_EXIT_DIALOG_CANCEL:
      break;
    default:
      g_return_if_reached ();
  }
}


void
display_update_menu_state(DDisplay *ddisp)
{
  GtkToggleAction *rulers;
  GtkToggleAction *visible_grid;
  GtkToggleAction *snap_to_grid;
  GtkToggleAction *visible_guides;
  GtkToggleAction *snap_to_guides;
  GtkToggleAction *show_cx_pts;
  GtkToggleAction *antialiased;
  gboolean scrollbars_shown;

  rulers         = GTK_TOGGLE_ACTION (menus_get_action ("ViewShowrulers"));
  visible_grid   = GTK_TOGGLE_ACTION (menus_get_action ("ViewShowgrid"));
  snap_to_grid   = GTK_TOGGLE_ACTION (menus_get_action ("ViewSnaptogrid"));
  visible_guides = GTK_TOGGLE_ACTION (menus_get_action ("ViewShowguides"));
  snap_to_guides = GTK_TOGGLE_ACTION (menus_get_action ("ViewSnaptoguides"));
  show_cx_pts    = GTK_TOGGLE_ACTION (menus_get_action ("ViewShowconnectionpoints"));
  antialiased    = GTK_TOGGLE_ACTION (menus_get_action ("ViewAntialiased"));

  gtk_action_set_sensitive (menus_get_action ("ViewAntialiased"),
                            g_type_from_name ("DiaCairoInteractiveRenderer") != 0);

  ddisplay_do_update_menu_sensitivity (ddisp);

  gtk_toggle_action_set_active (rulers, display_get_rulers_showing(ddisp));

  scrollbars_shown = gtk_widget_get_visible (ddisp->hsb);
  gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (menus_get_action ("ViewShowscrollbars")),
                                scrollbars_shown);

  gtk_toggle_action_set_active (visible_grid,
                                ddisp->grid.visible);
  gtk_toggle_action_set_active (snap_to_grid,
                                ddisp->grid.snap);
  gtk_toggle_action_set_active (visible_guides,
                                ddisp->guides_visible);
  gtk_toggle_action_set_active (snap_to_guides,
                                ddisp->guides_snap);
  gtk_toggle_action_set_active (show_cx_pts,
                                ddisp->show_cx_pts);

  gtk_toggle_action_set_active (antialiased,
                                ddisp->aa_renderer);
}

void
ddisplay_do_update_menu_sensitivity (DDisplay *ddisp)
{
    Diagram *dia;

    if (ddisp == NULL) {
      gtk_action_group_set_sensitive (menus_get_display_actions (), FALSE);
      return;
    }
    gtk_action_group_set_sensitive (menus_get_display_actions (), TRUE);
    dia = ddisp->diagram;
    diagram_update_menu_sensitivity (dia);
}


/* This is called when ddisp->shell is destroyed... */
void
ddisplay_really_destroy (DDisplay *ddisp)
{
  if (active_display == ddisp)
    display_set_active(NULL);

  if (ddisp->diagram) {
    diagram_remove_ddisplay (ddisp->diagram, ddisp);
    /* if we are the last user of the diagram it will be unref'ed */
    g_clear_object (&ddisp->diagram);
  }

  g_clear_object (&ddisp->renderer);

  /* Free update_areas list: */
  ddisplay_free_update_areas(ddisp);

  g_clear_pointer (&ddisp, g_free);
}


void
ddisplay_set_title (DDisplay *ddisp, char *title)
{
  if (ddisp->is_standalone_window) {
    gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
  } else {
    GtkNotebook *notebook = g_object_get_data (G_OBJECT (ddisp->shell),
                                               DIA_MAIN_NOTEBOOK);
    /* Find the page with ddisp then set the label on the tab */
    int num_pages = gtk_notebook_get_n_pages (notebook);
    int num;
    GtkWidget *page;
    for (num = 0 ; num < num_pages ; num++) {
      page = gtk_notebook_get_nth_page (notebook,num);
      if (g_object_get_data (G_OBJECT (page), "DDisplay") == ddisp) {
        GtkLabel *label = g_object_get_data (G_OBJECT (page), "tab-label");
        /* not using the passed in title here, because it may be too long */
        char *name = diagram_get_name (ddisp->diagram);

        gtk_label_set_text (label,name);

        g_clear_pointer (&name, g_free);

        break;
      }
    }
    /* now modify the application window title */
    {
      const char *pname = g_get_application_name ();
      char *fulltitle = g_strdup_printf ("%s — %s", title, pname);

      gtk_window_set_title (GTK_WINDOW (ddisp->shell), fulltitle);

      g_clear_pointer (&fulltitle, g_free);
    }
  }
}


void
ddisplay_set_all_cursor (GdkCursor *cursor)
{
  Diagram *dia;
  DDisplay *ddisp;
  GList *list;
  GSList *slist;

  current_cursor = cursor;

  list = dia_open_diagrams();
  while (list != NULL) {
    dia = (Diagram *) list->data;

    slist = dia->displays;
    while (slist != NULL) {
      ddisp = (DDisplay *) slist->data;

      ddisplay_set_cursor(ddisp, cursor);

      slist = g_slist_next(slist);
    }

    list = g_list_next(list);
  }
}

void
ddisplay_set_all_cursor_name (GdkDisplay  *disp,
                              const gchar *cursor_name)
{
  GdkDisplay *actual_disp;
  GdkCursor *cursor;

  if (disp) {
    actual_disp = disp;
  } else {
    actual_disp = gdk_display_get_default ();
  }

  cursor = gdk_cursor_new_from_name (actual_disp, cursor_name);

  ddisplay_set_all_cursor (cursor);
}

void
ddisplay_set_cursor(DDisplay *ddisp, GdkCursor *cursor)
{
  if (gtk_widget_get_window(ddisp->canvas))
    gdk_window_set_cursor(gtk_widget_get_window(ddisp->canvas), cursor);
}


/**
 * display_get_rulers_showing:
 * @ddisp: the #DDisplay
 *
 * Returns: whether the rulers are currently showing on the display.
 */
gboolean
display_get_rulers_showing (DDisplay *ddisp) {
  return ddisp->rulers_are_showing;
}


/**
 * display_rulers_show:
 * @ddisp: The #DDisplay to show the rulers on.
 *
 * Shows the rulers and sets flag ddisp->rulers_are_showing.  This
 * is needed to detect whether a show() has been issued.  There is a
 * delay between the time that gtk_widget_show() is called and the time
 * when GTK_WIDGET_IS_VISIBLE(w) will indicate true.
 *
 * Since: dawn-of-time
 */
void
display_rulers_show (DDisplay *ddisp)
{
  if (ddisp) {
    GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (ddisp->origin));

    gtk_widget_show (ddisp->origin);
    gtk_widget_show (ddisp->hrule);
    gtk_widget_show (ddisp->vrule);

    if (gtk_widget_get_visible (parent)) {
      gtk_widget_queue_resize (parent);
    }

    ddisp->rulers_are_showing = TRUE;
  }
}


/**
 * display_rulers_hide:
 * @ddisp: The #DDisplay to hide the rulers on.
 *
 * Hides the rulers and resets the flag ddisp->rulers_are_showing. This
 * is needed to detect whether a hide() has been issued. There is a
 * delay between the time that gtk_widget_hide() is called and the time
 * when GTK_WIDGET_IS_VISIBLE(w) will indicate false.
 *
 * Since: dawn-of-time
 */
void
display_rulers_hide (DDisplay *ddisp)
{
  if (ddisp) {
    GtkWidget *parent = gtk_widget_get_parent (GTK_WIDGET (ddisp->origin));

    gtk_widget_hide (ddisp->origin);
    gtk_widget_hide (ddisp->hrule);
    gtk_widget_hide (ddisp->vrule);

    if (gtk_widget_get_visible (parent)) {
      gtk_widget_queue_resize (parent);
    }

    ddisp->rulers_are_showing = FALSE;
  }
}


void
ddisplay_update_statusbar(DDisplay *ddisp)
{
  update_zoom_status (ddisp);
  update_snap_grid_status (ddisp);
  update_mainpoint_status (ddisp);
}

void
display_set_active(DDisplay *ddisp)
{
  if (ddisp != active_display) {
    active_display = ddisp;

    /* perform notification here (such as switch layers dialog) */
    layer_dialog_set_diagram (ddisp ? ddisp->diagram : NULL);
    dia_diagram_properties_dialog_set_diagram (dia_diagram_properties_dialog_get_default (),
                                               ddisp ? ddisp->diagram : NULL);

    if (ddisp) {
      if (ddisp->is_standalone_window)
      {
        display_update_menu_state(ddisp);

        if (prefs.toolbox_on_top) {
          gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
                                       GTK_WINDOW(ddisp->shell));
        } else {
          gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
                                       NULL);
        }
      } else {
        GtkNotebook *notebook = g_object_get_data (G_OBJECT (ddisp->shell),
                                                   DIA_MAIN_NOTEBOOK);
        /* Find the page with ddisp then set the label on the tab */
        int num_pages = gtk_notebook_get_n_pages (notebook);
        int num;
        GtkWidget *page;
        for (num = 0 ; num < num_pages ; num++)
        {
          page = gtk_notebook_get_nth_page (notebook,num);
          if (g_object_get_data (G_OBJECT (page), "DDisplay") == ddisp)
          {
            gtk_notebook_set_current_page (notebook,num);
            break;
          }
        }
        /* synchronize_ui_to_active_display (ddisp); */
        /* updates display title, etc */
        diagram_modified(ddisp->diagram);

        /* ZOOM */
        update_zoom_status (ddisp);

        /* Snap to grid */
        ddisplay_set_snap_to_grid (ddisp, ddisp->grid.snap); /* menus */

        /* Object snapping */
        ddisplay_set_snap_to_objects (ddisp, ddisp->mainpoint_magnetism);

        /* Snap to guides */
        ddisplay_set_snap_to_guides (ddisp, ddisp->guides_snap);

        display_update_menu_state (ddisp);

        gtk_window_present (GTK_WINDOW(ddisp->shell));
      }
    } else {
      /* TODO: Prevent gtk_window_set_transient_for() in Integrated UI case */
      gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
                                   NULL);
    }
  }
}


void
ddisplay_im_context_preedit_reset (DDisplay *ddisp, Focus *focus)
{
  if (ddisp->preedit_string != NULL) {
    if (focus != NULL) {
      DiaObjectChange *change;

      for (int i = 0; i < g_utf8_strlen (ddisp->preedit_string, -1); i++) {
        (focus->key_event) (focus, 0, GDK_KEY_BackSpace, NULL, 0, &change);
      }
    }

    g_clear_pointer (&ddisp->preedit_string, g_free);
  }

  g_clear_pointer (&ddisp->preedit_attrs, pango_attr_list_unref);
}


/**
 * ddisplay_active_focus:
 * @ddisp: #DDisplay to get active focus for. This display need not be the
 *         currently active display.
 *
 * Get the active focus for the given display, or %NULL.
 *
 * Returns: The focus that is active for the given display, or %NULL if no
 *          focus is active (i.e. no text is being edited).
 *
 * Since: dawn-of-time
 */
Focus *
ddisplay_active_focus (DDisplay *ddisp)
{
  /* The functions doing the transition rely on this being slightly
   * out of sync with get_active_focus(). But we would not need the
   * pointer, because the return value is only checked being !=NULL
   */
  return ddisp ? ddisp->active_focus : NULL;
}


/**
 * ddisplay_set_active_focus:
 * @ddisp: The display to set active focus for.
 * @focus: The focus that should be active for this display. May be %NULL,
 *         indicating that no text is currently being edited on this display.
 *
 * Set the currently active focus for this display. This field should be
 * set to non-%NULL when a text is being edited and to %NULL when no text
 * is being edited.  Only textedit.c really needs to call this function.
 *
 * Since: dawn-of-time
 */
void
ddisplay_set_active_focus (DDisplay *ddisp, Focus *focus)
{
  ddisp->active_focus = focus;
}


void
ddisplay_show_all (DDisplay *ddisp)
{
  Diagram *dia;
  double magnify_x, magnify_y;
  int width, height;
  Point middle;

  g_return_if_fail (ddisp != NULL);

  dia = ddisp->diagram;

  width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  /* if there is something selected show that instead of all existing objects */
  if (dia->data->selected) {
    GList *list = dia->data->selected;
    DiaRectangle extents = *dia_object_get_enclosing_box ((DiaObject*)list->data);
    list = g_list_next(list);
    while (list) {
      DiaObject *obj = (DiaObject *)list->data;
      rectangle_union(&extents, dia_object_get_enclosing_box (obj));
      list = g_list_next(list);
    }
    magnify_x = (double)width / (extents.right - extents.left) / ddisp->zoom_factor;
    magnify_y = (double)height / (extents.bottom - extents.top) / ddisp->zoom_factor;
    middle.x = extents.left + (extents.right - extents.left) / 2.0;
    middle.y = extents.top + (extents.bottom - extents.top) / 2.0;
  } else {
    magnify_x = (double)width /
      (dia->data->extents.right - dia->data->extents.left) / ddisp->zoom_factor;
    magnify_y = (double)height /
      (dia->data->extents.bottom - dia->data->extents.top) / ddisp->zoom_factor;

    middle.x = dia->data->extents.left +
      (dia->data->extents.right - dia->data->extents.left) / 2.0;
    middle.y = dia->data->extents.top +
      (dia->data->extents.bottom - dia->data->extents.top) / 2.0;
  }

  ddisplay_zoom (ddisp, &middle,
		 ((magnify_x<magnify_y)?magnify_x:magnify_y)/1.05);

  ddisplay_update_scrollbars(ddisp);
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
}


/** Set the display's snap-to-guides setting, updating menu and button
 * in the process */
void
ddisplay_set_snap_to_guides (DDisplay *ddisp, gboolean snap)
{
  GtkToggleAction *snap_to_guides;
  ddisp->guides_snap = snap;

  snap_to_guides = GTK_TOGGLE_ACTION (menus_get_action ("ViewSnaptoguides"));

  if (is_integrated_ui ())
    integrated_ui_toolbar_guides_snap_synchronize_to_display (ddisp);

  /* Currently, this can cause double emit, but that's a small problem. */
  gtk_toggle_action_set_active (snap_to_guides, ddisp->guides_snap);
  ddisplay_update_statusbar (ddisp);
}
