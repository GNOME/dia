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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "object.h"

#include "intl.h"
#include "sybase.h"
#include "plug-ins.h"

Color computer_color = { 0.7, 0.7, 0.7 };

extern DiaObjectType dataserver_type;
extern DiaObjectType repserver_type;
extern DiaObjectType ltm_type;
extern DiaObjectType stableq_type;
extern DiaObjectType client_type;
extern DiaObjectType rsm_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Sybase",
			    _("Sybase replication domain diagram objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  /* object_register_type(&dataserver_type); */
  /* object_register_type(&repserver_type); */
  /* object_register_type(&ltm_type); */
  /* object_register_type(&stableq_type); */
  /* object_register_type(&client_type); */
  /* object_register_type(&rsm_type); */

  return DIA_PLUGIN_INIT_OK;
}

