/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * filter.c: definitions for adding new loaders and export filters.
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "filter.h"
#include "intl.h"
#include <string.h>

static GList *export_filters = NULL;
static GList *import_filters = NULL;
static GList *callback_filters = NULL;

static gint
export_filter_compare(gconstpointer a, gconstpointer b)
{
  const DiaExportFilter *fa = a, *fb = b;

  return g_strcasecmp(_(fa->description), _(fb->description));
}

void
filter_register_export(DiaExportFilter *efilter)
{
  if (efilter->description == NULL) {
    return;
  }
  export_filters = g_list_insert_sorted(export_filters, efilter,
					export_filter_compare);
}

/* returns a sorted list of the export filters. */
GList *
filter_get_export_filters(void)
{
  return export_filters;
}

/* creates a nice label for the export filter (must be g_free'd) */
gchar *
filter_get_export_filter_label(DiaExportFilter *efilter)
{
  GString *str = g_string_new(_(efilter->description));
  gint ext = 0;
  gchar *ret;

  for (ext = 0; efilter->extensions[ext] != NULL; ext++) {
    if (ext == 0)
      g_string_append(str, " (*.");
    else
      g_string_append(str, ", *.");
    g_string_append(str, efilter->extensions[ext]);
  }
  if (ext > 0)
    g_string_append(str, ")");
  ret = str->str;
  g_string_free(str, FALSE);
  return ret;
}

/* guess the filter for a given filename. */
DiaExportFilter *
filter_guess_export_filter(const gchar *filename)
{
  GList *tmp;
  gchar *ext;

  ext = strrchr(filename, '.');
  if (ext)
    ext++;
  else
    ext = "";

  for (tmp = export_filters; tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;
    gint i;

    for (i = 0; ef->extensions[i] != NULL; i++)
      if (!g_strcasecmp(ef->extensions[i], ext))
	return ef;
  }
  return NULL;
}

static gint
import_filter_compare(gconstpointer a, gconstpointer b)
{
  const DiaImportFilter *fa = a, *fb = b;

  return g_strcasecmp(_(fa->description), _(fb->description));
}

void
filter_register_import(DiaImportFilter *ifilter)
{
  if (ifilter->description == NULL) {
    return;
  }
  import_filters = g_list_insert_sorted(import_filters, ifilter,
					import_filter_compare);
}

/* returns a sorted list of the export filters. */
GList *
filter_get_import_filters(void)
{
  return import_filters;
}

/* creates a nice label for the export filter (must be g_free'd) */
gchar *
filter_get_import_filter_label(DiaImportFilter *ifilter)
{
  GString *str = g_string_new(_(ifilter->description));
  gint ext = 0;
  gchar *ret;

  for (ext = 0; ifilter->extensions[ext] != NULL; ext++) {
    if (ext == 0)
      g_string_append(str, " (*.");
    else
      g_string_append(str, ", *.");
    g_string_append(str, ifilter->extensions[ext]);
  }
  if (ext > 0)
    g_string_append(str, ")");
  ret = str->str;
  g_string_free(str, FALSE);
  return ret;
}

/* guess the filter for a given filename. */
DiaImportFilter *
filter_guess_import_filter(const gchar *filename)
{
  GList *tmp;
  gchar *ext;

  ext = strrchr(filename, '.');
  if (ext)
    ext++;
  else
    ext = "";

  for (tmp = import_filters; tmp != NULL; tmp = tmp->next) {
    DiaImportFilter *efilter = tmp->data;
    gint i;

    for (i = 0; efilter->extensions[i] != NULL; i++)
      if (!g_strcasecmp(efilter->extensions[i], ext))
	return efilter;
  }
  return NULL;
}

/* register a new callback from a plug-in */
void 
filter_register_callback(DiaCallbackFilter *cbfilter)
{
  /* sanity check */
  g_return_if_fail (cbfilter != NULL);
  g_return_if_fail (cbfilter->callback != NULL);
  g_return_if_fail (cbfilter->menupath != NULL);
  g_return_if_fail (cbfilter->description != NULL);

  /* callback_filters is always pointing to the last element */
  callback_filters = g_list_append (callback_filters, (gpointer)cbfilter);
}

/* return the registered callbacks list, called once to build menus */
GList *
filter_get_callbacks(void)
{
  return g_list_first (callback_filters);
}
