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
#ifndef POLY_CONN_H
#define POLY_CONN_H

#include "diatypes.h"
#include "object.h"
#include "boundingbox.h"

#define HANDLE_CORNER (HANDLE_CUSTOM1)

/*!
 * \brief helper for implementing polyline connections
 * 
 * This is a subclass of DiaObject used to help implementing objects
 * that connect points with polygonal line-segments.
 *
 * \extends _DiaObject
 */
struct _PolyConn {
  /* DiaObject must be first because this is a 'subclass' of it. */
  DiaObject object;

  int numpoints; /* >= 2 */
  Point *points;

  PolyBBExtras extra_spacing;
};


void polyconn_update_data(PolyConn *poly);
void polyconn_update_boundingbox(PolyConn *poly);
void polyconn_init(PolyConn *poly, int num_points);
void polyconn_set_points(PolyConn *poly, int num_points, Point *points);
void polyconn_destroy(PolyConn *poly);
void polyconn_copy(PolyConn *from, PolyConn *to);
void polyconn_save(PolyConn *poly, ObjectNode obj_node, DiaContext *ctx);
void polyconn_load(PolyConn *poly, ObjectNode obj_node, DiaContext *ctx);  /* NOTE: Does object_init() */
ObjectChange *polyconn_add_point(PolyConn *poly, int segment, Point *point);
ObjectChange *polyconn_remove_point(PolyConn *poly, int point);
ObjectChange *polyconn_move_handle(PolyConn *poly, Handle *id,
				   Point *to, ConnectionPoint *cp,
				   HandleMoveReason reason,
				   ModifierKeys modifiers);
ObjectChange *polyconn_move(PolyConn *poly, Point *to);
real polyconn_distance_from(PolyConn *poly, Point *point,
			    real line_width);
Handle *polyconn_closest_handle(PolyConn *poly, Point *point);
int polyconn_closest_segment(PolyConn *poly, Point *point,
			     real line_width);
/* base property stuff... */
#define POLYCONN_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "poly_points", PROP_TYPE_POINTARRAY, 0, "polconn points", NULL} \

#define POLYCONN_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "poly_points", PROP_TYPE_POINTARRAY, \
     offsetof(PolyConn,points), offsetof(PolyConn,numpoints)} \

#endif /* POLY_CONN_H */
