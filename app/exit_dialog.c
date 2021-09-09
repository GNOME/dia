/*
 * exit_dialog.h: Dialog to allow the user to choose which data to
 * save on exit or to cancel exit.
 *
 * Copyright (C) 2007 Patrick Hallinan
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

#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gi18n-lib.h>

#include "exit_dialog.h"


typedef struct _DiaExitDialogPrivate DiaExitDialogPrivate;
struct _DiaExitDialogPrivate {
  GtkWidget    *dialog;

  GtkWidget    *file_box;
  GtkWidget    *file_list;
  GtkListStore *file_store;
};


enum {
  COL_SAVE,
  COL_NAME,
  COL_PATH,
  COL_DIAGRAM,
  NUM_COL
};


G_DEFINE_TYPE_WITH_PRIVATE (DiaExitDialog, dia_exit_dialog, G_TYPE_OBJECT)


static void
dia_exit_dialog_finalize (GObject *object)
{
  DiaExitDialog *self = DIA_EXIT_DIALOG (object);
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);

  g_signal_handlers_disconnect_by_data (priv->dialog, self);
  gtk_widget_destroy (priv->dialog);

  g_clear_object (&priv->file_store);

  G_OBJECT_CLASS (dia_exit_dialog_parent_class)->finalize (object);
}


static void
dia_exit_dialog_class_init (DiaExitDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_exit_dialog_finalize;
}


static void
clear_item (gpointer data)
{
  DiaExitDialogItem *item = data;

  g_clear_pointer (&item->name, g_free);
  g_clear_pointer (&item->path, g_free);
  g_clear_object (&item->data);

  g_free (data);
}


/**
 * get_selected_items:
 * @self: the #DiaExitDialog
 *
 * Gets the list of items selected for saving by the user.
 *
 * Returns: The selected items.
 *
 * Since: 0.98
 */
static GPtrArray *
get_selected_items (DiaExitDialog *self)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);
  GtkTreeIter           iter;
  gboolean              valid;
  GPtrArray            *selected;

  selected = g_ptr_array_new_with_free_func (clear_item);

  /* Get the first iter in the list */
  valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (priv->file_store),
                                         &iter);

  /* Get the selected items */
  while (valid) {
    DiaExitDialogItem *item = g_new (DiaExitDialogItem, 1);
    gboolean           is_selected;

    gtk_tree_model_get (GTK_TREE_MODEL (priv->file_store), &iter,
                        COL_SAVE,    &is_selected,
                        COL_NAME,    &item->name,
                        COL_PATH,    &item->path,
                        COL_DIAGRAM, &item->data,
                        -1);

    if (is_selected) {
      g_ptr_array_add (selected, item);
    } else {
      clear_item (item);
    }

    valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (priv->file_store),
                                      &iter);
  }

  return selected;
}


static void
save_sensitive (DiaExitDialog *self)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);
  GPtrArray *selected = get_selected_items (self);

  gtk_dialog_set_response_sensitive (GTK_DIALOG (priv->dialog),
                                     DIA_EXIT_DIALOG_SAVE,
                                     selected->len > 0);

  g_clear_pointer (&selected, g_ptr_array_unref);
}


/*
 * Handler for the check box (button) in the exit dialogs treeview.
 * This is needed to cause the check box to change state when the
 * user clicks it
 */
static void
toggle_check_button (GtkCellRendererToggle *renderer,
                     char                  *path,
                     DiaExitDialog         *self)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);
  GtkTreeIter           iter;
  gboolean              value;

  if (gtk_tree_model_get_iter_from_string (GTK_TREE_MODEL (priv->file_store),
                                           &iter,
                                           path)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->file_store), &iter,
                        COL_SAVE, &value,
                        -1);
    gtk_list_store_set (priv->file_store, &iter, COL_SAVE, !value, -1);

    save_sensitive (self);
  }
}


static void
dia_exit_dialog_init (DiaExitDialog *self)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);

  GtkWidget *label;

  GtkWidget *scrolled;
  GtkWidget *button;

  GtkCellRenderer   *renderer;
  GtkTreeViewColumn *column;

  priv->dialog = gtk_message_dialog_new (NULL,
                                         GTK_DIALOG_MODAL,
                                         GTK_MESSAGE_QUESTION,
                                         GTK_BUTTONS_NONE,
                                         NULL);

  gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (priv->dialog),
                                            _("If you don’t save, all your changes will be permanently lost."));

  button = gtk_dialog_add_button (GTK_DIALOG (priv->dialog),
                                  _("Close _without Saving"),
                                  DIA_EXIT_DIALOG_QUIT);

  /* "use" it for gtk2 */
  gtk_widget_get_name (button);
  /*
  GTK3:
  gtk_style_context_add_class (gtk_widget_get_style_context (button),
                               "destructive-action");
  */

  gtk_dialog_add_button (GTK_DIALOG (priv->dialog),
                         _("_Cancel"),
                         DIA_EXIT_DIALOG_CANCEL);

  gtk_dialog_add_button (GTK_DIALOG (priv->dialog),
                         _("_Save"),
                         DIA_EXIT_DIALOG_SAVE);

  gtk_dialog_set_default_response (GTK_DIALOG (priv->dialog),
                                   DIA_EXIT_DIALOG_SAVE);


  priv->file_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (priv->dialog))),
                      priv->file_box,
                      FALSE,
                      FALSE,
                      0);
  gtk_widget_show (priv->file_box);


  /* Scrolled window for displaying things which need saving */
  scrolled = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled),
                                       GTK_SHADOW_IN);
  /* gtk_scrolled_window_set_min_content_height (GTK_SCROLLED_WINDOW (scrolled),
                                              90); */
  g_object_set (scrolled, "height-request", 90, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
                                  GTK_POLICY_NEVER,
                                  GTK_POLICY_AUTOMATIC);
  gtk_widget_show (scrolled);

  priv->file_store = gtk_list_store_new (NUM_COL,
                                         G_TYPE_BOOLEAN,
                                         G_TYPE_STRING,
                                         G_TYPE_STRING,
                                         DIA_TYPE_DIAGRAM);

  priv->file_list = gtk_tree_view_new_with_model (GTK_TREE_MODEL (priv->file_store));
  gtk_tree_view_set_tooltip_column (GTK_TREE_VIEW (priv->file_list),
                                    COL_PATH);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->file_list), FALSE);
  gtk_widget_show (priv->file_list);

  renderer = gtk_cell_renderer_toggle_new ();
  gtk_cell_renderer_toggle_set_activatable (GTK_CELL_RENDERER_TOGGLE (renderer),
                                            TRUE);
  column = gtk_tree_view_column_new_with_attributes (_("Save"), renderer,
                                                     "active", COL_SAVE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list), column);

  g_signal_connect (G_OBJECT (renderer), "toggled",
                    G_CALLBACK (toggle_check_button),
                    self);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
                                                     "text", COL_NAME,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (priv->file_list), column);


  gtk_container_add (GTK_CONTAINER (scrolled), priv->file_list);

  label = g_object_new (GTK_TYPE_LABEL,
                        "label", _("S_elect the diagrams you want to save:"),
                        "use-underline", TRUE,
                        "mnemonic-widget", priv->file_list,
                        "xalign", 0.0,
                        "visible", TRUE,
                        NULL);


  gtk_box_pack_start (GTK_BOX (priv->file_box), label, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (priv->file_box), scrolled, FALSE, FALSE, 0);

  g_signal_connect_swapped (G_OBJECT (priv->dialog), "destroy",
                            G_CALLBACK (g_object_unref),
                            self);
}


/**
 * dia_exit_dialog_new:
 * @parent: This is needed for modal behavior.
 *
 * A dialog to allow a user to select which unsaved files to save
 * (if any) or to abort exiting
 *
 * Since: 0.98
 */
DiaExitDialog *
dia_exit_dialog_new (GtkWindow *parent)
{
  DiaExitDialog *self = g_object_new (DIA_TYPE_EXIT_DIALOG, NULL);
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);

  gtk_window_set_transient_for (GTK_WINDOW (priv->dialog), parent);

  return self;
}


/**
 * dia_exit_dialog_add_item:
 * @self: the #DiaExitDialog
 * @name: User identifiable name of the thing which needs saving.
 * @filepath: File system path of the thing which needs saving.
 * @diagram: The unsaved #Diagram
 *
 * Add name and path of a file that needs to be saved
 *
 * Since: 0.98
 */
void
dia_exit_dialog_add_item (DiaExitDialog *self,
                          const char    *name,
                          const char    *filepath,
                          Diagram       *diagram)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);
  GtkTreeIter   iter;
  char *title = NULL;
  int n;

  gtk_list_store_append (priv->file_store, &iter);
  gtk_list_store_set (priv->file_store, &iter,
                      COL_SAVE, 1,
                      COL_NAME, name,
                      COL_PATH, filepath,
                      COL_DIAGRAM, diagram,
                      -1);

  save_sensitive (self);

  n = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (priv->file_store), NULL);

  if (n == 1) {
    title = g_markup_printf_escaped ("Save changes to diagram “%s” before closing?",
                                     name);
    gtk_widget_hide (priv->file_box);
  } else {
    title = g_markup_printf_escaped (ngettext ("There is %d diagram with unsaved changes. "
                                              "Save changes before closing?",
                                              "There are %d diagrams with unsaved changes. "
                                              "Save changes before closing?",
                                              n), n);
    gtk_widget_show (priv->file_box);
  }

  g_object_set (priv->dialog, "text", title, NULL);

  g_clear_pointer (&title, g_free);
}


DiaExitDialogResult
dia_exit_dialog_run (DiaExitDialog  *self,
                     GPtrArray     **items)
{
  DiaExitDialogPrivate *priv = dia_exit_dialog_get_instance_private (self);
  int                   result;

  result = gtk_dialog_run (GTK_DIALOG (priv->dialog));

  *items = NULL;

  if (result == DIA_EXIT_DIALOG_SAVE) {
    *items = get_selected_items (self);
  } else if (result != DIA_EXIT_DIALOG_QUIT &&
             result != DIA_EXIT_DIALOG_CANCEL) {
    result = DIA_EXIT_DIALOG_CANCEL;
  }

  return result;
}
