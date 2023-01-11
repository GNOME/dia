/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
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

#include "aadl.h"

extern DiaObjectType  aadldata_type;
extern DiaObjectType  aadlprocessor_type;
extern DiaObjectType  aadldevice_type;
extern DiaObjectType  aadlsystem_type;
extern DiaObjectType  aadlsubprogram_type;
extern DiaObjectType  aadlthreadgroup_type;
extern DiaObjectType  aadlprocess_type;
extern DiaObjectType  aadlthread_type;
extern DiaObjectType  aadlbus_type;
extern DiaObjectType  aadlmemory_type;
extern DiaObjectType  aadlpackage_type;


DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "AADL",
	       _("Architecture Analysis & Design Language diagram objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&aadldata_type);
  object_register_type(&aadlprocessor_type);
  object_register_type(&aadldevice_type);
  object_register_type(&aadlsystem_type);
  object_register_type(&aadlsubprogram_type);
  object_register_type(&aadlthreadgroup_type);
  object_register_type(&aadlprocess_type);
  object_register_type(&aadlthread_type);
  object_register_type(&aadlbus_type);
  object_register_type(&aadlmemory_type);
  object_register_type(&aadlpackage_type);

  return DIA_PLUGIN_INIT_OK;
}

