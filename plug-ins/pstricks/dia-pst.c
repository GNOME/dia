/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#include "dia-pst-renderer.h"
#include "filter.h"
#include "plug-ins.h"


static gboolean
dia_pst_export (DiagramData *data,
                DiaContext  *ctx,
                const char  *filename,
                const char  *diafilename,
                void        *user_data)
{
  DiaPstRenderer *renderer = g_object_new (DIA_PST_TYPE_RENDERER, NULL);
  gboolean result = dia_pst_renderer_export (renderer,
                                             ctx,
                                             data,
                                             filename,
                                             diafilename);

  g_clear_object (&renderer);

  return result;
}


static const char *extensions[] = { "tex", NULL };
DiaExportFilter pstricks_export_filter = {
  N_("TeX PSTricks macros"),
  extensions,
  dia_pst_export,
  NULL,
  "pstricks-tex"
};


static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}


static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_export (&pstricks_export_filter);
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init (PluginInfo *info)
{
  if (!dia_plugin_info_init (info,
                             "Pstricks",
                             _("TeX PSTricks export filter"),
                             _plugin_can_unload,
                             _plugin_unload)) {
    return DIA_PLUGIN_INIT_ERROR;
  }

  filter_register_export (&pstricks_export_filter);

  return DIA_PLUGIN_INIT_OK;
}
