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
#ifndef CONNECTIONPOINT_H
#define CONNECTIONPOINT_H

#include "diatypes.h"
#include <glib.h>

#define CONNECTIONPOINT_SIZE 5
#define CHANGED_TRESHOLD 0.001

#include "geometry.h"
#include "object.h"

/* Connections directions, used as hints to zigzaglines */
#define DIR_NORTH 1
#define DIR_SOUTH 2
#define DIR_EAST  4
#define DIR_WEST  8

struct _ConnectionPoint {
  Point pos;         /* position of this connection point */
  Point last_pos;    /* Used by update_connections_xxx only. */
  Object *object;    /* pointer to the object having this point */
  GList *connected;  /* list of 'Object *' connected to this point*/
  gchar directions;  /* Directions that this connection point is open to */
};

#endif /* CONNECTIONPOINT_H */







