/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998,1999 Alexander Larsson
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
#include <string.h>
#include <locale.h>

#include <tree.h>

#include "utils.h"
#include "dia_xml.h"
#include "message.h"

AttributeNode
object_find_attribute(ObjectNode obj_node,
		      const char *attrname)
{
  AttributeNode attr;
  char *name;

  attr =  obj_node->childs;
  while (attr != NULL) {

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) ) {
      free(name);
      return attr;
    }
    if (name) free(name);
    
    attr = attr->next;
  }
  return NULL;
}

AttributeNode
composite_find_attribute(DataNode composite_node,
			 const char *attrname)
{
  AttributeNode attr;
  char *name;

  attr =  composite_node->childs;
  while (attr != NULL) {

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) ) {
      free(name);
      return attr;
    }
    if (name) free(name);
    
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
  char *val;
  int res;
  
  if (data_type(data)!=DATATYPE_INT) {
    message_error("Taking int value of non-int node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = atoi(val);
  if (val) free(val);
  
  return res;
}

int data_enum(DataNode data)
{
  char *val;
  int res;
  
  if (data_type(data)!=DATATYPE_ENUM) {
    message_error("Taking enum value of non-enum node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = atoi(val);
  if (val) free(val);
  
  return res;
}

real
data_real(DataNode data)
{
  char *val;
  real res;
  
  if (data_type(data)!=DATATYPE_REAL) {
    message_error("Taking real value of non-real node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = g_strtod(val, NULL);
  if (val) free(val);
  
  return res;
}

int
data_boolean(DataNode data)
{
  char *val;
  int res;
  
  if (data_type(data)!=DATATYPE_BOOLEAN) {
    message_error("Taking boolean value of non-boolean node.");
    return 0;
  }

  val = xmlGetProp(data, "val");

  if ((val) && (strcmp(val, "true")==0))
    res =  TRUE;
  else 
    res = FALSE;

  if (val) free(val);

  return res;
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
  char *val;
  int r=0, g=0, b=0;
  
  if (data_type(data)!=DATATYPE_COLOR) {
    message_error("Taking color value of non-color node.");
    return;
  }

  val = xmlGetProp(data, "val");

  /* Format #RRGGBB */
  /*        0123456 */

  if ((val) && (strlen(val)>=7)) {
    r = hex_digit(val[1])*16 + hex_digit(val[2]);
    g = hex_digit(val[3])*16 + hex_digit(val[4]);
    b = hex_digit(val[5])*16 + hex_digit(val[6]);
  }

  if (val) free(val);
  
  col->red = ((float)r)/255.0;
  col->green = ((float)g)/255.0;
  col->blue = ((float)b)/255.0;
}

void
data_point(DataNode data, Point *point)
{
  char *val;
  char *str;
  
  if (data_type(data)!=DATATYPE_POINT) {
    message_error("Taking point value of non-point node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  point->x = g_strtod(val, &str);
  while ((*str != ',') && (*str!=0))
    str++;
  if (*str==0){
    point->y = 0.0;
    message_error("Error parsing point.");
    free(val);
    return;
  }
    
  point->y = g_strtod(str+1, NULL);
  free(val);
}

void
data_rectangle(DataNode data, Rectangle *rect)
{
  char *val;
  char *str;
  
  if (data_type(data)!=DATATYPE_RECTANGLE) {
    message_error("Taking rectangle value of non-rectangle node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  
  rect->left = g_strtod(val, &str);
  
  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    free(val);
    return;
  }
    
  rect->top = g_strtod(str+1, &str);

  while ((*str != ';') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    free(val);
    return;
  }

  rect->right = g_strtod(str+1, &str);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    free(val);
    return;
  }

  rect->bottom = g_strtod(str+1, NULL);
  
  free(val);
}

char *
data_string(DataNode data)
{
  char *val;
  char *str, *p;
  int len;
  
  if (data_type(data)!=DATATYPE_STRING) {
    message_error("Taking string value of non-string node.");
    return NULL;
  }

  val = xmlGetProp(data, "val");
  if (val != NULL) { /* Old kind of string. Left for backwards compatibility */
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
    free(val);
    return str;
  }

  if (data->childs!=NULL) {
    p = xmlNodeListGetString(data->doc, data->childs, TRUE);

    if (*p!='#')
      message_error("Error in file, string not starting with #\n");
    
    len = strlen(p)-1; /* Ignore first '#' */
      
    str = g_malloc(len+1);

    strncpy(str, p+1, len);
    str[len]=0; /* For safety */

    str[strlen(str)-1] = 0; /* Remove last '#' */
    
    free(p);

    return str;
  }
    
  return NULL;
}

Font *
data_font(DataNode data)
{
  char *name;
  Font *font;
  
  if (data_type(data)!=DATATYPE_FONT) {
    message_error("Taking font value of non-font node.");
    return NULL;
  }

  name = xmlGetProp(data, "name");
  font = font_getfont(name);
  if (name) free(name);
  
  return font;
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

  g_snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "int", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_enum(AttributeNode attr, int data)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  g_snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "enum", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_real(AttributeNode attr, real data)
{
  DataNode data_node;
  char buffer[40+1]; /* Large enought? */
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buffer, 40, "%g", data);
  setlocale(LC_NUMERIC, old_locale);
  
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
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buffer, 80, "%g,%g", point->x, point->y);
  setlocale(LC_NUMERIC, old_locale);
  
  data_node = xmlNewChild(attr, NULL, "point", NULL);
  xmlSetProp(data_node, "val", buffer);
}

void
data_add_rectangle(AttributeNode attr, Rectangle *rect)
{
  DataNode data_node;
  char buffer[160+1]; /* Large enought? */
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buffer, 160, "%g,%g;%g,%g",
	     rect->left, rect->top,
	     rect->right, rect->bottom);
  setlocale(LC_NUMERIC, old_locale);
  
  data_node = xmlNewChild(attr, NULL, "rectangle", NULL);
  xmlSetProp(data_node, "val", buffer);

}

static int
escaped_str_len(char *str)
{
  int len;
  char c;
  
  if (str==NULL)
    return 0;
  
  len = 0;

  while ((c=*str++) != 0) {
    switch (c) {
    case '&':
      len += 5; /* "&amp;" */
      break;
    case '<':
    case '>':
      len += 4; /* "&lt;" or "&gt;" */
      break;
    default:
      len++;
    }
  }
  
  return len;
}

static void
escape_string(char *buffer, char *str)
{
  int len;
  char c;
  
  *buffer = 0;
  
  if (str==NULL) 
    return;
  
  len = 0;

  while ((c=*str++) != 0) {
    switch (c) {
    case '&':
      strcpy(buffer, "&amp;");
      buffer += 5;
      break;
    case '<':
      strcpy(buffer, "&lt;");
      buffer += 4;
      break;
    case '>':
      strcpy(buffer, "&gt;");
      buffer += 4;
     break;
    default:
      *buffer = c;
      buffer++;
    }
  }
  *buffer = 0;
}

void
data_add_string(AttributeNode attr, char *str)
{
  DataNode data_node;
  char *escaped_str;
  int len;

  if (str==NULL) {
    data_node = xmlNewChild(attr, NULL, "string", NULL);
  } else {
    len = 2+escaped_str_len(str);
    escaped_str = g_malloc(len+1);
    *escaped_str='#';
    escape_string(escaped_str+1, str);
    strcat(escaped_str, "#");
    
    data_node = xmlNewChild(attr, NULL, "string", escaped_str);
  
    g_free(escaped_str);
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


