/* Dia -- an diagram creation/manipulation program -*- c -*- 
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
#include "boundingbox.h"

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
  Point *points; /* [numpoints] */

  int numorient; /* always numpoints-1 */
  Orientation *orientation; /*[numpoints - 1]*/

  int numhandles; /* should be == numorient */
  Handle **handles; /*[numpoints - 1] */
  /* Each line segment has one handle. The first and last handles
   * are placed in the end of their segment, the other in the middle.
   * The end handles are connectable, the others not. (That would be
   * problematic, as they can only move freely in one direction.)
   * The array of pointers is ordered in segment order.
   */
  PolyBBExtras extra_spacing;
};

void orthconn_update_data(OrthConn *orth);
void orthconn_update_boundingbox(OrthConn *orth);
void orthconn_simple_draw(OrthConn *orth, DiaRenderer *renderer,
			  real width);
void orthconn_init(OrthConn *orth, Point *startpoint);
void orthconn_destroy(OrthConn *orth);
void orthconn_copy(OrthConn *from, OrthConn *to);
void orthconn_save(OrthConn *orth, ObjectNode obj_node);
void orthconn_load(OrthConn *orth, ObjectNode obj_node);  /* NOTE: Does object_init() */
void orthconn_move_handle(OrthConn *orth, Handle *id,
			  Point *to, HandleMoveReason reason);
void orthconn_move(OrthConn *orth, Point *to);
real orthconn_distance_from(OrthConn *orth, Point *point,
			    real line_width);
Handle* orthconn_get_middle_handle(OrthConn *orth);

int orthconn_can_delete_segment(OrthConn *orth, Point *clickedpoint);
int orthconn_can_add_segment(OrthConn *orth, Point *clickedpoint);
ObjectChange *orthconn_delete_segment(OrthConn *orth, Point *clickedpoint);
ObjectChange *orthconn_add_segment(OrthConn *orth, Point *clickedpoint);

/* base property stuff... */
#define ORTHCONN_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "orth_points", PROP_TYPE_POINTARRAY, 0, "orthconn points", NULL}, \
  { "orth_orient", PROP_TYPE_ENUMARRAY, 0, "orthconn orientations", NULL} \

#define ORTHCONN_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "orth_points", PROP_TYPE_POINTARRAY, \
     offsetof(OrthConn,points), offsetof(OrthConn,numpoints)}, \
  { "orth_orient", PROP_TYPE_ENUMARRAY, \
     offsetof(OrthConn,orientation), offsetof(OrthConn,numorient)} \

#endif /* ORTH_CONN_H */


