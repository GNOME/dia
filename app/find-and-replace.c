/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * find-and-replace.c - common functionality applied to diagram
 *
 * Copyright (C) 2008 Hans Breuer
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

#include <gtk/gtk.h>

#include "intl.h"

#include "diagram.h"
#include "display.h"
#include "object.h"

#include "find-and-replace.h"

enum {
  RESPONSE_FIND = -20,
  RESPONSE_REPLACE = -21,
  RESPONSE_REPLACE_ALL = -23
};

typedef struct _SearchData {
  const gchar *key;
} SearchData;

static void
find_func (DiaObject *obj, gpointer user_data)
{
  SearchData *data = (SearchData *)user_data;

  gchar* name = object_get_displayname (obj);
  if (strstr (name, data->key) != NULL)
    g_print ("%s\n", name);
  g_free (name);
}

static gint
fnr_respond (GtkWidget *widget, gint response_id, gpointer data)
{
  const gchar *search = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "search-entry")); 
  const gchar *replace;
  DDisplay *ddisp = (DDisplay*)data;
  GList *list;
  SearchData sd;

  switch (response_id) {
  case RESPONSE_FIND :
    sd.key = search;
    data_foreach_object (ddisp->diagram->data, find_func, &sd);
    break;
  case RESPONSE_REPLACE :
    replace = gtk_entry_get_text (g_object_get_data (G_OBJECT (widget), "replace-entry"));
    g_print ("Replace: %s\n", replace);
    break;
  case RESPONSE_REPLACE_ALL :
    break;
  default:
    gtk_widget_hide (widget);
  }
  return 0;
}

static void
fnr_dialog_setup_common (GtkWidget *dialog, gboolean is_replace, DDisplay *ddisp)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *search_entry;
  GtkWidget *match_case;
  GtkWidget *match_word;

  gtk_dialog_set_default_response (GTK_DIALOG (dialog), RESPONSE_FIND);

  /* don't destroy dialog when window manager close button pressed */
  g_signal_connect(G_OBJECT (dialog), "response",
		   G_CALLBACK(fnr_respond), ddisp);
  g_signal_connect(G_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_widget_hide), NULL);
  g_signal_connect(GTK_OBJECT(dialog), "delete_event",
		   G_CALLBACK(gtk_true), NULL);
  g_signal_connect(GTK_OBJECT(dialog), "destroy",
		   G_CALLBACK(gtk_widget_destroyed), &dialog);

  vbox = GTK_DIALOG(dialog)->vbox;

  hbox = gtk_hbox_new (FALSE, 12);
  label = gtk_label_new_with_mnemonic (_("_Search for:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  search_entry = gtk_entry_new ();
  g_object_set_data (G_OBJECT (dialog), "search-entry", search_entry);
  gtk_label_set_mnemonic_widget (GTK_LABEL (label), search_entry);
  gtk_entry_set_width_chars (GTK_ENTRY (search_entry), 30);
  gtk_box_pack_start (GTK_BOX (hbox), search_entry, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  
  if (is_replace) {
    GtkWidget *replace_entry;

    hbox = gtk_hbox_new (FALSE, 12);
    label = gtk_label_new_with_mnemonic (_("Replace _with:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
    replace_entry = gtk_entry_new ();
    g_object_set_data (G_OBJECT (dialog), "replace-entry", replace_entry);
    gtk_label_set_mnemonic_widget (GTK_LABEL (label), replace_entry);
    gtk_entry_set_width_chars (GTK_ENTRY (replace_entry), 30);
    gtk_box_pack_start (GTK_BOX (hbox), replace_entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 6);
  }

  match_case = gtk_check_button_new_with_mnemonic (_("_Match case"));
  gtk_box_pack_start (GTK_BOX (vbox), match_case, FALSE, FALSE, 6);

  match_word = gtk_check_button_new_with_mnemonic (_("Match _entire word only"));
  gtk_box_pack_start (GTK_BOX (vbox), match_word, FALSE, FALSE, 6);

  gtk_widget_show_all (vbox);
}

/**
 * React to <Display>/Edit/Find
 */
void
edit_find_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  /* no static var, instead we are attaching the dialog to the diplay shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-find-dialog");
  if (!dialog) {
    dialog = gtk_dialog_new_with_buttons (
		_("Find"), 
		GTK_WINDOW (ddisp->shell), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		GTK_STOCK_FIND, RESPONSE_FIND,
		NULL);

    fnr_dialog_setup_common (dialog, FALSE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-find-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));  
}

/**
 * React to <Display>/Edit/Replace
 */
void
edit_replace_callback(gpointer data, guint action, GtkWidget *widget)
{
  DDisplay *ddisp;
  Diagram *dia;
  GtkWidget *dialog;

  ddisp = ddisplay_active();
  if (!ddisp) return;
  dia = ddisp->diagram;

  /* no static var, instead we are attaching the dialog to the diplay shell */
  dialog = g_object_get_data (G_OBJECT (ddisp->shell), "edit-replace-dialog");
  if (!dialog) {
    dialog = gtk_dialog_new_with_buttons (
		_("Replace"), 
		GTK_WINDOW (ddisp->shell), GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
		_("Replace _All"), RESPONSE_REPLACE_ALL,
		GTK_STOCK_FIND_AND_REPLACE, RESPONSE_REPLACE,
		GTK_STOCK_FIND, RESPONSE_FIND,
		NULL);
    
    fnr_dialog_setup_common (dialog, TRUE, ddisp);
  }
  g_object_set_data (G_OBJECT (ddisp->shell), "edit-replace-dialog", dialog);

  gtk_dialog_run (GTK_DIALOG (dialog));  
}

