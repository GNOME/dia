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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "dia-file-selector.h"

struct _DiaFileSelector {
  GtkHBox hbox;
  GtkEntry *entry;
  GtkButton *browse;
  GtkWidget *dialog;
  char *sys_filename;
  char *pattern; /* for supported formats */
};

G_DEFINE_TYPE (DiaFileSelector, dia_file_selector, GTK_TYPE_HBOX)

enum {
  DFILE_VALUE_CHANGED,
  DFILE_LAST_SIGNAL
};

static guint dfile_signals[DFILE_LAST_SIGNAL] = { 0 };


static void
dia_file_selector_dispose (GObject *object)
{
  DiaFileSelector *fs = DIA_FILE_SELECTOR (object);

  g_clear_pointer (&fs->dialog, gtk_widget_destroy);
  g_clear_pointer (&fs->sys_filename, g_free);
  g_clear_pointer (&fs->pattern, g_free);

  G_OBJECT_CLASS (dia_file_selector_parent_class)->dispose (object);
}


static void
dia_file_selector_class_init (DiaFileSelectorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dia_file_selector_dispose;

  dfile_signals[DFILE_VALUE_CHANGED] = g_signal_new ("value-changed",
                                                     G_TYPE_FROM_CLASS (klass),
                                                     G_SIGNAL_RUN_FIRST,
                                                     0, NULL, NULL,
                                                     g_cclosure_marshal_VOID__VOID,
                                                     G_TYPE_NONE, 0);
}


static void
dia_file_selector_entry_changed (GtkEditable *editable,
                                 gpointer     data)
{
  DiaFileSelector *fs = DIA_FILE_SELECTOR (data);
  g_signal_emit (fs, dfile_signals[DFILE_VALUE_CHANGED], 0);
}


static void
file_open_response_callback (GtkWidget       *dialog,
                             int              response,
                             DiaFileSelector *fs)
{
  if (response == GTK_RESPONSE_ACCEPT || response == GTK_RESPONSE_OK) {
    char *utf8 = g_filename_to_utf8 (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog)),
                                     -1, NULL, NULL, NULL);

    gtk_entry_set_text (GTK_ENTRY (fs->entry), utf8);

    g_clear_pointer (&utf8, g_free);
  }

  g_clear_pointer (&fs->dialog, gtk_widget_destroy);
}


static void
dia_file_selector_browse_pressed (GtkWidget *widget, gpointer data)
{
  GtkWidget *dialog;
  DiaFileSelector *fs = DIA_FILE_SELECTOR (data);
  char *filename;
  GtkWidget *toplevel = gtk_widget_get_toplevel (widget);

  if (toplevel && !GTK_WINDOW (toplevel)) {
    toplevel = NULL;
  }

  if (fs->dialog == NULL) {
    GtkFileFilter *filter;

    dialog = fs->dialog =
      gtk_file_chooser_dialog_new (_("Select image file"),
                                   toplevel ? GTK_WINDOW (toplevel) : NULL,
                                   GTK_FILE_CHOOSER_ACTION_OPEN,
                                   _("_Cancel"), GTK_RESPONSE_CANCEL,
                                   _("_Open"), GTK_RESPONSE_ACCEPT,
                                   NULL);
    gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_ACCEPT);
    g_signal_connect (G_OBJECT (dialog), "response",
                      G_CALLBACK (file_open_response_callback), fs);
    g_signal_connect (G_OBJECT (fs->dialog), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &fs->dialog);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    if (fs->pattern) {
      gtk_file_filter_add_pattern (filter, fs->pattern);
    } else {
      /* fallback */
      gtk_file_filter_add_pixbuf_formats (filter);
    }
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);

    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (dialog), filter);
  }

  filename = g_filename_from_utf8 (gtk_entry_get_text (fs->entry), -1, NULL, NULL, NULL);
  /* selecting something in the filechooser officially sucks. See e.g. http://bugzilla.gnome.org/show_bug.cgi?id=307378 */
  if (g_path_is_absolute (filename)) {
    gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (fs->dialog), filename);
  }

  g_clear_pointer (&filename, g_free);

  gtk_widget_show (GTK_WIDGET (fs->dialog));
}


static void
dia_file_selector_init (DiaFileSelector *fs)
{
  /* Here's where we set up the real thing */
  fs->dialog = NULL;
  fs->sys_filename = NULL;
  fs->pattern = NULL;

  fs->entry = GTK_ENTRY (gtk_entry_new ());
  gtk_box_pack_start (GTK_BOX (fs), GTK_WIDGET (fs->entry), FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (fs->entry),
                    "changed", G_CALLBACK (dia_file_selector_entry_changed),
                    fs);
  gtk_widget_show (GTK_WIDGET (fs->entry));

  fs->browse = GTK_BUTTON (gtk_button_new_with_label (_("Browse")));
  gtk_box_pack_start (GTK_BOX (fs), GTK_WIDGET (fs->browse), FALSE, TRUE, 0);
  g_signal_connect (G_OBJECT (fs->browse),
                    "clicked", G_CALLBACK (dia_file_selector_browse_pressed),
                    fs);
  gtk_widget_show (GTK_WIDGET (fs->browse));
}


GtkWidget *
dia_file_selector_new (void)
{
  return g_object_new (DIA_TYPE_FILE_SELECTOR, NULL);
}


void
dia_file_selector_set_extensions (DiaFileSelector *fs, const char **exts)
{
  GString *pattern = g_string_new ("*.");
  int i = 0;

  g_clear_pointer (&fs->pattern, g_free);

  while (exts[i] != NULL) {
    if (i != 0) {
      g_string_append (pattern, "|*.");
    }
    g_string_append (pattern, exts[i]);
    ++i;
  }

  fs->pattern = g_string_free (pattern, FALSE);
}


void
dia_file_selector_set_file (DiaFileSelector *fs, char *file)
{
  GError *error = NULL;
  /* filename is in system encoding */
  char *utf8 = g_filename_to_utf8 (file, -1, NULL, NULL, &error);

  gtk_entry_set_text (GTK_ENTRY (fs->entry), utf8);
  if (error) {
    g_warning ("Unable to show filename: %s", error->message);
  }

  g_clear_error (&error);
  g_clear_pointer (&utf8, g_free);
}


const char *
dia_file_selector_get_file (DiaFileSelector *fs)
{
  /* let it behave like gtk_file_selector_get_file */
  g_clear_pointer (&fs->sys_filename, g_free);
  fs->sys_filename = g_filename_from_utf8 (gtk_entry_get_text (GTK_ENTRY (fs->entry)),
                                          -1, NULL, NULL, NULL);
  return fs->sys_filename;
}
