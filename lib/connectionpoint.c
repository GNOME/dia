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

#include "config.h"
#include "connectionpoint.h"

/** Update the object-settable parts of a connectionpoints.
 * p: A ConnectionPoint pointer (non-NULL).
 * x: The x coordinate of the connectionpoint.
 * y: The y coordinate of the connectionpoint.
 * dirs: The directions that are open for connections on this point.
 */
void 
connpoint_update(ConnectionPoint *p, real x, real y, gint dirs)
{
  p->pos.x = x;
  p->pos.y = y;
  p->directions = dirs;
}
