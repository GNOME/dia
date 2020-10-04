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

#pragma once

#include "diatypes.h"
#include "object.h"
#include "boundingbox.h"
#include "bezier-common.h"

G_BEGIN_DECLS

#define DIA_TYPE_BEZIER_CONN_POINT_OBJECT_CHANGE dia_bezier_conn_point_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaBezierConnPointObjectChange, dia_bezier_conn_point_object_change, DIA, BEZIER_CONN_POINT_OBJECT_CHANGE, DiaObjectChange)


#define DIA_TYPE_BEZIER_CONN_CORNER_OBJECT_CHANGE dia_bezier_conn_corner_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaBezierConnCornerObjectChange, dia_bezier_conn_corner_object_change, DIA, BEZIER_CONN_CORNER_OBJECT_CHANGE, DiaObjectChange)


/*!
 * \brief Helper class to implement bezier connections
 *
 * This is a subclass of DiaObject used to help implementing objects
 * that connect points with polygonal line-segments.
 *
 * \extends _DiaObject
 */
struct _BezierConn {
  DiaObject object; /**< inheritance */

  /*! Common bezier object stuff */
  BezierCommon bezier;

  PolyBBExtras extra_spacing;
};


void             bezierconn_update_data          (BezierConn       *bezier);
void             bezierconn_update_boundingbox   (BezierConn       *bezier);
void             bezierconn_init                 (BezierConn       *bezier,
                                                  int               num_points);
void             bezierconn_destroy              (BezierConn       *bezier);
void             bezierconn_copy                 (BezierConn       *from,
                                                  BezierConn       *to);
void             bezierconn_save                 (BezierConn       *bezier,
                                                  ObjectNode        obj_node,
                                                  DiaContext       *ctx);
/* NOTE: Does object_init() */
void             bezierconn_load                 (BezierConn       *bezier,
                                                  ObjectNode        obj_node,
                                                  DiaContext       *ctx);
DiaObjectChange *bezierconn_add_segment          (BezierConn       *bezier,
                                                  int               segment,
                                                  Point            *point);
DiaObjectChange *bezierconn_remove_segment       (BezierConn       *bezier,
                                                  int               point);
DiaObjectChange *bezierconn_set_corner_type      (BezierConn       *bezier,
                                                  Handle           *handle,
                                                  BezCornerType     style);
DiaObjectChange *bezierconn_move_handle          (BezierConn       *bezier,
                                                  Handle           *handle,
                                                  Point            *to,
                                                  ConnectionPoint  *cp,
                                                  HandleMoveReason  reason,
                                                  ModifierKeys      modifiers);
DiaObjectChange *bezierconn_move                 (BezierConn       *bezier,
                                                  Point            *to);
double           bezierconn_distance_from        (BezierConn       *bezier,
                                                  Point            *point,
                                                  double            line_width);
Handle          *bezierconn_closest_handle       (BezierConn       *bezier,
                                                  Point            *point);
Handle          *bezierconn_closest_major_handle (BezierConn       *bezier,
                                                  Point            *point);


#define BEZCONN_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "bez_points", PROP_TYPE_BEZPOINTARRAY, 0, "bezierconn points", NULL} \

#define BEZCONN_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "bez_points", PROP_TYPE_BEZPOINTARRAY, \
     offsetof(BezierConn,bezier.points), offsetof(BezierConn,bezier.num_points)} \

G_END_DECLS
