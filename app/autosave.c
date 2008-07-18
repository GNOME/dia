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

/* Autosave */

/* Automatically save a copy with the .autosave extension after idle time.
 * Don't autosave unmodified diagrams, and remove the autosave file when 
 * the diagram is saved successfully.  Also remove autosave file when the
 * diagram is closed, even if it was modified.
 * If (auto)saving crashes you, this will really fuck you over!
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "intl.h"
#include "dia_dirs.h"
#include "diagram.h"
#include "load_save.h"
#include "dialogs.h"
#include <gtk/gtk.h>

gboolean autosave_check_autosave(gpointer data);

static void
autosave_save_diagram(gpointer data)
{
  Diagram *dia = (Diagram *)data;

  g_idle_remove_by_data(data);

  diagram_autosave(dia);
}

/** Makes autosave copies of the diagrams in the appropriate directory
 * This function will be called after a diagram is modified, and will
 * only save documents that are modified and not already autosaved,
 * and only at the next idle period.
 */
gboolean
autosave_check_autosave(gpointer data)
{
  GList *diagrams = dia_open_diagrams();
  Diagram *diagram;

  while (diagrams != NULL) {
    diagram = (Diagram *)diagrams->data;
    if (diagram_is_modified(diagram) && 
	!diagram->autosaved) {
      /* Diagram has been modified.  At next idleness, save it */
      g_idle_add (G_CALLBACK (autosave_save_diagram), diagram);
    }
    diagrams = g_list_next(diagrams);
  }
  return TRUE;
}

/* This is old stuff for a restore dialog */
#if 0
/* Doesn't work with autosave files stored in the files dir */

/** Create a dialog that asks for files to be restore */
static void
autosave_make_restore_dialog(GList *files)
{
  GtkWidget *ok, *cancel;
  GtkWidget *dialog = dialog_make(_("Recovering autosaved diagrams"),
				  NULL, NULL, &ok, &cancel);
  GtkWidget *vbox = GTK_DIALOG(dialog)->vbox;
  GtkWidget *selectarea = gtk_clist_new(1);
  GList *iter;
  gchar **filearray = (gchar**)g_new(gchar *, g_list_length(files)+1);
  int i;

  gtk_box_pack_start_defaults(GTK_BOX(vbox), gtk_label_new(_("Autosaved files exist.\nPlease select those you wish to recover.")));

  for (i = 0, iter = files; iter != NULL; i++, iter = g_list_next(iter)) {
    filearray[i] = (gchar *)iter->data;
  }
  filearray[i] = 0;
  gtk_clist_append(GTK_CLIST(selectarea), filearray);

  gtk_box_pack_start_defaults(GTK_BOX(vbox), selectarea);
  gtk_widget_show_all(dialog);
}

/** If autosave files exist, ask the user if they should be restored
 */
void
autosave_restore_documents(void)
{
  gchar *savedir = dia_config_filename("autosave" G_DIR_SEPARATOR_S);
  GDir *dir = g_dir_open(savedir, 0, NULL);
  const char *ent;
  GList *files = NULL;

  if (dir == NULL) return;
  while ((ent = g_dir_read_name(dir)) != NULL) {
    printf("Found autosave file %s\n", ent);
    files = g_list_prepend(files, g_strdup(ent));
  }

  if (files != NULL) {
    autosave_make_restore_dialog(files);
  }
  g_dir_close(dir);
  g_free(savedir);
  g_list_free(files);
}
#endif
