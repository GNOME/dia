#ifndef PLUG_INS_H
#define PLUG_INS_H

#include <glib.h>
#include <gmodule.h>

#define DIA_PLUGIN_API_VERSION 1

typedef struct _PluginInfo PluginInfo;

typedef enum {
  DIA_PLUGIN_INIT_OK,
  DIA_PLUGIN_INIT_ERROR
} PluginInitResult;

typedef PluginInitResult (*PluginInitFunc) (PluginInfo *info);
typedef gboolean (*PluginCanUnloadFunc) (PluginInfo *info);
typedef void (*PluginUnloadFunc) (PluginInfo *info);

/* functions for use by plugins ... */

gboolean dia_plugin_info_init(PluginInfo *info, gchar *name,
			      gchar *description,
			      PluginCanUnloadFunc can_unload_func, 
			      PluginUnloadFunc unload_func);

gchar *dia_plugin_check_version(gint version);


/* functiosn for use by dia ... */

const gchar *dia_plugin_get_filename(PluginInfo *info);
const gchar *dia_plugin_get_name(PluginInfo *info);
const gchar *dia_plugin_get_description(PluginInfo *info);
gboolean dia_plugin_can_unload(PluginInfo *info);

void dia_register_plugin(const gchar *filename);
void dia_register_plugins_in_dir(const gchar *directory);
void dia_register_plugins(void);

void dia_register_builtin_plugin(PluginInitFunc init_func);

GList *dia_list_plugins(void);

/* macro defining the version check that should be implemented by all
 * plugins. */
#define DIA_PLUGIN_CHECK_INIT \
G_MODULE_EXPORT const gchar *g_module_check_init(GModule *gmodule); \
const gchar * \
g_module_check_init(GModule *gmodule) \
{ \
  return dia_plugin_check_version(DIA_PLUGIN_API_VERSION); \
}

/* prototype for plugin init function (should be implemented by plugin) */
G_MODULE_EXPORT PluginInitResult dia_plugin_init(PluginInfo *info);

#endif
