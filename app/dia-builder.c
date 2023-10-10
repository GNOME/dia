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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#include "dia-builder.h"
#include "dia_dirs.h"


typedef struct _DiaBuilderPrivate DiaBuilderPrivate;
struct _DiaBuilderPrivate {
  GHashTable *callbacks;

  gboolean already_connected;
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaBuilder, dia_builder, GTK_TYPE_BUILDER)


/**
 * SECTION:dia-builder
 * @title: DiaBuilder
 * @short_description: Enhanced GtkBuilder
 *
 * DiaBuilder extends #GtkBuilder with utilities for faking templates in gtk2
 */


static void
dia_builder_finalize (GObject *object)
{
  DiaBuilder *self = DIA_BUILDER (object);
  DiaBuilderPrivate *priv = dia_builder_get_instance_private (self);

  g_clear_pointer (&priv->callbacks, g_hash_table_destroy);

  G_OBJECT_CLASS (dia_builder_parent_class)->finalize (object);
}


static void
dia_builder_class_init (DiaBuilderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_builder_finalize;
}


static void
dia_builder_init (DiaBuilder *self)
{

}


/**
 * dia_builder_new:
 * @path: path to load, relative to the datadir
 *
 * Returns: The new #DiaBuilder
 *
 * Since: 0.98
 */
DiaBuilder *
dia_builder_new (const char *path)
{
  GError *error = NULL;
  char *uifile;
  DiaBuilder *builder;

  builder = g_object_new (DIA_TYPE_BUILDER, NULL);
  uifile = dia_builder_path (path);
  if (!gtk_builder_add_from_file (GTK_BUILDER (builder), uifile, &error)) {
    g_warning ("Couldn't load builder file: %s", error->message);
  }

  g_clear_error (&error);
  g_clear_pointer (&uifile, g_free);

  return builder;
}


/**
 * dia_builder_get:
 * @self: the #DiaBuilder
 * @first_object_name: the name of the first object to get
 * @...: where to put the first object, followed optionally by more
 * name/target pairs, followed by %NULL
 *
 * Get various objects from @self
 *
 * |[<!-- language="C" -->
 * GtkWidget *widget;
 * GtkWidget *other_widget;
 *
 * dia_builder_get (builder,
 *                  "widget", &widget,
 *                  "other-widget", &other_widget,
 *                  NULL);
 * ]|
 *
 * Since: 0.98
 */
void
dia_builder_get (DiaBuilder *self,
                 const char *first_object_name,
                 ...)
{
  va_list var_args;
  const char *object_name;
  GObject **object_target;

  g_return_if_fail (DIA_IS_BUILDER (self));
  g_return_if_fail (first_object_name && first_object_name[0]);

  object_name = first_object_name;

  va_start (var_args, first_object_name);

  object_target = va_arg (var_args, GObject **);

  do {
    *object_target = gtk_builder_get_object (GTK_BUILDER (self),
                                             object_name);

    object_name = va_arg (var_args, const char *);

    if (object_name) {
      object_target = va_arg (var_args, GObject **);
    }
  } while (object_name != NULL);

  va_end (var_args);
}


/**
 * dia_builder_connect:
 * @self: the #DiaBuilder
 * @data: the user_data passed to handlers
 * @first_signal_name: the name of the first signal handler
 * @first_signal_symbol: the first signal handler
 * @...: additional signal/handler pairs, followed by %NULL
 *
 * Bind handlers to callbacks defined in the source file
 *
 * Once dia_builder_connect_signals() has been called (including by this
 * method) you cannot call it again on @self
 *
 * |[<!-- language="C" -->
 * dia_builder_connect (builder,
 *                      "widget_changed", G_CALLBACK (widget_changed),
 *                      "other_widget_clicked", G_CALLBACK (other_widget_clicked),
 *                      NULL);
 * ]|
 *
 * Since: 0.98
 */
void
dia_builder_connect (DiaBuilder *self,
                     gpointer    data,
                     const char *first_signal_name,
                     GCallback   first_signal_symbol,
                     ...)
{
  va_list var_args;
  const char *signal_name;
  GCallback signal_symbol;

  g_return_if_fail (DIA_IS_BUILDER (self));
  g_return_if_fail (first_signal_name && first_signal_name[0]);
  g_return_if_fail (first_signal_symbol != NULL);

  signal_name = first_signal_name;
  signal_symbol = first_signal_symbol;

  va_start (var_args, first_signal_symbol);

  do {
    dia_builder_add_callback_symbol (self,
                                     signal_name,
                                     signal_symbol);

    signal_name = va_arg (var_args, const char *);

    if (signal_name) {
      signal_symbol = va_arg (var_args, GCallback);
    }

  } while (signal_name != NULL);

  va_end (var_args);

  dia_builder_connect_signals (self, data);
}


/**
 * dia_builder_add_callback_symbol:
 * @self: the #DiaBuilder
 * @callback_name: the name of the signal handler
 * @callback_symbol: the signal handler
 *
 * Bind a handlers to a callback defined in the source file
 *
 * Since: 0.98
 */
void
dia_builder_add_callback_symbol (DiaBuilder *self,
                                 const char *callback_name,
                                 GCallback   callback_symbol)
{
  DiaBuilderPrivate *priv;

  g_return_if_fail (DIA_IS_BUILDER (self));
  g_return_if_fail (callback_name && callback_name[0]);
  g_return_if_fail (callback_symbol != NULL);

  priv = dia_builder_get_instance_private (self);

  if (!priv->callbacks) {
    priv->callbacks = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                      g_free, NULL);
  }

  g_hash_table_insert (priv->callbacks,
                       g_strdup (callback_name),
                       callback_symbol);
}


/**
 * dia_builder_lookup_callback_symbol:
 * @self: the #DiaBuilder
 * @callback_name: the name of the signal handler
 *
 * Get the #GCallback associated with @callback_name
 *
 * Returns: The #GCallback previously set for @callback_name, or %NULL
 *
 * Since: 0.98
 */
GCallback
dia_builder_lookup_callback_symbol (DiaBuilder *self,
                                    const char *callback_name)
{
  DiaBuilderPrivate *priv;

  g_return_val_if_fail (DIA_IS_BUILDER (self), NULL);
  g_return_val_if_fail (callback_name && callback_name[0], NULL);

  priv = dia_builder_get_instance_private (self);

  if (!priv->callbacks)
    return NULL;

  return g_hash_table_lookup (priv->callbacks, callback_name);
}


static void
connect_signals (GtkBuilder    *builder,
                 GObject       *object,
                 const char    *signal_name,
                 const char    *handler_name,
                 GObject       *connect_object,
                 GConnectFlags  flags,
                 gpointer       data)
{
  GCallback func;

  func = dia_builder_lookup_callback_symbol (DIA_BUILDER (builder),
                                             handler_name);

  if (!func) {
    g_warning ("Could not find signal handler “%s”", handler_name);
    return;
  }

  if (connect_object) {
    g_signal_connect_object (object, signal_name, func, connect_object, flags);
  } else {
    g_signal_connect_data (object, signal_name, func, data, NULL, flags);
  }
}


/**
 * dia_builder_connect_signals:
 * @self: the #DiaBuilder
 * @user_data: the user_data to pass to handlers
 *
 * Connect all the handlers previously set with
 * dia_builder_lookup_callback_symbol() to their various objects
 *
 * This can only be called once on @self ( dia_builder_connect() uses this
 * method internally)
 *
 * Since: 0.98
 */
void
dia_builder_connect_signals (DiaBuilder *self,
                             gpointer    user_data)
{
  DiaBuilderPrivate *priv;

  g_return_if_fail (DIA_IS_BUILDER (self));

  priv = dia_builder_get_instance_private (self);

  g_return_if_fail (!priv->already_connected);

  gtk_builder_connect_signals_full (GTK_BUILDER (self),
                                    connect_signals,
                                    user_data);

  priv->already_connected = TRUE;
}


/**
 * dia_builder_path:
 * @name: the filename to get a path for
 *
 * Get the full path to @name located in the datadir
 *
 * Note: dia_builder_new will call this for you
 *
 * Returns: the newly allocated path
 *
 * Since: 0.98
 */
char *
dia_builder_path (const char* name)
{
  char* uifile;

  if (g_getenv ("DIA_BASE_PATH") != NULL) {
    uifile = g_build_filename (g_getenv ("DIA_BASE_PATH"), "data", name, NULL);
  } else {
    uifile = dia_get_data_directory (name);
  }

  return uifile;
}
