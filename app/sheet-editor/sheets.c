/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * sheets.c : a sheets and objects dialog
 * Copyright (C) 2002 M.C. Nelson
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
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#include <gtk/gtk.h>

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gmodule.h>

#include "plug-ins.h"
#include "sheet.h"
#include "message.h"
#include "object.h"
#include "interface.h"
#include "sheets.h"
#include "sheets_dialog.h"
#include "gtkhwrapbox.h"
#include "preferences.h"
#include "toolbox.h"  /* just for interface_current_sheet_name */
#include "commands.h" /* sheets_dialog_show_callback */

GtkWidget *sheets_dialog = NULL;
GSList *sheets_mods_list = NULL;

/* Given a SheetObject and a SheetMod, create a new SheetObjectMod
   and hook it into the 'objects' list in the SheetMod->sheet

   NOTE: that the objects structure points to SheetObjectMod's and
        *not* SheetObject's */

SheetObjectMod *
sheets_append_sheet_object_mod (SheetObject *so,
                                SheetMod    *sm)
{
  SheetObjectMod *sheet_object_mod;

  g_return_val_if_fail (so != NULL && sm != NULL, NULL);

  sheet_object_mod = g_new0 (SheetObjectMod, 1);
  sheet_object_mod->sheet_object = *so;
  sheet_object_mod->mod = SHEET_OBJECT_MOD_NONE;

  sm->sheet.objects = g_slist_append (sm->sheet.objects, sheet_object_mod);

  return sheet_object_mod;
}

/* Given a Sheet, create a SheetMod wrapper for a list of SheetObjectMod's */

SheetMod *
sheets_append_sheet_mods (Sheet *sheet)
{
  SheetMod *sheet_mod;
  GSList *sheet_objects_list;

  g_return_val_if_fail (sheet != NULL, NULL);

  sheet_mod = g_new0 (SheetMod, 1);
  sheet_mod->sheet = *sheet;
  sheet_mod->original = sheet;
  sheet_mod->mod = SHEETMOD_MOD_NONE;
  sheet_mod->sheet.objects = NULL;

  for (sheet_objects_list = sheet->objects; sheet_objects_list;
       sheet_objects_list = g_slist_next (sheet_objects_list)) {
    sheets_append_sheet_object_mod (sheet_objects_list->data, sheet_mod);
  }

  sheets_mods_list = g_slist_append (sheets_mods_list, sheet_mod);

  return sheet_mod;
}


struct FindSheetData {
  GtkWidget  *combo;
  const char *find;
};


static gboolean
find_sheet (GtkTreeModel *model,
            GtkTreePath  *path,
            GtkTreeIter  *iter,
            gpointer      udata)
{
  struct FindSheetData *data = udata;
  char *item;
  gboolean res = FALSE;

  gtk_tree_model_get (model,
                      iter,
                      SO_COL_NAME, &item,
                      -1);

  res = g_strcmp0 (data->find, item) == 0;
  if (res) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (data->combo), iter);
  }

  g_clear_pointer (&item, g_free);

  return res;
}


void
select_sheet (GtkWidget *combo,
              gchar     *sheet_name)
{
  GtkListStore *store = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (combo)));
  GtkTreeIter iter;

  /* If we were passed a sheet_name, then make the optionmenu point to that
     name after creation */

  if (sheet_name) {
    struct FindSheetData data;

    data.combo = combo;
    data.find = sheet_name;

    gtk_tree_model_foreach (GTK_TREE_MODEL (store), find_sheet, &data);
  } else {
    if (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (combo), &iter);
    }
  }
}


void
populate_store (GtkListStore *store)
{
  GtkTreeIter iter;
  GSList *sheets_list;

  /* Delete the contents, if any, of this optionmenu first */

  gtk_list_store_clear (store);

  for (sheets_list = sheets_mods_list; sheets_list;
       sheets_list = g_slist_next (sheets_list)) {
    SheetMod *sheet_mod;
    gchar *location;

    sheet_mod = sheets_list->data;

    /* We don't display sheets which have been deleted prior to Apply */

    if (sheet_mod->mod == SHEETMOD_MOD_DELETED) {
      continue;
    }

    location = sheet_mod->sheet.scope == SHEET_SCOPE_SYSTEM ? _("System")
                                                            : _("User");

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store,
                        &iter,
                        SO_COL_NAME, gettext (sheet_mod->sheet.name),
                        SO_COL_LOCATION, location,
                        SO_COL_MOD, sheet_mod,
                        -1);
  }
}


gboolean
sheets_dialog_create (void)
{
  GSList *sheets_list;
  GtkWidget *option_menu;
  GtkWidget *sw;
  GtkWidget *wrapbox;
  gchar *sheet_left;
  gchar *sheet_right;

  g_clear_slist (&sheets_mods_list, g_free);

  for (sheets_list = get_sheets_list (); sheets_list;
      sheets_list = g_slist_next (sheets_list)) {
    sheets_append_sheet_mods (sheets_list->data);
  }

  if (sheets_dialog == NULL) {
    sheets_dialog = create_sheets_main_dialog ();
    if (!sheets_dialog) {
      /* don not let a broken builder file crash Dia */
      g_warning ("SheetDialog creation failed");
      return FALSE;
    }
    /* Make sure to null our pointer when destroyed */
    g_signal_connect (G_OBJECT (sheets_dialog), "destroy",
                      G_CALLBACK (gtk_widget_destroyed),
                      &sheets_dialog);

    sheet_left = NULL;
    sheet_right = NULL;
  } else {
    option_menu = lookup_widget (sheets_dialog, "combo_left");
    sheet_left = g_object_get_data (G_OBJECT (option_menu),
                                    "active_sheet_name");

    option_menu = lookup_widget (sheets_dialog, "combo_right");
    sheet_right = g_object_get_data (G_OBJECT (option_menu),
                                     "active_sheet_name");

    wrapbox = lookup_widget (sheets_dialog, "wrapbox_left");
    if (wrapbox) {
      gtk_widget_destroy (wrapbox);
    }

    wrapbox = lookup_widget (sheets_dialog, "wrapbox_right");
    if (wrapbox) {
      gtk_widget_destroy (wrapbox);
    }
  }

  sw = lookup_widget (sheets_dialog, "scrolledwindow_right");
  /* In case glade already add a child to scrolledwindow */
  wrapbox = gtk_bin_get_child (GTK_BIN (sw));
  if (wrapbox) {
    gtk_container_remove (GTK_CONTAINER (sw), wrapbox);
  }
  wrapbox = gtk_hwrap_box_new (FALSE);
  g_object_ref (wrapbox);
  g_object_set_data (G_OBJECT (sheets_dialog), "wrapbox_right", wrapbox);
  gtk_container_add (GTK_CONTAINER (sw), wrapbox);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (wrapbox), GTK_JUSTIFY_LEFT);
  gtk_widget_show (wrapbox);
  g_object_set_data (G_OBJECT (wrapbox), "is_left", FALSE);
  option_menu = lookup_widget (sheets_dialog, "combo_right");
  g_object_set_data (G_OBJECT (option_menu), "wrapbox", wrapbox);
  select_sheet (option_menu, sheet_right);

  sw = lookup_widget (sheets_dialog, "scrolledwindow_left");
  /* In case glade already add a child to scrolledwindow */
  wrapbox = gtk_bin_get_child (GTK_BIN (sw));
  if (wrapbox) {
    gtk_container_remove (GTK_CONTAINER (sw), wrapbox);
  }
  wrapbox = gtk_hwrap_box_new (FALSE);
  g_object_ref (wrapbox);
  g_object_set_data (G_OBJECT (sheets_dialog), "wrapbox_left", wrapbox);
  gtk_container_add (GTK_CONTAINER (sw), wrapbox);
  gtk_wrap_box_set_justify (GTK_WRAP_BOX (wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify (GTK_WRAP_BOX (wrapbox), GTK_JUSTIFY_LEFT);
  gtk_widget_show (wrapbox);
  g_object_set_data (G_OBJECT (wrapbox), "is_left", (gpointer) TRUE);
  option_menu = lookup_widget (sheets_dialog, "combo_left");
  g_object_set_data (G_OBJECT (option_menu), "wrapbox", wrapbox);
  select_sheet (option_menu, sheet_left);

  return TRUE;
}


GtkWidget*
lookup_widget (GtkWidget   *widget,
               const gchar *widget_name)
{
  GtkWidget *parent, *found_widget;
  GtkBuilder *builder;

  g_return_val_if_fail (widget != NULL, NULL);

  for (;;) {
    if (GTK_IS_MENU (widget)) {
      parent = gtk_menu_get_attach_widget (GTK_MENU (widget));
    } else {
      parent = gtk_widget_get_parent (widget);
    }

    if (parent == NULL) {
      break;
    }

    widget = parent;
  }

  builder = g_object_get_data (G_OBJECT (widget), "_sheet_dialogs_builder");
  found_widget = GTK_WIDGET (gtk_builder_get_object (builder, widget_name));
  /* not everything is under control of the builder,
   * e.g. "wrapbox_left" */
  if (!found_widget) {
    found_widget = (GtkWidget*) g_object_get_data (G_OBJECT (widget), widget_name);
  }
  if (!found_widget) {
    g_warning (_("Widget not found: %s"), widget_name);
  }
  return found_widget;
}

/* The menu calls us here, after we've been instantiated */
void
sheets_dialog_show_callback (GtkAction *action)
{
  GtkWidget *option_menu;

  if (!sheets_dialog) {
    sheets_dialog_create ();
  }
  if (!sheets_dialog) {
    return;
  }

  option_menu = lookup_widget (sheets_dialog, "combo_left");
  select_sheet (option_menu, interface_current_sheet_name);

  g_assert (GTK_IS_WIDGET (sheets_dialog));
  gtk_widget_show (sheets_dialog);
}
