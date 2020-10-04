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

#pragma once

#include "diatypes.h"
#include "object.h"
#include "boundingbox.h"


/**
 * Connection:
 *
 * Base class for simple (straight) lines connecting object
 *
 * This is a subclass of #DiaObject used to help implementing objects
 * of a line-like type.
 */
struct _Connection {
  DiaObject object;

  Point endpoints[2]; /*!< start and end position */
  Handle endpoint_handles[2]; /*!< start and end handles */
  LineBBExtras extra_spacing; /*!< calcualted bounding box */
};


void             connection_update_handles     (Connection       *conn);
void             connection_update_boundingbox (Connection       *conn);
void             connection_init               (Connection       *conn,
                                                int               num_handles,
                                                int               num_connections);
void             connection_destroy            (Connection       *conn);
void             connection_copy               (Connection       *from,
                                                Connection       *to);
void             connection_save               (Connection       *conn,
                                                ObjectNode        obj_node,
                                                DiaContext       *ctx);
void             connection_load               (Connection       *conn,
                                                ObjectNode        obj_node,
                                                DiaContext       *ctx);
DiaObjectChange* connection_move_handle        (Connection       *conn,
                                                HandleId          id,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
void             connection_adjust_for_autogap (Connection       *conn);

/* base property stuff... */
#define CONNECTION_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "conn_endpoints", PROP_TYPE_ENDPOINTS, 0, "Connection endpoints", NULL}

#define CONNECTION_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "conn_endpoints", PROP_TYPE_ENDPOINTS, offsetof(Connection,endpoints)}
