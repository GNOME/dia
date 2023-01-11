/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Miscellaneous objects
 * Copyright (C) 2002 Cyrille Chépélov
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

#include "plug-ins.h"

extern DiaObjectType analog_clock_type;
extern DiaObjectType grid_object_type;
extern DiaObjectType tree_type;
extern DiaObjectType measure_type;
extern DiaObjectType diagram_as_element_type;
extern DiaObjectType _ngon_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Misc",_("Miscellaneous objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&analog_clock_type);
  object_register_type(&grid_object_type);
  object_register_type(&tree_type);
  object_register_type(&measure_type);
  object_register_type(&diagram_as_element_type);
  object_register_type(&_ngon_type);

  return DIA_PLUGIN_INIT_OK;
}
