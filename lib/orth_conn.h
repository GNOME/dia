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
#ifndef ORTH_CONN_H
#define ORTH_CONN_H

#include "object.h"

typedef enum {
  HORIZONTAL,
  VERTICAL
} Orientation;

#define FLIP_ORIENT(x) (((x)==HORIZONTAL)?VERTICAL:HORIZONTAL)

#define HANDLE_MIDPOINT (HANDLE_CUSTOM1)

typedef struct _OrthConn OrthConn;

/* This is a subclass of Object used to help implementing objects
 * that connect points with orthogonal line-segments.
 */
struct _OrthConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 3 */
  Point *points;
  Handle endpoint_handles[2];
  Orientation *orientation; /*[numpoints - 1]*/
  Handle **midpoint_handles; /*[numpoints - 1]*/
};
extern void orthconn_update_data(OrthConn *orth);
extern void orthconn_update_boundingbox(OrthConn *orth);
extern void orthconn_simple_draw(OrthConn *orth, Renderer *renderer,
				 real width);
extern void orthconn_init(OrthConn *orth, Point *startpoint);
extern void orthconn_destroy(OrthConn *orth);
extern void orthconn_copy(OrthConn *from, OrthConn *to);
extern void orthconn_save(OrthConn *orth, ObjectNode obj_node);
extern void orthconn_load(OrthConn *orth, ObjectNode obj_node);  /* NOTE: Does object_init() */
extern void orthconn_move_handle(OrthConn *orth, Handle *id,
				 Point *to, HandleMoveReason reason);
extern void orthconn_move(OrthConn *orth, Point *to);
extern real orthconn_distance_from(OrthConn *orth, Point *point,
				   real line_width);
#endif /* ORTH_CONN_H */
