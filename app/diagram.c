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
#include "layer_dialog.h"

GList *open_diagrams = NULL;

Diagram *
new_diagram(char *filename)  /* Note: filename is copied */
{
  Diagram *dia = g_new(Diagram, 1);

  dia->data = new_diagram_data();

  dia->filename = g_malloc(strlen(filename)+1);
  strcpy(dia->filename, filename);
  
  dia->unsaved = TRUE;
  dia->modified = FALSE;

  dia->display_count = 0;
  dia->displays = NULL;

  open_diagrams = g_list_prepend(open_diagrams, dia);
  layer_dialog_update_diagram_list();
  return dia;
}

void
diagram_destroy(Diagram *dia)
{
  assert(dia->displays==NULL);

  diagram_data_destroy(dia->data);
  
  g_free(dia->filename);

  open_diagrams = g_list_remove(open_diagrams, dia);
  layer_dialog_update_diagram_list();
  
  g_free(dia);
}

void
diagram_modified(Diagram *dia)
{
  diagram_set_modified (dia, TRUE);
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
      ddisplay_update_statusbar (display);
      displays = g_slist_next (displays);
    }
  }
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
    diagram_destroy(dia);
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
diagram_remove_all_selected(Diagram *diagram, int delete_empty)
{
  GList *list = diagram->data->selected;

  /* Unselected all, remove any empty selected objects: */
  while (list!=NULL) {
    Object *selected_obj = (Object *) list->data;
    object_add_updates(selected_obj, diagram);
    
    if (delete_empty && selected_obj->ops->is_empty(selected_obj)) {
      /*      printf("removed empty object.\n"); */
      layer_remove_object(diagram->data->active_layer, selected_obj);
      selected_obj->ops->destroy(selected_obj);
      g_free(selected_obj);
    }
    
    list = g_list_next(list);
  }
	
  data_remove_all_selected(diagram->data);
  
  remove_focus();
}

void
diagram_remove_selected(Diagram *diagram, Object *obj)
{
  object_add_updates(obj, diagram);
  
  data_remove_selected(diagram->data, obj);
  
  if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
    remove_focus();
  }

  if (obj->ops->is_empty(obj)) {
    /*    printf("removed empty object.\n"); */
    layer_remove_object(diagram->data->active_layer, obj);
    obj->ops->destroy(obj);
    g_free(obj);
  }
}

void
diagram_add_selected(Diagram *diagram, Object *obj)
{
  data_add_selected(diagram->data, obj);
  object_add_updates(obj, diagram);
}

void
diagram_add_selected_list(Diagram *dia, GList *list)
{

  while (list != NULL) {
    Object *obj = (Object *)list->data;

    diagram_add_selected(dia, obj);

    list = g_list_next(list);
  }
}

int
diagram_is_selected(Diagram *diagram, Object *obj)
{
  return g_list_find(diagram->data->selected, obj) != NULL;
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
    }
  }
}

/* Remove connections from obj to objects outside created group. */
static void
strip_connections(Object *obj, GList *not_strip_list)
{
  int i;
  Handle *handle;

  for (i=0;i<obj->num_handles;i++) {
    handle = obj->handles[i];
    if ((handle->connected_to != NULL) &&
	(g_list_find(not_strip_list, handle->connected_to->object)==NULL)) {
      object_unconnect(obj, handle);
    }
  }
}

void diagram_group_selected(Diagram *dia)
{
  GList *list;
  GList *group_list;
  Object *group;
  Object *obj;
  
  /* We have to rebuild the selection list so that it is the same
     order as in the Diagram list. */
  group_list = diagram_get_sorted_selected_remove(dia);
  
  list = group_list;
  while (list != NULL) {
    obj = (Object *)list->data;
    strip_connections(obj, dia->data->selected);
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  data_remove_all_selected(dia->data);
  
  group = group_create(group_list);
  diagram_add_object(dia, group);
  diagram_add_selected(dia, group);

  diagram_modified(dia);
  diagram_flush(dia);
}

void diagram_ungroup_selected(Diagram *dia)
{
  Object *group;
  GList *group_list;
  GList *list;
  
  if (dia->data->selected_count != 1) {
    message_error(_("Trying to ungroup with more or less that one selected object."));
    return;
  }
  
  group = (Object *)dia->data->selected->data;

  if (IS_GROUP(group)) {
    diagram_remove_selected(dia, group);
    
    group_list = group_objects(group);
    list = group_list;
    while (list != NULL) {
      Object *obj = (Object *)list->data;
      object_add_updates(obj, dia);
      diagram_add_selected(dia, obj);

      list = g_list_next(list);
    }

    layer_replace_object_with_list(dia->data->active_layer,
				   group, group_list);
    
    group_destroy_shallow(group);

    diagram_modified(dia);
  }
  
  diagram_flush(dia);
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
  GList *list;
  GList *sorted_list;
  Object *obj;

  if (dia->data->selected_count == 0)
    return;

  sorted_list = diagram_get_sorted_selected_remove(dia);
  
  list = sorted_list;
  while (list != NULL) {
    obj = (Object *)list->data;
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  layer_add_objects_first(dia->data->active_layer, sorted_list);
  
  diagram_modified(dia);
  diagram_flush(dia);
}

void
diagram_place_over_selected(Diagram *dia)
{
  GList *list;
  GList *sorted_list;
  Object *obj;

  if (dia->data->selected_count == 0)
    return;

  sorted_list = diagram_get_sorted_selected_remove(dia);
  
  list = sorted_list;
  while (list != NULL) {
    obj = (Object *)list->data;
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  layer_add_objects(dia->data->active_layer, sorted_list);

  diagram_modified(dia);
  diagram_flush(dia);
}

void
diagram_set_filename(Diagram *dia, char *filename)
{
  GSList *l;
  DDisplay *ddisp;
  char *title;
  
  g_free(dia->filename);
  dia->filename = strdup(filename);


  title = strrchr(filename, '/');
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
}


void
diagram_export_to_eps(Diagram *dia, char *filename)
{
  RendererEPS *renderer;
 
  renderer = new_eps_renderer(dia, filename);

  data_render(dia->data, (Renderer *)renderer, NULL, NULL);
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


