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

/************************************************************
 ******************** ATTRIBUTES ****************************
 ************************************************************/

enum {
  COL_ATTR_TITLE,
  COL_ATTR_ATTR,
  COL_ATTR_UNDERLINE,
  N_COLS
};


static gboolean
get_current_attribute (UMLClassDialog  *dialog,
                       UMLAttribute   **attr,
                       GtkTreeIter     *c_iter)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->attributes));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_ATTR_ATTR, attr, -1);

    if (c_iter) {
      *c_iter = iter;
    }

    return TRUE;
  }

  return FALSE;
}


static void
update_attribute (UMLClassDialog *dialog,
                  UMLAttribute   *attr,
                  GtkTreeIter    *iter)
{
  PangoUnderline underline;
  char *title;

  underline = attr->class_scope ? PANGO_UNDERLINE_SINGLE : PANGO_UNDERLINE_NONE;
  title = uml_attribute_get_string (attr);

  gtk_list_store_set (dialog->attributes_store,
                      iter,
                      COL_ATTR_ATTR, attr,
                      COL_ATTR_TITLE, title,
                      COL_ATTR_UNDERLINE, underline,
                      -1);

  g_clear_pointer (&title, g_free);
}


static void
attributes_set_sensitive (UMLClassDialog *prop_dialog, gboolean val)
{
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_name), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_type), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_value), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_comment), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_visible), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->attr_class_scope), val);
}


static void
attributes_set_values (UMLClassDialog *prop_dialog, UMLAttribute *attr)
{
  char *comment = NULL;

  gtk_entry_set_text (prop_dialog->attr_name,
                      attr->name? attr->name : "");
  gtk_entry_set_text (prop_dialog->attr_type,
                      attr->type? attr->type : "");
  gtk_entry_set_text (prop_dialog->attr_value,
                      attr->value? attr->value : "");

  // So, this shouldn't need to have a strdup but weird stuff happens without
  comment = g_strdup (attr->comment? attr->comment : "");
  gtk_text_buffer_set_text (prop_dialog->attr_comment_buffer,
                            comment,
                            -1);
  g_clear_pointer (&comment, g_free);

  dia_option_menu_set_active (DIA_OPTION_MENU (prop_dialog->attr_visible),
                              attr->visibility);
  gtk_toggle_button_set_active (prop_dialog->attr_class_scope,
                                attr->class_scope);
}


static void
attributes_clear_values (UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text (prop_dialog->attr_name, "");
  gtk_entry_set_text (prop_dialog->attr_type, "");
  gtk_entry_set_text (prop_dialog->attr_value, "");
  gtk_text_buffer_set_text (prop_dialog->attr_comment_buffer, "", -1);
  gtk_toggle_button_set_active (prop_dialog->attr_class_scope, FALSE);
}


static void
attributes_list_new_callback (GtkWidget *button,
                              UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  prop_dialog = umlclass->properties_dialog;

  attr = uml_attribute_new ();

  /* need to make the new ConnectionPoint valid and remember them */
  uml_attribute_ensure_connection_points (attr, &umlclass->element.object);
  prop_dialog->added_connections =
    g_list_prepend (prop_dialog->added_connections, attr->left_connection);
  prop_dialog->added_connections =
    g_list_prepend (prop_dialog->added_connections, attr->right_connection);

  gtk_list_store_append (prop_dialog->attributes_store, &iter);
  update_attribute (prop_dialog, attr, &iter);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->attributes));
  gtk_tree_selection_select_iter (selection, &iter);

  g_clear_pointer (&attr, uml_attribute_unref);
}


static void
attributes_list_delete_callback (GtkWidget *button,
                                 UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    if (attr->left_connection != NULL) {
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        attr->left_connection);
      prop_dialog->deleted_connections =
        g_list_prepend (prop_dialog->deleted_connections,
                        attr->right_connection);
    }

    gtk_list_store_remove (prop_dialog->attributes_store, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
attributes_list_move_up_callback (GtkWidget *button,
                                  UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *current_attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &current_attr, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->attributes_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->attributes_store), &prev, path)) {
      gtk_list_store_move_before (prop_dialog->attributes_store, &iter, &prev);
    } else {
      gtk_list_store_move_before (prop_dialog->attributes_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->attributes));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_attr, uml_attribute_unref);
  }
}


static void
attributes_list_move_down_callback (GtkWidget *button,
                                    UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *current_attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &current_attr, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->attributes_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL) {
      gtk_tree_path_next (path);
      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->attributes_store), &prev, path)) {
        gtk_list_store_move_after (prop_dialog->attributes_store, &iter, &prev);
      } else {
        gtk_list_store_move_after (prop_dialog->attributes_store, &iter, NULL);
      }
    } else {
      gtk_list_store_move_after (prop_dialog->attributes_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->attributes));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_attr, uml_attribute_unref);
  }
}


static void
name_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    g_clear_pointer (&attr->name, g_free);
    attr->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_attribute (prop_dialog, attr, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
type_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    g_clear_pointer (&attr->type, g_free);
    attr->type = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_attribute (prop_dialog, attr, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
value_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    g_clear_pointer (&attr->value, g_free);
    attr->value = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_attribute (prop_dialog, attr, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
comment_changed (GtkTextBuffer *buffer, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, NULL)) {
    g_clear_pointer (&attr->comment, g_free);
    attr->comment = buffer_get_text (prop_dialog->attr_comment_buffer);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
visibility_changed (DiaOptionMenu *selector, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    attr->visibility = (DiaUmlVisibility) dia_option_menu_get_active (selector);

    update_attribute (prop_dialog, attr, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


static void
scope_changed (GtkToggleButton *toggle, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_attribute (prop_dialog, &attr, &iter)) {
    attr->class_scope = gtk_toggle_button_get_active (toggle);

    update_attribute (prop_dialog, attr, &iter);

    g_clear_pointer (&attr, uml_attribute_unref);
  }
}


struct AddAttrData {
  UMLClass       *class;
  UMLClassDialog *dialog;
  int             connection_index;
};


static gboolean
add_attr_to_list (GtkTreeModel *model,
                  GtkTreePath  *path,
                  GtkTreeIter  *iter,
                  gpointer      udata)
{
  struct AddAttrData *data = udata;
  UMLAttribute *attr = NULL;

  // Don't free attr, transfering to the list
  gtk_tree_model_get (model, iter, COL_ATTR_ATTR, &attr, -1);

  data->class->attributes = g_list_append (data->class->attributes, attr);

  if (attr->left_connection == NULL) {
    uml_attribute_ensure_connection_points (attr, DIA_OBJECT (data->class));

    data->dialog->added_connections =
      g_list_prepend (data->dialog->added_connections,
                      attr->left_connection);
    data->dialog->added_connections =
      g_list_prepend (data->dialog->added_connections,
                      attr->right_connection);
  }

  if (( gtk_toggle_button_get_active (data->dialog->attr_vis)) &&
      (!gtk_toggle_button_get_active (data->dialog->attr_supp))) {
    DIA_OBJECT (data->class)->connections[data->connection_index] = attr->left_connection;
    data->connection_index++;
    DIA_OBJECT (data->class)->connections[data->connection_index] = attr->right_connection;
    data->connection_index++;
  } else {
    _umlclass_store_disconnects (data->dialog, attr->left_connection);
    object_remove_connections_to (attr->left_connection);
    _umlclass_store_disconnects (data->dialog, attr->right_connection);
    object_remove_connections_to (attr->right_connection);
  }

  return FALSE;
}


void
_attributes_read_from_dialog (UMLClass       *umlclass,
                              UMLClassDialog *prop_dialog,
                              int             connection_index)
{
  struct AddAttrData data;

  /* Free current attributes: */
  g_list_free_full (umlclass->attributes, (GDestroyNotify) uml_attribute_unref);
  umlclass->attributes = NULL;

  data.class = umlclass;
  data.dialog = prop_dialog;
  data.connection_index = connection_index;

  gtk_tree_model_foreach (GTK_TREE_MODEL (prop_dialog->attributes_store),
                          add_attr_to_list,
                          &data);
  gtk_list_store_clear (prop_dialog->attributes_store);

#if 0 /* UMLClass is *known* to be in an incositent state here, check later or crash ... */
  umlclass_sanity_check(umlclass, "Read from dialog");
#endif
}

void
_attributes_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLAttribute *attr_copy;
  GtkTreeIter iter;
  GList *list;

#ifdef DEBUG
  umlclass_sanity_check(umlclass, "Filling in dialog");
#endif

  prop_dialog = umlclass->properties_dialog;

  gtk_list_store_clear (prop_dialog->attributes_store);

  /* copy in new attributes: */
  list = umlclass->attributes;
  while (list != NULL) {
    UMLAttribute *attr = (UMLAttribute *) list->data;

    attr_copy = uml_attribute_copy (attr);

    /* looks wrong but required for complicated ConnectionPoint memory management */
    attr_copy->left_connection = attr->left_connection;
    attr_copy->right_connection = attr->right_connection;

    gtk_list_store_append (prop_dialog->attributes_store, &iter);
    update_attribute (prop_dialog, attr_copy, &iter);

    list = g_list_next(list);

    g_clear_pointer (&attr_copy, uml_attribute_unref);
  }

  /* set attributes non-sensitive */
  attributes_set_sensitive (prop_dialog, FALSE);
  attributes_clear_values (prop_dialog);
}


static void
attribute_selected (GtkTreeSelection *selection,
                    UMLClass         *umlclass)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  UMLAttribute *attr;
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog) {
    return; /* maybe hiding a bug elsewhere */
  }

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_ATTR_ATTR, &attr, -1);

    attributes_set_values (prop_dialog, attr);
    attributes_set_sensitive (prop_dialog, TRUE);

    g_clear_pointer (&attr, uml_attribute_unref);

    gtk_widget_grab_focus (GTK_WIDGET (prop_dialog->attr_name));
  } else {
    attributes_set_sensitive (prop_dialog, FALSE);
    attributes_clear_values (prop_dialog);
  }
}


void
_attributes_create_page (GtkNotebook *notebook, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  GtkWidget *page_label;
  GtkWidget *label;
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *vbox2;
  GtkWidget *hbox2;
  GtkWidget *grid;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkWidget *scrolled_win;
  GtkWidget *button;
  GtkWidget *frame;
  GtkWidget *omenu;
  GtkWidget *scrolledwindow;
  GtkWidget *image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  prop_dialog = umlclass->properties_dialog;

  /* Attributes page: */
  page_label = gtk_label_new_with_mnemonic (_("_Attributes"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
  gtk_widget_show (vbox);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  gtk_widget_show (hbox);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  prop_dialog->attributes_store = gtk_list_store_new (N_COLS,
                                                      G_TYPE_STRING,
                                                      DIA_UML_TYPE_ATTRIBUTE,
                                                      PANGO_TYPE_UNDERLINE);
  prop_dialog->attributes = gtk_tree_view_new_with_model (GTK_TREE_MODEL (prop_dialog->attributes_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prop_dialog->attributes), FALSE);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (prop_dialog->attributes),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->attributes));
  g_signal_connect (G_OBJECT (select),
                    "changed",
                    G_CALLBACK (attribute_selected),
                    umlclass);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "family", "monospace", NULL);
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     COL_ATTR_TITLE,
                                                     "underline",
                                                     COL_ATTR_UNDERLINE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prop_dialog->attributes),
                               column);

  gtk_container_add (GTK_CONTAINER (scrolled_win), prop_dialog->attributes);
  gtk_widget_show (prop_dialog->attributes);


  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_show (vbox2);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-add",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Add Attribute"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (attributes_list_new_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-remove",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Remove Attribute"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (attributes_list_delete_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-up",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Attribute Up"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (attributes_list_move_up_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-down",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Attribute Down"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (attributes_list_move_down_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (_("Attribute data"));
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);
  gtk_widget_show (vbox2);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 0);
  gtk_widget_show (grid);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->attr_name = GTK_ENTRY (entry);
  g_signal_connect (G_OBJECT (entry),
                    "changed",
                    G_CALLBACK (name_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_widget_show (label);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->attr_type = GTK_ENTRY (entry);
  g_signal_connect (G_OBJECT (entry),
                    "changed",
                    G_CALLBACK (type_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1 ,1, 1);
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_widget_show (label);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Value:"));
  entry = gtk_entry_new ();
  prop_dialog->attr_value = GTK_ENTRY (entry);
  g_signal_connect (G_OBJECT (entry),
                    "changed",
                    G_CALLBACK (value_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 2, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 2, 1, 1);
  gtk_widget_set_hexpand(entry, TRUE);
  gtk_widget_show (label);
  gtk_widget_show (entry);

  label = gtk_label_new (_("Comment:"));
  scrolledwindow = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow),
                                       GTK_SHADOW_IN);

  prop_dialog->attr_comment_buffer = gtk_text_buffer_new (NULL);
  entry = gtk_text_view_new_with_buffer (prop_dialog->attr_comment_buffer);
  prop_dialog->attr_comment = GTK_TEXT_VIEW (entry);
  g_signal_connect (prop_dialog->attr_comment_buffer,
                    "changed",
                    G_CALLBACK (comment_changed),
                    umlclass);
  gtk_container_add (GTK_CONTAINER (scrolledwindow), entry);
  gtk_text_view_set_wrap_mode (GTK_TEXT_VIEW (entry), GTK_WRAP_WORD);
  gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (entry), TRUE);
  gtk_widget_show (entry);
#if 0 /* while the GtkEntry has a "activate" signal, GtkTextView does not.
       * Maybe we should connect to "set-focus-child" instead?
       */
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK (attributes_update), umlclass);
#endif
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 1, 3, 1, 1);
  gtk_widget_set_hexpand (scrolledwindow, TRUE);
  gtk_widget_show (label);
  gtk_widget_show (scrolledwindow);


  label = gtk_label_new (_("Visibility:"));
  prop_dialog->attr_visible = omenu = dia_option_menu_new ();
  g_signal_connect (G_OBJECT (omenu),
                    "changed",
                    G_CALLBACK (visibility_changed),
                    umlclass);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Public"), DIA_UML_PUBLIC);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Private"), DIA_UML_PRIVATE);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Protected"), DIA_UML_PROTECTED);
  dia_option_menu_add_item (DIA_OPTION_MENU (omenu), _("Implementation"), DIA_UML_IMPLEMENTATION);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 4, 1, 1);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_widget_show (label);
  gtk_widget_show (omenu);

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (hbox2), omenu, FALSE, TRUE, 0);
  gtk_grid_attach (GTK_GRID (grid), hbox2, 1, 4, 1, 1);
  gtk_widget_show (hbox2);

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  checkbox = gtk_check_button_new_with_label (_("Class scope"));
  prop_dialog->attr_class_scope = GTK_TOGGLE_BUTTON (checkbox);
  g_signal_connect (checkbox,
                    "toggled",
                    G_CALLBACK (scope_changed),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox2, FALSE, TRUE, 0);
  gtk_widget_show (hbox2);
  gtk_widget_show (checkbox);

  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}
