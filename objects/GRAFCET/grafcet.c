/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * 
 * GRAFCET charts support
 * Copyright (C) 2000 Cyrille Chepelov
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
#include "object.h"

#include "config.h"
#include "intl.h"
#include "grafcet.h"
#include "plug-ins.h"

extern ObjectType step_type;
extern ObjectType action_type;
extern ObjectType transition_type;
extern ObjectType vergent_type; 
extern ObjectType vector_type;
extern ObjectType condition_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "GRAFCET", _("GRAFCET diagram objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&step_type);
  object_register_type(&action_type);
  object_register_type(&transition_type);
  object_register_type(&vergent_type); 
  object_register_type(&vector_type);
  object_register_type(&condition_type);

  return DIA_PLUGIN_INIT_OK;
}

