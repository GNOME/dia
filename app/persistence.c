/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 2003 Lars Clausen
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

/* persistence.c -- functions that handle persistent stores, such as 
 * window positions and sizes, font menu, document history etc.
 */

#include "config.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "persistence.h"
#include "dia_dirs.h"
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include <gtk/gtk.h>
#include <libxml/tree.h>

/* Hash table from window role (string) to PersistentWindow structure.
 */
static GHashTable *persistent_windows;

/* Returns the name used for a window in persistence.
 */
static gchar *
persistence_get_window_name(GtkWindow *window)
{
  gchar *name = gtk_window_get_role(window);
  if (name == NULL) {
    printf("Internal:  Window %s has no role.\n", gtk_window_get_title(window));
    return NULL;
  }
  return name;
}

static xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
  while (p && 0 != strcmp(p->name, name))
    p = p->next;
  return p;
}

static void
persistence_load_window(xmlNodePtr node)
{
  AttributeNode attr;
  PersistentWindow *wininfo = g_new0(PersistentWindow, 1);
  gchar *name;

  attr = composite_find_attribute(node, "role");
  if (attr != NULL) {
    name = data_string(attribute_first_data(attr));
  } else {
    g_free(wininfo);
    return;
  }

  attr = composite_find_attribute(node, "xpos");
  if (attr != NULL)
    wininfo->x = data_int(attribute_first_data(attr));
  attr = composite_find_attribute(node, "ypos");
  if (attr != NULL)
    wininfo->y = data_int(attribute_first_data(attr));
  attr = composite_find_attribute(node, "width");
  if (attr != NULL)
    wininfo->width = data_int(attribute_first_data(attr));
  attr = composite_find_attribute(node, "height");
  if (attr != NULL)
    wininfo->height = data_int(attribute_first_data(attr));
  attr = composite_find_attribute(node, "isopen");
  if (attr != NULL)
    wininfo->isopen = data_boolean(attribute_first_data(attr));

  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  } else {
    /* Do anything to avoid dups? */
  }
  g_hash_table_insert(persistent_windows, name, wininfo);
}

/* Load all persistent data. */
void
persistence_load()
{
  xmlDocPtr doc;
  gchar *filename = dia_config_filename("persistence");
  struct stat statbuf;

  if (stat(filename, &statbuf) == -1) return;

  doc = xmlDiaParseFile(filename);
  if (doc != NULL) {
    if (doc->xmlRootNode != NULL) {
      xmlNsPtr namespace = xmlSearchNs(doc, doc->xmlRootNode, "dia");
      if (!strcmp (doc->xmlRootNode->name, "persistence") &&
	  namespace != NULL) {
	xmlNodePtr window_node;
	window_node = find_node_named(doc->xmlRootNode->xmlChildrenNode,
				      "window");
	while (window_node != NULL) {
	  persistence_load_window(window_node);
	  window_node = window_node->next;
	}
      }
    }
    xmlFreeDoc(doc);
  }
  g_free(filename);
}

static void
persistence_store_window_info(GtkWindow *window, PersistentWindow *wininfo,
			      gboolean isclosed)
{
  /* Drawable means visible & mapped, what we usually think of as open. */
  if (!isclosed) {
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    gtk_window_get_size(window, &wininfo->width, &wininfo->height);
    wininfo->isopen = TRUE;
  } else {
    wininfo->isopen = FALSE;
  }
}

gboolean
persistence_update_window(GtkWindow *window, GdkEvent *event, gpointer data)
{
  gchar *name = persistence_get_window_name(window);
  PersistentWindow *wininfo;
  gboolean isclosed;

  if (name == NULL) return FALSE;
  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);

  /* Can't tell the window state from the window itself yet. */
  isclosed = (event->type == GDK_UNMAP);
  if (wininfo != NULL) {
    persistence_store_window_info(window, wininfo, isclosed);
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    persistence_store_window_info(window, wininfo, isclosed);
    g_hash_table_insert(persistent_windows, name, wininfo);
  }
  if (wininfo->window != NULL && wininfo->window != window) {
    g_object_unref(wininfo->window);
    wininfo->window = NULL;
  }
  if (wininfo->window == NULL) {
    wininfo->window = window;
    g_object_ref(window);
  }
  return FALSE;
}

/* Call this function after the window has a role assigned to use any
 * persistence information about the window.
 */
void
persistence_register_window(GtkWindow *window)
{
  gchar *name = persistence_get_window_name(window);
  PersistentWindow *wininfo;

  if (name == NULL) return;
  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);
  if (wininfo != NULL) {
    gtk_window_move(window, wininfo->x, wininfo->y);
    gtk_window_resize(window, wininfo->width, wininfo->height);
    if (wininfo->isopen) gtk_widget_show(window);
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    gtk_window_get_size(window, &wininfo->width, &wininfo->height);
    /* Drawable means visible & mapped, what we usually think of as open. */
    wininfo->isopen = GTK_WIDGET_DRAWABLE(GTK_WIDGET(window));
    g_hash_table_insert(persistent_windows, name, wininfo);
  }
  if (wininfo->window != NULL && wininfo->window != window) {
    g_object_unref(wininfo->window);
    wininfo->window = NULL;
  }
  if (wininfo->window == NULL) {
    wininfo->window = window;
    g_object_ref(window);
  }

  gtk_signal_connect(GTK_OBJECT(window), "configure-event",
		     GTK_SIGNAL_FUNC(persistence_update_window), NULL);

  gtk_signal_connect(GTK_OBJECT(window), "unmap-event",
		     GTK_SIGNAL_FUNC(persistence_update_window), NULL);
}

/** Call this function at start-up to have a window creation function
 * called if the window should be opened at startup.
 * If no persistence information is available for the given role,
 * nothing happens.
 * @arg role The role of the window, as will be set by gtk_window_set_role()
 * @arg createfunc A 0-argument function that creates the window.  This
 * function will be called if the persistence information indicates that the
 * window should be open.  The function should create and show the window.
 */
void
persistence_register_window_create(gchar *role, NullaryFunc *func)
{
  PersistentWindow *wininfo;

  if (role == NULL) return;
  if (persistent_windows == NULL) return;
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, role);
  if (wininfo != NULL) {
    (*func)();
  }
}

/* Save the position of a window  */
static void
persistence_save_window(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  PersistentWindow *window_pos = (PersistentWindow *)value;
  ObjectNode window;
  AttributeNode attr;

  window = (ObjectNode)xmlNewChild(tree, NULL, "window", NULL);

  data_add_string(new_attribute(window, "role"), key);
  data_add_int(new_attribute(window, "xpos"), window_pos->x);
  data_add_int(new_attribute(window, "ypos"), window_pos->y);
  data_add_int(new_attribute(window, "width"), window_pos->width);
  data_add_int(new_attribute(window, "height"), window_pos->height);
  data_add_boolean(new_attribute(window, "isopen"), window_pos->isopen);

}

/* Save all persistent data. */
void
persistence_save()
{
  xmlDocPtr doc;
  xmlNs *name_space;
  gchar *filename = dia_config_filename("persistence");

  doc = xmlNewDoc("1.0");
  doc->encoding = xmlStrdup("UTF-8");
  doc->xmlRootNode = xmlNewDocNode(doc, NULL, "persistence", NULL);

  name_space = xmlNewNs(doc->xmlRootNode, 
                        "http://www.lysator.liu.se/~alla/dia/",
			"dia");
  xmlSetNs(doc->xmlRootNode, name_space);

  /* Save any persistent window positions */
  if (persistent_windows != NULL &&
      g_hash_table_size(persistent_windows) != 0) {
    g_hash_table_foreach(persistent_windows, persistence_save_window, 
			 doc->xmlRootNode);
  }

  xmlDiaSaveFile(filename, doc);
  g_free(filename);
  xmlFreeDoc(doc);
}
