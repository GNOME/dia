/* xxxxxx -- an diagram creation/manipulation program
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

extern void file_quit_callback(GtkWidget *widget, gpointer data);
extern void file_open_callback(GtkWidget *widget, gpointer data);
extern void file_save_callback(GtkWidget *widget, gpointer data);
extern void file_save_as_callback(GtkWidget *widget, gpointer data);
extern void file_export_to_eps_callback(GtkWidget *widget, gpointer data);
extern void file_close_callback(GtkWidget *widget, gpointer data);
extern void file_new_callback(GtkWidget *widget, gpointer data);

void edit_copy_callback(GtkWidget *widget, gpointer data);
void edit_cut_callback(GtkWidget *widget, gpointer data);
void edit_paste_callback(GtkWidget *widget, gpointer data);
void edit_delete_callback(GtkWidget *widget, gpointer data);

extern void view_zoom_in_callback(GtkWidget *widget, gpointer data);
extern void view_zoom_out_callback(GtkWidget *widget, gpointer data);
extern void view_zoom_set_callback(GtkWidget *widget, gpointer data);
extern void view_visible_grid_callback(gpointer  callback_data,
				       guint callback_action,
				       GtkWidget *widget);
extern void view_snap_to_grid_callback(gpointer  callback_data,
				       guint callback_action,
				       GtkWidget *widget);
extern void view_toggle_rulers_callback(gpointer  callback_data,
				       guint callback_action,
				       GtkWidget *widget);
extern void view_new_view_callback(GtkWidget *widget, gpointer data);
extern void view_show_all_callback(GtkWidget *widget, gpointer data);
extern void view_edit_grid_callback(GtkWidget *widget, gpointer data);

extern void objects_place_over_callback(GtkWidget *widget, gpointer data);
extern void objects_place_under_callback(GtkWidget *widget, gpointer data);
extern void objects_group_callback(GtkWidget *widget, gpointer data);
extern void objects_ungroup_callback(GtkWidget *widget, gpointer data);


#endif /* COMMANDS_H */
