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

/*!
 * \file connectionpoint.h -- Connection Points together with Handles allow to connect objects
 * \ingroup ObjectConnects
 */
#ifndef CONNECTIONPOINT_H
#define CONNECTIONPOINT_H

#include "diatypes.h"
#include <glib.h>
#include "geometry.h"

/*! \brief Connections directions, used as hints to e.g. zigzaglines 
 * Ordered this way to let *2 be rotate clockwise, /2 rotate counterclockwise.
 * Used as bits
 * \ingroup ObjectConnects
 */
typedef enum {
  DIR_NONE  = 0,
  DIR_NORTH = (1<<0),
  DIR_EAST  = (1<<1),
  DIR_SOUTH = (1<<2),
  DIR_WEST  = (1<<3),
  /* Convenience directions */
  DIR_NORTHEAST = (DIR_NORTH|DIR_EAST),
  DIR_SOUTHEAST = (DIR_SOUTH|DIR_EAST),
  DIR_NORTHWEST = (DIR_NORTH|DIR_WEST),
  DIR_SOUTHWEST = (DIR_SOUTH|DIR_WEST),
  DIR_ALL       = (DIR_NORTH|DIR_SOUTH|DIR_EAST|DIR_WEST)
} ConnectionPointDirection;

/*!
 * \brief Additional behaviour flags for connection points
 * \ingroup ObjectConnects
 */
typedef enum {
  CP_FLAG_ANYPLACE = (1<<0), /*!< Set if this connpoint is the one that
			          is connected to when a connection is
			          dropped on an object. */
  CP_FLAG_AUTOGAP =  (1<<1), /*!< Set if this connpoint is internal
			          and so should force a gap on the lines. */

  /*! Most non-connection objects want exactly one CP with this, in the middle. */
  CP_FLAGS_MAIN	=   (CP_FLAG_ANYPLACE|CP_FLAG_AUTOGAP) /*!< Use this for the central CP that
							    takes connections from all over the
							    object and has autogap. */
} ConnectionPointFlags;

/*!
 * \brief To connect object with other objects handles
 * \ingroup ObjectConnects
 */
struct _ConnectionPoint {
  Point pos;         /*!< position of this connection point */
  DiaObject *object; /*!< pointer to the object having this point */
  GList *connected;  /*!< list of 'DiaObject *' connected to this point*/
  guint8 directions; /*!< Directions that this connection point is open to */
  guint8 flags;      /*!< Flags set for this connpoint.  See CP_FLAGS_* above. */
};

/** 
 * \brief Returns the available directions on a slope.
 * The right-hand side of the line is assumed to be within the object,
 * and thus not available. 
 * \ingroup ObjectConnects
 */
gint find_slope_directions(Point from, Point to);
/*!
 * \brief Update the object-settable parts of a connectionpoints.
 * @param p A ConnectionPoint pointer (non-NULL).
 * @param x The x coordinate of the connectionpoint.
 * @param y The y coordinate of the connectionpoint.
 * @param dirs The directions that are open for connections on this point.
 * \ingroup ObjectConnects
 */
void connpoint_update(ConnectionPoint *p, real x, real y, gint dirs);
/*!
 * \brief Adaptive rendering to the boundary of the connected object
 * \ingroup ObjectConnects
 */
gboolean connpoint_is_autogap(ConnectionPoint *cp);


#endif /* CONNECTIONPOINT_H */
