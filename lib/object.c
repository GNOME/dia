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
#include <string.h>

#include <gtk/gtk.h>

#include "object.h"
#include "message.h"

#include "dummy_dep.h"

void
object_init(Object *obj,
	    int num_handles,
	    int num_connections)
{
  obj->num_handles = num_handles;
  if (num_handles>0)
    obj->handles = g_new0(Handle *,num_handles);
  else
    obj->handles = NULL;

  obj->num_connections = num_connections;
  if (num_connections>0)
    obj->connections = g_new0(ConnectionPoint *,num_connections);
  else
    obj->connections = NULL;
}

void
object_destroy(Object *obj)
{
  object_unconnect_all(obj);
  
  if (obj->handles)
    g_free(obj->handles);

  if (obj->connections)
    g_free(obj->connections);

}


/* After this copying you have to fix up:
   handles
   connections
*/
void
object_copy(Object *from, Object *to)
{
  to->type = from->type;
  to->position = from->position;
  to->bounding_box = from->bounding_box;
  
  to->num_handles = from->num_handles;
  if (to->handles != NULL) g_free(to->handles);
  if (to->num_handles>0)
    to->handles = g_malloc(sizeof(Handle *)*to->num_handles);
  else
    to->handles = NULL;

  to->num_connections = from->num_connections;
  if (to->connections != NULL) g_free(to->connections);
  if (to->num_connections>0)
    to->connections = g_malloc(sizeof(ConnectionPoint *) * to->num_connections);
  else
    to->connections = NULL;

  to->ops = from->ops;
}


void
destroy_object_list(GList *list_to_be_destroyed)
{
  GList *list;
  Object *obj;
  
  list = list_to_be_destroyed;
  while (list != NULL) {
    obj = (Object *)list->data;

    obj->ops->destroy(obj);
    g_free(obj);
    
    list = g_list_next(list);
  }

  g_list_free(list_to_be_destroyed);
}

void
object_add_handle(Object *obj, Handle *handle)
{
  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
  
  obj->handles[obj->num_handles-1] = handle;
}

void
object_add_handle_at(Object *obj, Handle *handle, int pos)
{
  int i;
  
  obj->num_handles++;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));

  for (i=obj->num_handles-1; i > pos; i--) {
    obj->handles[i] = obj->handles[i-1];
  }
  obj->handles[pos] = handle;
}

void
object_remove_handle(Object *obj, Handle *handle)
{
  int i, handle_nr;

  handle_nr = -1;
  for (i=0;i<obj->num_handles;i++) {
    if (obj->handles[i] == handle)
      handle_nr = i;
  }

  if (handle_nr < 0) {
    message_error("Internal error, object_remove_handle: Handle doesn't exist");
    return;
  }

  for (i=handle_nr;i<(obj->num_handles-1);i++) {
    obj->handles[i] = obj->handles[i+1];
  }
  obj->handles[obj->num_handles-1] = NULL;
    
  obj->num_handles--;

  obj->handles =
    g_realloc(obj->handles, obj->num_handles*sizeof(Handle *));
}

void
object_add_connectionpoint(Object *obj, ConnectionPoint *conpoint)
{
  obj->num_connections++;

  obj->connections =
    g_realloc(obj->connections, 
	      obj->num_connections*sizeof(ConnectionPoint *));
  
  obj->connections[obj->num_connections-1] = conpoint;
}

void 
object_add_connectionpoint_at(Object *obj, 
			      ConnectionPoint *conpoint, int pos)
{
  int i;
  
  obj->num_connections++;

  obj->connections =
    g_realloc(obj->connections, 
	      obj->num_connections*sizeof(ConnectionPoint *));

  for (i=obj->num_connections-1; i > pos; i--) {
    obj->connections[i] = obj->connections[i-1];
  }
  obj->connections[pos] = conpoint;
}

void
object_remove_connectionpoint(Object *obj, ConnectionPoint *conpoint)
{
  int i, nr;

  object_remove_connections_to(conpoint);

  nr = -1;
  for (i=0;i<obj->num_connections;i++) {
    if (obj->connections[i] == conpoint)
      nr = i;
  }

  if (nr < 0) {
    message_error("Internal error, object_remove_connectionpoint: "
                  "ConnectionPoint doesn't exist");
    return;
  }

  for (i=nr;i<(obj->num_connections-1);i++) {
    obj->connections[i] = obj->connections[i+1];
  }
  obj->connections[obj->num_connections-1] = NULL;
    
  obj->num_connections--;

  obj->connections =
    g_realloc(obj->connections, obj->num_connections*sizeof(ConnectionPoint *));
}


void
object_connect(Object *obj, Handle *handle,
	       ConnectionPoint *connectionpoint)
{
  if (handle->connect_type==HANDLE_NONCONNECTABLE) {
    message_error("Error? trying to connect a non connectable handle.\n"
		  "Check this out...\n");
    return;
  }
  handle->connected_to = connectionpoint;
  connectionpoint->connected =
    g_list_prepend(connectionpoint->connected, obj);
}

void
object_unconnect(Object *connected_obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  connectionpoint = handle->connected_to;

  if (connectionpoint!=NULL) {
    connectionpoint->connected =
      g_list_remove(connectionpoint->connected, connected_obj);
    handle->connected_to = NULL;
  }
}

void
object_remove_connections_to(ConnectionPoint *conpoint)
{
  GList *list;
  Object *connected_obj;
  int i;
  
  list = conpoint->connected;
  while (list != NULL) {
    connected_obj = (Object *)list->data;

    for (i=0;i<connected_obj->num_handles;i++) {
      if (connected_obj->handles[i]->connected_to == conpoint) {
	connected_obj->handles[i]->connected_to = NULL;
      }
    }
    list = g_list_next(list);
  }
  g_list_free(conpoint->connected);
  conpoint->connected = NULL;
}

void
object_unconnect_all(Object *obj)
{
  int i;
  
  for (i=0;i<obj->num_handles;i++) {
    object_unconnect(obj, obj->handles[i]);
  }
  for (i=0;i<obj->num_connections;i++) {
    object_remove_connections_to(obj->connections[i]);
  }
}

void object_save(Object *obj, ObjectNode obj_node)
{
  data_add_point(new_attribute(obj_node, "obj_pos"),
		 &obj->position);
  data_add_rectangle(new_attribute(obj_node, "obj_bb"),
		     &obj->bounding_box);
}

void object_load(Object *obj, ObjectNode obj_node)
{
  AttributeNode attr;

  obj->position.x = 0.0;
  obj->position.y = 0.0;
  attr = object_find_attribute(obj_node, "obj_pos");
  if (attr != NULL)
    data_point( attribute_first_data(attr), &obj->position );

  obj->bounding_box.left = obj->bounding_box.right = 0.0;
  obj->bounding_box.top = obj->bounding_box.bottom = 0.0;
  attr = object_find_attribute(obj_node, "obj_bb");
  if (attr != NULL)
    data_rectangle( attribute_first_data(attr), &obj->bounding_box );
}

/****** Object register: **********/

static guint hash(gpointer key)
{
  char *string = (char *)key;
  int sum;

  sum = 0;
  while (*string) {
    sum += (*string);
    string++;
  }

  return sum;
}

static gint compare(gpointer a, gpointer b)
{
  return strcmp((char *)a, (char *)b)==0;
}

static GHashTable *object_type_table = NULL;

void
object_registry_init(void)
{
  object_type_table = g_hash_table_new( (GHashFunc) hash, (GCompareFunc) compare );
}

void
object_register_type(ObjectType *type)
{
  if (g_hash_table_lookup(object_type_table, type->name) != NULL) {
    message_warning("Several object-types were named %s.\n"
		    "Only first one will be used.\n"
		    "Some things might not work as expected.\n",
		    type->name);
    return;
  }
  g_hash_table_insert(object_type_table, type->name, type);
}

ObjectType *
object_get_type(char *name)
{
  return (ObjectType *) g_hash_table_lookup(object_type_table, name);
}


int
object_return_false(Object *obj)
{
  return FALSE;
}

void *
object_return_null(Object *obj)
{
  return NULL;
}

void
object_return_void(Object *obj)
{
  return;
}

Object *
object_load_using_properties(const ObjectType *type,
                             ObjectNode obj_node, int version,
                             const char *filename)
{
  Object *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  
  obj = type->ops->create(&startpoint,NULL, &handle1,&handle2);
  object_load_props(obj,obj_node);
  return obj;
}

void 
object_save_using_properties(Object *obj, ObjectNode obj_node, 
                             int version, const char *filename)
{
  object_save_props(obj,obj_node);
}

Object *object_copy_using_properties(Object *obj)
{
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;
  Object *newobj = obj->type->ops->create(&startpoint,NULL,
                                          &handle1,&handle2);
  object_copy_props(newobj,obj);
  return newobj;
}

