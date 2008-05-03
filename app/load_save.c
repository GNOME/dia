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

/* so we get fdopen declared even when compiling with -ansi */
#define _POSIX_C_SOURCE 2
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h> /* g_access() and friends */
#include <errno.h>

#ifndef W_OK
#define W_OK 2
#endif

#include "intl.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include "load_save.h"
#include "group.h"
#include "diagramdata.h"
#include "object.h"
#include "message.h"
#include "preferences.h"
#include "diapagelayout.h"
#include "autosave.h"
#include "newgroup.h"

#ifdef G_OS_WIN32
#include <io.h>
#endif

static void read_connections(GList *objects, xmlNodePtr layer_node,
			     GHashTable *objects_hash);
static void GHFuncUnknownObjects(gpointer key,
				 gpointer value,
				 gpointer user_data);
static GList *read_objects(xmlNodePtr objects,
			   GHashTable *objects_hash,
			   const char *filename, DiaObject *parent);
static void hash_free_string(gpointer       key,
			     gpointer       value,
			     gpointer       user_data);
static xmlNodePtr find_node_named (xmlNodePtr p, const char *name);
static gboolean diagram_data_load(const char *filename, DiagramData *data,
				  void* user_data);
static gboolean write_objects(GList *objects, xmlNodePtr objects_node,
			      GHashTable *objects_hash, int *obj_nr, 
			      const char *filename);
static gboolean write_connections(GList *objects, xmlNodePtr layer_node,
				  GHashTable *objects_hash);
static xmlDocPtr diagram_data_write_doc(DiagramData *data, const char *filename);
static int diagram_data_raw_save(DiagramData *data, const char *filename);
static int diagram_data_save(DiagramData *data, const char *filename);


static void
GHFuncUnknownObjects(gpointer key,
                     gpointer value,
                     gpointer user_data)
{
  GString* s = (GString*)user_data;
  g_string_append(s, "\n");
  g_string_append(s, (gchar*)key);
  g_free(key);
}

/**
 * Recursive function to read objects from a specific level in the xml.
 *
 * Nowadays there are quite a few of them :
 * - Layers : a diagram may have different layers, but this function does *not*
 *   add the created objects to them as it does not know on which nesting level it
 *   is called. So the topmost caller must add the returned objects to the layer.
 * - Groups : groups in itself can have an arbitrary nesting level including other
 *   groups or objects or both of them. A group not containing any objects is by 
 *   definition useless. So it is not created. This is to avoid trouble with some older
 *   diagrams which happen to be saved with empty groups.
 * - Parents : if the parent relations would have been there from the beginning of
 *   Dias file format they probably would have been added as nesting level 
 *   themselves. But to maintain forward compatibility (allow to let older versions
 *   of Dia to see as much as possible) they were added all on the same level and
 *   the parent child relation is reconstructed from additional attributes.
 */
static GList *
read_objects(xmlNodePtr objects, 
             GHashTable *objects_hash,const char *filename, DiaObject *parent)
{
  GList *list;
  DiaObjectType *type;
  DiaObject *obj;
  ObjectNode obj_node;
  char *typestr;
  char *versionstr;
  char *id;
  int version;
  GHashTable* unknown_hash;
  GString*    unknown_str;
  xmlNodePtr child_node;

  unknown_hash = g_hash_table_new(g_str_hash, g_str_equal);
  unknown_str  = g_string_new("Unknown types while reading diagram file");

  list = NULL;

  obj_node = objects->xmlChildrenNode;

  while ( obj_node != NULL) {
    if (xmlIsBlankNode(obj_node)) {
      obj_node = obj_node->next;
      continue;
    }

    if (!obj_node) break;

    if (xmlStrcmp(obj_node->name, (const xmlChar *)"object")==0) {
      typestr = (char *) xmlGetProp(obj_node, (const xmlChar *)"type");
      versionstr = (char *) xmlGetProp(obj_node, (const xmlChar *)"version");
      id = (char *) xmlGetProp(obj_node, (const xmlChar *)"id");
      
      version = 0;
      if (versionstr != NULL) {
	version = atoi(versionstr);
	xmlFree(versionstr);
      }

      type = object_get_type((char *)typestr);
      
      if (!type) {
	if (NULL == g_hash_table_lookup(unknown_hash, typestr))
	    g_hash_table_insert(unknown_hash, g_strdup(typestr), 0);
      }
      else
      {
        obj = type->ops->load(obj_node, version, filename);
        list = g_list_append(list, obj);

        if (parent)
        {
          obj->parent = parent;
          parent->children = g_list_append(parent->children, obj);
        }

        g_hash_table_insert(objects_hash, g_strdup((char *)id), obj);

        child_node = obj_node->children;

        while(child_node)
        {
          if (xmlStrcmp(child_node->name, (const xmlChar *)"children") == 0)
          {
	    GList *children_read = read_objects(child_node, objects_hash, filename, obj);
            list = g_list_concat(list, children_read);
            break;
          }
          child_node = child_node->next;
        }
      }
      if (typestr) xmlFree(typestr);
      if (id) xmlFree (id);
    } else if (xmlStrcmp(obj_node->name, (const xmlChar *)"group")==0
               && obj_node->children) {
      /* don't create empty groups */
      obj = group_create(read_objects(obj_node, objects_hash, filename, NULL));
#ifdef USE_NEWGROUP
      /* Old group objects had objects recursively inside them.  Since
       * objects are now done with parenting, we need to extract those objects,
       * make a newgroup object, set parent-child relationships, and add
       * all of them to the diagram.
       */
      {
	DiaObject *newgroup;
	GList *objects;
	Point lower_right;
	type = object_get_type("Misc - NewGroup");
	lower_right = obj->position;
	newgroup = type->ops->create(&lower_right,
				     NULL,
				     NULL,
				     NULL);
	list = g_list_append(list, newgroup);
	
	for (objects = group_objects(obj); objects != NULL; objects = g_list_next(objects)) {
	  DiaObject *subobj = (DiaObject *) objects->data;
	  list = g_list_append(list, subobj);
	  subobj->parent = newgroup;
          newgroup->children = g_list_append(newgroup->children, subobj);
	  if (subobj->bounding_box.right > lower_right.x) {
	    lower_right.x = subobj->bounding_box.right;
	  }
	  if (subobj->bounding_box.bottom > lower_right.y) {
	    lower_right.y = subobj->bounding_box.bottom;
	  }
	}
	newgroup->ops->move_handle(newgroup, newgroup->handles[7],
			       &lower_right, NULL,
			       HANDLE_MOVE_CREATE_FINAL, 0);
	/* We've used the info from the old group, destroy it */
	group_destroy_shallow(obj);
      }
#else
      list = g_list_append(list, obj);
#endif
    } else {
      /* silently ignore other nodes */
    }

    obj_node = obj_node->next;
  }
  /* show all the unknown types in one message */
  if (0 < g_hash_table_size(unknown_hash)) {
    g_hash_table_foreach(unknown_hash,
			 GHFuncUnknownObjects,
			 unknown_str);
    message_error("%s", unknown_str->str);
  }
  g_hash_table_destroy(unknown_hash);
  g_string_free(unknown_str, TRUE);

  return list;
}

static void
read_connections(GList *objects, xmlNodePtr layer_node,
		 GHashTable *objects_hash)
{
  ObjectNode obj_node;
  GList *list;
  xmlNodePtr connections;
  xmlNodePtr connection;
  char *handlestr;
  char *tostr;
  char *connstr;
  int handle, conn;
  DiaObject *to;
  
  list = objects;
  obj_node = layer_node->xmlChildrenNode;
  while ((list != NULL) && (obj_node != NULL)) {
    DiaObject *obj = (DiaObject *) list->data;

    /* the obj and there node must stay in sync to properly setup connections */
    while (obj_node && (xmlIsBlankNode(obj_node) || XML_COMMENT_NODE == obj_node->type)) 
      obj_node = obj_node->next;
    if (!obj_node) break;
    
    if IS_GROUP(obj) {
      read_connections(group_objects(obj), obj_node, objects_hash);
    } else {
      gboolean broken = FALSE;
      /* an invalid bounding box is a good sign for some need of corrections */
      gboolean wants_update = obj->bounding_box.left >= obj->bounding_box.left 
                           || obj->bounding_box.top >= obj->bounding_box.bottom;
      connections = obj_node->xmlChildrenNode;
      while ((connections!=NULL) &&
	     (xmlStrcmp(connections->name, (const xmlChar *)"connections")!=0))
	connections = connections->next;
      if (connections != NULL) {
	connection = connections->xmlChildrenNode;
	while (connection != NULL) {
	  char *donestr;
          if (xmlIsBlankNode(connection)) {
            connection = connection->next;
            continue;
          }
	  handlestr = (char * )xmlGetProp(connection, (const xmlChar *)"handle");
	  tostr = (char *) xmlGetProp(connection, (const xmlChar *)"to");
 	  connstr = (char *) xmlGetProp(connection, (const xmlChar *)"connection");
	  
	  handle = atoi(handlestr);
	  conn = strtol(connstr, &donestr, 10);
	  if (*donestr != '\0') { /* Didn't convert it all -- use string */
	    conn = -1;
	  }

	  to = g_hash_table_lookup(objects_hash, tostr);

	  if (to == NULL) {
	    message_error(_("Error loading diagram.\n"
			    "Linked object not found in document."));
	    broken = TRUE;
	  } else if (handle < 0 || handle >= obj->num_handles) {
	    message_error(_("Error loading diagram.\n"
			    "connection handle does not exist."));
	    broken = TRUE;
	  } else {
	    if (conn == -1) { /* Find named connpoint */
	      int i;
	      for (i = 0; i < to->num_connections; i++) {
		if (to->connections[i]->name != NULL &&
		    strcmp(to->connections[i]->name, connstr) == 0) {
		  conn = i;
		  break;
		}
	      }
	    }
	    if (conn >= 0 && conn < to->num_connections) {
	      object_connect(obj, obj->handles[handle],
			     to->connections[conn]);
	      /* force an update on the connection, helpful with (incomplete) generated files */
	      if (wants_update) {
	        obj->handles[handle]->pos = 
	          to->connections[conn]->last_pos = to->connections[conn]->pos;
#if 0
	        obj->ops->move_handle(obj, obj->handles[handle], &to->connections[conn]->pos,
				      to->connections[conn], HANDLE_MOVE_CONNECTED,0);
#endif
	      }
	    } else {
	      message_error(_("Error loading diagram.\n"
			      "connection point %s does not exist."),
			    connstr);
	      broken = TRUE;
	    }
	  }

	  if (handlestr) xmlFree(handlestr);
	  if (tostr) xmlFree(tostr);
	  if (connstr) xmlFree(connstr);

	  connection = connection->next;
	}
        /* Fix positions of the connection object for (de)generated files.
         * Only done for the last point connected otherwise the intermediate posisitions
         * may screw the auto-routing algorithm.
         */
        if (!broken && obj && obj->ops->set_props && wants_update) {
	  /* called for it's side-effect of update_data */
	  obj->ops->move(obj,&obj->position);

	  for (handle = 0; handle < obj->num_handles; ++handle) {
	    if (obj->handles[handle]->connected_to)
	      obj->ops->move_handle(obj, obj->handles[handle], &obj->handles[handle]->pos,
				    obj->handles[handle]->connected_to, HANDLE_MOVE_CONNECTED,0);
	  }
	}
      }
    }
    
    /* Now set up parent relationships. */
    connections = obj_node->xmlChildrenNode;
    while ((connections!=NULL) &&
	   (xmlStrcmp(connections->name, (const xmlChar *)"childnode")!=0))
      connections = connections->next;
    if (connections != NULL) {
      tostr = (char *)xmlGetProp(connections, (const xmlChar *)"parent");
      if (tostr) {
	obj->parent = g_hash_table_lookup(objects_hash, tostr);
	if (obj->parent == NULL) {
	  message_error(_("Can't find parent %s of %s object\n"), 
			tostr, obj->type->name);
	} else {
	  obj->parent->children = g_list_prepend(obj->parent->children, obj);
	}
      }
    }

    list = g_list_next(list);
    obj_node = obj_node->next;
  }
}

static void 
hash_free_string(gpointer       key,
		 gpointer       value,
		 gpointer       user_data)
{
  g_free(key);
}

static xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
  while (p && 0 != xmlStrcmp(p->name, (xmlChar *)name))
    p = p->next;
  return p;
}

static gboolean
diagram_data_load(const char *filename, DiagramData *data, void* user_data)
{
  GHashTable *objects_hash;
  int fd;
  GList *list;
  xmlDocPtr doc;
  xmlNodePtr diagramdata;
  xmlNodePtr paperinfo, gridinfo, guideinfo;
  xmlNodePtr layer_node;
  AttributeNode attr;
  Layer *layer;
  xmlNsPtr namespace;
  gchar firstchar;
  Diagram *diagram = DIA_IS_DIAGRAM (data) ? DIA_DIAGRAM (data) : NULL;

  if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
    message_error(_("You must specify a file, not a directory.\n"));
    return FALSE;
  }

  fd = g_open(filename, O_RDONLY, 0);

  if (fd==-1) {
    message_error(_("Couldn't open: '%s' for reading.\n"),
		  dia_message_filename(filename));
    return FALSE;
  }

  if (read(fd, &firstchar, 1)) {
    data->is_compressed = (firstchar != '<');
  } else {
    /* Couldn't read a single char?  Set to default. */
    data->is_compressed = prefs.new_diagram.compress_save;
  }
  
  /* Note that this closing and opening means we can't read from a pipe */
  close(fd);

  doc = xmlDiaParseFile(filename);

  if (doc == NULL){
    message_error(_("Error loading diagram %s.\nUnknown file type."),
		  dia_message_filename(filename));
    return FALSE;
  }
  
  if (doc->xmlRootNode == NULL) {
    message_error(_("Error loading diagram %s.\nUnknown file type."),
		  dia_message_filename(filename));
    xmlFreeDoc (doc);
    return FALSE;
  }

  namespace = xmlSearchNs(doc, doc->xmlRootNode, (const xmlChar *)"dia");
  if (xmlStrcmp (doc->xmlRootNode->name, (const xmlChar *)"diagram") || (namespace == NULL)){
    message_error(_("Error loading diagram %s.\nNot a Dia file."), 
		  dia_message_filename(filename));
    xmlFreeDoc (doc);
    return FALSE;
  }
  
  /* Destroy the default layer: */
  g_ptr_array_remove(data->layers, data->active_layer);
  layer_destroy(data->active_layer);
  
  diagramdata = 
    find_node_named (doc->xmlRootNode->xmlChildrenNode, "diagramdata");

  /* Read in diagram data: */
  data->bg_color = prefs.new_diagram.bg_color;
  attr = composite_find_attribute(diagramdata, "background");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &data->bg_color);

  if (diagram) {
    diagram->pagebreak_color = prefs.new_diagram.pagebreak_color;
    attr = composite_find_attribute(diagramdata, "pagebreak");
    if (attr != NULL)
      data_color(attribute_first_data(attr), &diagram->pagebreak_color);
  }
  /* load paper information from diagramdata section */
  attr = composite_find_attribute(diagramdata, "paper");
  if (attr != NULL) {
    paperinfo = attribute_first_data(attr);

    attr = composite_find_attribute(paperinfo, "name");
    if (attr != NULL) {
      g_free(data->paper.name);
      data->paper.name = data_string(attribute_first_data(attr));
    }
    if (data->paper.name == NULL || data->paper.name[0] == '\0') {
      data->paper.name = g_strdup(prefs.new_diagram.papertype);
    }
    /* set default margins for paper size ... */
    dia_page_layout_get_default_margins(data->paper.name,
					&data->paper.tmargin,
					&data->paper.bmargin,
					&data->paper.lmargin,
					&data->paper.rmargin);
    
    attr = composite_find_attribute(paperinfo, "tmargin");
    if (attr != NULL)
      data->paper.tmargin = data_real(attribute_first_data(attr));
    attr = composite_find_attribute(paperinfo, "bmargin");
    if (attr != NULL)
      data->paper.bmargin = data_real(attribute_first_data(attr));
    attr = composite_find_attribute(paperinfo, "lmargin");
    if (attr != NULL)
      data->paper.lmargin = data_real(attribute_first_data(attr));
    attr = composite_find_attribute(paperinfo, "rmargin");
    if (attr != NULL)
      data->paper.rmargin = data_real(attribute_first_data(attr));

    attr = composite_find_attribute(paperinfo, "is_portrait");
    data->paper.is_portrait = TRUE;
    if (attr != NULL)
      data->paper.is_portrait = data_boolean(attribute_first_data(attr));

    attr = composite_find_attribute(paperinfo, "scaling");
    data->paper.scaling = 1.0;
    if (attr != NULL)
      data->paper.scaling = data_real(attribute_first_data(attr));

    attr = composite_find_attribute(paperinfo, "fitto");
    data->paper.fitto = FALSE;
    if (attr != NULL)
      data->paper.fitto = data_boolean(attribute_first_data(attr));

    attr = composite_find_attribute(paperinfo, "fitwidth");
    data->paper.fitwidth = 1;
    if (attr != NULL)
      data->paper.fitwidth = data_int(attribute_first_data(attr));

    attr = composite_find_attribute(paperinfo, "fitheight");
    data->paper.fitheight = 1;
    if (attr != NULL)
      data->paper.fitheight = data_int(attribute_first_data(attr));

    /* calculate effective width/height */
    dia_page_layout_get_paper_size(data->paper.name,
				   &data->paper.width,
				   &data->paper.height);
    if (!data->paper.is_portrait) {
      gfloat tmp = data->paper.width;

      data->paper.width = data->paper.height;
      data->paper.height = tmp;
    }
    data->paper.width -= data->paper.lmargin;
    data->paper.width -= data->paper.rmargin;
    data->paper.height -= data->paper.tmargin;
    data->paper.height -= data->paper.bmargin;
    data->paper.width /= data->paper.scaling;
    data->paper.height /= data->paper.scaling;
  }

  if (diagram) {
    attr = composite_find_attribute(diagramdata, "grid");
    if (attr != NULL) {
      gridinfo = attribute_first_data(attr);

      attr = composite_find_attribute(gridinfo, "width_x");
      if (attr != NULL)
        diagram->grid.width_x = data_real(attribute_first_data(attr));
      attr = composite_find_attribute(gridinfo, "width_y");
      if (attr != NULL)
        diagram->grid.width_y = data_real(attribute_first_data(attr));
      attr = composite_find_attribute(gridinfo, "visible_x");
      if (attr != NULL)
        diagram->grid.visible_x = data_int(attribute_first_data(attr));
      attr = composite_find_attribute(gridinfo, "visible_y");
      if (attr != NULL)
        diagram->grid.visible_y = data_int(attribute_first_data(attr));

      diagram->grid.colour = prefs.new_diagram.grid_color;
      attr = composite_find_attribute(diagramdata, "color");
      if (attr != NULL)
        data_color(attribute_first_data(attr), &diagram->grid.colour);
    }
  }
  if (diagram) {
    attr = composite_find_attribute(diagramdata, "guides");
    if (attr != NULL) {
      guint i;
      DataNode guide;

      guideinfo = attribute_first_data(attr);

      attr = composite_find_attribute(guideinfo, "hguides");
      if (attr != NULL) {
        diagram->guides.nhguides = attribute_num_data(attr);
        g_free(diagram->guides.hguides);
        diagram->guides.hguides = g_new(real, diagram->guides.nhguides);
    
        guide = attribute_first_data(attr);
        for (i = 0; i < diagram->guides.nhguides; i++, guide = data_next(guide))
	  diagram->guides.hguides[i] = data_real(guide);
      }
      attr = composite_find_attribute(guideinfo, "vguides");
      if (attr != NULL) {
        diagram->guides.nvguides = attribute_num_data(attr);
        g_free(diagram->guides.vguides);
        diagram->guides.vguides = g_new(real, diagram->guides.nvguides);
    
        guide = attribute_first_data(attr);
        for (i = 0; i < diagram->guides.nvguides; i++, guide = data_next(guide))
	  diagram->guides.vguides[i] = data_real(guide);
      }
    }
  }

  /* Read in all layers: */
  layer_node =
    find_node_named (doc->xmlRootNode->xmlChildrenNode, "layer");

  objects_hash = g_hash_table_new(g_str_hash, g_str_equal);

  while (layer_node != NULL) {
    gchar *name;
    char *visible;
    
    if (xmlIsBlankNode(layer_node)) {
      layer_node = layer_node->next;
      continue;
    }

    if (!layer_node) break;
    
    name = (char *)xmlGetProp(layer_node, (const xmlChar *)"name");
    if (!name) break; /* name is mandatory */

    visible = (char *)xmlGetProp(layer_node, (const xmlChar *)"visible");

    layer = new_layer(g_strdup(name), data);
    if (name) xmlFree(name);

    layer->visible = FALSE;
    if ((visible) && (strcmp(visible, "true")==0))
      layer->visible = TRUE;
    if (visible) xmlFree(visible);

    /* Read in all objects: */
    
    list = read_objects(layer_node, objects_hash, filename, NULL);
    layer_add_objects (layer, list);
    read_connections( list, layer_node, objects_hash);

    data_add_layer(data, layer);

    layer_node = layer_node->next;
  }

  data->active_layer = (Layer *) g_ptr_array_index(data->layers, 0);

  xmlFreeDoc(doc);
  
  g_hash_table_foreach(objects_hash, hash_free_string, NULL);
  
  g_hash_table_destroy(objects_hash);

  if (data->layers->len < 1) {
    message_error (_("Error loading diagram:\n%s.\n"
                     "A valid Dia file defines at least one layer."),
		     dia_message_filename(filename));
    return FALSE;
  }

  return TRUE;
}

static gboolean
write_objects(GList *objects, xmlNodePtr objects_node,
	      GHashTable *objects_hash, int *obj_nr, const char *filename)
{
  char buffer[31];
  ObjectNode obj_node;
  xmlNodePtr group_node;
  GList *list;

  list = objects;
  while (list != NULL) {
    DiaObject *obj = (DiaObject *) list->data;

    if (g_hash_table_lookup(objects_hash, obj))
    {
      list = g_list_next(list);
      continue;
    }

    if (IS_GROUP(obj) && group_objects(obj) != NULL) {
      group_node = xmlNewChild(objects_node, NULL, (const xmlChar *)"group", NULL);
      write_objects(group_objects(obj), group_node,
		    objects_hash, obj_nr, filename);
    } else {
      obj_node = xmlNewChild(objects_node, NULL, (const xmlChar *)"object", NULL);
    
      xmlSetProp(obj_node, (const xmlChar *)"type", (xmlChar *)obj->type->name);
      g_snprintf(buffer, 30, "%d", obj->type->version);
      xmlSetProp(obj_node, (const xmlChar *)"version", (xmlChar *)buffer);
      
      g_snprintf(buffer, 30, "O%d", *obj_nr);
      xmlSetProp(obj_node, (const xmlChar *)"id", (xmlChar *)buffer);

      (*obj->type->ops->save)(obj, obj_node, filename);

      /* Add object -> obj_nr to hash table */
      g_hash_table_insert(objects_hash, obj, GINT_TO_POINTER(*obj_nr));
      (*obj_nr)++;
      
      /*
      if (object_flags_set(obj, DIA_OBJECT_CAN_PARENT) && obj->children)
      {
	int res;
	xmlNodePtr parent_node;
        parent_node = xmlNewChild(obj_node, NULL, "children", NULL);
        res = write_objects(obj->children, parent_node, objects_hash, obj_nr, filename);
	if (!res) return FALSE;
      }
      */
    }

    list = g_list_next(list);
  }
  return TRUE;
}

/* Parents broke assumption that both objects and xml nodes are laid out
 * linearly.  
 */

static gboolean
write_connections(GList *objects, xmlNodePtr layer_node,
		  GHashTable *objects_hash)
{
  ObjectNode obj_node;
  GList *list;
  xmlNodePtr connections;
  xmlNodePtr connection;
  char buffer[31];
  int i;

  list = objects;
  obj_node = layer_node->xmlChildrenNode;
  while ((list != NULL) && (obj_node != NULL)) {
    DiaObject *obj = (DiaObject *) list->data;

    while (obj_node && xmlIsBlankNode(obj_node)) obj_node = obj_node->next;
    if (!obj_node) break;

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
	  DiaObject *other_obj;
	  int con_point_nr;
	  
	  other_obj = con_point->object;
	  
	  con_point_nr=0;
	  while (other_obj->connections[con_point_nr] != con_point) {
	    con_point_nr++;
	    if (con_point_nr>=other_obj->num_connections) {
	      message_error("Internal error saving diagram\n con_point_nr >= other_obj->num_connections\n");
	      return FALSE;
	    }
	  }
	  
	  if (connections == NULL)
	    connections = xmlNewChild(obj_node, NULL, (const xmlChar *)"connections", NULL);
	  
	  connection = xmlNewChild(connections, NULL, (const xmlChar *)"connection", NULL);
	  /* from what handle on this object*/
	  g_snprintf(buffer, 30, "%d", i);
	  xmlSetProp(connection, (const xmlChar *)"handle", (xmlChar *)buffer);
	  /* to what object */
	  g_snprintf(buffer, 30, "O%d",
		     GPOINTER_TO_INT(g_hash_table_lookup(objects_hash,
							 other_obj)));

	  xmlSetProp(connection, (const xmlChar *)"to", (xmlChar *) buffer);
	  /* to what connection_point on that object */
	  if (other_obj->connections[con_point_nr]->name != NULL) {
	    g_snprintf(buffer, 30, "%s", other_obj->connections[con_point_nr]->name);
	  } else {
	    g_snprintf(buffer, 30, "%d", con_point_nr);
	  }
	  xmlSetProp(connection, (const xmlChar *)"connection", (xmlChar *) buffer);
	}
      }
    }

    if (obj->parent) {
      DiaObject *other_obj = obj->parent;
      xmlNodePtr parent;
      g_snprintf(buffer, 30, "O%d",
		 GPOINTER_TO_INT(g_hash_table_lookup(objects_hash, other_obj)));
      parent = xmlNewChild(obj_node, NULL, (const xmlChar *)"childnode", NULL);
      xmlSetProp(parent, (const xmlChar *)"parent", (xmlChar *)buffer);
    }
    
    list = g_list_next(list);
    obj_node = obj_node->next;
  }
  return TRUE;
}

/* Filename seems to be junk, but is passed on to objects */
static xmlDocPtr
diagram_data_write_doc(DiagramData *data, const char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNodePtr pageinfo, gridinfo, guideinfo;
  xmlNodePtr layer_node;
  GHashTable *objects_hash;
  gboolean res;
  int obj_nr;
  guint i;
  Layer *layer;
  AttributeNode attr;
  xmlNs *name_space;
  Diagram *diagram = DIA_IS_DIAGRAM (data) ? DIA_DIAGRAM (data) : NULL;

  doc = xmlNewDoc((const xmlChar *)"1.0");
  doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  doc->xmlRootNode = xmlNewDocNode(doc, NULL, (const xmlChar *)"diagram", NULL);

  name_space = xmlNewNs(doc->xmlRootNode, 
                        (const xmlChar *)DIA_XML_NAME_SPACE_BASE,
			(const xmlChar *)"dia");
  xmlSetNs(doc->xmlRootNode, name_space);

  tree = xmlNewChild(doc->xmlRootNode, name_space, (const xmlChar *)"diagramdata", NULL);
  
  attr = new_attribute((ObjectNode)tree, "background");
  data_add_color(attr, &data->bg_color);

  if (diagram) {
    attr = new_attribute((ObjectNode)tree, "pagebreak");
    data_add_color(attr, &diagram->pagebreak_color);
  }
  attr = new_attribute((ObjectNode)tree, "paper");
  pageinfo = data_add_composite(attr, "paper");
  data_add_string(composite_add_attribute(pageinfo, "name"),
		  data->paper.name);
  data_add_real(composite_add_attribute(pageinfo, "tmargin"),
		data->paper.tmargin);
  data_add_real(composite_add_attribute(pageinfo, "bmargin"),
		data->paper.bmargin);
  data_add_real(composite_add_attribute(pageinfo, "lmargin"),
		data->paper.lmargin);
  data_add_real(composite_add_attribute(pageinfo, "rmargin"),
		data->paper.rmargin);
  data_add_boolean(composite_add_attribute(pageinfo, "is_portrait"),
		   data->paper.is_portrait);
  data_add_real(composite_add_attribute(pageinfo, "scaling"),
		data->paper.scaling);
  data_add_boolean(composite_add_attribute(pageinfo, "fitto"),
		   data->paper.fitto);
  if (data->paper.fitto) {
    data_add_int(composite_add_attribute(pageinfo, "fitwidth"),
		 data->paper.fitwidth);
    data_add_int(composite_add_attribute(pageinfo, "fitheight"),
		 data->paper.fitheight);
  }

  if (diagram) {
    attr = new_attribute((ObjectNode)tree, "grid");
    gridinfo = data_add_composite(attr, "grid");
    data_add_real(composite_add_attribute(gridinfo, "width_x"),
		  diagram->grid.width_x);
    data_add_real(composite_add_attribute(gridinfo, "width_y"),
		  diagram->grid.width_y);
    data_add_int(composite_add_attribute(gridinfo, "visible_x"),
	         diagram->grid.visible_x);
    data_add_int(composite_add_attribute(gridinfo, "visible_y"),
	         diagram->grid.visible_y);
    attr = new_attribute((ObjectNode)tree, "color");
    data_add_composite(gridinfo, "color");
    data_add_color(attr, &diagram->grid.colour);
  
    attr = new_attribute((ObjectNode)tree, "guides");
    guideinfo = data_add_composite(attr, "guides");
    attr = composite_add_attribute(guideinfo, "hguides");
    for (i = 0; i < diagram->guides.nhguides; i++)
      data_add_real(attr, diagram->guides.hguides[i]);
    attr = composite_add_attribute(guideinfo, "vguides");
    for (i = 0; i < diagram->guides.nvguides; i++)
    data_add_real(attr, diagram->guides.vguides[i]);
  }

  objects_hash = g_hash_table_new(g_direct_hash, g_direct_equal);

  obj_nr = 0;

  for (i = 0; i < data->layers->len; i++) {
    layer_node = xmlNewChild(doc->xmlRootNode, name_space, (const xmlChar *)"layer", NULL);
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    xmlSetProp(layer_node, (const xmlChar *)"name", (xmlChar *)layer->name);

    if (layer->visible)
      xmlSetProp(layer_node, (const xmlChar *)"visible", (const xmlChar *)"true");
    else
      xmlSetProp(layer_node, (const xmlChar *)"visible", (const xmlChar *)"false");
    
    write_objects(layer->objects, layer_node,
		  objects_hash, &obj_nr, filename);
  
    res = write_connections(layer->objects, layer_node, objects_hash);
    /* Why do we bail out like this?  It leaks! */
    if (!res)
      return NULL;
  }
  g_hash_table_destroy(objects_hash);

  if (data->is_compressed)
    xmlSetDocCompressMode(doc, 9);
  else
    xmlSetDocCompressMode(doc, 0);

  return doc;
}

/** This tries to save the diagram into a file, without any backup
 * Returns >= 0 on success.
 * Only for internal use. */
static int
diagram_data_raw_save(DiagramData *data, const char *filename)
{
  xmlDocPtr doc;
  int ret;

  doc = diagram_data_write_doc(data, filename);

  ret = xmlDiaSaveFile (filename, doc);
  xmlFreeDoc(doc);

  return ret;
}

/** This saves the diagram, using a backup in case of failure.
 * @param data
 * @param filename
 * @returns TRUE on successfull save, FALSE otherwise.  If a failure is
 * indicated, an error message will already have been given to the user.
 */
static int
diagram_data_save(DiagramData *data, const char *filename)
{
  FILE *file;
  char *bakname,*tmpname,*dirname,*p;
  int mode,_umask;
  int fildes;
  int ret;

  /* Once we depend on GTK 2.8+, we can use these tests. */
#if GLIB_CHECK_VERSION(2,8,0) && !defined G_OS_WIN32
  /* Check that we're allowed to write to the target file at all. */
  /* not going to work with 'My Docments' - read-only but still useable, see bug #504469 */
  if (   g_file_test(filename, G_FILE_TEST_EXISTS)
      && g_access(filename, W_OK) != 0) {
    message_error(_("Not allowed to write to output file %s\n"), 
		  dia_message_filename(filename));
    return FALSE;
  }
#endif

  /* build the temporary and backup file names */
  dirname = g_strdup(filename);
  p = strrchr((char *)dirname,G_DIR_SEPARATOR);
  if (p) {
    *(p+1) = 0;
  } else {
    g_free(dirname);
    dirname = g_strdup("." G_DIR_SEPARATOR_S);
  }
  tmpname = g_strconcat(dirname,"__diaXXXXXX",NULL);
  bakname = g_strconcat(filename,"~",NULL);

#if GLIB_CHECK_VERSION(2,8,0) && !defined G_OS_WIN32
  /* Check that we can create the other files */
  if (   g_file_test(dirname, G_FILE_TEST_EXISTS) 
      && g_access(dirname, W_OK) != 0) {
    message_error(_("Not allowed to write temporary files in %s\n"), 
		  dia_message_filename(dirname));
    return FALSE;
  }
#endif

  /* open a temporary name, and fix the modes to match what fopen() would have
     done (mkstemp() is (rightly so) a bit paranoid for what we do).  */
  fildes = g_mkstemp(tmpname);
  /* should not be necessary anymore on *NIXas well, because we are using g_mkstemp ? */
  _umask = umask(0); umask(_umask);
  mode = 0666 & ~_umask;
#ifndef G_OS_WIN32
  ret = fchmod(fildes,mode);
#else
  ret = 0; /* less paranoia on windoze */ 
#endif
  file = fdopen(fildes,"wb");

  /* Now write the data in the temporary file name. */

  if (file==NULL) {
    message_error(_("Can't open output file %s: %s\n"), 
		  dia_message_filename(tmpname), strerror(errno));
    return FALSE;
  }
  fclose(file);

  ret = diagram_data_raw_save(data, tmpname);

  if (ret < 0) {
    /* Save failed; we clean our stuff up, without touching the file named
       "filename" if it existed. */
    message_error(_("Internal error %d writing file %s\n"), 
		  ret, dia_message_filename(tmpname));
    g_unlink(tmpname);
    g_free(tmpname);
    g_free(dirname);
    g_free(bakname);
    return FALSE;
  }
  /* save succeeded. We kill the old backup file, move the old file into 
     backup, and the temp file into the new saved file. */
  g_unlink(bakname);
  g_rename(filename,bakname);
  ret = g_rename(tmpname,filename);
  if (ret < 0) {
    message_error(_("Can't rename %s to final output file %s: %s\n"), 
		  dia_message_filename(filename), 
		  dia_message_filename(filename), strerror(errno));
  }
  g_free(tmpname);
  g_free(dirname);
  g_free(bakname);
  return (ret?FALSE:TRUE);
}

int
diagram_save(Diagram *dia, const char *filename)
{
  gboolean res = diagram_data_save(dia->data, filename);

  if (!res) {
    return res;
  }

  dia->unsaved = FALSE;
  undo_mark_save(dia->undo);
  diagram_set_modified (dia, FALSE);

  diagram_cleanup_autosave(dia);

  return TRUE;
}

/* Autosave stuff.  Needs to use low-level save to avoid setting and resetting flags */
void
diagram_cleanup_autosave(Diagram *dia)
{
  gchar *savefile;
  struct stat statbuf;

  savefile = dia->autosavefilename;
  if (savefile == NULL) return;
#ifdef TRACES
  g_print("Cleaning up autosave %s for %s\n", 
          savefile, dia->filename ? dia->filename : "<no name>");
#endif
  if (g_stat(savefile, &statbuf) == 0) { /* Success */
    g_unlink(savefile);
  }
  g_free(savefile);
  dia->autosavefilename = NULL;
  dia->autosaved = FALSE;
}

/** Absolutely autosave a diagram.
 * Called after a periodic check at the first idleness.
 */
void
diagram_autosave(Diagram *dia)
{
  gchar *save_filename;

  /* Must check if the diagram is still valid, or Death Ensues! */
  GList *diagrams = dia_open_diagrams();
  Diagram *diagram;
  while (diagrams != NULL) {
    diagram = (Diagram *)diagrams->data;
    if (diagram == dia &&
	diagram_is_modified(diagram) && 
	!diagram->autosaved) {
      save_filename = g_strdup_printf("%s.autosave", dia->filename);

      if (dia->autosavefilename != NULL) 
	g_free(dia->autosavefilename);
      dia->autosavefilename = save_filename;
      diagram_data_raw_save(dia->data, save_filename);
      dia->autosaved = TRUE;
      return;
    }
    diagrams = g_list_next(diagrams);
  }
}

/* --- filter interfaces --- */
static void
export_native(DiagramData *data, const gchar *filename,
	      const gchar *diafilename, void* user_data)
{
  diagram_data_save(data, filename);
}

static const gchar *extensions[] = { "dia", NULL };
DiaExportFilter dia_export_filter = {
  N_("Dia Diagram File"),
  extensions,
  export_native
};
DiaImportFilter dia_import_filter = {
  N_("Dia Diagram File"),
  extensions,
  diagram_data_load
};
