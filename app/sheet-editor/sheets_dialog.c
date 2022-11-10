/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets_dialog.c : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
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
#include <gdk/gdkkeysyms.h>

#include "sheets.h"
#include "sheets_dialog_callbacks.h"
#include "sheets_dialog.h"

#include "persistence.h"
#include "dia-builder.h"

/* TODO avoid putting this into a global variable */
static GtkListStore *store;

static void
sheets_dialog_destroyed (GtkWidget *widget, gpointer user_data)
{
  GObject *builder = g_object_get_data (G_OBJECT (widget), "_sheet_dialogs_builder");

  g_clear_object (&builder);

  g_object_set_data (G_OBJECT (widget), "_sheet_dialogs_builder", NULL);
}


GtkWidget *
create_sheets_main_dialog (void)
{
  GtkWidget *sheets_main_dialog;
  GtkWidget *combo_left, *combo_right;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-main-dialog.ui");

  dia_builder_get (builder,
                   "sheets_main_dialog", &sheets_main_dialog,
                   "combo_left", &combo_left,
                   "combo_right", &combo_right,
                   NULL);

  g_object_set_data (G_OBJECT (sheets_main_dialog), "_sheet_dialogs_builder", builder);

  store = gtk_list_store_new (SO_N_COL,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_POINTER);

  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
                                        SO_COL_NAME,
                                        GTK_SORT_ASCENDING);

  populate_store (store);

  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_left), GTK_TREE_MODEL (store));
  gtk_combo_box_set_model (GTK_COMBO_BOX (combo_right), GTK_TREE_MODEL (store));

  dia_builder_connect (builder,
                       GTK_TREE_MODEL (store),
                       "sheets_dialog_destroyed", G_CALLBACK (sheets_dialog_destroyed),
                       "on_sheets_dialog_combo_changed", G_CALLBACK (on_sheets_dialog_combo_changed),
                       "on_sheets_main_dialog_delete_event", G_CALLBACK (on_sheets_main_dialog_delete_event),
                       "on_sheets_dialog_button_copy_clicked", G_CALLBACK (on_sheets_dialog_button_copy_clicked),
                       "on_sheets_dialog_button_copy_all_clicked", G_CALLBACK (on_sheets_dialog_button_copy_all_clicked),
                       "on_sheets_dialog_button_move_clicked", G_CALLBACK (on_sheets_dialog_button_move_clicked),
                       "on_sheets_dialog_button_move_all_clicked", G_CALLBACK (on_sheets_dialog_button_move_all_clicked),
                       "on_sheets_dialog_button_new_clicked", G_CALLBACK (on_sheets_dialog_button_new_clicked),
                       "on_sheets_dialog_button_move_up_clicked", G_CALLBACK (on_sheets_dialog_button_move_up_clicked),
                       "on_sheets_dialog_button_move_down_clicked", G_CALLBACK (on_sheets_dialog_button_move_down_clicked),
                       "on_sheets_dialog_button_edit_clicked", G_CALLBACK (on_sheets_dialog_button_edit_clicked),
                       "on_sheets_dialog_button_remove_clicked", G_CALLBACK (on_sheets_dialog_button_remove_clicked),
                       "on_sheets_dialog_button_apply_clicked", G_CALLBACK (on_sheets_dialog_button_apply_clicked),
                       "on_sheets_dialog_button_revert_clicked", G_CALLBACK (on_sheets_dialog_button_revert_clicked),
                       "on_sheets_dialog_button_close_clicked", G_CALLBACK (on_sheets_dialog_button_close_clicked),
                       NULL);

  persistence_register_window (GTK_WINDOW (sheets_main_dialog));

  return sheets_main_dialog;
}


GtkWidget *
create_sheets_new_dialog (void)
{
  GtkWidget *sheets_new_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-new-dialog.ui");
  sheets_new_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                          "sheets_new_dialog"));
  g_object_set_data (G_OBJECT (sheets_new_dialog), "_sheet_dialogs_builder", builder);

  g_signal_connect (G_OBJECT (sheets_new_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_svg_shape"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_svg_shape_toggled),
                    sheets_new_dialog);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_sheet"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_sheet_toggled),
                    sheets_new_dialog);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_line_break"),
                    "toggled",
                    G_CALLBACK (on_sheets_new_dialog_radiobutton_line_break_toggled),
                    sheets_new_dialog);
  g_signal_connect (sheets_new_dialog,
                    "response",
                    G_CALLBACK (on_sheets_new_dialog_response),
                    store);

  return sheets_new_dialog;
}


GtkWidget*
create_sheets_edit_dialog (void)
{
  GtkWidget *sheets_edit_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-edit-dialog.ui");
  sheets_edit_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                           "sheets_edit_dialog"));
  g_object_set_data (G_OBJECT (sheets_edit_dialog),
                     "_sheet_dialogs_builder",
                     builder);

  g_signal_connect (G_OBJECT (sheets_edit_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_object_description"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_object_description_changed),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_sheet_description"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_sheet_description_changed),
                    NULL);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "entry_sheet_name"),
                    "changed",
                    G_CALLBACK (on_sheets_edit_dialog_entry_sheet_name_changed),
                    NULL);
  g_signal_connect (sheets_edit_dialog,
                    "response",
                    G_CALLBACK (on_sheets_edit_dialog_response),
                    NULL);

  return sheets_edit_dialog;
}


GtkWidget*
create_sheets_remove_dialog (void)
{
  GtkWidget *sheets_remove_dialog;
  DiaBuilder *builder;

  builder = dia_builder_new ("ui/sheets-remove-dialog.ui");
  sheets_remove_dialog = GTK_WIDGET (gtk_builder_get_object (GTK_BUILDER (builder),
                                                             "sheets_remove_dialog"));
  g_object_set_data (G_OBJECT (sheets_remove_dialog),
                     "_sheet_dialogs_builder",
                     builder);

  g_signal_connect (G_OBJECT (sheets_remove_dialog), "destroy",
                    G_CALLBACK (sheets_dialog_destroyed), NULL);

  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_object"),
                    "toggled",
                    G_CALLBACK (on_sheets_remove_dialog_radiobutton_object_toggled),
                    sheets_remove_dialog);
  g_signal_connect (gtk_builder_get_object (GTK_BUILDER (builder),
                                            "radiobutton_sheet"),
                    "toggled",
                    G_CALLBACK (on_sheets_remove_dialog_radiobutton_sheet_toggled),
                    sheets_remove_dialog);
  g_signal_connect (sheets_remove_dialog,
                    "response",
                    G_CALLBACK (on_sheets_remove_dialog_response),
                    store);
  /* FIXME:
  gtk_widget_grab_default (button_ok);
  */
  return sheets_remove_dialog;
}
