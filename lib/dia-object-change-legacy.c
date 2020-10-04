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

#include "dia-object-change-legacy.h"


struct _DiaObjectChangeLegacy {
  DiaObjectChange parent_instance;

  ObjectChange *legacy;
};


DIA_DEFINE_OBJECT_CHANGE (DiaObjectChangeLegacy, dia_object_change_legacy)


static void
dia_object_change_legacy_apply (DiaObjectChange *self, DiaObject *object)
{
  DiaObjectChangeLegacy *change = DIA_OBJECT_CHANGE_LEGACY (self);

  change->legacy->apply (change->legacy, object);
}


static void
dia_object_change_legacy_revert (DiaObjectChange *self, DiaObject *object)
{
  DiaObjectChangeLegacy *change = DIA_OBJECT_CHANGE_LEGACY (self);

  change->legacy->revert (change->legacy, object);
}


static void
dia_object_change_legacy_free (DiaObjectChange *self)
{
  DiaObjectChangeLegacy *change = DIA_OBJECT_CHANGE_LEGACY (self);

  if (change->legacy->free) {
    change->legacy->free (change->legacy);
  }

  g_clear_pointer (&change->legacy, g_free);
}


DiaObjectChange *
dia_object_change_legacy_new (ObjectChange *legacy)
{
  DiaObjectChangeLegacy *change = dia_object_change_new (DIA_TYPE_OBJECT_CHANGE_LEGACY);

  change->legacy = legacy;

  return DIA_OBJECT_CHANGE (change);
}
