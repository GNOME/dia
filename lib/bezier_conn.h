/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * BezierConn  Copyright (C) 1999 James Henstridge
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
#ifndef BEZIER_CONN_H
#define BEZIER_CONN_H

#include "object.h"
#include "boundingbox.h"

typedef struct _BezierConn BezierConn;

typedef enum {
  BEZ_CORNER_SYMMETRIC,
  BEZ_CORNER_SMOOTH,
  BEZ_CORNER_CUSP
} BezCornerType;

/* This is a subclass of Object used to help implementing objects
 * that connect points with polygonal line-segments.
 */
struct _BezierConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 2 */
  BezPoint *points;
  BezCornerType *corner_types;
  PolyBBExtras extra_spacing;
};

void bezierconn_update_data(BezierConn *bez);
void bezierconn_update_boundingbox(BezierConn *bez);
void bezierconn_simple_draw(BezierConn *bez, DiaRenderer *renderer,
			    real width);
void bezierconn_draw_control_lines(BezierConn *bez, DiaRenderer *renderer);
void bezierconn_init(BezierConn *bez, int num_points);
void bezierconn_set_points(BezierConn *poly, int num_points, BezPoint *points);
void bezierconn_destroy(BezierConn *bez);
void bezierconn_copy(BezierConn *from, BezierConn *to);
void bezierconn_save(BezierConn *bez, ObjectNode obj_node);
void bezierconn_load(BezierConn *bez, ObjectNode obj_node);  /* NOTE: Does object_init() */
ObjectChange *bezierconn_add_segment(BezierConn *bez, int segment, Point *point);
ObjectChange *bezierconn_remove_segment(BezierConn *bez, int point);
ObjectChange *bezierconn_set_corner_type(BezierConn *bez,
					 Handle *handle,
					 BezCornerType style);
void bezierconn_move_handle(BezierConn *bez, Handle *id,
			    Point *to, HandleMoveReason reason);
void bezierconn_move(BezierConn *bez, Point *to);
real bezierconn_distance_from(BezierConn *bez, Point *point,
			      real line_width);
Handle *bezierconn_closest_handle(BezierConn *bez, Point *point);
Handle *bezierconn_closest_major_handle(BezierConn *bez, Point *point);
int bezierconn_closest_segment(BezierConn *bez, Point *point,
			       real line_width);
#endif /* BEZIER_CONN_H */
