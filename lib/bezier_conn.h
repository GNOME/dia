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

typedef struct _BezierConn BezierConn;

/* This is a subclass of Object used to help implementing objects
 * that connect points with polygonal line-segments.
 */
struct _BezierConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 2 */
  BezPoint *points;
};

extern void bezierconn_update_data(BezierConn *bez);
extern void bezierconn_update_boundingbox(BezierConn *bez);
extern void bezierconn_simple_draw(BezierConn *bez, Renderer *renderer,
				 real width);
extern void bezierconn_init(BezierConn *bez);
extern void bezierconn_destroy(BezierConn *bez);
extern void bezierconn_copy(BezierConn *from, BezierConn *to);
extern void bezierconn_save(BezierConn *bez, ObjectNode obj_node);
extern void bezierconn_load(BezierConn *bez, ObjectNode obj_node);  /* NOTE: Does object_init() */
extern ObjectChange *bezierconn_add_segment(BezierConn *bez, int segment, BezPoint *point);
extern ObjectChange *bezierconn_remove_segment(BezierConn *bez, int point);
extern void bezierconn_move_handle(BezierConn *bez, Handle *id,
				 Point *to, HandleMoveReason reason);
extern void bezierconn_move(BezierConn *bez, Point *to);
extern real bezierconn_distance_from(BezierConn *bez, Point *point,
				   real line_width);
extern Handle *bezierconn_closest_handle(BezierConn *bez, Point *point);
extern int bezierconn_closest_segment(BezierConn *bez, Point *point,
				    real line_width);
#endif /* BEZIER_CONN_H */
