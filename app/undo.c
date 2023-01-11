/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#define G_LOG_DOMAIN "DiaUndo"

#include "config.h"

#include <glib/gi18n-lib.h>

#include "undo.h"
#include "object_ops.h"
#include "properties-dialog.h"
#include "connectionpoint_ops.h"
#include "focus.h"
#include "group.h"
#include "preferences.h"
#include "textedit.h"
#include "parent.h"
#include "dia-layer.h"


void undo_update_menus (UndoStack *stack);

/**
 * DiaTransactionPointChange:
 *
 * Fake #DiaChange used as a marker
 */
struct _DiaTransactionPointChange {
  DiaChange change;
};

DIA_DEFINE_CHANGE (DiaTransactionPointChange, dia_transaction_point_change)


static void
dia_transaction_point_change_apply (DiaChange *self, DiagramData *dia)
{
}


static void
dia_transaction_point_change_revert (DiaChange *self, DiagramData *dia)
{
}


static void
dia_transaction_point_change_free (DiaChange *self)
{
}


static DiaChange *
dia_transaction_point_change_new (void)
{
  return dia_change_new (DIA_TYPE_TRANSACTION_POINT_CHANGE);
}


UndoStack *
new_undo_stack (Diagram *dia)
{
  UndoStack *stack;
  DiaChange *transaction;

  stack = g_new (UndoStack, 1);
  if (stack != NULL){
    stack->dia = dia;
    transaction = dia_transaction_point_change_new ();
    transaction->next = transaction->prev = NULL;
    stack->last_change = transaction;
    stack->current_change = transaction;
    stack->last_save = transaction;
    stack->depth = 0;
  }
  return stack;
}


void
undo_destroy (UndoStack *stack)
{
  undo_clear (stack);

  g_free (stack);
}


static void
undo_remove_redo_info (UndoStack *stack)
{
  DiaChange *change;
  DiaChange *next_change;

  g_debug ("Removing redo info");

  change = stack->current_change->next;
  stack->current_change->next = NULL;
  stack->last_change = stack->current_change;

  while (change != NULL) {
    next_change = change->next;
    dia_change_unref (change);
    change = next_change;
  }
  undo_update_menus (stack);
}


void
undo_push_change (UndoStack *stack, DiaChange *change)
{
  if (stack->current_change != stack->last_change) {
    undo_remove_redo_info (stack);
  }

  g_debug ("Push %s at %d", DIA_CHANGE_TYPE_NAME (change), stack->depth);

  change->prev = stack->last_change;
  change->next = NULL;
  if (stack->last_change) {
    stack->last_change->next = change;
  }
  stack->last_change = change;
  stack->current_change = change;
  undo_update_menus (stack);
}


static void
undo_delete_lowest_transaction (UndoStack *stack)
{
  DiaChange *change;
  DiaChange *next_change;

  /* Find the lowest change: */
  change = stack->current_change;
  while (change->prev != NULL) {
    change = change->prev;
  }

  /* Remove changes from the bottom until (and including)
   * the first transactionpoint.
   * Stop if we reach current_change or NULL.
   */
  do {
    if ((change == NULL) || (change == stack->current_change)) {
      break;
    }

    next_change = change->next;
    g_debug ("freeing one change from the bottom.");
    dia_change_unref (change);
    change = next_change;
  } while (!DIA_IS_TRANSACTION_POINT_CHANGE (change));

  if (DIA_IS_TRANSACTION_POINT_CHANGE (change)) {
    stack->depth--;
    g_debug ("Decreasing stack depth to: %d", stack->depth);
  }

  if (change) {
    /* play safe */
    change->prev = NULL;
  }
}


void
undo_set_transactionpoint (UndoStack *stack)
{
  DiaChange *transaction;

  if (DIA_IS_TRANSACTION_POINT_CHANGE (stack->current_change)) {
    return;
  }

  transaction = dia_transaction_point_change_new ();

  undo_push_change (stack, transaction);
  stack->depth++;
  g_debug ("Increasing stack depth to: %d", stack->depth);

  if (prefs.undo_depth > 0) {
    while (stack->depth > prefs.undo_depth) {
      undo_delete_lowest_transaction (stack);
    }
  }
}


void
undo_revert_to_last_tp (UndoStack *stack)
{
  DiaChange *change;
  DiaChange *prev_change;

  if (stack->current_change->prev == NULL) {
    return; /* Can't revert first transactionpoint */
  }

  change = stack->current_change;
  do {
    prev_change = change->prev;
    dia_change_revert (change, DIA_DIAGRAM_DATA (stack->dia));
    change = prev_change;
  } while (!DIA_IS_TRANSACTION_POINT_CHANGE (change));
  stack->current_change  = change;
  stack->depth--;
  undo_update_menus (stack);
  g_debug ("Decreasing stack depth to: %d", stack->depth);
}


void
undo_apply_to_next_tp (UndoStack *stack)
{
  DiaChange *change;
  DiaChange *next_change;

  change = stack->current_change;

  if (change->next == NULL) {
    return /* Already at top. */;
  }

  do {
    next_change = change->next;
    dia_change_apply (change, DIA_DIAGRAM_DATA (stack->dia));
    change = next_change;
  } while ((change != NULL) && (!DIA_IS_TRANSACTION_POINT_CHANGE (change)));

  if (change == NULL) {
    change = stack->last_change;
  }

  stack->current_change = change;
  stack->depth++;
  undo_update_menus (stack);
  g_debug ("Increasing stack depth to: %d", stack->depth);
}


void
undo_clear (UndoStack *stack)
{
  DiaChange *change;

  g_debug ("undo_clear()");

  change = stack->current_change;

  while (change->prev != NULL) {
    change = change->prev;
  }

  stack->current_change = change;
  stack->depth = 0;
  undo_remove_redo_info (stack);
  undo_update_menus (stack);
}


/**
 * undo_update_menus:
 * @stack: the #UndoStack
 *
 * Make updates to menus associated with undo.
 *
 * Currently just changes sensitivity, but should in the future also
 * include changing the labels.
 *
 * Since: dawn-of-time
 */
void
undo_update_menus (UndoStack *stack)
{
  ddisplay_do_update_menu_sensitivity (ddisplay_active ());
}


/**
 * undo_mark_save:
 * @stack: the #UndoStack
 *
 * Marks the undo stack at the time of a save.
 */
void
undo_mark_save (UndoStack *stack)
{
  stack->last_save = stack->current_change;
}


/**
 * undo_is_saved:
 * @stack: the #UndoStack
 *
 * Returns: %TRUE if the diagram is undo-wise in the same state as at last
 *          save, i.e. the current change is the same as it was at last save.
 */
gboolean
undo_is_saved (UndoStack *stack)
{
  return stack->last_save == stack->current_change;
}


/**
 * undo_available:
 * @stack: the #UndoStack
 * @undo: /shrug
 *
 * If @undo is %TRUE, returns %TRUE if there's something to undo, otherwise
 * returns %TRUE if there's something to redo.
 *
 * Returns: %TRUE if there is an undo or redo item available in the stack.
 *
 * Since: dawn-of-time
 */
gboolean
undo_available (UndoStack *stack, gboolean undo)
{
  if (undo) {
    return stack->current_change != NULL;
  } else {
    return stack->current_change != NULL && stack->current_change->next != NULL;
  }
}


/**
 * undo_remove_to:
 * @stack: The undo stack to remove items from.
 * @type: Indicator of undo type to remove: An apply function.
 *
 * Remove items from the undo stack until we hit an undo item of a given
 *  type (indicated by its apply function).  Beware that the items are not
 *  just reverted, but totally removed.  This also takes with it all the
 *  changes above current_change.
 *
 * Returns: The #DiaChange object that stopped the search, if any was found,
 * or %NULL otherwise.  In the latter case, the undo stack will be empty.
 */
DiaChange *
undo_remove_to (UndoStack *stack, GType type)
{
  DiaChange *current_change = stack->current_change;

  if (current_change == NULL) {
    return NULL;
  }

  while (current_change &&
         !g_type_is_a (DIA_CHANGE_TYPE (current_change), type)) {
    current_change = current_change->prev;
  }

  if (current_change != NULL) {
    stack->current_change = current_change;
    undo_remove_redo_info (stack);
    return current_change;
  } else {
    undo_clear (stack);
    return NULL;
  }
}


/****************************************************************/
/****************************************************************/
/*****************                          *********************/
/***************** Specific undo functions: *********************/
/*****************                          *********************/
/****************************************************************/
/****************************************************************/

/******** Move object list: */

struct _DiaMoveObjectsChange {
  DiaChange change;

  Point *orig_pos;
  Point *dest_pos;
  GList *obj_list;
};

DIA_DEFINE_CHANGE (DiaMoveObjectsChange, dia_move_objects_change)


static void
dia_move_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaMoveObjectsChange *change = DIA_MOVE_OBJECTS_CHANGE (self);
  GList *list;
  int i;
  DiaObject *obj;

  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  list = change->obj_list;
  i = 0;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    dia_object_move (obj, &change->dest_pos[i]);

    list = g_list_next (list);
    i++;
  }

  list = change->obj_list;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    diagram_update_connections_object (DIA_DIAGRAM (dia), obj, TRUE);

    list = g_list_next (list);
  }

  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));
}


static void
dia_move_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaMoveObjectsChange *change = DIA_MOVE_OBJECTS_CHANGE (self);
  GList *list;
  int i;
  DiaObject *obj;

  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  list = change->obj_list;
  i = 0;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    dia_object_move (obj, &change->orig_pos[i]);

    list = g_list_next (list);
    i++;
  }

  list = change->obj_list;
  while (list != NULL) {
    obj = DIA_OBJECT (list->data);

    diagram_update_connections_object (DIA_DIAGRAM (dia), obj, TRUE);

    list = g_list_next (list);
  }

  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));
}


static void
dia_move_objects_change_free (DiaChange *self)
{
  DiaMoveObjectsChange *change = DIA_MOVE_OBJECTS_CHANGE (self);

  g_clear_pointer (&change->orig_pos, g_free);
  g_clear_pointer (&change->dest_pos, g_free);
  g_list_free (change->obj_list);
}


DiaChange *
dia_move_objects_change_new (Diagram *dia,
                             Point   *orig_pos,
                             Point   *dest_pos,
                             GList   *obj_list)
{
  DiaMoveObjectsChange *change = dia_change_new (DIA_TYPE_MOVE_OBJECTS_CHANGE);

  change->orig_pos = orig_pos;
  change->dest_pos = dest_pos;
  change->obj_list = obj_list;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/********** Move handle: */

struct _DiaMoveHandleChange {
  DiaChange change;

  Point orig_pos;
  Point dest_pos;
  Handle *handle;
  DiaObject *obj;

  int modifiers;
};

DIA_DEFINE_CHANGE (DiaMoveHandleChange, dia_move_handle_change)


static void
dia_move_handle_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaMoveHandleChange *change = DIA_MOVE_HANDLE_CHANGE (self);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  dia_object_move_handle (change->obj,
                          change->handle,
                          &change->dest_pos,
                          NULL,
                          HANDLE_MOVE_USER_FINAL,
                          change->modifiers);
  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  diagram_update_connections_object (DIA_DIAGRAM (dia), change->obj, TRUE);
}


static void
dia_move_handle_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaMoveHandleChange *change = DIA_MOVE_HANDLE_CHANGE (self);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  dia_object_move_handle (change->obj,
                          change->handle,
                          &change->orig_pos,
                          NULL,
                          HANDLE_MOVE_USER_FINAL,
                          change->modifiers);
  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  diagram_update_connections_object (DIA_DIAGRAM (dia), change->obj, TRUE);
}


static void
dia_move_handle_change_free (DiaChange *self)
{
}


DiaChange *
dia_move_handle_change_new (Diagram   *dia,
                            Handle    *handle,
                            DiaObject *obj,
                            Point      orig_pos,
                            Point      dest_pos,
                            int        modifiers)
{
  DiaMoveHandleChange *change = dia_change_new (DIA_TYPE_MOVE_HANDLE_CHANGE);

  change->orig_pos = orig_pos;
  change->dest_pos = dest_pos;
  change->handle = handle;
  change->obj = obj;

  change->modifiers = modifiers;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}

/***************** Connect object: */

struct _DiaConnectChange {
  DiaChange change;

  DiaObject *obj;
  Handle *handle;
  ConnectionPoint *connectionpoint;
  Point handle_pos;
};

DIA_DEFINE_CHANGE (DiaConnectChange, dia_connect_change)


static void
dia_connect_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaConnectChange *change = DIA_CONNECT_CHANGE (self);

  object_connect (change->obj, change->handle, change->connectionpoint);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  dia_object_move_handle (change->obj,
                          change->handle,
                          &change->connectionpoint->pos,
                          change->connectionpoint,
                          HANDLE_MOVE_CONNECTED,
                          0);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
}


static void
dia_connect_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaConnectChange *change = DIA_CONNECT_CHANGE (self);

  object_unconnect (change->obj, change->handle);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
  dia_object_move_handle (change->obj,
                          change->handle,
                          &change->handle_pos,
                          NULL,
                          HANDLE_MOVE_CONNECTED,
                          0);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
}


static void
dia_connect_change_free (DiaChange *self)
{
}


DiaChange *
dia_connect_change_new (Diagram         *dia,
                        DiaObject       *obj,
                        Handle          *handle,
                        ConnectionPoint *connectionpoint)
{
  DiaConnectChange *change = dia_change_new (DIA_TYPE_CONNECT_CHANGE);

  change->obj = obj;
  change->handle = handle;
  change->handle_pos = handle->pos;
  change->connectionpoint = connectionpoint;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/*************** Unconnect object: */

struct _DiaUnconnectChange {
  DiaChange change;

  DiaObject *obj;
  Handle *handle;
  ConnectionPoint *connectionpoint;
};

DIA_DEFINE_CHANGE (DiaUnconnectChange, dia_unconnect_change)


static void
dia_unconnect_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaUnconnectChange *change = DIA_UNCONNECT_CHANGE (self);

  object_unconnect (change->obj, change->handle);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
}


static void
dia_unconnect_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaUnconnectChange *change = DIA_UNCONNECT_CHANGE (self);

  object_connect (change->obj, change->handle, change->connectionpoint);

  object_add_updates (change->obj, DIA_DIAGRAM (dia));
}


static void
dia_unconnect_change_free (DiaChange *self)
{
}


DiaChange *
dia_unconnect_change_new (Diagram *dia, DiaObject *obj, Handle *handle)
{
  DiaUnconnectChange *change = dia_change_new (DIA_TYPE_UNCONNECT_CHANGE);

  change->obj = obj;
  change->handle = handle;
  change->connectionpoint = handle->connected_to;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/******** Delete object list: */

struct _DiaDeleteObjectsChange {
  DiaChange change;

  DiaLayer *layer;
  GList *obj_list; /* Owning reference when applied */
  GList *original_objects;
  int applied;
};

DIA_DEFINE_CHANGE (DiaDeleteObjectsChange, dia_delete_objects_change)


static void
dia_delete_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaDeleteObjectsChange *change = DIA_DELETE_OBJECTS_CHANGE (self);
  GList *list;

  g_debug ("delete_objects_apply()");
  change->applied = 1;
  diagram_unselect_objects (DIA_DIAGRAM (dia), change->obj_list);
  dia_layer_remove_objects (change->layer, change->obj_list);
  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  list = change->obj_list;
  while (list != NULL) {
    DiaObject *obj = DIA_OBJECT (list->data);

    /* Have to hide any open properties dialog
       if it contains some object in cut_list */
    properties_hide_if_shown (DIA_DIAGRAM (dia), obj);

    if (obj->parent) {
      /* Lose references to deleted object */
      obj->parent->children = g_list_remove (obj->parent->children, obj);
    }

    list = g_list_next (list);
  }
}


static void
dia_delete_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaDeleteObjectsChange *change = DIA_DELETE_OBJECTS_CHANGE (self);
  GList *list;

  g_debug ("delete_objects_revert()");
  change->applied = 0;
  dia_layer_set_object_list (change->layer,
                             g_list_copy (change->original_objects));
  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  list = change->obj_list;
  while (list) {
    DiaObject *obj = DIA_OBJECT (list->data);
    if (obj->parent) {

      /* Restore child references */
      obj->parent->children = g_list_append (obj->parent->children, obj);
    }
    /* no need to emit object_add signal, already done by layer_set_object_list */
    list = g_list_next (list);
  }
}


static void
dia_delete_objects_change_free (DiaChange *self)
{
  DiaDeleteObjectsChange *change = DIA_DELETE_OBJECTS_CHANGE (self);

  g_debug ("delete_objects_free()");
  if (change->applied) {
    destroy_object_list (change->obj_list);
  } else {
    g_list_free (change->obj_list);
  }
  g_list_free (change->original_objects);
}


/*
  This function deletes specified objects along with any children
  they might have.
  undo_delete_objects() only deletes the objects that are specified.
*/
DiaChange *
dia_delete_objects_change_new_with_children (Diagram *dia, GList *obj_list)
{
  return dia_delete_objects_change_new (dia, parent_list_affected (obj_list));
}


DiaChange *
dia_delete_objects_change_new (Diagram *dia, GList *obj_list)
{
  DiaDeleteObjectsChange *change = dia_change_new (DIA_TYPE_DELETE_OBJECTS_CHANGE);

  change->layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  change->obj_list = obj_list;
  change->original_objects = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));
  change->applied = 0;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/******** Insert object list: */

struct _DiaInsertObjectsChange {
  DiaChange change;

  DiaLayer *layer;
  GList *obj_list; /* Owning reference when not applied */
  int applied;
};

DIA_DEFINE_CHANGE (DiaInsertObjectsChange, dia_insert_objects_change)


static void
dia_insert_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaInsertObjectsChange *change = DIA_INSERT_OBJECTS_CHANGE (self);

  g_debug ("insert_objects_apply()");
  change->applied = 1;
  dia_layer_add_objects (change->layer, g_list_copy (change->obj_list));
  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));
}


static void
dia_insert_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaInsertObjectsChange *change = DIA_INSERT_OBJECTS_CHANGE (self);
  GList *list;

  g_debug ("insert_objects_revert()");
  change->applied = 0;
  diagram_unselect_objects (DIA_DIAGRAM (dia), change->obj_list);
  dia_layer_remove_objects (change->layer, change->obj_list);
  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  list = change->obj_list;
  while (list != NULL) {
    DiaObject *obj = DIA_OBJECT (list->data);

    /* Have to hide any open properties dialog
       if it contains some object in cut_list */
    properties_hide_if_shown (DIA_DIAGRAM (dia), obj);

    list = g_list_next (list);
  }
}


static void
dia_insert_objects_change_free (DiaChange *self)
{
  DiaInsertObjectsChange *change = DIA_INSERT_OBJECTS_CHANGE (self);

  g_debug ("insert_objects_free()");
  if (!change->applied) {
    destroy_object_list (change->obj_list);
  } else {
    g_list_free (change->obj_list);
  }
}


DiaChange *
dia_insert_objects_change_new (Diagram *dia, GList *obj_list, int applied)
{
  DiaInsertObjectsChange *change = dia_change_new (DIA_TYPE_INSERT_OBJECTS_CHANGE);

  change->layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  change->obj_list = obj_list;
  change->applied = applied;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/******** Reorder object list: */

struct _DiaReorderObjectsChange {
  DiaChange parent;

  DiaLayer *layer;
  GList *changed_list; /* Owning reference when applied */
  GList *original_objects;
  GList *reordered_objects;
};

DIA_DEFINE_CHANGE (DiaReorderObjectsChange, dia_reorder_objects_change)


static void
dia_reorder_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaReorderObjectsChange *change = DIA_REORDER_OBJECTS_CHANGE (self);

  g_debug ("reorder_objects_apply()");
  dia_layer_set_object_list (change->layer,
                             g_list_copy (change->reordered_objects));
  object_add_updates_list (change->changed_list, DIA_DIAGRAM (dia));
}


static void
dia_reorder_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaReorderObjectsChange *change = DIA_REORDER_OBJECTS_CHANGE (self);

  g_debug ("reorder_objects_revert()");
  dia_layer_set_object_list (change->layer,
                             g_list_copy (change->original_objects));
  object_add_updates_list (change->changed_list, DIA_DIAGRAM (dia));
}


static void
dia_reorder_objects_change_free (DiaChange *self)
{
  DiaReorderObjectsChange *change = DIA_REORDER_OBJECTS_CHANGE (self);

  g_debug ("reorder_objects_free()");
  g_list_free (change->changed_list);
  g_list_free (change->original_objects);
  g_list_free (change->reordered_objects);
}


DiaChange *
dia_reorder_objects_change_new (Diagram *dia, GList *changed_list, GList *orig_list)
{
  DiaReorderObjectsChange *change = dia_change_new (DIA_TYPE_REORDER_OBJECTS_CHANGE);

  change->layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  change->changed_list = changed_list;
  change->original_objects = orig_list;
  change->reordered_objects = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/******** ObjectChange: */

struct _DiaObjectChangeChange {
  DiaChange change;

  DiaObject *obj;
  DiaObjectChange *obj_change;
};

DIA_DEFINE_CHANGE (DiaObjectChangeChange, dia_object_change_change)


static void
_connections_update_func (gpointer data, gpointer user_data)
{
  DiaObject *obj = data;
  DiagramData *dia = DIA_DIAGRAM_DATA (user_data);

  diagram_update_connections_object (DIA_DIAGRAM (dia), obj, TRUE);
}


static void
dia_object_change_change_apply (DiaChange   *self,
                                DiagramData *dia)
{
  DiaObjectChangeChange *change = DIA_OBJECT_CHANGE_CHANGE (self);

  if (change->obj) {
    object_add_updates (change->obj, DIA_DIAGRAM (dia));
  }

  dia_object_change_apply (change->obj_change, change->obj);

  if (change->obj) {
    /* Make sure object updates its data: */
    Point p = change->obj->position;
    dia_object_move (change->obj, &p);

    object_add_updates (change->obj, DIA_DIAGRAM (dia));
    diagram_update_connections_object (DIA_DIAGRAM (dia), change->obj, TRUE);
    properties_update_if_shown (DIA_DIAGRAM (dia), change->obj);
  } else {
    /* pretty big hammer - update all connections */
    data_foreach_object (DIA_DIAGRAM_DATA (dia),
                         _connections_update_func,
                         DIA_DIAGRAM (dia));
    diagram_add_update_all (DIA_DIAGRAM (dia));
  }
}


static void
dia_object_change_change_revert (DiaChange   *self,
                                 DiagramData *dia)
{
  DiaObjectChangeChange *change = DIA_OBJECT_CHANGE_CHANGE (self);

  if (change->obj) {
    object_add_updates (change->obj, DIA_DIAGRAM (dia));
  }

  dia_object_change_revert (change->obj_change, change->obj);

  if (change->obj) {
    /* Make sure object updates its data: */
    Point p = change->obj->position;
    dia_object_move (change->obj, &p);

    object_add_updates(change->obj, DIA_DIAGRAM (dia));
    diagram_update_connections_object (DIA_DIAGRAM (dia), change->obj, TRUE);
    properties_update_if_shown (DIA_DIAGRAM (dia), change->obj);
  } else {
    data_foreach_object (DIA_DIAGRAM_DATA (dia),
                         _connections_update_func,
                         DIA_DIAGRAM (dia));
    diagram_add_update_all (DIA_DIAGRAM (dia));
  }
}


static void
dia_object_change_change_free (DiaChange *self)
{
  DiaObjectChangeChange *change = DIA_OBJECT_CHANGE_CHANGE (self);

  g_debug ("state_change_free()");

  g_clear_pointer (&change->obj_change, dia_object_change_unref);
}


/**
 * dia_object_change_change_new:
 * @dia: the #Diagram the change is from
 * @obj: the #DiaObject in the @dia
 * @change: (transfer full): the #DiaObjectChange to wrap
 */
DiaChange *
dia_object_change_change_new (Diagram         *dia,
                              DiaObject       *obj,
                              DiaObjectChange *change)
{
  DiaObjectChangeChange *self = dia_change_new (DIA_TYPE_OBJECT_CHANGE_CHANGE);

  self->obj = obj;
  self->obj_change = change;

  undo_push_change (dia->undo, DIA_CHANGE (self));

  return DIA_CHANGE (self);
}

/******** Group object list: */

/**
 * DiaGroupObjectsChange:
 *
 * Grouping and ungrouping are two subtly different changes:  While
 * ungrouping preserves the front/back position, grouping cannot do that,
 * since the act of grouping destroys positions for the members of the
 * group, and those positions have to be restored in the undo.
 */

struct _DiaGroupObjectsChange {
  DiaChange parent;

  DiaLayer *layer;
  DiaObject *group;   /* owning reference if not applied */
  GList *obj_list; /* The list of objects in this group.  Owned by the
		      group */
  GList *orig_list; /* A copy of the original list of all objects,
		       from before the group was created.  Owned by
		       the group */
  int applied;
};

DIA_DEFINE_CHANGE (DiaGroupObjectsChange, dia_group_objects_change)


static void
dia_group_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaGroupObjectsChange *change = DIA_GROUP_OBJECTS_CHANGE (self);

  GList *list;

  g_debug ("group_objects_apply()");

  change->applied = 1;

  diagram_unselect_objects (DIA_DIAGRAM (dia), change->obj_list);
  dia_layer_remove_objects (change->layer, change->obj_list);
  dia_layer_add_object (change->layer, change->group);
  object_add_updates (change->group, DIA_DIAGRAM (dia));

  list = change->obj_list;
  while (list != NULL) {
    DiaObject *obj = DIA_OBJECT (list->data);

    /* Have to hide any open properties dialog
       if it contains some object in cut_list */
    properties_hide_if_shown (DIA_DIAGRAM (dia), obj);

    object_add_updates (obj, DIA_DIAGRAM (dia));

    list = g_list_next (list);
  }
}


static void
dia_group_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaGroupObjectsChange *change = DIA_GROUP_OBJECTS_CHANGE (self);

  g_debug ("group_objects_revert()");
  change->applied = 0;

  diagram_unselect_object (DIA_DIAGRAM (dia), change->group);
  object_add_updates (change->group, DIA_DIAGRAM (dia));

  dia_layer_set_object_list (change->layer, g_list_copy (change->orig_list));

  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  properties_hide_if_shown (DIA_DIAGRAM (dia), change->group);
}


static void
dia_group_objects_change_free (DiaChange *self)
{
  DiaGroupObjectsChange *change = DIA_GROUP_OBJECTS_CHANGE (self);

  g_debug ("group_objects_free()");
  if (!change->applied) {
    group_destroy_shallow (change->group);
    change->group = NULL;
    change->obj_list = NULL;
    /** Leak here? */
  }
  g_list_free (change->orig_list);
}


DiaChange *
dia_group_objects_change_new (Diagram   *dia,
                              GList     *obj_list,
                              DiaObject *group,
                              GList     *orig_list)
{
  DiaGroupObjectsChange *change = dia_change_new (DIA_TYPE_GROUP_OBJECTS_CHANGE);

  change->layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  change->group = group;
  change->obj_list = obj_list;
  change->orig_list = orig_list;
  change->applied = 1;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}

/******** Ungroup object list: */


struct _DiaUngroupObjectsChange {
  DiaChange parent;

  DiaLayer *layer;
  DiaObject *group;   /* owning reference if applied */
  GList *obj_list; /* This list is owned by the ungroup. */
  int group_index;
  int applied;
};

DIA_DEFINE_CHANGE (DiaUngroupObjectsChange, dia_ungroup_objects_change)


static void
dia_ungroup_objects_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaUngroupObjectsChange *change = DIA_UNGROUP_OBJECTS_CHANGE (self);

  g_debug ("ungroup_objects_apply()");

  change->applied = 1;

  diagram_unselect_object (DIA_DIAGRAM (dia), change->group);
  object_add_updates (change->group, DIA_DIAGRAM (dia));
  dia_layer_replace_object_with_list (change->layer,
                                      change->group,
                                      g_list_copy (change->obj_list));
  object_add_updates_list (change->obj_list, DIA_DIAGRAM (dia));

  properties_hide_if_shown (DIA_DIAGRAM (dia), change->group);
}


static void
dia_ungroup_objects_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaUngroupObjectsChange *change = DIA_UNGROUP_OBJECTS_CHANGE (self);
  GList *list;

  g_debug ("ungroup_objects_revert()");
  change->applied = 0;


  diagram_unselect_objects (DIA_DIAGRAM (dia), change->obj_list);
  dia_layer_remove_objects (change->layer, change->obj_list);
  dia_layer_add_object_at (change->layer, change->group, change->group_index);
  object_add_updates (change->group, DIA_DIAGRAM (dia));

  list = change->obj_list;
  while (list != NULL) {
    DiaObject *obj = DIA_OBJECT (list->data);

    /* Have to hide any open properties dialog
       if it contains some object in cut_list */
    properties_hide_if_shown (DIA_DIAGRAM (dia), obj);

    list = g_list_next (list);
  }
}


static void
dia_ungroup_objects_change_free (DiaChange *self)
{
  DiaUngroupObjectsChange *change = DIA_UNGROUP_OBJECTS_CHANGE (self);

  g_debug ("ungroup_objects_free()");

  if (change->applied) {
    group_destroy_shallow (change->group);
    change->group = NULL;
    change->obj_list = NULL;
  }
}


DiaChange *
dia_ungroup_objects_change_new (Diagram   *dia,
                                GList     *obj_list,
                                DiaObject *group,
                                int        group_index)
{
  DiaUngroupObjectsChange *change = dia_change_new (DIA_TYPE_UNGROUP_OBJECTS_CHANGE);

  change->layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  change->group = group;
  change->obj_list = obj_list;
  change->group_index = group_index;
  change->applied = 1;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



/******* PARENTING */

struct _DiaParentingChange {
  DiaChange parent_instance;

  DiaObject *parentobj, *childobj;
  gboolean parent;
};

DIA_DEFINE_CHANGE (DiaParentingChange, dia_parenting_change)


/** Performs the actual parenting of a child to a parent.
 * Since no display changes arise from this, we need call no update. */
static void
parent_object (DiagramData *dia, DiaObject *parent, DiaObject *child)
{
  child->parent = parent;
  parent->children = g_list_prepend (parent->children, child);
}


/** Performs the actual removal of a child from a parent.
 * Since no display changes arise from this, we need call no update. */
static void
unparent_object (DiagramData *dia, DiaObject *parent, DiaObject *child)
{
  child->parent = NULL;
  parent->children = g_list_remove (parent->children, child);
}


/** Applies the given ParentChange */
static void
dia_parenting_change_apply (DiaChange *change, DiagramData *dia)
{
  DiaParentingChange *parentchange = DIA_PARENTING_CHANGE (change);

  if (parentchange->parent) {
    parent_object (dia, parentchange->parentobj, parentchange->childobj);
  } else {
    unparent_object (dia, parentchange->parentobj, parentchange->childobj);
  }
}


/** Reverts the given ParentChange */
static void
dia_parenting_change_revert (DiaChange *change, DiagramData *dia)
{
  DiaParentingChange *parentchange = DIA_PARENTING_CHANGE (change);

  if (!parentchange->parent) {
    parent_object (dia, parentchange->parentobj, parentchange->childobj);
  } else {
    unparent_object (dia, parentchange->parentobj, parentchange->childobj);
  }
}


/** Frees items in the change -- none really */
static void
dia_parenting_change_free (DiaChange *change)
{
}


/**
 * dia_parenting_change_new:
 *
 *
 * Create a new #DiaChange object for parenting/unparenting.
 * `parent' is %TRUE if applying this change makes childobj a
 * child of parentobj.
 *
 * Since: 0.98
 */
DiaChange *
dia_parenting_change_new (Diagram   *dia,
                          DiaObject *parentobj,
                          DiaObject *childobj,
                          gboolean   parent)
{
  DiaParentingChange *parentchange = dia_change_new (DIA_TYPE_PARENTING_CHANGE);

  parentchange->parentobj = parentobj;
  parentchange->childobj = childobj;
  parentchange->parent = parent;

  undo_push_change (dia->undo, DIA_CHANGE (parentchange));

  return DIA_CHANGE (parentchange);
}



/* ********* MOVE TO OTHER LAYER  */

struct _DiaMoveObjectToLayerChange {
  DiaChange parent;

  /** The objects we are moving */
  GList *objects;
  /** All objects in the original layer */
  GList *orig_list;
  /** The active layer when started */
  DiaLayer *orig_layer;
  gboolean moving_up;
};

DIA_DEFINE_CHANGE (DiaMoveObjectToLayerChange, dia_move_object_to_layer_change)


/*!
 * BEWARE: we need to notify the DiagramTree somehow - maybe
 * better make it listen to object-add signal?
 */
static void
move_object_layer_relative (DiagramData *dia, GList *objects, int dist)
{
  /* from the active layer to above or below */
  DiaLayer *active, *target;
  guint pos;

  g_return_if_fail (data_layer_count (dia) != 0);

  active = dia_diagram_data_get_active_layer (dia);

  g_return_if_fail (active);

  pos = data_layer_get_index (dia, active);

  pos = (pos + data_layer_count (dia) + dist) % data_layer_count (dia);
  target = data_layer_get_nth (dia, pos);
  object_add_updates_list (objects, DIA_DIAGRAM (dia));
  dia_layer_remove_objects (active, objects);
  dia_layer_add_objects (target, g_list_copy (objects));
  data_set_active_layer (dia, target);
}


static void
dia_move_object_to_layer_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaMoveObjectToLayerChange *change = DIA_MOVE_OBJECT_TO_LAYER_CHANGE (self);

  if (change->moving_up) {
    move_object_layer_relative (dia, change->objects, 1);
  } else {
    move_object_layer_relative (dia, change->objects, -1);
  }
}


static void
dia_move_object_to_layer_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaMoveObjectToLayerChange *change = DIA_MOVE_OBJECT_TO_LAYER_CHANGE (self);

  if (change->moving_up) {
    move_object_layer_relative (dia, change->objects, -1);
  } else {
    diagram_unselect_objects (DIA_DIAGRAM (dia), change->objects);
    move_object_layer_relative (dia, change->objects, 1);
    /* overwriting the 'unsorted' list of objects to the order it had before */
    dia_layer_set_object_list (change->orig_layer, g_list_copy (change->orig_list));
    object_add_updates_list (change->orig_list, DIA_DIAGRAM (dia));
  }
}


static void
dia_move_object_to_layer_change_free (DiaChange *self)
{
  DiaMoveObjectToLayerChange *change = DIA_MOVE_OBJECT_TO_LAYER_CHANGE (self);

  g_list_free (change->objects);
  g_list_free (change->orig_list);

  change->objects = NULL;
  change->orig_list = NULL;
}


DiaChange *
dia_move_object_to_layer_change_new (Diagram  *dia,
                                     GList    *selected_list,
                                     gboolean  moving_up)
{
  DiaMoveObjectToLayerChange *movetolayerchange = dia_change_new (DIA_TYPE_MOVE_OBJECT_TO_LAYER_CHANGE);

  movetolayerchange->orig_layer = dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia));
  movetolayerchange->orig_list = g_list_copy (dia_layer_get_object_list (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia))));
  movetolayerchange->objects = g_list_copy (selected_list);
  movetolayerchange->moving_up = moving_up;

  undo_push_change (dia->undo, DIA_CHANGE (movetolayerchange));

  return DIA_CHANGE (movetolayerchange);
}



struct _DiaImportChange {
  DiaChange parent;

  Diagram *dia;     /*!< the diagram under inspection */
  GList   *layers;  /*!< layers added */
  GList   *objects; /*!< objects added */
};

DIA_DEFINE_CHANGE (DiaImportChange, dia_import_change)


static void
dia_import_change_apply (DiaChange   *self,
                         DiagramData *dia)
{
  DiaImportChange *change = DIA_IMPORT_CHANGE (self);
  GList *list;
  DiaLayer *layer = dia_diagram_data_get_active_layer (dia);

  /* add all objects and layers added from the diagram */
  for (list = change->layers; list != NULL; list = list->next) {
    layer = DIA_LAYER (list->data);
    data_add_layer (DIA_DIAGRAM_DATA (change->dia), layer);
  }

  for (list = change->objects; list != NULL; list = list->next) {
    DiaObject *obj = DIA_OBJECT (list->data);
    /* TODO: layer assignment wont be triggered for removed objects.
     * Maybe we need to store all the layers with the objects ourself?
     */
    if (dia_object_get_parent_layer (obj)) {
      layer = dia_object_get_parent_layer (obj);
    }

    dia_layer_add_object (layer, obj);
  }

  diagram_update_extents (DIA_DIAGRAM (change->dia));
}


static void
dia_import_change_revert (DiaChange   *self,
                          DiagramData *diagram)
{
  DiaImportChange *change = DIA_IMPORT_CHANGE (self);
  GList *list;

  /* otherwise we might end up with an empty selection */
  diagram_unselect_objects (DIA_DIAGRAM (change->dia), change->objects);
  /* remove all objects and layers added from the diagram */
  for (list = change->objects; list != NULL; list = list->next) {
    DiaObject *obj = DIA_OBJECT (list->data);
    DiaLayer *layer = dia_object_get_parent_layer (obj);
    dia_layer_remove_object (layer, obj);
  }

  for (list = change->layers; list != NULL; list = list->next) {
    DiaLayer *layer = DIA_LAYER (list->data);
    data_remove_layer (DIA_DIAGRAM_DATA (change->dia), layer);
  }

  diagram_update_extents (change->dia);
}


static void
dia_import_change_free (DiaChange *self)
{
  DiaImportChange *change = DIA_IMPORT_CHANGE (self);

  g_return_if_fail (change->dia != NULL);

  /* FIXME: do we need to delete more? */
  g_list_free (change->objects);
  g_list_free (change->layers);
}


/* listen on the diagram for object add */
static void
_import_object_add (DiagramData     *dia,
                    DiaLayer        *layer,
                    DiaObject       *obj,
                    DiaImportChange *change)
{
  g_return_if_fail (change->dia == DIA_DIAGRAM (dia));

  if (!obj)
    change->layers = g_list_prepend (change->layers, layer);
  else
    change->objects = g_list_prepend (change->objects, obj);
}


/**
 * undo_import_change_setup:
 * @dia: the #Diagram to watch
 *
 * Create an import change object listening on diagram additions
 *
 * Since: 0.98
 */
DiaChange *
dia_import_change_new (Diagram *dia)
{
  DiaImportChange *change = dia_change_new (DIA_TYPE_IMPORT_CHANGE);

  change->dia = dia;

  g_signal_connect (G_OBJECT (dia),
                    "object_add",
                    G_CALLBACK (_import_object_add),
                    change);

  return DIA_CHANGE (change);
}


/**
 * dia_import_change_done
 * @dia: the #Diagram
 * @chg: the #DiaChange object created by dia_import_change_new()
 *
 * Finish listening on the diagram changes
 *
 * Returns: %TRUE if the change was added to the undo stack, %FALSE otherwise
 *
 * Since: 0.98
 */
gboolean
dia_import_change_done (Diagram *dia, DiaChange *chg)
{
  DiaImportChange *change;

  g_return_val_if_fail (chg && DIA_IS_IMPORT_CHANGE (chg), FALSE);

  change = DIA_IMPORT_CHANGE (chg);

  /* stop listening on this diagram */
  g_signal_handlers_disconnect_by_func (change->dia, _import_object_add, change);

  if (change->layers != NULL || change->objects != NULL) {
    undo_push_change (dia->undo, chg);
    return TRUE;
  }

  return FALSE;
}



struct _DiaMemSwapChange {
  DiaChange parent;

  gsize   size;
  guint8 *dest;   /* for 'write trough' */
  guint8 *mem;
};

DIA_DEFINE_CHANGE (DiaMemSwapChange, dia_mem_swap_change)


static void
_swap_mem (DiaMemSwapChange *change, DiagramData *dia)
{
  gsize  i;

  for (i = 0; i < change->size; ++i) {
    guint8 tmp = change->mem[i];
    change->mem[i] = change->dest[i];
    change->dest[i] = tmp;
  }

  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));
}


static void
dia_mem_swap_change_apply (DiaChange   *change,
                           DiagramData *diagram)
{
  _swap_mem (DIA_MEM_SWAP_CHANGE (change), diagram);
}


static void
dia_mem_swap_change_revert (DiaChange   *change,
                            DiagramData *diagram)
{
  _swap_mem (DIA_MEM_SWAP_CHANGE (change), diagram);
}


static void
dia_mem_swap_change_free (DiaChange *change)
{
  DiaMemSwapChange *self = DIA_MEM_SWAP_CHANGE (change);

  g_clear_pointer (&self->mem, g_free);
}


/**
 * dia_mem_swap_change_new:
 * @dia: the Diagram to record the change for
 * @dest: a pointer somewhere in the Diagram
 * @size: of the region to copy and restore
 *
 * Record a memory region for undo (before actually changing it)
 *
 * Since: 0.98
 */
DiaChange *
dia_mem_swap_change_new (Diagram *dia, gpointer dest, gsize size)
{
  DiaMemSwapChange *change = dia_change_new (DIA_TYPE_MEM_SWAP_CHANGE);

  change->dest = dest;
  change->size = size;
  change->mem = g_new0 (guint8, size);
  /* initialize for swap */
  for (gsize i = 0; i < size; ++i) {
    change->mem[i] = change->dest[i];
  }

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



struct _DiaMoveGuideChange {
  DiaChange parent;

  double orig_pos;
  double dest_pos;
  DiaGuide *guide;
};

DIA_DEFINE_CHANGE (DiaMoveGuideChange, dia_move_guide_change)


static void
dia_move_guide_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaMoveGuideChange *change = DIA_MOVE_GUIDE_CHANGE (self);

  change->guide->position = change->dest_pos;

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));
}


static void
dia_move_guide_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaMoveGuideChange *change = DIA_MOVE_GUIDE_CHANGE (self);

  change->guide->position = change->orig_pos;

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));
}


static void
dia_move_guide_change_free (DiaChange *change)
{
  /* Nothing to free. */
}


DiaChange *
dia_move_guide_change_new (Diagram  *dia,
                           DiaGuide *guide,
                           double    orig_pos,
                           double    dest_pos)
{
  DiaMoveGuideChange *change = dia_change_new (DIA_TYPE_MOVE_GUIDE_CHANGE);

  change->orig_pos = orig_pos;
  change->dest_pos = dest_pos;
  change->guide = guide;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



struct _DiaAddGuideChange {
  DiaChange parent;

  DiaGuide *guide;
  int applied;
};

DIA_DEFINE_CHANGE (DiaAddGuideChange, dia_add_guide_change)


static void
dia_add_guide_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaAddGuideChange *change = DIA_ADD_GUIDE_CHANGE (self);
  DiaGuide *new_guide;

  g_debug ("add_guide_apply()");

  new_guide = dia_diagram_add_guide (DIA_DIAGRAM (dia),
                                     change->guide->position,
                                     change->guide->orientation,
                                     FALSE);
  g_clear_pointer (&change->guide, g_free);
  change->guide = new_guide;

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));

  /* Set flag. */
  change->applied = 1;
}


static void
dia_add_guide_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaAddGuideChange *change = DIA_ADD_GUIDE_CHANGE (self);

  g_debug ("add_guide_revert()");

  dia_diagram_remove_guide (DIA_DIAGRAM (dia), change->guide, FALSE);

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));

  /* Set flag. */
  change->applied = 0;
}


static void
dia_add_guide_change_free (DiaChange *self)
{
  DiaAddGuideChange *change = DIA_ADD_GUIDE_CHANGE (self);

  g_debug ("add_guide_free()");

  if (!change->applied) {
    g_clear_pointer (&change->guide, g_free);
  }
}


DiaChange *
dia_add_guide_change_new (Diagram *dia, DiaGuide *guide, int applied)
{
  DiaAddGuideChange *change = dia_change_new (DIA_TYPE_ADD_GUIDE_CHANGE);

  change->guide = guide;
  change->applied = applied;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}



struct _DiaDeleteGuideChange {
  DiaChange parent;

  DiaGuide *guide;
  int applied;
};

DIA_DEFINE_CHANGE (DiaDeleteGuideChange, dia_delete_guide_change)


static void
dia_delete_guide_change_apply (DiaChange *self, DiagramData *dia)
{
  DiaDeleteGuideChange *change = DIA_DELETE_GUIDE_CHANGE (self);

  g_debug ("delete_guide_apply()");

  dia_diagram_remove_guide (DIA_DIAGRAM (dia), change->guide, FALSE);

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));

  /* Set flag. */
  change->applied = 1;
}


static void
dia_delete_guide_change_revert (DiaChange *self, DiagramData *dia)
{
  DiaDeleteGuideChange *change = DIA_DELETE_GUIDE_CHANGE (self);

  /* Declare variable. */
  DiaGuide *new_guide;

  /* Log message. */
  g_debug ("delete_guide_revert()");

  /* Add it again. */
  new_guide = dia_diagram_add_guide (DIA_DIAGRAM (dia),
                                     change->guide->position,
                                     change->guide->orientation,
                                     FALSE);

  /* Reassign. */
  g_clear_pointer (&change->guide, g_free);
  change->guide = new_guide;

  /* Force redraw. */
  diagram_add_update_all (DIA_DIAGRAM (dia));
  diagram_modified (DIA_DIAGRAM (dia));
  diagram_flush (DIA_DIAGRAM (dia));

  /* Set flag. */
  change->applied = 0;
}


static void
dia_delete_guide_change_free (DiaChange *self)
{
  DiaDeleteGuideChange *change = DIA_DELETE_GUIDE_CHANGE (self);

  g_debug ("delete_guide_free()");

  if (change->applied) {
    g_clear_pointer (&change->guide, g_free);
  }
}


DiaChange *
dia_delete_guide_change_new (Diagram *dia, DiaGuide *guide, int applied)
{
  DiaDeleteGuideChange *change = dia_change_new (DIA_TYPE_DELETE_GUIDE_CHANGE);

  change->guide = guide;
  change->applied = applied;

  undo_push_change (dia->undo, DIA_CHANGE (change));

  return DIA_CHANGE (change);
}
