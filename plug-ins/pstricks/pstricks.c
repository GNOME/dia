/* -*- Mode: C; c-basic-offset: 4 -*- */
#include "config.h"
#include "render_pstricks.h"
#include "intl.h"
#include "filter.h"
#include "plug-ins.h"

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Pstricks",
			    _("TeX Pstricks export filter"), NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&pstricks_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
