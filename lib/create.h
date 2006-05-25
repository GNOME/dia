/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2001 Lars Clausen
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

/** This file contains user_data structures for creating the non-trivial 
    standard objects (polylines & polygons).
*/

#ifndef STANDARD_OBJECT_CREATE_H
#define STANDARD_OBJECT_CREATE_H

#include "diatypes.h"

/** \brief Can be used as extra parameter at create. Usually discouraged, you can set via StdProp API */
struct _MultipointCreateData {
  int num_points; /**< count */
  Point *points; /**< data */
};

/** \brief Can be used as extra parameter at create. Usually discouraged, you can set via StdProp API */
struct _BezierCreateData {
  int num_points; /**< count */
  BezPoint *points; /**< data */
};

/** Create a text object for the diagram.
 * @param xpos X position (in cm from the origo) of the object.
 * @param ypos Y position (in cm from the origo) of the object.
 * @param 
 */
DiaObject *
create_standard_text(real xpos, real ypos);
DiaObject *
create_standard_ellipse(real xpos, real ypos, real width, real height);
DiaObject *
create_standard_box(real xpos, real ypos, real width, real height);
DiaObject *
create_standard_polyline(int num_points,  Point *points,
			 Arrow *end_arrow, Arrow *start_arrow);
DiaObject *
create_standard_polygon(int num_points, Point *points);
DiaObject *
create_standard_bezierline(int num_points, BezPoint *points,
			   Arrow *end_arrow, Arrow *start_arrow);
DiaObject *
create_standard_beziergon(int num_points, BezPoint *points);
DiaObject *
create_standard_arc(real x1, real y1, real x2, real y2,
		    real radius, 
		    Arrow *end_arrow, Arrow *start_arrow);
DiaObject *
create_standard_image(real xpos, real ypos, real width, real height,
		      char *file);
#endif
