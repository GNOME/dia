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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
#ifndef PROPERTIES_H
#define PROPERTIES_H

#include "diatypes.h"
#include "diagram.h"

void object_properties_show(Diagram *dia, DiaObject *obj);
void object_list_properties_show(Diagram *dia, GList *objects);
void properties_update_if_shown(Diagram *dia, DiaObject *obj);
void properties_hide_if_shown(Diagram *dia, DiaObject *obj);


#endif /* PROPERTIES_H */


