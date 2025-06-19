/*
 * File: plug-ins/xslt/xslt.c
 *
 * Made by Matthieu Sozeau <mattam@netcourrier.com>
 *
 * Started on  Thu May 16 23:21:43 2002 Matthieu Sozeau
 * Last update Tue Jun  4 21:00:30 2002 Matthieu Sozeau
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

#define G_LOG_DOMAIN "DiaXslt"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib/gstdio.h>

#include <libxml/tree.h>
#include <libxslt/transform.h>
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/xsltutils.h>

#include "filter.h"
#include "message.h"
#include "dia_dirs.h"
#include "dia-io.h"

#include "xslt.h"


/* Possible stylesheets */
GPtrArray *froms = NULL;

/* Selected stylesheets */
toxsl_t *xsl_to;
fromxsl_t *xsl_from;


static char *diafilename = NULL;
static char *filename = NULL;

static gboolean
export_xslt (DiagramData *data,
             DiaContext  *ctx,
             const gchar *f,
             const gchar *diaf,
             void        *user_data)
{
  g_clear_pointer (&filename, g_free);

  filename = g_strdup (f);
  g_clear_pointer (&diafilename, g_free);

  diafilename = g_strdup (diaf);

  xslt_dialog_create ();

  /* assume the dialog does all the error reporting */
  return TRUE;
}


void
xslt_ok (void)
{
  DiaContext *ctx = dia_context_new (_("XLST"));
  FILE *out = NULL;
  int err;
  char *stylefname = NULL;
  char *params[] = { "directory", NULL, NULL };
  xsltStylesheetPtr style = NULL, codestyle = NULL;
  xmlDocPtr doc = NULL, res = NULL;
  char *directory = g_path_get_dirname (filename);
  char *uri = g_filename_to_uri (directory, NULL, NULL);
  g_clear_pointer (&directory, g_free);

  dia_context_set_filename (ctx, diafilename);

  /* strange: requires an uri, but the last char is platform specifc?! */
  params[1] = g_strconcat ("'", uri, G_DIR_SEPARATOR_S, "'", NULL);
  g_clear_pointer (&uri, g_free);

  out = g_fopen (filename, "w+");
  if (out == NULL) {
    dia_context_add_message (ctx,
                             _("Can't open output file %s: %s"),
                             dia_message_filename (filename),
                             strerror (errno));
    goto out;
  }

  xmlSubstituteEntitiesDefault (0);
  doc = dia_io_load_document (diafilename, ctx, NULL);
  if (doc == NULL) {
    dia_context_add_message (ctx,
                             _("Error while parsing: %s"),
                             dia_message_filename (diafilename));

    goto out;
  }

  stylefname = xsl_from->xsl;

  style = xsltParseStylesheetFile ((const xmlChar *) stylefname);
  if (style == NULL) {
    dia_context_add_message (ctx,
                             _("Error while parsing stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  res = xsltApplyStylesheet (style, doc, NULL);
  if (res == NULL) {
    dia_context_add_message (ctx,
                             _("Error while applying stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  stylefname = xsl_to->xsl;

  codestyle = xsltParseStylesheetFile ((const xmlChar *) stylefname);
  if (codestyle == NULL) {
    dia_context_add_message (ctx,
                             _("Error while parsing stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  g_clear_pointer (&doc, xmlFreeDoc);

  doc = xsltApplyStylesheet (codestyle, res, (const char **) params);
  if (doc == NULL) {
    dia_context_add_message (ctx,
                             _("Error while applying stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  /* Returns the number of byte written or -1 in case of failure. */
  err = xsltSaveResultToFile (out, doc, codestyle);
  if(err < 0) {
    dia_context_add_message (ctx,
                             _("Error while saving result: %s"),
                             dia_message_filename (filename));

    goto out;
  }

  fprintf (out, "From:\t%s\n", diafilename);
  fprintf (out, "With:\t%s\n", stylefname);
  fprintf (out, "To:\t%s=%s\n", params[0], params[1]);

out:
  g_clear_pointer (&out, fclose);

  g_clear_pointer (&codestyle, xsltFreeStylesheet);
  g_clear_pointer (&style, xsltFreeStylesheet);
  g_clear_pointer (&res, xmlFreeDoc);
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);

  xsltCleanupGlobals ();

  xslt_clear ();
}


static toxsl_t *
read_implementations (xmlNodePtr cur, gchar *path)
{
  toxsl_t *first, *curto;
  first = curto = NULL;

  cur = cur->xmlChildrenNode;

  while (cur) {
    toxsl_t *to;
    if (xmlIsBlankNode(cur) || xmlNodeIsText(cur)) {
      cur = cur->next;
      continue;
    }
    to = g_new0 (toxsl_t, 1);
    to->next = NULL;
    to->name = (gchar *) xmlGetProp (cur, (const xmlChar *) "name");
    to->xsl = (gchar *) xmlGetProp (cur, (const xmlChar *) "stylesheet");

    if (!(to->name && to->xsl)) {
      g_warning ("Name and stylesheet attributes are required for the implementation element %s in XSLT plugin configuration file", cur->name);
      if (to->name) {
        xmlFree(to->name);
      }
      if (to->xsl) {
        xmlFree(to->xsl);
      }
      g_clear_pointer (&to, g_free);
      to = NULL;
    } else {
      if (first == NULL) {
        first = curto = to;
      } else {
        curto->next = to;
        curto = to;
      }
      /* make filename absolute */
      {
        gchar *fname = g_strconcat(path, G_DIR_SEPARATOR_S, to->xsl, NULL);
        xmlFree(to->xsl);
        to->xsl = fname;
      }
    }
    cur = cur->next;
  }

  return first;
}


static PluginInitResult
read_configuration (const char *config)
{
  DiaContext *ctx = dia_context_new (_("Initalise XSLT"));
  xmlDocPtr doc = NULL;
  xmlNodePtr cur;
  /* Primary xsl */
  char *path = NULL;
  PluginInitResult result = DIA_PLUGIN_INIT_ERROR;

  dia_context_set_filename (ctx, config);

  if (!g_file_test (config, G_FILE_TEST_EXISTS)) {
    goto out;
  }

  doc = dia_io_load_document (config, ctx, NULL);
  if (doc == NULL) {
    dia_context_add_message (ctx,
                             _("Couldn't parse XSLT plugin's configuration file %s"),
                             config);

    goto out;
  }

  cur = xmlDocGetRootElement(doc);
  if (cur == NULL) {
    dia_context_add_message (ctx,
                             _("XSLT plugin's configuration file %s is empty"),
                             config);

    goto out;
  }

  path = g_path_get_dirname(config);

  /* We don't care about the top level element's name */

  cur = cur->xmlChildrenNode;

  while (cur) {
    if (xmlIsBlankNode (cur) || xmlNodeIsText (cur)) {
      cur = cur->next;
      continue;
    } else if (!xmlStrcmp (cur->name, (const xmlChar *) "language")) {
      fromxsl_t *new_from = g_new0 (fromxsl_t, 1);

      new_from->name = (char *) xmlGetProp (cur, (const xmlChar *) "name");
      new_from->xsl = (char *) xmlGetProp (cur, (const xmlChar *) "stylesheet");

      if (!(new_from->name && new_from->xsl)) {
        dia_context_add_message (ctx,
                                 _("'name' and 'stylesheet' attributes are required for the language element %s in XSLT plugin configuration file"),
                                 cur->name);

        g_clear_pointer (&new_from, g_free);
      } else {
        /* make filename absolute */
        char *fname = g_strconcat (path, G_DIR_SEPARATOR_S, new_from->xsl, NULL);

        xmlFree (new_from->xsl);
        new_from->xsl = fname;

        new_from->xsls = read_implementations (cur, path);
        if (new_from->xsls == NULL) {
          g_warning ("No implementation stylesheet for language %s in XSLT plugin configuration file", new_from->name);
        }

        g_ptr_array_add (froms, new_from);
      }
    } else {
      g_warning ("Wrong node name %s in XSLT plugin configuration file, 'language' expected", cur->name);
    }
    cur = cur -> next;
  }

  if (froms->len == 0) {
    g_warning ("No stylesheets configured in %s for XSLT plugin", config);
  }

  result = DIA_PLUGIN_INIT_OK;

out:
  g_clear_pointer (&path, g_free);
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);

  xmlCleanupParser ();

  return result;
}


/* --- dia plug-in interface --- */

#define MY_RENDERER_NAME "XSL Transformation filter"

static const gchar *extensions[] = { "code", NULL };
static DiaExportFilter my_export_filter = {
  N_(MY_RENDERER_NAME),
  extensions,
  export_xslt
};

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init (PluginInfo *info)
{
  gchar *path;
  PluginInitResult global_res, user_res;

  froms = g_ptr_array_new_with_free_func (g_free);

  if (!dia_plugin_info_init (info,
                             "XSLT",
                             _("XSL Transformation filter"),
                             NULL, NULL)) {
    return DIA_PLUGIN_INIT_ERROR;
  }

  if (g_getenv ("DIA_XSLT_PATH") != NULL) {
    path = g_build_path (G_DIR_SEPARATOR_S, g_getenv ("DIA_XSLT_PATH"), "stylesheets.xml", NULL);
  } else {
    path = dia_get_data_directory ("xslt" G_DIR_SEPARATOR_S "stylesheets.xml");
  }

  global_res = read_configuration (path);
  g_clear_pointer (&path, g_free);

  path = dia_config_filename ("xslt" G_DIR_SEPARATOR_S "stylesheets.xml");
  user_res = read_configuration(path);
  g_clear_pointer (&path, g_free);

  if (global_res == DIA_PLUGIN_INIT_OK || user_res == DIA_PLUGIN_INIT_OK) {
    xsl_from = g_ptr_array_index (froms, 0);
    xsl_to = xsl_from->xsls;

    filter_register_export (&my_export_filter);
    return DIA_PLUGIN_INIT_OK;
  } else {
    message_error (_("No valid configuration files found for the XSLT plugin; not loading."));
    return DIA_PLUGIN_INIT_ERROR;
  }
}

gboolean
xslt_can_unload (PluginInfo *info)
{
  return TRUE;
}

void
xslt_unload(PluginInfo *info)
{
  g_ptr_array_unref (froms);
}
