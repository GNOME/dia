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

#include "filedlg.h"
#include <sys/stat.h>
#include <unistd.h>
#include <gtk/gtk.h>
#include "intl.h"
#include "filter.h"
#include "display.h"

static GtkWidget *exportdlg = NULL, *export_options = NULL, *export_omenu=NULL;

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
export_set_extension(GtkObject *item)
{
  DiaExportFilter *ef = gtk_object_get_user_data(item);
  GString *s;
  gchar *text = gtk_entry_get_text(GTK_ENTRY(GTK_FILE_SELECTION(exportdlg)
					     ->selection_entry));
  gchar *last_dot = strrchr(text, '.');

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
    gtk_signal_connect(GTK_OBJECT(item), "activate",
		       GTK_SIGNAL_FUNC(export_set_extension), NULL);
    gtk_container_add(GTK_CONTAINER(menu), item);
    gtk_widget_show(item);
  }
  return menu;
}

void
file_export_ok_callback(GtkWidget *w, GtkFileSelection *fs)
{
  char *filename;
  Diagram *dia;
  DiaExportFilter *ef;
  struct stat statbuf;

  dia = gtk_object_get_user_data(GTK_OBJECT(fs));
  diagram_update_extents(dia);

  filename = gtk_file_selection_get_filename(fs);

  if (stat(filename, &statbuf) == 0) {
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

  ef = gtk_object_get_user_data(GTK_OBJECT(GTK_OPTION_MENU(export_omenu)
					   ->menu_item));
  if (!ef)
    ef = filter_guess_export_filter(filename);
  if (!ef)
    return; /* do something here */

  ef->export(dia->data, filename);
  gtk_widget_hide(exportdlg);
}

void
file_export_callback(GtkWidget *widget, gpointer user_data)
{
  DDisplay *ddisp;
  Diagram *dia;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  if (!exportdlg) {
    exportdlg = gtk_file_selection_new(_("Export Diagram"));
    gtk_window_set_wmclass(GTK_WINDOW(exportdlg), "export_diagram", "Dia");
    gtk_window_set_position(GTK_WINDOW(exportdlg), GTK_WIN_POS_MOUSE);
    gtk_signal_connect_object(
		GTK_OBJECT(GTK_FILE_SELECTION(exportdlg)->cancel_button),
		"clicked", GTK_SIGNAL_FUNC(file_dialog_hide),
		GTK_OBJECT(exportdlg));
    gtk_signal_connect(GTK_OBJECT(exportdlg), "delete_event",
		       GTK_SIGNAL_FUNC(file_dialog_hide), NULL);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(exportdlg)->ok_button),
		       "clicked", GTK_SIGNAL_FUNC(file_export_ok_callback),
		       exportdlg);
    gtk_quit_add_destroy(1, GTK_OBJECT(exportdlg));
  } else {
    gtk_widget_set_sensitive(exportdlg, TRUE);
    if (GTK_WIDGET_VISIBLE(exportdlg))
      return;
    gtk_file_selection_set_filename(GTK_FILE_SELECTION(exportdlg),
				    dia->filename ? dia->filename
				    : "." G_DIR_SEPARATOR_S);
  }
  if (!export_options) {
    GtkWidget *hbox, *label, *omenu, *menu;

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
  gtk_widget_show(export_options);
  gtk_widget_show(exportdlg);
}
