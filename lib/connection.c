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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h> /* memcpy() */

#include "connection.h"


/**
 * connection_adjust_for_autogap:
 *
 * Adjust connection endings for autogap.  This function actually moves the
 * ends of the connection, but only when the end is connected to
 * a mainpoint.
 */
void
connection_adjust_for_autogap (Connection *connection)
{
  Point endpoints[2];
  ConnectionPoint *start_cp, *end_cp;

  start_cp = connection->endpoint_handles[0].connected_to;
  end_cp = connection->endpoint_handles[1].connected_to;

  endpoints[0] = connection->endpoints[0];
  endpoints[1] = connection->endpoints[1];

  if (connpoint_is_autogap (start_cp)) {
    endpoints[0] = start_cp->pos;
  }

  if (connpoint_is_autogap (end_cp)) {
    endpoints[1] = end_cp->pos;
  }

  if (connpoint_is_autogap (start_cp)) {
    connection->endpoints[0] = calculate_object_edge (&endpoints[0],
                                                      &endpoints[1],
                                                      start_cp->object);
  }

  if (connpoint_is_autogap (end_cp)) {
    connection->endpoints[1] = calculate_object_edge (&endpoints[1],
                                                      &endpoints[0],
                                                      end_cp->object);
  }
}


/**
 * connection_move_handle:
 * @conn: The object whose handle is being moved.
 * @id: The handle being moved.
 * @to: The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 * @cp: If non-%NULL, the connectionpoint found at this position.
 *   If @cp is %NULL, there may or may not be a connectionpoint.
 * @reason: The reason the handle was moved.
 * - %HANDLE_MOVE_USER means the user is dragging the point.
 * - %HANDLE_MOVE_USER_FINAL means the user let go of the point.
 * - %HANDLE_MOVE_CONNECTED means it was moved because something it was
 *   connected to moved.
 * @modifiers: gives a bitset of modifier keys currently held down
 * - %MODIFIER_SHIFT is either shift key
 * - %MODIFIER_ALT is either alt key
 * - %MODIFIER_CONTROL is either control key
 * Each has %MODIFIER_LEFT_* and %MODIFIER_RIGHT_* variants
 *
 * Function called to move one of the handles associated with the object.
 *
 * Returns: An #DiaObjectChange with additional undo information, or
 *          (in most cases) %NULL. Undo for moving the handle itself is
 *          handled elsewhere.
 */
DiaObjectChange *
connection_move_handle (Connection       *conn,
                        HandleId          id,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  switch (id) {
    case HANDLE_MOVE_STARTPOINT:
      conn->endpoints[0] = *to;
      break;
    case HANDLE_MOVE_ENDPOINT:
      conn->endpoints[1] = *to;
      break;
    case HANDLE_RESIZE_NW:
    case HANDLE_RESIZE_N:
    case HANDLE_RESIZE_NE:
    case HANDLE_RESIZE_W:
    case HANDLE_RESIZE_E:
    case HANDLE_RESIZE_SW:
    case HANDLE_RESIZE_S:
    case HANDLE_RESIZE_SE:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_return_val_if_reached (NULL);
      break;
  }
  return NULL;
}


/**
 * connection_update_handles:
 * @conn: The connection object to do the update on.
 *
 * Update the type and placement of the two handles.
 * This only updates handles 0 and 1, not any handles added by subclasses.
 */
void
connection_update_handles (Connection *conn)
{
  conn->endpoint_handles[0].id = HANDLE_MOVE_STARTPOINT;
  conn->endpoint_handles[0].pos = conn->endpoints[0];

  conn->endpoint_handles[1].id = HANDLE_MOVE_ENDPOINT;
  conn->endpoint_handles[1].pos = conn->endpoints[1];
}


/**
 * connection_update_boundingbox:
 * @conn: The #Connection to update bounding box on.
 *
 * Update the bounding box for a connection.
 */
void
connection_update_boundingbox (Connection *conn)
{
  g_return_if_fail (conn != NULL);

  line_bbox (&conn->endpoints[0],
             &conn->endpoints[1],
             &conn->extra_spacing,
             &conn->object.bounding_box);
}


/**
 * connection_init:
 * @conn: A newly allocated connection object.
 * @num_handles: How many handles to allocate room for.
 * @num_connections: How many connectionpoints to allocate room for.
 *
 * Initialize a connection objects.
 * This will in turn call object_init.
 */
void
connection_init (Connection *conn, int num_handles, int num_connections)
{
  g_return_if_fail (num_handles >= 2);

  object_init (DIA_OBJECT (conn), num_handles, num_connections);

  g_return_if_fail (DIA_OBJECT (conn)->handles != NULL);

  for (int i = 0; i < 2; i++) {
    DIA_OBJECT (conn)->handles[i] = &conn->endpoint_handles[i];
    DIA_OBJECT (conn)->handles[i]->type = HANDLE_MAJOR_CONTROL;
    DIA_OBJECT (conn)->handles[i]->connect_type = HANDLE_CONNECTABLE;
    DIA_OBJECT (conn)->handles[i]->connected_to = NULL;
  }
}


/**
 * connection_copy:
 * @from: The object to copy from.
 * @to: The newly allocated object to copy into.
 *
 * Copies an object connection-level and down.
 */
void
connection_copy (Connection *from, Connection *to)
{
  int i;

  object_copy (DIA_OBJECT (from), DIA_OBJECT (to));

  for (i = 0; i < 2; i++) {
    to->endpoints[i] = from->endpoints[i];
  }

  for (i = 0; i < 2; i++) {
    /* handles: */
    to->endpoint_handles[i] = from->endpoint_handles[i];
    to->endpoint_handles[i].connected_to = NULL;
    DIA_OBJECT (to)->handles[i] = &to->endpoint_handles[i];
  }

  memcpy (&to->extra_spacing,
          &from->extra_spacing,
          sizeof (to->extra_spacing));
}


/**
 * connection_destroy:
 * @conn: An object to destroy.
 *
 * Function called before an object is deleted.
 * This function must call the parent class's DestroyFunc, and then free
 * the memory associated with the object, but not the object itself
 * Must also unconnect itself from all other objects, which can be done
 * by calling object_destroy, or letting the super-class call it)
 * This is one of the object_ops functions.
 */
void
connection_destroy (Connection *conn)
{
  object_destroy (DIA_OBJECT (conn));
}


/**
 * connection_save:
 * @conn: The connection to save.
 * @obj_node: The XML node to save it to.
 * @ctx: the current #DiaContext
 *
 * Save a connections data to XML.
 */
void
connection_save (Connection *conn, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;

  object_save (DIA_OBJECT (conn), obj_node, ctx);

  attr = new_attribute (obj_node, "conn_endpoints");
  data_add_point (attr, &conn->endpoints[0], ctx);
  data_add_point (attr, &conn->endpoints[1], ctx);
}


/**
 * connection_load:
 * @conn: A fresh connection object to load into.
 * @obj_node: The XML node to load from.
 * @ctx: The context in which this function is called
 *
 * Load a connections data from XML.
 */
void
connection_load (Connection *conn, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;
  DataNode data;

  object_load (DIA_OBJECT (conn), obj_node, ctx);

  attr = object_find_attribute (obj_node, "conn_endpoints");
  if (attr != NULL) {
    data = attribute_first_data (attr);
    data_point (data, &conn->endpoints[0], ctx);
    data = data_next (data);
    data_point (data, &conn->endpoints[1], ctx);
  }
}
