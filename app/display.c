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
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "config.h"
#ifdef GNOME
#include <gnome.h>
#endif

#include "intl.h"

#include "display.h"
#include "group.h"
#include "interface.h"
#include "color.h"
#include "handle_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "cut_n_paste.h"
#include "message.h"
#include "preferences.h"
#include "app_procs.h"

static GHashTable *display_ht = NULL;
static GdkCursor *current_cursor = NULL;

GdkCursor *default_cursor = NULL;

typedef struct _IRectangle {
  int top, bottom;
  int left,right;
} IRectangle;

static guint display_hash(DDisplay *ddisp);

static void
update_zoom_status(DDisplay *ddisp)
{
  GtkStatusbar *statusbar;
  guint context_id;
  gchar *zoom_text;

  statusbar = GTK_STATUSBAR (ddisp->zoom_status);
  context_id = gtk_statusbar_get_context_id (statusbar, "Zoom");
  
  gtk_statusbar_pop (statusbar, context_id); 
  zoom_text = g_malloc (sizeof (guchar) * (strlen (_("Zoom")) + 1 + 8 + 1));
  sprintf (zoom_text, "%s % 7.1f%c", _("Zoom"),
	   ddisp->zoom_factor * 100.0 / DDISPLAY_NORMAL_ZOOM, '%');
  gtk_statusbar_push (statusbar, context_id, zoom_text);

  g_free (zoom_text);
}

static void
update_modified_status(DDisplay *ddisp)
{
  GtkStatusbar *statusbar;
  guint context_id;

  if (ddisp->diagram->modified)
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

DDisplay *
new_display(Diagram *dia)
{
  DDisplay *ddisp;
  char *filename;
  int embedded = app_is_embedded();

  ddisp = g_new(DDisplay,1);

  ddisp->diagram = dia;

  ddisp->grid.visible = prefs.grid.visible;
  ddisp->grid.snap = prefs.grid.snap;
  ddisp->grid.width_x = prefs.grid.x;
  ddisp->grid.width_y = prefs.grid.y;
  ddisp->grid.gc = NULL;
  ddisp->grid.dialog = NULL;
  ddisp->grid.entry_x = NULL;
  ddisp->grid.entry_y = NULL;

  ddisp->show_cx_pts = prefs.show_cx_pts;

  ddisp->autoscroll = TRUE;

  ddisp->aa_renderer = 0;
  ddisp->renderer = (Renderer *)new_gdk_renderer(ddisp);;
  
  ddisp->update_areas = NULL;
  ddisp->display_areas = NULL;

  filename = strrchr(dia->filename, '/');
  if (filename==NULL) {
    filename = dia->filename;
  } else {
    filename++;
  }

  diagram_add_ddisplay(dia, ddisp);

  ddisp->origo.x = 0.0;
  ddisp->origo.y = 0.0;
  ddisp->zoom_factor = prefs.new_view.zoom/100.0*DDISPLAY_NORMAL_ZOOM;
  ddisp->visible.left = 0.0;
  ddisp->visible.top = 0.0;
  ddisp->visible.right = prefs.new_view.width/ddisp->zoom_factor;
  ddisp->visible.bottom = prefs.new_view.height/ddisp->zoom_factor;

  create_display_shell(ddisp, prefs.new_view.width, prefs.new_view.height,
		       filename, !embedded);

  
  /*  ddisplay_set_origo(ddisp, 0.0, 0.0); */

  ddisplay_update_statusbar (ddisp);
  ddisplay_update_scrollbars(ddisp);

  if (!display_ht)
    display_ht = g_hash_table_new ((GHashFunc) display_hash, NULL);

  if (!embedded)
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
ddisplay_transform_coords_double(DDisplay *ddisp,
				 coord x, coord y,
				 double *xi, double *yi)
{
  Rectangle *visible = &ddisp->visible;
  double width = ddisp->renderer->pixel_width;
  double height = ddisp->renderer->pixel_height;
  
  *xi = (x - visible->left)  * (real)width / (visible->right - visible->left);
  *yi = (y - visible->top)  * (real)height / (visible->bottom - visible->top);
}


void
ddisplay_transform_coords(DDisplay *ddisp,
			  coord x, coord y,
			  int *xi, int *yi)
{
  Rectangle *visible = &ddisp->visible;
  int width = ddisp->renderer->pixel_width;
  int height = ddisp->renderer->pixel_height;
  
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
  int width = ddisp->renderer->pixel_width;
  int height = ddisp->renderer->pixel_height;
  
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
  int width = ddisp->renderer->pixel_width;
  int height = ddisp->renderer->pixel_height;

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
  Rectangle *r, totrect;
  Renderer *renderer;
  
  /* Renders updates to pixmap + copies display_areas to canvas(screen) */

  renderer = ddisp->renderer;

  (renderer->interactive_ops->clip_region_clear)(renderer);

  l = ddisp->update_areas;
  if (l==NULL)
    totrect = ddisp->visible;
  else 
    totrect = *(Rectangle *) l->data;

  while(l!=NULL) {
    r = (Rectangle *) l->data;

    rectangle_union(&totrect, r);
    (renderer->interactive_ops->clip_region_add_rect)(renderer,  r);
    
    l = g_slist_next(l);
  }
  
  /* Free update_areas list: */
  l = ddisp->update_areas;
  while(l!=NULL) {
    g_free(l->data);
    l = g_slist_next(l);
  }
  g_slist_free(ddisp->update_areas);
  ddisp->update_areas = NULL;

  totrect.left -= 0.1;
  totrect.right += 0.1;
  totrect.top -= 0.1;
  totrect.bottom += 0.1;
  
  ddisplay_render_pixmap(ddisp, &totrect);

  l = ddisp->display_areas;
  while(l!=NULL) {
    ir = (IRectangle *) l->data;

    if (ddisp->aa_renderer)
      renderer_libart_copy_to_window((RendererLibart *)ddisp->renderer, ddisp->canvas->window,
				     ir->left, ir->top,
				     ir->right - ir->left, ir->bottom - ir->top);
    else 
      renderer_gdk_copy_to_window((RendererGdk *)ddisp->renderer, ddisp->canvas->window,
				  ir->left, ir->top,
				  ir->right - ir->left, ir->bottom - ir->top);
    
    l = g_slist_next(l);
  }

  /* Free display_areas list */
  l = ddisp->display_areas;
  while(l!=NULL) {
    g_free(l->data);
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
  if (active_layer && ddisp->show_cx_pts) {
    for (i=0;i<obj->num_connections;i++) {
      connectionpoint_draw(obj->connections[i], ddisp);
    }
  }
}

void
ddisplay_render_pixmap(DDisplay *ddisp, Rectangle *update)
{
  GList *list;
  Object *obj;
  int i;
  Renderer *renderer;
  
  if (ddisp->renderer==NULL) {
    printf("ERROR! Renderer was NULL!!\n");
    return;
  }

  renderer = ddisp->renderer;

  /* Erase background */
  (renderer->interactive_ops->fill_pixel_rect)(renderer,
					       0, 0,
					       renderer->pixel_width-1,
					       renderer->pixel_height-1,
					       &ddisp->diagram->data->bg_color);

  /* Draw grid */
  grid_draw(ddisp, update);

  data_render(ddisp->diagram->data, (Renderer *)ddisp->renderer, update,
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
  real extra_border_x, extra_border_y;

  extra_border_x = (visible->right - visible->left);
  extra_border_y = (visible->bottom - visible->top);
  
  hsbdata = ddisp->hsbdata;
  /* Horizontal: */
  hsbdata->lower = MIN(extents->left, visible->left)  - extra_border_x;
  hsbdata->upper = MAX(extents->right, visible->right)  + extra_border_x;
  hsbdata->page_size = visible->right - visible->left - 0.0001;
  /* remove some to fix strange behaviour in gtk_range_adjustment_changed */
  hsbdata->page_increment = (visible->right - visible->left) / 2.0;
  hsbdata->step_increment = (visible->right - visible->left) / 10.0;
  hsbdata->value = visible->left;

  gtk_signal_emit_by_name (GTK_OBJECT (ddisp->hsbdata), "changed");

  /* Vertical: */
  vsbdata = ddisp->vsbdata;
  vsbdata->lower = MIN(extents->top, visible->top)  - extra_border_y;
  vsbdata->upper = MAX(extents->bottom, visible->bottom) + extra_border_y;
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
  int width, height;

  /*  updaterar origo+visible+rulers */
  ddisp->origo.x = x;
  ddisp->origo.y = y;

  if (ddisp->zoom_factor<DDISPLAY_MIN_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MIN_ZOOM;
  
  if (ddisp->zoom_factor > DDISPLAY_MAX_ZOOM)
    ddisp->zoom_factor = DDISPLAY_MAX_ZOOM;

  width = ddisp->renderer->pixel_width;
  height = ddisp->renderer->pixel_height;
  
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
    scroll.x = ddisplay_untransform_length(ddisp, scroll.x);
    scroll.y = ddisplay_untransform_length(ddisp, scroll.y);

    ddisplay_scroll(ddisp, &scroll);
    ddisplay_flush(ddisp);

    return TRUE;
  }
  else
  {
    return FALSE;
  }
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
ddisplay_set_renderer(DDisplay *ddisp, int aa_renderer)
{
  int width, height;

  if (ddisp->aa_renderer) {
    if (ddisp->renderer)
      destroy_libart_renderer((RendererLibart *)ddisp->renderer);
  } else {
    if (ddisp->renderer)
      destroy_gdk_renderer((RendererGdk *)ddisp->renderer);
  }

  ddisp->aa_renderer = aa_renderer;

  width = ddisp->canvas->allocation.width;
  height = ddisp->canvas->allocation.height;

  if (ddisp->aa_renderer){
    ddisp->renderer = (Renderer *)new_libart_renderer(ddisp, 1);
    libart_renderer_set_size((RendererLibart *)ddisp->renderer, ddisp->canvas->window, width, height);
  } else {
    ddisp->renderer = (Renderer *)new_gdk_renderer(ddisp);
    gdk_renderer_set_size((RendererGdk *)ddisp->renderer, ddisp->canvas->window, width, height);
  }
}

void
ddisplay_resize_canvas(DDisplay *ddisp,
		       int width,  int height)
{
  if (ddisp->renderer==NULL) {
    if (ddisp->aa_renderer)
      ddisp->renderer = (Renderer *)new_libart_renderer(ddisp, 1);
    else
      ddisp->renderer = (Renderer *)new_gdk_renderer(ddisp);
  }

  if (ddisp->aa_renderer)
    libart_renderer_set_size((RendererLibart *)ddisp->renderer, ddisp->canvas->window, width, height);
  else
    gdk_renderer_set_size((RendererGdk *)ddisp->renderer, ddisp->canvas->window, width, height);

  ddisplay_set_origo(ddisp, ddisp->origo.x, ddisp->origo.y);

  ddisplay_add_update(ddisp, &ddisp->visible);
  ddisplay_flush(ddisp);
}

extern DDisplay *ddisplay_active(void)
{
  DDisplay *ddisp;
  GtkWidget *event_widget = NULL;
  GtkWidget *toplevel_widget = NULL;
  GdkEvent *event;

  /*  If the popup shell is valid, then get the gdisplay
      associated with that shell  */
  event = gtk_get_current_event ();
  if (event)
    {
      event_widget = gtk_get_event_widget (event);
      gdk_event_free (event);
    }

  if (event_widget == NULL)
    return NULL;

  if (display_ht == NULL)
    return NULL;

  toplevel_widget = gtk_widget_get_toplevel (event_widget);
  ddisp = g_hash_table_lookup (display_ht, toplevel_widget);

  if (ddisp)
    return ddisp;

  if (popup_shell) {
    ddisp = gtk_object_get_user_data (GTK_OBJECT (popup_shell));
    return ddisp;
  }

  /*
  message_error(_("Internal error: "
		"Strange, shouldn't come here (ddisplay_active())")); 
  */
  return NULL;
}

static void
are_you_sure_close_dialog_no(GtkWidget *widget, GtkWidget *dialog)
{
  gtk_widget_destroy(dialog);
}

static void
are_you_sure_close_dialog_yes(GtkWidget *widget,
			      GtkWidget *dialog)
{
  DDisplay *ddisp;

  ddisp =  gtk_object_get_user_data(GTK_OBJECT(dialog));
  
  gtk_widget_destroy(dialog);
  gtk_widget_destroy (ddisp->shell);
}


void
ddisplay_close(DDisplay *ddisp)
{
  Diagram *dia;
  GtkWidget *dialog, *vbox;
  GtkWidget *label;
  GtkWidget *button;
  
  dia = ddisp->diagram;
  
  if ( (dia->display_count > 1) ||
       (!dia->modified) ) {
    gtk_widget_destroy (ddisp->shell);
    return;
  }

#ifdef GNOME
  dialog = gnome_dialog_new(_("Really close?"),
			    GNOME_STOCK_BUTTON_YES,GNOME_STOCK_BUTTON_NO,NULL);
  gnome_dialog_set_default(GNOME_DIALOG(dialog), 0);
  vbox = GNOME_DIALOG(dialog)->vbox;
#else
  dialog = gtk_dialog_new();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Really close?"));
  gtk_container_set_border_width (GTK_CONTAINER (dialog), 0);
  vbox = GTK_DIALOG(dialog)->vbox;
#endif
  
  label = gtk_label_new (_("This diagram has not been saved.\n"
			 "Are you sure you want to close this window?"));
  
  gtk_misc_set_padding (GTK_MISC (label), 10, 10);
  gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);
  
  gtk_widget_show (label);

  gtk_object_set_user_data(GTK_OBJECT(dialog), ddisp);

#ifdef GNOME
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 0,
			      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_yes),
			      dialog);
  gnome_dialog_button_connect(GNOME_DIALOG(dialog), 1,
			      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_no),
			      dialog);
#else
  button = gtk_button_new_with_label (_("Yes"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_yes),
		      dialog);
  gtk_widget_show (button);
  
  button = gtk_button_new_with_label (_("No"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
		      button, TRUE, TRUE, 0);

  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(are_you_sure_close_dialog_no),
		      dialog);

  gtk_widget_show (button);
#endif

  /* Make dialog modal: */
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  
  gtk_widget_show(dialog);
}

void
display_update_menu_state(DDisplay *ddisp)
{
  Diagram *dia;
  static int initialized = 0;

  static GtkWidget *rulers;
  static GtkWidget *visible_grid;
  static GtkWidget *snap_to_grid;
  static GtkWidget *show_cx_pts;
  static GString *path;
  char *display = "<Display>";

  if (initialized==0) {
    path = g_string_new (display);
    g_string_append (path,_("/View/Show Rulers"));
    rulers       = menus_get_item_from_path(path->str);
    g_string_append (g_string_assign(path, display),_("/View/Visible Grid"));
    visible_grid = menus_get_item_from_path(path->str);
    g_string_append (g_string_assign(path, display),_("/View/Snap To Grid"));
    snap_to_grid = menus_get_item_from_path(path->str);
    g_string_append (g_string_assign(path, display),_("/View/Show Connection Points"));
    show_cx_pts  = menus_get_item_from_path(path->str);

    g_string_free (path,FALSE);
    initialized = 1;
  }
  
  dia = ddisp->diagram;

  diagram_update_menu_sensitivity(dia);

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(rulers),
				 GTK_WIDGET_VISIBLE (ddisp->hrule) ? 1 : 0); 
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(visible_grid),
				 ddisp->grid.visible);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(snap_to_grid),
				 ddisp->grid.snap);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(show_cx_pts),
				 ddisp->show_cx_pts); 
}

/* This is called when ddisp->shell is destroyed... */
void
ddisplay_really_destroy(DDisplay *ddisp)
{
  Diagram *dia;
  GSList *l;

  dia = ddisp->diagram;
  
  diagram_remove_ddisplay(dia, ddisp);

  grid_destroy_dialog(&ddisp->grid);

  if (ddisp->aa_renderer)
    destroy_libart_renderer((RendererLibart *)ddisp->renderer);
  else
    destroy_gdk_renderer((RendererGdk *)ddisp->renderer);
    
  
  g_hash_table_remove(display_ht, ddisp->shell);
  g_hash_table_remove(display_ht, ddisp->canvas);

  /* Free update_areas list: */
  l = ddisp->update_areas;
  while(l!=NULL) {
    g_free(l->data);
    l = g_slist_next(l);
  }
  g_slist_free(ddisp->update_areas);
  /* Free display_areas list */
  l = ddisp->display_areas;
  while(l!=NULL) {
    g_free(l->data);
    l = g_slist_next(l);
  }
  g_slist_free(ddisp->display_areas);

  
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

void 
ddisplay_update_statusbar(DDisplay *ddisp)
{
  update_zoom_status (ddisp);
  update_modified_status (ddisp);
}
