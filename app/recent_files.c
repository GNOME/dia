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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef GNOME
#include <gnome.h>
#else
#include <gdk/gdkkeysyms.h>
#endif

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
#include "layer_dialog.h"
#include "preferences.h"
#include "../lib/filter.h"
#include "../lib/intl.h"
#include "message.h"
#include "persistence.h"

static GtkTooltips *tooltips = 0;

static void open_recent_file_callback (GtkWidget *widget, gpointer data);
void recent_file_history_remove (const char *fname);

static GtkWidget *
recent_file_filemenu_get(void)
{
    /* Use the Plugins menu item to get a pointer to the File menu,
       but any item on the File menu will do */
    
    return GTK_WIDGET(menus_get_item_from_path(N_("<Toolbox>/File/Plugins..."),
					       NULL))->parent;
}

static void
recent_file_history_clear_menu()
{
    GtkWidget *file_menu = recent_file_filemenu_get();
    GtkMenuItem *menu_item = 
	menus_get_item_from_path(N_("<Toolbox>/File/Quit"), NULL);

    GList *menu_items = GTK_MENU_SHELL(file_menu)->children;
    GList *next_item;
    
    for (;menu_items != NULL; menu_items = next_item) {
	GtkMenuItem *item;
	next_item = g_list_next(menu_items);
	item = GTK_MENU_ITEM(menu_items->data);
	if (g_signal_handler_find(G_OBJECT(item), G_SIGNAL_MATCH_FUNC,
				  0, 0, NULL, open_recent_file_callback, NULL)) {
	    GList *tmplist;
	    /* Unlink first, then destroy */
	    g_list_remove_link(GTK_MENU_SHELL(file_menu)->children,
			       menu_items);
	    gtk_widget_destroy(GTK_WIDGET(item));
	    g_list_free_1(menu_items);
	}
    }
}

/** Create a single menu item at position pos in the recent files list.
 * Pos starts from 0.
 */
static void
recent_file_menuitem_create(GtkWidget *menu, gchar *filename, 
			    guint pos, guint offset)
{
    gchar *basename, *escaped, *label;
    GtkWidget *item;
    GtkAccelGroup *accel_group;
    
    basename = g_path_get_basename(filename);
    basename = g_strdelimit(basename, "_", '\\');
    escaped  = g_strescape(basename, NULL);
    //g_free(basename);
    basename = escaped;
    basename = g_strdelimit(basename, "\\", '_');
    
    label = g_strdup_printf("%d. %s", pos+1, basename);
    item = gtk_menu_item_new_with_label(label);
    gtk_menu_insert(GTK_MENU(menu), item,
		    pos + offset);
    
    g_signal_connect(GTK_OBJECT(item), "activate",
		     G_CALLBACK(open_recent_file_callback), filename);
    
    gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), item,
    			 filename, NULL);
    
    if (pos < 9)
    {
	accel_group = gtk_accel_group_new();
	gtk_window_add_accel_group(GTK_WINDOW(interface_get_toolbox_shell()),
				   accel_group);
	gtk_widget_add_accelerator(item, "activate", accel_group,
				   GDK_1 + pos,
				   GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    }
    
    gtk_widget_show(item);
    
    g_free(basename);
    /* gtk_label_set_text() g_strdup's our label, so... */
    g_free(label);
}

/** Build and insert the recent files menu */
void
recent_file_history_make_menu()
{
    guint i;

    GList *items = persistent_list_get_glist("recent-files");

    GtkWidget *file_menu = recent_file_filemenu_get();

    GtkMenuItem *menu_item = 
	menus_get_item_from_path(N_("<Toolbox>/File/Quit"), NULL);

    GList *list_item = g_list_find(GTK_MENU_SHELL(file_menu)->children,
				   (gpointer)menu_item);
    
    int offset = g_list_position(GTK_MENU_SHELL(file_menu)->children,
			  list_item) - 1;  /* fudge factor */

    for (i = 0; items != NULL && i < prefs.recent_documents_list_size;
	 items = g_list_next(items), i++) {
	recent_file_menuitem_create(file_menu, (gchar *)items->data, i, offset);
    }
}

/** Add a new item to the file history list.
 * Since this only happens when a new files is opened, we can afford the
 * time it takes to rebuild the menus, rather than messing around with
 * moving them.
 */
void
recent_file_history_add(const char *fname)
{
    gchar *absname = dia_get_absolute_filename(fname);
    recent_file_history_clear_menu();
    persistent_list_add("recent-files", absname);
    g_free(absname);
    
    recent_file_history_make_menu();
}

/* load the recent file history */
void
recent_file_history_init() {
    PersistentList *plist;

    prefs.recent_documents_list_size = 
	CLAMP(prefs.recent_documents_list_size, 0, 16);
    
    tooltips = gtk_tooltips_new();

    persistence_register_list("recent-files");

    recent_file_history_make_menu();
}

/* remove a broken file from the history and update menu accordingly
 * Xing Wang, 2002.06 */
void
recent_file_history_remove (const char *fname) 
{
    gchar *absname = dia_get_absolute_filename(fname);
    recent_file_history_clear_menu();

    persistent_list_remove("recent-files", absname);
    g_free(absname);

    recent_file_history_make_menu();
}
    
static void
open_recent_file_callback(GtkWidget *widget, gpointer data)
{
	DiaImportFilter *ifilter = NULL;
	Diagram *diagram = NULL;
	gchar *filename = (gchar *)data;

	ifilter = filter_guess_import_filter(filename);
	
	diagram = diagram_load(filename, ifilter);
	if (diagram != NULL) {
	    diagram_update_extents(diagram);
	    diagram_set_current(diagram);
	    new_display(diagram);
	} else
	    recent_file_history_remove (filename);
}
