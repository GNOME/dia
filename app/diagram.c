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
#include <assert.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "diagram.h"
#include "group.h"
#include "object_ops.h"
#include "render_eps.h"
#include "focus.h"
#include "message.h"
#include "menus.h"
#include "preferences.h"
#include "properties.h"
#include "cut_n_paste.h"
#include "layer_dialog.h"
#include "app_procs.h"
#include "dia_dirs.h"
#include "load_save.h"
#include "recent_files.h"

GList *open_diagrams = NULL;

Diagram *
diagram_load(const char *filename, DiaImportFilter *ifilter)
{
  Diagram *diagram;

  if (!ifilter)
    ifilter = filter_guess_import_filter(filename);
  if (!ifilter)  /* default to native format */
    ifilter = &dia_import_filter;
  diagram = new_diagram(filename);
  if (ifilter->import(filename, diagram->data, ifilter->user_data)) {
    diagram->unsaved = FALSE;
    diagram_set_modified(diagram, FALSE);
    recent_file_history_add(filename, ifilter);
  } else {
    diagram_destroy(diagram);
    diagram = NULL;
  }
  return diagram;
}

Diagram *
new_diagram(const char *filename)  /* Note: filename is copied */
{
  Diagram *dia = g_new(Diagram, 1);
  
  dia->data = new_diagram_data();
  dia->data->grid.width_x = prefs.grid.x;
  dia->data->grid.width_y = prefs.grid.y;

  dia->filename = g_strdup(filename);
  
  dia->unsaved = TRUE;
  dia->modified = FALSE;

  dia->display_count = 0;
  dia->displays = NULL;

  dia->undo = new_undo_stack(dia);
  
  open_diagrams = g_list_prepend(open_diagrams, dia);
  layer_dialog_update_diagram_list();
  return dia;
}

void
diagram_destroy(Diagram *dia)
{
  char *home_path;

  assert(dia->displays==NULL);

  diagram_data_destroy(dia->data);
  
  g_free(dia->filename);

  /* Save menu accelerators */
  home_path = dia_config_filename("menus" G_DIR_SEPARATOR_S "display");

  if (home_path != NULL) {
    GtkPatternSpec pattern;

    gtk_pattern_spec_init(&pattern, "*<Display>*");

    gtk_item_factory_dump_rc (home_path, &pattern, TRUE);
    g_free (home_path);
    gtk_pattern_spec_free_segs(&pattern);
  }

  open_diagrams = g_list_remove(open_diagrams, dia);
  layer_dialog_update_diagram_list();

  undo_destroy(dia->undo);
  
  g_free(dia);
}

void
diagram_modified(Diagram *dia)
{
  diagram_set_modified(dia, TRUE);
  diagram_update_menu_sensitivity(dia);
}

void
diagram_set_modified(Diagram *dia, int modified)
{
  GSList *displays;

  if (dia->modified != modified)
  {
    dia->modified = modified;
    displays = dia->displays;
    while (displays != NULL)
    {
      DDisplay *display = (DDisplay*) displays->data;
      ddisplay_update_statusbar(display);
      displays = g_slist_next(displays);
    }
  }
}

void diagram_update_menu_sensitivity(Diagram *dia)
{
  static int initialized = 0;
  static GtkWidget *copy;
  static GtkWidget *cut;
  static GtkWidget *paste;
#ifndef GNOME
  static GtkWidget *delete;
#endif
  static GtkWidget *copy_text;
  static GtkWidget *cut_text;
  static GtkWidget *paste_text;

  static GtkWidget *send_to_back;
  static GtkWidget *bring_to_front;

  static GtkWidget *group;
  static GtkWidget *ungroup;

  static GtkWidget *align_h_l;
  static GtkWidget *align_h_c;
  static GtkWidget *align_h_r;
  static GtkWidget *align_h_e;
  static GtkWidget *align_h_a;

  static GtkWidget *align_v_t;
  static GtkWidget *align_v_c;
  static GtkWidget *align_v_b;
  static GtkWidget *align_v_e;
  static GtkWidget *align_v_a;
  
  if (initialized==0) {
#   ifdef GNOME
    if (ddisplay_active () == NULL) return;
#   endif
    copy = menus_get_item_from_path("<Display>/Edit/Copy");
    cut = menus_get_item_from_path("<Display>/Edit/Cut");
    paste = menus_get_item_from_path("<Display>/Edit/Paste");
#   ifndef GNOME
    delete = menus_get_item_from_path("<Display>/Edit/Delete");
#   endif

    copy_text = menus_get_item_from_path("<Display>/Edit/Copy Text");
    cut_text = menus_get_item_from_path("<Display>/Edit/Cut Text");
    paste_text = menus_get_item_from_path("<Display>/Edit/Paste Text");

    send_to_back = menus_get_item_from_path("<Display>/Objects/Send to Back");
    bring_to_front =
      menus_get_item_from_path("<Display>/Objects/Bring to Front");
    group = menus_get_item_from_path("<Display>/Objects/Group");
    ungroup = menus_get_item_from_path("<Display>/Objects/Ungroup");
    align_h_l =
      menus_get_item_from_path("<Display>/Objects/Align Horizontal/Left");
    align_h_c =
      menus_get_item_from_path("<Display>/Objects/Align Horizontal/Center");
    align_h_r =
      menus_get_item_from_path("<Display>/Objects/Align Horizontal/Right");
    align_h_e =
      menus_get_item_from_path(
	  "<Display>/Objects/Align Horizontal/Equal Distance");
    align_h_a =
      menus_get_item_from_path("<Display>/Objects/Align Horizontal/Adjacent");
    align_v_t =
      menus_get_item_from_path("<Display>/Objects/Align Vertical/Top");
    align_v_c =
      menus_get_item_from_path("<Display>/Objects/Align Vertical/Center");
    align_v_b =
      menus_get_item_from_path("<Display>/Objects/Align Vertical/Bottom");
    align_v_e =
      menus_get_item_from_path(
	  "<Display>/Objects/Align Vertical/Equal Distance");
    align_v_a =
      menus_get_item_from_path("<Display>/Objects/Align Vertical/Adjacent");
    
    initialized = 1;
  }
  
  gtk_widget_set_sensitive(copy, dia->data->selected_count > 0);
  gtk_widget_set_sensitive(cut, dia->data->selected_count > 0);
  gtk_widget_set_sensitive(paste, cnp_exist_stored_objects());
#ifndef GNOME
  gtk_widget_set_sensitive(delete, dia->data->selected_count > 0);
#endif

  gtk_widget_set_sensitive(copy_text, active_focus() != NULL);
  gtk_widget_set_sensitive(cut_text, 0); 
  gtk_widget_set_sensitive(paste_text, active_focus() != NULL);

  gtk_widget_set_sensitive(send_to_back, dia->data->selected_count > 0);
  gtk_widget_set_sensitive(bring_to_front, dia->data->selected_count > 0);
  
  gtk_widget_set_sensitive(group, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(ungroup,
			   (dia->data->selected_count == 1) &&
			   IS_GROUP((Object *)dia->data->selected->data));

  gtk_widget_set_sensitive(align_h_l, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_h_c, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_h_r, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_h_e, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_h_a, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_v_t, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_v_c, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_v_b, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_v_e, dia->data->selected_count > 1);
  gtk_widget_set_sensitive(align_v_a, dia->data->selected_count > 1);
}


void
diagram_add_ddisplay(Diagram *dia, DDisplay *ddisp)
{
  dia->displays = g_slist_prepend(dia->displays, ddisp);
  dia->display_count++;
}

void
diagram_remove_ddisplay(Diagram *dia, DDisplay *ddisp)
{
  dia->displays = g_slist_remove(dia->displays, ddisp);
  dia->display_count--;

  if(dia->display_count == 0) {
    if (!app_is_embedded()) {
      /* Don't delete embedded diagram when last view is closed */
      diagram_destroy(dia);
    }
  }
}

void
diagram_add_object(Diagram *dia, Object *obj)
{
  layer_add_object(dia->data->active_layer, obj);

  diagram_modified(dia);
}

void
diagram_add_object_list(Diagram *dia, GList *list)
{
  layer_add_objects(dia->data->active_layer, list);

  diagram_modified(dia);
}

void
diagram_selected_break_external(Diagram *dia)
{
  GList *list;
  GList *connected_list;
  Object *obj;
  Object *other_obj;
  int i,j;

  list = dia->data->selected;
  while (list != NULL) {
    obj = (Object *)list->data;
    
    /* Break connections between this object and objects not selected: */
    for (i=0;i<obj->num_handles;i++) {
      ConnectionPoint *con_point;
      con_point = obj->handles[i]->connected_to;
      
      if ( con_point == NULL )
	break; /* Not connected */
      
      other_obj = con_point->object;
      if (g_list_find(dia->data->selected, other_obj) == NULL) {
	/* other_obj is not selected, break connection */
	Change *change = undo_unconnect(dia, obj, obj->handles[i]);
	(change->apply)(change, dia);
	object_add_updates(obj, dia);
      }
    }
    
    /* Break connections from non selected objects to this object: */
    for (i=0;i<obj->num_connections;i++) {
      connected_list = obj->connections[i]->connected;
      
      while (connected_list != NULL) {
	other_obj = (Object *)connected_list->data;
	
	if (g_list_find(dia->data->selected, other_obj) == NULL) {
	  /* other_obj is not in list, break all connections
	     to obj from other_obj */
	  
	  for (j=0;j<other_obj->num_handles;j++) {
	    ConnectionPoint *con_point;
	    con_point = other_obj->handles[j]->connected_to;

	    if (con_point && (con_point->object == obj)) {
	      Change *change;
	      connected_list = g_list_previous(connected_list);
	      change = undo_unconnect(dia, other_obj,
					      other_obj->handles[j]);
	      (change->apply)(change, dia);
	      if (connected_list == NULL)
		connected_list = obj->connections[i]->connected;
	    }
	  }
	}
	
	connected_list = g_list_next(connected_list);
      }
    }
    
    list = g_list_next(list);
  }
}

void
diagram_remove_all_selected(Diagram *diagram, int delete_empty)
{
  object_add_updates_list(diagram->data->selected, diagram);
	
  data_remove_all_selected(diagram->data);
  
  remove_focus();
}

void
diagram_unselect_object(Diagram *diagram, Object *obj)
{
  object_add_updates(obj, diagram);
  
  data_unselect(diagram->data, obj);
  
  if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
    remove_focus();
  }
}

void
diagram_unselect_objects(Diagram *dia, GList *obj_list)
{
  GList *list;
  Object *obj;

  list = obj_list;
  while (list != NULL) {
    obj = (Object *) list->data;

    if (g_list_find(dia->data->selected, obj) != NULL){
      diagram_unselect_object(dia, obj);
    }

    list = g_list_next(list);
  }
}

void
diagram_select(Diagram *diagram, Object *obj)
{
  data_select(diagram->data, obj);
  object_add_updates(obj, diagram);
}

void
diagram_select_list(Diagram *dia, GList *list)
{

  while (list != NULL) {
    Object *obj = (Object *)list->data;

    diagram_select(dia, obj);

    list = g_list_next(list);
  }
}

int
diagram_is_selected(Diagram *diagram, Object *obj)
{
  return g_list_find(diagram->data->selected, obj) != NULL;
}

void
diagram_redraw_all()
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    diagram_add_update_all(dia);
    diagram_flush(dia);

    list = g_list_next(list);
  }
  return;
}

void
diagram_add_update_all(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;
  
  l = dia->displays;
  while(l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_all(ddisp);
    
    l = g_slist_next(l);
  }
}
void
diagram_add_update(Diagram *dia, Rectangle *update)
{
  GSList *l;
  DDisplay *ddisp;
  
  l = dia->displays;
  while(l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update(ddisp, update);
    
    l = g_slist_next(l);
  }
}

void
diagram_add_update_pixels(Diagram *dia, Point *point,
			  int pixel_width, int pixel_height)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while(l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_add_update_pixels(ddisp, point, pixel_width, pixel_height);
    
    l = g_slist_next(l);
  }
}

void
diagram_flush(Diagram *dia)
{
  GSList *l;
  DDisplay *ddisp;

  l = dia->displays;
  while(l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_flush(ddisp);
    
    l = g_slist_next(l);
  }
}

Object *
diagram_find_clicked_object(Diagram *dia, Point *pos,
			    real maxdist)
{
  return layer_find_closest_object(dia->data->active_layer, pos, maxdist);
}

/*
 * Always returns the last handle in an object that has
 * the closest distance
 */
real
diagram_find_closest_handle(Diagram *dia, Handle **closest,
			    Object **object, Point *pos)
{
  GList *l;
  Object *obj;
  Handle *handle;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Realy big value... */
  
  *closest = NULL;
  
  l = dia->data->selected;
  while(l!=NULL) {
    obj = (Object *) l->data;

    for (i=0;i<obj->num_handles;i++) {
      handle = obj->handles[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &handle->pos);
      if (dist<=mindist) { 
	mindist = dist;
	*closest = handle;
	*object = obj;
      }
    }
    
    l = g_list_next(l);
  }

  return mindist;
}

real
diagram_find_closest_connectionpoint(Diagram *dia,
				     ConnectionPoint **closest,
				     Point *pos)
{
  return layer_find_closest_connectionpoint(dia->data->active_layer,
					    closest, pos);
}

void
diagram_update_extents(Diagram *dia)
{
  gfloat cur_scale = dia->data->paper.scaling;
  /* anropar update_scrollbars() */

  if (layer_update_extents(dia->data->active_layer)) {
    if (data_update_extents(dia->data)) {
      /* Update scrollbars because extents were changed: */
      GSList *l;
      DDisplay *ddisp;
      
      l = dia->displays;
      while(l!=NULL) {
	ddisp = (DDisplay *) l->data;
	
	ddisplay_update_scrollbars(ddisp);
	
	l = g_slist_next(l);
      }
      if (cur_scale != dia->data->paper.scaling) {
	diagram_add_update_all(dia);
	diagram_flush(dia);
      }
    }
  }
}

/* Remove connections from obj to objects outside created group. */
static void
strip_connections(Object *obj, GList *not_strip_list, Diagram *dia)
{
  int i;
  Handle *handle;
  Change *change;

  for (i=0;i<obj->num_handles;i++) {
    handle = obj->handles[i];
    if ((handle->connected_to != NULL) &&
	(g_list_find(not_strip_list, handle->connected_to->object)==NULL)) {
      change = undo_unconnect(dia, obj, handle);
      (change->apply)(change, dia);
    }
  }
}

void diagram_group_selected(Diagram *dia)
{
  GList *list;
  GList *group_list;
  Object *group;
  Object *obj;
  GList *orig_list;


  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  /* We have to rebuild the selection list so that it is the same
     order as in the Diagram list. */
  group_list = diagram_get_sorted_selected_remove(dia);
  
  list = group_list;
  while (list != NULL) {
    obj = (Object *)list->data;

    /* Remove connections from obj to objects outside created group. */
    strip_connections(obj, dia->data->selected, dia);
    
    /* Have to hide any open properties dialog
     * if it contains some object in cut_list */
    properties_hide_if_shown(dia, obj);

    /* Remove focus if active */
    if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
      remove_focus();
    }

    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  data_remove_all_selected(dia->data);
  
  group = group_create(group_list);
  diagram_add_object(dia, group);
  diagram_select(dia, group);

  undo_group_objects(dia, group_list, group, orig_list);
  
  diagram_modified(dia);
  diagram_flush(dia);

  undo_set_transactionpoint(dia->undo);
}

void diagram_ungroup_selected(Diagram *dia)
{
  Object *group;
  GList *group_list;
  GList *list;
  int group_index;
  
  if (dia->data->selected_count != 1) {
    message_error("Trying to ungroup with more or less that one selected object.");
    return;
  }
  
  group = (Object *)dia->data->selected->data;

  if (IS_GROUP(group)) {
    diagram_unselect_object(dia, group);

    group_index = layer_object_index(dia->data->active_layer, group);
    
    group_list = group_objects(group);
    list = group_list;
    while (list != NULL) {
      Object *obj = (Object *)list->data;
      object_add_updates(obj, dia);
      diagram_select(dia, obj);

      list = g_list_next(list);
    }

    layer_replace_object_with_list(dia->data->active_layer,
				   group, g_list_copy(group_list));

    undo_ungroup_objects(dia, group_list, group, group_index);
    properties_hide_if_shown(dia, group);

    diagram_modified(dia);
  }
  
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

GList *
diagram_get_sorted_selected(Diagram *dia)
{
  return data_get_sorted_selected(dia->data);
}

/* Removes selected from objects list, NOT selected list! */
GList *
diagram_get_sorted_selected_remove(Diagram *dia)
{
  diagram_modified(dia);

  return data_get_sorted_selected_remove(dia->data);
}

void
diagram_place_under_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);

  sorted_list = diagram_get_sorted_selected_remove(dia);
  object_add_updates_list(sorted_list, dia);
  layer_add_objects_first(dia->data->active_layer, sorted_list);
  
  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_place_over_selected(Diagram *dia)
{
  GList *sorted_list;
  GList *orig_list;

  if (dia->data->selected_count == 0)
    return;

  orig_list = g_list_copy(dia->data->active_layer->objects);
  
  sorted_list = diagram_get_sorted_selected_remove(dia);
  object_add_updates_list(sorted_list, dia);
  layer_add_objects(dia->data->active_layer, sorted_list);
  
  undo_reorder_objects(dia, g_list_copy(sorted_list), orig_list);

  diagram_modified(dia);
  diagram_flush(dia);
  undo_set_transactionpoint(dia->undo);
}

void
diagram_set_filename(Diagram *dia, char *filename)
{
  GSList *l;
  DDisplay *ddisp;
  char *title;
  
  g_free(dia->filename);
  dia->filename = strdup(filename);


  title = strrchr(filename, G_DIR_SEPARATOR);
  if (title==NULL) {
    title = filename;
  } else {
    title++;
  }
  
  l = dia->displays;
  while(l!=NULL) {
    ddisp = (DDisplay *) l->data;

    ddisplay_set_title(ddisp, title);
    
    l = g_slist_next(l);
  }

  layer_dialog_update_diagram_list();
  recent_file_history_add((const char *)filename, NULL);
}


void
diagram_import_from_xfig(Diagram *dia, char *filename)
{
  printf("Fooled you!  Can't import %s yet!\n", filename);
}

int diagram_modified_exists(void)
{
  GList *list;
  Diagram *dia;

  list = open_diagrams;

  while (list != NULL) {
    dia = (Diagram *) list->data;

    if (dia->modified)
      return TRUE;

    list = g_list_next(list);
  }
  return FALSE;
}


