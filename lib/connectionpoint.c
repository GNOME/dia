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

/**
 * SECTION:connectionpoint
 * @title: ConnectionPoint
 * @short_description: Connections between #DiaObjects
 *
 * #ConnectionPoints together with #Handles allow to connect objects
 */

/**
 * find_slope_directions:
 * @from: Beginning of the slope line.
 * @to: End of the slope line.
 *
 * Returns the available directions on a slope.
 * The right-hand side of the line is assumed to be within the object,
 * and thus not available.
 *
 * Returns: The directions open (see #ConnectionPointDirection) open on the left-hand
 *  side of the slope.
 */
gint
find_slope_directions (Point from, Point to)
{
  gint dirs;
  gint slope;

  if (fabs (from.y-to.y) < 0.0000001)
    return (from.x > to.x?DIR_SOUTH:DIR_NORTH);
  if (fabs (from.x-to.x) < 0.0000001)
    return (from.y > to.y?DIR_WEST:DIR_EAST);

  point_sub (&to, &from);
  slope = fabs (to.y/to.x);

  dirs = 0;
  if (slope < 2) { /* Flat enough to allow north-south */
    if (to.x > 0) { /* The outside is north, not south */
      dirs |= DIR_NORTH;
    } else {
      dirs |= DIR_SOUTH;
    }
  }
  if (slope > .5) { /* Steep enough to allow east-west */
    if (to.y > 0) {  /* The outside is east, not west */
      dirs |= DIR_EAST;
    } else {
      dirs |= DIR_WEST;
    }
  }
  return dirs;
}


/**
 * connpoint_update:
 * @p: A ConnectionPoint pointer (non-%NULL).
 * @x: The x coordinate of the connectionpoint.
 * @y: The y coordinate of the connectionpoint.
 * @dirs: The directions that are open for connections on this point.
 *
 * Update the object-settable parts of a connectionpoints.
 */
void
connpoint_update (ConnectionPoint *p, real x, real y, gint dirs)
{
  p->pos.x = x;
  p->pos.y = y;
  p->directions = dirs;
}

/**
 * connpoint_is_autogap:
 * @cp: A #ConnectionPoint.
 *
 * Returns %TRUE if the given connection point is non-null and autogapped.
 *
 * Returns: %TRUE if the given connection point is non-null and has the
 *  %CP_FLAG_AUTOGAP flag set (i.e. lines connecting to it should try to end
 *  at the border of the object instead of at the connection point.
 */
gboolean
connpoint_is_autogap (ConnectionPoint *cp)
{
  return cp != NULL && (cp->flags & CP_FLAG_AUTOGAP) && (cp->connected != NULL);
}
