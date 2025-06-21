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

#include "dia_dirs.h"
#include "dia_xml.h"
#include "dia-io.h"
#include "filter.h"
#include "message.h"
#include "plug-ins.h"

#include "dia-xslt-dialogue.h"

#include "dia-xslt.h"


struct _DiaXsltStylesheetTarget {
  char  *name;
  GFile *xsl;
};


DiaXsltStylesheetTarget *
dia_xslt_stylesheet_target_ref (DiaXsltStylesheetTarget *self)
{
  return g_rc_box_acquire (self);
}


static void
clear_stylesheet_target (gpointer data)
{
  DiaXsltStylesheetTarget *self = data;

  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->xsl);
}


void
dia_xslt_stylesheet_target_unref (DiaXsltStylesheetTarget *self)
{
  g_rc_box_release_full (self, clear_stylesheet_target);
}


G_DEFINE_BOXED_TYPE (DiaXsltStylesheetTarget,
                     dia_xslt_stylesheet_target,
                     dia_xslt_stylesheet_target_ref,
                     dia_xslt_stylesheet_target_unref)


const char *
dia_xslt_stylesheet_target_get_name (DiaXsltStylesheetTarget *self)
{
  return self->name;
}


struct _DiaXsltStylesheet {
  char      *name;
  GFile     *xsl;
  GPtrArray *targets;
};


DiaXsltStylesheet *
dia_xslt_stylesheet_ref (DiaXsltStylesheet *self)
{
  return g_rc_box_acquire (self);
}


static void
clear_stylesheet (gpointer data)
{
  DiaXsltStylesheet *self = data;

  g_clear_pointer (&self->name, g_free);
  g_clear_object (&self->xsl);
  g_clear_pointer (&self->targets, g_ptr_array_unref);
}


void
dia_xslt_stylesheet_unref (DiaXsltStylesheet *self)
{
  g_rc_box_release_full (self, clear_stylesheet);
}


G_DEFINE_BOXED_TYPE (DiaXsltStylesheet,
                     dia_xslt_stylesheet,
                     dia_xslt_stylesheet_ref,
                     dia_xslt_stylesheet_unref)


const char *
dia_xslt_stylesheet_get_name (DiaXsltStylesheet *self)
{
  return self->name;
}


void
dia_xslt_stylesheet_get_targets (DiaXsltStylesheet         *self,
                                 DiaXsltStylesheetTarget ***targets,
                                 size_t                    *n_targets)
{
  *targets = (DiaXsltStylesheetTarget **) self->targets->pdata;
  *n_targets = self->targets->len;
}


struct _DiaXsltPlugin {
  GObject parent;

  /* Possible stylesheets */
  GPtrArray *froms;
};


G_DEFINE_FINAL_TYPE (DiaXsltPlugin, dia_xslt_plugin, G_TYPE_OBJECT)


static void
dia_xslt_plugin_dispose (GObject *object)
{
  DiaXsltPlugin *self = DIA_XSLT_PLUGIN (object);

  g_clear_pointer (&self->froms, g_ptr_array_unref);

  G_OBJECT_CLASS (dia_xslt_plugin_parent_class)->dispose (object);
}


static void
dia_xslt_plugin_class_init (DiaXsltPluginClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = dia_xslt_plugin_dispose;
}


static void
dia_xslt_plugin_init (DiaXsltPlugin *self)
{
  self->froms =
    g_ptr_array_new_with_free_func ((GDestroyNotify) dia_xslt_stylesheet_unref);
}


static inline void
read_implementations (DiaXsltStylesheet *sheet,
                      GFile             *config_dir,
                      xmlNodePtr         cur,
                      DiaContext        *ctx)
{
  cur = cur->xmlChildrenNode;

  while (cur) {
    DiaXsltStylesheetTarget *to = NULL;
    xmlChar *str = NULL;

    if (xmlIsBlankNode (cur) || xmlNodeIsText (cur)) {
      cur = cur->next;
      continue;
    }

    to = g_rc_box_new0 (DiaXsltStylesheetTarget);

    str = xmlGetProp (cur, (const xmlChar *) "name");
    if (!str) {
      dia_context_add_message (ctx,
                               _("Name and stylesheet attributes are required for the implementation element %s in XSLT plugin configuration file"),
                               cur->name);

      goto next;
    }
    g_set_str (&to->name, (char *) str);
    dia_clear_xml_string (&str);

    str = xmlGetProp (cur, (const xmlChar *) "stylesheet");
    if (!str) {
      dia_context_add_message (ctx,
                               _("Name and stylesheet attributes are required for the implementation element %s in XSLT plugin configuration file"),
                               cur->name);

      goto next;
    }
    to->xsl = g_file_get_child (config_dir, (char *) str);
    dia_clear_xml_string (&str);

    g_ptr_array_add (sheet->targets, dia_xslt_stylesheet_target_ref (to));

next:
    g_clear_pointer (&to, dia_xslt_stylesheet_target_unref);

    cur = cur->next;
  }
}


static PluginInitResult
read_configuration (DiaXsltPlugin *self, const char *config, DiaContext *ctx)
{
  GFile *config_file = g_file_new_for_path (config);
  GFile *config_dir = g_file_get_parent (config_file);
  xmlDocPtr doc = NULL;
  PluginInitResult result = DIA_PLUGIN_INIT_ERROR;
  xmlNodePtr cur;
  /* Primary xsl */

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

  /* We don't care about the top level element's name */

  cur = cur->xmlChildrenNode;

  while (cur) {
    if (xmlIsBlankNode (cur) || xmlNodeIsText (cur)) {
      cur = cur->next;
      continue;
    } else if (!xmlStrcmp (cur->name, (const xmlChar *) "language")) {
      DiaXsltStylesheet *sheet = g_rc_box_new0 (DiaXsltStylesheet);
      xmlChar *str = NULL;

      str = xmlGetProp (cur, (const xmlChar *) "name");
      if (!str) {
        dia_context_add_message (ctx,
                                 _("'name' and 'stylesheet' attributes are required for the language element %s in XSLT plugin configuration file"),
                                 cur->name);

        goto next;
      }
      g_set_str (&sheet->name, (char *) str);
      dia_clear_xml_string (&str);

      str = xmlGetProp (cur, (const xmlChar *) "stylesheet");
      if (!str) {
        dia_context_add_message (ctx,
                                 _("'name' and 'stylesheet' attributes are required for the language element %s in XSLT plugin configuration file"),
                                 cur->name);

        goto next;
      }
      sheet->xsl = g_file_get_child (config_dir, (char *) str);
      dia_clear_xml_string (&str);

      sheet->targets =
        g_ptr_array_new_null_terminated (10,
                                         (GDestroyNotify) dia_xslt_stylesheet_target_unref,
                                        TRUE);

      read_implementations (sheet, config_dir, cur, ctx);

      if (sheet->targets->len < 1) {
        dia_context_add_message (ctx,
                                 _("No implementation stylesheet for language %s in XSLT plugin configuration file"),
                                 sheet->name);

        goto next;
      }

      g_ptr_array_add (self->froms, dia_xslt_stylesheet_ref (sheet));

next:
      g_clear_pointer (&sheet, dia_xslt_stylesheet_unref);
    } else {
      dia_context_add_message (ctx,
                               _("Wrong node name %s in XSLT plugin configuration file, 'language' expected"),
                               cur->name);
    }

    cur = cur -> next;
  }

  if (self->froms->len == 0) {
    g_warning ("No stylesheets configured in %s for XSLT plugin", config);
  }

  result = DIA_PLUGIN_INIT_OK;

out:
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_object (&config_dir);
  g_clear_object (&config_file);

  xmlCleanupParser ();

  return result;
}


static gboolean
dia_xslt_plugin_load_stylesheets (DiaXsltPlugin *self, DiaContext *ctx)
{
  PluginInitResult global_res, user_res;
  char *path;

  if (g_getenv ("DIA_XSLT_PATH") != NULL) {
    path = g_build_filename (g_getenv ("DIA_XSLT_PATH"), "stylesheets.xml", NULL);
  } else {
    path = dia_get_data_directory ("xslt" G_DIR_SEPARATOR_S "stylesheets.xml");
  }

  global_res = read_configuration (self, path, ctx);
  g_clear_pointer (&path, g_free);

  path = dia_config_filename ("xslt" G_DIR_SEPARATOR_S "stylesheets.xml");
  user_res = read_configuration (self, path, ctx);
  g_clear_pointer (&path, g_free);

  if (global_res != DIA_PLUGIN_INIT_OK && user_res != DIA_PLUGIN_INIT_OK) {
    dia_context_add_message (ctx,
                             _("No valid configuration files found for the XSLT plugin; not loading."));

    return FALSE;
  }

  return TRUE;
}


typedef struct _ExportData ExportData;
struct _ExportData {
  DiaContext        *ctx;
  GFile             *source_file;
  GFile             *target_file;
  GFileOutputStream *target_stream;
};


static void
export_data_free (gpointer data)
{
  ExportData *self = data;

  g_clear_pointer (&self->ctx, dia_context_release);
  g_clear_object (&self->source_file);
  g_clear_object (&self->target_file);
  g_clear_object (&self->target_stream);

  g_free (self);
}


static int
write_stream (gpointer user_data, const char *buffer, int length)
{
  ExportData *data = user_data;
  GError *error = NULL;
  int written = g_output_stream_write (G_OUTPUT_STREAM (data->target_stream),
                                       buffer,
                                       length,
                                       NULL,
                                       &error);

  if (error) {
    char *uri = g_file_get_uri (data->target_file);

    dia_context_add_message (data->ctx,
                             _("Error while saving %s: %s"),
                             uri,
                             error->message);

    g_clear_pointer (&uri, g_free);
    g_clear_error (&error);

    return -1;
  }

  return written;
}


static int
dummy_close (gpointer user_data)
{
  return 0;
}


static void
apply_transformation (DiaXsltDialogue         *dialog,
                      DiaXsltStylesheet       *stylesheet,
                      DiaXsltStylesheetTarget *target,
                      gpointer                 user_data)
{
  ExportData *data = user_data;
  GFile *directory = g_file_get_parent (data->target_file);
  char *dir_uri = g_file_get_uri (directory);
  char *uri = g_file_get_uri (data->target_file);
  GError *error = NULL;
  GString *content = g_string_new (NULL);
  const char *stylefname = NULL;
  char *params[] = { "directory", NULL, NULL };
  xsltStylesheetPtr style = NULL, codestyle = NULL;
  xmlDocPtr doc = NULL, res = NULL;
  xmlOutputBufferPtr out_buf = NULL;

  /* strange: requires a uri, but the last char is platform specifc?! */
  params[1] = g_strconcat ("'", dir_uri, G_DIR_SEPARATOR_S, "'", NULL);

  data->target_stream = g_file_replace (data->target_file,
                                        NULL,
                                        TRUE,
                                        G_FILE_CREATE_PRIVATE | G_FILE_CREATE_REPLACE_DESTINATION,
                                        NULL,
                                        &error);
  if (error) {
    dia_context_add_message (data->ctx,
                             _("Can't open output file %s: %s"),
                             uri,
                             error->message);

    goto out;
  }

  xmlSubstituteEntitiesDefault (0);
  doc = dia_io_load_document (g_file_peek_path (data->source_file),
                              data->ctx,
                              NULL);
  if (doc == NULL) {
    char *source_uri = g_file_get_uri (data->source_file);

    dia_context_add_message (data->ctx,
                             _("Error while parsing: %s"),
                             source_uri);

    g_clear_pointer (&source_uri, g_free);

    goto out;
  }

  stylefname = g_file_peek_path (stylesheet->xsl);

  style = xsltParseStylesheetFile ((const xmlChar *) stylefname);
  if (style == NULL) {
    dia_context_add_message (data->ctx,
                             _("Error while parsing stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  res = xsltApplyStylesheet (style, doc, NULL);
  if (res == NULL) {
    dia_context_add_message (data->ctx,
                             _("Error while applying stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  stylefname = g_file_peek_path (target->xsl);

  codestyle = xsltParseStylesheetFile ((const xmlChar *) stylefname);
  if (codestyle == NULL) {
    dia_context_add_message (data->ctx,
                             _("Error while parsing stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  g_clear_pointer (&doc, xmlFreeDoc);

  doc = xsltApplyStylesheet (codestyle, res, (const char **) params);
  if (doc == NULL) {
    dia_context_add_message (data->ctx,
                             _("Error while applying stylesheet: %s"),
                             dia_message_filename (stylefname));

    goto out;
  }

  /* Returns the number of byte written or -1 in case of failure. */
  out_buf = xmlOutputBufferCreateIO (write_stream, dummy_close, data, NULL);

  if (xsltSaveResultTo (out_buf, doc, codestyle) < 0) {
    dia_context_add_message (data->ctx,
                             _("Error while saving result: %s"),
                             uri);

    goto out;
  }

  if (xmlOutputBufferFlush (out_buf) < 0) {
    dia_context_add_message (data->ctx,
                             _("Error while saving result: %s"),
                             uri);

    goto out;
  }

  g_string_append_printf (content, "From:\t%s\n", g_file_peek_path (data->source_file));
  g_string_append_printf (content, "With:\t%s\n", stylefname);
  g_string_append_printf (content, "To:\t%s=%s\n", params[0], params[1]);

  g_output_stream_write_all (G_OUTPUT_STREAM (data->target_stream),
                             content->str,
                             content->len,
                             NULL,
                             NULL,
                             &error);
  if (error) {
    dia_context_add_message (data->ctx,
                             _("Error while saving %s: %s"),
                             uri,
                             error->message);

    goto out;
  }

  g_output_stream_close (G_OUTPUT_STREAM (data->target_stream), NULL, &error);
  if (error) {
    dia_context_add_message (data->ctx,
                             _("Error while saving %s: %s"),
                             uri,
                             error->message);

    goto out;
  }

out:
  g_clear_object (&directory);
  g_clear_pointer (&dir_uri, g_free);
  g_clear_pointer (&uri, g_free);
  g_clear_error (&error);
  if (content) {
    g_string_free (content, TRUE);
  }
  g_clear_pointer (&params[1], g_free);
  g_clear_pointer (&style, xsltFreeStylesheet);
  g_clear_pointer (&codestyle, xsltFreeStylesheet);
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&res, xmlFreeDoc);
  g_clear_pointer (&out_buf, xmlOutputBufferClose);

  xsltCleanupGlobals ();
}


static void
dia_xslt_plugin_export (DiaXsltPlugin *self,
                        GFile         *source_file,
                        GFile         *target_file,
                        DiaContext    *ctx)
{
  GtkWidget *dialog = g_object_new (DIA_XSLT_TYPE_DIALOGUE,
                                    "stylesheets", self->froms,
                                    NULL);
  ExportData *data = g_new0 (ExportData, 1);

  g_set_object (&data->ctx, ctx);
  g_set_object (&data->source_file, source_file);
  g_set_object (&data->target_file, target_file);

  g_signal_connect_data (dialog,
                         "apply-transformation", G_CALLBACK (apply_transformation),
                         data,
                         (GClosureNotify) export_data_free,
                         G_CONNECT_DEFAULT);

  /* getting a bit messy with the context here, fix this when we get async
   * export operations */
  gtk_window_present (GTK_WINDOW (dialog));
}


static DiaXsltPlugin *plugin = NULL;


static gboolean
export_xslt (DiagramData *data,
             DiaContext  *ctx,
             const char  *f,
             const char  *diaf,
             void        *user_data)
{
  GFile *source_file = g_file_new_for_path (diaf);
  GFile *target_file = g_file_new_for_path (f);

  dia_xslt_plugin_export (plugin, source_file, target_file, ctx);

  g_clear_object (&source_file);
  g_clear_object (&target_file);

  return TRUE;
}


/* --- dia plug-in interface --- */

#define MY_RENDERER_NAME "XSL Transformation filter"

static const char *extensions[] = { "code", NULL };
static DiaExportFilter my_export_filter = {
  N_(MY_RENDERER_NAME),
  extensions,
  export_xslt
};

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init (PluginInfo *info)
{
  DiaContext *ctx = dia_context_new (_("XSLT Plugin"));
  PluginInitResult res = DIA_PLUGIN_INIT_ERROR;

  plugin = g_object_new (DIA_XSLT_TYPE_PLUGIN, NULL);

  if (!dia_plugin_info_init (info,
                             "XSLT",
                             _("XSL Transformation filter"),
                             NULL, NULL)) {
    goto out;
  }

  if (!dia_xslt_plugin_load_stylesheets (plugin, ctx)) {
    g_clear_object (&plugin);

    goto out;
  }

  filter_register_export (&my_export_filter);

  res = DIA_PLUGIN_INIT_OK;

out:
  g_clear_pointer (&ctx, dia_context_release);

  return res;
}
