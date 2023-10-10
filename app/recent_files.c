/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998-2000 Alexander Larsson
 *
 * recent_files.c: recent files menu dia
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

#include <glib/gi18n-lib.h>

#include <gdk/gdkkeysyms.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "dia_dirs.h"
#include "recent_files.h"
#include "menus.h"
#include "diagram.h"
#include "display.h"
#include "interface.h"
#include "layer-editor/layer_dialog.h"
#include "preferences.h"
#include "../lib/filter.h"
#include "message.h"
#include "persistence.h"

static void open_recent_file_callback (GtkWidget *widget, gpointer data);
void recent_file_history_remove (const char *fname);

static void
recent_file_history_clear_menu (void)
{
  menus_clear_recent ();
}


/**
 * recent_file_history_make_menu:
 *
 * Build and insert the recent files menu.
 *
 * Since: dawn-of-time
 */
static void
recent_file_history_make_menu (void)
{
  GList *items;
  GtkActionGroup *group;
  GtkAction *action;
  int i = 0;

  items = persistent_list_get_glist ("recent-files");
  if (!items)
    return; /* on first start this is the usual case */

  group = gtk_action_group_new ("recent-files");
  gtk_action_group_set_translation_domain (group, NULL);

  for (i = 0;
       items != NULL && i < prefs.recent_documents_list_size;
       items = g_list_next(items), i++) {

    gchar *name = g_strdup_printf ("FileRecent_%d", i);
    gchar *file = g_path_get_basename ((const gchar *) items->data);
    gchar **split = g_strsplit (file, "_", -1);
    gchar *file_escaped = g_strjoinv ("__", split);
    gchar *label = g_strdup_printf ("_%d. %s", i + 1, file_escaped);
    gchar *accel = g_strdup_printf ("<control>%d", i + 1);

    action = gtk_action_new (name, label,
			     (const gchar *) items->data,
			     NULL);
    g_signal_connect (G_OBJECT (action), "activate",
		      G_CALLBACK (open_recent_file_callback),
		      items->data);

    gtk_action_group_add_action_with_accel (group, action, accel);

    g_clear_pointer (&name, g_free);
    g_clear_pointer (&file, g_free);
    g_strfreev (split);
    g_clear_pointer (&file_escaped, g_free);
    g_clear_pointer (&label, g_free);
    g_clear_pointer (&accel, g_free);
  }
  menus_set_recent (group);
}


/**
 * recent_file_history_add:
 * @fname: filename to add
 *
 * Add a new item to the file history list.
 * Can also handle the addition of an already existing file. The whole recent menu is rebuild every time but
 * it should be fast enough to favor simplicity of the code.
 *
 * Since: dawn-of-time
 */
void
recent_file_history_add (const char *fname)
{
  char *absname = g_canonicalize_filename (fname, NULL);
  char *filename = g_filename_to_utf8 (absname, -1, NULL, NULL, NULL);

  recent_file_history_clear_menu ();
  persistent_list_add ("recent-files", filename);

  g_clear_pointer (&absname, g_free);
  g_clear_pointer (&filename, g_free);

  recent_file_history_make_menu ();
}


/* load the recent file history */
void
recent_file_history_init (void)
{
  prefs.recent_documents_list_size = CLAMP(prefs.recent_documents_list_size, 0, 16);

  persistence_register_list("recent-files");

  recent_file_history_make_menu();
}


/**
 * recent_file_history_remove:
 * @fname: filename to remove
 *
 * Remove a broken file from the history and update menu accordingly
 *
 * Xing Wang, 2002.06
 *
 * Since: dawn-of-time
 */
void
recent_file_history_remove (const char *fname)
{
  char *absname = g_canonicalize_filename (fname, NULL);
  char *filename = g_filename_to_utf8 (absname, -1, NULL, NULL, NULL);

  recent_file_history_clear_menu ();

  persistent_list_remove ("recent-files", filename);

  g_clear_pointer (&absname, g_free);
  g_clear_pointer (&filename, g_free);

  recent_file_history_make_menu ();
}


static void
open_recent_file_callback(GtkWidget *widget, gpointer data)
{
  DiaImportFilter *ifilter = NULL;
  Diagram *diagram = NULL;
  gchar *filename = g_filename_from_utf8((gchar *)data, -1, NULL, NULL, NULL);

  ifilter = filter_guess_import_filter(filename);

  diagram = diagram_load(filename, ifilter);
  if (diagram != NULL) {
    diagram_update_extents(diagram);
    layer_dialog_set_diagram(diagram);
    if (diagram->displays == NULL) {
      new_display(diagram);
    }
  } else {
    recent_file_history_remove (filename);
  }

  g_clear_pointer (&filename, g_free);
}
