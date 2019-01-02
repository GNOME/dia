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

#include <gtk/gtk.h>

#include "class.h"
#include "class_dialog.h"
#include "dia_dirs.h"
#include "editor/dia-uml-class-editor.h"
#include "editor/dia-uml-list-store.h"
#include "dia-uml-class.h"

/*************************************************************
 ******************** OPERATIONS *****************************
 *************************************************************/

void
_operations_read_from_dialog (UMLClass *umlclass,
                              UMLClassDialog *prop_dialog,
                              int connection_index)
{
  GListModel *list_store;
  DiaUmlListData *itm;
  DiaObject *obj;
  DiaUmlClass *editor_state;
  gboolean op_visible = TRUE;
  int i = 0;

  /* Cast to DiaObject (UMLClass -> Element -> DiaObject) */
  obj = &umlclass->element.object;

  editor_state = dia_uml_class_editor_get_class (DIA_UML_CLASS_EDITOR (umlclass->properties_dialog->editor));

  /* Free current operations: */
  /* Clear those already stored */
  g_list_free_full (umlclass->operations, g_object_unref);
  umlclass->operations = NULL;

  /* If operations visible and not suppressed */
  op_visible = ( gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop_dialog->op_vis ))) &&
               (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (prop_dialog->op_supp)));

  list_store = dia_uml_class_get_operations (editor_state);
  /* Insert new operations and remove them from gtklist: */
  while ((itm = g_list_model_get_item (G_LIST_MODEL (list_store), i))) {
    DiaUmlOperation *op = DIA_UML_OPERATION (itm);
    umlclass->operations = g_list_append(umlclass->operations, g_object_ref (op));

    if (op->l_connection == NULL) {
      dia_uml_operation_ensure_connection_points (op, obj);
      
      prop_dialog->added_connections = g_list_prepend (prop_dialog->added_connections,
                                                       op->l_connection);
      prop_dialog->added_connections = g_list_prepend (prop_dialog->added_connections,
                                                       op->r_connection);
    }

    if (op_visible) { 
      obj->connections[connection_index] = op->l_connection;
      connection_index++;
      obj->connections[connection_index] = op->r_connection;
      connection_index++;
    } else {
      _umlclass_store_disconnects(prop_dialog, op->l_connection);
      object_remove_connections_to(op->l_connection);
      _umlclass_store_disconnects(prop_dialog, op->r_connection);
      object_remove_connections_to(op->r_connection);
    }

    i++;
  }
}

void 
_operations_create_page(GtkNotebook *notebook, UMLClass *umlclass)
{
  GtkWidget *page_label;

  /* Operations page: */
  page_label = gtk_label_new_with_mnemonic (_("_Operations"));
  umlclass->properties_dialog->editor = dia_uml_class_editor_new (dia_uml_class_new (umlclass));

  gtk_widget_show (page_label);
  gtk_widget_show (umlclass->properties_dialog->editor);

  gtk_notebook_append_page (notebook, umlclass->properties_dialog->editor, page_label);
}

void
list_box_separators (GtkListBoxRow *row,
                     GtkListBoxRow *before,
                     gpointer       user_data)
{
  GtkWidget *current;

  if (before == NULL) {
    gtk_list_box_row_set_header (row, NULL);
    return;
  }

  current = gtk_list_box_row_get_header (row);
  if (current == NULL) {
    current = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
    gtk_widget_show (current);
    gtk_list_box_row_set_header (row, current);
  }
}
