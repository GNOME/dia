/* xxxxxx -- an diagram creation/manipulation program
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
#include <gtk/gtk.h>
#include <string.h>

#include "menus.h"
#include "commands.h"
#include "message.h"

#ifdef GTK_HAVE_FEATURES_1_1_0
#define MY_GTK_ACCEL_NAME accel_group
#else
#define MY_GTK_ACCEL_NAME table
#endif /* GTK_HAVE_FEATURES_1_1_0 */

static void menus_init (void);
static void menus_create (GtkMenuEntry         *entries,
			  int                   nmenu_entries);
static gint menus_install_accel (GtkWidget *widget,
				 gchar     *signal_name,
				 gchar      key,
				 gchar      modifiers,
				 gchar     *path);
static void menus_remove_accel (GtkWidget *widget,
				gchar     *signal_name,
				gchar     *path);
static GtkMenuEntry menu_items[] =
{
  {"<Toolbox>/File/New", "<control>N", file_new_callback, NULL},
  {"<Toolbox>/File/Open", "<control>O", file_open_callback, NULL},
  {"<Toolbox>/File/<separator>", NULL, NULL, NULL},
  {"<Toolbox>/File/Quit", "<control>Q", file_quit_callback, NULL},
  /*  {"<Toolbox>/Options/Test", NULL, NULL, NULL},*/
  
  {"<Display>/File/New", "<control>N", file_new_callback, NULL},
  {"<Display>/File/Open", "<control>O", file_open_callback, NULL},
  {"<Display>/File/Save", "<control>S", file_save_callback, NULL},
  {"<Display>/File/Save as", NULL, file_save_as_callback, NULL},
  {"<Display>/File/Export To EPS", NULL, file_export_to_eps_callback, NULL},
  {"<Display>/File/<separator>", NULL, NULL, NULL},
  {"<Display>/File/Close", NULL, file_close_callback, NULL},
  {"<Display>/File/Quit", "<control>Q", file_quit_callback, NULL},
  {"<Display>/Edit/Copy", "<control>C", edit_copy_callback, NULL},
  {"<Display>/Edit/Cut", "<control>X", edit_cut_callback, NULL},
  {"<Display>/Edit/Paste", "<control>V", edit_paste_callback, NULL},
  {"<Display>/Edit/Delete", "<control>D", edit_delete_callback, NULL},
  {"<Display>/View/Zoom in", "+", view_zoom_in_callback, NULL},
  {"<Display>/View/Zoom out", "-", view_zoom_out_callback, NULL},
  {"<Display>/View/Zoom/400%", NULL, view_zoom_set_callback, (gpointer) 400},
  {"<Display>/View/Zoom/200%", NULL, view_zoom_set_callback, (gpointer) 200},
  {"<Display>/View/Zoom/100%", NULL, view_zoom_set_callback, (gpointer) 100},
  {"<Display>/View/Zoom/50%", NULL, view_zoom_set_callback, (gpointer) 50},
  {"<Display>/View/Zoom/25%", NULL, view_zoom_set_callback, (gpointer) 25},
  {"<Display>/View/Edit Grid...", NULL, view_edit_grid_callback, NULL},
  {"<Display>/View/<check>Visible Grid", NULL, view_visible_grid_callback, NULL},
  {"<Display>/View/<check>Snap To Grid", NULL, view_snap_to_grid_callback, NULL},
  {"<Display>/View/<check>Toggle Rulers", NULL, view_toggle_rulers_callback, NULL},
  {"<Display>/View/<separator>", NULL, NULL, NULL},
  {"<Display>/View/New View", NULL, view_new_view_callback, NULL},
  {"<Display>/View/Show All", NULL, view_show_all_callback, NULL},
  {"<Display>/Objects/Place Under", NULL, objects_place_under_callback, NULL},
  {"<Display>/Objects/Place Over", NULL, objects_place_over_callback, NULL},
  {"<Display>/Objects/<separator>", NULL, NULL, NULL},
  {"<Display>/Objects/Group", NULL, objects_group_callback, NULL},
  {"<Display>/Objects/Ungroup", NULL, objects_ungroup_callback, NULL},
};

/* calculate the number of menu_item's */
static int nmenu_items = sizeof(menu_items) / sizeof(menu_items[0]);

static int initialize = TRUE;
static GtkMenuFactory *factory = NULL;
static GtkMenuFactory *subfactories[2];
static GHashTable *entry_ht = NULL;

static void 
menus_init(void)
{
  if (initialize) {
    initialize = FALSE;
              
    factory = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
    
    subfactories[0] = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU_BAR);
    gtk_menu_factory_add_subfactory(factory, subfactories[0], "<Toolbox>");

    subfactories[1] = gtk_menu_factory_new(GTK_MENU_FACTORY_MENU);
    gtk_menu_factory_add_subfactory(factory, subfactories[1], "<Display>");

    menus_create(menu_items, nmenu_items);
  }
}


void
menus_get_toolbox_menubar (GtkWidget         **menubar,
			   MY_GTK_ACCEL_TYPE **accel)
{
  if (initialize)
    menus_init ();

  if (menubar)
    *menubar = subfactories[0]->widget;
  if (accel) {
    *accel = subfactories[0] -> MY_GTK_ACCEL_NAME;
  }
}

void
menus_get_image_menu (GtkWidget         **menu,
		      MY_GTK_ACCEL_TYPE **accel)
{
  if (initialize)
    menus_init ();

    if (menu)
      *menu = subfactories[1]->widget;
    if (accel) {
      *accel = subfactories[1]-> MY_GTK_ACCEL_NAME;
    }
}


void
menus_create (GtkMenuEntry *entries,
	      int           nmenu_entries)
{
  char *accelerator;
  int i;

  if (initialize)
    menus_init ();
  
  if (entry_ht) {
    for (i = 0; i < nmenu_entries; i++) {
      accelerator = g_hash_table_lookup (entry_ht, entries[i].path);
      if (accelerator) {
	if (accelerator[0] == '\0')
	  entries[i].accelerator = NULL;
	else
	  entries[i].accelerator = accelerator;
      }
    }
  }
  gtk_menu_factory_add_entries (factory, entries, nmenu_entries);

  
#ifndef GTK_HAVE_FEATURES_1_1_0
  for (i = 0; i < nmenu_entries; i++) {
    if (entries[i].widget && GTK_BIN (entries[i].widget)->child) {
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "install_accelerator",
			    (GtkSignalFunc) menus_install_accel,
			    entries[i].path);
	gtk_signal_connect (GTK_OBJECT (entries[i].widget), "remove_accelerator",
			    (GtkSignalFunc) menus_remove_accel,
			    entries[i].path);
    }
  }
#endif
}


static gint
menus_install_accel (GtkWidget *widget,
		     gchar     *signal_name,
		     gchar      key,
		     gchar      modifiers,
		     gchar     *path)
{
  char accel[64];
  char *t1, t2[2];

  accel[0] = '\0';
  if (modifiers & GDK_CONTROL_MASK)
    strcat (accel, "<control>");
  if (modifiers & GDK_SHIFT_MASK)
    strcat (accel, "<shift>");
  if (modifiers & GDK_MOD1_MASK)
    strcat (accel, "<alt>");

  t2[0] = key;
  t2[1] = '\0';
  strcat (accel, t2);

  if (entry_ht)
    {
      t1 = g_hash_table_lookup (entry_ht, path);
      g_free (t1);
    }
  else
    entry_ht = g_hash_table_new (g_str_hash, g_str_equal);

  g_hash_table_insert (entry_ht, path, g_strdup (accel));

  return TRUE;
}

static void
menus_remove_accel (GtkWidget *widget,
		    gchar     *signal_name,
		    gchar     *path)
{
  char *t;

  if (entry_ht)
    {
      t = g_hash_table_lookup (entry_ht, path);
      g_free (t);

      g_hash_table_insert (entry_ht, path, g_strdup (""));
    }
}

void
menus_set_sensitive (char *path,
                     int   sensitive)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    gtk_widget_set_sensitive (menu_path->widget, sensitive);
  else
    message_error("Unable to set sensitivity for menu which doesn't exist: %s", path);
}

void
menus_set_state (char *path,
                 int   state)
{
  GtkMenuPath *menu_path;

  if (initialize)
    menus_init ();

  menu_path = gtk_menu_factory_find (factory, path);
  if (menu_path)
    {
      if (GTK_IS_CHECK_MENU_ITEM (menu_path->widget))
        gtk_check_menu_item_set_state (GTK_CHECK_MENU_ITEM (menu_path->widget), state);
    }
  else
    message_error("Unable to set state for menu which doesn't exist: %s", path);
}
