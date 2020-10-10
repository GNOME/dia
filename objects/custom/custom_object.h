/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
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

G_BEGIN_DECLS

#define DIA_TYPE_CUSTOM_OBJECT_CHANGE dia_custom_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaCustomObjectChange,
                      dia_custom_object_change,
                      DIA, CUSTOM_OBJECT_CHANGE,
                      DiaObjectChange)

void custom_setup_properties (ShapeInfo *info, xmlNodePtr node);
void custom_object_new(ShapeInfo *info, DiaObjectType **otype);

G_END_DECLS
