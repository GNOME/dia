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
#ifndef OBJECT_OPS_H
#define OBJECT_OPS_H

#include "diatypes.h"
#include "display.h"
#include "diagram.h"

/* Note that TOP/LEFT and BOTTOM/RIGHT are the same */
#define DIA_ALIGN_TOP 0
#define DIA_ALIGN_LEFT 0
#define DIA_ALIGN_CENTER 1
#define DIA_ALIGN_BOTTOM 2
#define DIA_ALIGN_RIGHT 2
#define DIA_ALIGN_POSITION 3
#define DIA_ALIGN_EQUAL 4
#define DIA_ALIGN_ADJACENT 5

void object_add_updates(DiaObject *obj, Diagram *dia);
void object_add_updates_list(GList *list, Diagram *dia);
ConnectionPoint *object_find_connectpoint_display(DDisplay *ddisp,
						  Point *pos,
						  DiaObject *notthis,
						  gboolean snap_to_objects);

void object_connect_display(DDisplay *ddisp, DiaObject *obj,
			    Handle *handle, gboolean snap_to_objects);
/* Adds Undo info for connected objects. */

Point object_list_corner(GList *list);
void object_list_align_h(GList *objects, Diagram *dia, int align);
void object_list_align_v(GList *objects, Diagram *dia, int align);
void object_list_align_connected (GList *objects, Diagram *dia, int align);

typedef enum {
  DIR_UP = 1,
  DIR_DOWN,
  DIR_LEFT,
  DIR_RIGHT
} Direction;

void object_list_nudge(GList *objects, Diagram *dia, Direction dir, real step);

#endif /* OBJECT_OPS_H */

