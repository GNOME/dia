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
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>

#include <glib.h>
#include <gdk_imlib.h>

#include "config.h"
#ifdef GNOME
#include <gnome.h>
#endif
#ifdef GNOME_PRINT
#include "paginate_gnomeprint.h"
#else
#include "paginate_psprint.h"
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
#include "preferences.h"
#include "layer_dialog.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "pagesetup.h"

GdkImlibImage *logo;

/* if user already has a diagram open, then start out in that directory */
static void set_default_file_selection_directory (GtkWidget *fs)
{
  DDisplay *ddisp = ddisplay_active();
  if (ddisp && ddisp->diagram && (! ddisp->diagram->unsaved)) {
    char *last_slash;
    char *fn = (char *) malloc (strlen (ddisp->diagram->filename) + 10);
    strcpy (fn, ddisp->diagram->filename);
    last_slash = strrchr (fn, '/');
    if (last_slash) {
      *(last_slash+1) = '\0';
      gtk_file_selection_set_filename (GTK_FILE_SELECTION (fs), fn);
    }
    free (fn);
  }
}


/* if the a diagram is open and has a file name, set a default
   file name is given extension */
static void set_default_file_selection_extension (GtkWidget *fs, char *ext)
{
  DDisplay *ddisp = ddisplay_active();
  if (ddisp && ddisp->diagram && (! ddisp->diagram->unsaved)) {
    char *last_slash;
    char *last_dot;
    char *ext_index;
    char *fn = (char *) malloc (strlen (ddisp->diagram->filename) +
				10 + strlen (ext));
    strcpy (fn, ddisp->diagram->filename);
    
    /* put a .ext extention on the file name */
    last_slash = strrchr (fn, '/');
    last_dot = strrchr (fn, '.');
    if (last_slash && last_dot && (last_dot > last_slash))
      ext_index = last_dot;
    else if ((! last_slash) && last_dot)
      ext_index = last_dot;
    else
      ext_index = fn + strlen (fn);
    strcpy (ext_index, ext);
    
    gtk_file_selection_set_filename (GTK_FILE_SELECTION (fs), fn);
    free (fn);
  }
}


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
    layer_dialog_set_diagram(diagram);
  
    ddisp = new_display(diagram);
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

  set_default_file_selection_directory (window);
  
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
    diagram_update_extents(ddisp->diagram);
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
    g_snprintf(buffer, 300,
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

  diagram_update_extents(dia);

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

  set_default_file_selection_directory (window);
  
  gtk_widget_show (window);

  /* Make dialog modal: */
  gtk_widget_grab_focus(window);
  gtk_grab_add(window);
}

static void
file_import_from_xfig_dialog_ok_callback (GtkWidget        *w,
				     GtkFileSelection *fs)
{
  char *filename;
  Diagram *dia;
  struct stat stat_struct;

  dia = gtk_object_get_user_data(GTK_OBJECT(fs));
  
  filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (fs));

  if (stat(filename, &stat_struct)!=0) {
    message_error(_("No such file found\n%s\n"), filename);
    gtk_grab_remove(GTK_WIDGET(fs));
    gtk_widget_destroy(GTK_WIDGET(fs));
    return;
  }
  
  diagram_import_from_xfig(dia, filename);
  
  gtk_grab_remove(GTK_WIDGET(fs));
  gtk_widget_destroy(GTK_WIDGET(fs));
}

void
file_import_from_xfig_callback (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  GtkWidget *window = NULL;

  ddisp = ddisplay_active();

  window = gtk_file_selection_new (_("Import from XFig"));
  gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
  
  gtk_object_set_user_data(GTK_OBJECT(window), ddisp->diagram);
  
  gtk_signal_connect (GTK_OBJECT (GTK_FILE_SELECTION (window)->ok_button),
		      "clicked", GTK_SIGNAL_FUNC(file_import_from_xfig_dialog_ok_callback),
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
file_pagesetup_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia;

  dia = ddisplay_active()->diagram;
  create_page_setup_dlg(dia);
}

void
file_print_callback(GtkWidget *widget, gpointer data)
{
  Diagram *dia;

  dia = ddisplay_active()->diagram;
#ifdef GNOME_PRINT
  diagram_print_gnome(dia);
#else
  diagram_print_ps(dia);
#endif
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

  g_snprintf(buffer, 24, _("Untitled-%d"), untitled_nr++);
  
  dia = new_diagram(buffer);
  ddisp = new_display(dia);
}

void
file_preferences_callback(GtkWidget *widget, gpointer data)
{
  prefs_show();
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
  Change *change;

  ddisp = ddisplay_active();

  diagram_selected_break_external(ddisp->diagram);

  cut_list = diagram_get_sorted_selected(ddisp->diagram);

  cnp_store_objects(object_copy_list(cut_list));

  change = undo_delete_objects(ddisp->diagram, cut_list);
  (change->apply)(change, ddisp->diagram);
  
  diagram_update_menu_sensitivity(ddisp->diagram);
  diagram_flush(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
}

void
edit_paste_callback(GtkWidget *widget, gpointer data)
{
  GList *paste_list;
  DDisplay *ddisp;
  Point paste_corner;
  Point delta;
  Change *change;
  
  ddisp = ddisplay_active();

  if (!cnp_exist_stored_objects()) {
    message_warning(_("No existing object to paste.\n"));
    return;
  }
  
  paste_list = cnp_get_stored_objects(); /* Gets a copy */

  paste_corner = object_list_corner(paste_list);
  
  delta.x = ddisp->visible.left - paste_corner.x;
  delta.y = ddisp->visible.top - paste_corner.y;

  /* Move down some 10% of the visible area. */
  delta.x += (ddisp->visible.right - ddisp->visible.left)*0.1;
  delta.y += (ddisp->visible.bottom - ddisp->visible.top)*0.1;

  object_list_move_delta(paste_list, &delta);

  change = undo_insert_objects(ddisp->diagram, paste_list, 0);
  (change->apply)(change, ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
  
  diagram_remove_all_selected(ddisp->diagram, TRUE);
  diagram_select_list(ddisp->diagram, paste_list);

  diagram_flush(ddisp->diagram);
}

void
edit_delete_callback(GtkWidget *widget, gpointer data)
{
  GList *delete_list;
  DDisplay *ddisp;

  Change *change;

  ddisp = ddisplay_active();

  diagram_selected_break_external(ddisp->diagram);

  delete_list = diagram_get_sorted_selected(ddisp->diagram);

  change = undo_delete_objects(ddisp->diagram, delete_list);
  (change->apply)(change, ddisp->diagram);
  
  diagram_update_menu_sensitivity(ddisp->diagram);
  diagram_flush(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
} 

void
edit_undo_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Diagram *dia;
  
  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  undo_revert_to_last_tp(dia->undo);

  diagram_flush(dia);
} 

void
edit_redo_callback(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  Diagram *dia;
  
  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  undo_apply_to_next_tp(dia->undo);

  diagram_flush(dia);
} 

void
logo_expose_callback(GtkWidget *widget, GdkEventExpose *event)
{
  if (logo) {
    gdk_imlib_paste_image(logo, widget->window, event->area.x, event->area.y, 
			  event->area.width, event->area.height);
  }
}

void
help_about_callback(GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;
  char str[100];

  GtkWidget *drawarea;

  dialog = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog), "about_dialog", "Dia");
  gtk_window_set_title (GTK_WINDOW (dialog), _("About Dia"));
  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, FALSE, FALSE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  gtk_signal_connect (GTK_OBJECT (dialog), "destroy",
		      GTK_SIGNAL_FUNC (gtk_widget_destroy), 
		      GTK_OBJECT (dialog));

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

  g_snprintf(str, sizeof(str), "%s/dia_logo.png", DATADIR);
  logo = gdk_imlib_load_image(str);

  if (logo) {
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
    gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

    drawarea = gtk_drawing_area_new();
    gtk_container_add (GTK_CONTAINER(frame), drawarea);
    gtk_signal_connect (GTK_OBJECT (drawarea), "expose_event",
			(GtkSignalFunc) logo_expose_callback, NULL);
    
    gtk_drawing_area_size(GTK_DRAWING_AREA(drawarea), logo->rgb_width, logo->rgb_height);
  }

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 1);
  gtk_container_add (GTK_CONTAINER (frame), vbox);

  sprintf(str, _("Dia v %s by Alexander Larsson"), VERSION);
  label = gtk_label_new (str);
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  label = gtk_label_new (_("Please visit http://www.lysator.liu.se/~alla/dia "
			   "for more info"));
  gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, TRUE, 2);

  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), bbox, TRUE, TRUE, 5);
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), 80, 0);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 10);

  button = gtk_button_new_with_label(_("OK"));
  gtk_container_add(GTK_CONTAINER(bbox), button);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(dialog));

  gtk_widget_show_all (dialog);
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

# if GNOME
  {
    /* XXX get the % out of the menu's label -- this is gross */
    float v;
    GtkBin *mi = GTK_BIN (widget);
    GtkLabel *lbl = GTK_LABEL (mi->child);
    sscanf (lbl->label, "%f", &v);
    scale = ((real) v)/100.0 * DDISPLAY_NORMAL_ZOOM;
  }
# else
  percent = (int) data;
  scale = ((real) percent)/1000.0 * DDISPLAY_NORMAL_ZOOM;
# endif

  ddisplay_zoom(ddisp, &middle, scale / ddisp->zoom_factor);  
}

#ifdef GNOME
void
view_show_cx_pts_callback(GtkWidget *widget,
			  gpointer callback_data)
#else /* GNOME */
void
view_show_cx_pts_callback(gpointer callback_data,
                          guint callback_action,
                          GtkWidget *widget)
#endif /* GNOME */
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();

  old_val = ddisp->show_cx_pts;
  ddisp->show_cx_pts = GTK_CHECK_MENU_ITEM (widget)->active;
  
  if (old_val != ddisp->show_cx_pts) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

#ifdef GNOME
void
view_visible_grid_callback(GtkWidget *widget,
			   gpointer callback_data)
#else /* GNOME */
void
view_visible_grid_callback(gpointer callback_data,
			   guint callback_action,
			   GtkWidget *widget)
#endif /* GNOME */
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

#ifdef GNOME
void
view_snap_to_grid_callback(GtkWidget *widget,
			   gpointer callback_data)
#else /* GNOME */
void
view_snap_to_grid_callback(gpointer callback_data,
			   guint callback_action,
			   GtkWidget *widget)
#endif /* GNOME */
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  
  old_val = ddisp->grid.snap;
  ddisp->grid.snap = GTK_CHECK_MENU_ITEM (widget)->active;
}

#ifdef GNOME
void view_toggle_rulers_callback(GtkWidget *widget,
				 gpointer callback_data)
#else /* GNOME */
void view_toggle_rulers_callback(gpointer callback_data,
				 guint callback_action,
				 GtkWidget *widget)
#endif /* GNOME */
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
      GTK_WIDGET_SET_FLAGS (ddisp->origin, GTK_VISIBLE);
      GTK_WIDGET_SET_FLAGS (ddisp->hrule, GTK_VISIBLE);
      GTK_WIDGET_SET_FLAGS (ddisp->vrule, GTK_VISIBLE);
      
      gtk_widget_map (ddisp->origin);
      gtk_widget_map (ddisp->hrule);
      gtk_widget_map (ddisp->vrule);
      
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
  Diagram *dia;
  Object *selected;

  dia = ddisplay_active()->diagram; 

  if (dia->data->selected != NULL) {
       selected = dia->data->selected->data;
  } else {
         selected = NULL;
  } 
  properties_show(dia, selected);
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
  object_list_align_h(objects, dia, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_flush(dia);     

  undo_set_transactionpoint(dia->undo);
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
  object_list_align_v(objects, dia, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_flush(dia);     

  undo_set_transactionpoint(dia->undo);
}

