/* xxxxxx -- an diagram creation/manipulation program
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
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "display.h"
#include "group.h"
#include "interface.h"
#include "color.h"
#include "handle_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "cut_n_paste.h"
#include "message.h"


static GHashTable *display_ht = NULL;
static GdkCursor *current_cursor = NULL;

GdkCursor *default_cursor = NULL;

typedef struct _IRectangle {
  int top, bottom;
  int left,right;
} IRectangle;

static guint display_hash(DDisplay *ddisp);

DDisplay *
new_display(Diagram *dia)
{
  DDisplay *ddisp;
  char *filename;
  
  ddisp = g_new(DDisplay,1);

  ddisp->diagram = dia;

  ddisp->origo.x = 0.0;
  ddisp->origo.y = 0.0;
  ddisp->zoom_factor = DDISPLAY_NORMAL_ZOOM;

  ddisp->grid.visible = TRUE;
  ddisp->grid.snap = FALSE;
  ddisp->grid.width_x = 1.0;
  ddisp->grid.width_y = 1.0;
  ddisp->grid.gc = NULL;
  ddisp->grid.dialog = NULL;
  ddisp->grid.entry_x = NULL;
  ddisp->grid.entry_y = NULL;
  
  ddisp->renderer = NULL;
  
  ddisp->update_areas = NULL;
  ddisp->display_areas = NULL;

  filename = strrchr(dia->filename, '/');
  if (filename==NULL) {
    filename = dia->filename;
  } else {
    filename++;
  }
  
  create_display_shell(ddisp, 256, 256, filename);

  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) display_hash, NULL);

  ddisplay_set_cursor(ddisp, current_cursor);
  
  g_hash_table_insert (display_ht, ddisp->shell, ddisp);
  g_hash_table_insert (display_ht, ddisp->canvas, ddisp);

  return ddisp;  /*  set the user data  */
}

static guint
display_hash(DDisplay *ddisp)
{
  return (gulong) ddisp;
}

void
ddisplay_transform_coords(DDisplay *ddisp,
			  coord x, coord y,
			  int *xi, int *yi)
{
  Rectangle *visible = &ddisp->visible;
  int width = ddisp->renderer->renderer.pixel_width;
  int height = ddisp->renderer->renderer.pixel_height;
  
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
  int width = ddisp->renderer->renderer.pixel_width;
  int height = ddisp->renderer->renderer.pixel_height;
  
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

void
ddisplay_add_update_all(DDisplay *ddisp)
{
  ddisplay_add_update(ddisp, &ddisp->visible);
}

void
ddisplay_add_update(DDisplay *ddisp, Rectangle *rect)
{
  Rectangle *r;
  int top,bottom,left,right;
  Rectangle *visible;
  int width = ddisp->renderer->renderer.pixel_width;
  int height = ddisp->renderer->renderer.pixel_height;

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

void
ddisplay_flush(DDisplay *ddisp)
{
  GSList *l;
  IRectangle *ir;
  Rectangle *r;
  Renderer *renderer;

  /* Renders updates to pixmap + copies display_areas to canvas(screen) */

  renderer = &ddisp->renderer->renderer;

  (renderer->interactive_ops->clip_region_clear)(renderer);
  
  l = ddisp->update_areas;
  while(l!=NULL) {
    r = (Rectangle *) l->data;

    (renderer->interactive_ops->clip_region_add_rect)(renderer,  r);
    
    l = g_slist_next(l);
  }
  /* Free list: */
  g_slist_free(ddisp->update_areas);
  ddisp->update_areas = NULL;

  ddisplay_render_pixmap(ddisp);
  

  l = ddisp->display_areas;
  while(l!=NULL) {
    ir = (IRectangle *) l->data;

    renderer_gdk_copy_to_window(ddisp->renderer, ddisp->canvas->window,
				ir->left, ir->top,
				ir->right - ir->left, ir->bottom - ir->top);
    
    l = g_slist_next(l);
  }
 
  g_slist_free(ddisp->display_areas);
  ddisp->display_areas = NULL;
}

static void
ddisplay_obj_render(Object *obj, Renderer *renderer,
		    int active_layer,
		    gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;
  int i;

  obj->ops->draw(obj, renderer);
  if (active_layer) {
    for (i=0;i<obj->num_connections;i++) {
      connectionpoint_draw(obj->connections[i], ddisp);
    }
  }
}

void
ddisplay_render_pixmap(DDisplay *ddisp)
{
  GList *list;
  Object *obj;
  int i;
  Renderer *renderer;
  
  if (ddisp->renderer==NULL) {
    printf("ERROR! Renderer was NULL!!\n");
    return;
  }

  renderer = &ddisp->renderer->renderer;
  
    
  /* Erase background */
  (renderer->interactive_ops->fill_pixel_rect)(renderer,
					       0, 0,
					       renderer->pixel_width,
					       renderer->pixel_height,
					       &ddisp->diagram->data->bg_color);

  /* Draw grid */
  grid_draw(ddisp);

  data_render(ddisp->diagram->data, (Renderer *)ddisp->renderer,
	      ddisplay_obj_render, (gpointer) ddisp);
  
  /* Draw handles for all selected objects */
  list = ddisp->diagram->data->selected;
  while (list!=NULL) {
    obj = (Object *) list->data;

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

  gtk_signal_emit_by_name (GTK_OBJECT (ddisp->hsbdata), "changed");

  /* Vertical: */
  vsbdata = ddisp->vsbdata;
  vsbdata->lower = MIN(extents->top, visible->top);
  vsbdata->upper = MAX(extents->bottom, visible->bottom);
  vsbdata->page_size = visible->bottom - visible->top - 0.00001;
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  vsbdata->page_increment = (visible->bottom - visible->top) / 2.0;
  vsbdata->step_increment = (visible->bottom - visible->top) / 10.0;
  vsbdata->value = visible->top;

  gtk_signal_emit_by_name (GTK_OBJECT (ddisp->vsbdata), "changed");
}

void
ddisplay_set_origo(DDisplay *ddisp, coord x, coord y)
{
  Rectangle *extents = &ddisp->diagram->data->extents;
  Rectangle *visible = &ddisp->visible;
  int width = ddisp->renderer->renderer.pixel_width;
  int height = ddisp->renderer->renderer.pixel_height;
    
  /*  updaterar origo+visible+rulers */
  ddisp->origo.x = x;
  ddisp->origo.y = y;

  if (ddisp->zoom_factor<DDISPLAY_MIN_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MIN_ZOOM;
  
  if (ddisp->zoom_factor > DDISPLAY_MAX_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MAX_ZOOM;

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
}

void ddisplay_scroll(DDisplay *ddisp, Point *delta)
{
  Point new_origo;
  Rectangle extents;
  Rectangle *visible = &ddisp->visible;
  real width, height;
  
  new_origo = ddisp->origo;
  point_add(&new_origo, delta);

  width = visible->right - visible->left;
  height = visible->bottom - visible->top;
  
  extents = ddisp->diagram->data->extents;
  rectangle_union(&extents, visible);
  
  if (new_origo.x < extents.left)
    new_origo.x = extents.left;

  if (new_origo.x+width > extents.right)
    new_origo.x = extents.right - width;

  if (new_origo.y < extents.top)
    new_origo.y = extents.top;
  
  if (new_origo.y+height > extents.bottom)
    new_origo.y = extents.bottom - height;

  ddisplay_set_origo(ddisp, new_origo.x, new_origo.y);
  ddisplay_update_scrollbars(ddisp);
  ddisplay_add_update_all(ddisp);
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

void
ddisplay_resize_canvas(DDisplay *ddisp,
		       int width,  int height)
{
  if (ddisp->renderer==NULL) {
    ddisp->renderer = new_gdk_renderer(ddisp);
  }
  
  gdk_renderer_set_size(ddisp->renderer, ddisp->canvas->window, width, height);

  ddisplay_set_origo(ddisp, ddisp->origo.x, ddisp->origo.y);

  ddisplay_add_update(ddisp, &ddisp->visible);
  ddisplay_flush(ddisp);
}

extern DDisplay *ddisplay_active(void)
{
  DDisplay *ddisp;
  GtkWidget *event_widget;
  GtkWidget *toplevel_widget;
  GdkEvent *event;

  /*  If the popup shell is valid, then get the gdisplay associated with that shell  */
  event = gtk_get_current_event ();
  event_widget = gtk_get_event_widget (event);
  gdk_event_free (event);

  if (event_widget == NULL)
    return NULL;

  toplevel_widget = gtk_widget_get_toplevel (event_widget);
  ddisp = g_hash_table_lookup (display_ht, toplevel_widget);

  if (ddisp)
    return ddisp;

  if (popup_shell) {
    ddisp = gtk_object_get_user_data (GTK_OBJECT (popup_shell));
    return ddisp;
  }

  message_error("Internal error: "
		"Strange, shouldn't come here (ddisplay_active())"); 
  return NULL;
}

static void
are_you_sure_close_dialog_no(GtkWidget *widget, GtkWidget *dialog)
{
  gtk_grab_remove(dialog);
  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);
}

static void
are_you_sure_close_dialog_yes(GtkWidget *widget,
			      GtkWidget *dialog)
{
  DDisplay *ddisp;

  ddisp =  gtk_object_get_user_data(GTK_OBJECT(dialog));
  
  gtk_grab_remove(dialog);
  gtk_widget_hide(dialog);
  gtk_widget_destroy(dialog);
  gtk_widget_destroy (ddisp->shell);
}


void
ddisplay_close(DDisplay *ddisp)
{
  Diagram *dia;
  GtkWidget *dialog;
  GtkWidget *label;
  GtkWidget *button;
  
  dia = ddisp->diagram;
  
  if ( (dia->display_count > 1) ||
       (!dia->modified) ) {
    gtk_widget_destroy (ddisp->shell);
    return;
  }
  
  dialog = gtk_dialog_new();

  gtk_window_set_title (GTK_WINDOW (dialog), "Really close?");
  gtk_container_border_width (GTK_CONTAINER (dialog), 0);
  
  label = gtk_label_new ("This diagram has not been saved.\n"
			 "Are you sure you want to close this window?");
  
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
		      label, TRUE, TRUE, 0);
  
  gtk_widget_show (label);
  
  button = gtk_button_new_with_label ("Yes");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_object_set_user_data(GTK_OBJECT(dialog), ddisp);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_yes),
		      dialog);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label ("No");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      button, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_no),
		      dialog);

  gtk_widget_show (button);

  /* Make dialog modal: */
  gtk_widget_grab_focus(dialog);
  gtk_grab_add(dialog);
  
  gtk_widget_show (dialog);
}

void
display_set_menu_sensitivity(DDisplay *ddisp)
{
  Diagram *dia;
  
  dia = ddisp->diagram;

  menus_set_sensitive("<Display>/Edit/Copy",
		      dia->data->selected_count > 0);
  menus_set_sensitive("<Display>/Edit/Cut",
		      dia->data->selected_count > 0);
  menus_set_sensitive("<Display>/Edit/Paste",
		      cnp_exist_stored_objects());
  menus_set_sensitive("<Display>/Edit/Delete",
		      dia->data->selected_count > 0);

  menus_set_sensitive("<Display>/Objects/Place Under",
		      dia->data->selected_count > 0);
  menus_set_sensitive("<Display>/Objects/Place Over",
		      dia->data->selected_count > 0);
  menus_set_sensitive("<Display>/Objects/Group", dia->data->selected_count > 1);
  menus_set_sensitive("<Display>/Objects/Ungroup",
		      (dia->data->selected_count == 1) &&
		      IS_GROUP((Object *)dia->data->selected->data));

  menus_set_state ("<Display>/View/Toggle Rulers", GTK_WIDGET_VISIBLE (ddisp->hrule) ? 1 : 0);
  menus_set_state ("<Display>/View/Visible Grid", ddisp->grid.visible);
  menus_set_state ("<Display>/View/Snap To Grid", ddisp->grid.snap);
}

/* This is called when ddisp->shell is destroyed... */
void
ddisplay_really_destroy(DDisplay *ddisp)
{
  Diagram *dia;
  
  dia = ddisp->diagram;
  
  diagram_remove_ddisplay(dia, ddisp);

  grid_destroy_dialog(&ddisp->grid);
  
  destroy_gdk_renderer(ddisp->renderer);
  
  g_hash_table_remove(display_ht, ddisp->shell);
  g_hash_table_remove(display_ht, ddisp->canvas);

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
  
  list = open_diagrams;
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

