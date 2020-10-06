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

#pragma once

#include "diavar.h"
#include "object.h"

G_BEGIN_DECLS

#define DIA_TYPE_GROUP_OBJECT_CHANGE dia_group_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaGroupObjectChange, dia_group_object_change, DIA, GROUP_OBJECT_CHANGE, DiaObjectChange)


#define DIA_GROUP(object) ((Group *) object)

extern DIAVAR DiaObjectType group_type;

/* Make sure there are no connections from objects to objects
 * outside of the created group before calling group_create().
 */
DiaObject *group_create(GList *objects);
DiaObject *group_create_with_matrix(GList *objects, DiaMatrix *matrix);
GList *group_objects(DiaObject *group);

void group_destroy_shallow(DiaObject *group);
const DiaMatrix *group_get_transform (Group *group);

#define IS_GROUP(obj) ((obj)->type == &group_type)

G_END_DECLS
