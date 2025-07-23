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
/** \file dia_xml.c  Helper function to convert Dia's basic to and from XML */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "dia_xml.h"
#include "message.h"


/*!
 * \brief Find a named attribute node in an XML object node.
 *
 * Note that Dia has a concept of attribute node that is not the same
 * as an XML attribute.
 *
 * @param obj_node The node to look in.
 * @param attrname The name of the attribute node to find.
 * @return The node matching the given name, or NULL if none found.
 * \ingroup DiagramXmlIn
 */
AttributeNode
object_find_attribute(ObjectNode obj_node,
		      const char *attrname)
{
  AttributeNode attr;
  xmlChar *name;

  while (obj_node && xmlIsBlankNode(obj_node))
    obj_node = obj_node->next;
  if (!obj_node) return NULL;

  attr =  obj_node->xmlChildrenNode;
  while (attr != NULL) {
    if (xmlIsBlankNode(attr)) {
      attr = attr->next;
      continue;
    }

    name = xmlGetProp(attr, (const xmlChar *)"name");
    if ( (name!=NULL) && (strcmp((char *) name, attrname)==0) ) {
      xmlFree(name);
      return attr;
    }
    if (name) xmlFree(name);

    attr = attr->next;
  }
  return NULL;
}

/*!
 * \brief Find an attribute in a composite XML node.
 * @param composite_node The composite node to search.
 * @param attrname The name of the attribute node to find.
 * @return The desired node, or NULL if none exists in `composite_node'.
 * \ingroup DiagramXmlIn
 */
AttributeNode
composite_find_attribute(DataNode composite_node,
			 const char *attrname)
{
  AttributeNode attr;
  xmlChar *name;

  while (composite_node && xmlIsBlankNode(composite_node))
    composite_node = composite_node->next;
  if (!composite_node) return NULL;

  attr =  composite_node->xmlChildrenNode;
  while (attr != NULL) {
    if (xmlIsBlankNode(attr)) {
      attr = attr->next;
      continue;
    }

    name = xmlGetProp(attr, (const xmlChar *)"name");
    if ( (name!=NULL) && (strcmp((char *) name, attrname)==0) ) {
      xmlFree(name);
      return attr;
    }
    if (name) xmlFree(name);

    attr = attr->next;
  }
  return NULL;
}

/*!
 * \brief Count the number of non-blank data nodes in an attribute node.
 * @param attribute The attribute node to read from.
 * @return The number of non-blank data nodes in the node.
 * \ingroup DiagramXmlIn
 */
int
attribute_num_data(AttributeNode attribute)
{
  xmlNode *data;
  int nr=0;

  data =  attribute ? attribute->xmlChildrenNode : NULL;
  while (data != NULL) {
    if (xmlIsBlankNode(data)) {
      data = data->next;
      continue;
    }
    nr++;
    data = data->next;
  }
  return nr;
}

/*!
 * \brief Get the first data node in an attribute node.
 * @param attribute The attribute node to look through.
 * @return The first non-black data node in the attribute node.
 * \ingroup DiagramXmlIn
 */
DataNode
attribute_first_data(AttributeNode attribute)
{
  xmlNode *data = attribute ? attribute->xmlChildrenNode : NULL;
  while (data && xmlIsBlankNode(data)) data = data->next;
  return (DataNode) data;
}

/*!
 * \brief Get the next data node (sibling).
 * @param data A data node to start from (e.g. just processed)
 * @return The next sibling data node.
 * \ingroup DiagramXmlIn
 */
DataNode
data_next(DataNode data)
{

  if (data) {
    data = data->next;
    while (data && xmlIsBlankNode(data)) data = data->next;
  }
  return (DataNode) data;
}

/*!
 * \brief Get the type of a data node.
 * @param data The data node.
 * @param ctx The context in which this function is called
 * @return The type that the data node defines, or 0 on error.
 * @note This function does a number of strcmp calls, which may not be the
 *  fastest way to check if a node is of the expected type.
 * \ingroup DiagramXmlIn
 */
DataType
data_type(DataNode data, DiaContext *ctx)
{
  const char *name;

  name = data ? (const char *)data->name : (const char *)"";
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
  } else if (strcmp(name, "bezpoint")==0) {
    return DATATYPE_BEZPOINT;
  } else if (strcmp(name, "dict")==0) {
    return DATATYPE_DICT;
  } else if (strcmp(name, "pixbuf")==0) {
    return DATATYPE_PIXBUF;
  }

  dia_context_add_message (ctx, _("Unknown type of DataNode '%s'"), name);
  return 0;
}

/*!
 * \brief Return the value of an integer-type data node.
 * @param data The data node to read from.
 * @param ctx The context in which this function is called
 * @return The integer value found in the node.  If the node is not an
 *  integer node, an error message is displayed and 0 is returned.
 * \ingroup DiagramXmlIn
 */
int
data_int(DataNode data, DiaContext *ctx)
{
  xmlChar *val;
  int res;

  if (data_type(data, ctx)!=DATATYPE_INT) {
    dia_context_add_message (ctx, _("Taking int value of non-int node."));
    return 0;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");
  res = atoi((char *) val);
  if (val) xmlFree(val);

  return res;
}

/*!
 * \brief Return the value of an enum-type data node.
 * @param data The data node to read from.
 * @param ctx The context in which this function is called
 * @return The enum value found in the node.  If the node is not an
 *  enum node, an error message is displayed and 0 is returned.
 * \ingroup DiagramXmlIn
 */
int
data_enum(DataNode data, DiaContext *ctx)
{
  xmlChar *val;
  int res;

  if (data_type(data, ctx)!=DATATYPE_ENUM) {
    dia_context_add_message (ctx, "Taking enum value of non-enum node.");
    return 0;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");
  res = atoi((char *) val);
  if (val) xmlFree(val);

  return res;
}

/*!
 * \brief Return the value of a real-type data node.
 * @param data The data node to read from.
 * @param ctx The context in which this function is called
 * @return The real value found in the node.  If the node is not a
 *  real-type node, an error message is displayed and 0.0 is returned.
 * \ingroup DiagramXmlIn
 */
real
data_real(DataNode data, DiaContext *ctx)
{
  xmlChar *val;
  real res;

  if (data_type(data, ctx)!=DATATYPE_REAL) {
    dia_context_add_message (ctx, "Taking real value of non-real node.");
    return 0;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");
  res = g_ascii_strtod((char *) val, NULL);
  if (val) xmlFree(val);

  return res;
}

/*!
 * \brief Return the value of a boolean-type data node.
 * @param data The data node to read from.
 * @param ctx The context in which this function is called
 * @return The boolean value found in the node.  If the node is not a
 *  boolean node, an error message is displayed and FALSE is returned.
 * \ingroup DiagramXmlIn
 */
int
data_boolean(DataNode data, DiaContext *ctx)
{
  xmlChar *val;
  int res;

  if (data_type(data, ctx)!=DATATYPE_BOOLEAN) {
    dia_context_add_message (ctx, "Taking boolean value of non-boolean node.");
    return 0;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");

  if ((val) && (strcmp((char *) val, "true")==0))
    res =  TRUE;
  else
    res = FALSE;

  if (val) xmlFree(val);

  return res;
}

/*!
 * \brief Return the integer value of a hex digit.
 * @param c A hex digit, one of 0-9, a-f or A-F.
 * @param ctx The context in which this function is called
 * @return The value of the digit, i.e. 0-15.  If a non-gex digit is given
 *  an error is registered in ctx, and 0 is returned.
 * \ingroup DiagramXmlIn
 */
static int
hex_digit(char c, DiaContext *ctx)
{
  if ((c>='0') && (c<='9'))
    return c-'0';
  if ((c>='a') && (c<='f'))
    return (c-'a') + 10;
  if ((c>='A') && (c<='F'))
    return (c-'A') + 10;
  dia_context_add_message (ctx, "wrong hex digit %c", c);
  return 0;
}

/*!
 * \brief Return the value of a color-type data node.
 * @param data The XML node to read from
 * @param col A place to store the resulting RGBA values.  If the node does
 *  not contain a valid color value, an error message is registered in ctx
 *  and `col' is unchanged.
 * @param ctx The context in which this function is called
 * \ingroup DiagramXmlIn
 */
void
data_color(DataNode data, Color *col, DiaContext *ctx)
{
  xmlChar *val;
  int r=0, g=0, b=0, a=0;

  if (data_type(data, ctx)!=DATATYPE_COLOR) {
    dia_context_add_message (ctx, "Taking color value of non-color node.");
    return;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");

  /* Format #RRGGBB */
  /*        0123456 */

  if ((val) && (xmlStrlen(val)>=7)) {
    r = hex_digit(val[1], ctx)*16 + hex_digit(val[2], ctx);
    g = hex_digit(val[3], ctx)*16 + hex_digit(val[4], ctx);
    b = hex_digit(val[5], ctx)*16 + hex_digit(val[6], ctx);
    if (xmlStrlen(val) >= 9) {
      a = hex_digit(val[7], ctx)*16 + hex_digit(val[8], ctx);
    } else {
      a = 0xff;
    }
  }

  if (val) xmlFree(val);

  col->red = (float)(r/255.0);
  col->green = (float)(g/255.0);
  col->blue = (float)(b/255.0);
  col->alpha = (float)(a/255.0);
}

/*!
 * \brief Return the value of a point-type data node.
 * @param data The XML node to read from
 * @param point A place to store the resulting x, y values.  If the node does
 *  not contain a valid point value, an error message is registered in ctx
 *  and `point' is unchanged.
 * @param ctx The context in which this function is called
 * \ingroup DiagramXmlIn
 */
void
data_point(DataNode data, Point *point, DiaContext *ctx)
{
  xmlChar *val;
  gchar *str;
  real ax,ay;

  if (data_type(data, ctx)!=DATATYPE_POINT) {
    dia_context_add_message (ctx, _("Taking point value of non-point node."));
    return;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");
  point->x = g_ascii_strtod((char *)val, &str);
  ax = fabs(point->x);
  if ((ax > 1e9) || ((ax < 1e-9) && (ax != 0.0)) || isnan(ax) || isinf(ax)) {
    /* there is no provision to keep values larger when saving,
     * so do this 'reduction' silent */
    if (!(ax < 1e-9))
      g_warning(_("Incorrect x Point value \"%s\" %f; discarding it."),val,point->x);
    point->x = 0.0;
  }
  while ((*str != ',') && (*str!=0))
    str++;
  if (*str==0){
    point->y = 0.0;
    g_warning(_("Error parsing point."));
    xmlFree(val);
    return;
  }
  point->y = g_ascii_strtod(str+1, NULL);
  ay = fabs(point->y);
  if ((ay > 1e9) || ((ay < 1e-9) && (ay != 0.0)) || isnan(ay) || isinf(ay)) {
    if (!(ay < 1e-9)) /* don't bother with useless warnings (see above) */
      g_warning(_("Incorrect y Point value \"%s\" %f; discarding it."),str+1,point->y);
    point->y = 0.0;
  }
  xmlFree(val);
}

/*!
 * \brief Return the value of a bezpoint-type data node.
 * @param data The XML node to read from
 * @param point A place to store the resulting values.  If the node does
 *  not contain a valid bezpoint zero initialization is performed.
 * @param ctx The context in which this function is called
 * \ingroup DiagramXmlIn
 */
void
data_bezpoint(DataNode data, BezPoint *point, DiaContext *ctx)
{
  xmlChar *val;
  gchar *str;
  if (data_type(data, ctx)!=DATATYPE_BEZPOINT) {
    dia_context_add_message (ctx, _("Taking bezpoint value of non-point node."));
    return;
  }
  val = xmlGetProp(data, (const xmlChar *)"type");
  if (val) {
     if (strcmp((char *)val, "moveto") == 0)
       point->type = BEZ_MOVE_TO;
     else if (strcmp((char *)val, "lineto") == 0)
       point->type = BEZ_LINE_TO;
     else
       point->type = BEZ_CURVE_TO;
    xmlFree(val);
  }
  val = xmlGetProp(data, (const xmlChar *)"p1");
  if (val) {
    point->p1.x = g_ascii_strtod((char *)val, &str);
    if (*str==0) {
      point->p1.y = 0;
      g_warning(_("Error parsing bezpoint p1."));
    } else {
      point->p1.y = g_ascii_strtod(str+1, NULL);
    }
    xmlFree(val);
  } else {
    point->p1.x = 0;
    point->p1.y = 0;
  }
  val = xmlGetProp(data, (const xmlChar *)"p2");
  if (val) {
    point->p2.x = g_ascii_strtod((char *)val, &str);
    if (*str==0) {
      point->p2.y = 0;
      g_warning(_("Error parsing bezpoint p2."));
    } else {
      point->p2.y = g_ascii_strtod(str+1, NULL);
    }
    xmlFree(val);
  } else {
    point->p2.x = 0;
    point->p2.y = 0;
  }
  val = xmlGetProp(data, (const xmlChar *)"p3");
  if (val) {
    point->p3.x = g_ascii_strtod((char *)val, &str);
    if (*str==0) {
      point->p3.y = 0;
      g_warning(_("Error parsing bezpoint p3."));
    } else {
      point->p3.y = g_ascii_strtod(str+1, NULL);
    }
    xmlFree(val);
  } else {
    point->p3.x = 0;
    point->p3.y = 0;
  }
}

/*!
 * Return the value of a rectangle-type data node.
 * @param data The data node to read from.
 * @param rect A place to store the resulting values.  If the node does
 *  not contain a valid rectangle value, an error message is displayed to the
 *  user, and `rect' is unchanged.
 * @param ctx The context in which this function is called
 * \ingroup DiagramXmlIn
 */
void
data_rectangle(DataNode data, DiaRectangle *rect, DiaContext *ctx)
{
  xmlChar *val;
  gchar *str;

  if (data_type(data, ctx)!=DATATYPE_RECTANGLE) {
    dia_context_add_message (ctx, _("Taking rectangle value of non-rectangle node."));
    return;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");

  rect->left = g_ascii_strtod((char *)val, &str);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    dia_context_add_message (ctx, _("Error parsing rectangle."));
    xmlFree(val);
    return;
  }

  rect->top = g_ascii_strtod(str+1, &str);

  while ((*str != ';') && (*str!=0))
    str++;

  if (*str==0){
    dia_context_add_message (ctx, _("Error parsing rectangle."));
    xmlFree(val);
    return;
  }

  rect->right = g_ascii_strtod(str+1, &str);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    dia_context_add_message (ctx, _("Error parsing rectangle."));
    xmlFree(val);
    return;
  }

  rect->bottom = g_ascii_strtod(str+1, NULL);

  xmlFree(val);
}

/*!
 * \brief Return the value of a string-type data node.
 * @param data The data node to read from.
 * @return The string value found in the node.  If the node is not a
 *  string node, an error message is displayed and NULL is returned.  The
 *  returned valuee should be freed after use.
 * @note For historical reasons, strings in Dia XML are surrounded by ##.
 * @param ctx The context in which this function is called
 * \ingroup DiagramXmlIn
 */
gchar *
data_string(DataNode data, DiaContext *ctx)
{
  xmlChar *val;
  gchar *str, *p,*str2;
  int len;

  if (data_type(data, ctx)!=DATATYPE_STRING) {
    dia_context_add_message (ctx, _("Taking string value of non-string node."));
    return NULL;
  }

  val = xmlGetProp(data, (const xmlChar *)"val");
  if (val != NULL) {
    /* Old kind of string. Left for backwards compatibility */
    /* TODO: This "extra room" feels like nonsense, especially when introduced
     * when this was already legacy */
    str  = g_new0 (char, 4 * (xmlStrlen (val) + 1)); /* extra room for UTF8 */
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
            dia_context_add_message (ctx, _("Error in string tag."));
        }
      } else {
	*p++ = *val;
      }
      val++;
    }
    *p = 0;
    xmlFree(val);
    str2 = g_strdup(str);  /* to remove the extra space */
    g_clear_pointer (&str, g_free);
    return str2;
  }

  if (data->xmlChildrenNode!=NULL) {
    p = (char *)xmlNodeListGetString(data->doc, data->xmlChildrenNode, TRUE);

    if (*p!='#')
      dia_context_add_message (ctx, _("Error in file, string not starting with #"));

    len = strlen(p)-1; /* Ignore first '#' */

    str = g_new0 (char, len + 1);

    strncpy(str, p+1, len);
    str[len]=0; /* For safety */

    str[strlen(str)-1] = 0; /* Remove last '#' */
    xmlFree(p);
    return str;
  }

  return NULL;
}


/*!
 * \brief Return the value of a filename-type data node.
 * @param data The data node to read from.
 * @param ctx The context in which this function is called
 * @return The filename value found in the node.  If the node is not a
 *  filename node, an error message is added to ctx and NULL is returned.
 *  The resulting string is in the local filesystem's encoding rather than
 *  UTF-8, and should be freed after use.
 * \ingroup DiagramXmlIn
 */
char *
data_filename(DataNode data, DiaContext *ctx)
{
  char *utf8 = data_string(data, ctx);
  char *filename = NULL;

  if (utf8) {
    GError *error = NULL;
    if ((filename = g_filename_from_utf8 (utf8, -1, NULL, NULL, &error)) == NULL) {
      dia_context_add_message (ctx, "%s", error->message);
      g_clear_error (&error);
    }
    g_clear_pointer (&utf8, g_free);
  }

  return filename;
}


/**
 * data_font:
 * @data: The data node to read from.
 * @ctx: The context in which this function is called
 *
 * This handles both the current format (family and style) and the old
 * format (name).
 *
 * Returns: The font value found in the node. If the node is not a font node,
 *          an error message is registered and %NULL is returned. The resulting
 *          value should be freed after use.
 */
DiaFont *
data_font (DataNode data, DiaContext *ctx)
{
  xmlChar *family;
  DiaFont *font;

  if (data_type (data, ctx) != DATATYPE_FONT) {
    dia_context_add_message (ctx, _("Taking font value of non-font node."));
    return NULL;
  }

  family = xmlGetProp (data, (const xmlChar *) "family");
  /* always prefer the new format */
  if (family) {
    DiaFontStyle style;
    char *style_name = (char *) xmlGetProp (data, (const xmlChar *) "style");
    style = style_name ? atoi (style_name) : 0;

    font = dia_font_new ((char *) family, style, 1.0);

    dia_clear_xml_string (&family);
    dia_clear_xml_string (&style_name);
  } else {
    /* Legacy format support */
    char *name = (char *) xmlGetProp (data, (const xmlChar *) "name");

    font = dia_font_new_from_legacy_name (name);

    dia_clear_xml_string (&name);
  }

  return font;
}

/* ***** Saving XML **** */

/*!
 * \brief Create a new attribute node.
 * @param obj_node The object node to create the attribute node under.
 * @param attrname The name of the attribute node.
 * @return A new attribute node.
 * \ingroup DiagramXmlOut
 */
AttributeNode
new_attribute(ObjectNode obj_node,
	      const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(obj_node, NULL, (const xmlChar *)"attribute", NULL);
  xmlSetProp(attr, (const xmlChar *)"name", (xmlChar *)attrname);

  return attr;
}

/*!
 * \brief Add an attribute node to a composite node.
 * @param composite_node The composite node.
 * @param attrname The name of the new attribute node.
 * @return The attribute node added.
 * \ingroup DiagramXmlOut
 */
AttributeNode
composite_add_attribute(DataNode composite_node,
			const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(composite_node, NULL, (const xmlChar *)"attribute", NULL);
  xmlSetProp(attr, (const xmlChar *)"name", (xmlChar *)attrname);

  return attr;
}

/*!
 * \brief Add integer data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_int(AttributeNode attr, int data, DiaContext *ctx)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  g_snprintf(buffer, 20, "%d", data);

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"int", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);
}

/*!
 * \brief Add enum data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_enum(AttributeNode attr, int data, DiaContext *ctx)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  g_snprintf(buffer, 20, "%d", data);

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"enum", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);
}

/*!
 * \brief Add real-typed data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_real(AttributeNode attr, real data, DiaContext *ctx)
{
  DataNode data_node;
  char buffer[G_ASCII_DTOSTR_BUF_SIZE]; /* Large enought */

  g_ascii_dtostr(buffer, G_ASCII_DTOSTR_BUF_SIZE, data);

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"real", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);
}

/*!
 * \brief Add boolean data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_boolean(AttributeNode attr, int data, DiaContext *ctx)
{
  DataNode data_node;

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"boolean", NULL);
  if (data)
    xmlSetProp(data_node, (const xmlChar *)"val", (const xmlChar *)"true");
  else
    xmlSetProp(data_node, (const xmlChar *)"val", (const xmlChar *)"false");
}

/*!
 * \brief Convert a floating-point value to hexadecimal.
 * @param x The floating point value.
 * @param str A string to place the result in.
 * @note Currently only works for 0 <= x <= 255 and will silently cap the value
 *  to those limits.  Also expects str to have at least two bytes allocated,
 *  and doesn't null-terminate it.  This works well for converting a color
 *  value, but is pretty much useless for other values.
 * \ingroup DiagramXmlOut
 */
static void
convert_to_hex(float x, char *str)
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

/*!
 * \brief Add color data to an attribute node.
 * @param attr The attribute node.
 * @param col The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_color(AttributeNode attr, const Color *col, DiaContext *ctx)
{
  char buffer[1+8+1];
  DataNode data_node;

  buffer[0] = '#';
  convert_to_hex(col->red, &buffer[1]);
  convert_to_hex(col->green, &buffer[3]);
  convert_to_hex(col->blue, &buffer[5]);
  convert_to_hex(col->alpha, &buffer[7]);
  buffer[9] = 0;

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"color", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);
}

static gchar *
_str_point (const Point *point)
{
  gchar *buffer;
  gchar px_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar py_buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd(px_buf, sizeof(px_buf), "%g", point->x);
  g_ascii_formatd(py_buf, sizeof(py_buf), "%g", point->y);
  buffer = g_strconcat(px_buf, ",", py_buf, NULL);

  return buffer;
}

/*!
 * \brief Add point data to an attribute node.
 * @param attr The attribute node.
 * @param point The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_point(AttributeNode attr, const Point *point, DiaContext *ctx)
{
  DataNode data_node;
  gchar *buffer = _str_point (point);

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"point", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);
  g_clear_pointer (&buffer, g_free);
}

/*!
 * \brief Saving _BezPoint in a single node
 * \ingroup DiagramXmlOut
 */
void
data_add_bezpoint(AttributeNode attr, const BezPoint *point, DiaContext *ctx)
{
  DataNode data_node;
  gchar *buffer;

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"bezpoint", NULL);
  switch (point->type) {
  case BEZ_MOVE_TO :
    xmlSetProp(data_node, (const xmlChar *)"type", (const xmlChar *)"moveto");
    break;
  case BEZ_LINE_TO :
    xmlSetProp(data_node, (const xmlChar *)"type", (const xmlChar *)"lineto");
    break;
  case BEZ_CURVE_TO :
    xmlSetProp(data_node, (const xmlChar *)"type", (const xmlChar *)"curveto");
    break;
  default :
    g_assert_not_reached();
  }

  buffer = _str_point (&point->p1);
  xmlSetProp(data_node, (const xmlChar *)"p1", (xmlChar *)buffer);
  g_clear_pointer (&buffer, g_free);
  if (point->type == BEZ_CURVE_TO) {
    buffer = _str_point (&point->p2);
    xmlSetProp(data_node, (const xmlChar *)"p2", (xmlChar *)buffer);
    g_clear_pointer (&buffer, g_free);
    buffer = _str_point (&point->p3);
    xmlSetProp(data_node, (const xmlChar *)"p3", (xmlChar *)buffer);
    g_clear_pointer (&buffer, g_free);
  }
}

/*!
 * \brief Add rectangle data to an attribute node.
 * @param attr The attribute node.
 * @param rect The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_rectangle(AttributeNode attr, const DiaRectangle *rect, DiaContext *ctx)
{
  DataNode data_node;
  gchar *buffer;
  gchar rl_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar rr_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar rt_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar rb_buf[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd(rl_buf, sizeof(rl_buf), "%g", rect->left);
  g_ascii_formatd(rr_buf, sizeof(rr_buf), "%g", rect->right);
  g_ascii_formatd(rt_buf, sizeof(rt_buf), "%g", rect->top);
  g_ascii_formatd(rb_buf, sizeof(rb_buf), "%g", rect->bottom);

  buffer = g_strconcat(rl_buf, ",", rt_buf, ";", rr_buf, ",", rb_buf, NULL);

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"rectangle", NULL);
  xmlSetProp(data_node, (const xmlChar *)"val", (xmlChar *)buffer);

  g_clear_pointer (&buffer, g_free);
}

/*!
 * \brief Add string data to an attribute node.
 * @param attr The attribute node.
 * @param str The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_string(AttributeNode attr, const char *str, DiaContext *ctx)
{
    xmlChar *escaped_str;
    xmlChar *sharped_str;

    if (str==NULL) {
        (void)xmlNewChild(attr, NULL, (const xmlChar *)"string", (const xmlChar *)"##");
        return;
    }

    escaped_str = xmlEncodeEntitiesReentrant(attr->doc, (xmlChar *) str);

    sharped_str = (xmlChar *) g_strconcat("#", (char *) escaped_str, "#", NULL);

    xmlFree(escaped_str);

    (void)xmlNewChild(attr, NULL, (const xmlChar *)"string", (xmlChar *) sharped_str);

    g_clear_pointer (&sharped_str, g_free);
}

/*!
 * \brief Add filename data to an attribute node.
 * @param data The data node.
 * @param str The filename value to set. This should be n the local filesystem
 *  encoding, not utf-8.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_filename(DataNode data, const char *str, DiaContext *ctx)
{
  char *utf8 = g_filename_to_utf8(str, -1, NULL, NULL, NULL);

  data_add_string(data, utf8, ctx);

  g_clear_pointer (&utf8, g_free);
}

/*!
 * \brief Add font data to an attribute node.
 * @param attr The attribute node.
 * @param font The value to set.
 * @param ctx The context to transport error information.
 * \ingroup DiagramXmlOut
 */
void
data_add_font(AttributeNode attr, DiaFont *font, DiaContext *ctx)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"font", NULL);
  xmlSetProp(data_node, (const xmlChar *)"family", (xmlChar *) dia_font_get_family(font));
  g_snprintf(buffer, 20, "%d", dia_font_get_style(font));

  xmlSetProp(data_node, (const xmlChar *)"style", (xmlChar *) buffer);
  /* Legacy support: don't crash older Dia on missing 'name' attribute */
  xmlSetProp(data_node, (const xmlChar *)"name", (xmlChar *) dia_font_get_legacy_name(font));
}

/*!
 * \brief Add a new composite node to an attribute node.
 * @param attr The attribute node to add to.
 * @param type The type of the new node.
 * @param ctx The context to transport error information.
 * @return The new child of `attr'.
 * \ingroup DiagramXmlOut
 */
DataNode
data_add_composite(AttributeNode attr, const char *type, DiaContext *ctx)
{
  /* type can be NULL */
  DataNode data_node;

  data_node = xmlNewChild(attr, NULL, (const xmlChar *)"composite", NULL);
  if (type != NULL)
    xmlSetProp(data_node, (const xmlChar *)"type", (xmlChar *)type);

  return data_node;
}
