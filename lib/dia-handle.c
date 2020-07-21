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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#include "handle.h"


/**
 * dia_handle_set_position:
 * @self: the #Handle to move
 * @point: the new position of @self
 *
 * Move a handle to a #graphene_point_t
 *
 * Stability: Stable
 *
 * Since: 0.98
 */
void
dia_handle_set_position (Handle            *self,
                         graphene_point_t  *point)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (point != NULL);

  self->pos.x = point->x;
  self->pos.y = point->y;
}
