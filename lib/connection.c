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

#include <config.h>

#include <stdio.h>
#include <string.h> /* memcpy() */
#include <assert.h>

#include "connection.h"

/** Adjust connection endings for autogap.  This function actually moves the
 * ends of the connection, but only when the end is connected to 
 * a mainpoint.
 */
void
connection_adjust_for_autogap(Connection *connection)
{
  Point endpoints[2];
  ConnectionPoint *start_cp, *end_cp;

  start_cp = connection->endpoint_handles[0].connected_to;
  end_cp = connection->endpoint_handles[1].connected_to;

  endpoints[0] = connection->endpoints[0];
  endpoints[1] = connection->endpoints[1];

  if (connpoint_is_autogap(start_cp)) {
    endpoints[0] = start_cp->pos;
  }
  if (connpoint_is_autogap(end_cp)) {    
    endpoints[1] = end_cp->pos;
  }

  if (connpoint_is_autogap(start_cp)) {
    connection->endpoints[0] = calculate_object_edge(&endpoints[0],
						     &endpoints[1],
						     start_cp->object);
  }
  if (connpoint_is_autogap(end_cp)) {    
    connection->endpoints[1] = calculate_object_edge(&endpoints[1],
						     &endpoints[0],
						     end_cp->object);
  }
}

/** Function called to move one of the handles associated with the object. 
 * @param conn The object whose handle is being moved.
 * @param id The handle being moved.
 * @param to The position it has been moved to (corrected for
 *   vertical/horizontal only movement).
 * @param cp If non-NULL, the connectionpoint found at this position.
 *   If @a cp is NULL, there may or may not be a connectionpoint.
 * @param reason The reason the handle was moved.
 *     - HANDLE_MOVE_USER means the user is dragging the point.
 *     - HANDLE_MOVE_USER_FINAL means the user let go of the point.
 *     - HANDLE_MOVE_CONNECTED means it was moved because something
 *	    it was connected to moved.
 * @param modifiers gives a bitset of modifier keys currently held down
 *     - MODIFIER_SHIFT is either shift key
 *     - MODIFIER_ALT is either alt key
 *     - MODIFIER_CONTROL is either control key
 *	    Each has MODIFIER_LEFT_* and MODIFIER_RIGHT_* variants
 * @return An @a ObjectChange* with additional undo information, or
 *  (in most cases) NULL.  Undo for moving the handle itself is handled
 *  elsewhere.
 * \memberof _Connection
 */
ObjectChange*
connection_move_handle(Connection *conn, HandleId id,
		       Point *to, ConnectionPoint *cp,
		       HandleMoveReason reason, ModifierKeys modifiers)
{
  switch(id) {
  case HANDLE_MOVE_STARTPOINT:
    conn->endpoints[0] = *to;
    break;
  case HANDLE_MOVE_ENDPOINT:
    conn->endpoints[1] = *to;
    break;
  default:
    g_warning("Internal error in connection_move_handle.\n");
    break;
  }
  return NULL;
}

/** Update the type and placement of the two handles.
 * This only updates handles 0 and 1, not any handles added by subclasses.
 * @param conn The connection object to do the update on.
 */
void
connection_update_handles(Connection *conn)
{
  conn->endpoint_handles[0].id = HANDLE_MOVE_STARTPOINT;
  conn->endpoint_handles[0].pos = conn->endpoints[0];

  conn->endpoint_handles[1].id = HANDLE_MOVE_ENDPOINT;
  conn->endpoint_handles[1].pos = conn->endpoints[1];
}

/** Update the bounding box for a connection.
 * @param conn The connection to update bounding box on.
 */
void
connection_update_boundingbox(Connection *conn)
{
  assert(conn != NULL);
  
  line_bbox(&conn->endpoints[0],&conn->endpoints[1],
            &conn->extra_spacing,&conn->object.bounding_box);
}

/** Initialize a connection objects.
 *  This will in turn call object_init.
 * @param conn A newly allocated connection object.
 * @param num_handles How many handles to allocate room for.
 * @param num_connections How many connectionpoints to allocate room for.
 */
void
connection_init(Connection *conn, int num_handles, int num_connections)
{
  DiaObject *obj;
  int i;

  obj = &conn->object;

  assert(num_handles>=2);
  
  object_init(obj, num_handles, num_connections);

  assert(obj->handles!=NULL);
  
  for (i=0;i<2;i++) {
    obj->handles[i] = &conn->endpoint_handles[i];
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[i]->connect_type = HANDLE_CONNECTABLE;
    obj->handles[i]->connected_to = NULL;
  }
}

/** Copies an object connection-level and down.
 * @param from The object to copy from.
 * @param to The newly allocated object to copy into.
 */
void
connection_copy(Connection *from, Connection *to)
{
  int i;
  DiaObject *toobj;
  DiaObject *fromobj;

  toobj = &to->object;
  fromobj = &from->object;

  object_copy(fromobj, toobj);

  for (i=0;i<2;i++) {
    to->endpoints[i] = from->endpoints[i];
  }

  for (i=0;i<2;i++) {
    /* handles: */
    to->endpoint_handles[i] = from->endpoint_handles[i];
    to->endpoint_handles[i].connected_to = NULL;
    toobj->handles[i] = &to->endpoint_handles[i];
  }
  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));
}

/** Function called before an object is deleted.
 *  This function must call the parent class's DestroyFunc, and then free
 *  the memory associated with the object, but not the object itself
 *  Must also unconnect itself from all other objects, which can be done
 *  by calling object_destroy, or letting the super-class call it)
 *  This is one of the object_ops functions.
 * @param conn An object to destroy.
 */
void
connection_destroy(Connection *conn)
{
  object_destroy(&conn->object);
}

/** Save a connections data to XML.
 * @param conn The connection to save.
 * @param obj_node The XML node to save it to.
 */
void
connection_save(Connection *conn, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;
  
  object_save(&conn->object, obj_node, ctx);

  attr = new_attribute(obj_node, "conn_endpoints");
  data_add_point(attr, &conn->endpoints[0], ctx);
  data_add_point(attr, &conn->endpoints[1], ctx);
}

/** Load a connections data from XML.
 * @param conn A fresh connection object to load into.
 * @param obj_node The XML node to load from.
 * @param ctx The context in which this function is called
 */
void
connection_load(Connection *conn, ObjectNode obj_node, DiaContext *ctx)
{
  AttributeNode attr;
  DataNode data;

  object_load(&conn->object, obj_node, ctx);

  attr = object_find_attribute(obj_node, "conn_endpoints");
  if (attr != NULL) {
    data = attribute_first_data(attr);
    data_point(data, &conn->endpoints[0], ctx);
    data = data_next(data);
    data_point(data, &conn->endpoints[1], ctx);
  }
}
