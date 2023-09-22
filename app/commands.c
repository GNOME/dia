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

#include "config.h"

#include <glib/gi18n-lib.h>

#ifdef HAVE_UNISTD_H
#  include <unistd.h>
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <math.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gtk/gtk.h>

#ifdef G_OS_WIN32
#include <io.h>
#endif

#include <textedit.h>
#include <focus.h>
#include "confirm.h"
#include "dia-application.h"
#include "menus.h"
#include "widgets.h"
#include "intl.h"

/**
 * SECTION:commands
 *
 * Functions called on menu selects.
 *
 * Note that GTK (at least up to 2.12) doesn't disable the keyboard shortcuts
 * when the menu is made insensitive, so we have to check the constrains again
 * in the functions.
 */

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

#include "commands.h"
#include "app_procs.h"
#include "diagram.h"
#include "display.h"
#include "object_ops.h"
#include "cut_n_paste.h"
#include "interface.h"
#include "load_save.h"
#include "message.h"
#include "grid.h"
#include "properties-dialog.h"
#include "propinternals.h"
#include "preferences.h"
#include "layer-editor/layer_dialog.h"
#include "connectionpoint_ops.h"
#include "undo.h"
#include "pagesetup.h"
#include "text.h"
#include "dia_dirs.h"
#include "focus.h"
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include "lib/properties.h"
#include "lib/parent.h"
#include "dia-diagram-properties-dialog.h"
#include "authors.h"                /* master contributors data */
#include "object.h"
#include "dia-guide-dialog.h"
#include "dia-version-info.h"


void
file_quit_callback (GtkAction *action)
{
  app_exit ();
}

void
file_pagesetup_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }
  create_page_setup_dlg (dia);
}

void
file_print_callback (GtkAction *_action)
{
  Diagram *dia;
  DDisplay *ddisp;
  GtkAction *action;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }
  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  action = menus_get_action ("FilePrintGTK");
  if (action) {
    if (confirm_export_size (dia, GTK_WINDOW(ddisp->shell), CONFIRM_PRINT|CONFIRM_PAGES)) {
      gtk_action_activate (action);
    }
  } else {
    message_error (_("No print plugin found!"));
  }
}

void
file_close_callback (GtkAction *action)
{
  /* some people use tear-off menus and insist to close non existing displays */
  if (ddisplay_active ()) {
    ddisplay_close (ddisplay_active());
  }
}

void
file_new_callback (GtkAction *action)
{
  Diagram *dia;
  static int untitled_nr = 1;
  gchar *name;
  GFile *file;

  name = g_strdup_printf (_("Diagram%d.dia"), untitled_nr++);
  file = g_file_new_for_path (name);

  dia = dia_diagram_new (file);

  new_display (dia);

  g_clear_pointer (&name, g_free);
  g_clear_object (&file);
}


void
file_preferences_callback (GtkAction *action)
{
  dia_preferences_dialog_show ();
}


/* Signal handler for getting the clipboard contents */
/* Note that the clipboard is for M$-style cut/copy/paste copying, while
   the selection is for Unix-style mark-and-copy.  We can't really do
   mark-and-copy.
*/

static void
insert_text (DDisplay *ddisp, Focus *focus, const gchar *text)
{
  DiaObjectChange *change = NULL;
  int modified = FALSE, any_modified = FALSE;
  DiaObject *obj = focus_get_object (focus);

  while (text != NULL) {
    gchar *next_line = g_utf8_strchr (text, -1, '\n');

    if (next_line != text) {
      int len = g_utf8_strlen (text, (next_line-text));
      modified = (*focus->key_event) (focus, 0, GDK_KEY_A, text, len, &change);
    }

    if (next_line != NULL) {
      modified = (*focus->key_event) (focus, 0, GDK_KEY_Return, "\n", 1, &change);
      text = g_utf8_next_char (next_line);
    } else {
      text = NULL;
    }

    {
      /* Make sure object updates its data: */
      Point p = obj->position;
      (obj->ops->move) (obj, &p);
    }

    /* Perhaps this can be improved */
    object_add_updates (obj, ddisp->diagram);

    if (modified && (change != NULL)) {
      dia_object_change_change_new (ddisp->diagram, obj, change);
      any_modified = TRUE;
    }

    diagram_flush (ddisp->diagram);
  }

  if (any_modified) {
    diagram_modified (ddisp->diagram);
    diagram_update_extents (ddisp->diagram);
    undo_set_transactionpoint (ddisp->diagram->undo);
  }
}


static void
received_clipboard_text_handler (GtkClipboard *clipboard,
                                 const gchar  *text,
                                 gpointer      data)
{
  DDisplay *ddisp = (DDisplay *) data;
  Focus *focus = get_active_focus ((DiagramData *) ddisp->diagram);

  if (text == NULL) {
    return;
  }

  if ((focus == NULL) || (!focus->has_focus)) {
    return;
  }

  if (!g_utf8_validate (text, -1, NULL)) {
    message_error ("Not valid UTF8");
    return;
  }

  insert_text (ddisp, focus, text);
}

/*
 * Callback for gtk_clipboard_request_image
 */
static void
received_clipboard_image_handler (GtkClipboard *clipboard,
                                  GdkPixbuf    *pixbuf,
                                  gpointer      data)
{
  DDisplay *ddisp = (DDisplay *) data;
  Diagram  *dia = ddisp->diagram;
  GList *list = dia->data->selected;
  DiaObjectChange *change = NULL;

  if (!pixbuf) {
    message_error (_("No image from Clipboard to paste."));
    return;
  }

  while (list) {
    DiaObject *obj = (DiaObject *)list->data;

    /* size before ... */
    object_add_updates (obj, dia);
    change = dia_object_set_pixbuf (obj, pixbuf);
    if (change) {
      dia_object_change_change_new (dia, obj, change);
      /* ... and after the change */
      object_add_updates (obj, dia);
      diagram_modified (dia);
      diagram_flush (dia);
      break;
    }
    list = g_list_next (list);
  }

  if (!change) {
    Point pt;
    DiaObjectType *type;
    Handle *handle1;
    Handle *handle2;
    DiaObject *obj;

    pt = ddisplay_get_clicked_position (ddisp);
    snap_to_grid (ddisp, &pt.x, &pt.y);

    if (   ((type = object_get_type ("Standard - Image")) != NULL)
        && ((obj = dia_object_default_create (type,
                                              &pt,
                                              type->default_user_data,
                                              &handle1,
                                              &handle2)) != NULL)) {
      /* as above, transfer the data */
      change = dia_object_set_pixbuf (obj, pixbuf);

      /* ... but drop undo info */
      g_clear_pointer (&change, dia_object_change_unref);

      /* allow undo of the whole thing */
      dia_insert_objects_change_new (dia, g_list_prepend (NULL, obj), 1);

      diagram_add_object (dia, obj);
      diagram_select (dia, obj);
      object_add_updates (obj, dia);

      ddisplay_do_update_menu_sensitivity (ddisp);
      diagram_flush (dia);
    } else {
      message_warning (_("No selected object can take an image."));
    }
  }
  /* although freed above it is still the indicator of diagram modification */
  if (change) {
    diagram_update_extents (dia);
    undo_set_transactionpoint (dia->undo);
    diagram_modified (dia);
  }
}

static void
received_clipboard_content_handler (GtkClipboard     *clipboard,
                                    GtkSelectionData *selection_data,
                                    gpointer          user_data)
{
  DDisplay *ddisp = (DDisplay *) user_data;
  GdkAtom type_atom;
  gchar *type_name;
  int len;

  if ((len = gtk_selection_data_get_length (selection_data)) > 0) {
    const guchar *data = gtk_selection_data_get_data (selection_data);
    type_atom = gtk_selection_data_get_data_type (selection_data);
    type_name = gdk_atom_name (type_atom);
    if (type_name && (   strcmp (type_name, "image/svg") == 0
                      || strcmp (type_name, "image/svg+xml") == 0
                      || strcmp (type_name, "UTF8_STRING") == 0)) {
      DiaImportFilter *ifilter = filter_import_get_by_name ("dia-svg");
      DiaContext *ctx = dia_context_new (_("Clipboard Paste"));

      if (ifilter->import_mem_func) {
        DiaChange *change = dia_import_change_new (ddisp->diagram);

        if (!ifilter->import_mem_func (data,
                                       len,
                                       DIA_DIAGRAM_DATA (ddisp->diagram),
                                       ctx,
                                       ifilter->user_data)) {
          /* might become more right some day ;) */
          dia_context_add_message (ctx,
                                   _("Failed to import '%s' as SVG."),
                                   type_name);
          dia_context_release (ctx);
        }
        if (dia_import_change_done (ddisp->diagram, change)) {
          undo_set_transactionpoint (ddisp->diagram->undo);
          diagram_modified (ddisp->diagram);
          diagram_flush (ddisp->diagram);
        }
      }
    } else {
      /* fallback to pixbuf loader */
      GdkPixbuf *pixbuf = gtk_selection_data_get_pixbuf (selection_data);
      if (pixbuf) {
        received_clipboard_image_handler (clipboard, pixbuf, ddisp);
        g_clear_object (&pixbuf);
      } else {
        message_error (_("Paste failed: %s"), type_name);
      }
    }
    dia_log_message ("Content is %s (size=%d)", type_name, len);
    g_clear_pointer (&type_name, g_free);
  }
}

void
edit_paste_image_callback (GtkAction *action)
{
  GtkClipboard *clipboard = gtk_clipboard_get (GDK_NONE);
  DDisplay *ddisp;
  GdkAtom *targets;
  int n_targets;
  gboolean done = FALSE;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  if (gtk_clipboard_wait_for_targets (clipboard, &targets, &n_targets)) {
    int i;
    for (i = 0; i < n_targets; ++i) {
      gchar *aname = gdk_atom_name (targets[i]);
      if (   strcmp (aname, "image/svg") == 0
          || strcmp (aname, "image/svg+xml") == 0
          || strcmp (aname, "UTF8_STRING") == 0) {
        gtk_clipboard_request_contents (clipboard,
                                        targets[i],
                                        received_clipboard_content_handler,
                                        ddisp);
        done = TRUE;
      }
      dia_log_message ("clipboard-targets %d: %s", i, aname);
      g_clear_pointer (&aname, g_free);
      if (done) {
        break;
      }
    }
    if (!done) {
      gtk_clipboard_request_image (clipboard,
                                   received_clipboard_image_handler,
                                   ddisp);
    }
    g_clear_pointer (&targets, g_free);
  }
}

static PropDescription text_prop_singleton_desc[] = {
  { "text", PROP_TYPE_TEXT },
  PROP_DESC_END
};


static void
make_text_prop_singleton (GPtrArray **props, TextProperty **prop)
{
  *props = prop_list_from_descs (text_prop_singleton_desc, pdtpp_true);
  g_assert ((*props)->len == 1);

  *prop = g_ptr_array_index ((*props),0);
  g_clear_pointer (&(*prop)->text_data, g_free);
}


static GtkTargetEntry target_entries[] = {
  { "image/svg", GTK_TARGET_OTHER_APP, 1 },
  { "image/svg+xml", GTK_TARGET_OTHER_APP, 2 },
  { "image/png", GTK_TARGET_OTHER_APP, 3 },
  { "image/bmp", GTK_TARGET_OTHER_APP, 4 },
  { "image/tiff", GTK_TARGET_OTHER_APP, 5 },
#ifdef G_OS_WIN32
  /* this is not working on win32 either, maybe we need to register it with
   * CF_ENHMETAFILE in Gtk+? Change order? Direct use of SetClipboardData()?
   */
  { "image/emf", GTK_TARGET_OTHER_APP, 6 },
  { "image/wmf", GTK_TARGET_OTHER_APP, 7 },
#endif
};


/*
 * This gets called by Gtk+ if some other application is asking
 * for the clipboard content.
 */
static void
_clipboard_get_data_callback (GtkClipboard     *clipboard,
                              GtkSelectionData *selection_data,
                              guint             info,
                              gpointer          owner_or_user_data)
{
  DiaContext *ctx = dia_context_new (_("Clipboard Copy"));
  DiagramData *dia = owner_or_user_data; /* todo: check it's still valid */
  const char *ext = strchr (target_entries[info-1].target, '/') + 1;
  char *tmplate;
  char *outfname = NULL;
  GError *error = NULL;
  DiaExportFilter *ef = NULL;
  int fd;

  /* Although asked for bmp, use png here because of potentially better renderer
   * Dropping 'bmp' in target would exclude many win32 programs, but gtk+ can
   * convert from png on demand ... */
  if (strcmp (ext, "bmp") == 0) {
    tmplate = g_strdup ("dia-cb-XXXXXX.png");
  } else if (strcmp (ext, "tiff") == 0) {
    /* pixbuf on OS X offers qtif and qif - both look like a mistake to me ;) */
    tmplate = g_strdup ("dia-cb-XXXXXX.png");
  } else if (g_str_has_suffix (ext, "+xml")) {
    gchar *ext2 = g_strndup (ext, strlen (ext) - 4);
    tmplate = g_strdup_printf ("dia-cb-XXXXXX.%s", ext2);
    g_clear_pointer (&ext2, g_free);
  } else {
    tmplate = g_strdup_printf ("dia-cb-XXXXXX.%s", ext);
  }

  if ((fd = g_file_open_tmp (tmplate, &outfname, &error)) != -1) {
    ef = filter_guess_export_filter (outfname);
    close (fd);
  } else {
    g_warning ("%s", error->message);
    g_clear_error (&error);
  }
  g_clear_pointer (&tmplate, g_free);

  if (ef) {
    /* for png use alpha-rendering if available */
    if (strcmp (ext, "png") == 0 &&
        filter_export_get_by_name ("cairo-alpha-png") != NULL) {
      ef = filter_export_get_by_name ("cairo-alpha-png");
    }
    dia_context_set_filename (ctx, outfname);
    ef->export_func (DIA_DIAGRAM_DATA (dia),
                     ctx,
                     outfname,
                     "clipboard-copy",
                     ef->user_data);
    /* If we have a vector format, don't convert it to pixbuf.
     * Or even better: only use pixbuf transport when asked
     * for 'OS native bitmaps' BMP (win32), TIFF(osx), ...?
     */
    if (strcmp (ext, "bmp") == 0 || strcmp (ext, "tiff") == 0) {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (outfname, &error);
      if (pixbuf) {
        gtk_selection_data_set_pixbuf (selection_data, pixbuf);
        g_clear_object (&pixbuf);
      }
    } else {
      char *buf = NULL;
      gsize length = 0;

      if (g_file_get_contents (outfname, &buf, &length, &error)) {
        gtk_selection_data_set (selection_data,
                                gdk_atom_intern_static_string (target_entries[info - 1].target),
                                8,
                                (guint8 *) buf,
                                length);
        g_clear_pointer (&buf, g_free);
      } else {
        g_critical ("Failed to read %s: %s", outfname, error->message);
      }

      g_unlink (outfname);
    }
  }

  if (error) {
    dia_context_add_message (ctx, "%s", error->message);
  }

  g_clear_error (&error);
  dia_context_release (ctx);
}


/** GtkClipboardClearFunc */
static void
_clipboard_clear_data_callback (GtkClipboard *clipboard,
                                gpointer      owner_or_user_data)
{
  DiagramData *dia = owner_or_user_data; /* todo: check it's still valid */

  if (dia) {
    g_clear_object (&dia);
  }
}


void
edit_copy_callback (GtkAction *action)
{
  GList *copy_list;
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  if (textedit_mode (ddisp)) {
    Focus *focus = get_active_focus ((DiagramData *) ddisp->diagram);
    DiaObject *obj = focus_get_object (focus);
    GPtrArray *textprops;
    TextProperty *prop;

    if (obj->ops->get_props == NULL) {
      return;
    }

    make_text_prop_singleton (&textprops, &prop);
    /* Get the first text property */
    obj->ops->get_props (obj, textprops);

    /* GTK docs claim the selection clipboard is ignored on Win32.
     * The "clipboard" clipboard is (was) mostly ignored in Unix.
     * Nowadays the notation of one system global clipboard is common
     * for many programs on Linux, too.
     */
#ifndef GDK_WINDOWING_X11
    gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE),
                            prop->text_data, -1);
#else
    gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                            prop->text_data, -1);
#endif
    prop_list_free (textprops);
  } else {
    /* create a diagram of currently selected to the clipboard - it's rendered on request */
    DiagramData *data = diagram_data_clone_selected (ddisp->diagram->data);

    /* arbitrary scaling from the display, deliberately ignoring
     * the paper scaling, like the display code does
     */
    data->paper.scaling = (ddisp->zoom_factor / 20.0);

    gtk_clipboard_set_with_data (gtk_clipboard_get(GDK_NONE),
                                 target_entries,
                                 G_N_ELEMENTS (target_entries),
                                 _clipboard_get_data_callback,
                                 _clipboard_clear_data_callback,
                                 data);

    /* just internal copy of the selected objects */
    copy_list = parent_list_affected (diagram_get_sorted_selected (ddisp->diagram));

    cnp_store_objects (object_copy_list (copy_list), 1);
    g_list_free (copy_list);

    ddisplay_do_update_menu_sensitivity (ddisp);
  }
}

void
edit_cut_callback (GtkAction *action)
{
  GList *cut_list;
  DDisplay *ddisp;
  DiaChange *change;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  if (textedit_mode (ddisp)) {
  } else {
    diagram_selected_break_external (ddisp->diagram);

    cut_list = parent_list_affected (diagram_get_sorted_selected (ddisp->diagram));

    cnp_store_objects (object_copy_list (cut_list), 0);

    change = dia_delete_objects_change_new_with_children (ddisp->diagram, cut_list);
    dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));

    ddisplay_do_update_menu_sensitivity (ddisp);
    diagram_flush (ddisp->diagram);

    diagram_modified (ddisp->diagram);
    diagram_update_extents (ddisp->diagram);
    undo_set_transactionpoint (ddisp->diagram->undo);
  }
}

void
edit_paste_callback (GtkAction *action)
{
  GList *paste_list;
  DDisplay *ddisp;
  Point paste_corner;
  Point delta;
  DiaChange *change;
  int generation = 0;

  ddisp = ddisplay_active();
  if (!ddisp) {
    return;
  }

  if (textedit_mode (ddisp)) {
#ifndef GDK_WINDOWING_X11
    gtk_clipboard_request_text (gtk_clipboard_get (GDK_NONE),
                                received_clipboard_text_handler,
                                ddisp);
#else
    gtk_clipboard_request_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                                received_clipboard_text_handler,
                                ddisp);
#endif
  } else {
    if (!cnp_exist_stored_objects ()) {
      message_warning (_("No existing object to paste.\n"));
      return;
    }

    paste_list = cnp_get_stored_objects (&generation); /* Gets a copy */

    paste_corner = object_list_corner (paste_list);

    delta.x = ddisp->visible.left - paste_corner.x;
    delta.y = ddisp->visible.top - paste_corner.y;

    /* Move down some 10% of the visible area. */
    delta.x += (ddisp->visible.right - ddisp->visible.left) * 0.1 * generation;
    delta.y += (ddisp->visible.bottom - ddisp->visible.top) * 0.1 * generation;

    if (generation) {
      object_list_move_delta (paste_list, &delta);
    }

    change = dia_insert_objects_change_new (ddisp->diagram, paste_list, 0);
    dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));

    diagram_modified (ddisp->diagram);
    undo_set_transactionpoint (ddisp->diagram->undo);

    diagram_remove_all_selected (ddisp->diagram, TRUE);
    diagram_select_list (ddisp->diagram, paste_list);

    diagram_update_extents (ddisp->diagram);
    diagram_flush (ddisp->diagram);
  }
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
edit_duplicate_callback (GtkAction *action)
{
  GList *duplicate_list;
  DDisplay *ddisp;
  Point delta;
  DiaChange *change;

  ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }
  duplicate_list = object_copy_list (diagram_get_sorted_selected (ddisp->diagram));

  /* Move down some 10% of the visible area. */
  delta.x = (ddisp->visible.right - ddisp->visible.left)*0.05;
  delta.y = (ddisp->visible.bottom - ddisp->visible.top)*0.05;

  object_list_move_delta (duplicate_list, &delta);

  change = dia_insert_objects_change_new (ddisp->diagram, duplicate_list, 0);
  dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));

  diagram_modified (ddisp->diagram);
  undo_set_transactionpoint (ddisp->diagram->undo);

  diagram_remove_all_selected (ddisp->diagram, TRUE);
  diagram_select_list (ddisp->diagram, duplicate_list);

  diagram_flush (ddisp->diagram);

  ddisplay_do_update_menu_sensitivity (ddisp);
}

void
objects_move_up_layer (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  GList *selected_list;
  DiaChange *change;

  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }
  selected_list = diagram_get_sorted_selected (ddisp->diagram);

  change = dia_move_object_to_layer_change_new (ddisp->diagram, selected_list, TRUE);

  dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));

  diagram_modified (ddisp->diagram);
  undo_set_transactionpoint (ddisp->diagram->undo);

  diagram_flush (ddisp->diagram);

  ddisplay_do_update_menu_sensitivity (ddisp);
}

 void
objects_move_down_layer (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  GList *selected_list;
  DiaChange *change;

  if (!ddisp || textedit_mode(ddisp)) {
    return;
  }
  selected_list = diagram_get_sorted_selected (ddisp->diagram);

  /* Must check if move is legal here */

  change = dia_move_object_to_layer_change_new (ddisp->diagram, selected_list, FALSE);

  dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));

  diagram_modified (ddisp->diagram);
  undo_set_transactionpoint (ddisp->diagram->undo);

  diagram_flush (ddisp->diagram);

  ddisplay_do_update_menu_sensitivity (ddisp);
}

void
edit_copy_text_callback (GtkAction *action)
{
  Focus *focus;
  DDisplay *ddisp = ddisplay_active ();
  DiaObject *obj;
  GPtrArray *textprops;
  TextProperty *prop;

  if (ddisp == NULL) {
    return;
  }

  focus = get_active_focus ((DiagramData *) ddisp->diagram);

  if ((focus == NULL) || (!focus->has_focus)) {
    return;
  }

  obj = focus_get_object (focus);

  if (obj->ops->get_props == NULL) {
    return;
  }

  make_text_prop_singleton (&textprops,&prop);
  /* Get the first text property */
  obj->ops->get_props (obj, textprops);

  /* GTK docs claim the selection clipboard is ignored on Win32.
   * The "clipboard" clipboard is mostly ignored in Unix
   */
#ifndef GDK_WINDOWING_X11
  gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE),
                          prop->text_data, -1);
#else
  gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                          prop->text_data, -1);
#endif
  prop_list_free (textprops);
}

void
edit_cut_text_callback (GtkAction *action)
{
  Focus *focus;
  DDisplay *ddisp;
  DiaObject *obj;
  GPtrArray *textprops;
  TextProperty *prop;
  DiaObjectChange *change;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  focus = get_active_focus ((DiagramData *) ddisp->diagram);
  if ((focus == NULL) || (!focus->has_focus)) {
    return;
  }

  obj = focus_get_object (focus);

  if (obj->ops->get_props == NULL) {
    return;
  }

  make_text_prop_singleton (&textprops,&prop);
  /* Get the first text property */
  obj->ops->get_props (obj, textprops);

  /* GTK docs claim the selection clipboard is ignored on Win32.
   * The "clipboard" clipboard is mostly ignored in Unix
   */
#ifndef GDK_WINDOWING_X11
  gtk_clipboard_set_text (gtk_clipboard_get (GDK_NONE),
                          prop->text_data, -1);
#else
  gtk_clipboard_set_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                          prop->text_data, -1);
#endif

  prop_list_free (textprops);

  if (text_delete_all (focus->text, &change, obj)) {
    object_add_updates (obj, ddisp->diagram);
    dia_object_change_change_new (ddisp->diagram, obj, change);
    undo_set_transactionpoint (ddisp->diagram->undo);
    diagram_modified (ddisp->diagram);
    diagram_flush (ddisp->diagram);
  }
}

void
edit_paste_text_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

#ifndef GDK_WINDOWING_X11
  gtk_clipboard_request_text (gtk_clipboard_get (GDK_NONE),
                              received_clipboard_text_handler, ddisp);
#else
  gtk_clipboard_request_text (gtk_clipboard_get (GDK_SELECTION_PRIMARY),
                              received_clipboard_text_handler, ddisp);
#endif
}

void
edit_delete_callback (GtkAction *action)
{
  GList *delete_list;
  DDisplay *ddisp;

  /* Avoid crashing while moving or resizing and deleting ... */
  GdkDisplay *display = gdk_display_get_default ();
  GdkSeat *seat = gdk_display_get_default_seat (display);
  GdkDevice *device = gdk_seat_get_pointer (seat);

  if (gdk_display_device_is_grabbed (display, device)) {
    gdk_beep ();    /* ... no matter how much sense it makes. */
    return;
  }

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }
  if (textedit_mode (ddisp)) {
    DiaObjectChange *change = NULL;
    Focus *focus = get_active_focus ((DiagramData *) ddisp->diagram);
    if (!text_delete_key_handler (focus, &change)) {
      return;
    }
    object_add_updates (focus->obj, ddisp->diagram);
  } else {
    DiaChange *change = NULL;
    diagram_selected_break_external (ddisp->diagram);

    delete_list = diagram_get_sorted_selected (ddisp->diagram);
    change = dia_delete_objects_change_new_with_children (ddisp->diagram, delete_list);
    g_list_free (delete_list);
    dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));
  }
  diagram_modified (ddisp->diagram);
  diagram_update_extents (ddisp->diagram);

  ddisplay_do_update_menu_sensitivity (ddisp);
  diagram_flush (ddisp->diagram);

  undo_set_transactionpoint (ddisp->diagram->undo);
}

void
edit_undo_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  /* Handle text undo edit here! */
  undo_revert_to_last_tp (dia->undo);
  diagram_modified (dia);
  diagram_update_extents (dia);

  diagram_flush (dia);
}

void
edit_redo_callback (GtkAction *action)
{
  Diagram *dia;

/* Handle text undo edit here! */
  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  undo_apply_to_next_tp (dia->undo);
  diagram_modified (dia);
  diagram_update_extents (dia);

  diagram_flush (dia);
}

void
help_manual_callback (GtkAction *action)
{
  char *helpdir, *helpindex = NULL, *command;
  guint bestscore = G_MAXINT;
  GDir *dp;
  const char *dentry;
  GError *error = NULL;
  GdkScreen *screen;
  DDisplay *ddisp;
  ddisp = ddisplay_active ();
  screen = ddisp ? gtk_widget_get_screen (GTK_WIDGET (ddisp->shell))
                 : gdk_screen_get_default ();

  helpdir = dia_get_data_directory ("help");
  if (!helpdir) {
    message_warning (_("Could not find help directory"));
    return;
  }

  /* search through helpdir for the helpfile that matches the user's locale */
  dp = g_dir_open (helpdir, 0, &error);
  if (!dp) {
    message_warning (_("Could not open help directory:\n%s"),
                     error->message);
    g_clear_error (&error);
    return;
  }

  while ((dentry = g_dir_read_name (dp)) != NULL) {
    guint score;

    score = intl_score_locale (dentry);
    if (score < bestscore) {
      if (helpindex) {
        g_clear_pointer (&helpindex, g_free);
      }
#ifdef G_OS_WIN32
      /* use HTML Help on win32 if available */
      helpindex = g_strconcat (helpdir,
                               G_DIR_SEPARATOR_S,
                               dentry,
                               G_DIR_SEPARATOR_S "dia-manual.chm",
                               NULL);
      if (!g_file_test (helpindex, G_FILE_TEST_EXISTS)) {
        helpindex = g_strconcat (helpdir,
                                 G_DIR_SEPARATOR_S,
                                 dentry,
                                 G_DIR_SEPARATOR_S "index.html",
                                 NULL);
      }
#else
      helpindex = g_strconcat (helpdir,
                               G_DIR_SEPARATOR_S,
                               dentry,
                               G_DIR_SEPARATOR_S "index.html",
                               NULL);
#endif
      bestscore = score;
    }
  }
  g_dir_close (dp);
  g_clear_pointer (&helpdir, g_free);
  if (!helpindex) {
    message_warning (_("Could not find help directory"));
    return;
  }

#ifdef G_OS_WIN32
# define SW_SHOWNORMAL 1
  ShellExecuteA (0, "open", helpindex, NULL, helpdir, SW_SHOWNORMAL);
#else
  command = g_strdup_printf ("file://%s", helpindex);
  gtk_show_uri (screen, command, gtk_get_current_event_time (), NULL);
  g_clear_pointer (&command, g_free);
#endif

  g_clear_pointer (&helpindex, g_free);
}

void
activate_url (GtkWidget   *parent,
              const gchar *link,
              gpointer     data)
{
#ifdef G_OS_WIN32
  ShellExecuteA (0, "open", link, NULL, NULL, SW_SHOWNORMAL);
#else
  GdkScreen *screen;

  if (parent) {
    screen = gtk_widget_get_screen (GTK_WIDGET(parent));
  } else {
    screen = gdk_screen_get_default ();
  }
  gtk_show_uri (screen, link, gtk_get_current_event_time (), NULL);
#endif
}


void
help_about_callback (GtkAction *action)
{
  const char *translators = _("translator_credits-PLEASE_ADD_YOURSELF_HERE");
  // TODO: Gtk3 has license-type
  const char *license = _(
    "This program is free software; you can redistribute it and/or modify\n"
    "it under the terms of the GNU General Public License as published by\n"
    "the Free Software Foundation; either version 2 of the License, or\n"
    "(at your option) any later version.\n"
    "\n"
    "This program is distributed in the hope that it will be useful,\n"
    "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
    "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
    "GNU General Public License for more details.\n"
    "\n"
    "You should have received a copy of the GNU General Public License\n"
    "along with this program; if not, write to the Free Software\n"
    "Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.\n");

  GdkPixbuf *logo = pixbuf_from_resource ("/org/gnome/Dia/dia-splash.png");

  gtk_show_about_dialog (NULL,
                         "logo", logo,
                         "program-name", _("Dia Diagram Editor"),
                         "version", dia_version_string (),
                         "comments", _("A program for drawing structured diagrams."),
                         "copyright", "(C) 1998-2011 The Free Software Foundation and the authors\n"
                                      "© 2018-2021 Zander Brown et al\n",
                         "website", "https://wiki.gnome.org/Apps/Dia/",
                         "authors", authors,
                         "documenters", documentors,
                         "translator-credits", strcmp (translators, "translator_credits-PLEASE_ADD_YOURSELF_HERE")
                                                ? translators : NULL,
                         "license", license,
                         NULL);

  g_clear_object (&logo);
}


void
view_zoom_in_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_zoom_middle (ddisp, M_SQRT2);
}

void
view_zoom_out_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_zoom_middle (ddisp, M_SQRT1_2);
}

void
view_zoom_set_callback (GtkAction *action)
{
  int factor;
  /* HACK the actual factor is a suffix to the action name */
  factor = atoi (gtk_action_get_name (action) + strlen ("ViewZoom"));
  view_zoom_set (factor);
}

void
view_show_cx_pts_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int old_val;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  old_val = ddisp->show_cx_pts;
  ddisp->show_cx_pts = gtk_toggle_action_get_active (action);

  if (old_val != ddisp->show_cx_pts) {
    ddisplay_add_update_all (ddisp);
    ddisplay_flush (ddisp);
  }
}

void
view_unfullscreen (void)
{
  DDisplay *ddisp;
  GtkToggleAction *item;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  /* find the menuitem */
  item = GTK_TOGGLE_ACTION (menus_get_action ("ViewFullscreen"));
  if (item && gtk_toggle_action_get_active (item)) {
    gtk_toggle_action_set_active (item, FALSE);
  }
}

void
view_fullscreen_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int fs;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  fs = gtk_toggle_action_get_active (action);

  if (fs) { /* it is already toggled */
    gtk_window_fullscreen (GTK_WINDOW (ddisp->shell));
  } else {
    gtk_window_unfullscreen (GTK_WINDOW (ddisp->shell));
  }
}

void
view_aa_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  int aa;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  aa = gtk_toggle_action_get_active (action);

  if (aa != ddisp->aa_renderer) {
    ddisplay_set_renderer (ddisp, aa);
    ddisplay_add_update_all (ddisp);
    ddisplay_flush (ddisp);
  }
}

void
view_visible_grid_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  guint old_val;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  old_val = ddisp->grid.visible;
  ddisp->grid.visible = gtk_toggle_action_get_active (action);

  if (old_val != ddisp->grid.visible) {
    ddisplay_add_update_all (ddisp);
    ddisplay_flush (ddisp);
  }
}

void
view_snap_to_grid_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_set_snap_to_grid (ddisp, gtk_toggle_action_get_active (action));
}

void
view_snap_to_objects_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_set_snap_to_objects (ddisp, gtk_toggle_action_get_active (action));
}

void
view_toggle_rulers_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  if (!gtk_toggle_action_get_active (action)) {
    if (display_get_rulers_showing (ddisp)) {
      display_rulers_hide (ddisp);
    }
  } else {
    if (!display_get_rulers_showing (ddisp)) {
      display_rulers_show (ddisp);
    }
  }
}

void
view_toggle_scrollbars_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  if (gtk_toggle_action_get_active (action)) {
    gtk_widget_show (ddisp->hsb);
    gtk_widget_show (ddisp->vsb);
  } else {
    gtk_widget_hide (ddisp->hsb);
    gtk_widget_hide (ddisp->vsb);
  }
}
extern void
view_new_view_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  new_display (dia);
}

extern void
view_clone_view_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  copy_display (ddisp);
}

void
view_show_all_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_show_all (ddisp);
}

void
view_redraw_callback (GtkAction *action)
{
  DDisplay *ddisp;
  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);
}

void
view_diagram_properties_callback (GtkAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }
  diagram_properties_show (ddisp->diagram);
}

void
view_main_toolbar_callback (GtkAction *action)
{
  integrated_ui_toolbar_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}

void
view_main_statusbar_callback (GtkAction *action)
{
  integrated_ui_statusbar_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}

void
view_layers_callback (GtkAction *action)
{
  integrated_ui_layer_view_show (gtk_toggle_action_get_active (GTK_TOGGLE_ACTION (action)));
}


void
view_new_guide_callback (GtkAction *action)
{
  DDisplay *ddisp;
  GtkWidget *dlg;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  dlg = dia_guide_dialog_new (GTK_WINDOW (ddisp->shell), ddisp->diagram);
  gtk_widget_show (dlg);
}


void
view_visible_guides_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;
  gboolean old_val;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  old_val = ddisp->guides_visible;
  ddisp->guides_visible = gtk_toggle_action_get_active (action);

  if (old_val != ddisp->guides_visible) {
    ddisplay_add_update_all (ddisp);
    ddisplay_flush (ddisp);
  }
}


void
view_snap_to_guides_callback (GtkToggleAction *action)
{
  DDisplay *ddisp;

  ddisp = ddisplay_active ();
  if (!ddisp) {
    return;
  }

  ddisplay_set_snap_to_guides (ddisp, gtk_toggle_action_get_active (action));
}


void
view_remove_all_guides_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  dia_diagram_remove_all_guides (dia);
  diagram_add_update_all (dia);
  diagram_flush (dia);
}


void
layers_add_layer_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  diagram_edit_layer (dia, NULL);
}


void
layers_rename_layer_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }

  diagram_edit_layer (dia,
                      dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (dia)));
}


void
objects_place_over_callback (GtkAction *action)
{
  diagram_place_over_selected (ddisplay_active_diagram ());
}

void
objects_place_under_callback (GtkAction *action)
{
  diagram_place_under_selected (ddisplay_active_diagram ());
}

void
objects_place_up_callback (GtkAction *action)
{
  diagram_place_up_selected (ddisplay_active_diagram ());
}

void
objects_place_down_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_place_down_selected (ddisplay_active_diagram ());
}

void
objects_parent_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_parent_selected (ddisplay_active_diagram ());
}

void
objects_unparent_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_unparent_selected (ddisplay_active_diagram ());
}

void
objects_unparent_children_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_unparent_children_selected (ddisplay_active_diagram ());
}

void
objects_group_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_group_selected (ddisplay_active_diagram ());
  ddisplay_do_update_menu_sensitivity (ddisp);
}

void
objects_ungroup_callback (GtkAction *action)
{
  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  diagram_ungroup_selected (ddisplay_active_diagram ());
  ddisplay_do_update_menu_sensitivity (ddisp);
}

void
dialogs_properties_callback (GtkAction *action)
{
  Diagram *dia;

  dia = ddisplay_active_diagram ();
  if (!dia || textedit_mode (ddisplay_active ())) {
    return;
  }

  if (dia->data->selected != NULL) {
    object_list_properties_show (dia, dia->data->selected);
  } else {
    diagram_properties_show (dia);
  }
}

void
dialogs_layers_callback (GtkAction *action)
{
  layer_dialog_set_diagram (ddisplay_active_diagram ());
  layer_dialog_show ();
}


void
objects_align_h_callback (GtkAction *action)
{
  const gchar *a;
  int align = DIA_ALIGN_LEFT;
  Diagram *dia;
  GList *objects;

  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  /* HACK align is suffix to action name */
  a = gtk_action_get_name (action) + strlen ("ObjectsAlign");
  if (0 == strcmp ("Left", a)) {
    align = DIA_ALIGN_LEFT;
  } else if (0 == strcmp ("Center", a)) {
    align = DIA_ALIGN_CENTER;
  } else if (0 == strcmp ("Right", a)) {
    align = DIA_ALIGN_RIGHT;
  } else if (0 == strcmp ("Spreadouthorizontally", a)) {
    align = DIA_ALIGN_EQUAL;
  } else if (0 == strcmp ("Adjacent", a)) {
    align = DIA_ALIGN_ADJACENT;
  } else {
    g_warning ("objects_align_v_callback() called without appropriate align");
    return;
  }

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }
  objects = dia->data->selected;

  object_add_updates_list (objects, dia);
  object_list_align_h (objects, dia, align);
  diagram_update_connections_selection (dia);
  object_add_updates_list (objects, dia);
  diagram_modified (dia);
  diagram_flush (dia);

  undo_set_transactionpoint (dia->undo);
}

void
objects_align_v_callback (GtkAction *action)
{
  const gchar *a;
  int align;
  Diagram *dia;
  GList *objects;

  DDisplay *ddisp = ddisplay_active ();
  if (!ddisp || textedit_mode (ddisp)) {
    return;
  }

  /* HACK align is suffix to action name */
  a = gtk_action_get_name (action) + strlen ("ObjectsAlign");
  if (0 == strcmp ("Top", a)) {
    align = DIA_ALIGN_TOP;
  } else if (0 == strcmp ("Middle", a)) {
    align = DIA_ALIGN_CENTER;
  } else if (0 == strcmp ("Bottom", a)) {
    align = DIA_ALIGN_BOTTOM;
  } else if (0 == strcmp ("Spreadoutvertically", a)) {
    align = DIA_ALIGN_EQUAL;
  } else if (0 == strcmp ("Stacked", a)) {
    align = DIA_ALIGN_ADJACENT;
  } else {
    g_warning ("objects_align_v_callback() called without appropriate align");
    return;
  }

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }
  objects = dia->data->selected;

  object_add_updates_list (objects, dia);
  object_list_align_v (objects, dia, align);
  diagram_update_connections_selection (dia);
  object_add_updates_list (objects, dia);
  diagram_modified (dia);
  diagram_flush (dia);

  undo_set_transactionpoint (dia->undo);
}

void
objects_align_connected_callback (GtkAction *action)
{
  Diagram *dia;
  GList *objects;

  dia = ddisplay_active_diagram ();
  if (!dia) {
    return;
  }
  objects = dia->data->selected;

  object_add_updates_list (objects, dia);
  object_list_align_connected (objects, dia, 0);
  diagram_update_connections_selection (dia);
  object_add_updates_list (objects, dia);
  diagram_modified (dia);
  diagram_flush (dia);

  undo_set_transactionpoint (dia->undo);
}

/*! Open a file and show it in a new display */
void
dia_file_open (const gchar     *filename,
               DiaImportFilter *ifilter)
{
  Diagram *diagram;

  if (!ifilter) {
    ifilter = filter_guess_import_filter (filename);
  }

  diagram = diagram_load (filename, ifilter);
  if (diagram != NULL) {
    diagram_update_extents (diagram);
    layer_dialog_set_diagram (diagram);
    new_display (diagram);
  }
}
