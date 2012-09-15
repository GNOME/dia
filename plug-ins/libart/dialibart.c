/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialibart.c -- libart based export plugin for dia
 *                moved out of the core 2008, Hans Breuer, <Hans@Breuer.Org>
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
#include <stdio.h>

#include "intl.h"
#include "filter.h"
#include "plug-ins.h"

#include "dialibartrenderer.h"

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
extern DiaExportFilter png_export_filter;
#endif

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  /* Can't unlaod as long as we are giving away our types, e.g. dia_libart_renderer_get_type () */
  return FALSE;
}

static void
_plugin_unload (PluginInfo *info)
{
#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
  filter_unregister_export(&png_export_filter);
#endif
}

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Libart",
                            _("Libart-based rendering"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

#if defined(HAVE_LIBPNG) && defined(HAVE_LIBART)
  /* FIXME: need to think about of proper way of registartion, see also app/display.c */
  png_export_filter.renderer_type = dia_libart_renderer_get_type ();
  /* PNG with libart rendering */
  filter_register_export(&png_export_filter);
#endif
  
  return DIA_PLUGIN_INIT_OK;
}
