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

#include <graphene.h>

#include "diatypes.h"
#include "geometry.h"


/**
 * HandleId:
 * @HANDLE_RESIZE_NW: North/west or top/left
 * @HANDLE_RESIZE_N: North or top
 * @HANDLE_RESIZE_NE: North/east or top/right
 * @HANDLE_RESIZE_W: West or left
 * @HANDLE_RESIZE_E: East or right
 * @HANDLE_RESIZE_SW: South/west or bottom/left
 * @HANDLE_RESIZE_S: South or bottom
 * @HANDLE_RESIZE_SE: South/east or bottom/right
 * @HANDLE_MOVE_STARTPOINT: For lines: the beginning
 * @HANDLE_MOVE_ENDPOINT: For lines: the ending
 * @HANDLE_CUSTOM1: For custom use by objects
 * @HANDLE_CUSTOM2: For custom use by objects
 * @HANDLE_CUSTOM3: For custom use by objects
 * @HANDLE_CUSTOM4: For custom use by objects
 * @HANDLE_CUSTOM5: For custom use by objects
 * @HANDLE_CUSTOM6: For custom use by objects
 * @HANDLE_CUSTOM7: For custom use by objects
 * @HANDLE_CUSTOM8: For custom use by objects
 * @HANDLE_CUSTOM9: For custom use by objects
 *
 * Some object resizing depends on the placement of the handle
 */
typedef enum {
  HANDLE_RESIZE_NW,
  HANDLE_RESIZE_N,
  HANDLE_RESIZE_NE,
  HANDLE_RESIZE_W,
  HANDLE_RESIZE_E,
  HANDLE_RESIZE_SW,
  HANDLE_RESIZE_S,
  HANDLE_RESIZE_SE,
  HANDLE_MOVE_STARTPOINT,
  HANDLE_MOVE_ENDPOINT,

  HANDLE_CUSTOM1 = 200,
  HANDLE_CUSTOM2,
  HANDLE_CUSTOM3,
  HANDLE_CUSTOM4,
  HANDLE_CUSTOM5,
  HANDLE_CUSTOM6,
  HANDLE_CUSTOM7,
  HANDLE_CUSTOM8,
  HANDLE_CUSTOM9,
} HandleId;


/**
 * HandleType:
 * @HANDLE_NON_MOVABLE: ???
 * @HANDLE_MAJOR_CONTROL: ???
 * @HANDLE_MINOR_CONTROL: ???
 * @NUM_HANDLE_TYPES: Must be last
 *
 * HandleType is used for color coding the different handles
 */
typedef enum {
  HANDLE_NON_MOVABLE,
  HANDLE_MAJOR_CONTROL,
  HANDLE_MINOR_CONTROL,
  NUM_HANDLE_TYPES,
} HandleType;


/**
 * HandleMoveReason:
 * @HANDLE_MOVE_USER: ???
 * @HANDLE_MOVE_USER_FINAL: ???
 * @HANDLE_MOVE_CONNECTED: ???
 * @HANDLE_MOVE_CREATE: The initial drag during object placement
 * @HANDLE_MOVE_CREATE_FINAL: Finish of initial drag
 *
 * When an objects dia_object_move_handle() function is called this is passed in
 */
typedef enum {
  HANDLE_MOVE_USER,
  HANDLE_MOVE_USER_FINAL,
  HANDLE_MOVE_CONNECTED,
  HANDLE_MOVE_CREATE,
  HANDLE_MOVE_CREATE_FINAL,
} HandleMoveReason;


/**
 * HandleConnectType:
 * @HANDLE_NONCONNECTABLE: Not connectable
 * @HANDLE_CONNECTABLE: Connectable
 * @HANDLE_CONNECTABLE_NOBREAK: (unused) Don't break connection on object move
 *
 * If the handle is connectable or not
 */
typedef enum {
  HANDLE_NONCONNECTABLE,
  HANDLE_CONNECTABLE,
  HANDLE_CONNECTABLE_NOBREAK,
} HandleConnectType;


/**
 * Handle:
 * @id: Gives (mostly) the placement relative to the object
 * @type: Colour coding
 * @pos: Where the handle currently is in diagram coordinates
 * @connect_type: How to connect if at all
 * @connected_to: %NULL if not connected
 *
 * A handle is used to resize objects or to connet to them.
 */
struct _Handle {
  HandleId           id;
  HandleType         type;
  Point              pos;
  HandleConnectType  connect_type;
  ConnectionPoint   *connected_to;
};


void dia_handle_set_position (Handle            *self,
                              graphene_point_t  *point);
