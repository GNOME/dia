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
#include <assert.h>
#include <string.h>

#include "diagram.h"
#include "group.h"
#include "object_ops.h"
#include "render_eps.h"
#include "focus.h"
#include "message.h"


GList *open_diagrams = NULL;

Diagram *
new_diagram(char *filename)  /* Note: filename is copied */
{
  Diagram *dia = g_new(Diagram, 1);

  dia->extents.left = 0.0;
  dia->extents.right = 10.0;
  dia->extents.top = 0.0;
  dia->extents.bottom = 10.0;

  dia->bg_color = color_white;

  dia->filename = g_malloc(strlen(filename)+1);
  strcpy(dia->filename, filename);
  
  dia->unsaved = TRUE;
  dia->modified = FALSE;

  dia->objects = NULL;
  dia->selected_count = 0;
  dia->selected = NULL;
  dia->display_count = 0;
  dia->displays = NULL;

  open_diagrams = g_list_prepend(open_diagrams, dia);
  
  return dia;
}

void
diagram_destroy(Diagram *dia)
{
  assert(dia->displays==NULL);
  g_list_free(dia->selected);
  dia->selected = NULL; /* for safety */

  g_free(dia->filename);

  object_destroy_list(dia->objects);

  open_diagrams = g_list_remove(open_diagrams, dia);
  
  g_free(dia);
}

void
diagram_modified(Diagram *dia)
{
  dia->modified = TRUE;
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
  dia->objects = g_list_append(dia->objects, (gpointer) obj);

  diagram_modified(dia);
}

void
diagram_add_object_list(Diagram *dia, GList *list)
{
  dia->objects = g_list_concat(dia->objects, list);

  diagram_modified(dia);
}

void
diagram_remove_all_selected(Diagram *diagram)
{
  GList *list = diagram->selected;
  
  while (list!=NULL) {
    Object *selected_obj = (Object *) list->data;
    object_add_updates(selected_obj, diagram);
    
    if (selected_obj->ops->is_empty(selected_obj)) {
      printf("removed empty object.\n");
      diagram->objects = g_list_remove(diagram->objects, selected_obj);
      selected_obj->ops->destroy(selected_obj);
      g_free(selected_obj);
    }
    
    list = g_list_next(list);
  }
	
  g_list_free(diagram->selected); /* Remove previous selection */
  diagram->selected_count = 0;
  diagram->selected = NULL;

  remove_focus();
}

void
diagram_remove_selected(Diagram *diagram, Object *obj)
{
  object_add_updates(obj, diagram);
  diagram->selected = g_list_remove(diagram->selected, obj);
  diagram->selected_count--;

  if ((active_focus()!=NULL) && (active_focus()->obj == obj)) {
    remove_focus();
  }

  if (obj->ops->is_empty(obj)) {
    printf("removed empty object.\n");
    diagram->objects = g_list_remove(diagram->objects, obj);
    obj->ops->destroy(obj);
    g_free(obj);
  }
}

void
diagram_add_selected(Diagram *diagram, Object *obj)
{
  diagram->selected = g_list_prepend(diagram->selected, obj);
  object_add_updates(obj, diagram);
  diagram->selected_count++;
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
  return g_list_find(diagram->selected, obj) != NULL;
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
  GList *l;
  Object *closest;
  Object *obj;
  real dist;
  
  closest = NULL;
  
  l = dia->objects;
  while(l!=NULL) {
    obj = (Object *) l->data;

    /* Check bounding box here too. Might give speedup. */
    dist = obj->ops->distance_from(obj, pos);

    if (dist<=maxdist) {
      closest = obj;
    }
    
    l = g_list_next(l);
  }

  return closest;
}

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
  
  l = dia->selected;
  while(l!=NULL) {
    obj = (Object *) l->data;

    for (i=0;i<obj->num_handles;i++) {
      handle = obj->handles[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &handle->pos);
      if (dist<mindist) { 
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
  GList *l;
  Object *obj;
  ConnectionPoint *cp;
  real mindist, dist;
  int i;

  mindist = 1000000.0; /* Realy big value... */
  
  *closest = NULL;
  
  l = dia->objects;
  while(l!=NULL) {
    obj = (Object *) l->data;

    for (i=0;i<obj->num_connections;i++) {
      cp = obj->connections[i];
      /* Note: Uses manhattan metric for speed... */
      dist = distance_point_point_manhattan(pos, &cp->pos);
      if (dist<mindist) {
	mindist = dist;
	*closest = cp;
      }
    }
    
    l = g_list_next(l);
  }

  return mindist;
  
}

void
diagram_update_extents(Diagram *dia)
{
  /* anropar update_scrollbars() */
  GList *l;
  Object *obj;
  Rectangle new_extents;
  Rectangle *extents;
  
  extents = &dia->extents;

  l = dia->objects;
  if (l!=NULL) {
    obj = (Object *) l->data;
    new_extents = obj->bounding_box;
    l = g_list_next(l);
  
    while(l!=NULL) {
      obj = (Object *) l->data;
      rectangle_union(&new_extents, &obj->bounding_box);
      l = g_list_next(l);
    }
  }

  /* Update scrollbars if extents were changed */
  if ( (new_extents.left != extents->left) ||
       (new_extents.right != extents->right) ||
       (new_extents.top != extents->top) ||
       (new_extents.bottom != extents->bottom) ) {
    GSList *l2;
    DDisplay *ddisp;

    *extents = new_extents;

    l2 = dia->displays;
    while(l2!=NULL) {
      ddisp = (DDisplay *) l2->data;
      
      ddisplay_update_scrollbars(ddisp);
      
      l2 = g_slist_next(l2);
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
    strip_connections(obj, dia->selected);
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  g_list_free(dia->selected);
  dia->selected = NULL;
  dia->selected_count = 0;
  
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
  
  if (dia->selected_count != 1) {
    message_error("Trying to ungroup with more or less that one selected object.");
    return;
  }
  
  group = (Object *)dia->selected->data;

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
    
    list = g_list_find(dia->objects, group);
    if (list->prev == NULL) {
      dia->objects = group_list;
    } else {
      list->prev->next = group_list;
      group_list->prev = list->prev;
    }
    if (list->next != NULL) {
      GList *last;
      last = g_list_last(group_list);
      last->next = list->next;
      list->next->prev = last;
    }
    g_list_free_1(list);
    
    group_destroy_shallow(group);

    diagram_modified(dia);
  }
  
  diagram_flush(dia);
}

GList *
diagram_get_sorted_selected(Diagram *dia)
{
  GList *list;
  GList *sorted_list;
  GList *found;
  Object *obj;

  if (dia->selected_count == 0)
    return NULL;
  
  sorted_list = NULL;
  list = g_list_last(dia->objects);
  while (list != NULL) {
    found = g_list_find(dia->selected, list->data);
    if (found) {
      obj = (Object *)found->data;
      sorted_list = g_list_prepend(sorted_list, obj);
    }
    list = g_list_previous(list);
  }

  return sorted_list;
}

/* Removes selected from objects list, NOT selected list! */
GList *
diagram_get_sorted_selected_remove(Diagram *dia)
{
  GList *list,*tmp;
  GList *sorted_list;
  GList *found;
  Object *obj;
  
  if (dia->selected_count == 0)
    return NULL;
  
  sorted_list = NULL;
  list = g_list_last(dia->objects);
  while (list != NULL) {
    found = g_list_find(dia->selected, list->data);
    if (found) {
      obj = (Object *)found->data;
      sorted_list = g_list_prepend(sorted_list, obj);

      tmp = list;
      list = g_list_previous(list);
      dia->objects = g_list_remove_link(dia->objects, tmp);
    } else {
      list = g_list_previous(list);
    }
  }

  diagram_modified(dia);

  return sorted_list;
}

void
diagram_place_under_selected(Diagram *dia)
{
  GList *list;
  GList *sorted_list;
  Object *obj;

  if (dia->selected_count == 0)
    return;

  sorted_list = diagram_get_sorted_selected_remove(dia);
  
  list = sorted_list;
  while (list != NULL) {
    obj = (Object *)list->data;
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  dia->objects = g_list_concat(sorted_list, dia->objects);
  
  diagram_modified(dia);
  diagram_flush(dia);
}

void
diagram_place_over_selected(Diagram *dia)
{
  GList *list;
  GList *sorted_list;
  Object *obj;

  if (dia->selected_count == 0)
    return;

  sorted_list = diagram_get_sorted_selected_remove(dia);
  
  list = sorted_list;
  while (list != NULL) {
    obj = (Object *)list->data;
    object_add_updates(obj, dia);
    list = g_list_next(list);
  }

  dia->objects = g_list_concat(dia->objects, sorted_list );

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
}


void
diagram_export_to_eps(Diagram *dia, char *filename)
{
  RendererEPS *renderer;
  GList *list;
  Object *obj;
 
  renderer = new_eps_renderer(dia, filename);

  /* Draw all objects: */
  list = dia->objects;
  while (list!=NULL) {
    obj = (Object *) list->data;

    obj->ops->draw(obj, (Renderer *)renderer);

    list = g_list_next(list);
  }

  close_eps_renderer(renderer);
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


