/* xxxxxx -- an diagram creation/manipulation program
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
#ifndef OBJECT_OPS_H
#define OBJECT_OPS_H

#include "object.h"
#include "display.h"
#include "diagram.h"

extern void object_add_updates(Object *obj, Diagram *dia);
extern void object_add_updates_list(GList *list, Diagram *dia);
extern ConnectionPoint *object_find_connectpoint_display(DDisplay *ddisp,
							 Point *pos);
extern void object_connect_display(DDisplay *ddisp, Object *obj,
				   Handle *handle);
extern GList *object_copy_list(GList *list);
extern Point object_list_corner(GList *list);
extern void object_list_move_delta(GList *objects, Point *delta);
#endif /* OBJECT_OPS_H */

