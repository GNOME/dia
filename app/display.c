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

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>

#ifdef GNOME
#include <gnome.h>
#endif

#include "intl.h"

#include "display.h"
#include "group.h"
#include "interface.h"
#include "focus.h"
#include "color.h"
#include "handle_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "cut_n_paste.h"
#include "message.h"
#include "preferences.h"
#include "app_procs.h"
#include "layer_dialog.h"
#include "load_save.h"
#include "dia-props.h"
#include "render_gdk.h"
#include "render_libart.h"

#include "pixmaps/on-grid.xpm"
#include "pixmaps/off-grid.xpm"

static GHashTable *display_ht = NULL;
static GdkCursor *current_cursor = NULL;

GdkCursor *default_cursor = NULL;

static DDisplay *active_display = NULL;


typedef struct _IRectangle {
  int top, bottom;
  int left,right;
} IRectangle;

static guint display_hash(DDisplay *ddisp);

static void
update_zoom_status(DDisplay *ddisp)
{
  GtkWidget *zoomcombo;
  gchar zoom_text[7];

  zoomcombo = ddisp->zoom_status;
  sprintf (zoom_text, "%.1f%%",
	   ddisp->zoom_factor * 100.0 / DDISPLAY_NORMAL_ZOOM);
  gtk_entry_set_text(GTK_ENTRY(gtk_object_get_user_data(GTK_OBJECT(zoomcombo))),
		     zoom_text);
}

static void
update_modified_status(DDisplay *ddisp)
{
  GtkStatusbar *statusbar;
  guint context_id;

  if (diagram_is_modified(ddisp->diagram))
  {
    statusbar = GTK_STATUSBAR (ddisp->modified_status);
    context_id = gtk_statusbar_get_context_id (statusbar, "Changed");
  
    gtk_statusbar_pop (statusbar, context_id); 
    gtk_statusbar_push (statusbar, context_id, _("Diagram modified!"));
  }
  else
  {
    statusbar = GTK_STATUSBAR (ddisp->modified_status);
    context_id = gtk_statusbar_get_context_id (statusbar, "Changed");
  
    gtk_statusbar_pop (statusbar, context_id); 
  }
}

void
selection_changed (Diagram* dia, int n, DDisplay* ddisp)
{
  g_print ("Diagram 0x%08x  Display 0x%08x Selections %d\n", dia, ddisp, n);
}

DDisplay *
new_display(Diagram *dia)
{
  DDisplay *ddisp;
  char *filename;
  int embedded = app_is_embedded();
  Rectangle visible;
  GtkMenuItem* im_menu_item;
  GtkWidget* im_menu;
  GtkWidget* im_menu_tearoff;
  static gboolean input_methods_done = FALSE;
  
  ddisp = g_new0(DDisplay,1);

  ddisp->menu_bar = NULL;
  ddisp->mbar_item_factory = NULL;

  ddisp->rulers = NULL;
  ddisp->visible_grid = NULL;
  ddisp->snap_to_grid = NULL;
  ddisp->show_cx_pts_mitem = NULL;
#ifdef HAVE_LIBART
  ddisp->antialiased = NULL;
#endif
  /* initialize the whole struct to 0 so that we are sure to catch errors.*/
  memset (&ddisp->updatable_menu_items, 0, sizeof (UpdatableMenuItems));
  
  ddisp->diagram = dia;
  if (dia->display_count > 1)
    /* The first display gets the initial ref */
    g_object_ref(dia);

  ddisp->grid.visible = prefs.grid.visible;
  ddisp->grid.snap = prefs.grid.snap;

  ddisp->show_cx_pts = prefs.show_cx_pts;

  ddisp->autoscroll = TRUE;

  ddisp->aa_renderer = 0;
  ddisp->renderer = new_gdk_renderer(ddisp);
  
  ddisp->update_areas = NULL;
  ddisp->display_areas = NULL;
  ddisp->update_id = 0;

  filename = strrchr(dia->filename, G_DIR_SEPARATOR);
  if (filename==NULL) {
    filename = dia->filename;
  } else {
    filename++;
  }

  diagram_add_ddisplay(dia, ddisp);
  g_signal_connect (dia, "selection_changed", selection_changed, ddisp);
  ddisp->origo.x = 0.0;
  ddisp->origo.y = 0.0;
  ddisp->zoom_factor = prefs.new_view.zoom/100.0*DDISPLAY_NORMAL_ZOOM;
  if ((ddisp->diagram) && (ddisp->diagram->data)) {
    Rectangle *extents = &ddisp->diagram->data->extents;

    visible.left = extents->left;
    visible.top = extents->top;
  } else {
    visible.left = 0.0;
    visible.top = 0.0;
  }
  visible.right = visible.left + prefs.new_view.width/ddisp->zoom_factor;
  visible.bottom = visible.top + prefs.new_view.height/ddisp->zoom_factor;

  ddisp->visible = visible;

  ddisp->im_context = gtk_im_multicontext_new();
  g_signal_connect (G_OBJECT (ddisp->im_context), "commit",
                    G_CALLBACK (ddisplay_im_context_commit), ddisp);
  ddisp->preedit_string = NULL;
  g_signal_connect (G_OBJECT (ddisp->im_context), "preedit_changed",
                    G_CALLBACK (ddisplay_im_context_preedit_changed),
                    ddisp);
  ddisp->preedit_attrs = NULL;
  
  create_display_shell(ddisp, prefs.new_view.width, prefs.new_view.height,
		       filename, prefs.new_view.use_menu_bar, !embedded);
  
  ddisplay_update_statusbar (ddisp);

  ddisplay_set_origo(ddisp, visible.left, visible.top);
  ddisp->visible = visible; /* force the visible area extents */
  ddisplay_update_scrollbars(ddisp);
  ddisplay_add_update_all(ddisp);

  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) display_hash, NULL);

  if (!embedded)
    ddisplay_set_cursor(ddisp, current_cursor);
  
  g_hash_table_insert (display_ht, ddisp->shell, ddisp);
  g_hash_table_insert (display_ht, ddisp->canvas, ddisp);


  if (!input_methods_done) {
      im_menu_item = menus_get_item_from_path("<Display>/Input Methods", NULL);
      if (im_menu_item) {          
          im_menu = gtk_menu_new();
          im_menu_tearoff = gtk_tearoff_menu_item_new();
          gtk_menu_shell_append(GTK_MENU_SHELL(im_menu),im_menu_tearoff);
          gtk_im_multicontext_append_menuitems(
              GTK_IM_MULTICONTEXT(ddisp->im_context),
              GTK_MENU_SHELL(im_menu));
          
          gtk_menu_item_set_submenu( GTK_MENU_ITEM(im_menu_item), im_menu);
          input_methods_done = TRUE;
      }
  }

  return ddisp;  /*  set the user data  */
}

static guint
display_hash(DDisplay *ddisp)
{
  return (gulong) ddisp;
}

void
ddisplay_transform_coords_double(DDisplay *ddisp,
				 coord x, coord y,
				 double *xi, double *yi)
{
  Rectangle *visible = &ddisp->visible;
  double width = dia_renderer_get_width_pixels (ddisp->renderer);
  double height = dia_renderer_get_height_pixels (ddisp->renderer);
  
  *xi = (x - visible->left)  * (real)width / (visible->right - visible->left);
  *yi = (y - visible->top)  * (real)height / (visible->bottom - visible->top);
}


void
ddisplay_transform_coords(DDisplay *ddisp,
			  coord x, coord y,
			  int *xi, int *yi)
{
  Rectangle *visible = &ddisp->visible;
  int width = dia_renderer_get_width_pixels (ddisp->renderer);
  int height = dia_renderer_get_height_pixels (ddisp->renderer);
  
  *xi = ROUND ( (x - visible->left)  * (real)width /
		(visible->right - visible->left) );
  *yi = ROUND ( (y - visible->top)  * (real)height /
		(visible->bottom - visible->top) );
}

/* Takes real length and returns pixel length */
real
ddisplay_transform_length(DDisplay *ddisp, real len)
{
  return len * ddisp->zoom_factor;
}

/* Takes pixel length and returns real length */
real
ddisplay_untransform_length(DDisplay *ddisp, real len)
{
  return len / ddisp->zoom_factor;
}


void
ddisplay_untransform_coords(DDisplay *ddisp,
			    int xi, int yi,
			    coord *x, coord *y)
{
  Rectangle *visible = &ddisp->visible;
  int width = dia_renderer_get_width_pixels (ddisp->renderer);
  int height = dia_renderer_get_height_pixels (ddisp->renderer);
  
  *x = visible->left + xi*(visible->right - visible->left) / (real)width;
  *y = visible->top +  yi*(visible->bottom - visible->top) / (real)height;
}


void
ddisplay_add_update_pixels(DDisplay *ddisp, Point *point,
			  int pixel_width, int pixel_height)
{
  Rectangle rect;
  real size_x, size_y;

  size_x = ddisplay_untransform_length(ddisp, pixel_width+1);
  size_y = ddisplay_untransform_length(ddisp, pixel_height+1);

  rect.left = point->x - size_x/2.0;
  rect.top = point->y - size_y/2.0;
  rect.right = point->x + size_x/2.0;
  rect.bottom = point->y + size_y/2.0;

  ddisplay_add_update(ddisp, &rect);
}

/** Free display_areas list */
static void
ddisplay_free_display_areas(DDisplay *ddisp)
{
  GSList *l;
  l = ddisp->display_areas;
  while(l!=NULL) {
    g_free(l->data);
    l = g_slist_next(l);
  }
  g_slist_free(ddisp->display_areas);
  ddisp->display_areas = NULL;
}

/** Free update_areas list */
static void
ddisplay_free_update_areas(DDisplay *ddisp)
{
  GSList *l;
  l = ddisp->update_areas;
  while(l!=NULL) {
    g_free(l->data);
    l = g_slist_next(l);
  }
  g_slist_free(ddisp->update_areas);
  ddisp->update_areas = NULL;
}

/** Marks the entire visible area for update.  
 * Throws out old updates, since everything will be updated anyway.
 */
void
ddisplay_add_update_all(DDisplay *ddisp)
{
  if (ddisp->update_areas != NULL) {
    ddisplay_free_update_areas(ddisp);
  }
  if (ddisp->display_areas != NULL) {
    ddisplay_free_display_areas(ddisp);
  }
  ddisplay_add_update(ddisp, &ddisp->visible);
}

/** Marks a rectangle for update, with a pixel border around it.
 */
void
ddisplay_add_update_with_border(DDisplay *ddisp, Rectangle *rect,
				int pixel_border)
{
  Rectangle r;
  real size = ddisplay_untransform_length(ddisp, pixel_border+1);

  r.left = rect->left-size;
  r.top = rect->top-size;
  r.right = rect->right+size;
  r.bottom = rect->bottom+size;

  ddisplay_add_update(ddisp, &r);
}

void
ddisplay_add_update(DDisplay *ddisp, Rectangle *rect)
{
  Rectangle *r;
  int top,bottom,left,right;
  Rectangle *visible;
  int width = dia_renderer_get_width_pixels (ddisp->renderer);
  int height = dia_renderer_get_height_pixels (ddisp->renderer);

  if (!rectangle_intersects(rect, &ddisp->visible))
    return;

  /* Temporarily just do a union of all rectangles: */
  if (ddisp->update_areas==NULL) {
    r = g_new(Rectangle,1);
    *r = *rect;
    rectangle_intersection(r, &ddisp->visible);
    ddisp->update_areas = g_slist_prepend(ddisp->update_areas, r);
  } else {
    r = (Rectangle *) ddisp->update_areas->data;
    rectangle_union(r, rect);
    rectangle_intersection(r, &ddisp->visible);
  }
  
  visible = &ddisp->visible;
  left = floor( (r->left - visible->left)  * (real)width /
		(visible->right - visible->left) ) - 1;
  top = floor( (r->top - visible->top)  * (real)height /
	       (visible->bottom - visible->top) ) - 1; 
  right = ceil( (r->right - visible->left)  * (real)width /
		(visible->right - visible->left) ) + 1;
  bottom = ceil( (r->bottom - visible->top)  * (real)height /
		 (visible->bottom - visible->top) ) + 1;

  ddisplay_add_display_area(ddisp,
			    left, top, 
			    right, bottom);
}

void
ddisplay_add_display_area(DDisplay *ddisp,
			  int left, int top,
			  int right, int bottom)
{
  IRectangle *r;

  if (left < 0)
    left = 0;
  if (top < 0)
    top = 0;
  if (right > dia_renderer_get_width_pixels (ddisp->renderer))
    right = dia_renderer_get_width_pixels (ddisp->renderer); 
  if (bottom > dia_renderer_get_height_pixels (ddisp->renderer))
    bottom = dia_renderer_get_height_pixels (ddisp->renderer); 
  
  /* draw some rectangles to show where updates are...*/
  /*  gdk_draw_rectangle(ddisp->canvas->window, ddisp->canvas->style->black_gc, TRUE, left, top, right-left,bottom-top); */

  /* Temporarily just do a union of all Irectangles: */
  if (ddisp->display_areas==NULL) {
    r = g_new(IRectangle,1);
    r->top = top; r->bottom = bottom;
    r->left = left; r->right = right;
    ddisp->display_areas = g_slist_prepend(ddisp->display_areas, r);
  } else {
    r = (IRectangle *) ddisp->display_areas->data;
  
    r->top = MIN( r->top, top );
    r->bottom = MAX( r->bottom, bottom );
    r->left = MIN( r->left, left );
    r->right = MAX( r->right, right );
  }
}

static gboolean
ddisplay_update_handler(DDisplay *ddisp)
{
  GSList *l;
  IRectangle *ir;
  Rectangle *r, totrect;
  DiaInteractiveRendererInterface *renderer;

  /* Renders updates to pixmap + copies display_areas to canvas(screen) */
  renderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (ddisp->renderer);

  /* Only update if update_areas exist */
  l = ddisp->update_areas;
  if (l != NULL)
  {
    totrect = *(Rectangle *) l->data;
  
    g_return_val_if_fail (   renderer->clip_region_clear != NULL
                          && renderer->clip_region_add_rect != NULL, FALSE);

    renderer->clip_region_clear (ddisp->renderer);

    while(l!=NULL) {
      r = (Rectangle *) l->data;

      rectangle_union(&totrect, r);
      renderer->clip_region_add_rect (ddisp->renderer, r);
      
      l = g_slist_next(l);
    }
    /* Free update_areas list: */
    ddisplay_free_update_areas(ddisp);

    totrect.left -= 0.1;
    totrect.right += 0.1;
    totrect.top -= 0.1;
    totrect.bottom += 0.1;
    
    ddisplay_render_pixmap(ddisp, &totrect);
  }

  l = ddisp->display_areas;
  while(l!=NULL) {
    ir = (IRectangle *) l->data;

    g_return_val_if_fail (renderer->copy_to_window, FALSE);
    renderer->copy_to_window(ddisp->renderer, 
                             ddisp->canvas->window,
                             ir->left, ir->top,
                             ir->right - ir->left, ir->bottom - ir->top);
    
    l = g_slist_next(l);
  }

  ddisplay_free_display_areas(ddisp);

  ddisp->update_id = 0;

  return FALSE;
}

void
ddisplay_flush(DDisplay *ddisp)
{
  /* if no update is queued, queue update */
  if (!ddisp->update_id)
    ddisp->update_id = gtk_idle_add((GtkFunction) ddisplay_update_handler,
				    ddisp);
}

static void
ddisplay_obj_render(DiaObject *obj, DiaRenderer *renderer,
		    int active_layer,
		    gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;
  int i;

  DIA_RENDERER_GET_CLASS(renderer)->draw_object(renderer, obj);
  if (ddisp->show_cx_pts && 
      obj->parent_layer != NULL && obj->parent_layer->connectable) {
    for (i=0;i<obj->num_connections;i++) {
      connectionpoint_draw(obj->connections[i], ddisp);
    }
  }
}

void
ddisplay_render_pixmap(DDisplay *ddisp, Rectangle *update)
{
  GList *list;
  DiaObject *obj;
  int i;
  DiaInteractiveRendererInterface *renderer;
  
  if (ddisp->renderer==NULL) {
    printf("ERROR! Renderer was NULL!!\n");
    return;
  }

  renderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (ddisp->renderer);

  /* Erase background */
  g_return_if_fail (renderer->fill_pixel_rect != NULL);
  renderer->fill_pixel_rect (ddisp->renderer,
			     0, 0,
		             dia_renderer_get_width_pixels (ddisp->renderer),
			     dia_renderer_get_height_pixels (ddisp->renderer),
			     &ddisp->diagram->data->bg_color);

  /* Draw grid */
  grid_draw(ddisp, update);
  pagebreak_draw(ddisp, update);

  data_render(ddisp->diagram->data, ddisp->renderer, update,
	      ddisplay_obj_render, (gpointer) ddisp);
  
  /* Draw handles for all selected objects */
  list = ddisp->diagram->data->selected;
  while (list!=NULL) {
    obj = (DiaObject *) list->data;

    for (i=0;i<obj->num_handles;i++) {
       handle_draw(obj->handles[i], ddisp);
     }
    list = g_list_next(list);
  }
}

void
ddisplay_update_scrollbars(DDisplay *ddisp)
{
  Rectangle *extents = &ddisp->diagram->data->extents;
  Rectangle *visible = &ddisp->visible;
  GtkAdjustment *hsbdata, *vsbdata;

  hsbdata = ddisp->hsbdata;
  /* Horizontal: */
  hsbdata->lower = MIN(extents->left, visible->left);
  hsbdata->upper = MAX(extents->right, visible->right);
  hsbdata->page_size = visible->right - visible->left - 0.0001;
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  hsbdata->page_increment = (visible->right - visible->left) / 2.0;
  hsbdata->step_increment = (visible->right - visible->left) / 10.0;
  hsbdata->value = visible->left;

  g_signal_emit_by_name (G_OBJECT (ddisp->hsbdata), "changed");

  /* Vertical: */
  vsbdata = ddisp->vsbdata;
  vsbdata->lower = MIN(extents->top, visible->top);
  vsbdata->upper = MAX(extents->bottom, visible->bottom);
  vsbdata->page_size = visible->bottom - visible->top - 0.00001;
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  vsbdata->page_increment = (visible->bottom - visible->top) / 2.0;
  vsbdata->step_increment = (visible->bottom - visible->top) / 10.0;
  vsbdata->value = visible->top;

  g_signal_emit_by_name (G_OBJECT (ddisp->vsbdata), "changed");
}

void
ddisplay_set_origo(DDisplay *ddisp, coord x, coord y)
{
  Rectangle *extents = &ddisp->diagram->data->extents;
  Rectangle *visible = &ddisp->visible;
  int width, height;

  /*  updaterar origo+visible+rulers */
  ddisp->origo.x = x;
  ddisp->origo.y = y;

  if (ddisp->zoom_factor<DDISPLAY_MIN_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MIN_ZOOM;
  
  if (ddisp->zoom_factor > DDISPLAY_MAX_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MAX_ZOOM;

  width = dia_renderer_get_width_pixels (ddisp->renderer);
  height = dia_renderer_get_height_pixels (ddisp->renderer);
  
  visible->left = ddisp->origo.x;
  visible->top = ddisp->origo.y;
  visible->right = ddisp->origo.x + width/ddisp->zoom_factor;
  visible->bottom = ddisp->origo.y + height/ddisp->zoom_factor;

  gtk_ruler_set_range  (GTK_RULER (ddisp->hrule),
			visible->left,
			visible->right,
			0.0f /* position*/,
			MAX(extents->right, visible->right)/* max_size*/);
  gtk_ruler_set_range  (GTK_RULER (ddisp->vrule),
			visible->top,
			visible->bottom,
			0.0f /*        position*/,
			MAX(extents->bottom, visible->bottom)/* max_size*/);
}

void
ddisplay_zoom(DDisplay *ddisp, Point *point, real magnify)
{
  Rectangle *visible;
  real width, height;

  visible = &ddisp->visible;

  width = (visible->right - visible->left)/magnify;
  height = (visible->bottom - visible->top)/magnify;

  if ((ddisp->zoom_factor <= DDISPLAY_MIN_ZOOM) && (magnify<=1.0))
    return;
  if ((ddisp->zoom_factor >= DDISPLAY_MAX_ZOOM) && (magnify>=1.0))
    return;

  ddisp->zoom_factor *= magnify;

  ddisplay_set_origo(ddisp, point->x - width/2.0, point->y - height/2.0);
  
  ddisplay_update_scrollbars(ddisp);
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);

  update_zoom_status (ddisp);
}

/** Set the display's snap-to-grid setting, updating menu and button
 * in the process */
void
ddisplay_set_snap_to_grid(DDisplay *ddisp, gboolean snap)
{
  GtkCheckMenuItem *snap_to_grid;
  ddisp->grid.snap = snap;

  if (ddisp->menu_bar == NULL) {
    snap_to_grid = GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/Snap To Grid", NULL));
  } else {
    snap_to_grid = ddisp->snap_to_grid;
  }

  /* Currently, this can cause double emit, but that's a small problem.
   */
  gtk_check_menu_item_set_active(snap_to_grid,
				 ddisp->grid.snap);
  ddisplay_update_statusbar(ddisp);
}

/** Update the button showing whether snap-to-grid is on */
static void
update_snap_grid_status(DDisplay *ddisp)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(ddisp->grid_status),
			       ddisp->grid.snap);
}

/** Scroll display to where point x,y (window coords) is visible */
gboolean
ddisplay_autoscroll(DDisplay *ddisp, int x, int y)
{
  guint16 width, height;
  Point scroll;
  
  if (! ddisp->autoscroll)
    return FALSE;

  scroll.x = scroll.y = 0;

  width = GTK_WIDGET(ddisp->canvas)->allocation.width;
  height = GTK_WIDGET(ddisp->canvas)->allocation.height;

  if (x < 0)
  {
    scroll.x = x;
  }
  else if ( x > width)
  {
    scroll.x = x - width;
  }

  if (y < 0)
  {
    scroll.y = y;
  }
  else if (y > height)
  {
    scroll.y = y - height;
  }

  if ((scroll.x != 0) || (scroll.y != 0))
  {
    gboolean scrolled;

    scroll.x = ddisplay_untransform_length(ddisp, scroll.x);
    scroll.y = ddisplay_untransform_length(ddisp, scroll.y);

    scrolled = ddisplay_scroll(ddisp, &scroll);

    if (scrolled) {
      ddisplay_flush(ddisp);        
      return TRUE;
    }
  }
  return FALSE;
}

/** Scroll the display by delta (diagram coords) */
gboolean 
ddisplay_scroll(DDisplay *ddisp, Point *delta)
{
  Rectangle *visible = &ddisp->visible;
  real width = visible->right - visible->left;
  real height = visible->bottom - visible->top;

  Rectangle extents = ddisp->diagram->data->extents;
  real ex_width = extents.right - extents.left;
  real ex_height = extents.bottom - extents.top;

  Point new_origo = ddisp->origo;
  point_add(&new_origo, delta);

  rectangle_union(&extents, visible);

  if (new_origo.x < extents.left - ex_width)
    new_origo.x = extents.left - ex_width;

  if (new_origo.x+width > extents.right + ex_width)
    new_origo.x = extents.right - width + ex_width;

  if (new_origo.y < extents.top - ex_height)
    new_origo.y = extents.top - ex_height;
  
  if (new_origo.y+height > extents.bottom + ex_height)
    new_origo.y = extents.bottom - height + ex_height;

  if ( (new_origo.x != ddisp->origo.x) ||
       (new_origo.y != ddisp->origo.y) ) {
    ddisplay_set_origo(ddisp, new_origo.x, new_origo.y);
    ddisplay_update_scrollbars(ddisp);
    ddisplay_add_update_all(ddisp);
    return TRUE;
  }
  return FALSE;
}

void ddisplay_scroll_up(DDisplay *ddisp)
{
  Point delta;

  delta.x = 0;
  delta.y = -(ddisp->visible.bottom - ddisp->visible.top)/4.0;
  
  ddisplay_scroll(ddisp, &delta);
}

void ddisplay_scroll_down(DDisplay *ddisp)
{
  Point delta;

  delta.x = 0;
  delta.y = (ddisp->visible.bottom - ddisp->visible.top)/4.0;
  
  ddisplay_scroll(ddisp, &delta);
}

void ddisplay_scroll_left(DDisplay *ddisp)
{
  Point delta;

  delta.x = -(ddisp->visible.right - ddisp->visible.left)/4.0;
  delta.y = 0;
  
  ddisplay_scroll(ddisp, &delta);
}

void ddisplay_scroll_right(DDisplay *ddisp)
{
  Point delta;

  delta.x = (ddisp->visible.right - ddisp->visible.left)/4.0;
  delta.y = 0;
  
  ddisplay_scroll(ddisp, &delta);
}

/** Scroll display to have the diagram point p at the center. 
 * Returns TRUE if anything changed. */
gboolean
ddisplay_scroll_center_point(DDisplay *ddisp, Point *p)
{
  Point center;

  /* Find current center */
  center.x = (ddisp->visible.right+ddisp->visible.left)/2;
  center.y = (ddisp->visible.top+ddisp->visible.bottom)/2;

  point_sub(p, &center);
  return ddisplay_scroll(ddisp, p);
}

/** Scroll display so that obj is centered.
 * Returns TRUE if anything changed.  */
gboolean
ddisplay_scroll_to_object(DDisplay *ddisp, DiaObject *obj)
{
  Rectangle r = obj->bounding_box;

  Point p;
  p.x = (r.left+r.right)/2;
  p.y = (r.top+r.bottom)/2;

  return ddisplay_scroll_center_point(ddisp, &p);
}

void
ddisplay_set_renderer(DDisplay *ddisp, int aa_renderer)
{
  int width, height;

  if (ddisp->renderer)
    DIA_RENDERER_GET_CLASS(ddisp->renderer)->end_render(ddisp->renderer);

  if (ddisp->renderer)
    g_object_unref (ddisp->renderer);

  ddisp->aa_renderer = aa_renderer;

  width = ddisp->canvas->allocation.width;
  height = ddisp->canvas->allocation.height;

  if (ddisp->aa_renderer){
    ddisp->renderer = new_libart_renderer(
                         dia_transform_new (&ddisp->visible, 
                                            &ddisp->zoom_factor), 1);
  } else {
    ddisp->renderer = new_gdk_renderer(ddisp);
  }

  dia_renderer_set_size(ddisp->renderer, ddisp->canvas->window, width, height);
  DIA_RENDERER_GET_CLASS(ddisp->renderer)->begin_render(ddisp->renderer);
}

void
ddisplay_resize_canvas(DDisplay *ddisp,
		       int width,  int height)
{
  if (ddisp->renderer==NULL) {
    if (ddisp->aa_renderer)
      ddisp->renderer = new_libart_renderer(
                           dia_transform_new (&ddisp->visible, 
                                              &ddisp->zoom_factor), 1);
    else
      ddisp->renderer = new_gdk_renderer(ddisp);
  }

  dia_renderer_set_size(ddisp->renderer, ddisp->canvas->window, width, height);

  ddisplay_set_origo(ddisp, ddisp->origo.x, ddisp->origo.y);

  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
}

DDisplay *
ddisplay_active(void)
{
  return active_display;
}

Diagram *
ddisplay_active_diagram(void)
{
  DDisplay *ddisp = ddisplay_active ();

  if (!ddisp) return NULL;
  return ddisp->diagram;
}

static void 
ddisp_destroy(DDisplay *ddisp)
{
  if (ddisp->update_id) {
    gtk_idle_remove(ddisp->update_id);
    ddisp->update_id = 0;
  }

  g_signal_handlers_disconnect_by_func (ddisp->diagram, selection_changed, ddisp);

  g_object_unref (G_OBJECT (ddisp->diagram));
  g_object_unref (G_OBJECT (ddisp->im_context));
  ddisp->im_context = NULL;

  ddisplay_im_context_preedit_reset(ddisp, active_focus());

  /* This calls ddisplay_really_destroy */
  gtk_widget_destroy (ddisp->shell);
}

static void
are_you_sure_close_dialog_respond(GtkWidget *widget, /* the dialog */
                                  gint       response_id,
                                  gpointer   user_data) /* the display */
{
  DDisplay *ddisp = (DDisplay *)user_data;

  switch (response_id) {
  case GTK_RESPONSE_YES :  
    /* save changes */
    diagram_save(ddisp->diagram, ddisp->diagram->filename);
  
    if (ddisp->update_id) {
      gtk_idle_remove(ddisp->update_id);
      ddisp->update_id = 0;
    }
    /* fall through */
  case GTK_RESPONSE_NO :
    ddisp_destroy (ddisp);
    /* fall through */
  case GTK_RESPONSE_CANCEL :
  case GTK_RESPONSE_NONE :
  case GTK_RESPONSE_DELETE_EVENT : /* closing via window manager */
    gtk_widget_destroy(widget);
    break;
  default :
    g_assert_not_reached();
  }
}

void
ddisplay_close(DDisplay *ddisp)
{
  Diagram *dia;
  GtkWidget *dialog, *button;
  gchar *fname;
  gchar *msg;

  dia = ddisp->diagram;
  
  if ( (dia->display_count > 1) ||
       (!diagram_is_modified(dia)) ) {
    ddisp_destroy(ddisp);
    return;
  }

  fname = dia->filename;
  if (!fname)
    fname = _("<unnamed>");
  msg = g_strdup_printf (
          _("The diagram '%s'\n"
            "has not been saved. Save changes now?"),
	  fname);

  dialog = gtk_message_dialog_new(GTK_WINDOW (ddisp->shell), 
                                  GTK_DIALOG_MODAL,
                                  GTK_MESSAGE_QUESTION,
                                  GTK_BUTTONS_NONE, /* no standard buttons */
				  msg,
                                  NULL);
  g_free (msg);
  gtk_window_set_title (GTK_WINDOW(dialog), _("Close Diagram"));

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
  gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_CANCEL);

  button = gtk_button_new_with_label (_("Discard Changes"));
  gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_NO);

  /* button = gtk_button_new_with_label (_("Save and Close")); */
  button = gtk_button_new_from_stock (GTK_STOCK_SAVE);
  gtk_dialog_add_action_widget (GTK_DIALOG(dialog), button, GTK_RESPONSE_YES);
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_dialog_set_default_response (GTK_DIALOG(dialog), GTK_RESPONSE_YES);

  g_signal_connect (G_OBJECT (dialog), "response",
		    G_CALLBACK(are_you_sure_close_dialog_respond),
		    ddisp);

  gtk_widget_show_all(dialog);
}

void
display_update_menu_state(DDisplay *ddisp)
{
  static gboolean initialized = 0;

  static GtkCheckMenuItem *rulers;
  static GtkCheckMenuItem *visible_grid;
  static GtkCheckMenuItem *snap_to_grid;
  static GtkCheckMenuItem *show_cx_pts;
#ifdef HAVE_LIBART
  static GtkCheckMenuItem *antialiased;
#endif

  if ((!initialized) && (ddisp->menu_bar == NULL)) {
    rulers       = GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/Show Rulers", NULL));
    visible_grid = GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/Show Grid", NULL));
    snap_to_grid = GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/Snap To Grid", NULL));
    show_cx_pts  = 
      GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/Show Connection Points", NULL));
#ifdef HAVE_LIBART
    antialiased = GTK_CHECK_MENU_ITEM(menus_get_item_from_path("<Display>/View/AntiAliased", NULL));
#endif

    initialized = TRUE;
  }
  ddisplay_do_update_menu_sensitivity (ddisp);
  
  if (ddisp->menu_bar) {
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ddisp->rulers),
				     GTK_WIDGET_VISIBLE (ddisp->hrule) ? 1 : 0); 
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ddisp->visible_grid),
				     ddisp->grid.visible);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ddisp->snap_to_grid),
				     ddisp->grid.snap);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ddisp->show_cx_pts_mitem),
				     ddisp->show_cx_pts); 
#ifdef HAVE_LIBART
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ddisp->antialiased),
				     ddisp->aa_renderer);
#endif
  }
  else {
      gtk_check_menu_item_set_active(rulers,
				     GTK_WIDGET_VISIBLE (ddisp->hrule) ? 1 : 0); 
      gtk_check_menu_item_set_active(visible_grid,
				     ddisp->grid.visible);
      gtk_check_menu_item_set_active(snap_to_grid,
				     ddisp->grid.snap);
      gtk_check_menu_item_set_active(show_cx_pts,
				     ddisp->show_cx_pts); 
#ifdef HAVE_LIBART
      gtk_check_menu_item_set_active(antialiased,
				     ddisp->aa_renderer);
#endif 
  }  
}

void 
ddisplay_do_update_menu_sensitivity (DDisplay *ddisp)
{
    Diagram *dia;
    
    dia = ddisp->diagram; 
    if (ddisp->menu_bar) {
	diagram_update_menubar_sensitivity(dia, &ddisp->updatable_menu_items);
    }
    else {
	diagram_update_popupmenu_sensitivity(dia);
    }
}



/* This is called when ddisp->shell is destroyed... */
void
ddisplay_really_destroy(DDisplay *ddisp)
{
  Diagram *dia;

  if (active_display == ddisp)
    display_set_active(NULL);

  dia = ddisp->diagram;
  
  diagram_remove_ddisplay(dia, ddisp);

  g_object_unref (ddisp->renderer);
  ddisp->renderer = NULL;
  
  g_hash_table_remove(display_ht, ddisp->shell);
  g_hash_table_remove(display_ht, ddisp->canvas);

  /* Free update_areas list: */
  ddisplay_free_update_areas(ddisp);
  /* Free display_areas list */
  ddisplay_free_display_areas(ddisp);
  
  g_free(ddisp);
}


void
ddisplay_set_title(DDisplay  *ddisp, char *title)
{
  gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
}

void
ddisplay_set_all_cursor(GdkCursor *cursor)
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
ddisplay_set_cursor(DDisplay *ddisp, GdkCursor *cursor)
{
  gdk_window_set_cursor(ddisp->canvas->window, cursor);
}

void 
ddisplay_update_statusbar(DDisplay *ddisp)
{
  update_zoom_status (ddisp);
  update_snap_grid_status (ddisp);
  update_modified_status (ddisp);
}

void
display_set_active(DDisplay *ddisp)
{
  if (ddisp != active_display) {
    active_display = ddisp;

    /* perform notification here (such as switch layers dialog) */
    layer_dialog_set_diagram(ddisp ? ddisp->diagram : NULL);
    diagram_properties_set_diagram(ddisp ? ddisp->diagram : NULL);

    if (ddisp) {
      display_update_menu_state(ddisp);

      if (prefs.toolbox_on_top) {
        gtk_window_set_transient_for(GTK_WINDOW(interface_get_toolbox_shell()),
                                     GTK_WINDOW(ddisp->shell));
      }
    }
  }
}

void
ddisplay_im_context_preedit_reset(DDisplay *ddisp, Focus *focus)
{
  if (ddisp->preedit_string != NULL) {
    if (focus != NULL) {
      int i;
      ObjectChange *change;
      
      for (i = 0; i < g_utf8_strlen(ddisp->preedit_string, -1); i++) {
        (focus->key_event)(focus, GDK_BackSpace, NULL, 0, &change);
      }
    }
    
    g_free(ddisp->preedit_string);
    ddisp->preedit_string = NULL;
  }
  if (ddisp->preedit_attrs != NULL) {
    pango_attr_list_unref(ddisp->preedit_attrs);
    ddisp->preedit_attrs = NULL;
  }
}
