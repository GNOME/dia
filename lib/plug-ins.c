#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "plug-ins.h"
#include "intl.h"
#include "message.h"
#include "dia_dirs.h"

struct _PluginInfo {
  GModule *module;
  gchar *filename;      /* plugin filename */
  gchar *real_filename; /* not a .la file */

  gboolean is_loaded;

  gchar *name;
  gchar *description;

  PluginInitFunc init_func;
  PluginCanUnloadFunc can_unload_func;
  PluginUnloadFunc unload_func;
};

static GList *plugins = NULL;

gboolean
dia_plugin_info_init(PluginInfo *info, gchar *name, gchar *description,
		     PluginCanUnloadFunc can_unload_func, 
		     PluginUnloadFunc unload_func)
{
  g_free(info->name);
  info->name = g_strdup(name);
  g_free(info->description);
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
dia_plugin_get_description(PluginInfo *info) {
  return info->description;
}

gboolean
dia_plugin_can_unload(PluginInfo *info) {
  return (info->can_unload_func != NULL) && (* info->can_unload_func)(info);
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

static PluginInfo *
plugin_load(const gchar *filename)
{
  PluginInfo *info;
  gchar *real_filename;
  GModule *module;
  PluginInitFunc init_func;

  g_return_val_if_fail(filename != NULL, NULL);

  real_filename = find_real_filename(filename);
  if (real_filename == NULL) {
    message_error(_("Could not deduce correct path for `%s'"), filename);
    return NULL;
  }
  module = g_module_open(real_filename, G_MODULE_BIND_LAZY);
  if (!module) {
    g_free(real_filename);
    message_error(_("Could not load plugin `%s'\n%s"), filename,
		  g_module_error());
    return NULL;
  }

  if (!g_module_symbol(module, "dia_plugin_init", (gpointer)&init_func)) {
    g_module_close(module);
    g_free(real_filename);

    message_error(_("Could not find plugin init function in `%s'"), filename);
    return NULL;
  }

  info = g_new0(PluginInfo, 1);
  info->module = module;
  info->filename = g_strdup(filename);
  info->real_filename = real_filename;
  info->is_loaded = TRUE;
  info->init_func = init_func;
  
  if ((*info->init_func)(info) != DIA_PLUGIN_INIT_OK) {
    /* plugin displayed an error message */
    g_module_close(module);
    g_free(real_filename);
    g_free(info->filename);
    g_free(info);
    return NULL;
  }

  return info;
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

  info = plugin_load(filename);
  if (info)
    plugins = g_list_prepend(plugins, info);
}

void
dia_register_plugins_in_dir(const gchar *directory)
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
    gint len = strlen(dirp->d_name);

    if (len > 3 && !strcmp(&dirp->d_name[len-3], ".la")) {
      gchar *filename = g_strconcat(directory, G_DIR_SEPARATOR_S,
				    dirp->d_name, NULL);

      dia_register_plugin(filename);
      g_free(filename);
    }
  }
  closedir(dp);
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
    g_free(library_path);
  } else {
    library_path = dia_get_lib_directory("dia");
    dia_register_plugins_in_dir(library_path);
    g_free(library_path);
  }
}

void
dia_register_builtin_plugin(PluginInitFunc init_func)
{
  PluginInfo *info;

  info = g_new0(PluginInfo, 1);
  info->filename = "<builtin>";
  info->is_loaded = TRUE;

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
