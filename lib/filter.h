/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filter.h: definitions for adding new loaders and export filters.
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
#ifndef FILTER_H
#define FILTER_H

#include "diatypes.h"
#include <glib.h>
#include <diagramdata.h>

typedef void (* DiaExportFunc) (DiagramData *dia, const gchar *filename,
				const gchar *diafilename, void* user_data);

struct _DiaExportFilter {
  const gchar *description;
  /* NULL terminated array of extensions for this file format.  The first
   * one is the prefered extension for files of this type. */
  const gchar **extensions;
  DiaExportFunc export_func;
  void* user_data;
  /* A unique name that this filter can always be addressed as from the
   * command line.  If more than one filter can handle an extension, this
   * will allow disambiguation.  Note that import and export filters are
   * treated seperately for this.
   */
  const gchar *unique_name;
};

/* returns FALSE on error loading diagram */
typedef gboolean (* DiaImportFunc) (const gchar *filename, DiagramData *dia, 
                                    void* user_data);

struct _DiaImportFilter {
  const gchar *description;
  /* NULL terminated array of extensions for this file format.  The first
   * one is the prefered extension for files of this type. */
  const gchar **extensions;
  DiaImportFunc import_func;
  void* user_data;
  /* A unique name that this filter can always be addressed as from the
   * command line.  If more than one filter can handle an extension, this
   * will allow disambiguation.  Note that import and export filters are
   * treated seperately for this.
   */
  const gchar *unique_name;
};

/* gets called as menu callback */
typedef void (* DiaCallbackFunc) (DiagramData *dia,
                                  guint flags, /* further additions */
                                  void* user_data);

struct _DiaCallbackFilter {
  const gchar *description;
  const gchar *menupath;
  DiaCallbackFunc callback;
  void* user_data;
};

/* register an export filter.  The DiaExportFilter should be static, and
 * none of its fields should be freed */
void filter_register_export(DiaExportFilter *efilter);
/* except - maybe - after unregistering it */
void filter_unregister_export(DiaExportFilter *efilter);

/* returns a sorted list of the export filters. */
GList *filter_get_export_filters(void);

/* creates a nice label for the export filter (must be g_free'd) */
gchar *filter_get_export_filter_label(DiaExportFilter *efilter);

/* guess the filter for a given filename. */
DiaExportFilter *filter_guess_export_filter(const gchar *filename);
/* Get the filter for the unique filename. */
DiaExportFilter *filter_get_by_name(const gchar *name);

void filter_register_import(DiaImportFilter *ifilter);
void filter_unregister_import(DiaImportFilter *ifilter);
GList *filter_get_import_filters(void);
gchar *filter_get_import_filter_label(DiaImportFilter *ifilter);
DiaImportFilter *filter_guess_import_filter(const gchar *filename);

void filter_register_callback(DiaCallbackFilter *cbfilter);
/* returns all registered callbacks */
GList *filter_get_callbacks(void);

#endif
