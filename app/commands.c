/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson *
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

#include <config.h>

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

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

/* Added by Andrew Ferrier: for use of gnome_about_new() */

#ifdef GNOME
#  include <gnome.h>
#endif

#ifdef G_OS_WIN32
/*
 * Instead of polluting the Dia namespace with windoze headers, declare the
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

#include "paginate_psprint.h"
#ifdef G_OS_WIN32
#  include "paginate_gdiprint.h"
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
#include "propinternals.h"
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
#include "diagram_tree_window.h"
#include "authors.h"                /* master contributors data */

GdkPixbuf *logo;

void file_quit_callback(gpointer data, guint action, GtkWidget *widget)
{
  app_exit();
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
#ifdef G_OS_WIN32
  /* This option could be used with Gnome too. Does it make sense there ? */
  if (!prefs.prefer_psprint)
    diagram_print_gdi(dia);
  else
#endif
    diagram_print_ps(dia);
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

  g_snprintf(buffer, 24, _("Diagram%d.dia"), untitled_nr++);
  
  dia = new_diagram(buffer);
  ddisp = new_display(dia);
  diagram_tree_add(diagram_tree(), dia);
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
  copy_list = parent_list_affected(diagram_get_sorted_selected(ddisp->diagram));

  cnp_store_objects(object_copy_list(copy_list));
  g_list_free(copy_list);

  ddisplay_do_update_menu_sensitivity(ddisp);
}

void
edit_cut_callback(gpointer data, guint action, GtkWidget *widget)
{
  GList *cut_list;
  DDisplay *ddisp;
  Change *change;

  ddisp = ddisplay_active();

  diagram_selected_break_external(ddisp->diagram);

  cut_list = parent_list_affected(diagram_get_sorted_selected(ddisp->diagram));

  cnp_store_objects(object_copy_list(cut_list));

  change = undo_delete_objects_children(ddisp->diagram, cut_list);
  (change->apply)(change, ddisp->diagram);
  
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);


  diagram_modified(ddisp->diagram);
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

  diagram_modified(ddisp->diagram);
  undo_set_transactionpoint(ddisp->diagram->undo);
  
  diagram_remove_all_selected(ddisp->diagram, TRUE);
  diagram_select_list(ddisp->diagram, paste_list);

  diagram_flush(ddisp->diagram);
}

/*
 * ALAN: Paste should probably paste to different position, feels
 * wrong somehow.  ALAN: The offset should increase a little each time
 * if you paste/duplicate several times in a row, because it is
 * clearer what is happening than if you were to piling them all in
 * one place.
 *
 * completely untested, basically it is copy+paste munged together
 */
void
edit_duplicate_callback(gpointer data, guint action, GtkWidget *widget)
{ 
  GList *duplicate_list;
  DDisplay *ddisp;
  Point duplicate_corner;
  Point delta;
  Change *change;

  ddisp = ddisplay_active();
  duplicate_list = object_copy_list(diagram_get_sorted_selected(ddisp->diagram));
  duplicate_corner = object_list_corner(duplicate_list);
  
  /* Move down some 10% of the visible area. */
  delta.x = (ddisp->visible.right - ddisp->visible.left)*0.05;
  delta.y = (ddisp->visible.bottom - ddisp->visible.top)*0.05;

  object_list_move_delta(duplicate_list, &delta);

  change = undo_insert_objects(ddisp->diagram, duplicate_list, 0);
  (change->apply)(change, ddisp->diagram);

  diagram_modified(ddisp->diagram);
  undo_set_transactionpoint(ddisp->diagram->undo);
  
  diagram_remove_all_selected(ddisp->diagram, TRUE);
  diagram_select_list(ddisp->diagram, duplicate_list);

  diagram_flush(ddisp->diagram);
  
  ddisplay_do_update_menu_sensitivity(ddisp);
}




/* Signal handler for getting the clipboard contents */
/* Note that the clipboard is for M$-style cut/copy/paste copying, while
   the selection is for Unix-style mark-and-copy.  We can't really do
   mark-and-copy.
*/

static void
insert_text(DDisplay *ddisp, Focus *focus, const gchar *text)
{
  ObjectChange *change = NULL;
  int modified = FALSE, any_modified = FALSE;
  Object *obj = focus->obj;

  while (text != NULL) {
    gchar *next_line = g_utf8_strchr(text, -1, '\n');
    if (next_line != text) {
      gint len = g_utf8_strlen(text, (next_line-text));
      modified = (*focus->key_event)(focus, GDK_A, text, len, &change);
    }
    if (next_line != NULL) {
      modified = (*focus->key_event)(focus, GDK_Return, "\n", 1, &change);
      text = g_utf8_next_char(next_line);
    } else {
      text = NULL;
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


static void
received_clipboard_handler(GtkClipboard *clipboard, 
			   const gchar *text,
			   gpointer data) {
  Focus *focus = active_focus();
  DDisplay *ddisp = (DDisplay *)data;
  
  if (text == NULL) return;

  if ((focus == NULL) || (!focus->has_focus)) return;

  if (!g_utf8_validate(text, -1, NULL)) {
    message_error("Not valid UTF8");
    return;
  }

  insert_text(ddisp, focus, text);
}

static PropDescription text_prop_singleton_desc[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END};

static void 
make_text_prop_singleton(GPtrArray **props, TextProperty **prop) 
{
  *props = prop_list_from_descs(text_prop_singleton_desc,pdtpp_true);
  g_assert((*props)->len == 1);

  *prop = g_ptr_array_index((*props),0);
  g_free((*prop)->text_data);
  (*prop)->text_data = NULL;
}


void
edit_copy_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  Focus *focus = active_focus();
  DDisplay *ddisp;
  Object *obj;
  GPtrArray *textprops;
  TextProperty *prop;

  if ((focus == NULL) || (!focus->has_focus)) return;

  ddisp = ddisplay_active();

  obj = focus->obj;

  if (obj->ops->get_props == NULL) 
    return;

  make_text_prop_singleton(&textprops,&prop);
  /* Get the first text property */
  obj->ops->get_props(obj, textprops);
  
  /* GTK docs claim the selection clipboard is ignored on Win32.
   * The "clipboard" clipboard is mostly ignored in Unix
   */
#ifdef G_OS_WIN32
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE),
			 prop->text_data, -1);
#else
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
			 prop->text_data, -1);
#endif
  prop_list_free(textprops);
}

void
edit_cut_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  Focus *focus = active_focus();
  DDisplay *ddisp;
  Object *obj;
  Text *text;
  GPtrArray *textprops;
  TextProperty *prop;
  ObjectChange *change;

  if ((focus == NULL) || (!focus->has_focus)) return;

  ddisp = ddisplay_active();

  obj = focus->obj;
  text = (Text *)focus->user_data; 

  if (obj->ops->get_props == NULL) 
    return;

  make_text_prop_singleton(&textprops,&prop);
  /* Get the first text property */
  obj->ops->get_props(obj, textprops);
  
  /* GTK docs claim the selection clipboard is ignored on Win32.
   * The "clipboard" clipboard is mostly ignored in Unix
   */
#ifdef G_OS_WIN32
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_NONE),
			 prop->text_data, -1);
#else
  gtk_clipboard_set_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY),
			 prop->text_data, -1);
#endif

  prop_list_free(textprops);

  if (text_delete_all(text, &change)) { 
    object_add_updates(obj, ddisp->diagram);
    undo_object_change(ddisp->diagram, obj, change);
    undo_set_transactionpoint(ddisp->diagram->undo);
    diagram_flush(ddisp->diagram);
  }
}

void
edit_paste_text_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();

#ifdef G_OS_WIN32
  gtk_clipboard_request_text(gtk_clipboard_get(GDK_NONE), 
			     received_clipboard_handler, ddisp);
#else
  gtk_clipboard_request_text(gtk_clipboard_get(GDK_SELECTION_PRIMARY), 
			     received_clipboard_handler, ddisp);
#endif
}

void
edit_delete_callback(gpointer data, guint action, GtkWidget *widget)
{
  GList *delete_list;
  GList *ptr;
  DDisplay *ddisp;

  Change *change;

  ddisp = ddisplay_active();

  diagram_selected_break_external(ddisp->diagram);

  delete_list = diagram_get_sorted_selected(ddisp->diagram);
  change = undo_delete_objects_children(ddisp->diagram, delete_list);
  (change->apply)(change, ddisp->diagram);
  
  diagram_modified(ddisp->diagram);

  ddisplay_do_update_menu_sensitivity(ddisp);
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
  GDir *dp;
  const char *dentry;
  GError *error = NULL;

  helpdir = dia_get_data_directory("help");
  if (!helpdir) {
    message_warning(_("Could not find help directory"));
    return;
  }

  /* search through helpdir for the helpfile that matches the user's locale */
  dp = g_dir_open (helpdir, 0, &error);
  if (!dp) {
    message_warning(_("Could not open help directory:\n%s"),
                    error->message);
    g_error_free (error);
    return;
  }
  
  while ((dentry = g_dir_read_name(dp)) != NULL) {
    guint score;

    score = intl_score_locale(dentry);
    if (score < bestscore) {
      if (helpindex)
	g_free(helpindex);
      #ifdef G_OS_WIN32
      /* use HTML Help on win32 if available */
      helpindex = g_strconcat(helpdir, G_DIR_SEPARATOR_S, dentry,
			      G_DIR_SEPARATOR_S "dia-manual.chm", NULL);
      if (!g_file_test(helpindex, G_FILE_TEST_EXISTS)) {
	helpindex = g_strconcat(helpdir, G_DIR_SEPARATOR_S, dentry,
			      G_DIR_SEPARATOR_S "index.html", NULL);
      }
      #else
      helpindex = g_strconcat(helpdir, G_DIR_SEPARATOR_S, dentry,
			      G_DIR_SEPARATOR_S "index.html", NULL);
      #endif
      bestscore = score;
    }
  }
  g_dir_close (dp);
  g_free(helpdir);
  if (!helpindex) {
    message_warning(_("Could not find help directory"));
    return;
  }

#ifdef G_OS_WIN32
  #define SW_SHOWNORMAL 1
  ShellExecuteA (0, "open", helpindex, NULL, helpdir, SW_SHOWNORMAL);
#else
  command = getenv("BROWSER");
  command = g_strdup_printf("%s 'file://%s' &", command ? command : "netscape", helpindex);
  system(command);
  g_free(command);
#endif

  g_free(helpindex);
}

void
help_about_callback(gpointer data, guint action, GtkWidget *widget)
{
#ifdef GNOME

  /*  Take advantage of gnome_about_new(),
   *  which is much cleaner and GNOME2 HIG compliant,
   *  Originally implemented by Xing Wang, modified
   *  by Andrew Ferrier.
   *
   *  Note: in this function there is no need to discriminate
   *  between the different kinds of 'authors'.
   */
  
  static GtkWidget *about;
  
  /*
   * Translators should localize the following string
   * which will give them credit in the About box.
   * E.g. "Fulano de Tal <fulano@detal.com>"
   */
  
  gchar *translators = _("translator_credits-PLEASE_ADD_YOURSELF_HERE");
  gchar logo_file[100];
  
  if (!about) {
    GdkPixbuf *logo;
  
    gchar* datadir = dia_get_data_directory(""); 
    g_snprintf(logo_file, sizeof(logo_file),
                             "%s%sdia_logo.png", datadir, G_DIR_SEPARATOR_S);

  	logo = gdk_pixbuf_new_from_file(logo_file, NULL);
    g_free(datadir);
  
    about = gnome_about_new(
      _("Dia"),
      VERSION,
      _("Copyright (C) 1998-2002 The Free Software Foundation and the authors"),
      _("Dia is a program for drawing structured diagrams.\n"
      "Please visit http://www.lysator.liu.se/~alla/dia for more information."),
      authors,
      documentors,
      (strcmp (translators, "translator_credits-PLEASE_ADD_YOURSELF_HERE")
      ? translators : NULL),
      logo);
  
    if (logo)
        g_object_unref (logo);
      
  	g_signal_connect (about, "destroy",
         G_CALLBACK (gtk_widget_destroyed),
         &about);
  }

  gtk_widget_show_now (about);
  
#else

  /* No GNOME, fall back to the old GTK method */

  const gint nauthors = (sizeof(authors) / sizeof(authors[0])) - 1;
  const gint ndocumentors = (sizeof(documentors) / sizeof(documentors[0])) - 1;

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
  
  dialog = gtk_dialog_new ();
  gtk_window_set_role (GTK_WINDOW (dialog), "about_dialog");
  gtk_window_set_title (GTK_WINDOW (dialog), _("About Dia"));
  gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
  gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
  g_signal_connect (GTK_OBJECT (dialog), "destroy",
		    G_CALLBACK (gtk_widget_destroy), 
          GTK_OBJECT (dialog));

  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
  gtk_container_add (GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), vbox);

  if (!logo) {
      gchar* datadir = dia_get_data_directory(""); 
      g_snprintf(str, sizeof(str), "%s%sdia_logo.png", datadir, G_DIR_SEPARATOR_S);
      logo = gdk_pixbuf_new_from_file(str, NULL);
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

      /* Exact spelling is Ch&eacute;p&eacute;lov (using *ML entities) */
  label = gtk_label_new(_("Maintainers: Lars Clausen and Cyrille Chepelov"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 1,2,
       GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  label = gtk_label_new (_("Please visit http://www.lysator.liu.se/~alla/dia "
         "for more information"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 2,3,
       GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  label = gtk_label_new (_("Contributors:"));
  gtk_table_attach(GTK_TABLE(table), label, 0,2, 3,4,
       GTK_FILL|GTK_EXPAND, GTK_FILL, 0,2);

  for (i = 0; i < nauthors; i++) {
    label = gtk_label_new(authors[i]);
    gtk_table_attach(GTK_TABLE(table), label, i%2,i%2+1, i/2+4,i/2+5,
       GTK_FILL|GTK_EXPAND, GTK_FILL, 0,0);
  }

  for (i = nauthors; i < nauthors + ndocumentors; i++) {
    label = gtk_label_new(documentors[i - nauthors]);
    gtk_table_attach(GTK_TABLE(table), label, i%2,i%2+1, i/2+4,i/2+5,
       GTK_FILL|GTK_EXPAND, GTK_FILL, 0,0);
  }

  gtk_table_set_col_spacings(GTK_TABLE(table), 1);
  gtk_table_set_row_spacings(GTK_TABLE(table), 1);

  bbox = gtk_hbutton_box_new();
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), bbox, TRUE, TRUE, 5);
  gtk_button_box_set_child_size(GTK_BUTTON_BOX(bbox), 80, 0);
  gtk_button_box_set_spacing(GTK_BUTTON_BOX(bbox), 10);

  button = gtk_button_new_from_stock(GTK_STOCK_OK);
  gtk_container_add(GTK_CONTAINER(bbox), button);
  g_signal_connect_swapped(GTK_OBJECT (button), "clicked",
			   G_CALLBACK(gtk_widget_destroy),
          GTK_OBJECT(dialog));

  gtk_widget_show_all (dialog);

#endif /*  GNOME  */
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

  ddisp = ddisplay_active();
 
  aa =  GTK_CHECK_MENU_ITEM(widget)->active;
  
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

  ddisp = ddisplay_active();
  
  ddisplay_set_snap_to_grid(ddisp, GTK_CHECK_MENU_ITEM(widget)->active);
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

  width = dia_renderer_get_width_pixels (ddisp->renderer);
  height = dia_renderer_get_height_pixels (ddisp->renderer);

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
view_redraw_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  ddisp = ddisplay_active();
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
objects_place_up_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_place_up_selected(ddisplay_active()->diagram);
}

void
objects_place_down_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_place_down_selected(ddisplay_active()->diagram);
}

void
objects_parent_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_parent_selected(ddisplay_active()->diagram);
}

void
objects_unparent_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_unparent_selected(ddisplay_active()->diagram);
}

void
objects_unparent_children_callback(gpointer data, guint action, GtkWidget *widget)
{
  diagram_unparent_children_selected(ddisplay_active()->diagram);
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
    properties_show(dia, selected);
  } else {
    diagram_properties_show(dia);
  } 
}

void
dialogs_layers_callback(gpointer data, guint action, GtkWidget *widget)
{
  /* This shouldn't really be necessary */
  diagram_set_current(ddisplay_active()->diagram);
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

