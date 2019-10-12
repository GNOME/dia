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
 */
#include <config.h>

#include <assert.h>
#undef GTK_DISABLE_DEPRECATED /* GtkList, ... */
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


static void
parameters_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_value), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->param_kind), val);
}


static void
parameters_set_values (UMLClassDialog *prop_dialog,
                       UMLParameter   *param)
{
  gtk_entry_set_text (prop_dialog->param_name, param->name? param->name : "");
  gtk_entry_set_text (prop_dialog->param_type, param->type? param->type : "");
  gtk_entry_set_text (prop_dialog->param_value, param->value? param->value : "");
  gtk_text_buffer_set_text (prop_dialog->param_comment_buffer,
                            param->comment? g_strdup (param->comment) : "",
                            -1);

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
                              UML_UNDEF_KIND);
}


static gboolean
add_to_list (GtkTreeModel *model,
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

  gtk_tree_model_foreach (model, add_to_list, op);
}


static void
parameters_list_new_callback (GtkWidget *button,
                              UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  char *utf;
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  prop_dialog = umlclass->properties_dialog;

  current_op = (UMLOperation *)
    g_object_get_data (G_OBJECT (prop_dialog->current_op), "user_data");

  param = uml_parameter_new ();

  utf = uml_parameter_get_string (param);

  gtk_list_store_append (prop_dialog->parameters_store, &iter);
  gtk_list_store_set (prop_dialog->parameters_store,
                      &iter,
                      COL_PARAM_TITLE, utf,
                      COL_PARAM_PARAM, param,
                      -1);
  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  gtk_tree_selection_select_iter (selection, &iter);

  sync_params_to_operation (GTK_TREE_MODEL (prop_dialog->parameters_store),
                            current_op);

  g_clear_pointer (&utf, g_free);
  g_clear_pointer (&param, uml_parameter_unref);
}


static void
parameters_list_delete_callback (GtkWidget *button,
                                 UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  UMLParameter *param;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreeModel *model;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    /* Remove from current operations parameter list: */
    current_op = (UMLOperation *)
      g_object_get_data (G_OBJECT (prop_dialog->current_op), "user_data");

    gtk_list_store_remove (prop_dialog->parameters_store, &iter);

    sync_params_to_operation (model, current_op);

    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
parameters_list_move_up_callback (GtkWidget *button,
                                  UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreeModel *model;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (model, &iter);
    GtkTreeIter prev;

    if (path != NULL && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (model, &prev, path)) {
      gtk_list_store_move_before (prop_dialog->parameters_store, &iter, &prev);
    } else {
      gtk_list_store_move_before (prop_dialog->parameters_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    gtk_tree_selection_select_iter (selection, &iter);

    /* Move parameter in current operations list: */
    current_op = (UMLOperation *)
      g_object_get_data (G_OBJECT (prop_dialog->current_op), "user_data");

    sync_params_to_operation (model, current_op);
  }
}

static void
parameters_list_move_down_callback (GtkWidget *button,
                                    UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *current_op;
  GtkTreeIter iter;
  GtkTreeSelection *selection;
  GtkTreeModel *model;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (model,  &iter);
    GtkTreeIter prev;

    if (path != NULL) {
      gtk_tree_path_next (path);
      if (gtk_tree_model_get_iter (model, &prev, path)) {
        gtk_list_store_move_after (prop_dialog->parameters_store, &iter, &prev);
      } else {
        gtk_list_store_move_after (prop_dialog->parameters_store, &iter, NULL);
      }
    } else {
      gtk_list_store_move_after (prop_dialog->parameters_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    gtk_tree_selection_select_iter (selection, &iter);

    /* Move parameter in current operations list: */
    current_op = (UMLOperation *)
      g_object_get_data (G_OBJECT (prop_dialog->current_op), "user_data");

    sync_params_to_operation (model, current_op);
  }
}

static void
operations_set_sensitive(UMLClassDialog *prop_dialog, gint val)
{
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_name), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_stereotype), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_comment), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_visible), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_class_scope), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_inheritance_type), val);
  gtk_widget_set_sensitive(GTK_WIDGET(prop_dialog->op_query), val);

  gtk_widget_set_sensitive(prop_dialog->param_new_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_delete_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_down_button, val);
  gtk_widget_set_sensitive(prop_dialog->param_up_button, val);
}


static void
operations_set_values (UMLClassDialog *prop_dialog, UMLOperation *op)
{
  GList *list;
  UMLParameter *param;
  GtkTreeIter iter;
  gchar *str;

  gtk_entry_set_text (prop_dialog->op_name, op->name? op->name : "");
  gtk_entry_set_text (prop_dialog->op_type, op->type? op->type : "");
  gtk_entry_set_text (prop_dialog->op_stereotype, op->stereotype? op->stereotype : "");

  _class_set_comment (prop_dialog->op_comment, op->comment? op->comment : "");

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
    gtk_list_store_set (prop_dialog->parameters_store,
                        &iter,
                        COL_PARAM_TITLE, str,
                        COL_PARAM_PARAM, param,
                        -1);

    g_free (str);

    list = g_list_next (list);
  }
}

static void
operations_clear_values(UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text(prop_dialog->op_name, "");
  gtk_entry_set_text(prop_dialog->op_type, "");
  gtk_entry_set_text(prop_dialog->op_stereotype, "");
  _class_set_comment(prop_dialog->op_comment, "");
  gtk_toggle_button_set_active(prop_dialog->op_class_scope, FALSE);
  gtk_toggle_button_set_active(prop_dialog->op_query, FALSE);

  gtk_list_store_clear (prop_dialog->parameters_store);

  parameters_set_sensitive(prop_dialog, FALSE);
}


static void
operations_get_values (UMLClassDialog *prop_dialog,
                       UMLOperation   *op)
{
  g_clear_pointer (&op->name, g_free);
  g_clear_pointer (&op->type, g_free);
  g_clear_pointer (&op->comment, g_free);
  g_clear_pointer (&op->stereotype, g_free);

  op->name = g_strdup (gtk_entry_get_text (prop_dialog->op_name));
  op->type = g_strdup (gtk_entry_get_text (prop_dialog->op_type));
  op->comment = _class_get_comment (prop_dialog->op_comment);
  op->stereotype = g_strdup (gtk_entry_get_text (prop_dialog->op_stereotype));

  op->visibility = (UMLVisibility) dia_option_menu_get_active (DIA_OPTION_MENU (prop_dialog->op_visible));

  op->class_scope = gtk_toggle_button_get_active (prop_dialog->op_class_scope);
  op->inheritance_type = (UMLInheritanceType) dia_option_menu_get_active (DIA_OPTION_MENU (prop_dialog->op_inheritance_type));

  op->query = gtk_toggle_button_get_active (prop_dialog->op_query);
}


void
_operations_get_current_values (UMLClassDialog *prop_dialog)
{
  UMLOperation *current_op;
  GtkLabel *label;
  char *new_str;

  if (prop_dialog->current_op != NULL) {
    current_op = (UMLOperation *)
      g_object_get_data (G_OBJECT (prop_dialog->current_op), "user_data");
    if (current_op != NULL) {
      operations_get_values (prop_dialog, current_op);
      label = GTK_LABEL (gtk_bin_get_child (GTK_BIN (prop_dialog->current_op)));
      new_str = uml_get_operation_string (current_op);
      gtk_label_set_text (label, new_str);
      g_free (new_str);
    }
  }
}


static void
operations_list_item_destroy_callback(GtkWidget *list_item,
				      gpointer data)
{
  UMLOperation *op;

  op = (UMLOperation *) g_object_get_data(G_OBJECT(list_item), "user_data");

  if (op != NULL) {
    uml_operation_free (op);
    /*printf("Destroying operation list_item's user_data!\n");*/
  }
}

static void
operations_list_selection_changed_callback(GtkWidget *gtklist,
					   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GObject *list_item;
  UMLOperation *op;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog)
    return; /* maybe hiding a bug elsewhere */

  _operations_get_current_values(prop_dialog);

  list = GTK_LIST(gtklist)->selection;
  if (!list) { /* No selected */
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
    prop_dialog->current_op = NULL;
    return;
  }

  list_item = G_OBJECT(list->data);
  op = (UMLOperation *)g_object_get_data(G_OBJECT(list_item), "user_data");
  operations_set_values(prop_dialog, op);
  operations_set_sensitive(prop_dialog, TRUE);

  prop_dialog->current_op = GTK_LIST_ITEM(list_item);
  gtk_widget_grab_focus(GTK_WIDGET(prop_dialog->op_name));
}

static void
operations_list_new_callback(GtkWidget *button,
			     UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkWidget *list_item;
  UMLOperation *op;
  char *utfstr;

  prop_dialog = umlclass->properties_dialog;

  _operations_get_current_values(prop_dialog);

  op = uml_operation_new();
  /* need to make new ConnectionPoints valid and remember them */
  uml_operation_ensure_connection_points (op, &umlclass->element.object);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->left_connection);
  prop_dialog->added_connections =
    g_list_prepend(prop_dialog->added_connections, op->right_connection);


  utfstr = uml_get_operation_string (op);

  list_item = gtk_list_item_new_with_label (utfstr);
  gtk_widget_show (list_item);
  g_free (utfstr);

  g_object_set_data(G_OBJECT(list_item), "user_data", op);
  g_signal_connect (G_OBJECT (list_item), "destroy",
		    G_CALLBACK (operations_list_item_destroy_callback), NULL);

  list = g_list_append(NULL, list_item);
  gtk_list_append_items(prop_dialog->operations_list, list);

  if (prop_dialog->operations_list->children != NULL)
    gtk_list_unselect_child(prop_dialog->operations_list,
			    GTK_WIDGET(prop_dialog->operations_list->children->data));
  gtk_list_select_child(prop_dialog->operations_list, list_item);
}

static void
operations_list_delete_callback(GtkWidget *button,
				UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  UMLOperation *op;


  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    op = (UMLOperation *)
      g_object_get_data(G_OBJECT(gtklist->selection->data), "user_data");

    if (op->left_connection != NULL) {
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->left_connection);
      prop_dialog->deleted_connections =
	g_list_prepend(prop_dialog->deleted_connections,
		       op->right_connection);
    }

    list = g_list_prepend(NULL, gtklist->selection->data);
    gtk_list_remove_items(gtklist, list);
    g_list_free(list);
    operations_clear_values(prop_dialog);
    operations_set_sensitive(prop_dialog, FALSE);
  }
}

static void
operations_list_move_up_callback(GtkWidget *button,
				 UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i>0)
      i--;

    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }

}

static void
operations_list_move_down_callback(GtkWidget *button,
				   UMLClass *umlclass)
{
  GList *list;
  UMLClassDialog *prop_dialog;
  GtkList *gtklist;
  GtkWidget *list_item;
  int i;

  prop_dialog = umlclass->properties_dialog;
  gtklist = GTK_LIST(prop_dialog->operations_list);

  if (gtklist->selection != NULL) {
    list_item = GTK_WIDGET(gtklist->selection->data);

    i = gtk_list_child_position(gtklist, list_item);
    if (i<(g_list_length(gtklist->children)-1))
      i++;

    g_object_ref(list_item);
    list = g_list_prepend(NULL, list_item);
    gtk_list_remove_items(gtklist, list);
    gtk_list_insert_items(gtklist, list, i);
    g_object_unref(list_item);

    gtk_list_select_child(gtklist, list_item);
  }
}

void
_operations_read_from_dialog(UMLClass *umlclass,
			    UMLClassDialog *prop_dialog,
			    int connection_index)
{
  GList *list;
  UMLOperation *op;
  GtkWidget *list_item;
  GList *clear_list;
  DiaObject *obj;

  obj = &umlclass->element.object;

  /* if currently select op is changed in the entries, update from widgets */
  _operations_get_current_values (prop_dialog);

  /* Free current operations: */
  g_list_free_full (umlclass->operations, (GDestroyNotify) uml_operation_free);
  umlclass->operations = NULL;

  /* Insert new operations and remove them from gtklist: */
  list = GTK_LIST (prop_dialog->operations_list)->children;
  clear_list = NULL;
  while (list != NULL) {
    list_item = GTK_WIDGET(list->data);

    clear_list = g_list_prepend (clear_list, list_item);
    op = (UMLOperation *)
      g_object_get_data(G_OBJECT(list_item), "user_data");
    g_object_set_data(G_OBJECT(list_item), "user_data", NULL);
    umlclass->operations = g_list_append(umlclass->operations, op);

    if (op->left_connection == NULL) {
      uml_operation_ensure_connection_points (op, obj);

      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->left_connection);
      prop_dialog->added_connections =
	g_list_prepend(prop_dialog->added_connections,
		       op->right_connection);
    }

    if ( (gtk_toggle_button_get_active (prop_dialog->op_vis)) &&
         (!gtk_toggle_button_get_active (prop_dialog->op_supp)) ) {
      obj->connections[connection_index] = op->left_connection;
      connection_index++;
      obj->connections[connection_index] = op->right_connection;
      connection_index++;
    } else {
      _umlclass_store_disconnects(prop_dialog, op->left_connection);
      object_remove_connections_to(op->left_connection);
      _umlclass_store_disconnects(prop_dialog, op->right_connection);
      object_remove_connections_to(op->right_connection);
    }

    list = g_list_next(list);
  }
  clear_list = g_list_reverse (clear_list);
  gtk_list_remove_items (GTK_LIST (prop_dialog->operations_list), clear_list);
  g_list_free (clear_list);
}

void
_operations_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLOperation *op_copy;
  GtkWidget *list_item;
  GList *list;
  int i;

  prop_dialog = umlclass->properties_dialog;

  if (prop_dialog->operations_list->children == NULL) {
    i = 0;
    list = umlclass->operations;
    while (list != NULL) {
      UMLOperation *op = (UMLOperation *)list->data;
      gchar *opstr = uml_get_operation_string (op);

      list_item = gtk_list_item_new_with_label (opstr);
      op_copy = uml_operation_copy (op);
      /* Looks wrong but is required for the complicate connections memory management */
      op_copy->left_connection = op->left_connection;
      op_copy->right_connection = op->right_connection;
      g_object_set_data(G_OBJECT(list_item), "user_data", (gpointer) op_copy);
      g_signal_connect (G_OBJECT (list_item), "destroy",
			G_CALLBACK (operations_list_item_destroy_callback), NULL);
      gtk_container_add (GTK_CONTAINER (prop_dialog->operations_list), list_item);
      gtk_widget_show (list_item);

      list = g_list_next(list); i++;
      g_free (opstr);
    }

    /* set operations non-sensitive */
    prop_dialog->current_op = NULL;
    operations_set_sensitive(prop_dialog, FALSE);
    operations_clear_values(prop_dialog);
  }
}


static void
operations_update (GtkWidget *widget, UMLClass *umlclass)
{
  _operations_get_current_values (umlclass->properties_dialog);
}


static int
operations_update_event (GtkWidget     *widget,
                         GdkEventFocus *ev,
                         UMLClass      *umlclass)
{
  _operations_get_current_values (umlclass->properties_dialog);

  return 0;
}

static GtkWidget*
operations_data_create_hbox (UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *hbox;
  GtkWidget *vbox2;
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *omenu;
  GtkWidget *scrolledwindow;
  GtkWidget *checkbox;

  prop_dialog = umlclass->properties_dialog;

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);

  vbox2 = gtk_vbox_new(FALSE, 0);

  /* table containing operation 'name' up to 'query' and also the comment */
  table = gtk_table_new (5, 3, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox2), table, FALSE, FALSE, 0);

  label = gtk_label_new(_("Name:"));
  entry = gtk_entry_new();
  prop_dialog->op_name = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (operations_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Type:"));
  entry = gtk_entry_new();
  prop_dialog->op_type = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (operations_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new(_("Stereotype:"));
  entry = gtk_entry_new();
  prop_dialog->op_stereotype = GTK_ENTRY(entry);
  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (operations_update_event), umlclass);
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (operations_update), umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);


  label = gtk_label_new(_("Visibility:"));

  prop_dialog->op_visible = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu),
                    "changed",
                    G_CALLBACK (operations_update),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Public"), UML_PUBLIC);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Private"), UML_PRIVATE);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Protected"), UML_PROTECTED);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Implementation"), UML_IMPLEMENTATION);

  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
					/* left, right, top, bottom */
  gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);
  /* end: Visibility */

  label = gtk_label_new(_("Inheritance type:"));

  prop_dialog->op_inheritance_type = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu), "changed",
		    G_CALLBACK (operations_update), umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Abstract"), UML_ABSTRACT);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Polymorphic (virtual)"), UML_POLYMORPHIC);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Leaf (final)"), UML_LEAF);

  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);
  /* end: Inheritance type */

  checkbox = gtk_check_button_new_with_label(_("Class scope"));
  prop_dialog->op_class_scope = GTK_TOGGLE_BUTTON(checkbox);
  gtk_table_attach (GTK_TABLE (table), checkbox, 2,3,2,3, GTK_FILL,0, 0,2);

  checkbox = gtk_check_button_new_with_label(_("Query"));
  prop_dialog->op_query = GTK_TOGGLE_BUTTON(checkbox);
  gtk_table_attach (GTK_TABLE (table), checkbox, 3,4,2,3, GTK_FILL,0, 2,0);

  label = gtk_label_new(_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_IN);
  /* with GTK_POLICY_NEVER the comment filed gets smaller unti l text is entered; than it would resize the dialog! */
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  entry = gtk_text_view_new ();
  prop_dialog->op_comment = GTK_TEXT_VIEW(entry);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (entry),TRUE);

  g_signal_connect (G_OBJECT (entry), "focus_out_event",
		    G_CALLBACK (operations_update_event), umlclass);

  gtk_table_attach (GTK_TABLE (table), label, 4,5,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 4,5,1,3, GTK_FILL | GTK_EXPAND,0, 0,0);
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

  vbox2 = gtk_vbox_new(FALSE, 5);
  /* Parameters list label */
  hbox2 = gtk_hbox_new(FALSE, 5);

  label = gtk_label_new(_("Parameters:"));
  gtk_box_pack_start( GTK_BOX(hbox2), label, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, TRUE, TRUE, 0);

  /* Parameters list editor - with of list at least width of buttons*/
  hbox2 = gtk_vbox_new (FALSE, 6);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win), GTK_SHADOW_IN);
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
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  g_signal_connect (G_OBJECT (select),
                    "changed",
                    G_CALLBACK (parameter_selected),
                    umlclass);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     COL_PARAM_TITLE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prop_dialog->parameters),
                               column);

  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), prop_dialog->parameters);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (prop_dialog->parameters),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (prop_dialog->parameters);


  vbox3 = gtk_hbox_new (TRUE, 6);


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
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  UMLParameter *param;
  char *new_str;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    g_clear_pointer (&param->name, g_free);
    param->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    new_str = uml_parameter_get_string (param);

    gtk_list_store_set (prop_dialog->parameters_store,
                        &iter,
                        COL_PARAM_TITLE, new_str,
                        -1);

    g_clear_pointer (&new_str, g_free);
    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_type_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  UMLParameter *param;
  char *new_str;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    g_clear_pointer (&param->type, g_free);
    param->type = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    new_str = uml_parameter_get_string (param);

    gtk_list_store_set (prop_dialog->parameters_store,
                        &iter,
                        COL_PARAM_TITLE, new_str,
                        -1);

    g_clear_pointer (&new_str, g_free);
    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_value_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  UMLParameter *param;
  char *new_str;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    g_clear_pointer (&param->value, g_free);
    param->value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    new_str = uml_parameter_get_string (param);

    gtk_list_store_set (prop_dialog->parameters_store,
                        &iter,
                        COL_PARAM_TITLE, new_str,
                        -1);

    g_clear_pointer (&new_str, g_free);
    g_clear_pointer (&param, uml_parameter_unref);
  }
}


static void
param_comment_changed (GtkTextBuffer *buffer, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  UMLParameter *param;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

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
  GtkTreeModel *model;
  GtkTreeSelection *selection;
  UMLParameter *param;
  char *new_str;

  prop_dialog = umlclass->properties_dialog;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->parameters));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_PARAM_PARAM, &param, -1);

    param->kind = (UMLParameterKind) dia_option_menu_get_active (selector);

    new_str = uml_parameter_get_string (param);

    gtk_list_store_set (prop_dialog->parameters_store,
                        &iter,
                        COL_PARAM_TITLE, new_str,
                        -1);

    g_clear_pointer (&new_str, g_free);
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
  GtkWidget *table;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scrolledwindow;
  GtkWidget *omenu;

  prop_dialog = umlclass->properties_dialog;

  vbox2 = gtk_vbox_new(FALSE, 5);
  frame = gtk_frame_new(_("Parameter data"));
  vbox3 = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox3), 5);
  gtk_container_add (GTK_CONTAINER (frame), vbox3);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox2), frame, FALSE, TRUE, 0);

  table = gtk_table_new (3, 4, FALSE);
  gtk_table_set_col_spacings (GTK_TABLE (table), 5);
  gtk_box_pack_start (GTK_BOX (vbox3), table, FALSE, FALSE, 0);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->param_name = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_name_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,0,1, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,0,1, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->param_type = GTK_ENTRY (entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_type_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,1,2, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new (_("Def. value:"));
  gtk_widget_set_tooltip_text (label, _("Default value"));
  entry = gtk_entry_new ();
  prop_dialog->param_value = GTK_ENTRY(entry);
  g_signal_connect (entry,
                    "changed",
                    G_CALLBACK (param_value_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 0,1,2,3, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), entry, 1,2,2,3, GTK_FILL | GTK_EXPAND,0, 0,2);

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
  gtk_table_attach (GTK_TABLE (table), label, 2,3,1,2, GTK_FILL,0, 0,0);
  gtk_table_attach (GTK_TABLE (table), scrolledwindow, 3,4,1,3, GTK_FILL | GTK_EXPAND,0, 0,2);

  label = gtk_label_new (_("Direction:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_table_attach (GTK_TABLE (table), label, 2,3,0,1, GTK_FILL,0, 0,3);

  prop_dialog->param_kind = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu),
                    "changed",
                    G_CALLBACK (param_kind_changed),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Undefined"), UML_UNDEF_KIND);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("In"), UML_IN);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Out"), UML_OUT);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("In & Out"), UML_INOUT);
  dia_option_menu_set_active (DIA_OPTION_MENU (omenu), UML_UNDEF_KIND);
  gtk_table_attach (GTK_TABLE (table), omenu, 3,4,0,1, GTK_FILL, 0, 0,3);

  return vbox2;
}

void
_operations_create_page(GtkNotebook *notebook,  UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *vbox3;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *list;
  GtkWidget *frame;

  prop_dialog = umlclass->properties_dialog;

  /* Operations page: */
  page_label = gtk_label_new_with_mnemonic (_("_Operations"));

  vbox = gtk_vbox_new(FALSE, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 10);

  hbox = gtk_hbox_new(FALSE, 5);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  list = gtk_list_new ();
  prop_dialog->operations_list = GTK_LIST(list);
  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (list);

  g_signal_connect (G_OBJECT (list), "selection_changed",
		    G_CALLBACK(operations_list_selection_changed_callback),
		    umlclass);

  vbox2 = gtk_vbox_new(FALSE, 5);

  button = gtk_button_new_from_stock (GTK_STOCK_NEW);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(operations_list_new_callback),
		    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_DELETE);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(operations_list_delete_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_UP);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(operations_list_move_up_callback), umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);
  button = gtk_button_new_from_stock (GTK_STOCK_GO_DOWN);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(operations_list_move_down_callback), umlclass);

  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, TRUE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new(_("Operation data"));
  vbox2 = gtk_vbox_new(FALSE, 0);
  hbox = operations_data_create_hbox (umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 0);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show(frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  /* parameter stuff below operation stuff */
  hbox = gtk_hbox_new (FALSE, 5);
  vbox3 = operations_parameters_editor_create_vbox (umlclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);

  vbox3 = operations_parameters_data_create_vbox (umlclass);
  gtk_box_pack_start (GTK_BOX (hbox), vbox3, TRUE, TRUE, 5);

  gtk_box_pack_start (GTK_BOX (vbox2), hbox, TRUE, TRUE, 5);

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}
