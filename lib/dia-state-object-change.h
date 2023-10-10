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

#pragma once

#include "diatypes.h"
#include "dia-object-change.h"

G_BEGIN_DECLS

#define DIA_TYPE_STATE_OBJECT_CHANGE dia_state_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaStateObjectChange,
                      dia_state_object_change,
                      DIA, STATE_OBJECT_CHANGE,
                      DiaObjectChange)


/**
 * ObjectState:
 * @free: (nullable): destroy the #ObjectState content
 *
 * Since: dawn-of-time
 */
struct _ObjectState {
  void (*free) (ObjectState *state);
};


/**
 * GetStateFunc:
 *
 * Gets the internal state from the object.
 * This is used to snapshot the object state
 * so that it can be stored for undo/redo.
 *
 * Need not save state that only depends on
 * the object and it's handles positions.
 *
 * The calling function owns the returned reference.
 *
 * Since: dawn-of-time
 */
typedef ObjectState * (*GetStateFunc) (DiaObject* obj);


/**
 * SetStateFunc:
 *
 * Sets the internal state from the object.
 * This is used to snapshot the object state
 * so that it can be stored for undo/redo.
 *
 * The called function owns the reference and is
 * responsible for freeing it.
 *
 * Since: dawn-of-time
 */
typedef void (*SetStateFunc) (DiaObject* obj, ObjectState *state);

DiaObjectChange *dia_state_object_change_new (DiaObject    *obj,
                                              ObjectState  *old_state,
                                              GetStateFunc  get_state,
                                              SetStateFunc  set_state);

G_END_DECLS
