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
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <tree.h>
#include "utils.h"
#include "dia_xml.h"
#include "message.h"

AttributeNode
object_find_attribute(ObjectNode obj_node,
		      const char *attrname)
{
  AttributeNode attr;
  const char *name;

  attr =  obj_node->childs;
  while (attr != NULL) {

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) )
      return attr;
    
    attr = attr->next;
  }
  return NULL;
}

AttributeNode
composite_find_attribute(DataNode composite_node,
			 const char *attrname)
{
  AttributeNode attr;
  const char *name;

  attr =  composite_node->childs;
  while (attr != NULL) {

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) )
      return attr;
    
    attr = attr->next;
  }
  return NULL;
}

int
attribute_num_data(AttributeNode attribute)
{
  xmlNode *data;
  int nr=0;

  data =  attribute->childs;
  while (data != NULL) {
    nr++;
    data = data->next;
  }
  return nr;
}

DataNode
attribute_first_data(AttributeNode attribute)
{
  return (DataNode) attribute->childs;
}

DataNode
data_next(DataNode data)
{
  return (DataNode) data->next;
}

DataType
data_type(DataNode data)
{
  const char *name;

  name = data->name;
  if (strcmp(name, "composite")==0) {
    return DATATYPE_COMPOSITE;
  } else if (strcmp(name, "int")==0) {
    return DATATYPE_INT;
  } else if (strcmp(name, "enum")==0) {
    return DATATYPE_ENUM;
  } else if (strcmp(name, "real")==0) {
    return DATATYPE_REAL;
  } else if (strcmp(name, "boolean")==0) {
    return DATATYPE_BOOLEAN;
  } else if (strcmp(name, "color")==0) {
    return DATATYPE_COLOR;
  } else if (strcmp(name, "point")==0) {
    return DATATYPE_POINT;
  } else if (strcmp(name, "rectangle")==0) {
    return DATATYPE_RECTANGLE;
  } else if (strcmp(name, "string")==0) {
    return DATATYPE_STRING;
  } else if (strcmp(name, "font")==0) {
    return DATATYPE_FONT;
  }

  message_error("Unknown type of DataNode");
  return 0;
}


int
data_int(DataNode data)
{
  const char *val;
  
  if (data_type(data)!=DATATYPE_INT) {
    message_error("Taking int value of non-int node.");
    return 0;
  }

  val = xmlGetProp(data, "val");

  return atoi(val);
}

int data_enum(DataNode data)
{
  const char *val;
  
  if (data_type(data)!=DATATYPE_ENUM) {
    message_error("Taking enum value of non-enum node.");
    return 0;
  }

  val = xmlGetProp(data, "val");

  return atoi(val);
}

real
data_real(DataNode data)
{
  const char *val;
  
  if (data_type(data)!=DATATYPE_REAL) {
    message_error("Taking real value of non-real node.");
    return 0;
  }

  val = xmlGetProp(data, "val");

  return atof(val);
}

int
data_boolean(DataNode data)
{
  const char *val;
  
  if (data_type(data)!=DATATYPE_BOOLEAN) {
    message_error("Taking boolean value of non-boolean node.");
    return 0;
  }

  val = xmlGetProp(data, "val");

  if (strcmp(val, "true")==0) 
    return TRUE;
  else 
    return FALSE;
}

static int hex_digit(char c)
{
  if ((c>='0') && (c<='9'))
    return c-'0';
  if ((c>='a') && (c<='f'))
    return (c-'a') + 10;
  if ((c>='A') && (c<='F'))
    return (c-'A') + 10;
  message_error("wrong hex digit!");
  return 0;
}
void
data_color(DataNode data, Color *col)
{
  const char *val;
  int r,g,b;
  
  if (data_type(data)!=DATATYPE_COLOR) {
    message_error("Taking color value of non-color node.");
    return;
  }

  val = xmlGetProp(data, "val");

  /* Format #RRGGBB */
  /*        0123456 */
	    
  r = hex_digit(val[1])*16 + hex_digit(val[2]);
  g = hex_digit(val[3])*16 + hex_digit(val[4]);
  b = hex_digit(val[5])*16 + hex_digit(val[6]);

  col->red = ((float)r)/255.0;
  col->green = ((float)g)/255.0;
  col->blue = ((float)b)/255.0;
}

void
data_point(DataNode data, Point *point)
{
  const char *val;
  char *str;
  
  if (data_type(data)!=DATATYPE_POINT) {
    message_error("Taking point value of non-point node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  point->x = strtod(val, &str);
  while ((*str != ',') && (*str!=0))
    str++;
  if (*str==0){
    point->y = 0.0;
    message_error("Error parsing point.");
    return;
  }
    
  point->y = strtod(str+1, NULL);
}

void
data_rectangle(DataNode data, Rectangle *rect)
{
  const char *val;
  char *str;
  
  if (data_type(data)!=DATATYPE_RECTANGLE) {
    message_error("Taking rectangle value of non-rectangle node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  
  rect->left = strtod(val, &str);
  
  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    return;
  }
    
  rect->top = strtod(str+1, &str);

  while ((*str != ';') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    return;
  }

  rect->right = strtod(str+1, &str);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    return;
  }

  rect->bottom = strtod(str+1, NULL);
}

char *
data_string(DataNode data)
{
  const char *val;
  char *str, *p;
  
  if (data_type(data)!=DATATYPE_STRING) {
    message_error("Taking string value of non-string node.");
    return NULL;
  }

  val = xmlGetProp(data, "val");

  if (val == NULL) {
    return NULL;
  }
  
  str  = g_malloc(sizeof(char)*(strlen(val)+1));

  p = str;
  while (*val) {
    if (*val == '\\') {
      val++;
      switch (*val) {
      case '0':
	/* Just skip this. \0 means nothing */
	break;
      case 'n':
	*p++ = '\n';
	break;
      case 't':
	*p++ = '\t';
	break;
      case '\\':
	*p++ = '\\';
	break;
      default:
	message_error("Error in string tag.");
      }
    } else {
      *p++ = *val;
    }
    val++;
  }
  *p = 0;
  return str;
}

Font *
data_font(DataNode data)
{
  const char *name;
  
  if (data_type(data)!=DATATYPE_FONT) {
    message_error("Taking font value of non-font node.");
    return NULL;
  }

  name = xmlGetProp(data, "name");
  return font_getfont(name);
}

AttributeNode
new_attribute(ObjectNode obj_node,
	      const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(obj_node, NULL, "attribute", NULL);
  xmlSetProp(attr, "name", attrname);

  return attr;
}

AttributeNode
composite_add_attribute(DataNode composite_node,
			const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(composite_node, NULL, "attribute", NULL);
  xmlSetProp(attr, "name", attrname);

  return attr;
}

void
data_add_int(AttributeNode attr, int data)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "int", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_enum(AttributeNode attr, int data)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "enum", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_real(AttributeNode attr, real data)
{
  DataNode data_node;
  char buffer[40+1]; /* Large enought? */

  snprintf(buffer, 40, "%g", data);
  
  data_node = xmlNewChild(attr, NULL, "real", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_boolean(AttributeNode attr, int data)
{
  DataNode data_node;

  data_node = xmlNewChild(attr, NULL, "boolean", NULL);
  if (data)
    xmlSetProp(data_node, "val", "true");
  else
    xmlSetProp(data_node, "val", "false");
}


static void convert_to_hex(float x, char *str)
{
  static const char hex_digit[] = "0123456789abcdef";
  int val;

  val = x * 255.0;
  if (val>255)
    val = 255;
  if (val<0)
    val = 0;

  str[0] = hex_digit[val/16];
  str[1] = hex_digit[val%16];
}

void
data_add_color(AttributeNode attr, Color *col)
{
  char buffer[1+6+1];
  DataNode data_node;

  buffer[0] = '#';
  convert_to_hex(col->red, &buffer[1]);
  convert_to_hex(col->green, &buffer[3]);
  convert_to_hex(col->blue, &buffer[5]);
  buffer[7] = 0;

  data_node = xmlNewChild(attr, NULL, "color", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_point(AttributeNode attr, Point *point)
{
  DataNode data_node;
  char buffer[80+1]; /* Large enought? */

  snprintf(buffer, 80, "%g,%g", point->x, point->y);
  
  data_node = xmlNewChild(attr, NULL, "point", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_rectangle(AttributeNode attr, Rectangle *rect)
{
  DataNode data_node;
  char buffer[160+1]; /* Large enought? */

  snprintf(buffer, 160, "%g,%g;%g,%g",
	   rect->left, rect->top,
	   rect->right, rect->bottom);
  
  data_node = xmlNewChild(attr, NULL, "rectangle", NULL);
  xmlSetProp(data_node, "val", buffer);

}

void
data_add_string(AttributeNode attr, char *str)
{
  DataNode data_node;
  char *str2, *p;

  data_node = xmlNewChild(attr, NULL, "string", NULL);
  
  if (str==NULL) {
    /* No val if NULL */
  } else {
    str2 = g_malloc(strlen(str)*2+1);

    p = str2;
    while (*str) {
      switch (*str) {
      case '\\': 
	*p++ = '\\';
	*p++ = '\\';
	break;
      case '\n':
	*p++ = '\\';
	*p++ = 'n';
	break;
      case '\t':
	*p++ = '\\';
	*p++ = 't';
	break;
      default:
	*p++ = *str;
      }
      str++;
    }
    *p = 0;

    xmlSetProp(data_node, "val", str2);

    g_free(str2);
  }
}

void
data_add_font(AttributeNode attr, Font *font)
{
  DataNode data_node;
 
  data_node = xmlNewChild(attr, NULL, "font", NULL);
  xmlSetProp(data_node, "name", font->name);
}

DataNode
data_add_composite(AttributeNode attr, char *type) 
{
  /* type can be NULL */
  DataNode data_node;
 
  data_node = xmlNewChild(attr, NULL, "composite", NULL);
  if (type != NULL) 
    xmlSetProp(data_node, "type", type);

  return data_node;
}


