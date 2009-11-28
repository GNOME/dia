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

#include <config.h>

#include "object.h"

/******** ObjectChange for object that just need to get/set state: *****/

typedef struct _ObjectStateChange ObjectStateChange;

struct _ObjectStateChange {
  ObjectChange obj_change;
  
  GetStateFunc get_state;
  SetStateFunc set_state;

  ObjectState *saved_state;
  DiaObject *obj;
};

static void
object_state_change_apply_revert(ObjectStateChange *change, DiaObject *obj)
{
  ObjectState *old_state;
  
  old_state = change->get_state(change->obj);

  change->set_state(change->obj, change->saved_state);

  change->saved_state = old_state;
}

static void
object_state_change_free(ObjectStateChange *change)
{
  if ((change) && (change->saved_state)) {
    if (change->saved_state->free)
      (*change->saved_state->free)(change->saved_state);
    g_free(change->saved_state);
  }
}

ObjectChange *new_object_state_change(DiaObject *obj,
				      ObjectState *old_state,
				      GetStateFunc get_state,
				      SetStateFunc set_state )
{
  ObjectStateChange *change;

  g_return_val_if_fail (get_state != NULL && set_state != NULL, NULL);

  change = g_new(ObjectStateChange, 1);
  
  change->obj_change.apply =
    (ObjectChangeApplyFunc) object_state_change_apply_revert;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) object_state_change_apply_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) object_state_change_free;

  change->get_state = get_state;
  change->set_state = set_state;

  change->obj = obj;
  change->saved_state = old_state;

  return (ObjectChange *)change;
}

