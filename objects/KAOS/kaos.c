/***************************************************************************
                          kaos.c  -  description
                             -------------------
    begin                : Sat Nov 23 2002
    copyright            : (C) 2002 by cp
    email                : cp@DELL-FAUST
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
                
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "object.h"

#include "intl.h"
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
