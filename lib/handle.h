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

/*! 
 * \file handle.h - describing the different behavious of handles, used e.g. to resize objects
 * \ingroup ObjectConnects
 */
#ifndef HANDLE_H
#define HANDLE_H

#include "diatypes.h"
#include "geometry.h"

/*!
 * \brief Some object resizing depends on the placement of the handle
 * \ingroup ObjectConnects
 */
typedef enum {
  HANDLE_RESIZE_NW, /*!< north/west or top/left */
  HANDLE_RESIZE_N,  /*!< north or top */
  HANDLE_RESIZE_NE, /*!< north/east or top/right */
  HANDLE_RESIZE_W,  /*!< west or left */
  HANDLE_RESIZE_E,  /*!< east or right */
  HANDLE_RESIZE_SW, /*!< south/west or bottom/left */
  HANDLE_RESIZE_S,  /*!< south or bottom */
  HANDLE_RESIZE_SE, /*!< south/east or bottom/right */
  HANDLE_MOVE_STARTPOINT, /*!< for lines: the beginning */
  HANDLE_MOVE_ENDPOINT,   /*!< for lines: the ending */

  /*! These handles can be used privately by objects: */
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

/*!
 * \brief HandleType is used for color coding the different handles
 * \ingroup ObjectConnects
 */
typedef enum {
  HANDLE_NON_MOVABLE,
  HANDLE_MAJOR_CONTROL,
  HANDLE_MINOR_CONTROL,

  NUM_HANDLE_TYPES /* Must be last */
}  HandleType;

/*! 
 * \brief When an objects move_handle() function is called this is passed in
 * \ingroup ObjectConnects
 */
typedef enum {
  HANDLE_MOVE_USER,
  HANDLE_MOVE_USER_FINAL,
  HANDLE_MOVE_CONNECTED,
  HANDLE_MOVE_CREATE,       /*!< the initial drag during object placement */
  HANDLE_MOVE_CREATE_FINAL  /*!< finish of initial drag */
} HandleMoveReason;

/*!
 * \brief If the handle is connectable or not
 * \ingroup ObjectConnects
 */
typedef enum {
  HANDLE_NONCONNECTABLE,     /*!< not connectable */
  HANDLE_CONNECTABLE,        /*!< connectable */
  HANDLE_CONNECTABLE_NOBREAK /*!< (unused) Don't break connection on object move */
} HandleConnectType;

/*!
 * \brief A handle is used to resize objects or to connet to them.
 * \ingroup ObjectConnects
 */
struct _Handle {
  HandleId id; /*!< gives (mostly) the placement relative to the object */
  HandleType type; /*!< color coding */
  Point pos;   /*! where the handle currently is in diagram coordinates */
  
  HandleConnectType connect_type; /*!< how to connect if at all */
  ConnectionPoint *connected_to; /*!< NULL if not connected */
};

  
#endif /* HANDLE_H */
