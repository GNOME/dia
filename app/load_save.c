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

#include "intl.h"

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include "load_save.h"
#include "group.h"
#include "diagramdata.h"
#include "message.h"
#include "preferences.h"
#include "diapagelayout.h"
#include "autosave.h"

#ifdef G_OS_WIN32
#include <io.h>
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | _O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif
#ifdef __EMX__
#define mkstemp(s) _open(_mktemp(s), O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0644)
#define fchmod(f,m) (0)
#endif

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

static GList *
read_objects(xmlNodePtr objects, Layer *layer,
             GHashTable *objects_hash,const char *filename)
{
  GList *list;
  ObjectType *type;
  Object *obj;
  ObjectNode obj_node;
  char *typestr;
  char *versionstr;
  char *id;
  int version;
  GHashTable* unknown_hash;
  GString*    unknown_str;

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

    if (strcmp(obj_node->name, "object")==0) {
      typestr = xmlGetProp(obj_node, "type");
      versionstr = xmlGetProp(obj_node, "version");
      id = xmlGetProp(obj_node, "id");
      
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
        layer_add_object(layer,obj);
        list = g_list_append(list, obj);
      
        g_hash_table_insert(objects_hash, g_strdup((char *)id), obj);
      }
      if (typestr) xmlFree(typestr);
      if (id) xmlFree (id);
    } else if (strcmp(obj_node->name, "group")==0) {
      obj = group_create(read_objects(obj_node, layer,
                                      objects_hash, filename));
      layer_add_object(layer,obj);
      
      list = g_list_append(list, obj);
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
    message_error(unknown_str->str);
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
  Object *to;
  
  list = objects;
  obj_node = layer_node->xmlChildrenNode;
  while ((list != NULL) && (obj_node != NULL)) {
    Object *obj = (Object *) list->data;
    
    while (obj_node && xmlIsBlankNode(obj_node)) obj_node = obj_node->next;
    if (!obj_node) break;
    
    if IS_GROUP(obj) {
      read_connections(group_objects(obj), obj_node, objects_hash);
    } else {
      connections = obj_node->xmlChildrenNode;
      while ((connections!=NULL) &&
	     (strcmp(connections->name, "connections")!=0))
	connections = connections->next;
      if (connections != NULL) {
	connection = connections->xmlChildrenNode;
	while (connection != NULL) {
          if (xmlIsBlankNode(connection)) {
            connection = connection->next;
            continue;
          }
	  handlestr = xmlGetProp(connection, "handle");
	  tostr = xmlGetProp(connection, "to");
	  connstr = xmlGetProp(connection, "connection");
	  
	  handle = atoi(handlestr);
	  conn = atoi(connstr);
	  
	  to = g_hash_table_lookup(objects_hash, tostr);

	  if (handlestr) xmlFree(handlestr);
	  if (connstr) xmlFree(connstr);
	  if (tostr) xmlFree(tostr);

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
  g_free(key);
}

static xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
  while (p && 0 != strcmp(p->name, name))
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
  
  if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
    message_error(_("You must specify a file, not a directory.\n"));
    return FALSE;
  }

  fd = open(filename, O_RDONLY);

  if (fd==-1) {
    message_error(_("Couldn't open: '%s' for reading.\n"), filename);
    return FALSE;
  }

  close(fd);

  doc = xmlDiaParseFile(filename);

  if (doc == NULL){
    message_error(_("Error loading diagram %s.\nUnknown file type."),filename);
    return FALSE;
  }
  
  if (doc->xmlRootNode == NULL) {
    message_error(_("Error loading diagram %s.\nUnknown file type."),filename);
    xmlFreeDoc (doc);
    return FALSE;
  }

  namespace = xmlSearchNs(doc, doc->xmlRootNode, "dia");
  if (strcmp (doc->xmlRootNode->name, "diagram") || (namespace == NULL)){
    message_error(_("Error loading diagram %s.\nNot a Dia file."), filename);
    xmlFreeDoc (doc);
    return FALSE;
  }
  
  /* Destroy the default layer: */
  g_ptr_array_remove(data->layers, data->active_layer);
  layer_destroy(data->active_layer);
  
  diagramdata = 
    find_node_named (doc->xmlRootNode->xmlChildrenNode, "diagramdata");

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
  attr = composite_find_attribute(diagramdata, "grid");
  if (attr != NULL) {
    gridinfo = attribute_first_data(attr);

    attr = composite_find_attribute(gridinfo, "width_x");
    if (attr != NULL)
      data->grid.width_x = data_real(attribute_first_data(attr));
    attr = composite_find_attribute(gridinfo, "width_y");
    if (attr != NULL)
      data->grid.width_y = data_real(attribute_first_data(attr));
    attr = composite_find_attribute(gridinfo, "visible_x");
    if (attr != NULL)
      data->grid.visible_x = data_int(attribute_first_data(attr));
    attr = composite_find_attribute(gridinfo, "visible_y");
    if (attr != NULL)
      data->grid.visible_y = data_int(attribute_first_data(attr));
  }
  attr = composite_find_attribute(diagramdata, "guides");
  if (attr != NULL) {
    guint i;
    DataNode guide;

    guideinfo = attribute_first_data(attr);

    attr = composite_find_attribute(guideinfo, "hguides");
    if (attr != NULL) {
      data->guides.nhguides = attribute_num_data(attr);
      g_free(data->guides.hguides);
      data->guides.hguides = g_new(real, data->guides.nhguides);
    
      guide = attribute_first_data(attr);
      for (i = 0; i < data->guides.nhguides; i++, guide = data_next(guide))
	data->guides.hguides[i] = data_real(guide);
    }
    attr = composite_find_attribute(guideinfo, "vguides");
    if (attr != NULL) {
      data->guides.nvguides = attribute_num_data(attr);
      g_free(data->guides.vguides);
      data->guides.vguides = g_new(real, data->guides.nvguides);
    
      guide = attribute_first_data(attr);
      for (i = 0; i < data->guides.nvguides; i++, guide = data_next(guide))
	data->guides.vguides[i] = data_real(guide);
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
    
    name = (char *)xmlGetProp(layer_node, "name");
    if (!name) break; /* name is mandatory */

    visible = (char *)xmlGetProp(layer_node, "visible");

    layer = new_layer(g_strdup(name), data);
    if (name) xmlFree(name);

    layer->visible = FALSE;
    if ((visible) && (strcmp(visible, "true")==0))
      layer->visible = TRUE;
    if (visible) xmlFree(visible);

    /* Read in all objects: */
    
    list = read_objects(layer_node, layer, objects_hash, filename);
    layer->objects = list;
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
		     filename);
    return FALSE;
  }

  return TRUE;
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
      group_node = diaXmlNewChild(objects_node, NULL, "group", NULL);
      write_objects(group_objects(obj), group_node,
		    objects_hash, obj_nr, filename);
    } else {
      obj_node = diaXmlNewChild(objects_node, NULL, "object", NULL);
    
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
  obj_node = layer_node->xmlChildrenNode;
  while ((list != NULL) && (obj_node != NULL)) {
    Object *obj = (Object *) list->data;

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
	    connections = diaXmlNewChild(obj_node, NULL, "connections", NULL);
	  
	  connection = diaXmlNewChild(connections, NULL, "connection", NULL);
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

/* Filename seems to be junk, but is passed on to objects */
static xmlDocPtr
diagram_data_write_doc(DiagramData *data, const char *filename)
{
  xmlDocPtr doc;
  xmlNodePtr tree;
  xmlNodePtr pageinfo, gridinfo, guideinfo;
  xmlNodePtr layer_node;
  GHashTable *objects_hash;
  int res;
  int obj_nr;
  int i;
  Layer *layer;
  AttributeNode attr;
  xmlNs *name_space;

  doc = xmlNewDoc("1.0");
  doc->encoding = xmlStrdup("UTF-8");
  doc->xmlRootNode = xmlNewDocNode(doc, NULL, "diagram", NULL);

  name_space = xmlNewNs(doc->xmlRootNode, 
                        "http://www.lysator.liu.se/~alla/dia/",
			"dia");
  xmlSetNs(doc->xmlRootNode, name_space);

  tree = diaXmlNewChild(doc->xmlRootNode, name_space, "diagramdata", NULL);
  
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

  attr = new_attribute((ObjectNode)tree, "grid");
  gridinfo = data_add_composite(attr, "grid");
  data_add_real(composite_add_attribute(gridinfo, "width_x"),
		data->grid.width_x);
  data_add_real(composite_add_attribute(gridinfo, "width_y"),
		data->grid.width_y);
  data_add_int(composite_add_attribute(gridinfo, "visible_x"),
	       data->grid.visible_x);
  data_add_int(composite_add_attribute(gridinfo, "visible_y"),
	       data->grid.visible_y);

  attr = new_attribute((ObjectNode)tree, "guides");
  guideinfo = data_add_composite(attr, "guides");
  attr = composite_add_attribute(guideinfo, "hguides");
  for (i = 0; i < data->guides.nhguides; i++)
    data_add_real(attr, data->guides.hguides[i]);
  attr = composite_add_attribute(guideinfo, "vguides");
  for (i = 0; i < data->guides.nvguides; i++)
    data_add_real(attr, data->guides.vguides[i]);

  objects_hash = g_hash_table_new(g_direct_hash, g_direct_equal);

  obj_nr = 0;

  for (i = 0; i < data->layers->len; i++) {
    layer_node = diaXmlNewChild(doc->xmlRootNode, name_space, "layer", NULL);
    layer = (Layer *) g_ptr_array_index(data->layers, i);
    xmlSetProp(layer_node, "name", layer->name);

    if (layer->visible)
      xmlSetProp(layer_node, "visible", "true");
    else
      xmlSetProp(layer_node, "visible", "false");
    
    write_objects(layer->objects, layer_node,
		  objects_hash, &obj_nr, filename);
  
    res = write_connections(layer->objects, layer_node, objects_hash);
    /* Why do we bail out like this?  It leaks! */
    if (!res)
      return NULL;
  }
  g_hash_table_destroy(objects_hash);

  if (prefs.compress_save)
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

/** This saves the diagram, using a backup in case of failure */
static int
diagram_data_save(DiagramData *data, const char *filename)
{
  FILE *file;
  char *bakname,*tmpname,*dirname,*p;
  int mode,_umask;
  int fildes;
  int ret;
   
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

  ret = diagram_data_raw_save(data, tmpname);

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

  if (stat(savefile, &statbuf) == 0) { /* Success */
    unlink(savefile);
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
  GList *diagrams = open_diagrams;
  Diagram *diagram;
  while (diagrams != NULL) {
    diagram = (Diagram *)diagrams->data;
    if (diagram == dia &&
	diagram->modified && 
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
  N_("Native Dia Diagram"),
  extensions,
  export_native
};
DiaImportFilter dia_import_filter = {
  N_("Native Dia Diagram"),
  extensions,
  diagram_data_load
};
