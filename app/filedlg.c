/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filedlg.c: some dialogs for saving/loading/exporting files.
 * Copyright (C) 1999 James Henstridge
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

#include <config.h>

#include "filedlg.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <gtk/gtk.h>
#include "intl.h"
#include "filter.h"
#include "display.h"
#include "message.h"
#include "layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "interface.h"

static GtkWidget *opendlg = NULL, *open_options = NULL, *open_omenu = NULL;
static GtkWidget *savedlg = NULL;
static GtkWidget *exportdlg = NULL, *export_options = NULL, *export_omenu=NULL;
static GtkWidget *compressbutton = NULL;

static int
file_dialog_hide (GtkWidget *filesel)
{
  gtk_widget_hide (filesel);

#if 0
  menus_set_sensitive ("<Toolbox>/File/Open...", TRUE);
  menus_set_sensitive ("<Image>/File/Open...", TRUE);

  if (gdisplay_active ())
    {
      menus_set_sensitive ("<Image>/File/Save", TRUE);
      menus_set_sensitive ("<Image>/File/Save as...", TRUE);
    }
#endif

  return TRUE;
}

static void
set_true_callback(GtkWidget *w, int *data)
{
  *data = TRUE;
}

static void
toggle_compress_callback(GtkWidget *w, gpointer data)
{
  // Changes prefs exactly when the user toggles the setting, i.e.
  // the setting really remembers what the user chose last time, but
  // lets diagrams of the opposite kind stay that way unless the user
  // intervenes.
  prefs.new_diagram.compress_save =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compressbutton));
}

static void
open_set_extension(GtkObject *item)
{
  DiaImportFilter *ifilter = gtk_object_get_user_data(item);
  GString *s;
  const gchar *text = gtk_entry_get_text(GTK_ENTRY(GTK_FILE_SELECTION(opendlg)
                                                   ->selection_entry));
  const gchar *last_dot = strrchr(text, '.');

  if (!ifilter || last_dot == text || text[0] == '\0' ||
      ifilter->extensions[0] == NULL)
    return;

  s = g_string_new(text);
  if (last_dot)
    g_string_truncate(s, last_dot-text);
  g_string_append(s, ".");
  g_string_append(s, ifilter->extensions[0]);
  gtk_entry_set_text(GTK_ENTRY(GTK_FILE_SELECTION(opendlg)->selection_entry),
                     s->str);
  g_string_free (s, TRUE);
}

static GtkWidget *
create_open_menu(void)
{
  GtkWidget *menu;
  GtkWidget *item;
  GList *tmp;

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label(_("By extension"));
  gtk_container_add(GTK_CONTAINER(menu), item);
  gtk_widget_show(item);
  item = gtk_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu), item);
  gtk_widget_show(item);
  
  
  for (tmp = filter_get_import_filters(); tmp != NULL; tmp = tmp->next) {
    DiaImportFilter *ifilter = tmp->data;

    if (!ifilter)
      continue;
    item = gtk_menu_item_new_with_label(
		filter_get_import_filter_label(ifilter));
    gtk_object_set_user_data(GTK_OBJECT(item), ifilter);
    g_signal_connect(GTK_OBJECT(item), "activate",
                       GTK_SIGNAL_FUNC(open_set_extension), NULL);
    gtk_container_add(GTK_CONTAINER(menu), item);
    gtk_widget_show(item);
  }
  return menu;
}

static void
file_open_ok_callback(GtkWidget *w, GtkFileSelection *fs)
{
  const char *filename;
  Diagram *diagram = NULL;
  DiaImportFilter *ifilter;
  DDisplay *ddisp;

  filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));

  ifilter = gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(open_omenu)
						->menu_item));
  diagram = diagram_load(filename, ifilter);

  if (diagram != NULL) {
    diagram_update_extents(diagram);
    layer_dialog_set_diagram(diagram);

    ddisp = new_display(diagram);
  }

  gtk_widget_hide(opendlg);
}

void
file_open_callback(gpointer data, guint action, GtkWidget *widget)
{
  if (!opendlg) {
    DDisplay *ddisp;
    Diagram *dia = NULL;
    
    ddisp = ddisplay_active();
    if (ddisp) {
      dia = ddisp->diagram;
    }
    opendlg = gtk_file_selection_new(_("Open Diagram"));
    gtk_window_set_role(GTK_WINDOW(opendlg), "open_diagram");
    gtk_window_set_position(GTK_WINDOW(opendlg), GTK_WIN_POS_MOUSE);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(opendlg),
				    dia && dia->filename ? dia->filename
				    : "." G_DIR_SEPARATOR_S);
    g_signal_connect_swapped(
		GTK_OBJECT(GTK_FILE_SELECTION(opendlg)->cancel_button),
	    "clicked", G_CALLBACK(file_dialog_hide),
		GTK_OBJECT(opendlg));
    g_signal_connect(GTK_OBJECT(opendlg), "delete_event",
		     G_CALLBACK(file_dialog_hide), NULL);
    g_signal_connect(GTK_OBJECT(opendlg), "destroy",
		       G_CALLBACK(gtk_widget_destroyed), &opendlg);
    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(opendlg)->ok_button),
		       "clicked", G_CALLBACK(file_open_ok_callback),
		       opendlg);
    gtk_quit_add_destroy(1, GTK_OBJECT(opendlg));
  } else {
    gtk_widget_set_sensitive(opendlg, TRUE);
    if (GTK_WIDGET_VISIBLE(opendlg))
      return;
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(opendlg),
				    "." G_DIR_SEPARATOR_S);
  }
  if (!open_options) {
    GtkWidget *hbox, *label, *omenu;

    open_options = gtk_frame_new(_("Open Options"));
    gtk_frame_set_shadow_type(GTK_FRAME(open_options), GTK_SHADOW_ETCHED_IN);

    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_container_add(GTK_CONTAINER(open_options), hbox);
    gtk_widget_show(hbox);

    label = gtk_label_new (_("Determine file type:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    open_omenu = omenu = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);
    gtk_widget_show(omenu);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), create_open_menu());

    gtk_box_pack_end(GTK_BOX (GTK_FILE_SELECTION(opendlg)->main_vbox),
                      open_options, FALSE, FALSE, 5);
  }

  gtk_widget_show(open_options);
  gtk_widget_show(opendlg);
}

static void
file_save_as_ok_callback(GtkWidget *w, GtkFileSelection *fs)
{
  const char *filename;
  Diagram *dia;
  struct stat stat_struct;

  dia = gtk_object_get_user_data(GTK_OBJECT(fs));

  filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(fs));

  if (stat(filename, &stat_struct) == 0) {
    GtkWidget *dialog = NULL;
    char buffer[300];
    char *utf8filename = NULL;

    if (!g_utf8_validate(filename, -1, NULL)) {
      utf8filename = g_locale_to_utf8(filename, -1, NULL, NULL, NULL);
      if (utf8filename == NULL) {
	message_warning(_("Some characters in the filename are neither UTF-8 nor your local encoding.\nSome things will break."));
      }
    }
    if (utf8filename == NULL) utf8filename = g_strdup(filename);

    g_snprintf(buffer, 300,
	       _("The file '%s' already exists.\n"
		 "Do you want to overwrite it?"), utf8filename);
    g_free(utf8filename);

    dialog = gtk_message_dialog_new (GTK_WINDOW(fs),
                                     GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_YES_NO,
                                     buffer);
    gtk_window_set_title (GTK_WINDOW (dialog), _("File already exists"));
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES) {
      gtk_widget_hide(savedlg);
      gtk_widget_destroy(GTK_DIALOG (dialog));
      return;
    }
    gtk_widget_destroy(GTK_DIALOG (dialog));
  }

  dia->data->is_compressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compressbutton));

  diagram_update_extents(dia);

  diagram_set_filename(dia, filename);
  diagram_save(dia, filename);

  gtk_widget_hide(savedlg);
}

void
file_save_as_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  if (!savedlg) {
    savedlg = gtk_file_selection_new(_("Save Diagram"));
    gtk_window_set_role(GTK_WINDOW(savedlg), "save_diagram");
    gtk_window_set_position(GTK_WINDOW(savedlg), GTK_WIN_POS_MOUSE);
    /* Need better way to make it a reasonable size.  Isn't there some*/
    /* standard look for them (or is that just Gnome?)*/
    compressbutton = gtk_check_button_new_with_label(_("Compress diagram files"));
    gtk_box_pack_start_defaults(GTK_BOX(GTK_FILE_SELECTION(savedlg)->main_vbox),
				compressbutton);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressbutton),
				 dia->data->is_compressed);
    g_signal_connect(G_OBJECT(compressbutton), "toggled",
		     toggle_compress_callback, NULL);
    gtk_widget_show(compressbutton);
    gtk_tooltips_set_tip(tool_tips, compressbutton,
			 _("Compression reduces file size to less than 1/10th size and speeds up loading and saving.  Some text programs cannot manipulate compressed files."), NULL);
    g_signal_connect_swapped(
		GTK_OBJECT(GTK_FILE_SELECTION(savedlg)->cancel_button),
		"clicked", G_CALLBACK(file_dialog_hide),
		GTK_OBJECT(savedlg));
    g_signal_connect(GTK_OBJECT(savedlg), "delete_event",
		     G_CALLBACK(file_dialog_hide), NULL);
    g_signal_connect(GTK_OBJECT(savedlg), "destroy",
		     G_CALLBACK(gtk_widget_destroyed), &savedlg);
    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(savedlg)->ok_button),
		     "clicked", G_CALLBACK(file_save_as_ok_callback),
		       savedlg);
    gtk_quit_add_destroy(1, GTK_OBJECT(savedlg));
  } else {
    gtk_widget_set_sensitive(savedlg, TRUE);
    g_signal_handlers_block_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressbutton),
				 dia->data->is_compressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
    if (GTK_WIDGET_VISIBLE(savedlg))
      return;
  }
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(savedlg),
				  dia->filename ? dia->filename
				  : "." G_DIR_SEPARATOR_S);
  gtk_object_set_user_data(GTK_OBJECT(savedlg), dia);
  gtk_widget_show(savedlg);
}

void
file_save_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active();

  if (ddisp->diagram->unsaved) {
    file_save_as_callback(data, action, widget);
  } else {
    diagram_update_extents(ddisp->diagram);
    diagram_save(ddisp->diagram, ddisp->diagram->filename);
  }
}

static void
export_set_extension(GtkObject *item)
{
  DiaExportFilter *ef = gtk_object_get_user_data(item);
  GString *s;
  const gchar *text = gtk_entry_get_text(GTK_ENTRY(GTK_FILE_SELECTION(exportdlg)
                                                   ->selection_entry));
  const gchar *last_dot = strrchr(text, '.');

  if (!ef || last_dot == text || text[0] == '\0' || ef->extensions[0] == NULL)
    return;

  s = g_string_new(text);
  if (last_dot)
    g_string_truncate(s, last_dot-text);
  g_string_append(s, ".");
  g_string_append(s, ef->extensions[0]);
  gtk_entry_set_text(GTK_ENTRY(GTK_FILE_SELECTION(exportdlg)->selection_entry),
		     s->str);
  g_string_free (s, TRUE);
}

static GtkWidget *
create_export_menu(void)
{
  GtkWidget *menu;
  GtkWidget *item;
  GList *tmp;

  menu = gtk_menu_new();
  item = gtk_menu_item_new_with_label(_("By extension"));
  gtk_container_add(GTK_CONTAINER(menu), item);
  gtk_widget_show(item);
  item = gtk_menu_item_new();
  gtk_container_add(GTK_CONTAINER(menu), item);
  gtk_widget_show(item);
  
  
  for (tmp = filter_get_export_filters(); tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;

    if (!ef)
      continue;
    item = gtk_menu_item_new_with_label(filter_get_export_filter_label(ef));
    gtk_object_set_user_data(GTK_OBJECT(item), ef);
    g_signal_connect(GTK_OBJECT(item), "activate",
		     G_CALLBACK(export_set_extension), NULL);
    gtk_container_add(GTK_CONTAINER(menu), item);
    gtk_widget_show(item);
  }
  return menu;
}

static void
file_export_ok_callback(GtkWidget *w, GtkFileSelection *fs)
{
  const char *filename;
  Diagram *dia;
  DiaExportFilter *ef;
  struct stat statbuf;

  dia = gtk_object_get_user_data(GTK_OBJECT(fs));
  diagram_update_extents(dia);

  filename = gtk_file_selection_get_filename(fs);

  if (stat(filename, &statbuf) == 0) {
    GtkWidget *dialog = NULL;
    char buffer[300];
    int result;

    g_snprintf(buffer, 300,
	       _("The file '%s' already exists.\n"
		 "Do you want to overwrite it?"), filename);
    dialog = gtk_message_dialog_new (GTK_WINDOW(fs),
                                     GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, 
				     GTK_MESSAGE_QUESTION,
                                     GTK_BUTTONS_YES_NO,
                                     buffer);
    gtk_window_set_title (GTK_WINDOW (dialog), _("File already exists"));
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_YES);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) != GTK_RESPONSE_YES) {
      gtk_widget_hide(exportdlg);
      gtk_widget_destroy(GTK_DIALOG (dialog));
      return;
    }
    gtk_widget_destroy(GTK_DIALOG (dialog));
  }

  ef = gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(export_omenu)
					   ->menu_item));
  if (!ef)
    ef = filter_guess_export_filter(filename);
  if (ef)
      ef->export(dia->data, filename, dia->filename, ef->user_data);
  else
      message_error(_("Could not determine which export filter\n"
		      "to use to save '%s'"), filename);

  gtk_widget_hide(exportdlg);
}

void
file_export_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  GtkWidget *export_menu, *export_item;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  if (!exportdlg) {
    exportdlg = gtk_file_selection_new(_("Export Diagram"));
    gtk_window_set_role(GTK_WINDOW(exportdlg), "export_diagram");
    gtk_window_set_position(GTK_WINDOW(exportdlg), GTK_WIN_POS_MOUSE);
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(exportdlg),
				    dia->filename ? dia->filename
				    : "." G_DIR_SEPARATOR_S);
    g_signal_connect_swapped(
		GTK_OBJECT(GTK_FILE_SELECTION(exportdlg)->cancel_button),
	    "clicked", G_CALLBACK(file_dialog_hide),
		GTK_OBJECT(exportdlg));
    g_signal_connect(GTK_OBJECT(exportdlg), "delete_event",
		     G_CALLBACK(file_dialog_hide), NULL);
    g_signal_connect(GTK_OBJECT(exportdlg), "destroy",
		     G_CALLBACK(gtk_widget_destroyed), &exportdlg);
    g_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(exportdlg)->ok_button),
		     "clicked", G_CALLBACK(file_export_ok_callback),
		       exportdlg);
    gtk_quit_add_destroy(1, GTK_OBJECT(exportdlg));
  }
  if (!export_options) {
    GtkWidget *hbox, *label, *omenu;

    export_options = gtk_frame_new(_("Export Options"));
    gtk_frame_set_shadow_type(GTK_FRAME(export_options), GTK_SHADOW_ETCHED_IN);

    hbox = gtk_hbox_new(FALSE, 1);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_container_add(GTK_CONTAINER(export_options), hbox);
    gtk_widget_show(hbox);

    label = gtk_label_new (_("Determine file type:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    export_omenu = omenu = gtk_option_menu_new();
    gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);
    gtk_widget_show(omenu);

    gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), create_export_menu());

    gtk_box_pack_end(GTK_BOX (GTK_FILE_SELECTION(exportdlg)->main_vbox),
		      export_options, FALSE, FALSE, 5);
  }
 
  gtk_object_set_user_data(GTK_OBJECT(exportdlg), dia);
  gtk_widget_set_sensitive(exportdlg, TRUE);
  if (GTK_WIDGET_VISIBLE(exportdlg))
    return;
  gtk_file_selection_set_filename(GTK_FILE_SELECTION(exportdlg),
				  dia->filename ? dia->filename
				  : "." G_DIR_SEPARATOR_S);
  export_menu = gtk_option_menu_get_menu(GTK_OPTION_MENU(export_omenu));
  export_item = gtk_menu_get_active(GTK_MENU(export_menu));
  if (export_item)
    export_set_extension(GTK_OBJECT(export_item));
  gtk_widget_show(export_options);
  gtk_widget_show(exportdlg);
}
