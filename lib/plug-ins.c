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

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_DIRENT_H
#include <dirent.h>
#endif

#include <xmlmemory.h>
#include <parser.h>
#include <tree.h>
#include "dia_xml_libxml.h"
#include "dia_xml.h"

#include "charconv.h"
#include "plug-ins.h"
#include "intl.h"
#include "message.h"
#include "dia_dirs.h"

#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
#define XML2
#endif

#if defined(G_OS_WIN32) || defined(__EMX__)
#  define PLUG_IN_EXT ".dll"
#  define PLUG_IN_EXT_LEN 4
#  define USING_LIBTOOL 0
#else
/* this one should work on any platform where libtool is used to compile dia */
#  define PLUG_IN_EXT ".la"
#  define PLUG_IN_EXT_LEN 3
#  define USING_LIBTOOL 1
#endif

struct _PluginInfo {
  GModule *module;
  gchar *filename;      /* plugin filename */
  gchar *real_filename; /* not a .la file */

  gboolean is_loaded;
  gboolean inhibit_load;

  gchar *name;
  utfchar *description;

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
dia_plugin_info_init(PluginInfo *info, gchar *name, utfchar *description,
		     PluginCanUnloadFunc can_unload_func, 
		     PluginUnloadFunc unload_func)
{
	utfchar *utf;

	g_free(info->name);
	info->name = g_strdup(name);
	g_free(info->description);
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
	utf = charconv_local8_to_utf8 (description);
	info->description = utf;
#else
	info->description = g_strdup(description);
#endif
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


/* functiosn for use by dia ... */

const gchar *
dia_plugin_get_filename(PluginInfo *info)
{
  return info->filename;
}

const gchar *
dia_plugin_get_name(PluginInfo *info)
{
  return info->name;
}

const gchar *
dia_plugin_get_description(PluginInfo *info)
{
  return info->description;
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

/* implementation stollen from BEAST/BSE */
static gchar *
find_real_filename(const gchar *filename)
{
  const gint TOKEN_DLNAME = G_TOKEN_LAST + 1;
  GScanner *scanner;
  gint len, fd;
  gchar *str, *ret;

  g_return_val_if_fail(filename != NULL, NULL);

  len = strlen(filename);

  /* is this a libtool .la file? */
  if (len < 3 || strcmp(&filename[len-3], ".la") != 0)
    return g_strdup(filename);
  
  fd = open(filename, O_RDONLY, 0);
  if (fd < 0)
    return NULL;

  scanner = g_scanner_new(NULL);
  g_scanner_input_file(scanner, fd);
  scanner->config->symbol_2_token = TRUE;
  g_scanner_add_symbol(scanner, "dlname", GUINT_TO_POINTER(TOKEN_DLNAME));

  /* skip ahead to dlname part */
  while (!g_scanner_eof(scanner) &&
	 g_scanner_peek_next_token(scanner) != TOKEN_DLNAME)
    g_scanner_get_next_token(scanner);
  /* parse dlname */
  if (g_scanner_get_next_token (scanner) != TOKEN_DLNAME ||
      g_scanner_get_next_token (scanner) != '=' ||
      g_scanner_get_next_token (scanner) != G_TOKEN_STRING) {
    g_scanner_destroy(scanner);
    close(fd);
    return NULL;
  }
  
  str = g_dirname(filename);
  ret = g_strconcat(str, G_DIR_SEPARATOR_S, scanner->value.v_string,
		    NULL);
  g_free(str);
  g_scanner_destroy(scanner);
  close(fd);

  return ret;
}

void
dia_plugin_load(PluginInfo *info)
{
  g_return_if_fail(info != NULL);
  g_return_if_fail(info->filename != NULL);

  if (info->is_loaded)
    return;

  g_free(info->real_filename);
  info->real_filename = find_real_filename(info->filename);
  if (info->real_filename == NULL) {
    message_error(_("Could not deduce correct path for `%s'"), info->filename);
    return;
  }
  info->module = g_module_open(info->real_filename, G_MODULE_BIND_LAZY);
  if (!info->module) {
    message_error(_("Could not load plugin `%s'\n%s"), info->filename,
		  g_module_error());
    return;
  }

  info->init_func = NULL;
  if (!g_module_symbol(info->module, "dia_plugin_init",
		       (gpointer)&info->init_func)) {
    g_module_close(info->module);
    info->module = NULL;

    message_error(_("Could not find plugin init function in `%s'"),
		  info->filename);
    return;
  }

  if ((* info->init_func)(info) != DIA_PLUGIN_INIT_OK) {
    /* plugin displayed an error message */
    g_module_close(info->module);
    info->module = NULL;
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
    message(_("%s Plugin could not be unloaded"), info->name);
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

  /* check if plugin has already been registered */
  for (tmp = plugins; tmp != NULL; tmp = tmp->next) {
    info = tmp->data;
    if (!strcmp(info->filename, filename))
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

static gboolean
this_is_a_plugin(const gchar *name) 
{
#if USING_LIBTOOL
  gchar *soname,*basename;
  struct stat statbuf;  
#endif
  guint len = strlen(name);
  if (0 != strcmp(&name[len-PLUG_IN_EXT_LEN], PLUG_IN_EXT)) 
    return FALSE;
#if USING_LIBTOOL
  basename = g_strndup(name,len-PLUG_IN_EXT_LEN);
  soname = g_strconcat(basename,".so",NULL);
  if ((stat(soname, &statbuf) < 0)||(!S_ISREG(statbuf.st_mode))) {    
    g_free(basename);
    g_free(soname);
    return FALSE;
  }
  g_free(basename);
  g_free(soname);
#endif
  return TRUE;
}

typedef void (*ForEachInDirDoFunc)(const gchar *name);
typedef gboolean (*ForEachInDirFilterFunc)(const gchar *name);

static void 
for_each_in_dir(const gchar *directory, ForEachInDirDoFunc dofunc,
                ForEachInDirFilterFunc filter)
{
  struct stat statbuf;
  struct dirent *dirp;
  DIR *dp;

  if (stat(directory, &statbuf) < 0)
    return;
  if (!S_ISDIR(statbuf.st_mode)) {
    message_warning(_("`%s' is not a directory"), directory);
    return;
  }

  dp = opendir(directory);
  if (dp == NULL) {
    message_warning(_("Could not open `%s'"), directory);
    return;
  }

  while ((dirp = readdir(dp)) != NULL) {
    gchar *name = g_strconcat(directory,G_DIR_SEPARATOR_S,dirp->d_name,NULL);

    if (filter(name)) dofunc(name);
    g_free(name);
  }
  closedir(dp);
}

static gboolean 
directory_filter(const gchar *name)
{
  guint len = strlen(name);
  struct stat statbuf;

  if (0 == strcmp(G_DIR_SEPARATOR_S ".",
                  &name[len-strlen(G_DIR_SEPARATOR_S ".")])) return FALSE;
  if (0 == strcmp(G_DIR_SEPARATOR_S "..",
                  &name[len-strlen(G_DIR_SEPARATOR_S "..")])) return FALSE;

  if (stat(name,&statbuf) < 0) return FALSE;
  if (!S_ISDIR(statbuf.st_mode)) return FALSE;

  return TRUE;
}

static gboolean 
dia_plugin_filter(const gchar *name) 
{
  gint len = strlen(name);
  struct stat statbuf;
  if (stat(name,&statbuf) < 0) return FALSE;
  if (!S_ISREG(statbuf.st_mode) || S_ISDIR(statbuf.st_mode)) return FALSE;
  
  if (len <= PLUG_IN_EXT_LEN) return FALSE;

  return this_is_a_plugin(name);
}

static void
walk_dirs_for_plugins(const gchar *dirname)
{
  for_each_in_dir(dirname,walk_dirs_for_plugins,directory_filter);  
  for_each_in_dir(dirname,dia_register_plugin,dia_plugin_filter);
}

#define RECURSE (G_DIR_SEPARATOR_S G_DIR_SEPARATOR_S)

void
dia_register_plugins_in_dir(const gchar *directory)
{
  guint reclen = strlen(RECURSE);
  guint len = strlen(directory);

  if ((len >= reclen) &&
      (0 == strcmp(&directory[len-reclen],RECURSE))) {
    gchar *dirbase = g_strndup(directory,len-reclen);
    for_each_in_dir(dirbase,walk_dirs_for_plugins,directory_filter);
    g_free(dirbase);
  };
  /* intentional fallback. */
  for_each_in_dir(directory,dia_register_plugin,dia_plugin_filter);
}

void
dia_register_plugins(void)
{
  gchar *library_path;
  gchar *lib_dir;

  library_path = g_getenv("DIA_LIB_PATH");

  lib_dir = dia_config_filename("objects");

  if (lib_dir != NULL) {
    dia_register_plugins_in_dir(lib_dir);
    g_free(lib_dir);
  }

  if (library_path != NULL) {
    gchar **paths = g_strsplit(library_path, G_SEARCHPATH_SEPARATOR_S, 0);
    gint i;

    for (i = 0; paths[i] != NULL; i++) {
      dia_register_plugins_in_dir(paths[i]);
    }
    g_strfreev(paths);
  } else {
    library_path = dia_get_lib_directory("dia");

    dia_register_plugins_in_dir(library_path);
    g_free(library_path);
  }
  /* this isn't needed anymore */
  free_pluginrc();
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
    g_free(info);
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

  if (pluginrc)
    return;

  filename = dia_config_filename("pluginrc");
  pluginrc = xmlDiaParseFile(filename);
  g_free(filename);

  if (!pluginrc) {
    pluginrc = xmlNewDoc("1.0");
    pluginrc->encoding = xmlStrdup("UTF-8");
    xmlDocSetRootElement(pluginrc,
			 xmlNewDocNode(pluginrc, NULL, "plugins", NULL));
  }
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

    if (node->type != XML_ELEMENT_NODE || strcmp(node->name, "plugin") != 0)
      continue;
    node_filename = xmlGetProp(node, "filename");
    if (node_filename && !strcmp(filename, node_filename)) {
      xmlNodePtr node2;

      xmlFree(node_filename);
      for (node2 = node->xmlChildrenNode; 
           node2 != NULL; 
           node2 = node2->next) {
        if (xmlIsBlankNode(node2)) continue;
	if (node2->type == XML_ELEMENT_NODE &&
	    !strcmp(node2->name, "inhibit-load"))
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

    if (node->type != XML_ELEMENT_NODE || strcmp(node->name, "plugin") != 0)
      continue;
    node_filename = xmlGetProp(node, "filename");
    if (node_filename && !strcmp(info->filename, node_filename)) {
      xmlNodePtr node2;

      xmlFree(node_filename);
      for (node2 = node->xmlChildrenNode; 
           node2 != NULL; 
           node2 = node2->next) {
	char *content;

        if (xmlIsBlankNode(node2)) continue;

	if (node2->type != XML_ELEMENT_NODE)
	  continue;
	content = xmlNodeGetContent(node2);
	if (!strcmp(node2->name, "name")) {
	  g_free(info->name);
	  info->name = g_strdup(content);
	} else if (!strcmp(node2->name, "description")) {
	  g_free(info->description);
#ifdef UNICODE_WORK_IN_PROGRESS
	  info->description = g_strdup(content);
#else
    info->description = charconv_utf8_to_local8(content);
#endif
	}
	xmlFree(content);
      }
      break;
    }
    if (node_filename) xmlFree(node_filename);
  }
}

void
dia_pluginrc_write(void)
{
  gchar *filename;
  GList *tmp;

  ensure_pluginrc();
  for (tmp = plugins; tmp != NULL; tmp = tmp->next) {
    PluginInfo *info = tmp->data;
    xmlNodePtr node, pluginnode, datanode;

    if (info == NULL) {
      continue;
    }

    pluginnode = xmlNewNode(NULL, "plugin");
    datanode = xmlNewChild(pluginnode, NULL, "name", info->name);
#ifdef UNICODE_WORK_IN_PROGRESS
 {
     xmlChar *enc = xmlEncodeEntitiesReentrant(pluginnode->doc,
                                               info->description);
     datanode = xmlNewChild(pluginnode, NULL, "description", enc);
     xmlFree(enc);
 }
#else
 {
     gchar *utf = charconv_local8_to_utf8(info->description);
     xmlChar *enc = xmlEncodeEntitiesReentrant(pluginnode->doc,utf);
     g_free(utf);
     datanode = xmlNewChild(pluginnode, NULL, "description", enc);
     xmlFree(enc);
 }    
#endif
    if (info->inhibit_load)
      datanode = xmlNewChild(pluginnode, NULL, "inhibit-load", NULL);

    for (node = pluginrc->xmlRootNode->xmlChildrenNode; 
         node != NULL; 
         node = node->next) {
      xmlChar *node_filename;

      if (xmlIsBlankNode(node)) continue;

      if (node->type != XML_ELEMENT_NODE || strcmp(node->name, "plugin") != 0)
	continue;
      node_filename = xmlGetProp(node, "filename");
      if (node_filename && !strcmp(info->filename, node_filename)) {
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
    xmlSetProp(pluginnode, "filename", info->filename);
  }

  filename = dia_config_filename("pluginrc");
  
  xmlDiaSaveFile(filename,pluginrc);
  
  g_free(filename);
  free_pluginrc();
}


