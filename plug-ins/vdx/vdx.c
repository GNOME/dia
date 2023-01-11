/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * vdx.c: Visio XML import filter for dia
 * Copyright (C) 2006-2007 Ian Redfern
 * based on the xfig filter code
 * Copyright (C) 2001 Lars Clausen
 * based on the dxf filter code
 * Copyright (C) 2000 James Henstridge, Steffen Macke
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "filter.h"
#include "plug-ins.h"

extern DiaImportFilter vdx_import_filter;
extern DiaExportFilter vdx_export_filter;

DIA_PLUGIN_CHECK_INIT
static gboolean
_plugin_can_unload (PluginInfo *info)
{
    return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
    filter_unregister_export(&vdx_export_filter);
    filter_unregister_import(&vdx_import_filter);
}


PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "VDX",
			    _("Visio XML Format import and export filter"),
			    _plugin_can_unload,
                            _plugin_unload)
      )
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_import(&vdx_import_filter);
  filter_register_export(&vdx_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
