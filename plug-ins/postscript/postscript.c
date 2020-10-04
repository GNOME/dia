/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * postscript.c -- postscript rendering as a plug-in
 * Copyright (C) 2008, Hans Breuer, <Hans@Breuer.Org>
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

#include "intl.h"
#include "message.h"
#include "filter.h"
#include "plug-ins.h"

#include "render_eps.h"
#include "paginate_psprint.h"


static DiaObjectChange *
print_callback (DiagramData *data,
                const char  *filename,
                /* further additions */
                guint        flags,
                void        *user_data)
{
  if (!data) {
    message_error (_("Nothing to print"));
  } else {
    diagram_print_ps (data, filename ? filename : "output.ps");
  }

  return NULL;
}


static DiaCallbackFilter cb_ps_print = {
  "FilePrintPS",
  N_("Print (PS)"),
  "/InvisibleMenu/File/FilePrint",
  print_callback,
  NULL
};


/* --- dia plug-in interface --- */
static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return FALSE;
}

static void
_plugin_unload (PluginInfo *info)
{
  /* EPS with PS fonts */
  filter_unregister_export(&eps_export_filter);

#ifndef G_OS_WIN32
  /* on win32 this is too uncommon, can only print to postscript printers */
  filter_unregister_callback (&cb_ps_print);
#endif
}

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Postscript",
                            _("PostScript Rendering"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  /* EPS with PS fonts */
  filter_register_export(&eps_export_filter);

#ifndef G_OS_WIN32
  /* on win32 this is too uncommon, can only print to postscript printers */
  filter_register_callback (&cb_ps_print);
#endif

  return DIA_PLUGIN_INIT_OK;
}

