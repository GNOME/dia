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
#include <stdio.h>
#include <fcntl.h>
#include <glib.h>

/* gnome-xml: */
#include <tree.h>
#include <parser.h>

#include "load_save.h"
#include "files.h"
#include "dia_xml.h"
#include "group.h"
#include "message.h"

static GList *
read_objects(xmlNodePtr objects, GHashTable *objects_hash)
{
  GList *list;
  ObjectType *type;
  Object *obj;
  ObjectNode obj_node;
  const char *typestr;
  const char *versionstr;
  const char *id;
  int version;

  list = NULL;

  obj_node = objects->childs;
  
  while ( obj_node != NULL) {

    if (strcmp(obj_node->name, "object")==0) {
      typestr = xmlGetProp(obj_node, "type");
      versionstr = xmlGetProp(obj_node, "version");
      id = xmlGetProp(obj_node, "id");
      
      version = 0;
      if (versionstr != NULL)
	version = atoi(versionstr);
      
      type = object_get_type((char *)typestr);
      
      obj = type->ops->load(obj_node, version);
      list = g_list_append(list, obj);
      
      g_hash_table_insert(objects_hash, (char *)id, obj);
      
    } else if (strcmp(obj_node->name, "group")==0) {
      obj = group_create(read_objects(obj_node, objects_hash));
      list = g_list_append(list, obj);
    } else {
      message_error("Error reading diagram file\n");
    }

    obj_node = obj_node->next;
  }
  return list;
}

void
read_connections(GList *objects, xmlNodePtr objects_node,
		 GHashTable *objects_hash)
{
  ObjectNode obj_node;
  GList *list;
  xmlNodePtr connections;
  xmlNodePtr connection;
  const char *handlestr;
  const char *tostr;
  const char *connstr;
  int handle, conn;
  Object *to;
  
  list = objects;
  obj_node = objects_node->childs;
  while (list != NULL) {
    Object *obj = (Object *) list->data;

    if IS_GROUP(obj) {
      read_connections(group_objects(obj), obj_node, objects_hash);
    } else {
      connections = obj_node->childs;
      while ((connections!=NULL) &&
	     (strcmp(connections->name, "connections")!=0))
	connections = connections->next;
      if (connections != NULL) {
	connection = connections->childs;
	while (connection != NULL) {
	  handlestr = xmlGetProp(connection, "handle");
	  tostr = xmlGetProp(connection, "to");
	  connstr = xmlGetProp(connection, "connection");
	  handle = atoi(handlestr);
	  conn = atoi(connstr);


	  to = g_hash_table_lookup(objects_hash, tostr);

	  if (to == NULL) {
	    message_error("Error loading diagram.\n"
			  "Linked object not found in document.");
	  } else {
	    object_connect(obj, obj->handles[handle],
			   to->connections[conn]);
	  }

	  connection = connection->next;
	}
      }
      
    }
    
    list = g_list_next(list);
    obj_node = obj_node->next;
  }
}

Diagram *
diagram_load(char *filename)
{
  GHashTable *objects_hash;
  int fd;
  struct stat stat_buf;
  GList *list;
  Diagram *dia;
  xmlDocPtr doc;
  xmlNodePtr diagramdata;
  xmlNodePtr objects;
  AttributeNode attr;
  
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
  
  close(fd);

  doc = xmlParseFile(filename);

  if (doc == NULL){
    message_error("Error loading diagram.\n");
    return NULL;
  }
  
  /* Create the diagram: */
  dia = new_diagram(filename);

  dia->unsaved = FALSE;

  diagramdata = doc->root->childs;

  /* Read in diagram data: */
  dia->bg_color = color_white;
  attr = composite_find_attribute(diagramdata, "background");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &dia->bg_color);

  /* Read in all objects: */
  objects = diagramdata->next;
  
  objects_hash = g_hash_table_new(g_str_hash, g_str_equal);
  
  list = read_objects(objects, objects_hash);
  diagram_add_object_list(dia, list);
  read_connections( list, objects, objects_hash);
  

  xmlFreeDoc(doc);

  g_hash_table_destroy(objects_hash);
  
  dia->unsaved = FALSE;
  dia->modified = FALSE;

  return dia;
}



void
write_objects(GList *objects, xmlNodePtr objects_node,
	      GHashTable *objects_hash, int *obj_nr)
{
  char buffer[31];
  ObjectNode obj_node;
  xmlNodePtr group_node;
  GList *list;

  list = objects;
  while (list != NULL) {
    Object *obj = (Object *) list->data;

    if IS_GROUP(obj) {
      group_node = xmlNewChild(objects_node, NULL, "group", NULL);
      write_objects(group_objects(obj), group_node, objects_hash, obj_nr);
    } else {
      obj_node = xmlNewChild(objects_node, NULL, "object", NULL);
    
      xmlSetProp(obj_node, "type", obj->type->name);
      
      snprintf(buffer, 30, "%d", obj->type->version);
      xmlSetProp(obj_node, "version", buffer);
      
      snprintf(buffer, 30, "O%d", *obj_nr);
      xmlSetProp(obj_node, "id", buffer);

      (*obj->type->ops->save)(obj, obj_node);

      /* Add object -> obj_nr to hash table */
      g_hash_table_insert(objects_hash, obj, GINT_TO_POINTER(*obj_nr));
      (*obj_nr)++;
      
    }
    
    list = g_list_next(list);
  }
}

int
write_connections(GList *objects, xmlNodePtr objects_node,
		  GHashTable *objects_hash)
{
  ObjectNode obj_node;
  GList *list;
  xmlNodePtr connections;
  xmlNodePtr connection;
  char buffer[31];
  int i;

  list = objects;
  obj_node = objects_node->childs;
  while (list != NULL) {
    Object *obj = (Object *) list->data;

    if IS_GROUP(obj) {
      write_connections(group_objects(obj), obj_node, objects_hash);
    } else {
      connections = NULL;
    
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
	      message_error("Error saving diagram\n");
	      return FALSE;
	    }
	  }
	  
	  if (connections == NULL)
	    connections = xmlNewChild(obj_node, NULL, "connections", NULL);
	  
	  connection = xmlNewChild(connections, NULL, "connection", NULL);
	  /* from what handle on this object*/
	  snprintf(buffer, 30, "%d", i);
	  xmlSetProp(connection, "handle", buffer);
	  /* to what object */
	  snprintf(buffer, 30, "O%d",
		   GPOINTER_TO_INT(g_hash_table_lookup(objects_hash,
						       other_obj)));
	  xmlSetProp(connection, "to", buffer);
	  /* to what connection_point on that object */
	  snprintf(buffer, 30, "%d", con_point_nr);
	  xmlSetProp(connection, "connection", buffer);
	}
      }
    }
    
    list = g_list_next(list);
    obj_node = obj_node->next;
  }
  return TRUE;
}

int
diagram_save(Diagram *dia, char *filename)
{
  FILE *file;
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNodePtr objects_node;
  GHashTable *objects_hash;
  int res;
  int obj_nr;
  
  AttributeNode attr;
  
  file = fopen(filename, "w");

  if (file==NULL) {
    message_error("Couldn't open: '%s' for writing.\n", filename);
    return FALSE;
  }

  doc = xmlNewDoc("1.0");
  
  doc->root = xmlNewDocNode(doc, NULL, "diagram", NULL);

  tree = xmlNewChild(doc->root, NULL, "diagramdata", NULL);
  
  attr = new_attribute((ObjectNode)tree, "background");
  data_add_color(attr, &dia->bg_color);

  objects_node = xmlNewChild(doc->root, NULL, "objects", NULL);

  objects_hash = g_hash_table_new(g_direct_hash, g_direct_equal);

  obj_nr = 0;
  write_objects(dia->objects, objects_node, objects_hash, &obj_nr);
  
  res = write_connections(dia->objects, objects_node, objects_hash);
  if (!res)
    return FALSE;
  
  xmlDocDump(file, doc);
  xmlFreeDoc(doc);
  
  fclose(file);
  g_hash_table_destroy(objects_hash);

  dia->unsaved = FALSE;
  dia->modified = FALSE;

  return TRUE;
}


