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
