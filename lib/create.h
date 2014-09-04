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

/*!
 * \defgroup ObjectCreate Creation of standard objects
 * \brief Helpers for creation of the non-trivial standard objects
 *
 * \ingroup StandardObjects
 *
 * Typical import plug-ins translate some vector representation of the import format
 * into _DiaObject representations. This set of functions and structures simplifies 
 * the creation of standard objects.
 */

#ifndef STANDARD_OBJECT_CREATE_H
#define STANDARD_OBJECT_CREATE_H

#include "diatypes.h"

#include <glib.h>

G_BEGIN_DECLS

/*!
 * \brief Can be used as extra parameter at create. Usually discouraged, you can set via StdProp API
 * \ingroup ObjectCreate
 */
struct _MultipointCreateData {
  int num_points; /**< count */
  Point *points; /**< data */
};

/*! 
 * \brief Can be used as extra parameter at create. Usually discouraged, you can set via StdProp API
 * \ingroup ObjectCreate
 */
struct _BezierCreateData {
  int num_points; /**< count */
  BezPoint *points; /**< data */
};

/*!
 * \brief Create a text object for the diagram.
 * @param xpos X position (in cm from the origin) of the object.
 * @param ypos Y position (in cm from the origin) of the object.
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_text(real xpos, real ypos);
/*!
 * \brief Create an ellipse object for the diagram
 * @param xpos top-left corner
 * @param ypos top-left corner
 * @param width the horizontal diameter
 * @param height the vertical diameter
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_ellipse(real xpos, real ypos, real width, real height);
/*!
 * \brief Create a rectangular box
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_box(real xpos, real ypos, real width, real height);
/*!
 * \brief Create a _Polyline with arrows
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_polyline(int num_points,  Point *points,
				    Arrow *end_arrow, Arrow *start_arrow);
/*!
 * \brief Create a \ref _Zigzagline with arrows
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_zigzagline(int num_points, const Point *points,
				      const Arrow *end_arrow, const Arrow *start_arrow);
/*!
 * \brief Create a \ref _Polygon
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_polygon(int num_points, Point *points);
/*!
 * \brief Create a \ref _Bezierline with arrows
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_bezierline(int num_points, BezPoint *points,
				      Arrow *end_arrow, Arrow *start_arrow);
/*!
 * \brief Create a \ref _Beziergon
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_beziergon(int num_points, BezPoint *points);
/*!
 * \brief Create a \ref _StdPath
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_path(int num_points, BezPoint *points);
/*!
 * \brief Create a \ref _StdPath from the given _Text
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_path_from_text (const Text *text);

/*!
 * \brief Create an \ref _Arc with arrows 
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
/*! 
 * \brief Create an \ref _Image object from file with the given size and position 
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_image(real xpos, real ypos, real width, real height, char *file);
/*!
 * \brief Create a _Group of objects given by list.
 * The objects in list must not be added to the diagram at the same time.
 * \ingroup ObjectCreate
 */
DiaObject *create_standard_group(GList *items);

DiaObject *create_standard_path_from_object (DiaObject *obj);

DiaObject *create_standard_path_from_list (GList *objects, PathCombineMode  mode);

G_END_DECLS

#endif
