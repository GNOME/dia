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

/* select.c -- callbacks to implement the Select menu */
#include <assert.h>

#include "select.h"
#include "menus.h"
#include "intl.h"
#include "object_ops.h"

enum SelectionStyle selection_style = SELECT_REPLACE;

void
select_all_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia;
  GList *objects;

  dia = ddisplay_active()->diagram;

  objects = dia->data->active_layer->objects;

  while (objects != NULL) {
    Object *obj = (Object *)objects->data;
    
    if (!diagram_is_selected(dia, obj))
      diagram_select(dia, obj);
    
    objects = g_list_next(objects);
  }

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_none_callback(GtkWidget *widget, gpointer data)
{
  Diagram * dia = ddisplay_active()->diagram;

  diagram_remove_all_selected(dia, TRUE);

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_invert_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia;
  GList *tmp;

  dia = ddisplay_active()->diagram;

  tmp = dia->data->active_layer->objects;

  for (; tmp != NULL; tmp = g_list_next(tmp)) {
    Object *obj = (Object *)tmp->data;
    
    if (!diagram_is_selected(dia, obj))
      diagram_select(dia, obj);
    else
      diagram_unselect_object(dia, obj);
  }

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
  
}

void
select_connected_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia = ddisplay_active()->diagram;
  GList *objects, *tmp;

  objects = dia->data->selected;

  for (tmp = objects; tmp != NULL; tmp = g_list_next(tmp)) {
    Object *obj = (Object *)tmp->data;
    int i;

    for (i = 0; i < obj->num_handles; i++) {
      Handle *handle = obj->handles[i];
      
      if (handle->connected_to != NULL) {
	if (!diagram_is_selected(dia, handle->connected_to->object)) {
          diagram_select(dia, handle->connected_to->object);
	}
      }      
    }
  }

  for (tmp = objects; tmp != NULL; tmp = tmp->next) {
    Object *obj = (Object *)tmp->data;
    int i;

    for (i = 0; i < obj->num_connections; i++) {
      ConnectionPoint *connection = obj->connections[i];
      GList *conns = connection->connected;

      for (; conns != NULL; conns = g_list_next(conns)) {
        Object *obj2 = (Object *)conns->data;

	if (!diagram_is_selected(dia, obj2)) {
          diagram_select(dia, obj2);
	}
      }      
    }
  }

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

static void
select_transitively(Diagram *dia, Object *obj)
{
  int i;
  /* Do breadth-first to avoid overly large stack */
  GList *newly_selected = NULL;

  for (i = 0; i < obj->num_handles; i++) {
    Handle *handle = obj->handles[i];
    
    if (handle->connected_to != NULL) {
      if (!diagram_is_selected(dia, handle->connected_to->object)) {
	diagram_select(dia, handle->connected_to->object);
	newly_selected = g_list_prepend(newly_selected, handle->connected_to->object);
      }
    }      
  }

  for (i = 0; i < obj->num_connections; i++) {
    ConnectionPoint *connection = obj->connections[i];
    GList *conns = connection->connected;

    for (; conns != NULL; conns = g_list_next(conns)) {
      Object *obj2 = (Object *)conns->data;

      if (!diagram_is_selected(dia, obj2)) {
	diagram_select(dia, obj2);
	newly_selected = g_list_prepend(newly_selected, obj2);
      }
    }
  }
  
  while (newly_selected != NULL) {
    select_transitively(dia, (Object *)newly_selected->data);
    newly_selected = g_list_next(newly_selected);
  }
}

void
select_transitive_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia = ddisplay_active()->diagram;
  GList *objects, *tmp;

  objects = dia->data->selected;

  for (tmp = objects; tmp != NULL; tmp = g_list_next(tmp)) {
    select_transitively(dia, (Object *)tmp->data);
  }

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_same_type_callback(GtkWidget *widget, gpointer data)
{
  /* For now, do a brute force version:  Check vs. all earlier selected.
     Later, we should really sort the selecteds first to avoid n^2 */
  Diagram *dia;
  GList *objects, *tmp, *tmp2;

  dia = ddisplay_active()->diagram;

  tmp = dia->data->active_layer->objects;

  objects = dia->data->selected;

  for (; tmp != NULL; tmp = g_list_next(tmp)) {
    Object *obj = (Object *)tmp->data;

    if (!diagram_is_selected(dia, obj)) {
      for (tmp2 = objects; tmp2 != NULL; tmp2 = g_list_next(tmp2)) {
        Object *obj2 = (Object *)tmp2->data;
       
        if (obj->type == obj2->type) {
          diagram_select(dia, obj);
          break;
        }
      }
    }
  }

  diagram_update_menu_sensitivity(dia);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_style_callback(GtkWidget *widget, gpointer data)
{
  static GtkWidget *item;
  
#ifdef GNOME
  if (ddisplay_active () == NULL) return;
#endif

  item = menus_get_item_from_path("<Display>/Select/Replace");
  if (item==widget)
    {
      selection_style = SELECT_REPLACE;
      return;
    }  
  
  item = menus_get_item_from_path("<Display>/Select/Union");
  if (item==widget)
    {
      selection_style = SELECT_UNION;
      return;
    }  
  
  item = menus_get_item_from_path( "<Display>/Select/Intersect");
  if (item==widget)
    {
      selection_style = SELECT_INTERSECTION;
      return;
    }  
  
  item = menus_get_item_from_path("<Display>/Select/Remove");
  if (item==widget)
    {
      selection_style = SELECT_REMOVE;
      return;
    }  
  
  item = menus_get_item_from_path("<Display>/Select/Invert");
  if (item==widget)
    {
      selection_style = SELECT_INVERT;
      return;
    }  
}

