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
typedef struct _PolyConnState PolyConnState;

/* This is a subclass of Object used to help implementing objects
 * that connect points with polygonal line-segments.
 */
struct _PolyConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 2 */
  Point *points;
};

struct _PolyConnState {
  int numpoints;
  Point *points;
};


extern void polyconn_update_data(PolyConn *poly);
extern void polyconn_update_boundingbox(PolyConn *poly);
extern void polyconn_simple_draw(PolyConn *poly, Renderer *renderer,
				 real width);
extern void polyconn_init(PolyConn *poly);
extern void polyconn_destroy(PolyConn *poly);
extern void polyconn_copy(PolyConn *from, PolyConn *to);
extern void polyconn_save(PolyConn *poly, ObjectNode obj_node);
extern void polyconn_load(PolyConn *poly, ObjectNode obj_node);  /* NOTE: Does object_init() */
extern void polyconn_add_point(PolyConn *poly, int segment, Point *point);
extern void polyconn_remove_point(PolyConn *poly, int point);
extern void polyconn_move_handle(PolyConn *poly, Handle *id,
				 Point *to, HandleMoveReason reason);
extern void polyconn_move(PolyConn *poly, Point *to);
extern real polyconn_distance_from(PolyConn *poly, Point *point,
				   real line_width);
extern Handle *polyconn_closest_handle(PolyConn *poly, Point *point);
extern int polyconn_closest_segment(PolyConn *poly, Point *point,
				    real line_width);
extern void polyconn_state_get(PolyConnState *state, PolyConn *poly);
extern void polyconn_state_set(PolyConnState *state, PolyConn *poly);
extern void polyconn_state_free(PolyConnState *state);
#endif /* POLY_CONN_H */
