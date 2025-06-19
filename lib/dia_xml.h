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
#include "diacontext.h"
#include <libxml/tree.h>

/*!
 * \note Dia's diagram namespace
 * \ingroup DiagramXmlIo
 *
 * Though the Dia homepage is now http://www.gnome.org/projects/dia
 * Dia's xml namespace definition still needs to point to the
 * original site. In fact the xml namespace definition has nothing to do
 * with existing website, they just need to be unique to allow to
 * differentiate between varying definitions. Changing the namespace
 * from the old to the new site would strictly speaking break all
 * older diagrams - although some tools may not take the actual
 * key into account.
 * See also : doc/diagram.dtd
 */
#define DIA_XML_NAME_SPACE_BASE "http://www.lysator.liu.se/~alla/dia/"

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
  DATATYPE_FONT,
  DATATYPE_BEZPOINT,
  DATATYPE_DICT,
  DATATYPE_PIXBUF
} DataType;

AttributeNode object_find_attribute(ObjectNode obj_node,
				    const char *attrname);
AttributeNode composite_find_attribute(DataNode composite_node,
				       const char *attrname);
int attribute_num_data(AttributeNode attribute);
DataNode attribute_first_data(AttributeNode attribute);
DataNode data_next(DataNode data);
DataType data_type(DataNode data, DiaContext *ctx);
int data_int(DataNode data, DiaContext *ctx);
int data_enum(DataNode data, DiaContext *ctx);
real data_real(DataNode data, DiaContext *ctx);
int data_boolean(DataNode data, DiaContext *ctx);
void data_color(DataNode data, Color *col, DiaContext *ctx);
void data_point(DataNode data, Point *point, DiaContext *ctx);
void data_bezpoint(DataNode data, BezPoint *point, DiaContext *ctx);
void data_rectangle(DataNode data, DiaRectangle *rect, DiaContext *ctx);
char *data_string(DataNode data, DiaContext *ctx);
char *data_filename(DataNode data, DiaContext *ctx);
DiaFont *data_font(DataNode data, DiaContext *ctx);

AttributeNode new_attribute(ObjectNode obj_node, const char *attrname);
AttributeNode composite_add_attribute(DataNode composite_node,
				      const char *attrname);
void data_add_int(AttributeNode attr, int data, DiaContext *ctx);
void data_add_enum(AttributeNode attr, int data, DiaContext *ctx);
void data_add_real(AttributeNode attr, real data, DiaContext *ctx);
void data_add_boolean(AttributeNode attr, int data, DiaContext *ctx);
void data_add_color(AttributeNode attr, const Color *col, DiaContext *ctx);
void data_add_point(AttributeNode attr, const Point *point, DiaContext *ctx);
void data_add_bezpoint(AttributeNode attr, const BezPoint *point, DiaContext *ctx);
void data_add_rectangle(AttributeNode attr, const DiaRectangle *rect, DiaContext *ctx);
void data_add_string(AttributeNode attr, const char *str, DiaContext *ctx);
void data_add_filename(AttributeNode attr, const char *str, DiaContext *ctx);
void data_add_font(AttributeNode attr, DiaFont *font, DiaContext *ctx);
DataNode data_add_composite(AttributeNode attr,
			    const char *type, /* can be NULL */
			    DiaContext *ctx);

GHashTable *data_dict (DataNode data, DiaContext *ctx);
void data_add_dict (AttributeNode attr, GHashTable *data, DiaContext *ctx);

GdkPixbuf *data_pixbuf (DataNode data, DiaContext *ctx);
void data_add_pixbuf (AttributeNode attr, GdkPixbuf *pixbuf, DiaContext *ctx);

DiaMatrix *data_matrix(DataNode data);
void data_add_matrix(AttributeNode attr, DiaMatrix *matrix, DiaContext *ctx);

DiaPattern *data_pattern(DataNode data, DiaContext *ctx);
void data_add_pattern(AttributeNode attr, DiaPattern *pat, DiaContext *ctx);


#define dia_clear_xml_string(pointer) g_clear_pointer(pointer, xmlFree)


#endif /* DIA_XML_H */

