/* -*- Mode: C; c-basic-offset: 4 -*- */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "render_pstricks.h"
#include "filter.h"
#include "plug-ins.h"

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export(&pstricks_export_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Pstricks",
			    _("TeX PSTricks export filter"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&pstricks_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
