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
#ifndef _LINE_INFO_H_
#define _LINE_INFO_H_

#include <glib.h>

#include "geometry.h"
#include "dia_xml.h"
#include "object.h"
#include "text.h"
#include "properties.h"
#include "dia_svg.h"

typedef enum {
  CUSTOM_LINETYPE_ZIGZAGLINE,
  CUSTOM_LINETYPE_POLYLINE,
  CUSTOM_LINETYPE_BEZIERLINE,
  CUSTOM_LINETYPE_ALL
} CustomLineType;

extern char* custom_linetype_strings[];

typedef struct _LineInfo {
  char *line_info_filename;

  char *name;
  char *icon_filename;
  CustomLineType type;
  Color line_color;
  DiaLineStyle line_style;
  double dashlength;
  double line_width;
  double corner_radius;
  Arrow start_arrow, end_arrow;

  DiaObjectType* object_type;
} LineInfo;

/* there is no destructor for LineInfo at the moment */
LineInfo* line_info_load(const gchar *filename);
LineInfo* line_info_clone(LineInfo* info);

#endif /* _LINE_INFO_H_ */
