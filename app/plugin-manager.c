/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * plugin-manager.h: the dia plugin manager.
 * Copyright (C) 2000 James Henstridge
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#ifdef GNOME
#  include <gnome.h>
#endif

#include "plugin-manager.h"
#include "intl.h"
#include "plug-ins.h"

typedef struct {
  GtkWidget *window;
  GtkWidget *list;
  GtkWidget *name_label;
  GtkWidget *file_label;
  GtkWidget *description_label;
  GtkWidget *loaded_label;
  GtkWidget *autoload_cbutton;
  GtkWidget *load_button;
  GtkWidget *unload_button;

  PluginInfo *info;
} PluginManager;

static void
clist_select_row (GtkCList *clist, gint row, gint col, GdkEvent *event,
		  PluginManager *pm)
{
	pm->info = gtk_clist_get_row_data(clist, row);

	gtk_label_set_text (GTK_LABEL (pm->name_label),
			    dia_plugin_get_name (pm->info));
	gtk_label_set_text( GTK_LABEL(pm->file_label),
			    dia_plugin_get_filename (pm->info));
	gtk_label_set_text (GTK_LABEL (pm->description_label),
			    dia_plugin_get_description (pm->info));
	gtk_label_set_text (GTK_LABEL (pm->loaded_label),
			    dia_plugin_is_loaded (pm->info) ? _("yes") : _("no"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (pm->autoload_cbutton),
				      !dia_plugin_get_inhibit_load (pm->info));
	gtk_widget_set_sensitive (pm->autoload_cbutton, TRUE);

	if (dia_plugin_is_loaded (pm->info)) {
		gtk_widget_set_sensitive (pm->load_button, FALSE);
		if (dia_plugin_can_unload (pm->info))
			gtk_widget_set_sensitive (pm->unload_button, TRUE);
		else
			gtk_widget_set_sensitive (pm->unload_button, FALSE);
	} else {
		gtk_widget_set_sensitive (pm->load_button, TRUE);
		gtk_widget_set_sensitive (pm->unload_button, FALSE);
	}
}

static void
autoload_toggled(GtkToggleButton *toggle, PluginManager *pm)
{
  if (pm->info) {
    dia_plugin_set_inhibit_load(pm->info,
				!gtk_toggle_button_get_active(toggle));
  }
}

static void
load_clicked(GtkButton *button, PluginManager *pm)
{
  if (pm->info) {
    dia_plugin_load(pm->info);
    if (dia_plugin_is_loaded(pm->info)) {
      gtk_widget_set_sensitive(pm->load_button, FALSE);
      if (dia_plugin_can_unload(pm->info))
	gtk_widget_set_sensitive(pm->unload_button, TRUE);
      else
	gtk_widget_set_sensitive(pm->unload_button, FALSE);
    }
  }
}

static void
unload_clicked(GtkButton *button, PluginManager *pm)
{
  if (pm->info) {
    dia_plugin_unload(pm->info);
    if (!dia_plugin_is_loaded(pm->info)) {
      gtk_widget_set_sensitive(pm->load_button, TRUE);
      gtk_widget_set_sensitive(pm->unload_button, FALSE);
    }
  }
}

static PluginManager *
get_plugin_manager(void)
{
  static gboolean initialised = FALSE;
  static PluginManager pm;
  GtkWidget *vbox, *hbox, *table, *label, *wid;
  GList *tmp;

  if (initialised)
    return &pm;
  initialised = TRUE;

  /* build up the user interface */
#ifdef GNOME
  pm.window = gnome_dialog_new(_("Plug-ins"), GNOME_STOCK_BUTTON_CLOSE, NULL);
  gnome_dialog_close_hides(GNOME_DIALOG(pm.window), TRUE);
  gnome_dialog_set_close(GNOME_DIALOG(pm.window), TRUE);
  vbox = GNOME_DIALOG(pm.window)->vbox;
#else
  pm.window = gtk_dialog_new();
  gtk_window_set_title(GTK_WINDOW(pm.window), _("Plug-ins"));
  gtk_container_set_border_width(GTK_CONTAINER(pm.window), 2);
  gtk_window_set_policy(GTK_WINDOW(pm.window), FALSE, TRUE, FALSE);
  vbox = GTK_DIALOG(pm.window)->vbox;

  /* don't destroy dialog when window manager close button pressed */
  gtk_signal_connect(GTK_OBJECT(pm.window), "delete_event",
                     GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);
  gtk_signal_connect(GTK_OBJECT(pm.window), "delete_event",
		     GTK_SIGNAL_FUNC(gtk_true), NULL);

  wid = gtk_button_new_with_label(_("Close"));
  GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(pm.window)->action_area), wid,
		     TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT(wid), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),
			    GTK_OBJECT(pm.window));
#endif

  gtk_window_set_policy(GTK_WINDOW(pm.window), FALSE, TRUE, FALSE);

  hbox = gtk_hbox_new(FALSE, 5);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show(hbox);

  wid = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(wid),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start(GTK_BOX(hbox), wid, TRUE, TRUE, 0);
  gtk_widget_show(wid);

  /* create a clist that acts like a list */
  pm.list = gtk_clist_new(1);
  gtk_clist_set_column_auto_resize(GTK_CLIST(pm.list), 0, TRUE);
  gtk_clist_set_selection_mode(GTK_CLIST(pm.list), GTK_SELECTION_BROWSE);
  gtk_clist_set_sort_column(GTK_CLIST(pm.list), 0);
  gtk_clist_set_auto_sort(GTK_CLIST(pm.list), TRUE);
  gtk_container_add(GTK_CONTAINER(wid), pm.list);
  gtk_widget_show(pm.list);

  table = gtk_table_new(6, 2, FALSE);
  gtk_table_set_col_spacings(GTK_TABLE(table), 3);
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
  gtk_widget_show(table);

  label = gtk_label_new(_("Name:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 0,1,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  pm.name_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(pm.name_label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), pm.name_label, 1,2, 0,1,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(pm.name_label);

  label = gtk_label_new(_("File name:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 1,2,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  pm.file_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(pm.file_label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), pm.file_label, 1,2, 1,2,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(pm.file_label);

  label = gtk_label_new(_("Description:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 2,3,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  pm.description_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(pm.description_label), 0.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(pm.description_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL(pm.description_label), TRUE);
  gtk_table_attach(GTK_TABLE(table), pm.description_label, 1,2, 2,3,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(pm.description_label);

  label = gtk_label_new(_("Loaded:"));
  gtk_misc_set_alignment(GTK_MISC(label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), label, 0,1, 3,4,
		   GTK_FILL, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(label);

  pm.loaded_label = gtk_label_new("");
  gtk_misc_set_alignment(GTK_MISC(pm.loaded_label), 0.0, 0.5);
  gtk_table_attach(GTK_TABLE(table), pm.loaded_label, 1,2, 3,4,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(pm.loaded_label);

  pm.autoload_cbutton = gtk_check_button_new_with_label(
					_("Autoload at startup"));
  gtk_widget_set_sensitive(pm.autoload_cbutton, FALSE);
  gtk_table_attach(GTK_TABLE(table), pm.autoload_cbutton, 0,2, 4,5,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(pm.autoload_cbutton);

  hbox = gtk_hbox_new(TRUE, 3);
  gtk_table_attach(GTK_TABLE(table), hbox, 0,2, 5,6,
		   GTK_FILL|GTK_EXPAND, GTK_FILL|GTK_EXPAND, 0, 0);
  gtk_widget_show(hbox);

  pm.load_button = gtk_button_new_with_label(_("Load"));
  gtk_widget_set_sensitive(pm.load_button, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), pm.load_button, TRUE, TRUE, 0);
  gtk_widget_show(pm.load_button);

  pm.unload_button = gtk_button_new_with_label(_("Unload"));
  gtk_widget_set_sensitive(pm.unload_button, FALSE);
  gtk_box_pack_start(GTK_BOX(hbox), pm.unload_button, TRUE, TRUE, 0);
  gtk_widget_show(pm.unload_button);

  pm.info = NULL;

  /* fill list */
  for (tmp = dia_list_plugins(); tmp != NULL; tmp = tmp->next) {
    gchar *row[1];
    gint num;
    PluginInfo *info = tmp->data;

    row[0] = (gchar *)dia_plugin_get_name(info);
    num = gtk_clist_append(GTK_CLIST(pm.list), row);
    gtk_clist_set_row_data(GTK_CLIST(pm.list), num, info);
  }
  gtk_widget_set_usize(pm.list->parent, 150, -1);
  /* setup callbacks */
  gtk_signal_connect(GTK_OBJECT(pm.list), "select_row",
		     GTK_SIGNAL_FUNC(clist_select_row), &pm);
  if (dia_list_plugins())
    gtk_clist_select_row(GTK_CLIST(pm.list), 0, 0);

  gtk_signal_connect(GTK_OBJECT(pm.autoload_cbutton), "toggled",
		     GTK_SIGNAL_FUNC(autoload_toggled), &pm);
  gtk_signal_connect(GTK_OBJECT(pm.load_button), "clicked",
		     GTK_SIGNAL_FUNC(load_clicked), &pm);
  gtk_signal_connect(GTK_OBJECT(pm.unload_button), "clicked",
		     GTK_SIGNAL_FUNC(unload_clicked), &pm);

  return &pm;
}

void
file_plugins_callback(gpointer data, guint action, GtkWidget *widget)
{
  PluginManager *pm = get_plugin_manager();

  gtk_widget_show(pm->window);
}
