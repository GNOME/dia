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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#include "dia-object-change-list.h"


struct _DiaObjectChangeList {
  DiaObjectChange parent_instance;

  GPtrArray *changes;
};


DIA_DEFINE_OBJECT_CHANGE (DiaObjectChangeList, dia_object_change_list)


static void
dia_object_change_list_apply (DiaObjectChange *self, DiaObject *object)
{
  DiaObjectChangeList *change = DIA_OBJECT_CHANGE_LIST (self);

  for (int i = 0; i < change->changes->len; i++) {
    dia_object_change_apply (g_ptr_array_index (change->changes, i), object);
  }
}


static void
dia_object_change_list_revert (DiaObjectChange *self, DiaObject *object)
{
  DiaObjectChangeList *change = DIA_OBJECT_CHANGE_LIST (self);

  for (int i = 0; i < change->changes->len; i++) {
    dia_object_change_revert (g_ptr_array_index (change->changes, i), object);
  }
}


static void
dia_object_change_list_free (DiaObjectChange *self)
{
  DiaObjectChangeList *change = DIA_OBJECT_CHANGE_LIST (self);

  g_clear_pointer (&change->changes, g_ptr_array_unref);
}


DiaObjectChange *
dia_object_change_list_new (void)
{
  DiaObjectChangeList *self = dia_object_change_new (DIA_TYPE_OBJECT_CHANGE_LIST);

  self->changes = g_ptr_array_new_with_free_func (dia_object_change_unref);

  return DIA_OBJECT_CHANGE (self);
}


void
dia_object_change_list_add (DiaObjectChangeList *self,
                            DiaObjectChange     *change)
{
  g_return_if_fail (DIA_IS_OBJECT_CHANGE_LIST (self));

  if (change) {
    g_ptr_array_add (self->changes, dia_object_change_ref (change));
  }
}
