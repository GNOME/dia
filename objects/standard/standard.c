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
#include "sheet.h"
#include "intl.h"
#include "plug-ins.h"

/*!
\defgroup StandardObjects Standard Objects
\brief The minimal set of objects available to Dia

\ingroup Objects

The minimal set of objects available to Dia is defined in the directory objects/standard.
Although implemented as a plug-in Dia wont start without it being loaded.
These objects are not only referenced directly in the toolbox and application menu,
but they also are required for most of Dia's diagram importers.
*/

extern DiaObjectType *_arc_type;
extern DiaObjectType *_box_type;
extern DiaObjectType *_ellipse_type;
extern DiaObjectType *_line_type;
extern DiaObjectType *_zigzagline_type;
extern DiaObjectType *_polyline_type;
extern DiaObjectType *_bezierline_type;
extern DiaObjectType *_textobj_type;
extern DiaObjectType *_image_type;
extern DiaObjectType *_outline_type;
extern DiaObjectType *_polygon_type;
extern DiaObjectType *_beziergon_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "Standard", _("Standard objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(_arc_type);
  object_register_type(_box_type);
  object_register_type(_ellipse_type);
  object_register_type(_line_type);
  object_register_type(_polyline_type);
  object_register_type(_zigzagline_type);
  object_register_type(_bezierline_type);
  object_register_type(_textobj_type);
  object_register_type(_image_type);
#ifdef HAVE_CAIRO
  object_register_type(_outline_type);
#endif
  object_register_type(_polygon_type);
  object_register_type(_beziergon_type);

  return DIA_PLUGIN_INIT_OK;
}

