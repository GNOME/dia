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

static GList *recent_files = NULL;
static guint recent_files_length = 0;
static guint recent_files_menuitem_offset = 0;
static GtkTooltips *tooltips = 0;

/* file and import filter name */
typedef struct _RecentFileData
{
	gchar *filename;
	gchar *importfilterdescription;
} RecentFileData;

static void open_recent_file_callback (GtkWidget *widget, RecentFileData *file);
void recent_file_history_remove (const char *fname);

static RecentFileData *
recent_file_filedata_new(const char *fname, DiaImportFilter *ifilter)
{
	RecentFileData *filedata;

	filedata = g_new(RecentFileData, 1);
	filedata->filename = g_strdup(fname);

	if (ifilter)
		filedata->importfilterdescription
		              = g_strdup(ifilter->description);
	else
		filedata->importfilterdescription = NULL;

	return (filedata);
}

static void
recent_file_menuitem_create(GtkWidget *menu, 
                            RecentFileData *filedata, guint pos)
{
	gchar *basename, *escaped, *label;
	GtkWidget *item;
	GtkAccelGroup *accel_group;

	basename = g_path_get_basename(filedata->filename);
	basename = g_strdelimit(basename, "_", '\\');
	escaped  = g_strescape(basename, NULL);
	g_free(basename);
	basename = escaped;
	basename = g_strdelimit(basename, "\\", '_');

	label = g_strdup_printf("%d. %s", pos, basename);
	item = gtk_menu_item_new_with_label(label);

	gtk_menu_insert(GTK_MENU(menu), item,
	                pos + recent_files_menuitem_offset);

	g_signal_connect(GTK_OBJECT(item), "activate",
			 G_CALLBACK(open_recent_file_callback),filedata);

	gtk_tooltips_set_tip(GTK_TOOLTIPS(tooltips), item,
	                     filedata->filename, NULL);

	if (pos < 10)
	{
		accel_group = gtk_accel_group_new();
		gtk_window_add_accel_group(GTK_WINDOW(
		                           interface_get_toolbox_shell()),
		                           accel_group);
		gtk_widget_add_accelerator(item, "activate", accel_group,
		                           GDK_1 + pos - 1,
		                           GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
	}

	gtk_widget_show(item);

	g_free(basename);
	/* gtk_label_set_text() g_strdup's our label, so... */
	g_free(label);
}

static gint
recent_file_compare_fnames(gconstpointer element, gconstpointer userdata)
{
	/* no g_strcmp() in GLib */
	return strcmp(((RecentFileData *)element)->filename, userdata);
}

static GtkWidget *
recent_file_filemenu_get(void)
{
	/* Use the Plugins menu item to get a pointer to the File menu,
	   but any item on the File menu will do */

	return GTK_WIDGET(menus_get_item_from_path(N_("<Toolbox>/File/Plugins..."),
	                                NULL))->parent;
}

void
recent_file_history_add(const char *fname, DiaImportFilter *ifilter,
                        guint is_initial_load)
{
	GtkWidget *file_menu;
	RecentFileData *filedata;
	guint i, number_of_items;
	GList *menu_items, *item, *next;

	file_menu = recent_file_filemenu_get();

	/* An initial load places filename items in natural order 1, 2, 3,...
	   but new items get inserted in position 1, forcing other items down */

	if (is_initial_load)
	{
	        gchar *fullname = dia_get_absolute_filename(fname);
		filedata = recent_file_filedata_new(fullname, ifilter);
		recent_files = g_list_append(recent_files, filedata);

		i = g_list_length(recent_files);

		if (i <= recent_files_length)
			recent_file_menuitem_create(file_menu, filedata, i);
		g_free(fullname);
	}
	else
	{	
	        gchar *fullname = dia_get_absolute_filename(fname);

		/* Since the recent filenames on the menu have positional
		   text and accellerators, delete the existing recent file
		   menu items; start fresh each time */


		menu_items = GTK_MENU_SHELL(file_menu)->children;
		item = g_list_first(menu_items);

		for (i = 0; i <= recent_files_menuitem_offset; i++)
			item = g_list_next(item);

		number_of_items = MIN(g_list_length(recent_files),
		                      recent_files_length);

		for (i = 0; i < number_of_items; i++)
		{
			next = g_list_next(item);

			/* Unlink first, then destroy */

			menu_items = g_list_remove_link(menu_items, item);
			gtk_widget_destroy(item->data);
			g_list_free_1(item);

			item = next;
		}

		/* Three (3) cases:

		   a) The filename is not in the 'recent_files' list--
		      insert it in position 1;
		   b) The filename is in the 'recent_files' list but not 
		      displayed because it is clipped by 'recent_files_length'--
		      move it to position 1;
		   c) The filename is displayed on the menu--leave it in place.
		*/

		item = g_list_find_custom(recent_files, (gpointer)fullname,
		                          recent_file_compare_fnames);
		if (item)
		{
			i = g_list_position(recent_files, item);

/* 			if (i >= recent_files_length) */
			{
				recent_files = g_list_remove_link(recent_files,
				                                  item);
				recent_files = g_list_concat(item,
				                             recent_files);
			}
		}
		else
		{
			filedata = recent_file_filedata_new(fullname, ifilter);
			recent_files = g_list_prepend(recent_files, filedata);
		}

		number_of_items = MIN(g_list_length(recent_files),
		                      recent_files_length);

		item = g_list_first(recent_files);

		for (i = 1; i <= number_of_items; i++)
		{
			recent_file_menuitem_create(file_menu, item->data, i);
			item = g_list_next(item);
		}
		g_free(fullname);
	}
}

/* load the recent file history */
void
recent_file_history_init() {
	FILE *fp;
	char *buffer, *history_filename, *filename;
	DiaImportFilter *ifilter;
	GtkWidget *file_menu;
	GtkMenuItem *menu_item;
	GList *list_item;
	
	/* should be ~/.dia/history */
	history_filename = dia_config_filename("history");
	if((fp = fopen(history_filename, "r")) == NULL ) {
		g_free(history_filename);
		return;
	}

	/* Must restart dia to use new prefs value */

	prefs.recent_documents_list_size = 
		CLAMP(prefs.recent_documents_list_size, 0, 16);
	recent_files_length = prefs.recent_documents_list_size;

	menu_item = menus_get_item_from_path(N_("<Toolbox>/File/Quit"), NULL);

	file_menu = recent_file_filemenu_get();

	list_item = g_list_find(GTK_MENU_SHELL(file_menu)->children,
	                        (gpointer)menu_item);

	recent_files_menuitem_offset
	        = g_list_position(GTK_MENU_SHELL(file_menu)->children,
	                          list_item) - 2;  /* fudge factor */

	tooltips = gtk_tooltips_new();

	buffer = g_new(char, 16000);
	while (fgets(buffer, 16000, fp)!=NULL) {
		filename = g_strchomp(buffer);
		ifilter = filter_guess_import_filter(filename);
		recent_file_history_add(filename, ifilter, 1);
	}	
	fclose(fp);

	g_free(buffer);
	g_free(history_filename);
}

/*save the recent file history */
void
recent_file_history_write() {
	GList *recent_list_pointer;
	FILE *fp;
	gchar *history_filename;
	RecentFileData *filedata;
	
	/* should be ~/.dia/history */
	history_filename = dia_config_filename("history");
	if((fp = fopen(history_filename,"w")) == NULL) {
		message_error(N_("Can't open history file for writing."));
		g_free(history_filename);
		return;
	}
	recent_list_pointer = g_list_first(recent_files);
	while(recent_list_pointer != NULL) {
		filedata = (RecentFileData *)recent_list_pointer->data;
		fprintf(fp, "%s\n", filedata->filename);
		if(filedata) {
			if(filedata->filename)g_free(filedata->filename);
			if(filedata->importfilterdescription)g_free(filedata->importfilterdescription);
			g_free(filedata);
		}
		recent_list_pointer = g_list_next(recent_list_pointer);		
	}
	fclose(fp);
	g_list_free(recent_files);
	g_free(history_filename);
}

/* remove a broken file from the history and update menu accordingly
 * Xing Wang, 2002.06 */
void
recent_file_history_remove (const char *fname) 
{
    GtkWidget *file_menu;
    guint j, i, number_of_items;
    GList *file, *next, *item, *menu_items;

    number_of_items = MIN(g_list_length(recent_files),
			  recent_files_length);
    
    file = g_list_first (recent_files);
    
    for (j = 0; j < number_of_items; j++) {
	next = g_list_next (file);
	if (strcmp (((RecentFileData *)file->data)->filename, fname) == 0) {
	    file_menu = recent_file_filemenu_get ();
	    menu_items = GTK_MENU_SHELL(file_menu)->children;

		/* remove all menu items after THIS ONE  */
	    
	    item = g_list_nth (menu_items,
			       recent_files_menuitem_offset + j + 1);
	    for (i = j; i < number_of_items; i++) {
		next = g_list_next(item);

		    /* Unlink first, then destroy */

		menu_items = g_list_remove_link(menu_items, item);
		gtk_widget_destroy(item->data);
		g_list_free_1(item);
		
		item = next;
	    }

	    recent_files = g_list_delete_link (recent_files, file);

		/* recreate all menu items after THIS ONE  */
	    
	    number_of_items = MIN(g_list_length(recent_files),
				  recent_files_length);
	    item = g_list_nth (recent_files, j);
	    for (i = j + 1; i <= number_of_items; i++) {
		recent_file_menuitem_create(file_menu, item->data, i);
		item = g_list_next(item);
	    }
	    return;
	}
	file = next;
    }
}
    
static void
open_recent_file_callback (GtkWidget *widget, RecentFileData *file) {
	GList *import_filters;
	DiaImportFilter *ifilter = NULL;
	Diagram *diagram = NULL;
	
	/* find the import filter */
	if(file->importfilterdescription != NULL) {
		import_filters = filter_get_import_filters();
		do {
			if(strcmp(((DiaImportFilter *) import_filters->data)->description, 
		   	   file->importfilterdescription) == 0) {
				ifilter = import_filters->data;
				break;
			} 
		}
		while((import_filters = g_list_next(import_filters)) != NULL);
	}
	diagram = diagram_load(file->filename, ifilter);
	if (diagram != NULL) {
	    diagram_update_extents(diagram);
	    layer_dialog_set_diagram(diagram);
	    new_display(diagram);
	} else
	    recent_file_history_remove (file->filename);
}
