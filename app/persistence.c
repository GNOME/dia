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

#include "persistence.h"
#include "dia_dirs.h"
#include <dia_xml.h>

#include <gtk/gtk.h>
#include <libxml/tree.h>

/* Hash table from window role (string) to PersistentWindow structure.
 */
static GHashTable *persistent_windows;

/* Returns the name used for a window in persistence.
 */
gchar *
persistence_get_window_name(GtkWindow *window)
{
  gchar *name = gtk_window_get_role(window);
  if (name == NULL) {
    printf("Internal:  Window %s has no role.\n", gtk_window_get_title(window));
    return NULL;
  }
  return name;
}

/* Load all persistent data. */
void
persistence_load()
{
  printf("Loading persistence\n");
  
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
  printf("Registering %s\n", name);
  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);
  if (wininfo != NULL) {
    printf("Found old window at %d, %d\n", wininfo->x, wininfo->y);
    gtk_window_move(window, wininfo->x, wininfo->y);
    gtk_window_resize(window, wininfo->width, wininfo->y);
    if (wininfo->isopen) gtk_widget_show(window);
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    printf("New window at %d, %d\n", wininfo->x, wininfo->y);
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
}

void 
persistence_update_window(GtkWindow *window, GdkEvent *event, gpointer data)
{
  gchar *name = persistence_get_window_name(window);
  PersistentWindow *wininfo;

  if (name == NULL) return;
  printf("Updating %s\n", name);
  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);
  if (wininfo != NULL) {
    printf("Found old window at %d, %d\n", wininfo->x, wininfo->y);
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    gtk_window_get_size(window, &wininfo->width, &wininfo->height);
    /* Drawable means visible & mapped, what we usually think of as open. */
    wininfo->isopen = GTK_WIDGET_DRAWABLE(GTK_WIDGET(window));
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    printf("New window at %d, %d\n", wininfo->x, wininfo->y);
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
}

/* Save the position of a window  */
void
persistence_save_window(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  PersistentWindow *window_pos = (PersistentWindow *)value;
  ObjectNode window;
  AttributeNode attr;

  window = (ObjectNode)xmlNewChild(tree, NULL, "window", NULL);

  printf("Saving persistent window %s\n", key);
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

  printf("Saving persistence\n");
  if (!dia_config_ensure_dir(filename)) {
    printf("Failed to create persistence dir\n");
    return;
  }
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
