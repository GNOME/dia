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
/*! \file connectionpoint.h -- Connection Points together with Handles allow to connect objects */
#ifndef CONNECTIONPOINT_H
#define CONNECTIONPOINT_H

#include "diatypes.h"
#include <glib.h>
#include "geometry.h"

#define CONNECTIONPOINT_SIZE 5
#define CHANGED_TRESHOLD 0.001

/* Connections directions, used as hints to zigzaglines */
/* Ordered this way to let *2 be rotate clockwise, /2 rotate counterclockwise.
 * Used as bits */
#define DIR_NORTH 1
#define DIR_EAST  2
#define DIR_SOUTH 4
#define DIR_WEST  8

/* Convenience directions */
#define DIR_NORTHEAST (DIR_NORTH|DIR_EAST)
#define DIR_SOUTHEAST (DIR_SOUTH|DIR_EAST)
#define DIR_NORTHWEST (DIR_NORTH|DIR_WEST)
#define DIR_SOUTHWEST (DIR_SOUTH|DIR_WEST)
#define DIR_NONE      0
#define DIR_ALL       (DIR_NORTH|DIR_SOUTH|DIR_EAST|DIR_WEST)

#define CP_FLAG_ANYPLACE	1 /*!< Set if this connpoint is the one that
				     is connected to when a connection is
				     dropped on an object. */
#define CP_FLAG_AUTOGAP		2 /*!< Set if this connpoint is internal
				     and so should force a gap on the lines. */

/*! Most non-connection objects want exactly one CP with this, in the middle. */
#define CP_FLAGS_MAIN		3 /*!< Use this for the central CP that
				     takes connections from all over the
				     object and has autogap. */

/*!
 * \brief To connect object with other objects handles
 */
struct _ConnectionPoint {
  Point pos;         /*!< position of this connection point */
  Point last_pos;    /*!< Used by update_connections_xxx only. */
  DiaObject *object; /*!< pointer to the object having this point */
  GList *connected;  /*!< list of 'DiaObject *' connected to this point*/
  gchar directions;  /*!< Directions that this connection point is open to */
  gchar *name;       /*!< Name of this connpoint, NULL means uses number only.*/
  guint8 flags;      /*!< Flags set for this connpoint.  See CP_FLAGS_* above. */
};

/** 
 * Returns the available directions on a slope.
 * The right-hand side of the line is assumed to be within the object,
 * and thus not available. 
 */
gint find_slope_directions(Point from, Point to);
/** Update the object-settable parts of a connectionpoints.
 * p: A ConnectionPoint pointer (non-NULL).
 * x: The x coordinate of the connectionpoint.
 * y: The y coordinate of the connectionpoint.
 * dirs: The directions that are open for connections on this point.
 */
void connpoint_update(ConnectionPoint *p, real x, real y, gint dirs);

gboolean connpoint_is_autogap(ConnectionPoint *cp);


#endif /* CONNECTIONPOINT_H */
