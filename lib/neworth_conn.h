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
#ifndef NEWORTH_CONN_H
#define NEWORTH_CONN_H

#include "object.h"
#include "connpoint_line.h"
#include "orth_conn.h"

#if 0
typedef enum {
  HORIZONTAL,
  VERTICAL
} Orientation;

#define FLIP_ORIENT(x) (((x)==HORIZONTAL)?VERTICAL:HORIZONTAL)

#define HANDLE_MIDPOINT (HANDLE_CUSTOM1)
#endif 


typedef struct _NewOrthConn NewOrthConn;

/* This is a subclass of Object used to help implementing objects
 * that connect points with orthogonal line-segments.
 */
struct _NewOrthConn {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;

  int numpoints; /* >= 3 */
  Point *points; /* [numpoints] */
  Orientation *orientation; /*[numpoints - 1]*/
  Handle **handles; /*[numpoints - 1] */
  /* Each line segment has one handle. The first and last handles
   * are placed in the end of their segment, the other in the middle.
   * The end handles are connectable, the others not. (That would be
   * problematic, as they can only move freely in one direction.)
   * The array of pointers is ordered in segment order.
   */
  ConnPointLine *midpoints;
};

void neworthconn_update_data(NewOrthConn *orth);
void neworthconn_update_boundingbox(NewOrthConn *orth);
void neworthconn_simple_draw(NewOrthConn *orth, Renderer *renderer,
			     real width);
void neworthconn_init(NewOrthConn *orth, Point *startpoint);
void neworthconn_destroy(NewOrthConn *orth);
void neworthconn_copy(NewOrthConn *from, NewOrthConn *to);
void neworthconn_save(NewOrthConn *orth, ObjectNode obj_node);
void neworthconn_load(NewOrthConn *orth, ObjectNode obj_node);  /* NOTE: Does object_init() */
void neworthconn_move_handle(NewOrthConn *orth, Handle *id,
			     Point *to, HandleMoveReason reason);
void neworthconn_move(NewOrthConn *orth, Point *to);
real neworthconn_distance_from(NewOrthConn *orth, Point *point,
			       real line_width);
Handle* neworthconn_get_middle_handle(NewOrthConn *orth);

int neworthconn_can_delete_segment(NewOrthConn *orth, Point *clickedpoint);
int neworthconn_can_add_segment(NewOrthConn *orth, Point *clickedpoint);
ObjectChange *neworthconn_delete_segment(NewOrthConn *orth, Point *clickedpoint);
ObjectChange *neworthconn_add_segment(NewOrthConn *orth, Point *clickedpoint);
#endif /* NEWORTH_CONN_H */


