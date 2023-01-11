/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * svg.c: SVG export filters for dia
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "filter.h"
#include "plug-ins.h"

/*!
 * \defgroup SvgPlugin SVG Plug-in
 * \ingroup Plugins
 * \brief SVG Import/Export Plug-in
 *
 * One of the more capable exchange formats supported by Dia is SVG.
 * It is used internally for Shapes (\sa ObjectCustom) but is also available
 * for diagram/image exchange via file or clipboard.
 */

extern DiaExportFilter svg_export_filter;
extern DiaImportFilter svg_import_filter;

DIA_PLUGIN_CHECK_INIT

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export(&svg_export_filter);
  filter_unregister_import(&svg_import_filter);
}

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "SVG",
                            _("Scalable Vector Graphics import and export filters"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_export(&svg_export_filter);
  filter_register_import(&svg_import_filter);

  return DIA_PLUGIN_INIT_OK;
}
