/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * autolayout-register.c -  registeration of auto-layout algoritms as plug-in
 * Copyright (c) 2008 Hans Breuer <hans@breuer.org>
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

#include "message.h"
#include "filter.h"
#include "plug-ins.h"

#include "stress-memory.h"


static DiaObjectChange *
stress_memory_callback (DiagramData *data,
                        const char  *filename,
                        /* further additions */
                        guint        flags,
                        void        *user_data)
{
  guint64 avail;

  if (vmem_avail (&avail)) {
    guint64 eat = (avail * 9) / 10;

    if (vmem_reserve (eat)) {
      guint64 still_avail;
      vmem_avail (&still_avail);

      message_warning ("%d MB of memory reserved,\n%d MB still available.\n",
                       (guint)((avail - still_avail)>>20),
		       (guint)(still_avail >> 20));
    } else {
      message_error ("Failed to reserve memory.");
    }
  } else {
    message_error ("Failed to calculate available memory.");
  }
  return NULL;
}

static DiaCallbackFilter cb_stress_memory = {
    "StressMemory",
    N_("Stress memory"),
    "/ToolboxMenu/Debug/StressMemory",
    stress_memory_callback,
    NULL
};

static gboolean
_plugin_can_unload (PluginInfo *info)
{
  return TRUE;
}

static void
_plugin_unload (PluginInfo *info)
{
  filter_unregister_callback (&cb_stress_memory);
  vmem_release ();
}

/* --- dia plug-in interface --- */

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Stress",
                            _("Stress memory (development tool)"),
                            _plugin_can_unload,
                            _plugin_unload))
    return DIA_PLUGIN_INIT_ERROR;

  filter_register_callback (&cb_stress_memory);

  return DIA_PLUGIN_INIT_OK;
}
