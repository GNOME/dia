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

#include <glib.h>
#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>

#include "dia_dirs.h"
#include "recent_files.h"
#include "menus.h"
#include "diagram.h"
#include "display.h"
#include "layer_dialog.h"
#include "../lib/filter.h"
#include "../lib/intl.h"
#include "message.h"

static GList *recent_files = NULL;
#ifndef GNOME
static guint recent_files_length = 5;
#endif

/* file and import filter name */
typedef struct _RecentFileData
{
	gchar *filename;
	gchar *importfilterdescription;
} RecentFileData;

void open_recent_file_callback (GtkWidget *widget, RecentFileData *file);

void
recent_file_history_add(const char *fname, DiaImportFilter *ifilter) {
#ifndef GNOME
	GtkWidget *toolbox_filemenu;
	RecentFileData *filedata;
	GList *recent_list_pointer;
	gchar *basename, *menupath;
	GtkItemFactory *toolbox_item_factory;
	GtkItemFactoryEntry *new_entry;
	int length;
	
	/* make sure that the file is not in the list already */
	recent_list_pointer = recent_files;
	while(recent_list_pointer != NULL) {
		filedata = (RecentFileData *)(recent_list_pointer->data);
		if(strcmp(filedata->filename, fname) == 0) return;
		recent_list_pointer = g_list_next(recent_list_pointer);
	}
	
	/* remove recent files if list get's too long */
	toolbox_filemenu = menus_get_item_from_path(N_("/File/Plugins"), NULL)->parent;
	toolbox_item_factory = gtk_item_factory_from_widget(toolbox_filemenu);
	
	if(g_list_length(recent_files) == recent_files_length) {
		filedata = g_list_nth_data(recent_files, recent_files_length-1);
		basename = g_basename(filedata->filename);
		length = strlen(N_("/File/Open Recent/"))+strlen(basename)+1;
		menupath = g_new(char, length);
		g_snprintf(menupath, length, "%s%s", N_("/File/Open Recent/"),basename);
		gtk_item_factory_delete_item(toolbox_item_factory, menupath);

		recent_files = g_list_remove(recent_files, filedata);
		if(filedata) {
			if(filedata->filename)g_free(filedata->filename);
			if(filedata->importfilterdescription)g_free(filedata->importfilterdescription);
			g_free(filedata);
		}
		g_free(menupath);	
	}
	
	/* store in list */
	filedata = g_new(RecentFileData,1);
	filedata->filename = g_strdup(fname);
	if(ifilter != NULL) filedata->importfilterdescription = g_strdup(ifilter->description);
	else filedata->importfilterdescription = NULL;
	recent_files = g_list_prepend(recent_files, filedata);

	/* add to the menu */
	basename = g_strdup(g_basename(fname));
	basename = g_strdelimit(basename, "_", '\\');
#if GLIB_CHECK_VERSION (1,3,0)
	basename = g_strescape(basename, NULL);
#else
	basename = g_strescape(basename);
#endif
	basename = g_strdelimit(basename, "\\", '_');
	new_entry = g_new(GtkItemFactoryEntry, 1);
	length = strlen(N_("/File/Open Recent/"))+strlen(basename)+1;
	new_entry->path = g_new(char, length);
	g_snprintf(new_entry->path, length, "%s%s", N_("/File/Open Recent/"),basename);
	new_entry->accelerator = NULL;
	new_entry->callback = open_recent_file_callback;
	new_entry->callback_action = 0;
	new_entry->item_type = "<Item>";
	gtk_item_factory_create_item(toolbox_item_factory, new_entry, (gpointer)filedata,2);

	/* free stuff */
	g_free(new_entry->path);
	g_free(new_entry);
#endif /* !GNOME */
}

/* load the recent file history */
void
recent_file_history_init() {
	FILE *fp;
	char *buffer, *history_filename, *filename;
	DiaImportFilter *ifilter;
	
	/* should be ~/.dia/history */
	history_filename = dia_config_filename("history");
	if((fp = fopen(history_filename, "r")) == NULL ) {
		g_free(history_filename);
		return;
	}
	buffer = g_new(char, 16000);
	while (fgets(buffer, 16000, fp)!=NULL) {
		filename = g_strchomp(buffer);
		ifilter = filter_guess_import_filter(filename);
		recent_file_history_add(filename, ifilter);
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
		return;
	}
	recent_list_pointer = g_list_last(recent_files);
	while(recent_list_pointer != NULL) {
		filedata = (RecentFileData *)recent_list_pointer->data;
		fprintf(fp, "%s\n", filedata->filename);
		if(filedata) {
			if(filedata->filename)g_free(filedata->filename);
			if(filedata->importfilterdescription)g_free(filedata->importfilterdescription);
			g_free(filedata);
		}
		recent_list_pointer = g_list_previous(recent_list_pointer);		
	}
	fclose(fp);
	g_list_free(recent_files);
}

void
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
	}
}

