/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filter.h: definitions for adding new loaders and export filters.
 * Copyright (C) 1998 James Henstridge
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

#include <glib.h>
#include <diagramdata.h>

typedef struct _DiaExportFilter DiaExportFilter;
typedef void (* DiaExportFunc) (DiagramData *dia, const gchar *filename);

struct _DiaExportFilter {
  const gchar *description;
  /* NULL terminated array of extensions for this file format.  The first
   * one is the prefered extension for files of this type. */
  const gchar **extensions;
  DiaExportFunc export;
};

/* register an export filter.  The DiaExportFilter should be static, and
 * none of its fields should be freed */
void filter_register_export(DiaExportFilter *efilter);

/* returns a sorted list of the export filters. */
GList *filter_get_export_filters(void);

/* creates a nice label for the export filter (must be g_free'd) */
gchar *filter_get_export_filter_label(DiaExportFilter *efilter);

/* guess the filter for a given filename. */
DiaExportFilter *filter_guess_export_filter(const gchar *filename);

#endif
