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

#include "diagram.h"
#include "dia-guide.h"
#include "dia-change.h"
#include "dia-object-change.h"


struct _UndoStack {
  Diagram *dia;
  DiaChange *last_change; /* Points to the object on the top of stack. */
  DiaChange *current_change; /* Points to the last object currently applied */
  DiaChange *last_save;   /* Points to current_change at the time of last save. */
  int depth;
};

UndoStack *new_undo_stack(Diagram *dia);
void undo_destroy(UndoStack *stack);
void undo_push_change(UndoStack *stack, DiaChange *change);
void undo_set_transactionpoint(UndoStack *stack);
void undo_revert_to_last_tp(UndoStack *stack);
void undo_apply_to_next_tp(UndoStack *stack);
void undo_clear(UndoStack *stack);
void undo_mark_save(UndoStack *stack);
gboolean undo_is_saved(UndoStack *stack);
gboolean undo_available(UndoStack *stack, gboolean undo);
DiaChange* undo_remove_to(UndoStack *stack, GType type);

/* Specific undo functions: */

#define DIA_TYPE_TRANSACTION_POINT_CHANGE dia_transaction_point_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaTransactionPointChange, dia_transaction_point_change, DIA, TRANSACTION_POINT_CHANGE, DiaChange)

#define DIA_TYPE_MOVE_OBJECTS_CHANGE dia_move_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMoveObjectsChange, dia_move_objects_change, DIA, MOVE_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_move_objects_change_new                 (Diagram   *dia,
                                                        Point     *orig_pos,
                                                        Point     *dest_pos,
                                                        GList     *obj_list);


#define DIA_TYPE_MOVE_HANDLE_CHANGE dia_move_handle_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMoveHandleChange, dia_move_handle_change, DIA, MOVE_HANDLE_CHANGE, DiaChange)

DiaChange *dia_move_handle_change_new                  (Diagram   *dia,
                                                        Handle    *handle,
                                                        DiaObject *obj,
                                                        Point      orig_pos,
                                                        Point      dest_pos,
                                                        int        modifiers);


#define DIA_TYPE_CONNECT_CHANGE dia_connect_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaConnectChange, dia_connect_change, DIA, CONNECT_CHANGE, DiaChange)

DiaChange *dia_connect_change_new                      (Diagram   *dia,
                                                        DiaObject *obj,
                                                        Handle    *handle,
                                                        ConnectionPoint *connectionpoint);


#define DIA_TYPE_UNCONNECT_CHANGE dia_unconnect_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaUnconnectChange, dia_unconnect_change, DIA, UNCONNECT_CHANGE, DiaChange)

DiaChange *dia_unconnect_change_new                    (Diagram   *dia,
                                                        DiaObject *obj,
                                                        Handle    *handle);


#define DIA_TYPE_DELETE_OBJECTS_CHANGE dia_delete_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaDeleteObjectsChange, dia_delete_objects_change, DIA, DELETE_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_delete_objects_change_new_with_children (Diagram   *dia,
                                                        GList     *obj_list);
DiaChange *dia_delete_objects_change_new               (Diagram   *dia,
                                                        GList     *obj_list); /* Reads current obj list */


#define DIA_TYPE_INSERT_OBJECTS_CHANGE dia_insert_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaInsertObjectsChange, dia_insert_objects_change, DIA, INSERT_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_insert_objects_change_new               (Diagram   *dia,
                                                        GList     *obj_list,
                                                        int        applied);


#define DIA_TYPE_REORDER_OBJECTS_CHANGE dia_reorder_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaReorderObjectsChange, dia_reorder_objects_change, DIA, REORDER_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_reorder_objects_change_new              (Diagram   *dia,
                                                        GList     *changed_list,
                                                        GList     *orig_list); /* Reads current obj list */


#define DIA_TYPE_OBJECT_CHANGE_CHANGE dia_object_change_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaObjectChangeChange, dia_object_change_change, DIA, OBJECT_CHANGE_CHANGE, DiaChange)

DiaChange *dia_object_change_change_new                (Diagram         *dia,
                                                        DiaObject       *obj,
                                                        DiaObjectChange *change);


#define DIA_TYPE_GROUP_OBJECTS_CHANGE dia_group_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaGroupObjectsChange, dia_group_objects_change, DIA, GROUP_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_group_objects_change_new                (Diagram   *dia,
                                                        GList     *obj_list,
                                                        DiaObject *group,
                                                        GList     *orig_list);


#define DIA_TYPE_UNGROUP_OBJECTS_CHANGE dia_ungroup_objects_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaUngroupObjectsChange, dia_ungroup_objects_change, DIA, UNGROUP_OBJECTS_CHANGE, DiaChange)

DiaChange *dia_ungroup_objects_change_new              (Diagram   *dia,
                                                        GList     *obj_list,
                                                        DiaObject *group,
                                                        int        group_index);


#define DIA_TYPE_PARENTING_CHANGE dia_parenting_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaParentingChange, dia_parenting_change, DIA, PARENTING_CHANGE, DiaChange)

DiaChange *dia_parenting_change_new                    (Diagram   *dia,
                                                        DiaObject *parentobj,
                                                        DiaObject *childobj,
                                                        gboolean   parent);


#define DIA_TYPE_MOVE_OBJECT_TO_LAYER_CHANGE dia_move_object_to_layer_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMoveObjectToLayerChange, dia_move_object_to_layer_change, DIA, MOVE_OBJECT_TO_LAYER_CHANGE, DiaChange)

DiaChange *dia_move_object_to_layer_change_new         (Diagram   *diagram,
                                                        GList     *selected_list,
                                                        gboolean   moving_up);


#define DIA_TYPE_IMPORT_CHANGE dia_import_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaImportChange, dia_import_change, DIA, IMPORT_CHANGE, DiaChange)

/* Create an import change object listening on diagram additions/removals */
DiaChange *dia_import_change_new                       (Diagram   *diagram);
/* Finish listening on the diagram changes */
gboolean   dia_import_change_done                      (Diagram   *dia,
                                                        DiaChange *chg);


#define DIA_TYPE_MEM_SWAP_CHANGE dia_mem_swap_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMemSwapChange, dia_mem_swap_change, DIA, MEM_SWAP_CHANGE, DiaChange)

/* handle with care, just plain memory copy */
DiaChange *dia_mem_swap_change_new                     (Diagram   *dia,
                                                        gpointer   dest,
                                                        gsize      size);


#define DIA_TYPE_MOVE_GUIDE_CHANGE dia_move_guide_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMoveGuideChange, dia_move_guide_change, DIA, MOVE_GUIDE_CHANGE, DiaChange)

DiaChange *dia_move_guide_change_new                   (Diagram   *dia,
                                                        DiaGuide  *guide,
                                                        real       orig_pos,
                                                        real       dest_pos);


#define DIA_TYPE_ADD_GUIDE_CHANGE dia_add_guide_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaAddGuideChange, dia_add_guide_change, DIA, ADD_GUIDE_CHANGE, DiaChange)

DiaChange *dia_add_guide_change_new                    (Diagram   *dia,
                                                        DiaGuide  *guide,
                                                        int        applied);


#define DIA_TYPE_DELETE_GUIDE_CHANGE dia_delete_guide_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaDeleteGuideChange, dia_delete_guide_change, DIA, DELETE_GUIDE_CHANGE, DiaChange)

DiaChange *dia_delete_guide_change_new                 (Diagram   *dia,
                                                        DiaGuide  *guide,
                                                        int        applied);

#endif /* UNDO_H */

