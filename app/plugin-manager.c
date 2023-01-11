/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * plugin-manager.c: the dia plugin manager.
 * Copyright (C) 2000 James Henstridge
 * almost complete rewrite for gtk2
 * Copyright (C) 2002 Hans Breuer
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#include <gtk/gtk.h>

#include "plugin-manager.h"
#include "plug-ins.h"
#include "message.h"


static int
pm_respond (GtkWidget *widget, int response_id, gpointer data)
{
  if (response_id != GTK_RESPONSE_APPLY)
    gtk_widget_hide (widget);
  return 0;
}

enum
{
  LOADED_COLUMN,
  NAME_COLUMN,
  DESC_COLUMN,
  FILENAME_COLUMN,
  AUTOLOAD_COLUMN,
  PLUGIN_COLUMN,
  NUM_COLUMNS
};

static void
toggle_loaded_callback (GtkCellRendererToggle *celltoggle,
                        gchar                 *path_string,
                        GtkTreeView           *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean loaded;
  PluginInfo *info;

  path = gtk_tree_path_new_from_string (path_string);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      LOADED_COLUMN, &loaded, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      PLUGIN_COLUMN, &info, -1);

  if (loaded && dia_plugin_can_unload(info))
    {
      dia_plugin_unload(info);
      loaded = FALSE;
    }
  else if (!loaded)
    {
      dia_plugin_load(info);
      loaded = TRUE;
    }
  else
    message_notice("Can't unload plugin '%s'!", dia_plugin_get_name(info));
  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                      LOADED_COLUMN, loaded, -1);
}

static void
can_unload (GtkTreeViewColumn *tree_column,
	    GtkCellRenderer   *cell,
	    GtkTreeModel      *tree_model,
	    GtkTreeIter       *iter,
	    gpointer           data)
{
  PluginInfo *info;
  gboolean loaded;

  gtk_tree_model_get(tree_model, iter,
                     PLUGIN_COLUMN, &info, -1);
  gtk_tree_model_get(tree_model, iter,
                     LOADED_COLUMN, &loaded, -1);
  if (!loaded || (loaded && dia_plugin_can_unload(info)))
    {
      g_object_set (cell,
		    "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
		    "activatable", TRUE,
		    NULL);
    }
  else
    {
      g_object_set (cell,
		    "mode", GTK_CELL_RENDERER_MODE_INERT,
		    "activatable", FALSE,
		    NULL);
    }
}

static void
toggle_autoload_callback (GtkCellRendererToggle *celltoggle,
                          gchar                 *path_string,
                          GtkTreeView           *tree_view)
{
  GtkTreeModel *model = gtk_tree_view_get_model (tree_view);
  GtkTreePath *path;
  GtkTreeIter iter;
  gboolean load;
  PluginInfo *info;

  path = gtk_tree_path_new_from_string (path_string);
  if (!gtk_tree_model_get_iter (model, &iter, path))
    {
      g_warning ("%s: bad path?", G_STRLOC);
      return;
    }
  gtk_tree_path_free (path);

  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      AUTOLOAD_COLUMN, &load, -1);
  gtk_tree_model_get (GTK_TREE_MODEL (model), &iter,
                      PLUGIN_COLUMN, &info, -1);

  /* Disabling 'Standard' is fatal at next startup, while
   * disabling 'Internal' is simply impossible
   */
  if (   0 == strcmp(dia_plugin_get_name(info), "Standard")
      || 0 == strcmp(dia_plugin_get_name(info), "Internal"))
    message_notice("You don't want to inhibit loading\n"
                   "of plugin '%s'!", dia_plugin_get_name(info));
  else {
    dia_plugin_set_inhibit_load(info, load);
    gtk_list_store_set (GTK_LIST_STORE (model), &iter,
                        AUTOLOAD_COLUMN, !load, -1);
  }
}

static void
can_inhibit (GtkTreeViewColumn *tree_column,
	     GtkCellRenderer   *cell,
	     GtkTreeModel      *tree_model,
	     GtkTreeIter       *iter,
	     gpointer           data)
{
  PluginInfo *info;

  gtk_tree_model_get(tree_model, iter,
                     PLUGIN_COLUMN, &info, -1);
  if (   0 == strcmp(dia_plugin_get_name(info), "Standard")
      || 0 == strcmp(dia_plugin_get_name(info), "Internal"))
    {
      g_object_set (cell,
		    "mode", GTK_CELL_RENDERER_MODE_INERT,
		    "activatable", FALSE,
		    NULL);
    }
  else
    {
      g_object_set (cell,
		    "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE,
		    "activatable", TRUE,
		    NULL);
    }
}

static GtkWidget *
get_plugin_manager(void)
{
  static GtkWidget *dialog = NULL;
  GtkWidget *vbox, *scrolled_window, *tree_view;
  GtkListStore *store;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeIter iter;
  GList *tmp;

  if (dialog)
    return dialog;

  /* build up the user interface */
  dialog = gtk_dialog_new_with_buttons (_("Plugins"),
                                        NULL, 0,
                                        _("_Close"), GTK_RESPONSE_CLOSE,
                                        NULL);

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_CLOSE);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

  /* don't destroy dialog when window manager close button pressed */
  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(pm_respond), NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_true), NULL);
  g_signal_connect(G_OBJECT(dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);

  scrolled_window = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_window, TRUE, TRUE, 0);

  /* create the TreeStore */
  store = gtk_list_store_new (NUM_COLUMNS,
                              G_TYPE_BOOLEAN,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_POINTER);
  tree_view = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);

  /* fill list */
  for (tmp = dia_list_plugins(); tmp != NULL; tmp = tmp->next) {
    PluginInfo *info = tmp->data;

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
                        LOADED_COLUMN, dia_plugin_is_loaded(info),
			NAME_COLUMN, dia_plugin_get_name(info),
			DESC_COLUMN, dia_plugin_get_description(info),
			FILENAME_COLUMN, dia_plugin_get_filename(info),
                        AUTOLOAD_COLUMN, !dia_plugin_get_inhibit_load(info),
                        PLUGIN_COLUMN, info,
			-1);
  }

  /* setup renderers and view */
  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect(G_OBJECT (renderer), "toggled",
		   G_CALLBACK (toggle_loaded_callback), tree_view);
  col = gtk_tree_view_column_new_with_attributes(
		   _("Loaded"), renderer,
		   "active", LOADED_COLUMN, NULL);
  gtk_tree_view_column_set_cell_data_func (
  		   col, renderer, can_unload, NULL, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);

  col = gtk_tree_view_column_new_with_attributes(
		   _("Name"), gtk_cell_renderer_text_new (),
		   "text", NAME_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (col, NAME_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);

  col = gtk_tree_view_column_new_with_attributes(
		   _("Description"), gtk_cell_renderer_text_new (),
		   "text", DESC_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (col, DESC_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);

  renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect(G_OBJECT (renderer), "toggled",
		   G_CALLBACK (toggle_autoload_callback), tree_view);
  col = gtk_tree_view_column_new_with_attributes(
	      _("Load at Startup"), renderer,
	      "active", AUTOLOAD_COLUMN, NULL);
  gtk_tree_view_column_set_cell_data_func (
  		   col, renderer, can_inhibit, NULL, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);

  col = gtk_tree_view_column_new_with_attributes(
		   _("Filename"), gtk_cell_renderer_text_new (),
		   "text", FILENAME_COLUMN, NULL);
  gtk_tree_view_column_set_sort_column_id (col, FILENAME_COLUMN);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);

  gtk_container_add (GTK_CONTAINER (scrolled_window), tree_view);
  gtk_window_set_default_size (GTK_WINDOW (dialog), 480, 400);
  gtk_widget_show_all (dialog);

  return dialog;
}

void
file_plugins_callback(GtkAction *action)
{
  GtkWidget *pm = get_plugin_manager();

  gtk_widget_show(pm);
}
