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

#include "config.h"

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <math.h>
#include <glib.h>
#ifdef HAVE_DIRENT_H
#  include <dirent.h>
#endif

#include <gdk-pixbuf/gdk-pixbuf.h>

#ifdef G_OS_WIN32
/*
 * Instead of polluting the Dia namespace with indoze headers, declare the
 * required prototype here. This is bad style, but not as bad as namespace
 * clashes to be resolved without C++   --hb
 */
long __stdcall
ShellExecuteA (long        hwnd,
               const char* lpOperation,
               const char* lpFile,
               const char* lpParameters,
               const char* lpDirectory,
               int         nShowCmd);
#endif

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
#include "text.h"
#include "dia_dirs.h"
#include "focus.h"
#include "gdk/gdk.h"
#include "gdk/gdkkeysyms.h"
#include "lib/properties.h"
#include "dia-props.h"

GdkPixbuf *logo;

void file_quit_callback(gpointer data, guint action, GtkWidget *widget)
{
  app_exit();
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
file_import_from_xfig_callback (gpointer data, guint action, GtkWidget *widget)
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
file_pagesetup_callback(gpointer data, guint action, GtkWidget *widget)
{
  Diagram *dia;

  dia = ddisplay_active()->diagram;
  create_page_setup_dlg(dia);
}

void
file_print_callback(gpointer data, guint action, GtkWidget *widget)
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
file_close_callback(gpointer data, guint action, GtkWidget *widget)
{
  ddisplay_close(ddisplay_active());
} 

void
file_new_callback(gpointer data, guint action, GtkWidget *widget)
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
file_preferences_callback(gpointer data, guint action, GtkWidget *widget)
{
  prefs_show();
}


void
edit_copy_callback(gpointer data, guint action, GtkWidget *widget)
{
  GList *copy_list;
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  copy_list = diagram_get_sorted_selected(ddisp->diagram);

  cnp_store_objects(object_copy_list(copy_list));
}

void
edit_cut_callback(gpointer data, guint action, GtkWidget *widget)
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
edit_paste_callback(gpointer data, guint action, GtkWidget *widget)
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

/* Signal handler for getting the selection from whoever */
void
received_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
			   gpointer data)
{
  Focus *focus = active_focus();
  gchar *text;
  int i;
  ObjectChange *change = NULL;
  int modified, any_modified = FALSE;
  Object *obj;
  DDisplay *ddisp = ddisplay_active();

  if ((focus == NULL) || (!focus->has_focus)) return;

  obj = focus->obj;

  /* **** IMPORTANT **** Check to see if retrieval succeeded  */
  if (selection->length < 0)
  {
    g_print ("Selection retrieval failed\n");
    return;
  }
  /* Make sure we got the data in the expected form */
  if (selection->type != GDK_SELECTION_TYPE_STRING)
  {
    g_print ("Selection \"STRING\" was not returned as a string!\n");
    return;
  }

  text = (gchar *)selection->data;
  for (i = 0; i < selection->length; i++) {
    if (text[i] == '\n') {
      modified = (*focus->key_event)(focus, GDK_Return, "\n", 1, &change);
    } else {
      modified = (*focus->key_event)(focus, GDK_A, &text[i], 1, &change);
    }
    { /* Make sure object updates its data: */
      Point p = obj->position;
      (obj->ops->move)(obj,&p);  }

    /* Perhaps this can be improved */
    object_add_updates(obj, ddisp->diagram);
    
    if (modified && (change != NULL)) {
      undo_object_change(ddisp->diagram, obj, change);
      any_modified = TRUE;
    }
    
    diagram_flush(ddisp->diagram);
  }

  if (any_modified) 
    undo_set_transactionpoint(ddisp->diagram->undo);
}

static char *current_clipboard;

void
get_selection_handler(GtkWidget *widget, GtkSelectionData *selection,
		      gpointer data) {
  if (current_clipboard) {
    gtk_selection_data_set(selection, GDK_TARGET_STRING,
			   8, current_clipboard, strlen(current_clipboard));
  }
}

void
edit_copy_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  Focus *focus = active_focus();
  DDisplay *ddisp;
  Object *obj;
  Property textprop;

  if ((focus == NULL) || (!focus->has_focus)) return;

  ddisp = ddisplay_active();

  obj = focus->obj;

  if (obj->ops->get_props == NULL) 
    return;

  textprop.name = "text";
  textprop.type = PROP_TYPE_INVALID;
  PROP_VALUE_STRING(textprop) = NULL;

  /* Get the first text property */
  obj->ops->get_props(obj, &textprop, 1);
  
  if (current_clipboard) g_free(current_clipboard);

  if (textprop.type != PROP_TYPE_STRING)
    return;

  current_clipboard = g_strdup(PROP_VALUE_STRING(textprop));

  prop_free(&textprop);

  gtk_selection_owner_set(GTK_WIDGET(ddisp->shell),
			  GDK_SELECTION_PRIMARY,
			  GDK_CURRENT_TIME);
  gtk_selection_add_target(GTK_WIDGET(ddisp->shell),
			   GDK_SELECTION_PRIMARY,
			   GDK_TARGET_STRING, 0);
}

void
edit_cut_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  Focus *focus = active_focus();
  DDisplay *ddisp;
  Object *obj;
  Property textprop;
  Change *change;

  if ((focus == NULL) || (!focus->has_focus)) return;

  ddisp = ddisplay_active();

  obj = focus->obj;

  if (obj->ops->get_props == NULL) 
    return;

  textprop.name = "text";
  textprop.type = PROP_TYPE_INVALID;
  PROP_VALUE_STRING(textprop) = NULL;

  /* Get the first text property */
  obj->ops->get_props(obj, &textprop, 1);
  
  if (current_clipboard) g_free(current_clipboard);

  if (textprop.type != PROP_TYPE_STRING)
    return;

  current_clipboard = g_strdup(PROP_VALUE_STRING(textprop));

  prop_free(&textprop);

  gtk_selection_owner_set(GTK_WIDGET(ddisp->shell),
			  GDK_SELECTION_PRIMARY,
			  GDK_CURRENT_TIME);
  gtk_selection_add_target(GTK_WIDGET(ddisp->shell),
			   GDK_SELECTION_PRIMARY,
			   GDK_TARGET_STRING, 0);

  if (text_delete_all(obj, &change)) {
    (change->apply)(change, ddisp->diagram);
    
    diagram_update_menu_sensitivity(ddisp->diagram);
    diagram_flush(ddisp->diagram);
  }
}

void
edit_paste_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();

  gtk_selection_convert(ddisp->shell, GDK_SELECTION_PRIMARY, GDK_TARGET_STRING,
			GDK_CURRENT_TIME);
}

void
edit_delete_callback(gpointer data, guint action, GtkWidget *widget)
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
edit_undo_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  
  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  undo_revert_to_last_tp(dia->undo);

  diagram_flush(dia);
} 

void
edit_redo_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  
  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  undo_apply_to_next_tp(dia->undo);

  diagram_flush(dia);
} 

void
help_manual_callback(gpointer data, guint action, GtkWidget *widget)
{
  char *helpdir, *helpindex = NULL, *command;
  guint bestscore = G_MAXINT;
  DIR *dp;
  struct dirent *dirp;

  helpdir = dia_get_data_directory("help");
  if (!helpdir) {
    message_warning(_("Could not find help directory"));
    return;
  }

  /* search through helpdir for the helpfile that matches the user's locale */
  dp = opendir(helpdir);
  while ((dirp = readdir(dp)) != NULL) {
    guint score;

    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, ".."))
      continue;

    score = intl_score_locale(dirp->d_name);
    if (score < bestscore) {
      if (helpindex)
	g_free(helpindex);
      helpindex = g_strconcat(helpdir, G_DIR_SEPARATOR_S, dirp->d_name,
			      G_DIR_SEPARATOR_S "index.html", NULL);
      bestscore = score;
    }
  }
  g_free(helpdir);
  if (!helpindex) {
    message_warning(_("Could not find help directory"));
    return;
  }

#ifdef G_OS_WIN32
  ShellExecuteA (0, NULL, helpindex, NULL, helpdir, 0); 
#else
  command = g_strdup_printf("netscape '%s' &", helpindex);
  system(command);
  g_free(command);
#endif

  g_free(helpindex);
}

void
help_about_callback(gpointer data, guint action, GtkWidget *widget)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *table;
  GtkWidget *bbox;
  GtkWidget *frame;
  GtkWidget *label;
  GtkWidget *button;
  char str[100];
  gint i;
  
  GtkWidget *gpixmap;

  static const gchar *contributors[] = {
    "Jerome Abela",
    "Hans Breuer",
    "Emmanuel Briot",
    "Cyrille Chepelov",
    "Lars R. Clausen",
    "Fredrik Hallenberg",
    "Francis J. Lacoste",
    "Steffen Macke",
    "Jacek Pliszka",
    "Henk Jan Priester",
    "Alejandro Aguilar Sierra",
  };
  const gint ncontributors = sizeof(contributors) / sizeof(contributors[0]);

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

  if (!logo) {
      gchar* datadir = dia_get_data_directory(""); 
      g_snprintf(str, sizeof(str), "%s%sdia_logo.png", datadir, G_DIR_SEPARATOR_S);
      logo = gdk_pixbuf_new_from_file(str);
      g_free(datadir);
  }

  if (logo) {
      GdkPixmap *pixmap;
      GdkBitmap *bitmap;

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
      gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
      gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

      gdk_pixbuf_render_pixmap_and_mask(logo, &pixmap, &bitmap, 128);
      gpixmap = gtk_pixmap_new(pixmap, bitmap);
      gdk_pixmap_unref(pixmap);
      if (bitmap) gdk_bitmap_unref(bitmap);
      gtk_container_add (GTK_CONTAINER(frame), gpixmap);
  }

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_set_border_width (GTK_CONTAINER (frame), 1);
  gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, TRUE, 1);

  table = gtk_table_new(3, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (table), 1);
  gtk_container_add (GTK_CONTAINER (frame), table);

  g_snprintf(str, sizeof(str), _("Dia v %s by Alexander Larsson"), VERSION);
  label = gtk_label_new (str);
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);
  
  label = gtk_label_new(_("Maintainer: James Henstridge"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  label = gtk_label_new (_("Please visit http://www.lysator.liu.se/~alla/dia "
			   "for more info"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  label = gtk_label_new (_("Contributors:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  for (i = 0; i < ncontributors; i++) {
    label = gtk_label_new(contributors[i]);
    gtk_table_attach(GTK_TABLE(table), label, i%2,i%2+1, i/2+4,i/2+5,
		   GTK_FILL|GTK_EXPAND, GTK_FILL, 0,0);
  }
  gtk_table_set_col_spacings(GTK_TABLE(table), 1);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);

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
view_zoom_in_callback(gpointer data, guint action, GtkWidget *widget)
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
view_zoom_out_callback(gpointer data, guint action, GtkWidget *widget)
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
view_zoom_set_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  real scale;
  Point middle;
  Rectangle *visible;
  
  ddisp = ddisplay_active();
  visible = &ddisp->visible;
  middle.x = visible->left*0.5 + visible->right*0.5;
  middle.y = visible->top*0.5 + visible->bottom*0.5;

  scale = ((real) action)/1000.0 * DDISPLAY_NORMAL_ZOOM;

  ddisplay_zoom(ddisp, &middle, scale / ddisp->zoom_factor);  
}

void
view_show_cx_pts_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();

  old_val = ddisp->show_cx_pts;
  ddisp->show_cx_pts = GTK_CHECK_MENU_ITEM(widget)->active;
  
  if (old_val != ddisp->show_cx_pts) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_aa_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  int aa;
  static gboolean shown_warning = FALSE;

  ddisp = ddisplay_active();
 
  aa =  GTK_CHECK_MENU_ITEM(widget)->active;
  
  if (aa && !shown_warning)
    message_warning(_("The anti aliased renderer is buggy, and may cause\n"
		      "crashes.  We know there are bugs in it, so don't\n"
		      "bother submitting another report if it crashes"));
  shown_warning = TRUE;

  if (aa != ddisp->aa_renderer) {
    ddisplay_set_renderer(ddisp, aa);
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_visible_grid_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  
  old_val = ddisp->grid.visible;
  ddisp->grid.visible = GTK_CHECK_MENU_ITEM(widget)->active; 

  if (old_val != ddisp->grid.visible) {
    ddisplay_add_update_all(ddisp);
    ddisplay_flush(ddisp);
  }
}

void
view_snap_to_grid_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active();
  
  old_val = ddisp->grid.snap;
  ddisp->grid.snap =  GTK_CHECK_MENU_ITEM(widget)->active;
}

void view_toggle_rulers_callback(gpointer data, guint action, GtkWidget*widget)
{
  DDisplay *ddisp;
  
  ddisp = ddisplay_active();

  /* The following is borrowed straight from the Gimp: */
  
  /* This routine use promiscuous knowledge of gtk internals
   *  in order to hide and show the rulers "smoothly". This
   *  is kludgy and a hack and may break if gtk is changed
   *  internally.
   */
  if (!GTK_CHECK_MENU_ITEM(widget)->active) {
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
view_new_view_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;

  ddisp = ddisplay_active();
  dia = ddisp->diagram;
  
  ddisp = new_display(dia);
}

void
view_show_all_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  real magnify_x, magnify_y;
  int width, height;
  Point middle;

  ddisp = ddisplay_active();
  dia = ddisp->diagram;

  width = ddisp->renderer->pixel_width;
  height = ddisp->renderer->pixel_height;
  
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
view_diagram_properties_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();
  diagram_properties_show(ddisp->diagram);
}


void
objects_place_over_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_place_over_selected(ddisplay_active()->diagram);
}

void
objects_place_under_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_place_under_selected(ddisplay_active()->diagram);
}

void
objects_group_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_group_selected(ddisplay_active()->diagram);
} 

void
objects_ungroup_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_ungroup_selected(ddisplay_active()->diagram);
} 

void
dialogs_properties_callback(gpointer data, guint action, GtkWidget *widget)
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
dialogs_layers_callback(gpointer data, guint action, GtkWidget *widget)
{
  layer_dialog_set_diagram(ddisplay_active()->diagram);
  layer_dialog_show();
}


void
objects_align_h_callback(gpointer data, guint action, GtkWidget *widget)
{
  int align;
  Diagram *dia;
  GList *objects;

  align = action;

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
objects_align_v_callback(gpointer data, guint action, GtkWidget *widget)
{
  int align;
  Diagram *dia;
  GList *objects;

  align = action;

  dia = ddisplay_active()->diagram;
  objects = dia->data->selected;

  object_add_updates_list(objects, dia);
  object_list_align_v(objects, dia, align);
  diagram_update_connections_selection(dia);
  object_add_updates_list(objects, dia);
  diagram_flush(dia);     

  undo_set_transactionpoint(dia->undo);
}

