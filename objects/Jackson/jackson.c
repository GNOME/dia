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
#include "jackson.h"
#include "plug-ins.h"

extern DiaObjectType jackson_domain_type;
extern DiaObjectType jackson_requirement_type;
extern DiaObjectType jackson_phenomenon_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Jackson", _("Jackson diagram"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&jackson_domain_type);
  object_register_type(&jackson_requirement_type);
  object_register_type(&jackson_phenomenon_type);

  return DIA_PLUGIN_INIT_OK;
}
