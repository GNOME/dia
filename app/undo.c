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

#include "undo.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
static void
transaction_point_pointer(Change *change, Diagram *dia)
{
}

static int
is_transactionpoint(Change *change)
{
  return change->apply == transaction_point_pointer;
}

static Change *
new_transactionpoint(void)
{
  Change *transaction = g_new(Change, 1);

  if (transaction) {
    transaction->apply = transaction_point_pointer;
    transaction->revert = transaction_point_pointer;
    transaction->free = NULL;
  }
  
  return transaction;
}

UndoStack *
new_undo_stack(Diagram *dia)
{
  UndoStack *stack;
  Change *transaction;
  
  stack = g_new(UndoStack, 1);
  if (stack!=NULL){
    stack->dia = dia;
    transaction = new_transactionpoint();
    transaction->next = transaction->prev = NULL;
    stack->last_change = transaction;
    stack->current_change = transaction;
    
  }
  return stack;
}

void
undo_destroy(UndoStack *stack)
{
  undo_clear(stack);
  g_free(stack->current_change); /* Free first transaction point. */
  g_free(stack);
}



static void
undo_remove_redo_info(UndoStack *stack)
{
  Change *change;
  Change *next_change;

  printf("UNDO: Removing redo info\n");
  
  change = stack->current_change->next;
  stack->current_change->next = NULL;
  stack->last_change = stack->current_change;
    
  while (change != NULL) {
    next_change = change->next;
    if (change->free)
      (change->free)(change);
    g_free(change);
    change = next_change;
  }
}

static int
depth(UndoStack *stack)
{
  int i;
  Change *change;
  change = stack->current_change;

  i = 1;
  while (change != NULL) {
    change = change->prev;
    i++;
  }
  return i-1;
}

void
undo_push_change(UndoStack *stack, Change *change)
{
  if (stack->current_change != stack->last_change)
    undo_remove_redo_info(stack);

  printf("UNDO: Push new change at %d\n", depth(stack));
  
  change->prev = stack->last_change;
  change->next = NULL;
  stack->last_change->next = change;
  stack->last_change = change;
  stack->current_change = change;
}

void
undo_set_transactionpoint(UndoStack *stack)
{
  Change *transaction;

  if (is_transactionpoint(stack->current_change))
    return;

  printf("UNDO: Push new transactionpoint at %d\n", depth(stack));

  transaction = new_transactionpoint();

  undo_push_change(stack, transaction);
}

void
undo_revert_to_last_tp(UndoStack *stack)
{
  Change *change;
  Change *prev_change;
  
  if (stack->current_change->prev == NULL)
    return; /* Can't revert first transactionpoint */

  change = stack->current_change;
  do {
    prev_change = change->prev;
    (change->revert)(change, stack->dia);
    change = prev_change;
  } while (!is_transactionpoint(change));
  stack->current_change  = change;
}

void
undo_apply_to_next_tp(UndoStack *stack)
{
  Change *change;
  Change *next_change;
  
  change = stack->current_change;
  do {
    next_change = change->next;
    (change->apply)(change, stack->dia);
    change = next_change;
  } while ( (change != NULL) &&
	    (!is_transactionpoint(change)) );
  if (change == NULL)
    change = stack->last_change;
  stack->current_change = change;
}


void
undo_clear(UndoStack *stack)
{
  Change *change;

  change = stack->current_change;
    
  while (change->prev != NULL) {
    change = change->prev;
  }
  
  stack->current_change = change;
  
  undo_remove_redo_info(stack);
}


/****************************************************************/
/****************************************************************/
/*****************                          *********************/
/***************** Specific undo functions: *********************/
/*****************                          *********************/
/****************************************************************/
/****************************************************************/

/******** Move object list: */

struct MoveObjectsChange {
  Change change;

  Point *orig_pos;
  Point *dest_pos;
  GList *obj_list;
};

static void
move_objects_apply(struct MoveObjectsChange *change, Diagram *dia)
{
  GList *list;
  int i;
  Object *obj;

  object_add_updates_list(change->obj_list, dia);

  list = change->obj_list;
  i=0;
  while (list != NULL) {
    obj = (Object *)  list->data;
    
    obj->ops->move(obj, &change->dest_pos[i]);
    
    list = g_list_next(list); i++;
  }

  list = change->obj_list;
  while (list!=NULL) {
    obj = (Object *) list->data;
    
    diagram_update_connections_object(dia, obj, TRUE);
    
    list = g_list_next(list);
  }

  object_add_updates_list(change->obj_list, dia);
}

static void
move_objects_revert(struct MoveObjectsChange *change, Diagram *dia)
{
  GList *list;
  int i;
  Object *obj;

  object_add_updates_list(change->obj_list, dia);

  list = change->obj_list;
  i=0;
  while (list != NULL) {
    obj = (Object *)  list->data;
    
    obj->ops->move(obj, &change->orig_pos[i]);
    
    list = g_list_next(list); i++;
  }

  list = change->obj_list;
  while (list!=NULL) {
    obj = (Object *) list->data;
    
    diagram_update_connections_object(dia, obj, TRUE);
    
    list = g_list_next(list);
  }

  object_add_updates_list(change->obj_list, dia);
}

static void
move_objects_free(struct MoveObjectsChange *change)
{
  g_free(change->orig_pos);
  g_free(change->dest_pos);
  g_list_free(change->obj_list);
}

extern Change *
undo_move_objects(Diagram *dia, Point *orig_pos, Point *dest_pos,
		  GList *obj_list)
{
  struct MoveObjectsChange *change;

  change = g_new(struct MoveObjectsChange, 1);
  
  change->change.apply = (UndoApplyFunc) move_objects_apply;
  change->change.revert = (UndoRevertFunc) move_objects_revert;
  change->change.free = (UndoFreeFunc) move_objects_free;

  change->orig_pos = orig_pos;
  change->dest_pos = dest_pos;
  change->obj_list = obj_list;

  printf("UNDO: Push new move objects at %d\n", depth(dia->undo));
  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}

/********** Move handle: */

struct MoveHandleChange {
  Change change;

  Point orig_pos;
  Point dest_pos;
  Handle *handle;
  Object *obj;
};

static void
move_handle_apply(struct MoveHandleChange *change, Diagram *dia)
{
  object_add_updates(change->obj, dia);
  change->obj->ops->move_handle(change->obj, change->handle,
				 &change->dest_pos,
				 HANDLE_MOVE_USER_FINAL,0);
  object_add_updates(change->obj, dia);
}

static void
move_handle_revert(struct MoveHandleChange *change, Diagram *dia)
{
  object_add_updates(change->obj, dia);
  change->obj->ops->move_handle(change->obj, change->handle,
				 &change->orig_pos,
				 HANDLE_MOVE_USER_FINAL,0);
  object_add_updates(change->obj, dia);
}

static void
move_handle_free(struct MoveHandleChange *change)
{
}


Change *
undo_move_handle(Diagram *dia,
		 Handle *handle, Object *obj,
		 Point orig_pos, Point dest_pos)
{
  struct MoveHandleChange *change;

  change = g_new(struct MoveHandleChange, 1);
  
  change->change.apply = (UndoApplyFunc) move_handle_apply;
  change->change.revert = (UndoRevertFunc) move_handle_revert;
  change->change.free = (UndoFreeFunc) move_handle_free;

  change->orig_pos = orig_pos;
  change->dest_pos = dest_pos;
  change->handle = handle;
  change->obj = obj;

  printf("UNDO: Push new move handle at %d\n", depth(dia->undo));

  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}

/***************** Connect object: */

struct ConnectChange {
  Change change;

  Object *obj;
  Handle *handle;
  ConnectionPoint *connectionpoint;
  Point handle_pos;
};

static void
connect_apply(struct ConnectChange *change, Diagram *dia)
{
  object_connect(change->obj, change->handle, change->connectionpoint);
  
  object_add_updates(change->obj, dia);
  change->obj->ops->move_handle(change->obj, change->handle ,
				&change->connectionpoint->pos,
				HANDLE_MOVE_CONNECTED, 0);
  
  object_add_updates(change->obj, dia);
}

static void
connect_revert(struct ConnectChange *change, Diagram *dia)
{
  object_unconnect(change->obj, change->handle);
  
  object_add_updates(change->obj, dia);
  change->obj->ops->move_handle(change->obj, change->handle ,
				&change->handle_pos,
				HANDLE_MOVE_CONNECTED, 0);
  
  object_add_updates(change->obj, dia);
}

static void
connect_free(struct ConnectChange *change)
{
}

extern Change *
undo_connect(Diagram *dia, Object *obj, Handle *handle,
	     ConnectionPoint *connectionpoint)
{
  struct ConnectChange *change;

  change = g_new(struct ConnectChange, 1);
  
  change->change.apply = (UndoApplyFunc) connect_apply;
  change->change.revert = (UndoRevertFunc) connect_revert;
  change->change.free = (UndoFreeFunc) connect_free;

  change->obj = obj;
  change->handle = handle;
  change->handle_pos = handle->pos;
  change->connectionpoint = connectionpoint;

  printf("UNDO: Push new connect at %d\n", depth(dia->undo));
  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}

/*************** Unconnect object: */

struct UnconnectChange {
  Change change;

  Object *obj;
  Handle *handle;
  ConnectionPoint *connectionpoint;
};

static void
unconnect_apply(struct UnconnectChange *change, Diagram *dia)
{
  object_unconnect(change->obj, change->handle);
  
  object_add_updates(change->obj, dia);
}

static void
unconnect_revert(struct UnconnectChange *change, Diagram *dia)
{
  object_connect(change->obj, change->handle, change->connectionpoint);
  
  object_add_updates(change->obj, dia);
}

static void
unconnect_free(struct UnconnectChange *change)
{
}

extern Change *
undo_unconnect(Diagram *dia, Object *obj, Handle *handle)
{
  struct UnconnectChange *change;

  change = g_new(struct UnconnectChange, 1);
  
  change->change.apply = (UndoApplyFunc) unconnect_apply;
  change->change.revert = (UndoRevertFunc) unconnect_revert;
  change->change.free = (UndoFreeFunc) unconnect_free;

  change->obj = obj;
  change->handle = handle;
  change->connectionpoint = handle->connected_to;

  printf("UNDO: Push new unconnect at %d\n", depth(dia->undo));
  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}


/******** Delete object list: */

struct DeleteObjectsChange {
  Change change;

  Layer *layer;
  GList *obj_list;
};

static void
delete_objects_apply(struct DeleteObjectsChange *change, Diagram *dia)
{
  layer_remove_objects(change->layer, change->obj_list);
  object_add_updates_list(change->obj_list, dia);
}

static void
delete_objects_revert(struct DeleteObjectsChange *change, Diagram *dia)
{
  layer_add_objects(change->layer, change->obj_list);
  object_add_updates_list(change->obj_list, dia);
}

static void
delete_objects_free(struct DeleteObjectsChange *change)
{
  destroy_object_list(change->obj_list);
}

extern Change *
undo_delete_objects(Diagram *dia, GList *obj_list)
{
  struct DeleteObjectsChange *change;

  change = g_new(struct DeleteObjectsChange, 1);
  
  change->change.apply = (UndoApplyFunc) delete_objects_apply;
  change->change.revert = (UndoRevertFunc) delete_objects_revert;
  change->change.free = (UndoFreeFunc) delete_objects_free;

  change->layer = dia->data->active_layer;
  change->obj_list = obj_list;

  printf("UNDO: Push new delete objects at %d\n", depth(dia->undo));
  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}
