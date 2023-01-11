/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Objects for drawing i* actor diagrams.
 * Copyright (C) 2002 Christophe Ponsard
 *
 * based on Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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

#include "object.h"
#include "istar.h"
#include "plug-ins.h"

extern DiaObjectType istar_actor_type;
extern DiaObjectType istar_goal_type;
extern DiaObjectType istar_other_type;
extern DiaObjectType istar_link_type;


DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Istar", _("Istar diagram"), NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&istar_actor_type);
  object_register_type(&istar_goal_type);
  object_register_type(&istar_other_type);
  object_register_type(&istar_link_type);

  return DIA_PLUGIN_INIT_OK;
}
