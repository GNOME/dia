/* -*- Mode: C; c-basic-offset: 4 -*- */
#include "config.h"
#include "intl.h"
#include "filter.h"
#include "plug-ins.h"

extern DiaExportFilter dxf_export_filter;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "DXF",
			    _("Drawing Interchange File export filter"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&dxf_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
