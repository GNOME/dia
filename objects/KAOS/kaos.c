/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Objects for drawing KAOS goal diagrams.
 * This class supports the whole goal specialization hierarchy
 * Copyright (C) 2002 Christophe Ponsard
 *
 * Based on SADT box object
 * Copyright (C) 2000, 2001 Cyrille Chepelov
 *
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
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
#include "kaos.h"
#include "plug-ins.h"

extern DiaObjectType kaos_goal_type;
extern DiaObjectType kaos_other_type;
extern DiaObjectType kaos_maor_type;
extern DiaObjectType kaos_mbr_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "KAOS", _("KAOS diagram"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&kaos_goal_type);
  object_register_type(&kaos_other_type);
  object_register_type(&kaos_maor_type);
  object_register_type(&kaos_mbr_type);

  return DIA_PLUGIN_INIT_OK;
}
