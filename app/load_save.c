/* xxxxxx -- an diagram creation/manipulation program
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "load_save.h"
#include "files.h"
#include "group.h"
#include "message.h"

#define MAGIC_COOKIE 0x57FE34A3
#define FILE_VERSION 0x00000001

static guint
pointer_hash(gpointer some_pointer)
{
  return (guint) some_pointer;
}


static void
write_types(int fd, GList *list, GHashTable *types_hash, int *type_num)
{
  Object *obj;
  ObjectType *type;
  
  while (list != NULL) {
    obj = (Object *)list->data;
    type = obj->type;
    if (g_hash_table_lookup(types_hash, type) == NULL) {
      g_hash_table_insert(types_hash, type, (gpointer)*type_num);
      
      /* write type to disk. */
      write_int32(fd, *type_num);
      write_string(fd, type->name);
      write_int32(fd, type->version);
      
      (*type_num)++;
    }

    if IS_GROUP(obj) {
      write_types(fd, group_objects(obj), types_hash, type_num);
    }
    
    list = g_list_next(list);
  }
}

static void
write_objects(int fd, GList *list, GHashTable *types_hash,
	      GHashTable *objects_hash, int *object_num)
{
  Object *obj;
  int type_num;
  
  while (list != NULL) {
    obj = (Object *)list->data;
    
    g_hash_table_insert(objects_hash, obj, (gpointer)*object_num);
    (*object_num)++;

    type_num = (int)g_hash_table_lookup(types_hash, obj->type);
    
    write_int32(fd, type_num);
    write_int32(fd, obj->type->version);

    if (IS_GROUP(obj)) {
      write_objects(fd, group_objects(obj), types_hash,
		    objects_hash, object_num);
      write_int32(fd, 0); /* No more objects in group */
    } else {
      obj->type->ops->save(obj, fd);
    }

    list = g_list_next(list);
  }
}

int
write_connections(int fd, GList *list, GHashTable *objects_hash)
{
  Object *obj;
  int object_num;
  int i;
  
  while (list != NULL) {
    obj = (Object *)list->data;

    object_num = (int)g_hash_table_lookup(objects_hash, obj);

    for (i=0;i<obj->num_handles;i++) {
      ConnectionPoint *con_point;
      Handle *handle;
      
      handle = obj->handles[i];
      con_point = handle->connected_to;
      
      if ( con_point != NULL ) {
	Object *other_obj;
	int con_point_nr;
	
	other_obj = con_point->object;
	
	con_point_nr=0;
	while (other_obj->connections[con_point_nr] != con_point) {
	  con_point_nr++;
	  if (con_point_nr>=other_obj->num_connections) {
	    message_error("Error loading diagram\n");
	    return FALSE;
	  }
	}

	write_int32(fd, object_num); /* From what object */
	write_int32(fd, i);  /* from what handle on that object*/
	write_int32(fd,
		    (int)g_hash_table_lookup(objects_hash, other_obj));
	             /* to what object */
 	write_int32(fd, con_point_nr); /* to what connection_point on that object */
      }
    }

    if (IS_GROUP(obj)) {
      if ( !write_connections(fd, group_objects(obj), objects_hash) )
	return FALSE;
    }
    list = g_list_next(list);
  }
  return TRUE;
}


int
diagram_save(Diagram *dia, char *filename)
{
  GHashTable *types_hash;
  GHashTable *objects_hash;
  int type_num;
  int object_num;
  int fd;
  
  fd = open(filename, O_CREAT|O_TRUNC|O_WRONLY , S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

  if (fd==-1) {
    message_error("Couldn't open: '%s' for writing.\n", filename);
    return FALSE;
  }
  
  write_int32(fd, MAGIC_COOKIE);
  write_int32(fd, FILE_VERSION);

  /* Write out used types:
     index of first type is 1, because  0==NULL => no such type */
  types_hash = g_hash_table_new( (GHashFunc) pointer_hash, NULL);
  type_num = 1;
  write_types(fd, dia->objects, types_hash, &type_num);
  write_int32(fd, 0); /* Terminate list of used types. */

  /* Write out diagram data: */
  write_color(fd, &dia->bg_color);

  /* Write out objects: */
  objects_hash = g_hash_table_new( (GHashFunc) pointer_hash, NULL);
  object_num = 1;

  write_objects(fd, dia->objects, types_hash, objects_hash, &object_num);
  write_int32(fd, 0); /* No more objects */

  /* Save the connections between the objects */
  if ( !write_connections(fd, dia->objects, objects_hash) )
    return FALSE;
  write_int32(fd, 0); /* No more connections */
  
  close(fd);

  g_hash_table_destroy(types_hash);
  g_hash_table_destroy(objects_hash);

  dia->unsaved = FALSE;
  dia->modified = FALSE;
  
  return TRUE;
}

static GList *read_objects(int fd, GHashTable *types_hash,
			  GHashTable *objects_hash, int *object_num)
{
  GList *list;
  int type_num;
  int stored_num;
  int version;
  ObjectType *type;
  Object *obj;

  list = NULL;
  
  while ( (type_num=read_int32(fd)) != 0) {
    version = read_int32(fd);
    type = g_hash_table_lookup(types_hash, (gpointer)type_num);

    stored_num = *object_num;
    (*object_num)++;
    if (type == &group_type) {
      obj = group_create(read_objects(fd, types_hash,
				      objects_hash, object_num));
    } else {
      obj = type->ops->load(fd, version);
    }
    list = g_list_append(list, obj);
    g_hash_table_insert(objects_hash, (gpointer) stored_num, obj);
  }
  return list;
}


Diagram *
diagram_load(char *filename)
{
  GHashTable *types_hash;
  GHashTable *objects_hash;
  int fd;
  gint32 res;
  int type_num;
  int object_num;
  int version;
  GList *list;
  ObjectType *type;
  char *type_name;
  Diagram *dia;
  struct stat stat_buf;
  
  fd = open(filename, O_RDONLY);

  if (fd==-1) {
    message_error("Couldn't open: '%s' for reading.\n", filename);
    return NULL;
  }

  fstat(fd, &stat_buf);
  if (S_ISDIR(stat_buf.st_mode)) {
    message_error("You must specify a filename.\n");
    return NULL;
  }

  res = read_int32(fd);
  if (res != MAGIC_COOKIE) {
    message_error("Trying to load a file that is not a diagram.\n");
    return NULL;
  }
  
  res = read_int32(fd);
  if (res > FILE_VERSION) {
    message_error("Trying to load a file that has a higher version than supported.\n");
    return NULL;
  }

  /* Read in all used object types: */
  types_hash = g_hash_table_new((GHashFunc) pointer_hash, NULL);
  while ( (type_num=read_int32(fd)) != 0) {
    type_name = read_string(fd);

    version = read_int32(fd);
    
    type = object_get_type(type_name);
    if (type == NULL) {
      message_error("Don't know anything about the type '%s'.\n", type_name);
      g_free(type_name);
      return NULL;
    }
    if (type->version<version) {
      message_error("Your version of the '%s' object is to old, upgrade.\n", type_name);
      g_free(type_name);
      return NULL;
    }
    g_free(type_name);

    g_hash_table_insert(types_hash, (gpointer) type_num, type);
  }

  /* Create the diagram: */
  dia = new_diagram(filename);

  dia->unsaved = FALSE;
  
  /* Read in diagram data: */
  read_color(fd, &dia->bg_color);

  /* Read in all objects: */
  object_num = 1;
  objects_hash = g_hash_table_new((GHashFunc) pointer_hash, NULL);
  
  list = read_objects(fd, types_hash, objects_hash, &object_num);
  
  diagram_add_object_list(dia, list);
  
  /* Read in connections: */

  while ( (object_num=read_int32(fd)) != 0) {
    int to_nr, handle_nr, con_point_nr;
    Object *from, *to;

    handle_nr = read_int32(fd);
    to_nr = read_int32(fd);
    con_point_nr = read_int32(fd);

    from = g_hash_table_lookup(objects_hash, (gpointer)object_num);
    to = g_hash_table_lookup(objects_hash, (gpointer)to_nr);

    object_connect(from, from->handles[handle_nr], to->connections[con_point_nr]);
  }

  close(fd);

  g_hash_table_destroy(types_hash);
  g_hash_table_destroy(objects_hash);
  
  dia->unsaved = FALSE;
  dia->modified = FALSE;

  return dia;
}


