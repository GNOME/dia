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
#include <locale.h>
#include <math.h>
#include <fcntl.h>

#include <parser.h>
#include <parserInternals.h>
#include <xmlmemory.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <zlib.h>

#include "intl.h"
#include "utils.h"
#include "dia_xml_libxml.h"
#include "dia_xml.h"
#include "message.h"
#include "charconv.h"


#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
#define XML2
#endif


#ifdef G_OS_WIN32 /* apparently _MSC_VER and mingw */
#include <float.h>
#define isinf(a) (!_finite(a))
#endif


#define BUFLEN 1024


static const gchar *
xml_file_check_encoding(const gchar *filename, const gchar *default_enc)
{
  /* If all files produced by dia were good XML files, we wouldn't have to do 
     this little gymnastic. Alas, during the libxml1 days, we were outputting 
     files with no encoding specification (which means UTF-8 if we're in an
     asciish encoding) and strings encoded in local charset (so, we wrote
     broken files). 

     The following logic finds if we have a broken file, and attempts to fix 
     it if it's possible. If the file is correct or is unrecognisable, we pass
     it untouched to libxml2.
  */

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
    return filename; /* let libxml figure out what this is. */
  }
  /* now, we're sure we have some asciish XML file. */
  p += 5;
  while (((*p == 0x20)||(*p == 0x09)||(*p == 0x0d)||(*p == 0x0a))
         && (p<pmax)) p++;
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    return filename;
  }
  if (0 != strncmp(p,"version=\"",9)) {
    gzclose(zf); /* chicken out. */
    return filename;
  }
  p += 9;
  /* The header is rather well formed. */
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    return filename;
  }
  while ((*p != '"') && (p < pmax)) p++;
  p++;
  while (((*p == 0x20)||(*p == 0x09)||(*p == 0x0d)||(*p == 0x0a))
         && (p<pmax)) p++;
  if (p>=pmax) { /* whoops ? */
    gzclose(zf);
    return filename;
  }
  if (0 == strncmp(p,"encoding=\"",10)) {
    gzclose(zf); /* this file has an encoding string. Good. */
    return filename;
  }
  /* now let's read the whole file, to see if there are offending bits.
   * We can call it well formed UTF-8 if the highest isn't used
   */
  well_formed_utf8 = TRUE;
  do {
    int i;
    for (i = 0; i < len; i++)
      if (buf[i] & 0x80)
        well_formed_utf8 = FALSE;
    len = gzread(zf,buf,BUFLEN);
  } while (len > 0 && well_formed_utf8);
  if (well_formed_utf8) {
    gzclose(zf); /* this file is utf-8 compatible  */
    return filename;
  }

  if (0 != strcmp(default_enc,"UTF-8")) {
    message_warning(_("The file %s has no encoding specification;\n"
                      "assuming it is encoded in %s"),filename,default_enc);
  } else {
    gzclose(zf); /* we apply the standard here. */
    return filename;
  }

  tmp = getenv("TMP"); 
  if (!tmp) tmp = getenv("TEMP");
  if (!tmp) tmp = "/tmp";

  res = g_strconcat(tmp,G_DIR_SEPARATOR_S,"dia-xml-fix-encodingXXXXXX",NULL);
#if GLIB_CHECK_VERSION(1,3,4)
  uf = g_mkstemp(res);
#else
  uf = mkstemp(res);
#endif
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
  return res; /* caller frees the name and unlinks the file. */
}



xmlDocPtr
xmlDiaParseFile(const char *filename) {
  char *local_charset = NULL;
  
  if (   !get_local_charset(&local_charset)
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

xmlDocPtr
xmlDoParseFile(const char *filename) {
#ifdef XML2
  return xmlParseFile(filename);
#else
  int state = xmlUseNewParser(1); /* use the libxml2 parser in libxml1 */
  xmlDocPtr doc = xmlParseFile(filename);
  xmlUseNewParser(state);
  return doc;
#endif
}

AttributeNode
object_find_attribute(ObjectNode obj_node,
		      const char *attrname)
{
  AttributeNode attr;
  char *name;

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

AttributeNode
composite_find_attribute(DataNode composite_node,
			 const char *attrname)
{
  AttributeNode attr;
  char *name;

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

int
attribute_num_data(AttributeNode attribute)
{
  xmlNode *data;
  int nr=0;

  data =  attribute->xmlChildrenNode;
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

DataNode
attribute_first_data(AttributeNode attribute)
{
  xmlNode *data = attribute->xmlChildrenNode;
  while (data && xmlIsBlankNode(data)) data = data->next;
  return (DataNode) data;
}

DataNode
data_next(DataNode data)
{
  
  if (data) { 
    data = data->next;
    while (data && xmlIsBlankNode(data)) data = data->next;
  }
  return (DataNode) data;
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
  if (val) xmlFree(val);
  
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
  if (val) xmlFree(val);
  
  return res;
}

real
data_real(DataNode data)
{
  char *val;
  real res;
  char *old_locale;

  if (data_type(data)!=DATATYPE_REAL) {
    message_error("Taking real value of non-real node.");
    return 0;
  }

  val = xmlGetProp(data, "val");
  old_locale = setlocale(LC_NUMERIC, "C");
  res = strtod(val, NULL);
  setlocale(LC_NUMERIC, old_locale);
  if (val) xmlFree(val);
  
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

  if (val) xmlFree(val);

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

  if (val) xmlFree(val);
  
  col->red = ((float)r)/255.0;
  col->green = ((float)g)/255.0;
  col->blue = ((float)b)/255.0;
}

void
data_point(DataNode data, Point *point)
{
  char *val;
  char *str;
  char *old_locale;
  real ax,ay;

  if (data_type(data)!=DATATYPE_POINT) {
    message_error(_("Taking point value of non-point node."));
    return;
  }
  
  val = xmlGetProp(data, "val");
  old_locale = setlocale(LC_NUMERIC, "C");
  point->x = strtod(val, &str);
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
    setlocale(LC_NUMERIC, old_locale);
    point->y = 0.0;
    g_error(_("Error parsing point."));
    xmlFree(val);
    return;
  }
  point->y = strtod(str+1, NULL);
  ay = fabs(point->y);
  if ((ay > 1e9) || ((ay < 1e-9) && (ay != 0.0)) || isnan(ay) || isinf(ay)) {
    if (!(ay < 1e-9)) /* don't bother with useless warnings (see above) */
      g_warning(_("Incorrect y Point value \"%s\" %f; discarding it."),str+1,point->y);
    point->y = 0.0;
  }
  setlocale(LC_NUMERIC, old_locale);
  xmlFree(val);
}

void
data_rectangle(DataNode data, Rectangle *rect)
{
  char *val;
  char *str;
  char *old_locale;
  
  if (data_type(data)!=DATATYPE_RECTANGLE) {
    message_error("Taking rectangle value of non-rectangle node.");
    return;
  }
  
  val = xmlGetProp(data, "val");
  
  old_locale = setlocale(LC_NUMERIC, "C");
  rect->left = strtod(val, &str);
  setlocale(LC_NUMERIC, old_locale);
  
  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }
    
  old_locale = setlocale(LC_NUMERIC, "C");
  rect->top = strtod(str+1, &str);
  setlocale(LC_NUMERIC, old_locale);

  while ((*str != ';') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }

  old_locale = setlocale(LC_NUMERIC, "C");
  rect->right = strtod(str+1, &str);
  setlocale(LC_NUMERIC, old_locale);

  while ((*str != ',') && (*str!=0))
    str++;

  if (*str==0){
    message_error("Error parsing rectangle.");
    xmlFree(val);
    return;
  }

  old_locale = setlocale(LC_NUMERIC, "C");
  rect->bottom = strtod(str+1, NULL);
  setlocale(LC_NUMERIC, old_locale);
  
  xmlFree(val);
}

utfchar *
data_string(DataNode data)
{
  utfchar *val;
  utfchar *str, *p,*str2;
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
#ifdef UNICODE_WORK_IN_PROGRESS
    str2 = g_strdup(str);  /* to remove the extra space */
#else
    str2 = charconv_utf8_to_local8(str);
#endif
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
    
#ifdef UNICODE_WORK_IN_PROGRESS
    return str;
#else
    str2 = charconv_utf8_to_local8(str);
    g_free(str);
    return str2;
#endif
  }
    
  return NULL;
}

DiaFont *
data_font(DataNode data)
{
  char *name;
  DiaFont *font;
  
  if (data_type(data)!=DATATYPE_FONT) {
    message_error("Taking font value of non-font node.");
    return NULL;
  }

  name = xmlGetProp(data, "name");
  font = font_getfont(name);
  if (name) xmlFree(name);
  
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

void
data_add_point(AttributeNode attr, const Point *point)
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
data_add_rectangle(AttributeNode attr, const Rectangle *rect)
{
  DataNode data_node;
  char buffer[160+1]; /* Large enough? */
  char *old_locale;

  old_locale = setlocale(LC_NUMERIC, "C");
  g_snprintf(buffer, 160, "%g,%g;%g,%g",
	     rect->left, rect->top,
	     rect->right, rect->bottom);
  setlocale(LC_NUMERIC, old_locale);
  
  data_node = xmlNewChild(attr, NULL, "rectangle", NULL);
  xmlSetProp(data_node, "val", buffer);

}

void
data_add_string(AttributeNode attr, const char *str)
{
    DataNode data_node;
    xmlChar *escaped_str;
    xmlChar *sharped_str;
    int len;

    if (str==NULL) {
        data_node = xmlNewChild(attr, NULL, "string", NULL);
        return;
    } 

#ifndef UNICODE_WORK_IN_PROGRESS
    {
        utfchar *utfstr = charconv_local8_to_utf8(str);
        escaped_str = xmlEncodeEntitiesReentrant(attr->doc,utfstr);
        g_free(utfstr);
    }
#else
    escaped_str = xmlEncodeEntitiesReentrant(attr->node,str);
#endif
    
    len = 2+strlen(escaped_str);
    sharped_str = g_malloc0(len+1);
    *sharped_str='#';
    strcpy(sharped_str+1,escaped_str);
    strcat(sharped_str, "#");

    xmlFree(escaped_str);
    
    data_node = xmlNewChild(attr, NULL, "string", sharped_str);
  
    g_free(sharped_str);
}

void
data_add_font(AttributeNode attr, const DiaFont *font)
{
  DataNode data_node;
 
  data_node = xmlNewChild(attr, NULL, "font", NULL);
  xmlSetProp(data_node, "name", font->name);
}

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

int xmlDiaSaveFile(const char *filename,
                   xmlDocPtr cur)
{
#ifdef XML2
    return xmlSaveFileEnc(filename,cur, "UTF-8");
#else
        /* non-XML2 case. Let's have fun working around libxml1's
           brokenness.

           Here, we're basically using the code from libxml1's xmlSaveFile
           routine (Copyright 1998 Daniel.Veillard@w3.org, MIT, INRIA.
           License is LGPL).

           I've savagely modified it, so that it doesn't output too broken
           UTF-8 data. -- CC
        */ 
    
#ifdef HAVE_ZLIB_H
    gzFile zoutput = NULL;
    char mode[15];
#endif
    FILE *output = NULL;    
    int ret = 0;

    xmlChar tempbuf[BUFSIZE+OVERRUN_SAFETY];
    xmlChar *b, *bmax, *tb, *tbmax;
    gboolean done_encoding = FALSE;
    gboolean has_encoding = FALSE;

    xmlChar *buf;
    int buf_size;
    

#ifdef HAVE_ZLIB_H
    if (cur->compression < 0) cur->compression = 0;
    if ((cur->compression > 0) && (cur->compression <= 9)) {
        sprintf(mode, "w%d", cur->compression);
        if (!strcmp(filename, "-")) 
            zoutput = gzdopen(1, mode);
        else
            zoutput = gzopen(filename, mode);
    }
    if (zoutput == NULL) {
#endif
#ifdef WIN32
        output = fopen(filename, "wb");
#else
        output = fopen(filename, "w");
#endif
        if (output == NULL) {
            return(-1);
        }
#ifdef HAVE_ZLIB_H
    }
#endif
        /* This is not code from libxml1 */

    buf = NULL; buf_size = 0;
    xmlDocDumpMemory(cur, &buf, &buf_size);
    
    if ((!buf) || (!buf_size)) {
#ifdef HAVE_ZLIB_H
        if (zoutput != NULL) {
            gzclose(zoutput);
        } else {
#endif
            fclose(output);
#ifdef HAVE_ZLIB_H
        }
#endif
        return -1;
    }
        
    
    b = buf; bmax = buf + buf_size;
    tb = tempbuf; tbmax = tempbuf + BUFSIZE;

    while ((b < bmax)) {
        gboolean skip = FALSE;
        if (!done_encoding) {
                /* Begin of the file. For the moment, we'll look for
                   the EncodingDecl. If we don't find it, we'll provide it. */
            if ((b[0] == 'e') && (b[1] == 'n') && (b[2] == 'c') &&
                (b[3] == 'o') && (b[4] == 'd') && (b[5] == 'i') &&
                (b[6] == 'n') && (b[7] == 'g')) has_encoding = TRUE;
            if ((b[0] == '?') && (b[1] == '>')) {
                if (!has_encoding) {
                        /* We're writing UTF-8 ; and libxml1 thinks that
                           "implicit is better than explicit". Sorry, I'm more
                           in a Python mood -- CC */
                    *(tb++) = ' ';
                    *(tb++) = 'e';
                    *(tb++) = 'n';
                    *(tb++) = 'c';
                    *(tb++) = 'o';
                    *(tb++) = 'd';
                    *(tb++) = 'i';
                    *(tb++) = 'n';
                    *(tb++) = 'g';
                    *(tb++) = '=';
                    *(tb++) = '"';
                    *(tb++) = 'U';
                    *(tb++) = 'T';
                    *(tb++) = 'F';
                    *(tb++) = '-';
                    *(tb++) = '8';
                    *(tb++) = '"';
                        /* Yes, this sucks, but xmlChar could be different
                           from (char), so I prefer avoid strcpy. */
                }
                
                done_encoding = TRUE;
            }
        } else {
            if ((b[0] == '&') && (b[1] == '#')) {
                xmlChar *be = b + 2;
                xmlChar *newbe = be;
                long unsigned charnum = strtoul(be,(char **)&newbe,0);
                
                if (((*newbe) == ';') && (charnum > 127)) {
                    utfchar *frag = charconv_unichar_to_utf8(charnum);
                    while (*frag) *(tb++) = *(frag++);

                    b = newbe + 1;
                    skip = TRUE;
                }
            }
        }
        if (!skip) {
                /* do the actual copy... */
            *(tb++) = *(b++);
        }

        if ((tb >= tbmax) || (b >= bmax)) {
                /* flush the temp. buffer */
            size_t count_to_write = sizeof(xmlChar)*(tb-tempbuf);
#ifdef HAVE_ZLIB_H            
            if (zoutput != NULL) {
                ret += gzwrite(zoutput, tempbuf, count_to_write);
            } else {
#endif
                ret += fwrite(tempbuf,1,count_to_write,output);
#ifdef HAVE_ZLIB_H
            }
#endif
            tb = tempbuf;
        }
    }

#ifdef HAVE_ZLIB_H
    if (zoutput != NULL) {
        gzclose(zoutput);
    } else {
#endif
        fclose(output);
#ifdef HAVE_ZLIB_H
    }
#endif
    xmlFree(buf);
    
    return(ret * sizeof(xmlChar));     
#endif /* XML2 */


        /* We have to do this, because if the character encoding set is set to 
           UTF-8, libxml1 will NOT put an encoding declaration in the XML
           header. This sucks, because we have to support older non-standard
           files dia was generating, where files were stored encoded in the
           local charset *without* writing an encoding header. So, we store in
           local encoding and let libxml{1|2} handle the problem.

           The libxml folks, Daniel Veillard in particular, declared that
           libxml1 is totally obsolete, and have expressed no intention in
           helping us solve cleanly the issue (despite the fact that it's
           libxml1's brokenness which brought the problem in the first place).
           Well, their help will stop at letting us touch the libxml1 CVS
           branch. 

           As of this writing, several other libraries prevent us from going
           to libxml2, which is the proper way to do. */
}
