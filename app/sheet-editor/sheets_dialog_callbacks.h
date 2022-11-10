/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets_dialog_callbacks.c : a sheets and objects dialog
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

#include <gtk/gtk.h>

gboolean    on_sheets_main_dialog_delete_event                     (GtkWidget       *widget,
                                                                    GdkEvent        *event,
                                                                    gpointer         user_data);
void        on_sheets_dialog_combo_changed                         (GtkComboBox     *widget,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_move_up_clicked                (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_new_clicked                    (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_close_clicked                  (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_edit_clicked                   (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_remove_clicked                 (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_new_dialog_radiobutton_svg_shape_toggled     (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_new_dialog_radiobutton_line_break_toggled    (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_new_dialog_radiobutton_sheet_toggled         (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_remove_dialog_radiobutton_object_toggled     (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_remove_dialog_radiobutton_sheet_toggled      (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_move_down_clicked              (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_new_dialog_response                          (GtkWidget       *sheets_new_dialog,
                                                                    int              response,
                                                                    gpointer         user_data);
void        on_sheets_new_dialog_radiobutton_line_break_toggled    (GtkToggleButton *togglebutton,
                                                                    gpointer         user_data);
void        on_sheets_remove_dialog_response                       (GtkWidget       *button,
                                                                    int response,
                                                                    gpointer         user_data);
void        on_sheets_edit_dialog_response                         (GtkWidget       *sheets_edit_dialog,
                                                                    int              response,
                                                                    gpointer         user_data);
void        on_sheets_edit_dialog_entry_object_description_changed (GtkEditable     *editable,
                                                                    gpointer         user_data);
void        on_sheets_edit_dialog_entry_sheet_description_changed  (GtkEditable     *editable,
                                                                    gpointer         user_data);
void        on_sheets_edit_dialog_entry_sheet_name_changed         (GtkEditable     *editable,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_copy_clicked                   (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_copy_all_clicked               (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_move_clicked                   (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_move_all_clicked               (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_apply_clicked                  (GtkButton       *button,
                                                                    gpointer         user_data);
void        on_sheets_dialog_button_revert_clicked                 (GtkButton       *button,
                                                                    gpointer         user_data);
GtkWidget  *sheets_dialog_get_active_button                        (GtkWidget      **wrapbox,
                                                                    GList          **button_list);
