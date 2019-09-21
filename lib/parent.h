/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2003 Vadim Berezniker
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

#ifndef PARENT_H
#define PARENT_H

#include <glib.h>
#include "diatypes.h"
#include "geometry.h"

GList *parent_list_affected(GList *obj_list);
void parent_handle_extents (DiaObject *obj, DiaRectangle *extents);
Point parent_move_child_delta (DiaRectangle *p_ext, DiaRectangle *c_text, Point *delta);
void parent_point_extents (Point *point, DiaRectangle *extents);
gboolean parent_list_expand(GList *obj_list);
GList *parent_list_affected_hierarchy(GList *obj_list);
gboolean parent_handle_move_out_check(DiaObject *object, Point *to);
gboolean parent_handle_move_in_check(DiaObject *object, Point *to, Point *start_at);
void parent_apply_to_children(DiaObject *obj, DiaObjectFunc func);

#endif /* PARENT_H  */
