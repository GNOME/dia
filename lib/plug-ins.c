/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * plug-ins.c: plugin loading handling interfaces for dia
 * Copyright (C) 2000 James Henstridge
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

#define G_LOG_DOMAIN "Dia"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib/gstdio.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "dia_xml.h"
#include "message.h" /* only for dia_log_message() */
#include "dia-io.h"

#include "plug-ins.h"
#include "dia_dirs.h"

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <pango/pangowin32.h>
#endif

struct _PluginInfo {
  GModule *module;
  gchar *filename;      /* plugin filename */

  gboolean is_loaded;
  gboolean inhibit_load;

  gchar *name;
  gchar *description;

  PluginInitFunc init_func;
  PluginCanUnloadFunc can_unload_func;
  PluginUnloadFunc unload_func;
};


static GList *plugins = NULL;

static void     ensure_pluginrc(void);
static void     free_pluginrc(void);
static gboolean plugin_load_inhibited(const gchar *filename);
static void     info_fill_from_pluginrc(PluginInfo *info);

gboolean
dia_plugin_info_init(PluginInfo *info,
		     const gchar *name,
		     const gchar *description,
		     PluginCanUnloadFunc can_unload_func,
		     PluginUnloadFunc unload_func)
{
  g_clear_pointer (&info->name, g_free);
  info->name = g_strdup(name);
  g_clear_pointer (&info->description, g_free);
  info->description = g_strdup(description);
  info->can_unload_func = can_unload_func;
  info->unload_func = unload_func;

  return TRUE;
}

gchar *
dia_plugin_check_version(gint version)
{
  if (version != DIA_PLUGIN_API_VERSION)
    return "Wrong plugin API version";
  else
    return NULL;
}


/* functions for use by dia ... */

const gchar *
dia_plugin_get_filename(PluginInfo *info)
{
  return info->filename;
}

const gchar *
dia_plugin_get_name(PluginInfo *info)
{
  return info->name ? info->name : _("???");
}

const gchar *
dia_plugin_get_description(PluginInfo *info)
{
  return info->description;
}

const gpointer *
dia_plugin_get_symbol(PluginInfo *info, const gchar* name)
{
  gpointer symbol;

  if (!info->module)
    return NULL;

  if (g_module_symbol (info->module, name, &symbol))
    return symbol;

  return NULL;
}

gboolean
dia_plugin_can_unload(PluginInfo *info)
{
  return (info->can_unload_func != NULL) && (* info->can_unload_func)(info);
}

gboolean
dia_plugin_is_loaded(PluginInfo *info)
{
  return info->is_loaded;
}

gboolean
dia_plugin_get_inhibit_load(PluginInfo *info)
{
  return info->inhibit_load;
}

void
dia_plugin_set_inhibit_load(PluginInfo *info, gboolean inhibit_load)
{
  info->inhibit_load = inhibit_load;
}

void
dia_plugin_load(PluginInfo *info)
{
#ifdef G_OS_WIN32
  guint error_mode;
#endif
  g_return_if_fail(info != NULL);
  g_return_if_fail(info->filename != NULL);

  if (info->is_loaded)
    return;

#ifdef G_OS_WIN32
  /* suppress the systems error dialog, we can handle a plug-in not loadable */
  error_mode = SetErrorMode (SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
#endif

  dia_log_message ("plug-in '%s'", info->filename);
  info->module = g_module_open(info->filename, G_MODULE_BIND_LAZY);
#ifdef G_OS_WIN32
    SetErrorMode (error_mode);
#endif
  if (!info->module) {
    gchar *msg_utf8 = NULL;
    /* at least on windows the systems error message is not that useful */
    if (!g_file_test(info->filename, G_FILE_TEST_EXISTS))
      msg_utf8 = g_locale_to_utf8 (g_module_error(), -1, NULL, NULL, NULL);
    else
      msg_utf8 = g_strdup_printf (_("Missing dependencies for '%s'?"), info->filename);
    /* this is eating the conversion */
    info->description = msg_utf8;
    return;
  }

  info->init_func = NULL;
  if (!g_module_symbol(info->module, "dia_plugin_init",
		       (gpointer)&info->init_func)) {
    g_module_close(info->module);
    info->module = NULL;
    info->description = g_strdup(_("Missing symbol 'dia_plugin_init'"));
    return;
  }

  if ((* info->init_func)(info) != DIA_PLUGIN_INIT_OK) {
    /* plugin displayed an error message */
    g_module_close(info->module);
    info->module = NULL;
    info->description = g_strdup(_("dia_plugin_init() call failed"));
    return;
  }

  /* Corrupt? */
  if (info->description == NULL) {
    g_module_close(info->module);
    info->module = NULL;
    info->description = g_strdup(_("dia_plugin_init() call failed"));
    return;
  }

  info->is_loaded = TRUE;

  return;
}

void
dia_plugin_unload(PluginInfo *info)
{
  g_return_if_fail(info != NULL);
  g_return_if_fail(info->filename != NULL);

  if (!info->is_loaded)
    return;

  if (!dia_plugin_can_unload(info)) {
    /* calling this function w/o check first is a programmer's error */
    g_warning ("%s plugin could not be unloaded", info->name);
    return;
  }
  /* perform plugin cleanup */
  if (info->unload_func)
    (* info->unload_func)(info);
  g_module_close(info->module);
  info->module = NULL;
  info->init_func = NULL;
  info->can_unload_func = NULL;
  info->unload_func = NULL;

  info->is_loaded = FALSE;
}

void
dia_register_plugin(const gchar *filename)
{
  GList *tmp;
  PluginInfo *info;

  g_debug ("%s: Loading %s", G_STRLOC, filename);

  /* check if plugin has already been registered */
  for (tmp = plugins; tmp != NULL; tmp = tmp->next) {
    info = tmp->data;
    if (!strcmp(info->filename, filename))
      return;
  }

  /* If trying to load libdia, abort */
  if (strstr(filename, "libdia.")) {
    return;
  }

  /* set up plugin info structure */
  info = g_new0(PluginInfo, 1);
  info->filename = g_strdup(filename);
  info->is_loaded = FALSE;
  info->inhibit_load = FALSE;

  /* check whether loading of the plugin has been inhibited */
  if (plugin_load_inhibited(filename))
    info_fill_from_pluginrc(info);
  else
    dia_plugin_load(info);

  plugins = g_list_prepend(plugins, info);
}

typedef void (*ForEachInDirDoFunc)(const gchar *name);
typedef gboolean (*ForEachInDirFilterFunc)(const gchar *name);

static void
for_each_in_dir (const char             *directory,
                 ForEachInDirDoFunc      dofunc,
                 ForEachInDirFilterFunc  filter)
{
  const char *dentry;
  GDir *dp;
  GError *error = NULL;

  if (!g_file_test (directory, G_FILE_TEST_EXISTS)) {
    return;
  }

  dp = g_dir_open (directory, 0, &error);
  if (dp == NULL) {
    g_warning ("Could not open “%s”\n“%s”", directory, error->message);
    g_clear_error (&error);
    return;
  }

  while ((dentry = g_dir_read_name (dp)) != NULL) {
    char *name = g_build_filename (directory, dentry, NULL);

    if (filter (name)) {
      dofunc (name);
    }
    g_clear_pointer (&name, g_free);
  }
  g_dir_close (dp);
}


static gboolean
directory_filter (const char *name)
{
  return g_file_test (name, G_FILE_TEST_IS_DIR);
}


static gboolean
dia_plugin_filter (const char *name)
{
  return g_str_has_suffix (name, G_MODULE_SUFFIX)
              && g_file_test (name, G_FILE_TEST_IS_REGULAR);
}


void
dia_register_plugins_in_dir (const char *directory)
{
  g_debug ("%s: Registering plugins in %s", G_STRLOC, directory);

  for_each_in_dir (directory, dia_register_plugin, dia_plugin_filter);
  for_each_in_dir (directory, dia_register_plugins_in_dir, directory_filter);
}


void
dia_register_plugins(void)
{
  const char *library_path;
  char *lib_dir;

  library_path = g_getenv ("DIA_LIB_PATH");

  lib_dir = dia_config_filename ("objects");

  if (lib_dir != NULL) {
    dia_register_plugins_in_dir (lib_dir);
    g_clear_pointer (&lib_dir, g_free);
  }

  if (library_path != NULL) {
    gchar **paths = g_strsplit(library_path, G_SEARCHPATH_SEPARATOR_S, 0);
    gint i;

    for (i = 0; paths[i] != NULL; i++) {
      dia_register_plugins_in_dir(paths[i]);
    }
    g_strfreev(paths);
  } else {
    lib_dir = dia_get_lib_directory ();

    dia_register_plugins_in_dir (lib_dir);
    g_clear_pointer (&lib_dir, g_free);
  }
  /* FIXME: this isn't needed anymore */
  free_pluginrc ();
}

void
dia_register_builtin_plugin(PluginInitFunc init_func)
{
  PluginInfo *info;

  info = g_new0(PluginInfo, 1);
  info->filename = "<builtin>";
  info->is_loaded = TRUE;
  info->inhibit_load = FALSE;

  info->init_func = init_func;

  if ((* init_func)(info) != DIA_PLUGIN_INIT_OK) {
    g_clear_pointer (&info, g_free);
    return;
  }
  plugins = g_list_prepend(plugins, info);
}


GList *
dia_list_plugins(void)
{
  return plugins;
}

static xmlDocPtr pluginrc = NULL;
/* format is:
  <plugins>
    <plugin filename="filename">
      <name></name>
      <description></description>
      <inhibit-load/>
    </plugin>
  </plugins>
*/

static void
ensure_pluginrc(void)
{
  gchar *filename;
  DiaContext *ctx = NULL;

  if (pluginrc)
    return;
  ctx = dia_context_new (_("Plugin Configuration"));
  filename = dia_config_filename("pluginrc");
  dia_context_set_filename (ctx, filename);
  if (g_file_test (filename,  G_FILE_TEST_IS_REGULAR)) {
    pluginrc = dia_io_load_document (filename, ctx, NULL);
  } else {
    pluginrc = NULL;
  }

  g_clear_pointer (&filename, g_free);

  if (!pluginrc) {
    pluginrc = xmlNewDoc((const xmlChar *)"1.0");
    pluginrc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
    xmlDocSetRootElement(pluginrc,
			 xmlNewDocNode(pluginrc, NULL, (const xmlChar *)"plugins", NULL));
  }
  dia_context_release (ctx);
}

static void
free_pluginrc(void)
{
  if (pluginrc) {
    xmlFreeDoc(pluginrc);
    pluginrc = NULL;
  }
}

/* whether we should prevent loading on startup */
static gboolean
plugin_load_inhibited(const gchar *filename)
{
  xmlNodePtr node;

  ensure_pluginrc();
  for (node = pluginrc->xmlRootNode->xmlChildrenNode;
       node != NULL;
       node = node->next) {
    xmlChar *node_filename;

    if (xmlIsBlankNode(node)) continue;

    if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"plugin") != 0)
      continue;
    node_filename = xmlGetProp(node, (const xmlChar *)"filename");
    if (node_filename && !strcmp(filename, (gchar *)node_filename)) {
      xmlNodePtr node2;

      xmlFree(node_filename);
      for (node2 = node->xmlChildrenNode;
           node2 != NULL;
           node2 = node2->next) {
        if (xmlIsBlankNode(node2)) continue;
	if (node2->type == XML_ELEMENT_NODE &&
	    !xmlStrcmp(node2->name, (const xmlChar *)"inhibit-load"))
	  return TRUE;
      }
      return FALSE;
    }
    if (node_filename) xmlFree(node_filename);
  }
  return FALSE;
}

static void
info_fill_from_pluginrc(PluginInfo *info)
{
  xmlNodePtr node;

  info->module = NULL;
  info->name = NULL;
  info->description = NULL;
  info->is_loaded = FALSE;
  info->inhibit_load = TRUE;
  info->init_func = NULL;
  info->can_unload_func = NULL;
  info->unload_func = NULL;

  ensure_pluginrc();
  for (node = pluginrc->xmlRootNode->xmlChildrenNode;
       node != NULL;
       node = node->next) {
    xmlChar *node_filename;

    if (xmlIsBlankNode(node)) continue;

    if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"plugin") != 0)
      continue;
    node_filename = xmlGetProp(node, (const xmlChar *)"filename");
    if (node_filename && !strcmp(info->filename, (char *) node_filename)) {
      xmlNodePtr node2;

      xmlFree(node_filename);
      for (node2 = node->xmlChildrenNode;
           node2 != NULL;
           node2 = node2->next) {
	gchar *content;

        if (xmlIsBlankNode(node2)) continue;

	if (node2->type != XML_ELEMENT_NODE)
	  continue;
	content = (gchar *)xmlNodeGetContent(node2);
	if (!xmlStrcmp(node2->name, (const xmlChar *)"name")) {
	  g_clear_pointer (&info->name, g_free);
	  info->name = g_strdup(content);
	} else if (!xmlStrcmp(node2->name, (const xmlChar *)"description")) {
	  g_clear_pointer (&info->description, g_free);
	  info->description = g_strdup(content);
	}
	xmlFree(content);
      }
      break;
    }
    if (node_filename) xmlFree(node_filename);
  }
}


void
dia_pluginrc_write (void)
{
  char *filename;
  GList *tmp;

  ensure_pluginrc();
  for (tmp = plugins; tmp != NULL; tmp = tmp->next) {
    PluginInfo *info = tmp->data;
    xmlNodePtr node, pluginnode;

    if (info == NULL) {
      continue;
    }

    pluginnode = xmlNewNode(NULL, (const xmlChar *)"plugin");
    (void)xmlNewChild(pluginnode, NULL, (const xmlChar *)"name", (xmlChar *)info->name);
    /* FIXME: UNICODE_WORK_IN_PROGRESS why is this reencoding necessary ?*/
 {
     xmlChar *enc = xmlEncodeEntitiesReentrant(pluginnode->doc,
                                               (xmlChar *)info->description);
     (void)xmlNewChild(pluginnode, NULL, (const xmlChar *)"description", enc);
     xmlFree(enc);
 }
    if (info->inhibit_load)
      (void)xmlNewChild(pluginnode, NULL, (const xmlChar *)"inhibit-load", NULL);

    for (node = pluginrc->xmlRootNode->xmlChildrenNode;
         node != NULL;
         node = node->next) {
      xmlChar *node_filename;

      if (xmlIsBlankNode(node)) continue;

      if (node->type != XML_ELEMENT_NODE || xmlStrcmp(node->name, (const xmlChar *)"plugin") != 0)
	continue;
      node_filename = xmlGetProp(node, (const xmlChar *)"filename");
      if (node_filename && !strcmp(info->filename, (char *) node_filename)) {
	xmlFree(node_filename);
	xmlReplaceNode(node, pluginnode);
	xmlFreeNode(node);
	break;
      }
      if (node_filename) xmlFree(node_filename);
    }
    /* node wasn't in document ... */
    if (!node)
      xmlAddChild(pluginrc->xmlRootNode, pluginnode);
    /* have to call this after adding node to document */
    xmlSetProp(pluginnode, (const xmlChar *)"filename", (xmlChar *)info->filename);
  }

  {
    DiaContext *ctx = dia_context_new (_("Plugin Configuration"));
    filename = dia_config_filename ("pluginrc");
    dia_context_set_filename (ctx, filename);

    dia_io_save_document (filename, pluginrc, FALSE, ctx);

    g_clear_pointer (&ctx, dia_context_release);
  }

  g_clear_pointer (&filename, g_free);
  free_pluginrc();
}


