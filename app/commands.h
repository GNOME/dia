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

void file_quit_callback(gpointer data, guint action, GtkWidget *widget);
void file_pagesetup_callback(gpointer data, guint action, GtkWidget *widget);
void file_print_callback(gpointer data, guint action, GtkWidget *widget);
void file_close_callback(gpointer data, guint action, GtkWidget *widget);
void file_new_callback(gpointer data, guint action, GtkWidget *widget);
void file_preferences_callback(gpointer data, guint action, GtkWidget *widget);

void help_manual_callback(gpointer data, guint action, GtkWidget *widget);
void help_about_callback(gpointer data, guint action, GtkWidget *widget);

void edit_copy_callback(gpointer data, guint action, GtkWidget *widget);
void edit_cut_callback(gpointer data, guint action, GtkWidget *widget);
void edit_paste_callback(gpointer data, guint action, GtkWidget *widget);
void edit_duplicate_callback(gpointer data, guint action, GtkWidget *widget);
void edit_delete_callback(gpointer data, guint action, GtkWidget *widget);
void edit_undo_callback(gpointer data, guint action, GtkWidget *widget);
void edit_redo_callback(gpointer data, guint action, GtkWidget *widget);
void edit_paste_text_callback(gpointer data, guint action, GtkWidget *widget);
void edit_copy_text_callback(gpointer data, guint action, GtkWidget *widget);
void edit_cut_text_callback(gpointer data, guint action, GtkWidget *widget);

void received_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
				gpointer data);
void get_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
			   gpointer data);

void view_zoom_in_callback(gpointer data, guint action, GtkWidget *widget);
void view_zoom_out_callback(gpointer data, guint action, GtkWidget *widget);

void view_zoom_set_callback(gpointer data, guint action, GtkWidget *widget);

void view_unfullscreen(void);
void view_fullscreen_callback(gpointer data, guint action, GtkWidget *widget);

void view_aa_callback(gpointer data, guint action, GtkWidget *widget);
void view_visible_grid_callback (gpointer data, guint action,
				 GtkWidget *widget);
void view_snap_to_grid_callback (gpointer data, guint action,
				 GtkWidget *widget);
void view_snap_to_objects_callback(gpointer data, guint action,
				   GtkWidget *widget);
void view_toggle_rulers_callback(gpointer data, guint action,
				 GtkWidget *widget);
void view_show_cx_pts_callback  (gpointer data, guint action,
				 GtkWidget *widget);

void view_new_view_callback(gpointer data, guint action, GtkWidget *widget);
void view_clone_view_callback(gpointer data, guint action, GtkWidget *widget);
void view_show_all_callback(gpointer data, guint action, GtkWidget *widget);
void view_redraw_callback(gpointer data, guint action, GtkWidget *widget);
void view_diagram_properties_callback(gpointer data, guint action,
				      GtkWidget *widget);

void objects_place_over_callback(gpointer data, guint action,
				 GtkWidget *widget);
void objects_place_under_callback(gpointer data, guint action,
				  GtkWidget *widget);
void objects_place_up_callback(gpointer data, guint action,
				 GtkWidget *widget);
void objects_place_down_callback(gpointer data, guint action,
				  GtkWidget *widget);
void objects_parent_callback(gpointer data, guint action, GtkWidget *widget);
void objects_unparent_callback(gpointer data, guint action, GtkWidget *widget);
void objects_unparent_children_callback(gpointer data, guint action, GtkWidget *widget);
void objects_group_callback(gpointer data, guint action, GtkWidget *widget);
void objects_ungroup_callback(gpointer data, guint action, GtkWidget *widget);

void dialogs_properties_callback(gpointer data, guint action,
				 GtkWidget *widget);
void dialogs_layers_callback(gpointer data, guint action, GtkWidget *widget);

void objects_align_h_callback(gpointer data, guint action, GtkWidget *widget);
void objects_align_v_callback(gpointer data, guint action, GtkWidget *widget);

#endif /* COMMANDS_H */
