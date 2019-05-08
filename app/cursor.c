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

#include <config.h>

#include <gdk/gdk.h>

#include "display.h"
#include "cursor.h"

static int direction_cursors[MAX_CURSORS] = {
  /* for safety reasons these should be last and must be in the same order HANDLE_RESIZE_* */
  GDK_TOP_LEFT_CORNER,     /* CURSOR_DIRECTION_0 + NW */
  GDK_TOP_SIDE,            /* N */
  GDK_TOP_RIGHT_CORNER,    /* NE */
  GDK_LEFT_SIDE,           /* W  */
  GDK_RIGHT_SIDE,          /* E  */
  GDK_BOTTOM_LEFT_CORNER,  /* SE */
  GDK_BOTTOM_SIDE,         /* S  */
  GDK_BOTTOM_RIGHT_CORNER, /* SW */
};

GdkCursor *
get_direction_cursor (DiaCursorType ctype)
{
  if (ctype >= G_N_ELEMENTS (direction_cursors)) {
    return NULL;
  }

  return gdk_cursor_new (direction_cursors[ctype]);
}
