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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "persistence.h"
#include "dia_dirs.h"
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include <gtk/gtk.h>
#include <libxml/tree.h>

/* Hash table from window role (string) to PersistentWindow structure.
 */
static GHashTable *persistent_windows, *persistent_entrystrings, *persistent_lists;
static GHashTable *persistent_integers, *persistent_reals;
static GHashTable *persistent_booleans, *persistent_strings;
static GHashTable *persistent_colors;

/* *********************** GENERAL INTERNAL FUNCTIONS ************** */
/** Lookup an entry in any of the type tables, ensuring existence on the way.
 * @param type_table A pointer to one of the above type tables.
 * @param role The role (name) within the table.
 * @return The value of the role in the table, or NULL if either role is
 * NULL or the table does not contain the value.
 */
static gchar *
persistence_lookup(GHashTable **type_table, gchar *role)
{
  if (role == NULL) {
    return NULL;
  }
  if (*type_table == NULL) {
    *type_table = g_hash_table_new(g_str_hash, g_str_equal);
  }
  return g_hash_table_lookup(*type_table, role);
}


/* *********************** LOADING FUNCTIONS *********************** */

typedef void (*PersistenceLoadFunc)(gchar *role, xmlNodePtr node);

static void
persistence_load_window(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  PersistentWindow *wininfo = g_new0(PersistentWindow, 1);

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

  g_hash_table_insert(persistent_windows, role, wininfo);
}

/** Load a persistent string into the strings hashtable */
static void
persistence_load_entrystring(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  gchar *string = NULL;

  /* Find the contents? */
  attr = composite_find_attribute(node, "stringvalue");
  if (attr != NULL)
    string = data_string(attribute_first_data(attr));
  else 
    return;

  if (string != NULL)
    g_hash_table_insert(persistent_entrystrings, role, string);
}

static void
persistence_load_list(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  gchar *string = NULL;

  /* Find the contents? */
  attr = composite_find_attribute(node, "listvalue");
  if (attr != NULL)
    string = data_string(attribute_first_data(attr));
  else 
    return;

  if (string != NULL) {
    gchar **strings = g_strsplit(string, "\n", -1);
    PersistentList *plist;
    GList *list = NULL;
    int i;
    for (i = 0; strings[i] != NULL; i++) {
      list = g_list_append(list, g_strdup(strings[i]));
    }
    /* This frees the strings, too? */
    g_strfreev(strings);
    /* yes but not the other one --hb */
    g_free (string);
    plist = g_new(PersistentList, 1);
    plist->glist = list;
    plist->role = role;
    plist->sorted = FALSE;
    plist->max_members = G_MAXINT;
    g_hash_table_insert(persistent_lists, role, plist);
  }
}

static void
persistence_load_integer(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  gint *integer;

  /* Find the contents? */
  attr = composite_find_attribute(node, "intvalue");
  if (attr != NULL) {
    integer = g_new(gint, 1);
    *integer = data_int(attribute_first_data(attr));
  } else 
    return;

  if (g_hash_table_lookup(persistent_integers, role) == NULL) 
    g_hash_table_insert(persistent_integers, role, integer);
  else 
    printf("Int %s registered before loading persistence!\n", role);
}

static void
persistence_load_real(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  real *realval;

  /* Find the contents? */
  attr = composite_find_attribute(node, "realvalue");
  if (attr != NULL) {
    realval = g_new(real, 1);
    *realval = data_real(attribute_first_data(attr));
  } else 
    return;

  if (g_hash_table_lookup(persistent_reals, role) == NULL) 
    g_hash_table_insert(persistent_reals, role, realval);
  else 
    printf("Real %s registered before loading persistence!\n", role);
}

static void
persistence_load_boolean(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  gboolean *booleanval;

  /* Find the contents? */
  attr = composite_find_attribute(node, "booleanvalue");
  if (attr != NULL) {
    booleanval = g_new(gboolean, 1);
    *booleanval = data_boolean(attribute_first_data(attr));
  } else 
    return;

  if (g_hash_table_lookup(persistent_booleans, role) == NULL) 
    g_hash_table_insert(persistent_booleans, role, booleanval);
  else 
    printf("Boolean %s registered before loading persistence!\n", role);
}

static void
persistence_load_string(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  gchar *stringval;

  /* Find the contents? */
  attr = composite_find_attribute(node, "stringvalue");
  if (attr != NULL) {
    stringval = data_string(attribute_first_data(attr));
  } else 
    return;

  if (g_hash_table_lookup(persistent_strings, role) == NULL) 
    g_hash_table_insert(persistent_strings, role, stringval);
  else 
    printf("String %s registered before loading persistence!\n", role);
}

static void
persistence_load_color(gchar *role, xmlNodePtr node)
{
  AttributeNode attr;
  Color *colorval;

  /* Find the contents? */
  attr = composite_find_attribute(node, "colorvalue");
  if (attr != NULL) {
    colorval = g_new(Color, 1);
    data_color(attribute_first_data(attr), colorval);
  } else 
    return;

  if (g_hash_table_lookup(persistent_colors, role) == NULL) 
    g_hash_table_insert(persistent_colors, role, colorval);
  else 
    printf("Color %s registered before loading persistence!\n", role);
}

static xmlNodePtr
find_node_named (xmlNodePtr p, const char *name)
{
  while (p && 0 != strcmp((char *)p->name, name))
    p = p->next;
  return p;
}

static GHashTable *type_handlers;

/** Load the named type of entries using the given function.
 * func is a void (*func)(gchar *role, xmlNodePtr *node)
 */
static void
persistence_load_type(xmlNodePtr node)
{
  const gchar *typename = (gchar *) node->name;
  gchar *name;

  PersistenceLoadFunc func =
    (PersistenceLoadFunc)g_hash_table_lookup(type_handlers, typename);
  if (func == NULL) {
    return;
  }

  name = (gchar *)xmlGetProp(node, (const xmlChar *)"role");
  if (name == NULL) {
    return;
  }
  
  (*func)(name, node);
  node = node->next;
}

static void
persistence_set_type_handler(gchar *name, PersistenceLoadFunc func)
{
  if (type_handlers == NULL)
    type_handlers = g_hash_table_new(g_str_hash, g_str_equal);

  g_hash_table_insert(type_handlers, name, (gpointer)func);
}

static void
persistence_init()
{
  persistence_set_type_handler("window", persistence_load_window);
  persistence_set_type_handler("entrystring", persistence_load_entrystring);
  persistence_set_type_handler("list", persistence_load_list);
  persistence_set_type_handler("integer", persistence_load_integer);
  persistence_set_type_handler("real", persistence_load_real);
  persistence_set_type_handler("boolean", persistence_load_boolean);
  persistence_set_type_handler("string", persistence_load_string);
  persistence_set_type_handler("color", persistence_load_color);

  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_entrystrings == NULL) {
    persistent_entrystrings = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_lists == NULL) {
    persistent_lists = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_integers == NULL) {
    persistent_integers = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_reals == NULL) {
    persistent_reals = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_booleans == NULL) {
    persistent_booleans = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_strings == NULL) {
    persistent_strings = g_hash_table_new(g_str_hash, g_str_equal);
  }
  if (persistent_colors == NULL) {
    persistent_colors = g_hash_table_new(g_str_hash, g_str_equal);
  }
}

/* Load all persistent data. */
void
persistence_load()
{
  xmlDocPtr doc;
  gchar *filename = dia_config_filename("persistence");

  persistence_init();

  if (!g_file_test(filename, G_FILE_TEST_IS_REGULAR)) {
    g_free (filename);
    return;
  }
  doc = xmlDiaParseFile(filename);
  if (doc != NULL) {
    if (doc->xmlRootNode != NULL) {
      xmlNsPtr namespace = xmlSearchNs(doc, doc->xmlRootNode, (const xmlChar *)"dia");
      if (!xmlStrcmp (doc->xmlRootNode->name, (const xmlChar *)"persistence") &&
	  namespace != NULL) {
	xmlNodePtr child_node = doc->xmlRootNode->children;
	for (; child_node != NULL; child_node = child_node->next) {
	  persistence_load_type(child_node);
	}
      }
    }
    xmlFreeDoc(doc);
  }
  g_free(filename);
}

/* *********************** SAVING FUNCTIONS *********************** */

/* Save the position of a window  */
static void
persistence_save_window(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  PersistentWindow *window_pos = (PersistentWindow *)value;
  ObjectNode window;

  window = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"window", NULL);
  
  xmlSetProp(window, (const xmlChar *)"role", (xmlChar *) key);
  data_add_int(new_attribute(window, "xpos"), window_pos->x);
  data_add_int(new_attribute(window, "ypos"), window_pos->y);
  data_add_int(new_attribute(window, "width"), window_pos->width);
  data_add_int(new_attribute(window, "height"), window_pos->height);
  data_add_boolean(new_attribute(window, "isopen"), window_pos->isopen);

}

/* Save the contents of a string  */
static void
persistence_save_list(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode listnode;
  GString *buf;
  GList *items;

  listnode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"list", NULL);

  xmlSetProp(listnode, (const xmlChar *)"role", (xmlChar *) key);
  /* Make a string out of the list */
  buf = g_string_new("");
  for (items = ((PersistentList*)value)->glist; items != NULL;
       items = g_list_next(items)) {
    g_string_append(buf, (gchar *)items->data);
    if (g_list_next(items) != NULL) g_string_append(buf, "\n");
  }
  
  data_add_string(new_attribute(listnode, "listvalue"), buf->str);
  g_string_free(buf, TRUE);
}

static void
persistence_save_entrystring(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode stringnode;

  stringnode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"entrystring", NULL);

  xmlSetProp(stringnode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_string(new_attribute(stringnode, "stringvalue"), (char *)value);
}

static void
persistence_save_integer(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode integernode;

  integernode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"integer", NULL);

  xmlSetProp(integernode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_int(new_attribute(integernode, "intvalue"), *(gint *)value);
}

static void
persistence_save_real(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode realnode;

  realnode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"real", NULL);

  xmlSetProp(realnode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_real(new_attribute(realnode, "realvalue"), *(real *)value);
}

static void
persistence_save_boolean(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode booleannode;

  booleannode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"boolean", NULL);

  xmlSetProp(booleannode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_boolean(new_attribute(booleannode, "booleanvalue"), *(gboolean *)value);
}

static void
persistence_save_string(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode stringnode;

  stringnode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"string", NULL);

  xmlSetProp(stringnode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_string(new_attribute(stringnode, "stringvalue"), (gchar *)value);
}

static void
persistence_save_color(gpointer key, gpointer value, gpointer data)
{  
  xmlNodePtr tree = (xmlNodePtr)data;
  ObjectNode colornode;

  colornode = (ObjectNode)xmlNewChild(tree, NULL, (const xmlChar *)"color", NULL);

  xmlSetProp(colornode, (const xmlChar *)"role", (xmlChar *)key);
  data_add_color(new_attribute(colornode, "colorvalue"), (Color *)value);
}


static void
persistence_save_type(xmlDocPtr doc, GHashTable *entries, GHFunc func)
{
  if (entries != NULL && g_hash_table_size(entries) != 0) {
    g_hash_table_foreach(entries, func, doc->xmlRootNode);
  }
}

/* Save all persistent data. */
void
persistence_save()
{
  xmlDocPtr doc;
  xmlNs *name_space;
  gchar *filename = dia_config_filename("persistence");

  doc = xmlNewDoc((const xmlChar *)"1.0");
  doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  doc->xmlRootNode = xmlNewDocNode(doc, NULL, (const xmlChar *)"persistence", NULL);

  name_space = xmlNewNs(doc->xmlRootNode, 
                        (const xmlChar *) DIA_XML_NAME_SPACE_BASE,
			(const xmlChar *)"dia");
  xmlSetNs(doc->xmlRootNode, name_space);

  persistence_save_type(doc, persistent_windows, persistence_save_window);
  persistence_save_type(doc, persistent_entrystrings, persistence_save_string);
  persistence_save_type(doc, persistent_lists, persistence_save_list);
  persistence_save_type(doc, persistent_integers, persistence_save_integer);
  persistence_save_type(doc, persistent_reals, persistence_save_real);
  persistence_save_type(doc, persistent_booleans, persistence_save_boolean);
  persistence_save_type(doc, persistent_strings, persistence_save_string);
  persistence_save_type(doc, persistent_colors, persistence_save_color);

  xmlDiaSaveFile(filename, doc);
  g_free(filename);
  xmlFreeDoc(doc);
}

/* *********************** USAGE FUNCTIONS *********************** */

/* ********* WINDOWS ********* */

/* Returns the name used for a window in persistence.
 */
static const gchar *
persistence_get_window_name(GtkWindow *window)
{
  const gchar *name = gtk_window_get_role(window);
  if (name == NULL) {
    printf("Internal:  Window %s has no role.\n", gtk_window_get_title(window));
    return NULL;
  }
  return name;
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

/** Update the persistent information for a window.
 * @param window The GTK window object being stored.
 * @param isclosed Whether the window should be stored as closed or not.
 * In some cases, the window's open/close state is not updated by the time
 * the handler is called.
 */
static void
persistence_update_window(GtkWindow *window, gboolean isclosed)
{
  const gchar *name = persistence_get_window_name(window);
  PersistentWindow *wininfo;

  if (name == NULL) return;

  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);

  if (wininfo != NULL) {  
    persistence_store_window_info(window, wininfo, isclosed || !wininfo->isopen);
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    persistence_store_window_info(window, wininfo, FALSE);
    g_hash_table_insert(persistent_windows, (gchar *)name, wininfo);
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

/** Handler for window-related events that should cause persistent storage
 * changes.
 * @param window The GTK window to store for.
 * @param event the GDK event that caused us to be called.  Note that the 
 * window state hasn't been updated by the event yet.
 * @param data Userdata passed when adding signal handler.
 */
static gboolean
persistence_window_event_handler(GtkWindow *window, GdkEvent *event, gpointer data)
{
#if 0
  switch (event->type) {
  case GDK_UNMAP : printf ("unmap (%s)\n", persistence_get_window_name(window)); break;
  case GDK_MAP : printf ("map (%s)\n", persistence_get_window_name(window)); break;
  case GDK_CONFIGURE : printf ("configure (%s)\n", persistence_get_window_name(window)); break;
  }
#endif
  persistence_update_window(window, (event->type == GDK_UNMAP));
  return FALSE;
}

/** 
 * Handler for when a window has been opened or closed.
 * @param window The GTK window to store for.
 * @param data Userdata passed when adding signal handler.
 */
static gboolean
persistence_hide_show_window(GtkWindow *window, gpointer data)
{
  persistence_update_window(window, !GTK_WIDGET_VISIBLE(window));
  return FALSE;
}

/**
 * If the screen size has changed some persistent info maybe out of the visible area.
 * This function checks that stored coordinates are at least paritally visible on some
 * monitor. In GDK parlance a screen can have multiple monitors.
 */
static gboolean
wininfo_in_range (const PersistentWindow *wininfo)
{
  GdkScreen *screen = gdk_screen_get_default ();
  gint num_monitors = gdk_screen_get_n_monitors (screen), i;
  GdkRectangle rwin = {wininfo->x, wininfo->y, wininfo->width, wininfo->height};
  GdkRectangle rres = {0, 0, 0, 0};
  
  for (i = 0; i < num_monitors; ++i) {
    GdkRectangle rmon;
    
    gdk_screen_get_monitor_geometry (screen, i, &rmon);
    
    gdk_rectangle_intersect (&rwin, &rmon, &rres);
    if (rres.width * rres.height > 0)
      break;
  }

  return (rres.width * rres.height > 0);
}

/* Call this function after the window has a role assigned to use any
 * persistence information about the window.
 */
void
persistence_register_window(GtkWindow *window)
{
  const gchar *name = persistence_get_window_name(window);
  PersistentWindow *wininfo;

  if (name == NULL) return;
  if (persistent_windows == NULL) {
    persistent_windows = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  wininfo = (PersistentWindow *)g_hash_table_lookup(persistent_windows, name);
  if (wininfo != NULL) {
    if (wininfo_in_range (wininfo)) {
      /* only restore position if partially visible */
      gtk_window_move(window, wininfo->x, wininfo->y);
      gtk_window_resize(window, wininfo->width, wininfo->height);
    }
    if (wininfo->isopen) gtk_widget_show(GTK_WIDGET(window));
  } else {
    wininfo = g_new0(PersistentWindow, 1);
    gtk_window_get_position(window, &wininfo->x, &wininfo->y);
    gtk_window_get_size(window, &wininfo->width, &wininfo->height);
    /* Drawable means visible & mapped, what we usually think of as open. */
    wininfo->isopen = GTK_WIDGET_DRAWABLE(GTK_WIDGET(window));
    g_hash_table_insert(persistent_windows, (gchar *)name, wininfo);
  }
  if (wininfo->window != NULL && wininfo->window != window) {
    g_object_unref(wininfo->window);
    wininfo->window = NULL;
  }
  if (wininfo->window == NULL) {
    wininfo->window = window;
    g_object_ref(window);
  }

  g_signal_connect(GTK_OBJECT(window), "configure-event",
		   G_CALLBACK(persistence_window_event_handler), NULL);
  g_signal_connect(GTK_OBJECT(window), "map-event",
		   G_CALLBACK(persistence_window_event_handler), NULL);
  g_signal_connect(GTK_OBJECT(window), "unmap-event",
		   G_CALLBACK(persistence_window_event_handler), NULL);

  g_signal_connect(GTK_OBJECT(window), "hide",
		   G_CALLBACK(persistence_hide_show_window), NULL);
  g_signal_connect(GTK_OBJECT(window), "show",
		   G_CALLBACK(persistence_hide_show_window), NULL);
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


/* ********* STRING ENTRIES ********** */

static gboolean
persistence_update_string_entry(GtkWidget *widget, GdkEvent *event,
				gpointer userdata)
{
  gchar *role = (gchar*)userdata;

  if (event->type == GDK_FOCUS_CHANGE) {
    gchar *string = (gchar *)g_hash_table_lookup(persistent_entrystrings, role);
    const gchar *entrystring = gtk_entry_get_text(GTK_ENTRY(widget));
    if (string == NULL || strcmp(string, entrystring)) {
      g_hash_table_insert(persistent_entrystrings, role, g_strdup(entrystring));
      if (string != NULL) g_free(string);
    }
  }

  return FALSE;
}

/** Change the contents of the persistently stored string entry.
 * If widget is non-null, it is updated to reflect the change.
 * This can be used e.g. for when a dialog is cancelled and the old
 * contents should be restored.
 */
gboolean
persistence_change_string_entry(gchar *role, gchar *string,
				GtkWidget *widget)
{
  gchar *old_string = (gchar*)g_hash_table_lookup(persistent_entrystrings, role);
  if (old_string != NULL) {
    if (widget != NULL) {
      gtk_entry_set_text(GTK_ENTRY(widget), string);
    }
    g_hash_table_insert(persistent_entrystrings, role, g_strdup(string));
    g_free(old_string);
  }

  return FALSE;
}

/** Register a string in a GtkEntry for persistence.
 * This should include not only a unique name, but some way to update
 * whereever the string is used.
 */
void
persistence_register_string_entry(gchar *role, GtkWidget *entry)
{
  gchar *string;
  if (role == NULL) return;
  if (persistent_entrystrings == NULL) {
    persistent_entrystrings = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  string = (gchar *)g_hash_table_lookup(persistent_entrystrings, role);
  if (string != NULL) {
    gtk_entry_set_text(GTK_ENTRY(entry), string);
  } else {
    string = g_strdup(gtk_entry_get_text(GTK_ENTRY(entry)));
    g_hash_table_insert(persistent_entrystrings, role, string);
  }
  g_signal_connect(G_OBJECT(entry), "event", 
		   G_CALLBACK(persistence_update_string_entry), role);
}

/* ********* LISTS ********** */

/* Lists are used for e.g. recent files, selected fonts, etc. 
 * Anywhere where the user occasionally picks from a long list and
 * is likely to reuse the items.
 */

PersistentList *
persistence_register_list(const gchar *role)
{
  PersistentList *list;
  if (role == NULL) return NULL;
  if (persistent_lists == NULL) {
    persistent_lists = g_hash_table_new(g_str_hash, g_str_equal);
  } else {   
    list = (PersistentList *)g_hash_table_lookup(persistent_lists, role);
    if (list != NULL) {
      return list;
    }
  }
  list = g_new(PersistentList, 1);
  list->role = role;
  list->glist = NULL;
  list->sorted = FALSE;
  list->max_members = G_MAXINT;
  g_hash_table_insert(persistent_lists, (gchar *)role, list);
  return list;
}

PersistentList *
persistent_list_get(const gchar *role)
{
  PersistentList *list;
  if (role == NULL) return NULL;
  if (persistent_lists != NULL) {
    list = (PersistentList *)g_hash_table_lookup(persistent_lists, role);
    if (list != NULL) {
      return list;
    }
  }
  /* Not registered! */
  return NULL;
}

GList *
persistent_list_get_glist(const gchar *role)
{
  PersistentList *plist = persistent_list_get(role);
  if (plist == NULL) return NULL;
  return plist->glist;
}

static GList *
persistent_list_cut_length(GList *list, guint length)
{
  while (g_list_length(list) > length) {
    GList *last = g_list_last(list);
    /* Leaking data?  See not in persistent_list_add */
    list = g_list_remove_link(list, last);
    g_list_free(last);
  }
  return list;
}

/** Add a new entry to this persistent list.
 * @param role The name of a persistent list.
 * @param item An entry to add.
 * @returns FALSE if the entry already existed in the list, TRUE otherwise.
 */
gboolean
persistent_list_add(const gchar *role, const gchar *item)
{
  PersistentList *plist = persistent_list_get(role);
  if(plist == NULL) {
    printf("Can't find list for %s when adding %s\n", 
	   role, item);
    return TRUE;
  }
  if (plist->sorted) {
    /* Sorting not implemented yet. */
    return TRUE;
  } else {
    gboolean existed = FALSE;
    GList *tmplist = plist->glist;
    GList *old_elem = g_list_find_custom(tmplist, item, (GCompareFunc)g_ascii_strcasecmp);
    while (old_elem != NULL) {
      tmplist = g_list_remove_link(tmplist, old_elem);
      /* Don't free this, as it makes recent_files go boom after
       * selecting a file there several times.  Yes, it should be strdup'd,
       * but it isn't.
       */
      /*g_free(old_elem->data);*/
      g_list_free_1(old_elem);
      old_elem = g_list_find_custom(tmplist, item, (GCompareFunc)g_ascii_strcasecmp);
      existed = TRUE;
    }
    tmplist = g_list_prepend(tmplist, g_strdup(item));
    tmplist = persistent_list_cut_length(tmplist, plist->max_members);
    plist->glist = tmplist;
    return existed;
  }
}

void
persistent_list_set_max_length(const gchar *role, gint max)
{
  PersistentList *plist = persistent_list_get(role);
  plist->max_members = max;
  plist->glist = persistent_list_cut_length(plist->glist, max);
}

/** Remove an item from the persistent list.
 * @param role The name of the persistent list.
 * @param role The entry to remove.
 * @returns TRUE if the item existed in the list, FALSE otherwise.
 */
gboolean
persistent_list_remove(const gchar *role, const gchar *item)
{
  PersistentList *plist = persistent_list_get(role);
  /* Leaking data?  See not in persistent_list_add */
  GList *entry = g_list_find_custom(plist->glist, item, (GCompareFunc)g_ascii_strcasecmp);
  if (entry != NULL) {
    plist->glist = g_list_remove_link(plist->glist, entry);
    g_free(entry->data);
    return TRUE;
  } else return FALSE;
}

void
persistent_list_remove_all(const gchar *role)
{
  PersistentList *plist = persistent_list_get(role);
  persistent_list_cut_length(plist->glist, 0);
  plist->glist = NULL;
}

typedef struct {
  PersistenceCallback func;
  GObject *watch;
  gpointer userdata;
} ListenerData;

/** Add a listener to updates on the list, so that if another
 * instance changes the list, menus and such can be updated.
 * @param role The name of the persistent list to watch.
 * @param func A function to call when the list is updated, takes
 * the given userdata.
 * @param userdata Data passed back into the callback function.
 */
void
persistent_list_add_listener(const gchar *role, PersistenceCallback func, 
			     GObject *watch, gpointer userdata)
{
  PersistentList *plist = persistent_list_get(role);
  ListenerData *listener;

  if (plist != NULL) {
    listener = g_new(ListenerData, 1);
    listener->func = func;
    listener->watch = watch;
    g_object_add_weak_pointer(watch, (gpointer)&listener->watch);
    listener->userdata = userdata;
    plist->listeners = g_list_append(plist->listeners, listener);
  }
}

/** When changing the list, call the listeners.
 */
static void
persistent_list_invoke_listeners(gchar *role)
{
  GList *tmp;
  PersistentList *plist = persistent_list_get(role);
  if (plist != NULL) {
    for (tmp = plist->listeners; tmp != NULL; tmp = g_list_next(tmp)) {
      ListenerData *listener = (ListenerData*)tmp->data;
      if (listener->watch == NULL) {
	/* Listener died */
	plist->listeners = g_list_remove_link(plist->listeners, tmp->data);
	g_free(listener);
      } else {
	/* Still listening */
	(listener->func)(listener->watch, listener->userdata);
      }
    }
  }
}

/* ********* INTEGERS ********** */
gint
persistence_register_integer(gchar *role, int defaultvalue)
{
  gint *integer;
  if (role == NULL) return 0;
  if (persistent_integers == NULL) {
    persistent_integers = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  integer = (gint *)g_hash_table_lookup(persistent_integers, role);
  if (integer == NULL) {
    integer = g_new(gint, 1);
    *integer = defaultvalue;
    g_hash_table_insert(persistent_integers, role, integer);
  }
  return *integer;
}

gint
persistence_get_integer(gchar *role)
{
  gint *integer;
  if (persistent_integers == NULL) {
    printf("No persistent integers to get for %s!\n", role);
    return 0;
  }
  integer = (gint *)g_hash_table_lookup(persistent_integers, role);
  if (integer != NULL) return *integer;
  printf("No integer to get for %s\n", role);
  return 0;
}

void
persistence_set_integer(gchar *role, gint newvalue)
{
  gint *integer;
  if (persistent_integers == NULL) {
    printf("No persistent integers yet for %s!\n", role);
    return;
  }
  integer = (gint *)g_hash_table_lookup(persistent_integers, role);
  if (integer != NULL) *integer = newvalue;
  else printf("No integer to set for %s\n", role);
}

/* ********* REALS ********** */
real
persistence_register_real(gchar *role, real defaultvalue)
{
  real *realval;
  if (role == NULL) return 0;
  if (persistent_reals == NULL) {
    persistent_reals = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  realval = (real *)g_hash_table_lookup(persistent_reals, role);
  if (realval == NULL) {
    realval = g_new(real, 1);
    *realval = defaultvalue;
    g_hash_table_insert(persistent_reals, role, realval);
  }
  return *realval;
}

real
persistence_get_real(gchar *role)
{
  real *realval;
  if (persistent_reals == NULL) {
    printf("No persistent reals to get for %s!\n", role);
    return 0;
  }
  realval = (real *)g_hash_table_lookup(persistent_reals, role);
  if (realval != NULL) return *realval;
  printf("No real to get for %s\n", role);
  return 0.0;
}

void
persistence_set_real(gchar *role, real newvalue)
{
  real *realval;
  if (persistent_reals == NULL) {
    printf("No persistent reals yet for %s!\n", role);
    return;
  }
  realval = (real *)g_hash_table_lookup(persistent_reals, role);
  if (realval != NULL) *realval = newvalue;
  else printf("No real to set for %s\n", role);
}


/* ********* BOOLEANS ********** */
/** Returns true if the given role has been registered. */
gboolean
persistence_boolean_is_registered(gchar *role)
{
  gboolean *booleanval;
  if (role == NULL) return 0;
  if (persistent_booleans == NULL) {
    persistent_booleans = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  booleanval = (gboolean *)g_hash_table_lookup(persistent_booleans, role);
  return booleanval != NULL;
}

gboolean
persistence_register_boolean(gchar *role, gboolean defaultvalue)
{
  gboolean *booleanval;
  if (role == NULL) return 0;
  if (persistent_booleans == NULL) {
    persistent_booleans = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  booleanval = (gboolean *)g_hash_table_lookup(persistent_booleans, role);
  if (booleanval == NULL) {
    booleanval = g_new(gboolean, 1);
    *booleanval = defaultvalue;
    g_hash_table_insert(persistent_booleans, role, booleanval);
  }
  return *booleanval;
}

gboolean
persistence_get_boolean(gchar *role)
{
  gboolean *booleanval;
  if (persistent_booleans == NULL) {
    printf("No persistent booleans to get for %s!\n", role);
    return FALSE;
  }
  booleanval = (gboolean *)g_hash_table_lookup(persistent_booleans, role);
  if (booleanval != NULL) return *booleanval;
  printf("No boolean to get for %s\n", role);
  return FALSE;
}

void
persistence_set_boolean(gchar *role, gboolean newvalue)
{
  gboolean *booleanval;
  if (persistent_booleans == NULL) {
    printf("No persistent booleans yet for %s!\n", role);
    return;
  }
  booleanval = (gboolean *)g_hash_table_lookup(persistent_booleans, role);
  if (booleanval != NULL) *booleanval = newvalue;
  else printf("No boolean to set for %s\n", role);
}

/* ********* STRINGS ********** */
/** Register a string in persistence.
 * @param role The name used to refer to the string.  Must be unique within
 *             registered strings (and preferably with all registered items)
 * @param defaultvalue A value to use if the role does not exist yet.
 * @returns The value that role has after registering.  The caller is
 *          responsible for freeing this memory.  It will never be the same
 *          memory as defaultvalue.
 */
gchar *
persistence_register_string(gchar *role, gchar *defaultvalue)
{
  gchar *stringval;
  if (role == NULL) return 0;
  if (persistent_strings == NULL) {
    persistent_strings = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  stringval = (gchar *)g_hash_table_lookup(persistent_strings, role);
  if (stringval == NULL) {
    stringval = g_strdup(defaultvalue);
    g_hash_table_insert(persistent_strings, role, stringval);
  }
  return g_strdup(stringval);
}

gchar *
persistence_get_string(gchar *role)
{
  gchar *stringval;
  if (persistent_strings == NULL) {
    printf("No persistent strings to get for %s!\n", role);
    return NULL;
  }
  stringval = (gchar *)g_hash_table_lookup(persistent_strings, role);
  if (stringval != NULL) return g_strdup(stringval);
  printf("No string to get for %s\n", role);
  return NULL;
}

void
persistence_set_string(gchar *role, const gchar *newvalue)
{
  gchar *stringval;
  if (persistent_strings == NULL) {
    printf("No persistent strings yet for %s!\n", role);
    return;
  }
  stringval = (gchar *)g_hash_table_lookup(persistent_strings, role);
  if (stringval != NULL) {
    g_hash_table_insert(persistent_strings, role, g_strdup(newvalue));
    g_free(stringval);
  }
  else printf("No string to set for %s\n", role);
}

/* ********* COLORS ********** */
/* Remember that colors returned are private, not to be deallocated.
 * They will be smashed in some undefined way by persistence_set_color */
Color *
persistence_register_color(gchar *role, Color *defaultvalue)
{
  Color *colorval;
  if (role == NULL) return 0;
  if (persistent_colors == NULL) {
    persistent_colors = g_hash_table_new(g_str_hash, g_str_equal);
  }    
  colorval = (Color *)g_hash_table_lookup(persistent_colors, role);
  if (colorval == NULL) {
    colorval = g_new(Color, 1);
    *colorval = *defaultvalue;
    g_hash_table_insert(persistent_colors, role, colorval);
  }
  return colorval;
}

Color *
persistence_get_color(gchar *role)
{
  Color *colorval;
  if (persistent_colors == NULL) {
    printf("No persistent colors to get for %s!\n", role);
    return 0;
  }
  colorval = (Color *)g_hash_table_lookup(persistent_colors, role);
  if (colorval != NULL) return colorval;
  printf("No color to get for %s\n", role);
  return 0;
}

void
persistence_set_color(gchar *role, Color *newvalue)
{
  Color *colorval;
  if (persistent_colors == NULL) {
    printf("No persistent colors yet for %s!\n", role);
    return;
  }
  colorval = (Color *)g_hash_table_lookup(persistent_colors, role);
  if (colorval != NULL) *colorval = *newvalue;
  else printf("No color to set for %s\n", role);
}
