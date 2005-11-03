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
#include <config.h>

#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>

#include <libxml/parser.h>
#include <libxml/parserInternals.h>
#include <libxml/xmlmemory.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <zlib.h>

#include "intl.h"
#include "utils.h"
#include "dia_xml_libxml.h"
#include "dia_xml.h"
#include "geometry.h"		/* For isinf() on Solaris */
#include "message.h"

#ifdef G_OS_WIN32
#include <io.h> /* write, close */
#endif

#ifdef G_OS_WIN32 /* apparently _MSC_VER and mingw */
#include <float.h>
#define isinf(a) (!_finite(a))
#endif

#define BUFLEN 1024

/** If all files produced by dia were good XML files, we wouldn't have to do 
 *  this little gymnastic. Alas, during the libxml1 days, we were outputting 
 *  files with no encoding specification (which means UTF-8 if we're in an
 *  asciish encoding) and strings encoded in local charset (so, we wrote
 *  broken files). 
 *
 *  The following logic finds if we have a broken file, and attempts to fix 
 *  it if it's possible. If the file is correct or is unrecognisable, we pass
 *  it untouched to libxml2.
 * @param filename The name of the file to check.
 * @param default_enc The default encoding to use if none is given.
 * @returns The filename given if it seems ok, or the name of a new file
 *          with fixed contents, or NULL if we couldn't read the file.  The
 *          caller should free this string and unlink the file if it is not
 *          the same as `filename'.
 * @bugs The many gzclose-g_free-return sequences should be refactored into
 *       an "exception handle" (goto+label).
 */
static const gchar *
xml_file_check_encoding(const gchar *filename, const gchar *default_enc)
{
  gzFile zf = gzopen(filename,"rb");  
  gchar *buf;
  gchar *p,*pmax;
  int len;
  gchar *tmp,*res;
  int uf;
  gboolean well_formed_utf8;

  static char magic_xml[] = 
  {0x3c,0x3f,0x78,0x6d,0x6c,0x00}; /* "<?xml" in ASCII */

  if (!zf) {
    /*    message_error(_("The file %s can not be opened for reading"),filename); */ 
    /* XXX perhaps we can just chicken out to libxml ? -- CC */ 
    return NULL;
  }
  p = buf = g_malloc0(BUFLEN);  
  len = gzread(zf,buf,BUFLEN);
  pmax = p + len;

  /* first, we expect the magic <?xml string */
  if ((0 != strncmp(p,magic_xml,5)) || (len < 5)) {
    gzclose(zf);
    g_free(buf);
    return filename; /* let libxml figure out what this is. */
  }
  /* now, we're sure we have some asciish XML file. */
  p += 5;
  while (((*p == 0x20)||(*p == 0x09)||(*p == 0x0d)||(*p == 0x0a))
         && (p<pmax)) p++;
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    g_free(buf);
    return filename;
  }
  if (0 != strncmp(p,"version=\"",9)) {
    gzclose(zf); /* chicken out. */
    g_free(buf);
    return filename;
  }
  p += 9;
  /* The header is rather well formed. */
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    g_free(buf);
    return filename;
  }
  while ((*p != '"') && (p < pmax)) p++;
  p++;
  while (((*p == 0x20)||(*p == 0x09)||(*p == 0x0d)||(*p == 0x0a))
         && (p<pmax)) p++;
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    g_free(buf);
    return filename;
  }
  if (0 == strncmp(p,"encoding=\"",10)) {
    gzclose(zf); /* this file has an encoding string. Good. */
    g_free(buf);
    return filename;
  }
  /* now let's read the whole file, to see if there are offending bits.
   * We can call it well formed UTF-8 if the highest isn't used
   */
  well_formed_utf8 = TRUE;
  do {
    int i;
    for (i = 0; i < len; i++)
      if (buf[i] & 0x80 || buf[i] == '&')
        well_formed_utf8 = FALSE;
    len = gzread(zf,buf,BUFLEN);
  } while (len > 0 && well_formed_utf8);
  if (well_formed_utf8) {
    gzclose(zf); /* this file is utf-8 compatible  */
    g_free(buf);
    return filename;
  } else {
    gzclose(zf); /* poor man's fseek */
    zf = gzopen(filename,"rb"); 
    len = gzread(zf,buf,BUFLEN);
  }

  if (0 != strcmp(default_enc,"UTF-8")) {
    message_warning(_("The file %s has no encoding specification;\n"
                      "assuming it is encoded in %s"),
		    dia_message_filename(filename), default_enc);
  } else {
    gzclose(zf); /* we apply the standard here. */
    g_free(buf);
    return filename;
  }

  tmp = getenv("TMP"); 
  if (!tmp) tmp = getenv("TEMP");
  if (!tmp) tmp = "/tmp";

  res = g_strconcat(tmp,G_DIR_SEPARATOR_S,"dia-xml-fix-encodingXXXXXX",NULL);
  uf = g_mkstemp(res);
  write(uf,buf,p-buf);
  write(uf," encoding=\"",11);
  write(uf,default_enc,strlen(default_enc));
  write(uf,"\" ",2);
  write(uf,p,pmax - p);

  while (1) {
    len = gzread(zf,buf,BUFLEN);
    if (len <= 0) break;
    write(uf,buf,len);
  }
  gzclose(zf);
  close(uf);
  g_free(buf);
  return res; /* caller frees the name and unlinks the file. */
}

/** Parse a given file into XML, handling old broken files correctly.
 * @param filename The name of the file to read.
 * @returns An XML document parsed from the file.
 * @see xmlParseFile() in the XML2 library for details on the return value.
 */
xmlDocPtr
xmlDiaParseFile(const char *filename)
{
  G_CONST_RETURN char *local_charset = NULL;
  
  if (   !g_get_charset(&local_charset)
      && local_charset) {
    /* we're not in an UTF-8 environment. */ 
    const gchar *fname = xml_file_check_encoding(filename,local_charset);
    if (fname != filename) {
      /* We've got a corrected file to parse. */
      xmlDocPtr ret = xmlDoParseFile(fname);
      unlink(fname);
      /* printf("has read %s instead of %s\n",fname,filename); */
      g_free((void *)fname);
      return ret;
    } else {
      /* the XML file is good. libxml is "old enough" to handle it correctly.
       */
      return xmlDoParseFile(filename);        
    }
  } else {
    return xmlDoParseFile(filename);
  }
}

/** Relic of earlier, unhappier days.
 * @param filename A file to parse.
 * @returns An XML document.
 * @bugs Could probably be inlined.
 */
xmlDocPtr
xmlDoParseFile(const char *filename)
{
  return xmlParseFile(filename);
}

/** Find a named attribute node in an XML object node.
 *  Note that Dia has a concept of attribute node that is not the same
 *  as an XML attribute.
 * @param obj_node The node to look in.
 * @param attrname The name of the attribute node to find.
 * @returns The node matching the given name, or NULL if none found.
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

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) ) {
      xmlFree(name);
      return attr;
    }
    if (name) xmlFree(name);
    
    attr = attr->next;
  }
  return NULL;
}

/** Find an attribute in a composite XML node.
 * @param composite_node The composite node to search.
 * @param attrname The name of the attribute node to find.
 * @returns The desired node, or NULL if none exists in `composite_node'.
 * @bugs Describe in more detail how a composite node differs from an
 *   object node.
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

    name = xmlGetProp(attr, "name");
    if ( (name!=NULL) && (strcmp(name, attrname)==0) ) {
      xmlFree(name);
      return attr;
    }
    if (name) xmlFree(name);
    
    attr = attr->next;
  }
  return NULL;
}

/** The number of non-blank data nodes in an attribute node.
 * @param attribute The attribute node to read from.
 * @returns The number of non-blank data nodes in the node.
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

/** Get the first data node in an attribute node.
 * @param attribute The attribute node to look through.
 * @return The first non-black data node in the attribute node.
 */
DataNode
attribute_first_data(AttributeNode attribute)
{
  xmlNode *data = attribute ? attribute->xmlChildrenNode : NULL;
  while (data && xmlIsBlankNode(data)) data = data->next;
  return (DataNode) data;
}

/** Get the next data node (sibling).
 * @param data A data node to start from (e.g. just processed)
 * @returns The next sibling data node.
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

/** Get the type of a data node.
 * @param data The data node.
 * @returns The type that the data node defines, or 0 on error.  In case of 
 *  error, an error message is displayed.
 * @note This function does a number of strcmp calls, which may not be the
 *  fastest way to check if a node is of the expected type.
 * @bugs Make functions that check quickly if a node is of a specific type
 *  (but profile first).
 */
DataType
data_type(DataNode data)
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
  }

  message_error("Unknown type of DataNode");
  return 0;
}

/** Return the value of an integer-type data node.
 * @param data The data node to read from.
 * @returns The integer value found in the node.  If the node is not an
 *  integer node, an error message is displayed and 0 is returned.
 */
int
data_int(DataNode data)
{
  xmlChar *val;
  int res;
  
  if (data_type(data)!=DATATYPE_INT) {
    message_error("Taking int value of non-int node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = atoi(val);
  if (val) xmlFree(val);
  
  return res;
}

/** Return the value of an enum-type data node.
 * @param data The data node to read from.
 * @returns The enum value found in the node.  If the node is not an
 *  enum node, an error message is displayed and 0 is returned.
 */
int data_enum(DataNode data)
{
  xmlChar *val;
  int res;
  
  if (data_type(data)!=DATATYPE_ENUM) {
    message_error("Taking enum value of non-enum node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = atoi(val);
  if (val) xmlFree(val);
  
  return res;
}

/** Return the value of a real-type data node.
 * @param data The data node to read from.
 * @returns The real value found in the node.  If the node is not a
 *  real-type node, an error message is displayed and 0.0 is returned.
 */
real
data_real(DataNode data)
{
  xmlChar *val;
  real res;

  if (data_type(data)!=DATATYPE_REAL) {
    message_error("Taking real value of non-real node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  res = g_ascii_strtod(val, NULL);
  if (val) xmlFree(val);
  
  return res;
}

/** Return the value of a boolean-type data node.
 * @param data The data node to read from.
 * @returns The boolean value found in the node.  If the node is not a
 *  boolean node, an error message is displayed and FALSE is returned.
 */
int
data_boolean(DataNode data)
{
  xmlChar *val;
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

  if (val) xmlFree(val);

  return res;
}

/** Return the integer value of a hex digit.
 * @param c A hex digit, one of 0-9, a-f or A-F.
 * @returns The value of the digit, i.e. 0-15.  If a non-gex digit is given
 *  an error message is displayed to the user, and 0 is returned.
 */
static int 
hex_digit(char c)
{
  if ((c>='0') && (c<='9'))
    return c-'0';
  if ((c>='a') && (c<='f'))
    return (c-'a') + 10;
  if ((c>='A') && (c<='F'))
    return (c-'A') + 10;
  message_error("wrong hex digit %c", c);
  return 0;
}

/** Return the value of a color-type data node.
 * @param data The XML node to read from
 * @param col A place to store the resulting RGB values.  If the node does
 *  not contain a valid color value, an error message is displayed to the
 *  user, and `col' is unchanged.
 * @note Could be cool to use RGBA data here, even if we can't display it yet.
 */
void
data_color(DataNode data, Color *col)
{
  xmlChar *val;
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

  if (val) xmlFree(val);
  
  col->red = ((float)r)/255.0;
  col->green = ((float)g)/255.0;
  col->blue = ((float)b)/255.0;
}

/** Return the value of a point-type data node.
 * @param data The XML node to read from
 * @param point A place to store the resulting x, y values.  If the node does
 *  not contain a valid point value, an error message is displayed to the
 *  user, and `point' is unchanged.
 */
void
data_point(DataNode data, Point *point)
{
  xmlChar *val;
  gchar *str;
  real ax,ay;

  if (data_type(data)!=DATATYPE_POINT) {
    message_error(_("Taking point value of non-point node."));
    return;
  }
  
  val = xmlGetProp(data, "val");
  point->x = g_ascii_strtod(val, &str);
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

/** Return the value of a rectangle-type data node.
 * @param data The data node to read from.
 * @param rect A place to store the resulting values.  If the node does
 *  not contain a valid rectangle value, an error message is displayed to the
 *  user, and `rect' is unchanged.
 */
void
data_rectangle(DataNode data, Rectangle *rect)
{
  xmlChar *val;
  gchar *str;
  
  if (data_type(data)!=DATATYPE_RECTANGLE) {
    message_error("Taking rectangle value of non-rectangle node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  
  rect->left = g_ascii_strtod(val, &str);
  
  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }
    
  rect->top = g_ascii_strtod(str+1, &str);

  while ((*str != ';') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }

  rect->right = g_ascii_strtod(str+1, &str);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }

  rect->bottom = g_ascii_strtod(str+1, NULL);
  
  xmlFree(val);
}

/** Return the value of a string-type data node.
 * @param data The data node to read from.
 * @returns The string value found in the node.  If the node is not a
 *  string node, an error message is displayed and NULL is returned.  The
 *  returned valuee should be freed after use.
 * @note For historical reasons, strings in Dia XML are surrounded by ##.
 */
gchar *
data_string(DataNode data)
{
  xmlChar *val;
  gchar *str, *p,*str2;
  int len;
  
  if (data_type(data)!=DATATYPE_STRING) {
    message_error("Taking string value of non-string node.");
    return NULL;
  }

  val = xmlGetProp(data, "val");
  if (val != NULL) { /* Old kind of string. Left for backwards compatibility */
    str  = g_malloc(4 * (sizeof(char)*(strlen(val)+1))); /* extra room 
                                                            for UTF8 */
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
    xmlFree(val);
    str2 = g_strdup(str);  /* to remove the extra space */
    g_free(str);
    return str2;
  }

  if (data->xmlChildrenNode!=NULL) {
    p = xmlNodeListGetString(data->doc, data->xmlChildrenNode, TRUE);
    
    if (*p!='#')
      message_error("Error in file, string not starting with #\n");
    
    len = strlen(p)-1; /* Ignore first '#' */
      
    str = g_malloc(len+1);

    strncpy(str, p+1, len);
    str[len]=0; /* For safety */

    str[strlen(str)-1] = 0; /* Remove last '#' */
    xmlFree(p);
    return str;
  }
    
  return NULL;
}

/** Return the value of a filename-type data node.
 * @param data The data node to read from.
 * @returns The filename value found in the node.  If the node is not a
 *  filename node, an error message is displayed and NULL is returned.
 *  The resulting string is in the local filesystem's encoding rather than
 *  UTF-8, and should be freed after use.
 * @bugs data_string() can return NULL, what does g_filename_from_utf8 do then?
 */
char *
data_filename(DataNode data)
{
  char *utf8 = data_string(data);
  char *filename = g_filename_from_utf8(utf8, -1, NULL, NULL, NULL);
  g_free(utf8);
  return filename;
}

/** Return the value of a font-type data node.  This handles both the current
 * format (family and style) and the old format (name).
 * @param data The data node to read from.
 * @returns The font value found in the node.  If the node is not a
 *  font node, an error message is displayed and NULL is returned.  The
 *  resulting value should be freed after use.
 */
DiaFont *
data_font(DataNode data)
{
  xmlChar *family;
  DiaFont *font;
  
  if (data_type(data)!=DATATYPE_FONT) {
    message_error("Taking font value of non-font node.");
    return NULL;
  }

  family = xmlGetProp(data, "family");
  /* always prefer the new format */
  if (family) {
    DiaFontStyle style;
    xmlChar* style_name = xmlGetProp(data, "style");
    style = style_name ? atoi(style_name) : 0;

    font = dia_font_new (family, style, 1.0);
    if (family) xmlFree(family);
    if (style_name) xmlFree(style_name);
  } else {
    /* Legacy format support */
    char *name = xmlGetProp(data, "name");
    font = dia_font_new_from_legacy_name(name);
    xmlFree(name);
  }
  return font;
}

/* ***** Saving XML **** */

/** Create a new attribute node.
 * @param obj_node The object node to create the attribute node under.
 * @param attrname The name of the attribute node.
 * @returns A new attribute node.
 * @bugs Should have utility functions that creates the node and sets
 *  the value based on type.
 */
AttributeNode
new_attribute(ObjectNode obj_node,
	      const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(obj_node, NULL, "attribute", NULL);
  xmlSetProp(attr, "name", attrname);

  return attr;
}

/** Add an attribute node to a composite node.
 * @param composite_node The composite node.
 * @param attrname The name of the new attribute node.
 * @returns The attribute node added.
 * @bugs This does exactly the same as new_attribute.
 */
AttributeNode
composite_add_attribute(DataNode composite_node,
			const char *attrname)
{
  AttributeNode attr;
  attr = xmlNewChild(composite_node, NULL, "attribute", NULL);
  xmlSetProp(attr, "name", attrname);

  return attr;
}

/** Add integer data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 */
void
data_add_int(AttributeNode attr, int data)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  g_snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "int", NULL);
  xmlSetProp(data_node, "val", buffer);
}

/** Add enum data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 */
void
data_add_enum(AttributeNode attr, int data)
{
  DataNode data_node;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  g_snprintf(buffer, 20, "%d", data);
  
  data_node = xmlNewChild(attr, NULL, "enum", NULL);
  xmlSetProp(data_node, "val", buffer);
}

/** Add real-typed data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 */
void
data_add_real(AttributeNode attr, real data)
{
  DataNode data_node;
  char buffer[G_ASCII_DTOSTR_BUF_SIZE]; /* Large enought */

  g_ascii_dtostr(buffer, G_ASCII_DTOSTR_BUF_SIZE, data);
  
  data_node = xmlNewChild(attr, NULL, "real", NULL);
  xmlSetProp(data_node, "val", buffer);
}

/** Add boolean data to an attribute node.
 * @param attr The attribute node.
 * @param data The value to set.
 */
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

/** Convert a floating-point value to hexadecimal.
 * @param x The floating point value.
 * @param str A string to place the result in.
 * @note Currently only works for 0 <= x <= 255 and will silently cap the value
 *  to those limits.  Also expects str to have at least two bytes allocated,
 *  and doesn't null-terminate it.  This works well for converting a color
 *  value, but is pretty much useless for other values.
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

/** Add color data to an attribute node.
 * @param attr The attribute node.
 * @param col The value to set.
 */
void
data_add_color(AttributeNode attr, const Color *col)
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

/** Add point data to an attribute node.
 * @param attr The attribute node.
 * @param point The value to set.
 */
void
data_add_point(AttributeNode attr, const Point *point)
{
  DataNode data_node;
  gchar *buffer;
  gchar px_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar py_buf[G_ASCII_DTOSTR_BUF_SIZE];
  
  g_ascii_formatd(px_buf, sizeof(px_buf), "%g", point->x);
  g_ascii_formatd(py_buf, sizeof(py_buf), "%g", point->y);
  buffer = g_strconcat(px_buf, ",", py_buf, NULL);
  
  data_node = xmlNewChild(attr, NULL, "point", NULL);
  xmlSetProp(data_node, "val", buffer);
  g_free(buffer);
}

/** Add rectangle data to an attribute node.
 * @param attr The attribute node.
 * @param rect The value to set.
 */
void
data_add_rectangle(AttributeNode attr, const Rectangle *rect)
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
  
  data_node = xmlNewChild(attr, NULL, "rectangle", NULL);
  xmlSetProp(data_node, "val", buffer);

  g_free(buffer);
}

/** Add string data to an attribute node.
 * @param attr The attribute node.
 * @param str The value to set.
 */
void
data_add_string(AttributeNode attr, const char *str)
{
    DataNode data_node;
    xmlChar *escaped_str;
    xmlChar *sharped_str;

    if (str==NULL) {
        data_node = xmlNewChild(attr, NULL, "string", "##");
        return;
    } 

    escaped_str = xmlEncodeEntitiesReentrant(attr->doc,str);
    
    sharped_str = g_strconcat("#", escaped_str, "#", NULL);

    xmlFree(escaped_str);
    
    data_node = xmlNewChild(attr, NULL, "string", sharped_str);
  
    g_free(sharped_str);
}

/** Add filename data to an attribute node.
 * @param attr The attribute node.
 * @param filename The value to set.  This should be n the local filesystem
 *  encoding, not utf-8.
 */
void
data_add_filename(DataNode data, const char *str)
{
  char *utf8 = g_filename_to_utf8(str, -1, NULL, NULL, NULL);

  data_add_string(data, utf8);

  g_free(utf8);
}

/** Add font data to an attribute node.
 * @param attr The attribute node.
 * @param font The value to set.
 */
void
data_add_font(AttributeNode attr, const DiaFont *font)
{
  DataNode data_node;
  DiaFontStyle style;
  char buffer[20+1]; /* Enought for 64bit int + zero */

  data_node = xmlNewChild(attr, NULL, "font", NULL);
  style = dia_font_get_style (font);
  xmlSetProp(data_node, "family", dia_font_get_family(font));
  g_snprintf(buffer, 20, "%d", dia_font_get_style(font));
 
  xmlSetProp(data_node, "style", buffer);
  /* Legacy support: don't crash older Dia on missing 'name' attribute */
  xmlSetProp(data_node, "name", dia_font_get_legacy_name(font));
}

/** Add a new composite node to an attribute node.
 * @param attr The attribute node to add to.
 * @param type The type of the new node.
 * @returns The new child of `attr'.
 */
DataNode
data_add_composite(AttributeNode attr, const char *type) 
{
  /* type can be NULL */
  DataNode data_node;
 
  data_node = xmlNewChild(attr, NULL, "composite", NULL);
  if (type != NULL) 
    xmlSetProp(data_node, "type", type);

  return data_node;
}

/** Emit a warning to the user that having UTF-8 as local charset doesn't.
 */
void 
warn_about_broken_libxml1(void) 
{
  message_warning(_("Your local character set is UTF-8. Because of issues"
                    " with libxml1 and the support of files generated by"
                    " previous versions of dia, you will encounter "
                    " problems. Please report to dia-list@gnome.org if you"
                    " see this message."));
}

#define BUFSIZE 2048
#define OVERRUN_SAFETY 16

/* diarc option */
int pretty_formated_xml = TRUE;

/** Save an XML document to a file.
 * @param filename The file to save to.
 * @param cur The XML document structure.
 * @return The return value of xmlSaveFormatFileEnc.
 * @bugs Get the proper defn of the return value from libxml2.
 */
int
xmlDiaSaveFile(const char *filename,
                   xmlDocPtr cur)
{
    int old = 0, ret;

    if (pretty_formated_xml)
        old = xmlKeepBlanksDefault (0);
    ret = xmlSaveFormatFileEnc (filename,cur, "UTF-8", pretty_formated_xml ? 1 : 0);
    if (pretty_formated_xml)
        xmlKeepBlanksDefault (old);
    return ret;
}
