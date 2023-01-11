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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "select.h"
#include "menus.h"
#include "object_ops.h"
#include "textedit.h"
#include "object.h"
#include "dia-layer.h"

SelectionStyle selection_style = SELECT_REPLACE;

void
select_all_callback(GtkAction *action)
{
  Diagram *dia;
  GList *objects;
  DDisplay * ddisp = ddisplay_active();

  if (!ddisp || textedit_mode(ddisp)) return;
  dia = ddisp->diagram;

  objects = dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)));

  while (objects != NULL) {
    DiaObject *obj = DIA_OBJECT (objects->data);

    if (!diagram_is_selected (dia, obj)) {
      diagram_select (dia, obj);
    }

    objects = g_list_next (objects);
  }

  ddisplay_do_update_menu_sensitivity (ddisp);
  object_add_updates_list (dia->data->selected, dia);
  diagram_flush (dia);
}

void
select_none_callback(GtkAction *action)
{
  Diagram * dia;
  DDisplay * ddisp = ddisplay_active();

  if (!ddisp || textedit_mode(ddisp)) return;
  dia = ddisp->diagram;

  diagram_remove_all_selected(dia, TRUE);

  ddisplay_do_update_menu_sensitivity(ddisp);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_invert_callback (GtkAction *action)
{
  Diagram *dia;
  GList *tmp;
  DDisplay * ddisp = ddisplay_active ();

  if (!ddisp || textedit_mode (ddisp)) return;
  dia = ddisp->diagram;

  tmp =  dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)));

  for (; tmp != NULL; tmp = g_list_next (tmp)) {
    DiaObject *obj = DIA_OBJECT (tmp->data);

    if (!diagram_is_selected (dia, obj)) {
      diagram_select (dia, obj);
    } else {
      diagram_unselect_object (dia, obj);
    }
  }

  ddisplay_do_update_menu_sensitivity (ddisp);
  object_add_updates_list (dia->data->selected, dia);
  diagram_flush (dia);


}


/**
 * select_connected_callback:
 * @action: the #GtkAction
 *
 * Select objects that are directly connected to the currently selected
 * objects, but only in the active layer.
 *
 * Since: dawn-of-time
 */
void
select_connected_callback (GtkAction *action)
{
  Diagram *dia;
  DDisplay * ddisp = ddisplay_active ();
  GList *objects, *tmp;

  if (!ddisp || textedit_mode(ddisp)) return;
  dia = ddisp->diagram;

  objects = dia->data->selected;

  for (tmp = objects; tmp != NULL; tmp = g_list_next(tmp)) {
    DiaObject *obj = (DiaObject *)tmp->data;
    int i;

    for (i = 0; i < obj->num_handles; i++) {
      Handle *handle = obj->handles[i];

      if (handle->connected_to != NULL
	  && dia_object_is_selectable(handle->connected_to->object)
	  && !diagram_is_selected(dia, handle->connected_to->object)) {
	diagram_select(dia, handle->connected_to->object);
      }
    }
  }

  for (tmp = objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = (DiaObject *)tmp->data;
    int i;

    for (i = 0; i < dia_object_get_num_connections(obj); i++) {
      ConnectionPoint *connection = obj->connections[i];
      GList *conns = connection->connected;

      for (; conns != NULL; conns = g_list_next(conns)) {
        DiaObject *obj2 = (DiaObject *)conns->data;

	if (dia_object_is_selectable(obj2)
	    && !diagram_is_selected(dia, obj2)) {
          diagram_select(dia, obj2);
	}
      }
    }
  }

  ddisplay_do_update_menu_sensitivity(ddisp);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

static void
select_transitively(Diagram *dia, DiaObject *obj)
{
  int i;
  /* Do breadth-first to avoid overly large stack */
  GList *newly_selected = NULL;

  for (i = 0; i < obj->num_handles; i++) {
    Handle *handle = obj->handles[i];

    if (handle->connected_to != NULL &&
	dia_object_is_selectable(handle->connected_to->object)) {
      DiaObject *connected_object = handle->connected_to->object;
      if (!diagram_is_selected(dia, connected_object)) {
	diagram_select(dia, connected_object);
	newly_selected = g_list_prepend(newly_selected, connected_object);
      }
    }
  }

  for (i = 0; i < dia_object_get_num_connections(obj); i++) {
    ConnectionPoint *connection = obj->connections[i];
    GList *conns = connection->connected;

    for (; conns != NULL; conns = g_list_next(conns)) {
      DiaObject *obj2 = (DiaObject *)conns->data;

      if (dia_object_is_selectable(obj2) && !diagram_is_selected(dia, obj2)) {
	diagram_select(dia, obj2);
	newly_selected = g_list_prepend(newly_selected, obj2);
      }
    }
  }

  while (newly_selected != NULL) {
    select_transitively(dia, (DiaObject *)newly_selected->data);
    newly_selected = g_list_next(newly_selected);
  }
}


/**
 * select_transitive_callback:
 * @action: the #GtkAction
 *
 * Select objects that are in any way connected with a currently selected
 * object, but only in the active layer.
 *
 * Since: dawn-of-time
 */
void
select_transitive_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  Diagram *dia;
  GList *objects, *tmp;

  if (!ddisp || textedit_mode(ddisp)) return;
  dia = ddisp->diagram;

  objects = dia->data->selected;

  for (tmp = objects; tmp != NULL; tmp = g_list_next(tmp)) {
    select_transitively(dia, (DiaObject *)tmp->data);
  }

  ddisplay_do_update_menu_sensitivity(ddisp);
  object_add_updates_list(dia->data->selected, dia);
  diagram_flush(dia);
}

void
select_same_type_callback (GtkAction *action)
{
  /* For now, do a brute force version:  Check vs. all earlier selected.
     Later, we should really sort the selecteds first to avoid n^2 */
  DDisplay *ddisp = ddisplay_active ();
  Diagram *dia;
  GList *objects, *tmp, *tmp2;

  if (!ddisp || textedit_mode (ddisp)) return;
  dia = ddisp->diagram;

  tmp = dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)));

  objects = dia->data->selected;

  for (; tmp != NULL; tmp = g_list_next (tmp)) {
    DiaObject *obj = (DiaObject *)tmp->data;

    if (!diagram_is_selected (dia, obj)) {
      for (tmp2 = objects; tmp2 != NULL; tmp2 = g_list_next (tmp2)) {
        DiaObject *obj2 = DIA_OBJECT (tmp2->data);

        if (obj->type == obj2->type) {
          diagram_select (dia, obj);
          break;
        }
      }
    }
  }

  ddisplay_do_update_menu_sensitivity (ddisp);
  object_add_updates_list (dia->data->selected, dia);
  diagram_flush (dia);
}

void
select_style_callback(GtkAction *action, GtkRadioAction *current, gpointer user_data)
{
  DDisplay *ddisp = ddisplay_active();
  if (!ddisp || textedit_mode(ddisp)) return;

  /* simply set the selection style to the value of `action' */
  selection_style = gtk_radio_action_get_current_value (current);
}
