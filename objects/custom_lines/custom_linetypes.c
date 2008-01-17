/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Lines -- line shapes defined in XML rather than C.
 * Based on the original Custom Objects plugin.
 * Copyright (C) 1999 James Henstridge.
 * Adapted for Custom Lines plugin by Marcel Toele.
 * Modifications (C) 2007 Kern Automatiseringsdiensten BV.
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

#include <assert.h>
#include <gmodule.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "object.h"

#include "../standard/polyline.h"
#include "../standard/zigzagline.h"
#include "../standard/bezier.h"

#include "line_info.h"
#include "custom_linetypes.h"

#include "pixmaps/default.xpm"

ObjectTypeOps custom_zigzagline_type_ops;
ObjectTypeOps custom_polyline_type_ops;
ObjectTypeOps custom_bezierline_type_ops;


void custom_linetype_new(LineInfo *info, DiaObjectType **otype);
void zigzagline_apply_properties( Zigzagline* line, LineInfo* info );
DiaObject* customline_create(Point *startpoint,
                             void *user_data,
                             Handle **handle1,
                             Handle **handle2);

void custom_linetypes_init()
{ 
  custom_zigzagline_type_ops = zigzagline_type_ops;
  custom_zigzagline_type_ops.create = customline_create;
  custom_zigzagline_type_ops.get_defaults = NULL;
  custom_zigzagline_type_ops.apply_defaults = NULL;
  
  custom_polyline_type_ops = polyline_type_ops;
  custom_polyline_type_ops.create = customline_create;
  custom_polyline_type_ops.get_defaults = NULL;
  custom_polyline_type_ops.apply_defaults = NULL;
  
  custom_bezierline_type_ops = bezierline_type_ops;
  custom_bezierline_type_ops.create = customline_create;
  custom_bezierline_type_ops.get_defaults = NULL;
  custom_bezierline_type_ops.apply_defaults = NULL;
}

void zigzagline_apply_properties( Zigzagline* line, LineInfo* info )
{
  line->line_color = info->line_color;
  line->line_style = info->line_style;
  line->dashlength = info->dashlength;
  line->line_width = info->line_width;
  line->corner_radius = info->corner_radius;
  line->start_arrow = info->start_arrow;
  line->end_arrow = info->end_arrow;
}
void polyline_apply_properties( Polyline* line, LineInfo* info )
{
  line->line_color = info->line_color;
  line->line_style = info->line_style;
  line->dashlength = info->dashlength;
  line->line_width = info->line_width;
  line->corner_radius = info->corner_radius;
  line->start_arrow = info->start_arrow;
  line->end_arrow = info->end_arrow;
}
void bezierline_apply_properties( Bezierline* line, LineInfo* info )
{
  line->line_color = info->line_color;
  line->line_style = info->line_style;
  line->dashlength = info->dashlength;
  line->line_width = info->line_width;
  line->start_arrow = info->start_arrow;
  line->end_arrow = info->end_arrow;
}

DiaObject *
customline_create(Point *startpoint,
                  void *user_data,
                  Handle **handle1,
                  Handle **handle2)
{
  DiaObject* res = NULL;
  LineInfo* line_info = (LineInfo*)user_data;
	
  if( line_info->type == CUSTOM_LINETYPE_ZIGZAGLINE ) {
    res = zigzagline_create( startpoint, user_data, handle1, handle2 );
    zigzagline_apply_properties( (Zigzagline*)res, line_info );
  } else if( line_info->type == CUSTOM_LINETYPE_POLYLINE ) {
    res = polyline_create( startpoint, NULL, handle1, handle2 );
    polyline_apply_properties( (Polyline*)res, line_info );
  } else if( line_info->type == CUSTOM_LINETYPE_BEZIERLINE ) {
   res = bezierline_create( startpoint, NULL, handle1, handle2 );
    bezierline_apply_properties( (Bezierline*)res, line_info );
  } else
    g_warning(_("INTERNAL: CustomLines: Illegal line type in LineInfo object."));

  res->type = line_info->object_type;

  return( res );
}

void custom_linetype_new(LineInfo *info, DiaObjectType **otype)
{
  DiaObjectType *obj = g_new0(DiaObjectType, 1);

  obj->version = 1;
  obj->pixmap = default_xpm;

  if (info->type == CUSTOM_LINETYPE_ZIGZAGLINE)
    obj->ops = &custom_zigzagline_type_ops;
  else if (info->type == CUSTOM_LINETYPE_POLYLINE)
    obj->ops = &custom_polyline_type_ops;
  else if (info->type == CUSTOM_LINETYPE_BEZIERLINE)
    obj->ops = &custom_bezierline_type_ops;
  else
      g_warning(_("INTERNAL: CustomLines: Illegal line type in LineInfo object %s."),
                obj->name);

  obj->name = info->name;
  obj->default_user_data = info;

  if (info->icon_filename) {
    struct stat buf;
    if (0==stat(info->icon_filename,&buf)) {
      obj->pixmap = NULL;
      obj->pixmap_file = info->icon_filename;
    } else {
      g_warning(_("Cannot open icon file %s for object type '%s'."),
                info->icon_filename, obj->name);
    }
  }

  info->object_type = obj; /* <-- Reciproce type linking */

  obj->default_user_data = (void*)info;

  *otype = obj;
}
