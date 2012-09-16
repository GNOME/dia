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

/*! \file create.h contains user_data structures for creating the non-trivial 
 *   standard objects (polylines & polygons).
 * \defgroup ObjectCreate Creation of standard objects
 * \ingroup StandardObjects
 *
 * Typical import plugins translate some vector representation of the import format
 * into _DiaObject representations. This set of functions and structures simplifies 
 * the creation of standard objects.
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

/*! Create a text object for the diagram.
 * @param xpos X position (in cm from the origo) of the object.
 * @param ypos Y position (in cm from the origo) of the object.
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_text(real xpos, real ypos);
/*! Create an ellipse object for the diagram
 * @param xpos top-left corner
 * @param ypos top-lef corner
 * @param width the horizontal diameter
 * @param height the vertical diameter
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_ellipse(real xpos, real ypos, real width, real height);
/*! Create a rectangular box */
DiaObject *create_standard_box(real xpos, real ypos, real width, real height);
/*! Create a _Polyline with arrows */
DiaObject *create_standard_polyline(int num_points,  Point *points,
				    Arrow *end_arrow, Arrow *start_arrow);
/*! Create an _OrthConn with arrows */
DiaObject *create_standard_zigzagline(int num_points, const Point *points,
				      const Arrow *end_arrow, const Arrow *start_arrow);
DiaObject *create_standard_polygon(int num_points, Point *points);
/*! Create a _Bezierline with arrows */
DiaObject *create_standard_bezierline(int num_points, BezPoint *points,
				      Arrow *end_arrow, Arrow *start_arrow);
DiaObject *create_standard_beziergon(int num_points, BezPoint *points);
/*! Create an _Arc with arrows 
 * @param x1 arc start position
 * @param y1 arc start position
 * @param x2 arc end position
 * @param y2 arc end position
 * @param curve_distance perpendicular distance from the center of the chord to the circumference
 * @param end_arrow end _Arrow
 * @param start_arrow start _Arrow
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_arc(real x1, real y1, real x2, real y2,
			       real curve_distance, 
			       Arrow *end_arrow, Arrow *start_arrow);
/*! Create an image object from file with the given size and position 
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_image(real xpos, real ypos, real width, real height, char *file);
/*! Create a _Group of objects given by list.
 * The objects in list must not be added to the diagra at the same time.
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_group(GList *items);
#endif
