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

void file_quit_callback(GtkWidget *widget, gpointer data);
void file_import_from_xfig_callback(GtkWidget *widget, gpointer data);
void file_pagesetup_callback(GtkWidget *widget, gpointer data);
void file_print_callback(GtkWidget *widget, gpointer data);
void file_close_callback(GtkWidget *widget, gpointer data);
void file_new_callback(GtkWidget *widget, gpointer data);
void file_preferences_callback(GtkWidget *widget, gpointer data);

void help_about_callback(GtkWidget *widget, gpointer data);

void edit_copy_callback(GtkWidget *widget, gpointer data);
void edit_cut_callback(GtkWidget *widget, gpointer data);
void edit_paste_callback(GtkWidget *widget, gpointer data);
void edit_delete_callback(GtkWidget *widget, gpointer data);
void edit_undo_callback(GtkWidget *widget, gpointer data);
void edit_redo_callback(GtkWidget *widget, gpointer data);
void edit_paste_text_callback(GtkWidget *widget, gpointer data);
void edit_copy_text_callback(GtkWidget *widget, gpointer data);
void edit_cut_text_callback(GtkWidget *widget, gpointer data);

void received_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
				gpointer data);
void get_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
			   gpointer data);

void view_zoom_in_callback(GtkWidget *widget, gpointer data);
void view_zoom_out_callback(GtkWidget *widget, gpointer data);

#ifdef GNOME
void view_zoom_set_callback(GtkWidget *widget, gpointer data);
#else
void view_zoom_set_callback(GtkWidget *widget, gpointer data, guint action);
#endif

void view_aa_callback(GtkWidget *widget,
		      gpointer  callback_data);
void view_visible_grid_callback (GtkWidget *widget,
				 gpointer  callback_data);
void view_snap_to_grid_callback (GtkWidget *widget,
				 gpointer  callback_data);
void view_toggle_rulers_callback(GtkWidget *widget,
				 gpointer  callback_data);
void view_show_cx_pts_callback  (GtkWidget *widget,
				 gpointer  callback_data);

void view_new_view_callback(GtkWidget *widget, gpointer data);
void view_show_all_callback(GtkWidget *widget, gpointer data);
void view_diagram_properties_callback(GtkWidget *widget, gpointer data);

void objects_place_over_callback(GtkWidget *widget, gpointer data);
void objects_place_under_callback(GtkWidget *widget, gpointer data);
void objects_group_callback(GtkWidget *widget, gpointer data);
void objects_ungroup_callback(GtkWidget *widget, gpointer data);

void dialogs_properties_callback(GtkWidget *widget, gpointer data);
void dialogs_layers_callback(GtkWidget *widget, gpointer data);

#ifdef GNOME
void objects_align_h_callback(GtkWidget *widget, gpointer data);
void objects_align_v_callback(GtkWidget *widget, gpointer data);
#else
void objects_align_h_callback(GtkWidget *widget, gpointer data, guint action);
void objects_align_v_callback(GtkWidget *widget, gpointer data, guint action);
#endif

#endif /* COMMANDS_H */
