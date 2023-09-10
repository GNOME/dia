/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * © 2023 Hubert Figuière <hub@figuiere.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "class.h"
#include "diaoptionmenu.h"

#include "class_dialog.h"

/*************************************************************
 ******************** OPERATIONS *****************************
 *************************************************************/

enum {
  COL_PARAM_TITLE,
  COL_PARAM_PARAM,
  N_PARAM_COLS
};


enum {
  COL_OPER_TITLE,
  COL_OPER_OPER,
  COL_OPER_UNDERLINE,
  COL_OPER_BOLD,
  COL_OPER_ITALIC,
  N_OPER_COLS
};



static gboolean
get_current_operation (UMLClassDialog  *dialog,
                       UMLOperation   **oper,
                       GtkTreeIter     *c_iter)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->operations));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_OPER_OPER, oper, -1);

    if (c_iter) {
      *c_iter = iter;
    }

    return TRUE;
  }

  return FALSE;
}


static void
update_operation (UMLClassDialog *dialog,
                  UMLOperation   *op,
                  GtkTreeIter    *iter)
{
  PangoUnderline underline;
  PangoStyle italic = PANGO_STYLE_NORMAL;
  int weight = 400;
  char *title;

  underline = op->class_scope ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE;
  if (op->inheritance_type != DIA_UML_LEAF) {
    italic = PANGO_STYLE_ITALIC;
    if (op->inheritance_type == DIA_UML_ABSTRACT) {
      weight = 800;
    }
  }
  title = uml_get_operation_string (op);

  gtk_list_store_set (dialog->operations_store,
                      iter,
                      COL_OPER_OPER, op,
                      COL_OPER_TITLE, title,
                      COL_OPER_UNDERLINE, underline,
                      COL_OPER_BOLD, weight,
                      COL_OPER_ITALIC, italic,
                      -1);

  g_clear_pointer (&title, g_free);
}


static gboolean
get_current_parameter (UMLClassDialog  *dialog,
                       UMLParameter   **param,
                       GtkTreeIter     *c_iter)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, param, -1);

    if (c_iter) {
      *c_iter = iter;
    }

    return TRUE;
  }

  return FALSE;
}


static void
update_parameter (UMLClassDialog *dialog,
                  UMLParameter   *param,
                  GtkTreeIter    *iter)
{
  UMLOperation *op;
  GtkTreeIter op_iter;
  char *title;

  title = uml_parameter_get_string (param);

  gtk_list_store_set (dialog->parameters_store,
                      iter,
                      COL_PARAM_PARAM, param,
                      COL_PARAM_TITLE, title,
                      -1);

  if (get_current_operation (dialog, &op, &op_iter)) {
    update_operation (dialog, op, &op_iter);

    g_clear_pointer (&op, uml_operation_unref);
  }

  g_clear_pointer (&title, g_free);
}


static void
parameters_set_sensitive (UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->param_name), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->param_type), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->param_value), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->param_comment), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->param_kind), val);
}


static void
parameters_set_values (UMLClassDialog *prop_dialog,
                       UMLParameter   *param)
{
  char *comment;

  gtk_entry_set_text (prop_dialog->param_name, param->name? param->name : "");
  gtk_entry_set_text (prop_dialog->param_type, param->type? param->type : "");
  gtk_entry_set_text (prop_dialog->param_value, param->value? param->value : "");
  comment = g_strdup (param->comment? param->comment : "");
  gtk_text_buffer_set_text (prop_dialog->param_comment_buffer,
                            comment,
                            -1);
  g_clear_pointer (&comment, g_free);

  dia_option_menu_set_active (DIA_OPTION_MENU (prop_dialog->param_kind),
                              param->kind);
}


static void
parameters_clear_values (UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text (prop_dialog->param_name, "");
  gtk_entry_set_text (prop_dialog->param_type, "");
  gtk_entry_set_text (prop_dialog->param_value, "");
  gtk_text_buffer_set_text (prop_dialog->param_comment_buffer, "", -1);
  dia_option_menu_set_active (DIA_OPTION_MENU (prop_dialog->param_kind),
                              DIA_UML_UNDEF_KIND);
}


static gboolean
add_param_to_list (GtkTreeModel *model,
                   GtkTreePath *path,
                   GtkTreeIter *iter,
                   gpointer data)
{
  UMLOperation *op = data;
  UMLParameter *param = NULL;

  // Don't free param, transfering to the list
  gtk_tree_model_get (model, iter, COL_PARAM_PARAM, &param, -1);

  op->parameters = g_list_append (op->parameters, uml_parameter_ref (param));

  return FALSE;
}


static void
sync_params_to_operation (GtkTreeModel *model,
                          UMLOperation *op)
{
  g_list_free_full (op->parameters, (GDestroyNotify) uml_parameter_unref);
  op->parameters = NULL;

  gtk_tree_model_foreach (model, add_param_to_list, op);
}


static void
parameters_list_new_callback (GtkWidget *button,
                              UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  prop_dialog = umlclass->properties_dialog;

  if (!get_current_operation (prop_dialog, &current_op, NULL)) {
    return;
  }

  param = uml_parameter_new ();

  gtk_list_store_append (prop_dialog->parameters_store, &iter);
  update_parameter (prop_dialog, param, &iter);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  gtk_tree_selection_select_iter (selection, &iter);

  sync_params_to_operation (GTK_TREE_MODEL (prop_dialog->parameters_store),
                            current_op);

  g_clear_pointer (&param, uml_parameter_unref);
  g_clear_pointer (&current_op, uml_operation_unref);
}


static void
parameters_list_delete_callback (GtkWidget *button,
                                 UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    /* Remove from current operations parameter list: */
    if (!get_current_operation (prop_dialog, &current_op, NULL)) {
      return;
    }

    gtk_list_store_remove (prop_dialog->parameters_store, &iter);

    sync_params_to_operation (GTK_TREE_MODEL (prop_dialog->parameters_store),
                              current_op);

    g_clear_pointer (&param, uml_parameter_unref);
    g_clear_pointer (&current_op, uml_operation_unref);
  }
}


static void
parameters_list_move_up_callback (GtkWidget *button,
                                  UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->parameters_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;
    GtkTreeIter op_iter;

    if (path != NULL && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->parameters_store), &prev, path)) {
      gtk_list_store_move_before (prop_dialog->parameters_store, &iter, &prev);
    } else {
      gtk_list_store_move_before (prop_dialog->parameters_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
    gtk_tree_selection_select_iter (selection, &iter);

    /* Move parameter in current operations list: */
    if (get_current_operation (prop_dialog, &current_op, &op_iter)) {
      sync_params_to_operation (GTK_TREE_MODEL (prop_dialog->parameters_store),
                                current_op);

      update_operation (prop_dialog, current_op, &op_iter);

      g_clear_pointer (&current_op, uml_operation_unref);
    }

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
parameters_list_move_down_callback (GtkWidget *button,
                                    UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->parameters_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;
    GtkTreeIter op_iter;

    if (path != NULL) {
      gtk_tree_path_next (path);
      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->parameters_store), &prev, path)) {
        gtk_list_store_move_after (prop_dialog->parameters_store, &iter, &prev);
      } else {
        gtk_list_store_move_after (prop_dialog->parameters_store, &iter, NULL);
      }
    } else {
      gtk_list_store_move_after (prop_dialog->parameters_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
    gtk_tree_selection_select_iter (selection, &iter);

    /* Move parameter in current operations list: */
    if (get_current_operation (prop_dialog, &current_op, &op_iter)) {
      sync_params_to_operation (GTK_TREE_MODEL (prop_dialog->parameters_store),
                                current_op);

      update_operation (prop_dialog, current_op, &op_iter);

      g_clear_pointer (&current_op, uml_operation_unref);
    }

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
operations_set_sensitive (UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_name), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_type), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_stereotype), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_comment), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_visible), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_class_scope), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_inheritance_type), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->op_query), val);

  gtk_widget_set_sensitive (prop_dialog->param_new_button, val);
  gtk_widget_set_sensitive (prop_dialog->param_delete_button, val);
  gtk_widget_set_sensitive (prop_dialog->param_down_button, val);
  gtk_widget_set_sensitive (prop_dialog->param_up_button, val);

  if (!val) {
    parameters_set_sensitive (prop_dialog, FALSE);
  }
}


static void
operations_set_values (UMLClassDialog *prop_dialog, UMLOperation *op)
{
  GList *list;
  UMLParameter *param;
  GtkTreeIter iter;
  gchar *str;
  char *comment;

  gtk_entry_set_text (prop_dialog->op_name, op->name? op->name : "");
  gtk_entry_set_text (prop_dialog->op_type, op->type? op->type : "");
  gtk_entry_set_text (prop_dialog->op_stereotype, op->stereotype? op->stereotype : "");
  comment = g_strdup (op->comment? op->comment : "");
  gtk_text_buffer_set_text (prop_dialog->op_comment_buffer,
                            comment,
                            -1);
  g_clear_pointer (&comment, g_free);

  dia_option_menu_set_active (DIA_OPTION_MENU (prop_dialog->op_visible),
                              op->visibility);
  gtk_toggle_button_set_active (prop_dialog->op_class_scope, op->class_scope);
  gtk_toggle_button_set_active (prop_dialog->op_query, op->query);
  dia_option_menu_set_active (DIA_OPTION_MENU (prop_dialog->op_inheritance_type),
                              op->inheritance_type);

  gtk_list_store_clear (prop_dialog->parameters_store);
  parameters_set_sensitive (prop_dialog, FALSE);

  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;

    str = uml_parameter_get_string (param);

    gtk_list_store_append (prop_dialog->parameters_store, &iter);
    update_parameter (prop_dialog, param, &iter);

    g_clear_pointer (&str, g_free);

    list = g_list_next (list);
  }
}


static void
operations_clear_values (UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text (prop_dialog->op_name, "");
  gtk_entry_set_text (prop_dialog->op_type, "");
  gtk_entry_set_text (prop_dialog->op_stereotype, "");
  gtk_text_buffer_set_text (prop_dialog->op_comment_buffer, "", -1);
  gtk_toggle_button_set_active (prop_dialog->op_class_scope, FALSE);
  gtk_toggle_button_set_active (prop_dialog->op_query, FALSE);

  gtk_list_store_clear (prop_dialog->parameters_store);

  parameters_set_sensitive (prop_dialog, FALSE);
}


static void
operations_list_new_callback (GtkWidget *button,
                              UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  prop_dialog = umlclass->properties_dialog;

  op = uml_operation_new ();

  /* need to make new ConnectionPoints valid and remember them */
  uml_operation_ensure_connection_points (op, &umlclass->element.object);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->left_connection);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->right_connection);

  gtk_list_store_append (prop_dialog->operations_store, &iter);
  update_operation (prop_dialog, op, &iter);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->operations));
  gtk_tree_selection_select_iter (selection, &iter);

  g_clear_pointer (&op, uml_operation_unref);
}


static void
operations_list_delete_callback (GtkWidget *button,
                                 UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    if (op->left_connection != NULL) {
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        op->left_connection);
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        op->right_connection);
    }

    gtk_list_store_remove (prop_dialog->operations_store, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
operations_list_move_up_callback (GtkWidget *button,
                                  UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &current_op, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->operations_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->operations_store), &prev, path)) {
      gtk_list_store_move_before (prop_dialog->operations_store, &iter, &prev);
    } else {
      gtk_list_store_move_before (prop_dialog->operations_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->operations));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_op, uml_operation_unref);
  }
}


static void
operations_list_move_down_callback(GtkWidget *button,
                                   UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &current_op, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->operations_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL) {
      gtk_tree_path_next (path);
      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->operations_store), &prev, path)) {
        gtk_list_store_move_after (prop_dialog->operations_store, &iter, &prev);
      } else {
        gtk_list_store_move_after (prop_dialog->operations_store, &iter, NULL);
      }
    } else {
      gtk_list_store_move_after (prop_dialog->operations_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->operations));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_op, uml_operation_unref);
  }
}


struct AddOperData {
  UMLClass       *class;
  UMLClassDialog *dialog;
  int             connection_index;
};


static gboolean
add_oper_to_list (GtkTreeModel *model,
                  GtkTreePath *path,
                  GtkTreeIter *iter,
                  gpointer udata)
{
  struct AddOperData *data = udata;
  UMLOperation *op = NULL;

  // Don't free op, transfering to the list
  gtk_tree_model_get (model, iter, COL_OPER_OPER, &op, -1);

  data->class->operations = g_list_append (data->class->operations, op);

  if (op->left_connection == NULL) {
    uml_operation_ensure_connection_points (op, DIA_OBJECT (data->class));

    data->dialog->added_connections =
      g_list_prepend (data->dialog->added_connections,
                      op->left_connection);
    data->dialog->added_connections =
      g_list_prepend (data->dialog->added_connections,
                      op->right_connection);
  }

  if (gtk_toggle_button_get_active (data->dialog->op_vis) &&
      (!gtk_toggle_button_get_active (data->dialog->op_supp))) {
    DIA_OBJECT (data->class)->connections[data->connection_index] = op->left_connection;
    data->connection_index++;
    DIA_OBJECT (data->class)->connections[data->connection_index] = op->right_connection;
    data->connection_index++;
  } else {
    _umlclass_store_disconnects (data->dialog, op->left_connection);
    object_remove_connections_to (op->left_connection);
    _umlclass_store_disconnects (data->dialog, op->right_connection);
    object_remove_connections_to (op->right_connection);
  }

  return FALSE;
}


void
_operations_read_from_dialog (UMLClass       *umlclass,
                              UMLClassDialog *prop_dialog,
                              int             connection_index)
{
  struct AddOperData data;

  /* Free current operations: */
  g_list_free_full (umlclass->operations, (GDestroyNotify) uml_operation_unref);
  umlclass->operations = NULL;

  data.class = umlclass;
  data.dialog = prop_dialog;
  data.connection_index = connection_index;

  gtk_tree_model_foreach (GTK_TREE_MODEL (prop_dialog->operations_store),
                          add_oper_to_list,
                          &data);
  gtk_list_store_clear (prop_dialog->operations_store);
  operations_set_sensitive (prop_dialog, FALSE);
}


void
_operations_fill_in_dialog (UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op_copy;
  GtkTreeIter iter;
  GList *list;

  prop_dialog = umlclass->properties_dialog;

  gtk_list_store_clear (prop_dialog->operations_store);

  list = umlclass->operations;
  while (list != NULL) {
    UMLOperation *op = (UMLOperation *) list->data;

    op_copy = uml_operation_copy (op);

    /* Looks wrong but is required for the complicate connections memory management */
    op_copy->left_connection = op->left_connection;
    op_copy->right_connection = op->right_connection;

    gtk_list_store_append (prop_dialog->operations_store, &iter);
    update_operation (prop_dialog, op_copy, &iter);

    list = g_list_next (list);

    g_clear_pointer (&op_copy, uml_operation_unref);
  }

  operations_set_sensitive (prop_dialog, FALSE);
}


static void
oper_name_changed (GtkEntry *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    g_clear_pointer (&op->name, g_free);
    op->name = g_strdup (gtk_entry_get_text (entry));

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_type_changed (GtkEntry *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    g_clear_pointer (&op->type, g_free);
    op->type = g_strdup (gtk_entry_get_text (entry));

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_stereotype_changed (GtkEntry *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    g_clear_pointer (&op->stereotype, g_free);
    op->stereotype = g_strdup (gtk_entry_get_text (entry));

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_visible_changed (DiaOptionMenu *selector, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    op->visibility = (DiaUmlVisibility) dia_option_menu_get_active (selector);

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_inheritance_changed (DiaOptionMenu *selector, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    op->inheritance_type = (DiaUmlInheritanceType) dia_option_menu_get_active (selector);

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_comment_changed (GtkTextBuffer *buffer, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, NULL)) {
    g_clear_pointer (&op->comment, g_free);
    op->comment = buffer_get_text (buffer);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_scope_changed (GtkToggleButton *toggle, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    op->class_scope = gtk_toggle_button_get_active (toggle);

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static void
oper_query_changed (GtkToggleButton *toggle, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_operation (prop_dialog, &op, &iter)) {
    op->query = gtk_toggle_button_get_active (toggle);

    update_operation (prop_dialog, op, &iter);

    g_clear_pointer (&op, uml_operation_unref);
  }
}


static GtkWidget*
operations_data_create_hbox (UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *omenu;
  GtkWidget *scrolledwindow;
  GtkWidget *checkbox;

  prop_dialog = umlclass->properties_dialog;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  /* grid containing operation 'name' up to 'query' and also the comment */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 0);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->op_name = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (oper_name_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand (entry, TRUE);

  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->op_type = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (oper_type_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand (entry, TRUE);

  label = gtk_label_new (_("Stereotype:"));
  entry = gtk_entry_new ();
  prop_dialog->op_stereotype = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (oper_stereotype_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  gtk_widget_set_hexpand (entry, TRUE);


  label = gtk_label_new(_("Visibility:"));

  prop_dialog->op_visible = omenu = dia_option_menu_new ();
  g_signal_connect (omenu,
                    "changed",
                    G_CALLBACK (oper_visible_changed),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Public"), DIA_UML_PUBLIC);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Private"), DIA_UML_PRIVATE);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Protected"), DIA_UML_PROTECTED);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Implementation"), DIA_UML_IMPLEMENTATION);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
					/* left, right, top, bottom */
  gtk_grid_attach (GTK_GRID (grid), label, 2, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), omenu, 3, 0, 1, 1);
  gtk_widget_set_hexpand (omenu, TRUE);
  /* end: Visibility */

  label = gtk_label_new(_("Inheritance type:"));

  prop_dialog->op_inheritance_type = omenu = dia_option_menu_new ();
  g_signal_connect (omenu,
                    "changed",
                    G_CALLBACK (oper_inheritance_changed),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Abstract"), DIA_UML_ABSTRACT);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Polymorphic (virtual)"), DIA_UML_POLYMORPHIC);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Leaf (final)"), DIA_UML_LEAF);

  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), omenu, 3, 1, 1, 1);
  gtk_widget_set_hexpand (omenu, TRUE);
  /* end: Inheritance type */

  checkbox = gtk_check_button_new_with_label (_("Class scope"));
  g_signal_connect (checkbox,
                    "toggled",
                    G_CALLBACK (oper_scope_changed),
                    umlclass);
  prop_dialog->op_class_scope = GTK_TOGGLE_BUTTON (checkbox);

  gtk_grid_attach (GTK_GRID (grid), checkbox, 2, 2, 1, 1);

  checkbox = gtk_check_button_new_with_label (_("Query"));
  g_signal_connect (checkbox,
                    "toggled",
                    G_CALLBACK (oper_query_changed),
                    umlclass);
  prop_dialog->op_query = GTK_TOGGLE_BUTTON (checkbox);
  gtk_grid_attach (GTK_GRID (grid), checkbox, 3, 2, 1, 1);

  label = gtk_label_new (_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
  /* with GTK_POLICY_NEVER the comment filed gets smaller unti l text is entered; than it would resize the dialog! */
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  prop_dialog->op_comment_buffer = gtk_text_buffer_new (NULL);
  entry = gtk_text_view_new_with_buffer (prop_dialog->op_comment_buffer);
  prop_dialog->op_comment = GTK_TEXT_VIEW (entry);
  g_signal_connect (prop_dialog->op_comment_buffer,
                    "changed",
                    G_CALLBACK (oper_comment_changed),
                    umlclass);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (entry),TRUE);


  gtk_grid_attach (GTK_GRID (grid), label, 4, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 4, 1, 1, 2);
  gtk_widget_set_hexpand (scrolledwindow, TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), vbox2, TRUE, TRUE, 0);

  return hbox;
}


static void
parameter_selected (GtkTreeSelection *selection,
                    UMLClass         *umlclass)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  UMLParameter *param;
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog) {
    return; /* maybe hiding a bug elsewhere */
  }

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    parameters_set_values (prop_dialog, param);
    parameters_set_sensitive (prop_dialog, TRUE);

    g_clear_pointer (&param, uml_parameter_unref);

    gtk_widget_grab_focus (GTK_WIDGET (prop_dialog->param_name));
  } else {
    parameters_set_sensitive (prop_dialog, FALSE);
    parameters_clear_values (prop_dialog);
  }
}


static GtkWidget*
operations_parameters_editor_create_vbox (UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *vbox3;
  GtkWidget *label;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  prop_dialog = umlclass->properties_dialog;

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  /* Parameters list label */
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  label = gtk_label_new (_("Parameters:"));
  gtk_box_pack_start (GTK_BOX (hbox2), label, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  /* Parameters list editor - with of list at least width of buttons*/
  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox2), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  prop_dialog->parameters_store = gtk_list_store_new (N_PARAM_COLS,
                                                      G_TYPE_STRING,
                                                      DIA_UML_TYPE_PARAMETER);
  prop_dialog->parameters = gtk_tree_view_new_with_model (GTK_TREE_MODEL (prop_dialog->parameters_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prop_dialog->parameters), FALSE);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (prop_dialog->parameters),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  g_signal_connect (G_OBJECT (select),
                    "changed",
                    G_CALLBACK (parameter_selected),
                    umlclass);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "family", "monospace", NULL);
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     COL_PARAM_TITLE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prop_dialog->parameters),
                               column);

  gtk_container_add (GTK_CONTAINER (scrolled_win), prop_dialog->parameters);
  gtk_widget_show (prop_dialog->parameters);


  vbox3 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_box_set_homogeneous (GTK_BOX(vbox3), TRUE);


  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-add",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Add Parameter"));
  prop_dialog->param_new_button = button;
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (parameters_list_new_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);


  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-remove",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Remove Parameter"));
  prop_dialog->param_delete_button = button;
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (parameters_list_delete_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);


  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-up",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Parameter Up"));
  prop_dialog->param_up_button = button;
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (parameters_list_move_up_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);


  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-down",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Parameter Down"));
  prop_dialog->param_down_button = button;
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (parameters_list_move_down_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox3), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox2), vbox3, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, FALSE, 0);
  /* end: Parameter list editor */

  return vbox2;
}


static void
param_name_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    g_clear_pointer (&param->name, g_free);
    param->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_parameter (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_type_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    g_clear_pointer (&param->type, g_free);
    param->type = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_parameter (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_value_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    g_clear_pointer (&param->value, g_free);
    param->value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_parameter (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_comment_changed (GtkTextBuffer *buffer, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, NULL)) {
    g_clear_pointer (&param->comment, g_free);
    param->comment = buffer_get_text (prop_dialog->param_comment_buffer);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_kind_changed (DiaOptionMenu *selector, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_parameter (prop_dialog, &param, &iter)) {
    param->kind = (DiaUmlParameterKind) dia_option_menu_get_active (selector);

    update_parameter (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static GtkWidget*
operations_parameters_data_create_vbox (UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *vbox2;
  GtkWidget *frame;
  GtkWidget *vbox3;
  GtkWidget *grid;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scrolledwindow;
  GtkWidget *omenu;

  prop_dialog = umlclass->properties_dialog;

  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  frame = gtk_frame_new(_("Parameter data"));
  vbox3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, TRUE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox3), grid, FALSE, FALSE, 0);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->param_name = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_name_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);

  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->param_type = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_type_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);

  label = gtk_label_new (_("Def. value:"));
  gtk_widget_set_tooltip_text (label, _("Default value"));
  entry = gtk_entry_new ();
  prop_dialog->param_value = GTK_ENTRY(entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_value_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);

  label = gtk_label_new (_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow),
                                  GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  prop_dialog->param_comment_buffer = gtk_text_buffer_new (NULL);
  entry = gtk_text_view_new_with_buffer (prop_dialog->param_comment_buffer);
  prop_dialog->param_comment = GTK_TEXT_VIEW (entry);
  g_signal_connect (prop_dialog->param_comment_buffer,
                    "changed",
                    G_CALLBACK (param_comment_changed),
                    umlclass);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (entry),TRUE);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 2, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 3, 1, 1, 1);
  gtk_widget_set_hexpand(scrolledwindow, TRUE);

  label = gtk_label_new (_("Direction:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label,  2, 0, 1, 1);

  prop_dialog->param_kind = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu),
                    "changed",
                    G_CALLBACK (param_kind_changed),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Undefined"), DIA_UML_UNDEF_KIND);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("In"), DIA_UML_IN);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Out"), DIA_UML_OUT);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("In & Out"), DIA_UML_INOUT);
  dia_option_menu_set_active (DIA_OPTION_MENU (omenu), DIA_UML_UNDEF_KIND);
  gtk_grid_attach (GTK_GRID (grid), omenu, 3, 0, 1, 1);

  return vbox2;
}


static void
operation_selected (GtkTreeSelection *selection,
                    UMLClass         *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  if (get_current_operation (prop_dialog, &op, NULL)) {
    operations_set_values (prop_dialog, op);
    operations_set_sensitive (prop_dialog, TRUE);

    gtk_widget_grab_focus (GTK_WIDGET (prop_dialog->op_name));
  } else {
    operations_set_sensitive (prop_dialog, FALSE);
    operations_clear_values (prop_dialog);
  }
}


void
_operations_create_page (GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *image;
  GtkWidget *frame;
  GtkTreeSelection *select;
  GtkTreeViewColumn *column;
  GtkCellRenderer *renderer;

  prop_dialog = umlclass->properties_dialog;

  /* Operations page: */
  page_label = gtk_label_new_with_mnemonic (_("_Operations"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  prop_dialog->operations_store = gtk_list_store_new (N_OPER_COLS,
                                                      G_TYPE_STRING,
                                                      DIA_UML_TYPE_OPERATION,
                                                      PANGO_TYPE_UNDERLINE,
                                                      PANGO_TYPE_STYLE,
                                                      PANGO_TYPE_WEIGHT);
  prop_dialog->operations = gtk_tree_view_new_with_model (GTK_TREE_MODEL (prop_dialog->operations_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prop_dialog->operations), FALSE);

  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->operations));
  g_signal_connect (G_OBJECT (select),
                    "changed",
                    G_CALLBACK (operation_selected),
                    umlclass);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "family", "monospace", NULL);
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     COL_OPER_TITLE,
                                                     "underline",
                                                     COL_OPER_UNDERLINE,
                                                     "weight",
                                                     COL_OPER_BOLD,
                                                     "style",
                                                     COL_OPER_ITALIC,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prop_dialog->operations),
                               column);

  gtk_container_add (GTK_CONTAINER (scrolled_win), prop_dialog->operations);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (prop_dialog->operations),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (prop_dialog->operations);

  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-add",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Add Operation"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (operations_list_new_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-remove",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Remove Operation"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (operations_list_delete_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-up",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Operation Up"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (operations_list_move_up_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-down",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Operation Down"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (operations_list_move_down_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (_("Operation data"));
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  hbox = operations_data_create_hbox (umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  /* parameter stuff below operation stuff */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  vbox3 = operations_parameters_editor_create_vbox (umlclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 6);

  vbox3 = operations_parameters_data_create_vbox (umlclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 6);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 6);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}
