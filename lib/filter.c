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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "filter.h"

#include <string.h>

static GList *export_filters = NULL;
static GList *import_filters = NULL;
static GList *callback_filters = NULL;

static gint
export_filter_compare(gconstpointer a, gconstpointer b)
{
  const DiaExportFilter *fa = a, *fb = b;

  return g_ascii_strcasecmp(_(fa->description), _(fb->description));
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

void
filter_unregister_export(DiaExportFilter *efilter)
{
  export_filters = g_list_remove(export_filters, efilter);
}

/* returns a sorted list of the export filters. */
GList *
filter_get_export_filters(void)
{
  return export_filters;
}


/* creates a nice label for the export filter (must be g_free'd) */
char *
filter_get_export_filter_label (DiaExportFilter *efilter)
{
  GString *str = g_string_new (_(efilter->description));
  int ext = 0;

  for (ext = 0; efilter->extensions[ext] != NULL; ext++) {
    if (ext == 0) {
      g_string_append (str, " (*.");
    } else {
      g_string_append (str, ", *.");
    }
    g_string_append (str, efilter->extensions[ext]);
  }

  if (ext > 0) {
    g_string_append (str, ")");
  }

  return g_string_free (str, FALSE);
}


/* Get the list of unique names for the given extension */
GList *
filter_get_unique_export_names(const char *ext)
{
  GList *tmp, *res = NULL;

  for (tmp = export_filters; tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;
    gint i;

    for (i = 0; ef->extensions[i] != NULL; i++) {
      if (!g_ascii_strcasecmp(ef->extensions[i], ext) && ef->unique_name)
	res = g_list_append (res, (char *)ef->unique_name);
    }
  }
  return res;
}

static GHashTable *_favored_hash = NULL;

/* Set the favorit 'guess' */
void
filter_set_favored_export(const char *ext, const char *name)
{
  if (!_favored_hash)
    _favored_hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

  g_hash_table_insert(_favored_hash, g_ascii_strdown(ext, -1), g_strdup(name));
}

/* Guess the filter for a given filename.
 * Returns the first filter found that matches the extension on the filename,
 * or NULL if none such are found.
 * If there are multiple filters registered for the same extension some are
 * excluded from being returned here by the hint FILTER_DONT_GUESS.
 */
DiaExportFilter *
filter_guess_export_filter(const gchar *filename)
{
  GList *tmp;
  gchar *ext;
  gint   no_guess = 0;
  DiaExportFilter *dont_guess = NULL;
  const gchar *unique_name;

  ext = strrchr(filename, '.');
  if (ext)
    ext++;
  else
    ext = "";

  /* maybe ther is no need to guess? */
  unique_name = _favored_hash ? g_hash_table_lookup(_favored_hash, ext) : NULL;
  if (unique_name) {
    DiaExportFilter *ef = filter_export_get_by_name(unique_name);
    if (ef)
      return ef;
  }

  for (tmp = export_filters; tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;
    gint i;

    for (i = 0; ef->extensions[i] != NULL; i++) {
      if (!g_ascii_strcasecmp(ef->extensions[i], ext)) {
        if (ef->hints & FILTER_DONT_GUESS) {
          dont_guess = ef;
	  ++no_guess;
          continue;
	}
	return ef;
      }
    }
  }
  return (no_guess == 1) ? dont_guess : NULL;
}

/**
 * filter_export_get_by_name:
 * @name: the filter name
 *
 * Get an export filter by unique name.
 */
DiaExportFilter *
filter_export_get_by_name(const gchar *name)
{
  GList *tmp;
  DiaExportFilter *filter = NULL;

  for (tmp = export_filters; tmp != NULL; tmp = tmp->next) {
    DiaExportFilter *ef = tmp->data;
    if (ef->unique_name != NULL) {
      if (!g_ascii_strcasecmp(ef->unique_name, name)) {
	if (filter)
	  g_warning(_("Multiple export filters with unique name %s"), name);
	filter = ef;
      }
    }
  }
  return filter;
}
DiaImportFilter *
filter_import_get_by_name(const gchar *name)
{
  GList *tmp;
  DiaImportFilter *filter = NULL;

  for (tmp = import_filters; tmp != NULL; tmp = tmp->next) {
    DiaImportFilter *af = tmp->data;
    if (af->unique_name != NULL) {
      if (!g_ascii_strcasecmp(af->unique_name, name)) {
	if (filter)
	  g_warning(_("Multiple import filters with unique name %s"), name);
	filter = af;
      }
    }
  }
  return filter;
}

static gint
import_filter_compare(gconstpointer a, gconstpointer b)
{
  const DiaImportFilter *fa = a, *fb = b;

  return g_ascii_strcasecmp(_(fa->description), _(fb->description));
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

void
filter_unregister_import(DiaImportFilter *ifilter)
{
  import_filters = g_list_remove(import_filters, ifilter);
}

/* returns a sorted list of the export filters. */
GList *
filter_get_import_filters(void)
{
  return import_filters;
}


/* creates a nice label for the export filter (must be g_free'd) */
char *
filter_get_import_filter_label (DiaImportFilter *ifilter)
{
  GString *str = g_string_new (_(ifilter->description));
  int ext = 0;

  for (ext = 0; ifilter->extensions[ext] != NULL; ext++) {
    if (ext == 0) {
      g_string_append (str, " (*.");
    } else {
      g_string_append (str, ", *.");
    }
    g_string_append (str, ifilter->extensions[ext]);
  }

  if (ext > 0) {
    g_string_append(str, ")");
  }

  return g_string_free (str, FALSE);
}


/* guess the filter for a given filename.
 * If there are multiple filters registered for the same extension some are
 * excluded from being returned here by the hint FILTER_DONT_GUESS.
 */
DiaImportFilter *
filter_guess_import_filter(const gchar *filename)
{
  GList *tmp;
  gchar *ext;
  int no_guess = 0;
  DiaImportFilter *dont_guess = NULL;

  ext = strrchr(filename, '.');
  if (ext)
    ext++;
  else
    ext = "";

  for (tmp = import_filters; tmp != NULL; tmp = tmp->next) {
    DiaImportFilter *ifilter = tmp->data;
    gint i;

    for (i = 0; ifilter->extensions[i] != NULL; i++) {
      if (!g_ascii_strcasecmp(ifilter->extensions[i], ext)) {
        if (ifilter->hints & FILTER_DONT_GUESS) {
          dont_guess = ifilter;
          ++no_guess;
          continue;
	}
	return ifilter;
      }
    }
  }
  return (no_guess == 1) ? dont_guess : NULL;
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
  g_return_if_fail (cbfilter->action != NULL);

  /* callback_filters is always pointing to the last element */
  callback_filters = g_list_append (callback_filters, (gpointer)cbfilter);
}

void
filter_unregister_callback(DiaCallbackFilter *cbfilter)
{
  /* Unregistering a callback filter should remove it from the menu, too.
   * But that would involve quite some additional code, maybe some
   * notification to update menus ...
   * We take the easy approach to let the caller check if the callback
   * is still valid, i.e. the filter still exists.
   */
  callback_filters = g_list_remove (callback_filters, cbfilter);
}


/* return the registered callbacks list, called once to build menus */
GList *
filter_get_callbacks(void)
{
  return g_list_first (callback_filters);
}

