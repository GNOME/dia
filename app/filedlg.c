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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "filter.h"
#include "dia_dirs.h"
#include "persistence.h"
#include "display.h"
#include "message.h"
#include "layer-editor/layer_dialog.h"
#include "load_save.h"
#include "preferences.h"
#include "interface.h"
#include "recent_files.h"
#include "confirm.h"
#include "diacontext.h"

#include "filedlg.h"

static GtkWidget *opendlg = NULL;
static GtkWidget *savedlg = NULL;
static GtkWidget *exportdlg = NULL;

static void
toggle_compress_callback(GtkWidget *widget)
{
  /* Changes prefs exactly when the user toggles the setting, i.e.
   * the setting really remembers what the user chose last time, but
   * lets diagrams of the opposite kind stay that way unless the user
   * intervenes.
   */
  prefs.new_diagram.compress_save =
    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


/**
 * ifilter_by_index:
 * @index: the index of the #DiaImportFilter to get
 * @filename: filename to guess a filter for
 *
 * Given an import filter index and optionally a filename for fallback
 * return the import filter to use
 *
 * Since: dawn-of-time
 */
static DiaImportFilter *
ifilter_by_index (int index, const char* filename)
{
  DiaImportFilter *ifilter = NULL;

  if (index >= 0)
    ifilter = g_list_nth_data (filter_get_import_filters(), index);
  else if (filename) /* fallback, should not happen */
    ifilter = filter_guess_import_filter(filename);

  return ifilter;
}

typedef void* (* FilterGuessFunc) (const gchar* filename);


/**
 * matching_extensions_filter:
 * @fi: the #GtkFileFilterInfo
 * @data: the filter function
 *
 * Respond to the file chooser filter facility, that is match
 * the extension of a given filename to the selected filter
 *
 * Since: dawn-of-time
 */
static gboolean
matching_extensions_filter (const GtkFileFilterInfo* fi,
                            gpointer                 data)
{
  FilterGuessFunc guess_func = (FilterGuessFunc) data;

  g_assert (guess_func);

  if (!fi->filename)
    return 0; /* filter it, IMO should not happen --hb */

  if (guess_func (fi->filename))
     return 1;

  return 0;
}


/**
 * diagram_removed:
 * @dia: the #Diagram
 * @dialog: the dialogue
 *
 * React on the diagram::removed signal by destroying the dialog
 *
 * This function isn't used cause it conflicts with the pattern introduced:
 * instead of destroying the dialog with the diagram, the dialog is keeping
 * a reference to it. As a result we could access the diagram even when the
 * display of it is gone ...
 */
#if 0
static void
diagram_removed (Diagram* dia, GtkWidget* dialog)
{
  g_return_if_fail (DIA_IS_DIAGRAM (dia));
  g_return_if_fail (GTK_IS_WIDGET (dialog));

  gtk_widget_destroy (dialog);
}
#endif

static GtkFileFilter *
build_gtk_file_filter_from_index (int index)
{
  DiaImportFilter *ifilter = NULL;
  GtkFileFilter *filter = NULL;

  ifilter = g_list_nth_data (filter_get_import_filters(), index-1);
  if (ifilter) {
    GString *pattern = g_string_new ("*.");
    int i = 0;

    filter = gtk_file_filter_new ();

    while (ifilter->extensions[i] != NULL) {
      if (i != 0)
        g_string_append (pattern, "|*.");
      g_string_append (pattern, ifilter->extensions[i]);
      ++i;
    }
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    gtk_file_filter_add_pattern (filter, pattern->str);

    g_string_free (pattern, TRUE);

  } else {
    /* match the other selections extension */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    gtk_file_filter_add_custom (filter, GTK_FILE_FILTER_FILENAME,
                                matching_extensions_filter, filter_guess_import_filter, NULL);
  }
  return filter;
}

static void
import_adapt_extension_callback(GtkWidget *widget)
{
  int index = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  GtkFileFilter *former = NULL;
  GSList *list, *elem;

  list = gtk_file_chooser_list_filters (GTK_FILE_CHOOSER (opendlg));
  for (elem = list; elem != NULL; elem = g_slist_next (elem))
    if (strcmp (_("Supported Formats"), gtk_file_filter_get_name (GTK_FILE_FILTER(elem->data))) == 0)
      former = GTK_FILE_FILTER(elem->data);
  g_slist_free (list);

  if (former) {
    /* replace the previous filter */
    GtkFileFilter *filter = build_gtk_file_filter_from_index (index);
    gtk_file_chooser_remove_filter (GTK_FILE_CHOOSER (opendlg), former);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);
    gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (opendlg), filter);
  }
}


/**
 * create_open_menu:
 *
 * Create the combobox menu to select Import Filter options
 *
 * Since: dawn-of-time
 */
static GtkWidget *
create_open_menu (void)
{
  GtkWidget *menu;
  GList *tmp;


  menu = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(menu), _("By extension"));

  for (tmp = filter_get_import_filters(); tmp != NULL; tmp = tmp->next) {
    DiaImportFilter *ifilter = tmp->data;
    gchar *filter_label;

    if (!ifilter)
      continue;
    filter_label = filter_get_import_filter_label(ifilter);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(menu), filter_label);
    g_clear_pointer (&filter_label, g_free);
  }
  g_signal_connect (G_OBJECT (menu), "changed",
                    G_CALLBACK (import_adapt_extension_callback), NULL);
  return menu;
}


/**
 * file_open_response_callback:
 * @fs: the #GtkFileChooser
 * @response: the response
 * @user_data: the user data
 *
 * Respond to the user finishing the Open Dialog either accept or cancel/destroy
 *
 * Since: dawn-of-time
 */
static void
file_open_response_callback (GtkWidget *fs,
                             int        response,
                             gpointer   user_data)
{
  char *filename;
  Diagram *diagram = NULL;

  if (response == GTK_RESPONSE_ACCEPT) {
    int index = gtk_combo_box_get_active (GTK_COMBO_BOX (user_data));

    if (index >= 0) /* remember it */
      persistence_set_integer ("import-filter", index);
    filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(fs));

    diagram = diagram_load(filename, ifilter_by_index (index - 1, filename));

    g_clear_pointer (&filename, g_free);

    if (diagram != NULL) {
      diagram_update_extents(diagram);
      layer_dialog_set_diagram(diagram);

      if (diagram->displays == NULL) {
/*	GSList *displays = diagram->displays;
	GSList *displays_head = displays;
	diagram->displays = NULL;
	for (; displays != NULL; displays = g_slist_next(displays)) {
	  DDisplay *loaded_display = (DDisplay *)displays->data;
	  copy_display(loaded_display);
	  g_clear_pointer (&loaded_display, g_free);
	}
	g_slist_free(displays_head);
      } else {
*/
	new_display(diagram);
      }
    }
  }
  gtk_widget_destroy(opendlg);
}


/**
 * file_open_callback:
 * @action: the #GtkAction
 *
 * Handle menu click File/Open
 *
 * This is either with or without diagram
 *
 * Since: dawn-of-time
 */
void
file_open_callback (GtkAction *action)
{
  if (!opendlg) {
    DDisplay *ddisp;
    Diagram *dia = NULL;
    GtkWindow *parent_window;
    gchar *filename = NULL;

    /* FIXME: we should not use ddisp_active but instead get the current diagram
     * from caller. Thus we could offer the option to "load into" if invoked by
     * <Display/File/Open. It wouldn't make any sense if invoked by
     * <Toolbox>/File/Open ...
     */
    ddisp = ddisplay_active();
    if (ddisp) {
      dia = ddisp->diagram;
      parent_window = GTK_WINDOW(ddisp->shell);
    } else {
      parent_window = GTK_WINDOW(interface_get_toolbox_shell());
    }
    persistence_register_integer ("import-filter", 0);
    opendlg = gtk_file_chooser_dialog_new (_("Open Diagram"),
                                           parent_window,
                                           GTK_FILE_CHOOSER_ACTION_OPEN,
                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Open"), GTK_RESPONSE_ACCEPT,
                                           NULL);
    /* is activating gvfs really that easy - at least it works for samba shares*/
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER(opendlg), FALSE);
    gtk_dialog_set_default_response(GTK_DIALOG(opendlg), GTK_RESPONSE_ACCEPT);
    gtk_window_set_role(GTK_WINDOW(opendlg), "open_diagram");
    if (dia && dia->filename) {
      filename = g_filename_from_utf8 (dia->filename, -1, NULL, NULL, NULL);
    }
    if (filename != NULL) {
      char *fnabs = g_canonicalize_filename (filename, NULL);
      if (fnabs) {
        gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (opendlg), fnabs);
      }
      g_clear_pointer (&fnabs, g_free);
      g_clear_pointer (&filename, g_free);
    }
    g_signal_connect (G_OBJECT (opendlg), "destroy",
                      G_CALLBACK (gtk_widget_destroyed), &opendlg);
  } else {
    gtk_widget_set_sensitive (opendlg, TRUE);
    if (gtk_widget_get_visible (opendlg))
      return;
  }
  if (!gtk_file_chooser_get_extra_widget (GTK_FILE_CHOOSER (opendlg))) {
    GtkWidget *hbox, *label, *omenu, *options;
    GtkFileFilter* filter;

    options = gtk_frame_new(_("Open Options"));
    gtk_frame_set_shadow_type(GTK_FRAME(options), GTK_SHADOW_ETCHED_IN);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
    gtk_container_add(GTK_CONTAINER(options), hbox);
    gtk_widget_show(hbox);

    label = gtk_label_new (_("Determine file type:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    omenu = create_open_menu();
    gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 0);
    gtk_widget_show(omenu);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(opendlg),
				      options);

    gtk_widget_show(options);
    g_signal_connect(G_OBJECT(opendlg), "response",
		     G_CALLBACK(file_open_response_callback), omenu);

    /* set up the gtk file (name) filters */
    /* 0 = by extension */
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg),
	                         build_gtk_file_filter_from_index (0));
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (opendlg), filter);

    gtk_combo_box_set_active (GTK_COMBO_BOX (omenu), persistence_get_integer ("import-filter"));
  }

  gtk_widget_show(opendlg);
}


/**
 * file_save_as_response_callback:
 * @fs: the #GtkFileChooser
 * @response: the response
 * @user_data: the user data
 *
 * Respond to a button press (also destroy) in the save as dialog.
 *
 * Since: dawn-of-time
 */
static void
file_save_as_response_callback (GtkWidget *fs,
                                int        response,
                                gpointer   user_data)
{
  DiaContext *ctx = NULL;
  GFile *file = NULL;
  Diagram *dia;

  if (response == GTK_RESPONSE_ACCEPT) {
    dia = g_object_get_data (G_OBJECT (fs), "user_data");

    file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (fs));
    if (!file) {
      /* Not getting a filename looks like a contract violation in Gtk+ to me.
       * Still Dia would be crashing (bug #651949) - instead simply go back to the dialog. */
      gtk_window_present (GTK_WINDOW (fs));
      return;
    }

    dia->data->is_compressed =
        gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (user_data));

    diagram_update_extents (dia);

    ctx = dia_context_new (_("Save As"));

    dia_diagram_set_file (dia, file);

    dia_context_set_filename (ctx, g_file_peek_path (file));

    if (diagram_save (dia, g_file_peek_path (file), ctx)) {
      recent_file_history_add (g_file_peek_path (file));
    }

    dia_context_release (ctx);
    g_clear_object (&file);
  }

  /* if we have our own reference, drop it before destroy */
  g_object_set_data (G_OBJECT (fs), "user_data", NULL);

  /* if we destroy it gtk_dialog_run wont give the response */
  if (!g_object_get_data (G_OBJECT (fs), "dont-destroy")) {
    gtk_widget_destroy (GTK_WIDGET (fs));
  }
}

static GtkWidget *file_save_as_dialog_prepare (Diagram *dia, DDisplay *ddisp);


/**
 * file_save_as_callback:
 * @action: the #GtkAction
 *
 * Respond to the File/Save As.. menu
 *
 * We have only one file save dialog at a time. So if the dialog already exists
 * and the user tries to Save as once more only the diagram referenced will
 * change. Maybe we should also indicate the referenced diagram in the dialog.
 *
 * Since: dawn-of-time
 */
void
file_save_as_callback (GtkAction *action)
{
  DDisplay  *ddisp;
  Diagram   *dia;
  GtkWidget *dlg;

  ddisp = ddisplay_active ();
  if (!ddisp) return;
  dia = ddisp->diagram;

  dlg = file_save_as_dialog_prepare (dia, ddisp);

  gtk_widget_show (dlg);
}


gboolean
file_save_as(Diagram *dia, DDisplay *ddisp)
{
  GtkWidget *dlg;
  int response;

  dlg = file_save_as_dialog_prepare(dia, ddisp);

  /* if we destroy it gtk_dialog_run wont give the response */
  g_object_set_data (G_OBJECT(dlg), "dont-destroy", GINT_TO_POINTER (1));
  response = gtk_dialog_run(GTK_DIALOG(dlg));
  gtk_widget_destroy(GTK_WIDGET(dlg));

  return (GTK_RESPONSE_ACCEPT == response);
}


static GtkWidget *
file_save_as_dialog_prepare (Diagram *dia, DDisplay *ddisp)
{
  char *filename = NULL;

  if (!savedlg) {
    GtkWidget *compressbutton;

    savedlg = gtk_file_chooser_dialog_new (_("Save Diagram"),
                                           GTK_WINDOW (ddisp->shell),
                                           GTK_FILE_CHOOSER_ACTION_SAVE,
                                           _("_Cancel"), GTK_RESPONSE_CANCEL,
                                           _("_Save"), GTK_RESPONSE_ACCEPT,
                                           NULL);
    /* vfs saving is as easy - if you see 'bad file descriptor' there is
     * something wrong with the permissions of the share ;) */
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (savedlg), FALSE);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (savedlg), TRUE);
    gtk_window_set_role (GTK_WINDOW (savedlg), "save_diagram");

    /* Need better way to make it a reasonable size.  Isn't there some*/
    /* standard look for them (or is that just Gnome?)*/
    compressbutton = gtk_check_button_new_with_label (_("Compress diagram files"));
    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (savedlg),
                                       compressbutton);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (compressbutton),
                                  dia->data->is_compressed);
    g_signal_connect (G_OBJECT (compressbutton),
                      "toggled",
                      G_CALLBACK (toggle_compress_callback),
                      NULL);
    gtk_widget_show (compressbutton);
    gtk_widget_set_tooltip_text (compressbutton,
                                 _("Compression reduces file size to less than 1/10th "
                                   "size and speeds up loading and saving.  Some text "
                                   "programs cannot manipulate compressed files."));
    g_signal_connect (GTK_FILE_CHOOSER (savedlg),
                      "response",
                      G_CALLBACK (file_save_as_response_callback),
                      compressbutton);
    g_signal_connect (G_OBJECT (savedlg),
                      "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &savedlg);
  } else {
    GtkWidget *compressbutton = gtk_file_chooser_get_extra_widget(GTK_FILE_CHOOSER(savedlg));
    gtk_widget_set_sensitive(savedlg, TRUE);
    g_signal_handlers_block_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compressbutton),
				 dia->data->is_compressed);
    g_signal_handlers_unblock_by_func(G_OBJECT(compressbutton), toggle_compress_callback, NULL);
    if (gtk_widget_get_visible (savedlg)) {
      /* keep a reference to the diagram */
      g_object_set_data_full (G_OBJECT (savedlg),
                              "user_data",
                              g_object_ref (dia),
                              g_object_unref);
      gtk_window_present (GTK_WINDOW (savedlg));
      return savedlg;
    }
  }

  if (dia && dia->filename) {
    filename = g_filename_from_utf8 (dia->filename, -1, NULL, NULL, NULL);
  }

  if (filename != NULL) {
    char *fnabs = g_canonicalize_filename (filename, NULL);
    if (fnabs) {
      char *base = g_path_get_basename (dia->filename);
      gtk_file_chooser_set_filename (GTK_FILE_CHOOSER (savedlg), fnabs);
      /* FileChooser api insist on exiting files for set_filename  */
      /* ... and does not use filename encoding on this one. */
      gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (savedlg), base);
      g_clear_pointer (&base, g_free);
    }
    g_clear_pointer (&fnabs, g_free);
    g_clear_pointer (&filename, g_free);
  }

  g_object_set_data_full (G_OBJECT (savedlg),
                          "user_data",
                          g_object_ref (dia),
                          g_object_unref);

  return savedlg;
}


/*
 * file_save_callback:
 * @action: the #GtkAction
 *
 * Respond to the File/Save menu entry.
 *
 * Delegates to Save As if there is no filename set yet.
 *
 * Since: dawn-of-time
 */
void
file_save_callback (GtkAction *action)
{
  Diagram *diagram;

  diagram = ddisplay_active_diagram ();
  if (!diagram) return;

  if (diagram->unsaved) {
    file_save_as_callback (action);
  } else {
    char *filename = g_filename_from_utf8 (diagram->filename,
                                           -1,
                                           NULL,
                                           NULL,
                                           NULL);
    DiaContext *ctx = dia_context_new (_("Save"));

    diagram_update_extents (diagram);
    if (diagram_save (diagram, filename, ctx)) {
      recent_file_history_add (filename);
    }

    g_clear_pointer (&filename, g_free);

    dia_context_release (ctx);
  }
}


/*
 * efilter_by_index:
 * @index: filter index
 * @ext: (out): the extension of the output
 *
 * Given an export filter index return the export filter to use
 */
static DiaExportFilter *
efilter_by_index (int index, const char **ext)
{
  DiaExportFilter *efilter = NULL;

  /* the index in the selection list *is* the index of the filter,
   * filters supporting multiple formats are multiple times in the list */
  if (index >= 0) {
    efilter = g_list_nth_data (filter_get_export_filters(), index);
    if (efilter) {
      if (ext) {
        *ext = efilter->extensions[0];
      }
      return efilter;
    } else {
      /* getting here means invalid index */
      g_warning ("efilter_by_index() index=%d out of range", index);
    }
  }

  return efilter;
}


/**
 * export_adapt_extension:
 * @name: the filename
 * @index: the index of the filter
 *
 * Adapt the filename to the export filter index
 *
 * Since: dawn-of-time
 */
static void
export_adapt_extension (const char* name, int index)
{
  const gchar* ext = NULL;
  DiaExportFilter *efilter = efilter_by_index (index, &ext);
  gchar *basename = g_path_get_basename (name);
  gchar *utf8_name = NULL;

  if (!efilter || !ext)
    utf8_name = g_filename_to_utf8 (basename, -1, NULL, NULL, NULL);
  else {
    const gchar *last_dot = strrchr(basename, '.');
    GString *s = g_string_new(basename);
    if (last_dot)
      g_string_truncate(s, last_dot-basename);
    g_string_append(s, ".");
    g_string_append(s, ext);
    utf8_name = g_filename_to_utf8 (s->str, -1, NULL, NULL, NULL);
    g_string_free (s, TRUE);
  }
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (exportdlg), utf8_name);
  g_clear_pointer (&utf8_name, g_free);
  g_clear_pointer (&basename, g_free);
}


static void
export_adapt_extension_callback(GtkWidget *widget)
{
  int index = gtk_combo_box_get_active (GTK_COMBO_BOX(widget));
  gchar *name = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(exportdlg));

  if (name && index > 0) /* Ignore "By Extension" */
    export_adapt_extension (name, index - 1);
  g_clear_pointer (&name, g_free);
}


/**
 * create_export_menu:
 *
 * Create a new "option menu" for the export options
 */
static GtkWidget *
create_export_menu (void)
{
  GtkWidget *menu;
  GList *tmp;

  menu = gtk_combo_box_text_new ();
  gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (menu), _("By extension"));

  for (tmp = filter_get_export_filters (); tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;
    gchar *filter_label;

    if (!ef)
      continue;

    filter_label = filter_get_export_filter_label (ef);
    gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT(menu), filter_label);
    g_clear_pointer (&filter_label, g_free);
  }
  g_signal_connect (G_OBJECT (menu), "changed",
                    G_CALLBACK (export_adapt_extension_callback), NULL);
  return menu;
}


/**
 * file_export_response_callback:
 * @fs: the #GtkFileChooser
 * @response: the response
 * @user_data: the user data
 *
 * A button hit in the Export Dialog
 *
 * Since: dawn-of-time
 */
static void
file_export_response_callback (GtkWidget *fs,
                               int        response,
                               gpointer   user_data)
{
  char *filename;
  Diagram *dia;
  DiaExportFilter *ef;

  dia = g_object_get_data (G_OBJECT (fs), "user_data");
  g_return_if_fail (dia);

  if (response == GTK_RESPONSE_ACCEPT) {
    int index;

    diagram_update_extents(dia);

    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER(fs));

    index = gtk_combo_box_get_active (GTK_COMBO_BOX(user_data));
    if (index >= 0) {
      persistence_set_integer ("export-filter", index);
    }
    ef = efilter_by_index (index - 1, NULL);
    if (!ef) {
      ef = filter_guess_export_filter (filename);
    }
    if (ef) {
      DiaContext *ctx = dia_context_new (_("Export"));

      g_object_ref (dia->data);
      dia_context_set_filename (ctx, filename);
      ef->export_func (dia->data,
                       ctx,
                       filename,
                       dia->filename,
                       ef->user_data);
      g_object_unref (dia->data);
      dia_context_release (ctx);
    } else {
      message_error (_("Could not determine which export filter\n"
                       "to use to save '%s'"),
                     dia_message_filename (filename));
    }
    g_clear_pointer (&filename, g_free);
  }

  g_clear_object (&dia); /* drop our diagram reference */

  gtk_widget_destroy (exportdlg);
}


/**
 * file_export_callback:
 * @action: the #GtkAction
 *
 * React to `<Display>/File/Export`
 *
 * Since: dawn-of-time
 */
void
file_export_callback (GtkAction *action)
{
  DDisplay *ddisp;
  Diagram *dia;
  char *filename = NULL;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  dia = ddisp->diagram;

  if (!confirm_export_size (dia,
                            GTK_WINDOW (ddisp->shell),
                            CONFIRM_MEMORY | CONFIRM_PAGES))
    return;

  if (!exportdlg) {
    persistence_register_integer ("export-filter", 0);
    exportdlg = gtk_file_chooser_dialog_new (_("Export Diagram"),
                                             GTK_WINDOW (ddisp->shell),
                                             GTK_FILE_CHOOSER_ACTION_SAVE,
                                             _("_Cancel"), GTK_RESPONSE_CANCEL,
                                             _("_Save"), GTK_RESPONSE_ACCEPT,
                                             NULL);
    /* export via vfs gives: Permission denied - but only if you do not
     * have write permissions ;) */
    gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (exportdlg), FALSE);
    gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (exportdlg), TRUE);
    gtk_window_set_role (GTK_WINDOW (exportdlg), "export_diagram");

    g_signal_connect (G_OBJECT (exportdlg),
                      "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &exportdlg);
  }

  if (!gtk_file_chooser_get_extra_widget (GTK_FILE_CHOOSER (exportdlg))) {
    GtkWidget *hbox, *label, *omenu, *options;
    GtkFileFilter* filter;

    options = gtk_frame_new (_("Export Options"));
    gtk_frame_set_shadow_type (GTK_FRAME (options), GTK_SHADOW_ETCHED_IN);

    hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 1);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    gtk_container_add (GTK_CONTAINER (options), hbox);
    gtk_widget_show (hbox);

    label = gtk_label_new (_("Determine file type:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    omenu = create_export_menu ();
    gtk_box_pack_start (GTK_BOX (hbox), omenu, TRUE, TRUE, 0);
    gtk_widget_show (omenu);
    g_object_set_data (G_OBJECT (exportdlg), "export-menu", omenu);

    gtk_widget_show (options);
    gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (exportdlg), options);

    /* set up file filters */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("All Files"));
    gtk_file_filter_add_pattern (filter, "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (exportdlg), filter);

    /* match the other selections extension */
    filter = gtk_file_filter_new ();
    gtk_file_filter_set_name (filter, _("Supported Formats"));
    gtk_file_filter_add_custom (filter,
                                GTK_FILE_FILTER_FILENAME,
                                matching_extensions_filter,
                                filter_guess_export_filter,
                                NULL);
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (exportdlg), filter);

    gtk_combo_box_set_active (GTK_COMBO_BOX (omenu),
                              persistence_get_integer ("export-filter"));

    g_signal_connect (GTK_FILE_CHOOSER (exportdlg),
                      "response",
                      G_CALLBACK (file_export_response_callback),
                      omenu);
  }

  g_object_set_data_full (G_OBJECT (exportdlg),
                          "user_data",
                          g_object_ref (dia),
                          g_object_unref);
  gtk_widget_set_sensitive (exportdlg, TRUE);

  if (dia && dia->filename) {
    filename = g_filename_from_utf8 (dia->filename, -1, NULL, NULL, NULL);
  }

  if (filename != NULL) {
    char* fnabs = g_canonicalize_filename (filename, NULL);

    if (fnabs) {
      char *folder = g_path_get_dirname (fnabs);
      char *basename = g_path_get_basename (fnabs);

      /* can't use gtk_file_chooser_set_filename for various reasons, see e.g. bug #305850 */
      gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER(exportdlg), folder);
      export_adapt_extension (basename, persistence_get_integer ("export-filter") - 1);

      g_clear_pointer (&folder, g_free);
      g_clear_pointer (&basename, g_free);
    }

    g_clear_pointer (&fnabs, g_free);
    g_clear_pointer (&filename, g_free);
  }

  gtk_widget_show (exportdlg);
}
