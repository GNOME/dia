/* dia -- an diagram creation/manipulation program
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
#ifndef DIA_XML_H
#define DIA_XML_H

#include <glib.h>
#include "geometry.h"
#include "color.h"
#include "font.h"
#include "diavar.h"
#include <libxml/tree.h>

DIAVAR int pretty_formated_xml;

typedef xmlNodePtr XML_NODE;

typedef XML_NODE ObjectNode;
typedef XML_NODE AttributeNode;
typedef XML_NODE DataNode;

typedef enum{
  DATATYPE_COMPOSITE,
  DATATYPE_INT,
  DATATYPE_ENUM,
  DATATYPE_REAL,
  DATATYPE_BOOLEAN,
  DATATYPE_COLOR,
  DATATYPE_POINT,
  DATATYPE_RECTANGLE,
  DATATYPE_STRING,
  DATATYPE_FONT
} DataType;

AttributeNode object_find_attribute(ObjectNode obj_node,
				    const char *attrname);
AttributeNode composite_find_attribute(DataNode composite_node,
				       const char *attrname);
int attribute_num_data(AttributeNode attribute);
DataNode attribute_first_data(AttributeNode attribute);
DataNode data_next(DataNode data);
DataType data_type(DataNode data);
int data_int(DataNode data);
int data_enum(DataNode data);
real data_real(DataNode data);
int data_boolean(DataNode data);
void data_color(DataNode data, Color *col);
void data_point(DataNode data, Point *point);
void data_rectangle(DataNode data, Rectangle *rect);
char *data_string(DataNode data);
DiaFont *data_font(DataNode data);

AttributeNode new_attribute(ObjectNode obj_node, const char *attrname);
AttributeNode composite_add_attribute(DataNode composite_node,
				      const char *attrname);
void data_add_int(AttributeNode attr, int data);
void data_add_enum(AttributeNode attr, int data);
void data_add_real(AttributeNode attr, real data);
void data_add_boolean(AttributeNode attr, int data);
void data_add_color(AttributeNode attr, const Color *col);
void data_add_point(AttributeNode attr, const Point *point);
void data_add_rectangle(AttributeNode attr, const Rectangle *rect);
void data_add_string(AttributeNode attr, const char *str);
void data_add_font(AttributeNode attr, const DiaFont *font);
DataNode data_add_composite(AttributeNode attr, 
                            const char *type); /* can be NULL */

#endif /* DIA_XML_H */

