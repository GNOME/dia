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
#ifndef DIA_XML_H
#define DIA_XML_H

#include "config.h"
#include <glib.h>
#include "geometry.h"
#include "color.h"
#include "font.h"

#ifdef __XML_TREE_H__
typedef xmlNodePtr XML_NODE;
#else
typedef void * XML_NODE;
#endif

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

extern AttributeNode object_find_attribute(ObjectNode obj_node,
					   const char *attrname);
extern AttributeNode composite_find_attribute(DataNode composite_node,
					      const char *attrname);
extern int attribute_num_data(AttributeNode attribute);
extern DataNode attribute_first_data(AttributeNode attribute);
extern DataNode data_next(DataNode data);
extern DataType data_type(DataNode data);
extern int data_int(DataNode data);
extern int data_enum(DataNode data);
extern real data_real(DataNode data);
extern int data_boolean(DataNode data);
extern void data_color(DataNode data, Color *col);
extern void data_point(DataNode data, Point *point);
extern void data_rectangle(DataNode data, Rectangle *rect);
extern char *data_string(DataNode data);
extern Font *data_font(DataNode data);

extern AttributeNode new_attribute(ObjectNode obj_node,
				   const char *attrname);
extern AttributeNode composite_add_attribute(DataNode composite_node,
					     const char *attrname);
extern void data_add_int(AttributeNode attr, int data);
extern void data_add_enum(AttributeNode attr, int data);
extern void data_add_real(AttributeNode attr, real data);
extern void data_add_boolean(AttributeNode attr, int data);
extern void data_add_color(AttributeNode attr, Color *col);
extern void data_add_point(AttributeNode attr, Point *point);
extern void data_add_rectangle(AttributeNode attr, Rectangle *rect);
extern void data_add_string(AttributeNode attr, char *str);
extern void data_add_font(AttributeNode attr, Font *font);
extern DataNode data_add_composite(AttributeNode attr,
				   char *type); /* type can be NULL */

#endif /* DIA_XML_H */

