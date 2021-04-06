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

#pragma once

#include "geometry.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef void (NullaryFunc) (void);

void     persistence_load                   (void);
void     persistence_save                   (void);
void     persistence_register_window        (GtkWindow   *window);
void     persistence_register_window_create (char        *role,
                                             NullaryFunc *func);
void     persistence_register_string_entry  (char        *role,
                                             GtkWidget   *entry);
gboolean persistence_change_string_entry    (char        *role,
                                             char        *string,
                                             GtkWidget   *widget);


typedef struct _PersistentList PersistentList;

typedef void (*PersistenceCallback) (GObject *, gpointer);

PersistentList *persistence_register_list      (const char          *role);
PersistentList *persistent_list_get            (const char          *role);
GList          *persistent_list_get_glist      (const char          *role);
gboolean        persistent_list_add            (const char          *role,
                                                const char          *item);
void            persistent_list_set_max_length (const char          *role,
                                                int                  max);
gboolean        persistent_list_remove         (const char          *role,
                                                const char          *item);
void            persistent_list_remove_all     (const char          *role);
void            persistent_list_add_listener   (const char          *role,
                                                PersistenceCallback  func,
                                                GObject             *watch,
                                                gpointer             userdata);
void            persistent_list_clear          (const char          *role);
int             persistence_register_integer   (char                *role,
                                                int                  defaultvalue);
int             persistence_get_integer        (char                *role);
void            persistence_set_integer        (char                *role,
                                                int                  newvalue);
double          persistence_register_real      (char                *role,
                                                double               defaultvalue);
double          persistence_get_real           (char                *role);
void            persistence_set_real           (char                *role,
                                                double               newvalue);
gboolean        persistence_register_boolean   (const char          *role,
                                                gboolean             defaultvalue);
gboolean        persistence_get_boolean        (const char          *role);
void            persistence_set_boolean        (const char          *role,
                                                gboolean             newvalue);
char           *persistence_register_string    (char                *role,
                                                const char          *defaultvalue);
char           *persistence_get_string         (char                *role);
void            persistence_set_string         (char                *role,
                                                const char          *newvalue);
Color          *persistence_register_color     (char                *role,
                                                Color               *defaultvalue);
Color          *persistence_get_color          (char                *role);
void            persistence_set_color          (char                *role,
                                                Color               *newvalue);

G_END_DECLS
