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
#ifndef HANDLE_H
#define HANDLE_H

#include "diatypes.h"

/* Is this needed? */
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

  /* These handles can be used privately by objects: */
  HANDLE_CUSTOM1=200,
  HANDLE_CUSTOM2,
  HANDLE_CUSTOM3,
  HANDLE_CUSTOM4,
  HANDLE_CUSTOM5,
  HANDLE_CUSTOM6,
  HANDLE_CUSTOM7,
  HANDLE_CUSTOM8,
  HANDLE_CUSTOM9
} HandleId;

typedef enum {
  HANDLE_NON_MOVABLE,
  HANDLE_MAJOR_CONTROL,
  HANDLE_MINOR_CONTROL,

  NUM_HANDLE_TYPES /* Must be last */
}  HandleType;

typedef enum {
  HANDLE_MOVE_USER,
  HANDLE_MOVE_USER_FINAL,
  HANDLE_MOVE_CONNECTED,
  HANDLE_MOVE_CREATE,       /* the initial drag during object placement */
  HANDLE_MOVE_CREATE_FINAL  /* finish of initial drag */
} HandleMoveReason;

typedef enum {
  HANDLE_NONCONNECTABLE,
  HANDLE_CONNECTABLE,
  HANDLE_CONNECTABLE_NOBREAK /* Don't break connection on object move */
} HandleConnectType;

#include "geometry.h"
#include "object.h"

struct _Handle {
  HandleId id;
  HandleType type;
  Point pos;
  
  HandleConnectType connect_type;
  ConnectionPoint *connected_to; /* NULL if not connected */
};

  
#endif /* HANDLE_H */
