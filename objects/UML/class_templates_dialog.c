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
 ******************** TEMPLATES *****************************
 ************************************************************/

enum {
  COL_FORMAL_TITLE,
  COL_FORMAL_PARAM,
  N_COLS
};


static gboolean
get_current_formal_param (UMLClassDialog      *dialog,
                          UMLFormalParameter **param,
                          GtkTreeIter         *c_iter)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  GtkTreeSelection *selection;

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (dialog->templates));
  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_FORMAL_PARAM, param, -1);

    if (c_iter) {
      *c_iter = iter;
    }

    return TRUE;
  }

  return FALSE;
}


static void
update_formal_param (UMLClassDialog     *dialog,
                     UMLFormalParameter *param,
                     GtkTreeIter        *iter)
{
  char *title;

  title = uml_formal_parameter_get_string (param);

  gtk_list_store_set (dialog->templates_store,
                      iter,
                      COL_FORMAL_PARAM, param,
                      COL_FORMAL_TITLE, title,
                      -1);

  g_clear_pointer (&title, g_free);
}


static void
templates_set_sensitive (UMLClassDialog *prop_dialog, gboolean val)
{
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->templ_name), val);
  gtk_widget_set_sensitive (GTK_WIDGET (prop_dialog->templ_type), val);
}


static void
templates_set_values (UMLClassDialog     *prop_dialog,
                      UMLFormalParameter *param)
{
  gtk_entry_set_text (prop_dialog->templ_name, param->name? param->name : "");
  gtk_entry_set_text (prop_dialog->templ_type, param->type? param->type : "");
}


static void
templates_clear_values (UMLClassDialog *prop_dialog)
{
  gtk_entry_set_text (prop_dialog->templ_name, "");
  gtk_entry_set_text (prop_dialog->templ_type, "");
}


static void
templates_list_new_callback (GtkWidget *button,
                             UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param;
  GtkTreeIter iter;
  GtkTreeSelection *selection;

  prop_dialog = umlclass->properties_dialog;

  param = uml_formal_parameter_new ();

  gtk_list_store_append (prop_dialog->templates_store, &iter);
  update_formal_param (prop_dialog, param, &iter);

  selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->templates));
  gtk_tree_selection_select_iter (selection, &iter);

  g_clear_pointer (&param, uml_formal_parameter_unref);
}


static void
templates_list_delete_callback (GtkWidget *button,
                                UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_formal_param (prop_dialog, &param, &iter)) {
    gtk_list_store_remove (prop_dialog->templates_store, &iter);

    g_clear_pointer (&param, uml_formal_parameter_unref);
  }
}


static void
templates_list_move_up_callback (GtkWidget *button,
                                 UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *current_param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_formal_param (prop_dialog, &current_param, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->templates_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL && gtk_tree_path_prev (path)
        && gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->templates_store), &prev, path)) {
      gtk_list_store_move_before (prop_dialog->templates_store, &iter, &prev);
    } else {
      gtk_list_store_move_before (prop_dialog->templates_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->templates));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_param, uml_formal_parameter_unref);
  }
}


static void
templates_list_move_down_callback (GtkWidget *button,
                                   UMLClass  *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *current_param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_formal_param (prop_dialog, &current_param, &iter)) {
    GtkTreePath *path = gtk_tree_model_get_path (GTK_TREE_MODEL (prop_dialog->templates_store),
                                                 &iter);
    GtkTreeSelection *selection;
    GtkTreeIter prev;

    if (path != NULL) {
      gtk_tree_path_next (path);
      if (gtk_tree_model_get_iter (GTK_TREE_MODEL (prop_dialog->templates_store), &prev, path)) {
        gtk_list_store_move_after (prop_dialog->templates_store, &iter, &prev);
      } else {
        gtk_list_store_move_after (prop_dialog->templates_store, &iter, NULL);
      }
    } else {
      gtk_list_store_move_after (prop_dialog->templates_store, &iter, NULL);
    }
    gtk_tree_path_free (path);

    selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->templates));
    gtk_tree_selection_select_iter (selection, &iter);

    g_clear_pointer (&current_param, uml_formal_parameter_unref);
  }
}


static void
name_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_formal_param (prop_dialog, &param, &iter)) {
    g_clear_pointer (&param->name, g_free);
    param->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_formal_param (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_formal_parameter_unref);
  }
}


static void
type_changed (GtkWidget *entry, UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param;
  GtkTreeIter iter;

  prop_dialog = umlclass->properties_dialog;

  if (get_current_formal_param (prop_dialog, &param, &iter)) {
    g_clear_pointer (&param->type, g_free);
    param->type = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));

    update_formal_param (prop_dialog, param, &iter);

    g_clear_pointer (&param, uml_formal_parameter_unref);
  }
}


struct AddTmplData {
  UMLClass       *class;
  UMLClassDialog *dialog;
};


static gboolean
add_formal_param_to_list (GtkTreeModel *model,
                          GtkTreePath  *path,
                          GtkTreeIter  *iter,
                          gpointer      udata)
{
  struct AddTmplData *data = udata;
  UMLFormalParameter *param = NULL;

  // Don't free param, transfering to the list
  gtk_tree_model_get (model, iter, COL_FORMAL_PARAM, &param, -1);

  data->class->formal_params = g_list_append (data->class->formal_params, param);

  return FALSE;
}


void
_templates_read_from_dialog (UMLClass *umlclass, UMLClassDialog *prop_dialog)
{
  struct AddTmplData data;

  umlclass->template = gtk_toggle_button_get_active (prop_dialog->templ_template);

  /* Free current formal parameters: */
  g_list_free_full (umlclass->formal_params, (GDestroyNotify) uml_formal_parameter_unref);
  umlclass->formal_params = NULL;

  data.class = umlclass;
  data.dialog = prop_dialog;

  gtk_tree_model_foreach (GTK_TREE_MODEL (prop_dialog->templates_store),
                          add_formal_param_to_list,
                          &data);
  gtk_list_store_clear (prop_dialog->templates_store);
}


void
_templates_fill_in_dialog(UMLClass *umlclass)
{
  UMLClassDialog *prop_dialog;
  UMLFormalParameter *param_copy;
  GtkTreeIter iter;
  GList *list;

  prop_dialog = umlclass->properties_dialog;

  gtk_toggle_button_set_active (prop_dialog->templ_template, umlclass->template);

  gtk_list_store_clear (prop_dialog->templates_store);

  /* copy in new template-parameters: */
  list = umlclass->formal_params;
  while (list != NULL) {
    UMLFormalParameter *param = (UMLFormalParameter *) list->data;

    param_copy = uml_formal_parameter_copy (param);

    gtk_list_store_append (prop_dialog->templates_store, &iter);
    update_formal_param (prop_dialog, param_copy, &iter);

    list = g_list_next(list);

    g_clear_pointer (&param_copy, uml_formal_parameter_unref);
  }

  /* set templates non-sensitive */
  templates_set_sensitive (prop_dialog, FALSE);
  templates_clear_values (prop_dialog);
}


static void
formal_param_selected (GtkTreeSelection *selection,
                       UMLClass         *umlclass)
{
  GtkTreeIter iter;
  GtkTreeModel *model;
  UMLFormalParameter *formal_param;
  UMLClassDialog *prop_dialog;

  prop_dialog = umlclass->properties_dialog;

  if (!prop_dialog) {
    return; /* maybe hiding a bug elsewhere */
  }

  if (gtk_tree_selection_get_selected (selection, &model, &iter)) {
    gtk_tree_model_get (model, &iter, COL_FORMAL_PARAM, &formal_param, -1);

    templates_set_values (prop_dialog, formal_param);
    templates_set_sensitive (prop_dialog, TRUE);

    g_clear_pointer (&formal_param, uml_formal_parameter_unref);

    gtk_widget_grab_focus (GTK_WIDGET (prop_dialog->templ_name));
  } else {
    templates_set_sensitive (prop_dialog, FALSE);
    templates_clear_values (prop_dialog);
  }
}


void
_templates_create_page (GtkNotebook *notebook, UMLClass *umlclass)
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
  GtkWidget *image;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;
  GtkTreeSelection *select;

  prop_dialog = umlclass->properties_dialog;

  /* Templates page: */
  page_label = gtk_label_new_with_mnemonic (_("_Templates"));

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);

  hbox2 = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
  checkbox = gtk_check_button_new_with_label (_("Template class"));
  prop_dialog->templ_template = GTK_TOGGLE_BUTTON (checkbox);
  gtk_box_pack_start (GTK_BOX (hbox2), checkbox, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox2, FALSE, TRUE, 0);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);


  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (hbox), scrolled_win, TRUE, TRUE, 0);
  gtk_widget_show (scrolled_win);

  prop_dialog->templates_store = gtk_list_store_new (N_COLS,
                                                     G_TYPE_STRING,
                                                     DIA_UML_TYPE_FORMAL_PARAMETER);
  prop_dialog->templates = gtk_tree_view_new_with_model (GTK_TREE_MODEL (prop_dialog->templates_store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prop_dialog->templates), FALSE);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (prop_dialog->templates),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  select = gtk_tree_view_get_selection (GTK_TREE_VIEW (prop_dialog->templates));
  g_signal_connect (G_OBJECT (select),
                    "changed",
                    G_CALLBACK (formal_param_selected),
                    umlclass);
  gtk_tree_selection_set_mode (select, GTK_SELECTION_SINGLE);

  renderer = gtk_cell_renderer_text_new ();
  g_object_set (renderer, "family", "monospace", NULL);
  column = gtk_tree_view_column_new_with_attributes (NULL,
                                                     renderer,
                                                     "text",
                                                     COL_FORMAL_TITLE,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prop_dialog->templates),
                               column);

  gtk_container_add (GTK_CONTAINER (scrolled_win), prop_dialog->templates);
  gtk_widget_show (prop_dialog->templates);


  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-add",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Add Formal Parameter"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (templates_list_new_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-remove",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Remove Formal Parameter"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (templates_list_delete_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-up",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Formal Parameter Up"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (templates_list_move_up_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);

  button = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("go-down",
                                        GTK_ICON_SIZE_SMALL_TOOLBAR);
  gtk_widget_show (image);
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_set_tooltip_text (button, _("Move Formal Parameter Down"));
  g_signal_connect (G_OBJECT (button),
                    "clicked",
                    G_CALLBACK (templates_list_move_down_callback),
                    umlclass);
  gtk_box_pack_start (GTK_BOX (vbox2), button, FALSE, FALSE, 0);
  gtk_widget_show (button);


  gtk_box_pack_start (GTK_BOX (hbox), vbox2, FALSE, TRUE, 0);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

  frame = gtk_frame_new (_("Formal parameter data"));
  vbox2 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 5);
  gtk_container_set_border_width (GTK_CONTAINER (vbox2), 6);
  gtk_container_add (GTK_CONTAINER (frame), vbox2);
  gtk_widget_show (frame);
  gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, TRUE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 6);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (vbox2), grid, FALSE, FALSE, 0);

  label = gtk_label_new (_("Name:"));
  entry = gtk_entry_new ();
  prop_dialog->templ_name = GTK_ENTRY (entry);
  g_signal_connect (G_OBJECT (entry),
                    "changed",
                    G_CALLBACK (name_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 0, 1, 1);
  gtk_widget_set_hexpand (entry, TRUE);

  label = gtk_label_new (_("Type:"));
  entry = gtk_entry_new ();
  prop_dialog->templ_type = GTK_ENTRY (entry);
  g_signal_connect (G_OBJECT (entry),
                    "changed",
                    G_CALLBACK (type_changed),
                    umlclass);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_grid_attach (GTK_GRID (grid), label, 0, 1, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), entry, 1, 1, 1, 1);
  gtk_widget_set_hexpand (entry, TRUE);

  gtk_widget_show (vbox2);

  /* TODO: Add stuff here! */

  gtk_widget_show_all (vbox);
  gtk_widget_show (page_label);
  gtk_notebook_append_page (notebook, vbox, page_label);
}
