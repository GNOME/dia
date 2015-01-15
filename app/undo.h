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
#ifndef UNDO_H
#define UNDO_H


typedef struct _UndoStack UndoStack;
typedef struct _Change Change;

#include "diagram.h"

typedef void (*UndoApplyFunc)(Change *change, Diagram *dia);
typedef void (*UndoRevertFunc)(Change *change, Diagram *dia);
typedef void (*UndoFreeFunc)(Change *change);

struct _Change {
  /* If apply == transaction_point_pointer then this is a transaction
     point */
  UndoApplyFunc  apply;
  UndoRevertFunc revert;
  UndoFreeFunc   free; /* Remove extra data. Then this object is freed */
  Change *prev, *next;
};

struct _UndoStack {
  Diagram *dia;
  Change *last_change; /* Points to the object on the top of stack. */
  Change *current_change; /* Points to the last object currently applied */
  Change *last_save;   /* Points to current_change at the time of last save. */
  int depth;
};

UndoStack *new_undo_stack(Diagram *dia);
void undo_destroy(UndoStack *stack);
void undo_push_change(UndoStack *stack, Change *change);
void undo_set_transactionpoint(UndoStack *stack);
void undo_revert_to_last_tp(UndoStack *stack);
void undo_apply_to_next_tp(UndoStack *stack);
void undo_clear(UndoStack *stack);
void undo_mark_save(UndoStack *stack);
gboolean undo_is_saved(UndoStack *stack);
gboolean undo_available(UndoStack *stack, gboolean undo);
Change* undo_remove_to(UndoStack *stack, UndoApplyFunc *type);

/* Specific undo functions: */

Change *undo_move_objects(Diagram *dia, Point *orig_pos,
			  Point *dest_pos, GList *obj_list);
Change *undo_move_handle(Diagram *dia,
			 Handle *handle, DiaObject *obj,
			 Point orig_pos, Point dest_pos,
			 int modifiers);
Change *undo_connect(Diagram *dia, DiaObject *obj, Handle *handle,
		     ConnectionPoint *connectionpoint);
Change *undo_unconnect(Diagram *dia, DiaObject *obj, Handle *handle);
Change *undo_delete_objects_children(Diagram *dia, GList *obj_list);
Change *undo_delete_objects(Diagram *dia, GList *obj_list); /* Reads current obj list */
Change *undo_insert_objects(Diagram *dia, GList *obj_list,
			    int applied);
Change *undo_reorder_objects(Diagram *dia, GList *changed_list,
			     GList *orig_list); /* Reads current obj list */
Change *undo_object_change(Diagram *dia, DiaObject *obj,
			   ObjectChange *obj_change);
Change *undo_group_objects(Diagram *dia, GList *obj_list,
			   DiaObject *group, GList *orig_list);
Change *undo_ungroup_objects(Diagram *dia, GList *obj_list,
			     DiaObject *group, int group_index);
Change *undo_parenting(Diagram *dia, DiaObject *parentobj, DiaObject *childobj,
		       gboolean parent);
Change *undo_move_object_other_layer(Diagram *diagram, GList *selected_list,
				     gboolean moving_up);

/* Create an import change object listening on diagram additions/removals */
Change *undo_import_change_setup (Diagram *diagram);
/* Finish listening on the diagram changes */
gboolean undo_import_change_done (Diagram *dia, Change *chg);

/* handle with care, just plain memory copy */
Change *undo_change_memswap (Diagram *dia, gpointer dest, gsize size);

#endif /* UNDO_H */

