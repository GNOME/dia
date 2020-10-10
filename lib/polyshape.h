/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#pragma once

#include "diatypes.h"
#include "object.h"
#include "boundingbox.h"

G_BEGIN_DECLS

#define DIA_TYPE_POLY_SHAPE_OBJECT_CHANGE dia_poly_shape_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaPolyShapeObjectChange,
                      dia_poly_shape_object_change,
                      DIA, POLY_SHAPE_OBJECT_CHANGE,
                      DiaObjectChange)


#define HANDLE_CORNER (HANDLE_CUSTOM1)

/*!
 * \brief Helper class for polygon shaped objects
 *
 * This is a subclass of DiaObject used to help implementing objects
 * that form a polygon-like shape of line-segments.
 *
 * \extends _DiaObject
 */
struct _PolyShape {
  /* DiaObject must be first because this is a 'subclass' of it. */
  DiaObject object;

  int numpoints; /* >= 3 */
  Point *points;

  ElementBBExtras extra_spacing;
};

void polyshape_update_data(PolyShape *poly);
void polyshape_update_boundingbox(PolyShape *poly);
void polyshape_init(PolyShape *poly, int num_points);
void polyshape_set_points(PolyShape *poly, int num_points, Point *points);
void polyshape_destroy(PolyShape *poly);
void polyshape_copy(PolyShape *from, PolyShape *to);
void polyshape_save(PolyShape *poly, ObjectNode obj_node, DiaContext *ctx);
void polyshape_load(PolyShape *poly, ObjectNode obj_node, DiaContext *ctx);  /* NOTE: Does object_init() */
DiaObjectChange *polyshape_add_point        (PolyShape        *poly,
                                             int               segment,
                                             Point            *point);
DiaObjectChange *polyshape_remove_point     (PolyShape        *poly,
                                             int               point);
DiaObjectChange *polyshape_move_handle      (PolyShape        *poly,
                                             Handle           *id,
                                             Point            *to,
                                             ConnectionPoint  *cp,
                                             HandleMoveReason  reason,
                                             ModifierKeys      modifiers);
DiaObjectChange *polyshape_move             (PolyShape        *poly,
                                             Point            *to);
real polyshape_distance_from(PolyShape *poly, Point *point,
			     real line_width);
Handle *polyshape_closest_handle(PolyShape *poly, Point *point);
int polyshape_closest_segment(PolyShape *poly, Point *point,
			      real line_width);

#define POLYSHAPE_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "poly_points", PROP_TYPE_POINTARRAY, 0, "polyshape points", NULL} \

#define POLYSHAPE_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "poly_points", PROP_TYPE_POINTARRAY, \
     offsetof(PolyShape,points), offsetof(PolyShape,numpoints)} \

G_END_DECLS
