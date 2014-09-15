/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * plug-ins.h: plugin loading handling interfaces for dia
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

#ifndef PLUG_INS_H
#define PLUG_INS_H

#include <glib.h>
#include <gmodule.h>
#include "diatypes.h"

G_BEGIN_DECLS

/*!
 * \brief Plug-in API version for breaking binary compatibility
 *
 * The api version to ensure dia core and the plug-ins agree on
 * talking about the same thing. If some incompatible change is 
 * made to the interface between plug-ins and core it needs to
 * be incremented to allow the core to detect outdated plug-ins
 * and avoid to load them - which otherwise would produce strange
 * misinterpretations or even crashes.
 *
 * Some of the things which require a new plug-in api version are :
 *  - Changes to DiaRenderer's vtable, that is adding new methods, 
 *    especially if they are shifting the placement of previously 
 *    defined ones, see : lib/diarenderer.h
 *  - Changes to lib/object.h:_ObjectOps
 *  - Changes to _DiaExportFilter, _DiaImportFilter, lib/filter.h
 *  - New, incompatibe versions of libraries where both the core
 *    and the plug-in depend on (Dia >0.90 crashing on pygtk 1.x
 *    could have been avoided this way)
 * The list is by no means complete. If in doubt about your change
 * please ask on dia-list or alternative increment ;-)      --hb
 *
 * \ingroup Plugins
 */
#define DIA_PLUGIN_API_VERSION 21

typedef enum {
  DIA_PLUGIN_INIT_OK,
  DIA_PLUGIN_INIT_ERROR
} PluginInitResult;

typedef PluginInitResult (*PluginInitFunc) (PluginInfo *info);
typedef gboolean (*PluginCanUnloadFunc) (PluginInfo *info);
typedef void (*PluginUnloadFunc) (PluginInfo *info);

/* functions for use by plugins ... */

gboolean dia_plugin_info_init(PluginInfo *info,
			      const gchar *name,
			      const gchar *description,
			      PluginCanUnloadFunc can_unload_func, 
			      PluginUnloadFunc unload_func);

gchar *dia_plugin_check_version(gint version);


/* functiosn for use by dia ... */
const gchar *dia_plugin_get_filename    (PluginInfo *info);
const gchar *dia_plugin_get_name        (PluginInfo *info);
const gchar *dia_plugin_get_description (PluginInfo *info);
const gpointer *dia_plugin_get_symbol   (PluginInfo *info, const gchar *name);

gboolean dia_plugin_can_unload       (PluginInfo *info);
gboolean dia_plugin_is_loaded        (PluginInfo *info);
gboolean dia_plugin_get_inhibit_load (PluginInfo *info);
void     dia_plugin_set_inhibit_load (PluginInfo *info, gboolean inhibit_load);

void dia_plugin_load   (PluginInfo *info);
void dia_plugin_unload (PluginInfo *info);

void dia_register_plugin         (const gchar *filename);
void dia_register_plugins_in_dir (const gchar *directory);
void dia_register_plugins        (void);
void dia_register_builtin_plugin (PluginInitFunc init_func);

GList *dia_list_plugins(void);

void dia_pluginrc_write(void);

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

G_END_DECLS

#endif
