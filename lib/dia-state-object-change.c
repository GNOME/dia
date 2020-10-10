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

#include "dia-state-object-change.h"


/**
 * SECTION:dia-state-object-change
 *
 * #DiaObjectChange for object that just need to get/set state
 */


struct _DiaStateObjectChange {
  DiaObjectChange obj_change;

  GetStateFunc get_state;
  SetStateFunc set_state;

  ObjectState *saved_state;
  DiaObject *obj;
};


DIA_DEFINE_OBJECT_CHANGE (DiaStateObjectChange, dia_state_object_change)


static void
object_state_change_apply_revert (DiaStateObjectChange *change, DiaObject *obj)
{
  ObjectState *old_state;

  old_state = change->get_state (change->obj);

  change->set_state (change->obj, change->saved_state);

  change->saved_state = old_state;
}


static void
dia_state_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  object_state_change_apply_revert (DIA_STATE_OBJECT_CHANGE (self), obj);
}


static void
dia_state_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  object_state_change_apply_revert (DIA_STATE_OBJECT_CHANGE (self), obj);
}


static void
dia_state_object_change_free (DiaObjectChange *self)
{
  DiaStateObjectChange *change = DIA_STATE_OBJECT_CHANGE (self);

  if ((change) && (change->saved_state)) {
    if (change->saved_state->free)
      (*change->saved_state->free) (change->saved_state);
    g_clear_pointer (&change->saved_state, g_free);
  }
}


/**
 * dia_state_object_change_new:
 * @obj: the #DiaObject whos state changed
 * @old_state: the previous #ObjectState
 * @get_state: a #GetStateFunc used to generate the #ObjectState of @obj
 * @set_state: a #SetStateFunc used to apply a #ObjectState to @obj
 *
 * Create a single change from the ObjectState
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
DiaObjectChange *
dia_state_object_change_new (DiaObject    *obj,
                             ObjectState  *old_state,
                             GetStateFunc  get_state,
                             SetStateFunc  set_state)
{
  DiaStateObjectChange *change;

  g_return_val_if_fail (get_state != NULL && set_state != NULL, NULL);

  change = dia_object_change_new (DIA_TYPE_STATE_OBJECT_CHANGE);

  change->get_state = get_state;
  change->set_state = set_state;

  change->obj = obj;
  change->saved_state = old_state;

  return DIA_OBJECT_CHANGE (change);
}
