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
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <math.h>

#include "config.h"
#ifdef GNOME
#include <gnome.h>
#endif
#include "intl.h"
#include "commands.h"
#include "app_procs.h"
#include "diagram.h"
#include "display.h"
#include "object_ops.h"
#include "cut_n_paste.h"
#include "load_save.h"
#include "utils.h"
#include "message.h"
#include "grid.h"
#include "properties.h"
#include "layer_dialog.h"
#include "connectionpoint_ops.h"

void file_quit_callback(GtkWidget *widget, gpointer data)
{
  app_exit();
}

static void
file_open_dialog_ok_callback (GtkWidget        *w,
			      GtkFileSelection *fs)
{
  char *filename;
  Diagram *diagram;
  DDisplay *ddisp;
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));
  diagram = diagram_load(filename);

  if (diagram != NULL) {
    diagram_update_extents(diagram);
  
    ddisp = new_display(diagram);
    diagram_add_ddisplay(diagram, ddisp);
  }
  /* Error messages are done in diagram_load() */

  gtk_widget_destroy (GTK_WIDGET (fs));
}

void file_open_callback(GtkWidget *widget, gpointer data)
{
  GtkWidget *window = NULL;

  window = gtk_file_selection_new (_("Open diagram"));

  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);

  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(file_open_dialog_ok_callback),
		      window);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT (window));
  
  gtk_widget_show (window);
}

void
file_save_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();

  if (ddisp->diagram->unsaved) {
    file_save_as_callback(widget, data);
  } else {
    diagram_save(ddisp->diagram, ddisp->diagram->filename);
  }
}

static void
set_true_callback(GtkWidget *w, int *data)
{
  *data = TRUE;
}

static void
file_save_as_dialog_ok_callback (GtkWidget        *w,
				 GtkFileSelection *fs)
{
  char *filename;
  Diagram *dia;
  struct stat stat_struct;
  
  dia = gtk_object_get_user_data(GTK_OBJECT(fs));
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  if (stat(filename, &stat_struct)==0) {
    GtkWidget *dialog = NULL;
    GtkWidget *button, *label;
    char buffer[300];
    int result;

    dialog = gtk_dialog_new();
  
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
			GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), _("File already exists"));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 0);
    snprintf(buffer, 300,
	     _("The file '%s' already exists.\n"
	     "Do you want to overwrite it?"), filename);
    label = gtk_label_new (buffer);
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

    result = FALSE;
    
    button = gtk_button_new_with_label (_("Yes"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(set_true_callback),
			&result);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    gtk_widget_show (button);
    
    button = gtk_button_new_with_label (_("No"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, TRUE, TRUE, 0);
    
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    
    gtk_widget_show (button);

    gtk_widget_show (dialog);

    /* Make dialog modal: */
    gtk_widget_grab_focus(dialog);
    gtk_grab_add(dialog);

    gtk_main();

    if (result==FALSE) {
      gtk_grab_remove(GTK_WIDGET(fs));
      gtk_widget_destroy(GTK_WIDGET(fs));
      return;
    }
  }

  diagram_set_filename(dia, filename);
  diagram_save(dia, filename);
  
  gtk_grab_remove(GTK_WIDGET(fs));
  gtk_widget_destroy(GTK_WIDGET(fs));
}

void
file_save_as_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  GtkWidget *window = NULL;

  ddisp = ddisplay_active();

  window = gtk_file_selection_new (_("Save diagram"));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  
  gtk_object_set_user_data(GTK_OBJECT(window), ddisp->diagram);
  
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(file_save_as_dialog_ok_callback),
		      window);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT (window));
  
  gtk_widget_show (window);

  /* Make dialog modal: */
  gtk_widget_grab_focus(window);
  gtk_grab_add(window);
}

static void
file_export_to_eps_dialog_ok_callback (GtkWidget        *w,
				       GtkFileSelection *fs)
{
  char *filename;
  Diagram *dia;
  struct stat stat_struct;

  dia = gtk_object_get_user_data(GTK_OBJECT(fs));
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  if (stat(filename, &stat_struct)==0) {
    GtkWidget *dialog = NULL;
    GtkWidget *button, *label;
    char buffer[300];
    int result;

    dialog = gtk_dialog_new();
  
    gtk_signal_connect (GTK_OBJECT (dialog), "destroy", 
					 GTK_SIGNAL_FUNC(gtk_main_quit), NULL);
    
    gtk_window_set_title (GTK_WINDOW (dialog), _("File already exists"));
    gtk_container_set_border_width (GTK_CONTAINER (dialog), 0);
    snprintf(buffer, 300,
	     _("The file '%s' already exists.\n"
	     "Do you want to overwrite it?"), filename);
    label = gtk_label_new (buffer);
  
    gtk_misc_set_padding (GTK_MISC (label), 10, 10);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), 
			label, TRUE, TRUE, 0);
  
    gtk_widget_show (label);

    result = FALSE;
    
    button = gtk_button_new_with_label (_("Yes"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
			button, TRUE, TRUE, 0);
    gtk_widget_grab_default (button);
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			GTK_SIGNAL_FUNC(set_true_callback),
			&result);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    gtk_widget_show (button);
    
    button = gtk_button_new_with_label (_("No"));
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area),
			button, TRUE, TRUE, 0);
    
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       GTK_SIGNAL_FUNC (gtk_widget_destroy),
			       GTK_OBJECT (dialog));
    
    gtk_widget_show (button);

    gtk_widget_show (dialog);

    /* Make dialog modal: */
    gtk_widget_grab_focus(dialog);
    gtk_grab_add(dialog);

    gtk_main();

    if (result==FALSE) {
      gtk_grab_remove(GTK_WIDGET(fs));
      gtk_widget_destroy(GTK_WIDGET(fs));
      return;
    }
  }
  
  diagram_export_to_eps(dia, filename);
  
  gtk_grab_remove(GTK_WIDGET(fs));
  gtk_widget_destroy(GTK_WIDGET(fs));
}

void
file_export_to_eps_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  GtkWidget *window = NULL;

  ddisp = ddisplay_active();

  window = gtk_file_selection_new (_("Export to postscript"));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  
  gtk_object_set_user_data(GTK_OBJECT(window), ddisp->diagram);
  
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(file_export_to_eps_dialog_ok_callback),
		      window);
  
  gtk_signal_connect_object (GTK_OBJECT (GTK_FILE_SELECTION (window)->cancel_button),
			     "clicked", GTK_SIGNAL_FUNC(gtk_widget_destroy),
			     GTK_OBJECT (window));
  
  gtk_widget_show (window);

  /* Make dialog modal: */
  gtk_widget_grab_focus(window);
  gtk_grab_add(window);
}

void
file_close_callback(GtkWidget *widget, gpointer data)
{
  ddisplay_close(ddisplay_active());
} 

void
file_new_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia;
  DDisplay *ddisp;
  static int untitled_nr = 1;
  char buffer[24];

  snprintf(buffer, 24, _("Untitled-%d"), untitled_nr++);
  
  dia = new_diagram(buffer);
  ddisp = new_display(dia);
  diagram_add_ddisplay(dia, ddisp);
}

void
edit_copy_callback(GtkWidget *widget, gpointer data)
{
  GList *copy_list;
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  copy_list = diagram_get_sorted_selected(ddisp->diagram);

  cnp_store_objects(object_copy_list(copy_list));
}

void
edit_cut_callback(GtkWidget *widget, gpointer data)
{
  GList *cut_list;
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  cut_list = diagram_get_sorted_selected_remove(ddisp->diagram);
  diagram_remove_all_selected(ddisp->diagram, FALSE);

  object_add_updates_list(cut_list, ddisp->diagram);

  cnp_store_objects(object_copy_list(cut_list));

  destroy_object_list(cut_list); /* Have to destroy it so that any attribut
				    dialogs open are closed. */
  
  diagram_flush(ddisp->diagram);
}

void
edit_paste_callback(GtkWidget *widget, gpointer data)
{
  GList *paste_list;
  DDisplay *ddisp;
  Point paste_corner;
  Point delta;
  
  ddisp = ddisplay_active();

  if (!cnp_exist_stored_objects()) {
    message_warning(_("No existing object to paste.\n"));
    return;
  }
  
  paste_list = cnp_get_stored_objects();


  paste_corner = object_list_corner(paste_list);
  
  delta.x = ddisp->visible.left - paste_corner.x;
  delta.y = ddisp->visible.top - paste_corner.y;

  /* Move down some 10% of the visible area. */
  delta.x += (ddisp->visible.right - ddisp->visible.left)*0.1;
  delta.y += (ddisp->visible.bottom - ddisp->visible.top)*0.1;

  object_list_move_delta(paste_list, &delta);

  paste_corner = object_list_corner(paste_list);

  diagram_remove_all_selected(ddisp->diagram, TRUE);
  diagram_add_selected_list(ddisp->diagram, paste_list);
  diagram_add_object_list(ddisp->diagram, paste_list);
  object_add_updates_list(paste_list, ddisp->diagram);

  diagram_flush(ddisp->diagram);
}

void
edit_delete_callback(GtkWidget *widget, gpointer data)
{
  GList *delete_list;
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  delete_list = diagram_get_sorted_selected_remove(ddisp->diagram);
  diagram_remove_all_selected(ddisp->diagram, FALSE);

  object_add_updates_list(delete_list, ddisp->diagram);
  destroy_object_list(delete_list);

  diagram_flush(ddisp->diagram);
} 

void
help_about_callback(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;
  
#ifdef GNOME
  const gchar *authors[] = { "Alexander Larsson", NULL };

  GtkWidget *about = 
    gnome_about_new (PACKAGE, VERSION,
		     _("Copyright (C) 1999"),
		     authors,
		     _("Please visit http://www.lysator.liu.se/~alla/dia "
		       "for more info"),
		     NULL);
  gtk_widget_show(about);
#else
  dialog = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "about_dialog", "Dia");
  gtk_window_set_title (GTK_WINDOW (dialog), _("About Dia"));
  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
  gtk_window_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), 
		      GTK_OBJECT (dialog));

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 5);
  gtk_container_add (GTK_CONTAINER (GTK_DIALOG (dialog)->vbox), frame);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  label = gtk_label_new ("Dia v" VERSION " by Alexander Larsson");
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  label = gtk_label_new (_("Please visit http://www.lysator.liu.se/~alla/dia "
			   "for more info"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  button = gtk_button_new_with_label (_("OK"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, FALSE, FALSE, 0);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(dialog));

  gtk_widget_show_all (dialog);
#endif
}

void
view_zoom_in_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Point middle;
  Rectangle *visible;

  ddisp = ddisplay_active();
  visible = &ddisp->visible;
  middle.x = visible->left*0.5 + visible->right*0.5;
  middle.y = visible->top*0.5 + visible->bottom*0.5;
  
  ddisplay_zoom(ddisp, &middle, M_SQRT2);
}

void
view_zoom_out_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Point middle;
  Rectangle *visible;
  
  ddisp = ddisplay_active();
  visible = &ddisp->visible;
  middle.x = visible->left*0.5 + visible->right*0.5;
  middle.y = visible->top*0.5 + visible->bottom*0.5;
  
  ddisplay_zoom(ddisp, &middle, M_SQRT1_2);
}

void
view_zoom_set_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  int percent;
  real scale;
  Point middle;
  Rectangle *visible;
  
  ddisp = ddisplay_active();
  visible = &ddisp->visible;
  middle.x = visible->left*0.5 + visible->right*0.5;
  middle.y = visible->top*0.5 + visible->bottom*0.5;

  percent = (int) data;
  scale = ((real) percent)/1000.0 * DDISPLAY_NORMAL_ZOOM;

  ddisplay_zoom(ddisp, &middle, scale / ddisp->zoom_factor);  
}

void
view_visible_grid_callback(gpointer callback_data,
			   guint callback_action,
			   GtkWidget *widget)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  
  old_val = ddisp->grid.visible;
  ddisp->grid.visible = GTK_CHECK_MENU_ITEM (widget)->active;

  if (old_val != ddisp->grid.visible) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_snap_to_grid_callback(gpointer callback_data,
			   guint callback_action,
			   GtkWidget *widget)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  
  old_val = ddisp->grid.snap;
  ddisp->grid.snap = GTK_CHECK_MENU_ITEM (widget)->active;
}

void view_toggle_rulers_callback(gpointer callback_data,
				 guint callback_action,
				 GtkWidget *widget)
{
  DDisplay *ddisp;
  
  ddisp = ddisplay_active();

  /* The following is borrowed straight from the Gimp: */
  
  /* This routine use promiscuous knowledge of gtk internals
   *  in order to hide and show the rulers "smoothly". This
   *  is kludgy and a hack and may break if gtk is changed
   *  internally.
   */
  if (!GTK_CHECK_MENU_ITEM (widget)->active) {
    if (GTK_WIDGET_VISIBLE (ddisp->origin)) {
      gtk_widget_unmap (ddisp->origin);
      gtk_widget_unmap (ddisp->hrule);
      gtk_widget_unmap (ddisp->vrule);
      
      GTK_WIDGET_UNSET_FLAGS (ddisp->origin, GTK_VISIBLE);
      GTK_WIDGET_UNSET_FLAGS (ddisp->hrule, GTK_VISIBLE);
      GTK_WIDGET_UNSET_FLAGS (ddisp->vrule, GTK_VISIBLE);
      
      gtk_widget_queue_resize (GTK_WIDGET (ddisp->origin->parent));
    }
  } else {
    if (!GTK_WIDGET_VISIBLE (ddisp->origin)) {
      gtk_widget_map (ddisp->origin);
      gtk_widget_map (ddisp->hrule);
      gtk_widget_map (ddisp->vrule);
      
      GTK_WIDGET_SET_FLAGS (ddisp->origin, GTK_VISIBLE);
      GTK_WIDGET_SET_FLAGS (ddisp->hrule, GTK_VISIBLE);
      GTK_WIDGET_SET_FLAGS (ddisp->vrule, GTK_VISIBLE);
      
      gtk_widget_queue_resize (GTK_WIDGET (ddisp->origin->parent));
    }
  }
}

extern void
view_new_view_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Diagram *dia;

  ddisp = ddisplay_active();
  dia = ddisp->diagram;
  
  ddisp = new_display(dia);
  diagram_add_ddisplay(dia, ddisp);
}

void
view_show_all_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Diagram *dia;
  real magnify_x, magnify_y;
  int width, height;
  Point middle;

  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  width = ddisp->renderer->renderer.pixel_width;
  height = ddisp->renderer->renderer.pixel_height;
  
  magnify_x = (real)width /
    (dia->data->extents.right - dia->data->extents.left) / ddisp->zoom_factor;
  magnify_y = (real)height /
    (dia->data->extents.bottom - dia->data->extents.top) / ddisp->zoom_factor;

  middle.x = dia->data->extents.left +
    (dia->data->extents.right - dia->data->extents.left) / 2.0;
  middle.y = dia->data->extents.top +
    (dia->data->extents.bottom - dia->data->extents.top) / 2.0;

  ddisplay_zoom (ddisp, &middle, (magnify_x<magnify_y)?magnify_x:magnify_y);

  ddisplay_update_scrollbars(ddisp);
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
}


void
view_edit_grid_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  
  ddisp = ddisplay_active();
  grid_show_dialog(&ddisp->grid, ddisp);
}


void
objects_place_over_callback(GtkWidget *widget, gpointer data)
{
  diagram_place_over_selected(ddisplay_active()->diagram);
}

void
objects_place_under_callback(GtkWidget *widget, gpointer data)
{
  diagram_place_under_selected(ddisplay_active()->diagram);
}

void
objects_group_callback(GtkWidget *widget, gpointer data)
{
  diagram_group_selected(ddisplay_active()->diagram);
} 

void
objects_ungroup_callback(GtkWidget *widget, gpointer data)
{
  diagram_ungroup_selected(ddisplay_active()->diagram);
} 

void
dialogs_properties_callback(GtkWidget *widget, gpointer data)
{
  properties_show(ddisplay_active()->diagram, NULL);
}

void
dialogs_layers_callback(GtkWidget *widget, gpointer data)
{
  layer_dialog_set_diagram(ddisplay_active()->diagram);
  layer_dialog_show();
}


void
objects_align_h_callback(GtkWidget *widget, gpointer data)
{
  int align;
  Diagram *dia;
  GList *objects;

  align = GPOINTER_TO_INT(data);

  dia = ddisplay_active()->diagram;
  objects = dia->data->selected;
  
  object_add_updates_list(objects, dia);
  object_list_align_h(objects, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_flush(dia);     
}

void
objects_align_v_callback(GtkWidget *widget, gpointer data)
{
  int align;
  Diagram *dia;
  GList *objects;

  align = GPOINTER_TO_INT(data);
  dia = ddisplay_active()->diagram;
  objects = dia->data->selected;

  object_add_updates_list(objects, dia);
  object_list_align_v(objects, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_flush(dia);     
}

