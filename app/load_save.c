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
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <glib.h>

#include "config.h"
#include "intl.h"

/* gnome-xml: */
#include <tree.h>
#include <parser.h>

#include "load_save.h"
#include "dia_xml.h"
#include "group.h"
#include "message.h"
#include "preferences.h"
#include "diapagelayout.h"

#ifdef G_OS_WIN32
#include <io.h>
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif
#ifdef __EMX__
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif

static GList *
read_objects(xmlNodePtr objects, GHashTable *objects_hash,const char *filename)
{
  GList *list;
  ObjectType *type;
  Object *obj;
  ObjectNode obj_node;
  char *typestr;
  char *versionstr;
  char *id;
  int version;

  list = NULL;

  obj_node = objects->childs;
  
  while ( obj_node != NULL) {

    if (strcmp(obj_node->name, "object")==0) {
      typestr = xmlGetProp(obj_node, "type");
      versionstr = xmlGetProp(obj_node, "version");
      id = xmlGetProp(obj_node, "id");
      
      version = 0;
      if (versionstr != NULL) {
	version = atoi(versionstr);
	free(versionstr);
      }
      
      type = object_get_type((char *)typestr);
      if (typestr) free(typestr);
      
      obj = type->ops->load(obj_node, version, filename);
      list = g_list_append(list, obj);
      
      g_hash_table_insert(objects_hash, (char *)id, obj);
   
    } else if (strcmp(obj_node->name, "group")==0) {
      obj = group_create(read_objects(obj_node, objects_hash, filename));
      list = g_list_append(list, obj);
    } else {
      message_error(_("Error reading diagram file\n"));
    }

    obj_node = obj_node->next;
  }
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
  Object *to;
  
  list = objects;
  obj_node = layer_node->childs;
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

	  if (handlestr) free(handlestr);
	  if (connstr) free(connstr);
	  if (tostr) free(tostr);

	  if (to == NULL) {
	    message_error(_("Error loading diagram.\n"
			    "Linked object not found in document."));
	  } else if (conn >= 0 && conn >= to->num_connections) {
	    message_error(_("Error loading diagram.\n"
			    "connection point does not exist."));
	  } else if (handle >= 0 && handle >= obj->num_handles) {
	    message_error(_("Error loading diagram.\n"
			    "connection handle does not exist."));
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

static void 
hash_free_string(gpointer       key,
		 gpointer       value,
		 gpointer       user_data)
{
  free(key);
}

static gboolean
diagram_data_load(const char *filename, DiagramData *data)
{
  GHashTable *objects_hash;
  int fd;
  struct stat stat_buf;
  GList *list;
  xmlDocPtr doc;
  xmlNodePtr diagramdata;
  xmlNodePtr paperinfo;
  xmlNodePtr layer_node;
  AttributeNode attr;
  Layer *layer;
  xmlNsPtr namespace;
  
  fd = open(filename, O_RDONLY);

  if (fd==-1) {
    message_error(_("Couldn't open: '%s' for reading.\n"), filename);
    return FALSE;
  }

  fstat(fd, &stat_buf);
  if (S_ISDIR(stat_buf.st_mode)) {
    message_error(_("You must specify a file, not a directory.\n"));
    return FALSE;
  }
  
  close(fd);

  doc = xmlParseFile(filename);

  if (doc == NULL){
    message_error(_("Error loading diagram %s.\nUnknown file type."),filename);
    return FALSE;
  }
  
  if (doc->root == NULL) {
    message_error(_("Error loading diagram %s.\nUnknown file type."),filename);
    xmlFreeDoc (doc);
    return FALSE;
  }

  namespace = xmlSearchNs(doc, doc->root, "dia");
  if (strcmp (doc->root->name, "diagram") || (namespace == NULL)){
    message_error(_("Error loading diagram %s.\nNot a Dia file."), filename);
    xmlFreeDoc (doc);
    return FALSE;
  }
  
  /* Destroy the default layer: */
  g_ptr_array_remove(data->layers, data->active_layer);
  layer_destroy(data->active_layer);
  
  diagramdata = doc->root->childs;

  /* Read in diagram data: */
  data->bg_color = color_white;
  attr = composite_find_attribute(diagramdata, "background");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &data->bg_color);

  /* load paper information from diagramdata section */
  attr = composite_find_attribute(diagramdata, "paper");
  if (attr != NULL) {
    paperinfo = attribute_first_data(attr);

    attr = composite_find_attribute(paperinfo, "name");
    if (attr != NULL) {
      g_free(data->paper.name);
      data->paper.name = data_string(attribute_first_data(attr));
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

  /* Read in all layers: */
  layer_node = diagramdata->next;

  objects_hash = g_hash_table_new(g_str_hash, g_str_equal);

  while (layer_node != NULL) {
    char *name;
    char *visible;
    name = (char *)xmlGetProp(layer_node, "name");
    visible = (char *)xmlGetProp(layer_node, "visible");
    layer = new_layer(g_strdup(name));
    if (name) free(name);
    
    layer->visible = FALSE;
    if ((visible) && (strcmp(visible, "true")==0))
      layer->visible = TRUE;
    if (visible) free(visible);

    /* Read in all objects: */
    
    list = read_objects(layer_node, objects_hash, filename);
    layer->objects = list;
    read_connections( list, layer_node, objects_hash);

    data_add_layer(data, layer);

    layer_node = layer_node->next;
  }

  data->active_layer = (Layer *) g_ptr_array_index(data->layers, 0);
  
  xmlFreeDoc(doc);
  
  g_hash_table_foreach(objects_hash, hash_free_string, NULL);
  
  g_hash_table_destroy(objects_hash);
  
  return TRUE;
}

Diagram *
diagram_load(const char *filename)
{
  Diagram *dia;

  dia = new_diagram(filename);
  if (diagram_data_load(filename, dia->data)) {
    dia->unsaved = FALSE;
    diagram_set_modified(dia, FALSE);
  } else {
    diagram_destroy(dia);
    dia = NULL;
  }
  return dia;
}

static void
write_objects(GList *objects, xmlNodePtr objects_node,
	      GHashTable *objects_hash, int *obj_nr, const char *filename)
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
      write_objects(group_objects(obj), group_node,
		    objects_hash, obj_nr, filename);
    } else {
      obj_node = xmlNewChild(objects_node, NULL, "object", NULL);
    
      xmlSetProp(obj_node, "type", obj->type->name);
      
      g_snprintf(buffer, 30, "%d", obj->type->version);
      xmlSetProp(obj_node, "version", buffer);
      
      g_snprintf(buffer, 30, "O%d", *obj_nr);
      xmlSetProp(obj_node, "id", buffer);

      (*obj->type->ops->save)(obj, obj_node, filename);

      /* Add object -> obj_nr to hash table */
      g_hash_table_insert(objects_hash, obj, GINT_TO_POINTER(*obj_nr));
      (*obj_nr)++;
      
    }
    
    list = g_list_next(list);
  }
}

static int
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
  obj_node = layer_node->childs;
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
	      message_error("Internal error saving diagram\n con_point_nr >= other_obj->num_connections\n");
	      return FALSE;
	    }
	  }
	  
	  if (connections == NULL)
	    connections = xmlNewChild(obj_node, NULL, "connections", NULL);
	  
	  connection = xmlNewChild(connections, NULL, "connection", NULL);
	  /* from what handle on this object*/
	  g_snprintf(buffer, 30, "%d", i);
	  xmlSetProp(connection, "handle", buffer);
	  /* to what object */
	  g_snprintf(buffer, 30, "O%d",
		     GPOINTER_TO_INT(g_hash_table_lookup(objects_hash,
							 other_obj)));
	  xmlSetProp(connection, "to", buffer);
	  /* to what connection_point on that object */
	  g_snprintf(buffer, 30, "%d", con_point_nr);
	  xmlSetProp(connection, "connection", buffer);
	}
      }
    }
    
    list = g_list_next(list);
    obj_node = obj_node->next;
  }
  return TRUE;
}

static int
diagram_data_save(DiagramData *data, const char *filename)
{
  FILE *file;
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNodePtr pageinfo;
  xmlNodePtr layer_node;
  GHashTable *objects_hash;
  int res;
  int obj_nr;
  int i;
  Layer *layer;
  AttributeNode attr;
  xmlNs *name_space;
  char *bakname,*tmpname,*dirname,*p;
  int mode,_umask;
  int fildes;
  int ret;
   
  /* build the temporary and backup file names */
  dirname = g_strdup(filename);
  p = strrchr((char *)dirname,'/');
  if (p) {
    *(p+1) = 0;
  } else {
    g_free(dirname);
    dirname = g_strdup("./");
  }
  tmpname = g_strconcat(dirname,"__diaXXXXXX",NULL);
  bakname = g_strconcat(filename,"~",NULL);

  /* open a temporary name, and fix the modes to match what fopen() would have
     done (mkstemp() is (rightly so) a bit paranoid for what we do). */
  fildes = mkstemp(tmpname);
  _umask = umask(0); umask(_umask);
  mode = 0666 & ~_umask;
  ret = fchmod(fildes,mode);
  file = fdopen(fildes,"wb");

  /* Now write the data in the temporary file name. */

  if (file==NULL) {
    message_error(_("Couldn't open: '%s' for writing.\n"), tmpname);
    return FALSE;
  }
  fclose(file);

  doc = xmlNewDoc("1.0");
  
  name_space = xmlNewGlobalNs( doc, "http://www.lysator.liu.se/~alla/dia/",
			       "dia" );
 
  doc->root = xmlNewDocNode(doc, name_space, "diagram", NULL);

  tree = xmlNewChild(doc->root, NULL, "diagramdata", NULL);
  
  attr = new_attribute((ObjectNode)tree, "background");
  data_add_color(attr, &data->bg_color);

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

  objects_hash = g_hash_table_new(g_direct_hash, g_direct_equal);

  obj_nr = 0;

  for (i = 0; i < data->layers->len; i++) {
    layer_node = xmlNewChild(doc->root, NULL, "layer", NULL);
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    xmlSetProp(layer_node, "name", layer->name);
    if (layer->visible)
      xmlSetProp(layer_node, "visible", "true");
    else
      xmlSetProp(layer_node, "visible", "false");
    
    write_objects(layer->objects, layer_node,
		  objects_hash, &obj_nr, filename);
  
    res = write_connections(layer->objects, layer_node, objects_hash);
    if (!res)
      return FALSE;
  }
  g_hash_table_destroy(objects_hash);

  if (prefs.compress_save)
    xmlSetDocCompressMode(doc, 9);
  else
    xmlSetDocCompressMode(doc, 0);
    
  ret = xmlSaveFile (tmpname, doc);
  xmlFreeDoc(doc);

  if (ret < 0) {
    /* Save failed; we clean our stuff up, without touching the file named
       "filename" if it existed. */
    unlink(tmpname);
    g_free(tmpname);
    g_free(dirname);
    g_free(bakname);
    return FALSE;
  }
  /* save succeeded. We kill the old backup file, move the old file into 
     backup, and the temp file into the new saved file. */
  unlink(bakname);
  rename(filename,bakname);
  ret = rename(tmpname,filename);
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
    message_error(_("Failed to save file '%s'.\n"), filename);    
    return res;
  }

  dia->unsaved = FALSE;
  diagram_set_modified (dia, FALSE);

  return TRUE;
}

/* --- filter interfaces --- */
static void
export_native(DiagramData *data, const gchar *filename,
	      const gchar *diafilename)
{
  diagram_data_save(data, filename);
}

static const gchar *extensions[] = { "dia", NULL };
DiaExportFilter dia_export_filter = {
  N_("Native Dia Diagram"),
  extensions,
  export_native
};
DiaImportFilter dia_import_filter = {
  N_("Native Dia Diagram"),
  extensions,
  diagram_data_load
};
