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

/* persistence.h -- definitions for persistent storage.
 */

#ifndef PERSISTENCE_H
#define PERSISTENCE_H
#include "config.h"

#include <gtk/gtk.h>

typedef struct {
  int x, y;
  int width, height;
  gboolean isopen;
  GtkWindow *window;
} PersistentWindow;

typedef void (NullaryFunc)();

void persistence_load();
void persistence_restore_window(GtkWindow *window);
void persistence_save();
void persistence_register_window(GtkWindow *window);
void persistence_register_window_create(gchar *role, NullaryFunc *func);
void persistence_register_string_entry(gchar *role, GtkWidget *entry);
gboolean persistence_change_string_entry(gchar *role, gchar *string,
					 GtkWidget *widget);

/** A persistently stored list of strings.
 * The list contains no duplicates.
 * If sorted is FALSE, any string added will be placed in front of the list
 * (possibly removing it from further down), thus making it an LRU list.
 * The list is not tied to any particular GTK widget, as it has uses
 * in a number of different places (though mostly in menus)
 */
typedef struct _PersistentList {
  const gchar *role;
  GList *glist;
  gboolean sorted;
  gint max_members;
} PersistentList;

PersistentList *persistence_register_list(const gchar *role);
PersistentList *persistent_list_get(const gchar *role);
GList *persistent_list_get_glist(const gchar *role);
void persistent_list_add(const gchar *role, const gchar *item);
void persistent_list_set_max_length(const gchar *role, gint max);
void persistent_list_remove(const gchar *role, const gchar *item);

#endif
