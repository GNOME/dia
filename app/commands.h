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
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include <gtk/gtk.h>

void file_quit_callback        (GtkAction *action);
void file_pagesetup_callback   (GtkAction *action);
void file_print_callback       (GtkAction *action);
void file_close_callback       (GtkAction *action);
void file_new_callback         (GtkAction *action);
void file_preferences_callback (GtkAction *action);

void help_manual_callback (GtkAction *action);
void help_about_callback  (GtkAction *action);

void activate_url (GtkWidget *parent, const gchar *url, gpointer data);

void edit_copy_callback       (GtkAction *action);
void edit_cut_callback        (GtkAction *action);
void edit_paste_callback      (GtkAction *action);
void edit_duplicate_callback  (GtkAction *action);
void edit_delete_callback     (GtkAction *action);
void edit_undo_callback       (GtkAction *action);
void edit_redo_callback       (GtkAction *action);
void edit_paste_text_callback (GtkAction *action);
void edit_copy_text_callback  (GtkAction *action);
void edit_cut_text_callback   (GtkAction *action);

void edit_paste_image_callback (GtkAction *action);

void view_zoom_in_callback           (GtkAction *action);
void view_zoom_out_callback          (GtkAction *action);
void view_zoom_set_callback          (GtkAction *action);
void view_unfullscreen               (void);
void view_fullscreen_callback        (GtkToggleAction *action);
void view_aa_callback                (GtkToggleAction *action);
void view_visible_grid_callback      (GtkToggleAction *action);
void view_snap_to_grid_callback      (GtkToggleAction *action);
void view_new_guide_callback         (GtkAction *action);
void view_visible_guides_callback    (GtkToggleAction *action);
void view_snap_to_guides_callback    (GtkToggleAction *action);
void view_remove_all_guides_callback (GtkAction *action);
void view_snap_to_objects_callback   (GtkToggleAction *action);
void view_toggle_rulers_callback     (GtkToggleAction *action);
void view_toggle_scrollbars_callback (GtkToggleAction *action);

void view_show_cx_pts_callback     (GtkToggleAction *action);
void view_new_view_callback           (GtkAction *action);
void view_clone_view_callback         (GtkAction *action);
void view_show_all_callback           (GtkAction *action);
void view_redraw_callback             (GtkAction *action);
void view_diagram_properties_callback (GtkAction *action);

/* Integrated UI callbacks */
void view_main_toolbar_callback       (GtkAction *action);
void view_main_statusbar_callback     (GtkAction *action);
void view_layers_callback             (GtkAction *action);

void layers_add_layer_callback    (GtkAction *action);
void layers_rename_layer_callback (GtkAction *action);

void objects_place_over_callback        (GtkAction *action);
void objects_place_under_callback       (GtkAction *action);
void objects_place_up_callback          (GtkAction *action);
void objects_place_down_callback        (GtkAction *action);
void objects_move_up_layer              (GtkAction *action);
void objects_move_down_layer            (GtkAction *action);

void objects_parent_callback            (GtkAction *action);
void objects_unparent_callback          (GtkAction *action);
void objects_unparent_children_callback (GtkAction *action);
void objects_group_callback             (GtkAction *action);
void objects_ungroup_callback           (GtkAction *action);
void objects_align_h_callback           (GtkAction *action);
void objects_align_v_callback           (GtkAction *action);
void objects_align_connected_callback   (GtkAction *action);

void dialogs_properties_callback (GtkAction *action);
void dialogs_layers_callback     (GtkAction *action);

/* from sheets.c */
void sheets_dialog_show_callback(GtkAction *action);

void dia_file_open (const gchar *filename, DiaImportFilter *ifilter);

#endif /* COMMANDS_H */
