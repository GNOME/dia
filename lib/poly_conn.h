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

#include "object.h"

#define HANDLE_CORNER (HANDLE_CUSTOM1)

typedef struct _PolyConn PolyConn;

/* This is a subclass of Object used to help implementing objects
 * that connect points with polygonal line-segments.
 */
struct _PolyConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 2 */
  Point *points;
};

void polyconn_update_data(PolyConn *poly);
void polyconn_update_boundingbox(PolyConn *poly);
void polyconn_simple_draw(PolyConn *poly, Renderer *renderer, real width);
void polyconn_init(PolyConn *poly);
void polyconn_destroy(PolyConn *poly);
void polyconn_copy(PolyConn *from, PolyConn *to);
void polyconn_save(PolyConn *poly, ObjectNode obj_node);
void polyconn_load(PolyConn *poly, ObjectNode obj_node);  /* NOTE: Does object_init() */
ObjectChange *polyconn_add_point(PolyConn *poly, int segment, Point *point);
ObjectChange *polyconn_remove_point(PolyConn *poly, int point);
void polyconn_move_handle(PolyConn *poly, Handle *id,
			  Point *to, HandleMoveReason reason);
void polyconn_move(PolyConn *poly, Point *to);
real polyconn_distance_from(PolyConn *poly, Point *point,
			    real line_width);
Handle *polyconn_closest_handle(PolyConn *poly, Point *point);
int polyconn_closest_segment(PolyConn *poly, Point *point,
			     real line_width);
#endif /* POLY_CONN_H */
