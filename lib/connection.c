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
#include <stdio.h>
#include <assert.h>

#include "connection.h"
#include "message.h"

void
connection_move_handle(Connection *conn, HandleId id,
		       Point *to, HandleMoveReason reason)
{
  switch(id) {
  case HANDLE_MOVE_STARTPOINT:
    conn->endpoints[0] = *to;
    break;
  case HANDLE_MOVE_ENDPOINT:
    conn->endpoints[1] = *to;
    break;
  default:
    message_error("Internal error in connection_move_handle.\n");
    break;
  }
}

void
connection_update_handles(Connection *conn)
{
  conn->endpoint_handles[0].id = HANDLE_MOVE_STARTPOINT;
  conn->endpoint_handles[0].pos = conn->endpoints[0];

  conn->endpoint_handles[1].id = HANDLE_MOVE_ENDPOINT;
  conn->endpoint_handles[1].pos = conn->endpoints[1];
}

void
connection_update_boundingbox(Connection *conn)
{
  Rectangle *bb;
  Point *endpoints;
  
  assert(conn != NULL);

  bb = &conn->object.bounding_box;
  endpoints = &conn->endpoints[0];
  
  bb->left = endpoints[0].x;
  bb->right = endpoints[1].x;
  if (bb->left > bb->right) {
    coord temp = bb->left;
    bb->left = bb->right;
    bb->right = temp;
  }
  bb->top = endpoints[0].y;
  bb->bottom = endpoints[1].y;
  if (bb->top > bb->bottom) {
    coord temp = bb->top;
    bb->top = bb->bottom;
    bb->bottom = temp;
  }
}

/* Needs to have at least 2 handles 
   The two first of each are used. */
void
connection_init(Connection *conn, int num_handles, int num_connections)
{
  Object *obj;
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


void
connection_copy(Connection *from, Connection *to)
{
  int i;
  Object *toobj;
  Object *fromobj;

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
}

void
connection_destroy(Connection *conn)
{
  object_destroy(&conn->object);
}


void
connection_save(Connection *conn, ObjectNode obj_node)
{
  AttributeNode attr;
  
  object_save(&conn->object, obj_node);

  attr = new_attribute(obj_node, "conn_endpoints");
  data_add_point(attr, &conn->endpoints[0]);
  data_add_point(attr, &conn->endpoints[1]);
}

void
connection_load(Connection *conn, ObjectNode obj_node)
{
  AttributeNode attr;
  DataNode data;

  object_load(&conn->object, obj_node);

  attr = object_find_attribute(obj_node, "conn_endpoints");
  if (attr != NULL) {
    data = attribute_first_data(attr);
    data_point( data , &conn->endpoints[0] );
    data = data_next(data);
    data_point( data , &conn->endpoints[1] );
  }
}
