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

#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>
#include <string.h>
#include "dia_xml_libxml.h"


#include <stdio.h>

#include "dia-enums.h"
#include "arrows.h"

#include "line_info.h"

char* custom_linetype_strings[] = {
  "Zigzagline",
  "Polyline",
  "Bezierline",
  "All"
};


/* to go in customline_util.c/h */
gchar *
custom_get_relative_filename(const gchar *current, const gchar *relative);
gchar *
custom_get_relative_filename(const gchar *current, const gchar *relative)
{
  gchar *dirname, *tmp;

  g_return_val_if_fail(current != NULL, NULL);
  g_return_val_if_fail(relative != NULL, NULL);

  if (g_path_is_absolute(relative))
    return g_strdup(relative);
  dirname = g_path_get_dirname(current);
  tmp = g_build_filename(dirname, relative, NULL);
  g_free(dirname);
  return tmp;
}

guint line_info_get_line_type( const gchar* filename, xmlNodePtr node )
{
  guint res = CUSTOM_LINETYPE_ZIGZAGLINE;
  char* tmp = xmlNodeGetContent(node);

  if( !strcmp(tmp, "Zigzagline") )
  	res = CUSTOM_LINETYPE_ZIGZAGLINE;
  else if( !strcmp(tmp, "Polyline") )
  	res = CUSTOM_LINETYPE_POLYLINE;
  else if( !strcmp(tmp, "Bezierline") )
  	res = CUSTOM_LINETYPE_BEZIERLINE;
  else if( !strcmp(tmp, "All") )
  	res = CUSTOM_LINETYPE_ALL;
  else
  	g_warning("%s: `%s' is not a valid line type",filename,tmp);
	
  xmlFree(tmp);
  
  return( res );
}


guint line_info_get_line_style( const gchar* filename, xmlNodePtr node )
{
  guint res = LINESTYLE_SOLID;
  char* tmp = xmlNodeGetContent(node);

  if( !strcmp(tmp, "Solid") )
  	res = LINESTYLE_SOLID;
  else if( !strcmp(tmp, "Dashed") )
  	res = LINESTYLE_DASHED;
  else if( !strcmp(tmp, "Dash-Dot") )
  	res = LINESTYLE_DASH_DOT;
  else if( !strcmp(tmp, "Dash-Dot-Dot") )
  	res = LINESTYLE_DASH_DOT_DOT;
  else if( !strcmp(tmp, "Dotted") )
  	res = LINESTYLE_DOTTED;
  else
  	g_warning("%s: `%s' is not a valid line style",filename,tmp);
	
  xmlFree(tmp);
  
  return( res );
}

gfloat line_info_get_as_float( const gchar* filename, xmlNodePtr node )
{
  gfloat res = 1.0f;
  char* tmp = xmlNodeGetContent(node);

  res = g_ascii_strtod( tmp, NULL );
    
  xmlFree(tmp);
  return( res );  
}

guint line_info_get_arrow_type( const gchar* filename, xmlNodePtr node )
{
  guint res = ARROW_NONE;
  char* tmp = xmlNodeGetContent(node);

  if( !strcmp(tmp, "None") )
  	res = ARROW_NONE;
  else if( !strcmp(tmp, "Lines") )
  	res = ARROW_LINES;
  else if( !strcmp(tmp, "Hollow-Triangle") )
  	res = ARROW_HOLLOW_TRIANGLE;
  else if( !strcmp(tmp, "Filled-Triangle") )
  	res = ARROW_FILLED_TRIANGLE;
  else if( !strcmp(tmp, "Hollow-Diamond") )
  	res = ARROW_HOLLOW_DIAMOND;
  else if( !strcmp(tmp, "Filled-Diamond") )
  	res = ARROW_FILLED_DIAMOND;
  else if( !strcmp(tmp, "Half-Head") )
  	res = ARROW_HALF_HEAD;
  else if( !strcmp(tmp, "Slashed-Cross") )
  	res = ARROW_SLASHED_CROSS;
  else if( !strcmp(tmp, "Filled-Ellipse") )
  	res = ARROW_FILLED_ELLIPSE;
  else if( !strcmp(tmp, "Hollow-Ellipse") )
  	res = ARROW_HOLLOW_ELLIPSE;
  else if( !strcmp(tmp, "Double-Hollow-Triangle ") )
  	res = ARROW_DOUBLE_HOLLOW_TRIANGLE;
  else if( !strcmp(tmp, "Double-Filled-Triangle") )
  	res = ARROW_DOUBLE_FILLED_TRIANGLE;
  else if( !strcmp(tmp, "Unfilled-Triangle ") )
  	res = ARROW_UNFILLED_TRIANGLE;
  else if( !strcmp(tmp, "Filled-Dot") )
  	res = ARROW_FILLED_DOT;
  else if( !strcmp(tmp, "Dimension-Origin") )
  	res = ARROW_DIMENSION_ORIGIN;
  else if( !strcmp(tmp, "Blanked-Dot") )
  	res = ARROW_BLANKED_DOT;
  else if( !strcmp(tmp, "Filled-Box") )
  	res = ARROW_FILLED_BOX;
  else if( !strcmp(tmp, "Blanked-Box") )
  	res = ARROW_BLANKED_BOX;
  else if( !strcmp(tmp, "Slash-Arrow") )
  	res = ARROW_SLASH_ARROW;
  else if( !strcmp(tmp, "Integral-Symbol") )
  	res = ARROW_INTEGRAL_SYMBOL;
  else if( !strcmp(tmp, "Crow-Foot") )
  	res = ARROW_CROW_FOOT;
  else if( !strcmp(tmp, "Cross") )
  	res = ARROW_CROSS;
  else if( !strcmp(tmp, "Filled-Concave") )
  	res = ARROW_FILLED_CONCAVE;
  else if( !strcmp(tmp, "Blanked-Concave") )
  	res = ARROW_BLANKED_CONCAVE;
  else if( !strcmp(tmp, "Rounded") )
  	res = ARROW_ROUNDED;
  else if( !strcmp(tmp, "Half-Diamond") )
  	res = ARROW_HALF_DIAMOND;
  else if( !strcmp(tmp, "Open-Rounded") )
  	res = ARROW_OPEN_ROUNDED;
  else if( !strcmp(tmp, "Filled-Dot-N-Triangle") )
  	res = ARROW_FILLED_DOT_N_TRIANGLE;
  else if( !strcmp(tmp, "One-Or-Many") )
  	res = ARROW_ONE_OR_MANY;
  else if( !strcmp(tmp, "None-Or-Many") )
  	res = ARROW_NONE_OR_MANY;
  else if( !strcmp(tmp, "One-Or-None") )
  	res = ARROW_ONE_OR_NONE;
  else if( !strcmp(tmp, "One-Exactly") )
  	res = ARROW_ONE_EXACTLY;
  else if( !strcmp(tmp, "Backslash") )
  	res = ARROW_BACKSLASH;
  else if( !strcmp(tmp, "Three-Dots") )
  	res = ARROW_THREE_DOTS;
  else
  	g_warning("%s: `%s' is not a valid arrow style",filename,tmp);
	
  xmlFree(tmp);
  
  return( res );
}

void line_info_get_arrow( const gchar* filename, xmlNodePtr node, Arrow* arrow )
{
  xmlNodePtr child_node = NULL;
  char* tmp = xmlNodeGetContent(node);

  for( child_node = node->xmlChildrenNode;
       child_node != NULL;
	   child_node = child_node->next )
  {
    if( xmlIsBlankNode(child_node) )
      continue;
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "type")) {
	  arrow->type = line_info_get_arrow_type(filename, child_node);
	}
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "length")) {
	  arrow->length = line_info_get_as_float(filename, child_node);
	}
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "width")) {
	  arrow->width = line_info_get_as_float(filename, child_node);
	}
  }
  
  /* res = g_ascii_strtod( tmp, NULL ); */
    
  xmlFree(tmp);
}

void line_info_get_arrows( const gchar* filename, xmlNodePtr node, LineInfo* info )
{
  xmlNodePtr child_node = NULL;
  char* tmp = xmlNodeGetContent(node);

  for( child_node = node->xmlChildrenNode;
       child_node != NULL;
	   child_node = child_node->next )
  {
    if( xmlIsBlankNode(child_node) )
      continue;
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "start")) {
	  line_info_get_arrow(filename, child_node, &(info->start_arrow));
	}
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "end")) {
	  line_info_get_arrow(filename, child_node, &(info->end_arrow));
	}
  }
  
  /* res = g_ascii_strtod( tmp, NULL ); */
    
  xmlFree(tmp);
}


void line_info_get_line_color( const gchar* filename, xmlNodePtr node, LineInfo* info )
{
  xmlNodePtr child_node = NULL;
  char* tmp = xmlNodeGetContent(node);

  for( child_node = node->xmlChildrenNode;
       child_node != NULL;
	   child_node = child_node->next )
  {
    if( xmlIsBlankNode(child_node) )
      continue;
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "red")) {
	  info->line_color.red = line_info_get_as_float(filename, child_node);
	}
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "green")) {
	  info->line_color.green = line_info_get_as_float(filename, child_node);
	}
	else if (/*node->ns == shape_ns &&*/ !strcmp(child_node->name, "blue")) {
	  info->line_color.blue = line_info_get_as_float(filename, child_node);
	}
  }
  
  /* res = g_ascii_strtod( tmp, NULL ); */
    
  xmlFree(tmp);
}

LineInfo* line_info_load_and_apply_from_xmlfile(const gchar *filename, LineInfo* info);

LineInfo* line_info_load(const gchar *filename)
{
  LineInfo* res = g_new0(LineInfo, 1);
  
  res->line_info_filename = filename;  

  res->name = "CustomLines - Default";
  res->icon_filename = NULL;
  res->type = CUSTOM_LINETYPE_ZIGZAGLINE;
  res->line_color.red   = 0.0f;
  res->line_color.green = 0.0f;
  res->line_color.blue  = 0.0f;
  res->line_style = LINESTYLE_SOLID;
  res->dashlength = 1.0f;
  res->line_width = 0.1f;
  res->corner_radius = 0.0f;
  res->start_arrow.type = ARROW_NONE;
  res->end_arrow.type = ARROW_NONE;

  /* warning: possible memory leak */
  res = line_info_load_and_apply_from_xmlfile( filename, res );
  
  return( res );
}

LineInfo* line_info_clone(LineInfo* info)
{
  LineInfo* res = g_new0(LineInfo, 1);
  
  res->line_info_filename 	= info->line_info_filename;  

  res->name 				= info->name;
  res->icon_filename 		= info->icon_filename;
  res->type 				= info->type;
  res->line_color.red   	= info->line_color.red;
  res->line_color.green 	= info->line_color.green;
  res->line_color.blue  	= info->line_color.blue;
  res->line_style 			= info->line_style;
  res->dashlength 			= info->dashlength;
  res->line_width 			= info->line_width;
  res->corner_radius 		= info->corner_radius;
  res->start_arrow.type 	= info->start_arrow.type;
  res->start_arrow.length 	= (info->start_arrow.length > 0) ?
                                    info->start_arrow.length : 1.0;
  res->start_arrow.width 	= (info->start_arrow.width > 0) ? 
                                    info->start_arrow.width : 1.0;
  res->end_arrow.type 		= info->end_arrow.type;
  res->end_arrow.length 	= (info->end_arrow.length > 0) ? 
                                    info->end_arrow.length : 1.0;
  res->end_arrow.width 		= (info->end_arrow.width > 0) ? 
                                    info->end_arrow.width : 1.0;

  return( res );
}

LineInfo* line_info_load_and_apply_from_xmlfile(const gchar *filename, LineInfo* info)
{
  xmlDocPtr doc = xmlDoParseFile(filename);
  xmlNsPtr shape_ns, svg_ns;
  xmlNodePtr node, root, ext_node = NULL;
  char *tmp;
  int i;

  if (!doc) {
    g_warning("parse error for %s", filename);
    return NULL;
  }
  /* skip (emacs) comments */
  root = doc->xmlRootNode;
  while (root && (root->type != XML_ELEMENT_NODE)) root = root->next;
  if (!root) return NULL;
  if (xmlIsBlankNode(root)) return NULL;

  i = 0;
  for (node = root->xmlChildrenNode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node))
		continue;
    else if (node->type != XML_ELEMENT_NODE)
		continue;
    else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "name")) {
      tmp = xmlNodeGetContent(node);
/*      g_free(info->name);*/
      info->name = g_strdup(tmp);
/*	  fprintf( stderr, "New shape of type: `%s'\n", info->name ); */
      xmlFree(tmp);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "icon")) {
      tmp = xmlNodeGetContent(node);
      g_free(info->icon_filename);
      info->icon_filename = custom_get_relative_filename(filename, tmp);
      xmlFree(tmp);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "type")) {
      info->type = line_info_get_line_type(filename, node);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "line-style")) {
      info->line_style = line_info_get_line_style(filename, node);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "dash-length")) {
      info->dashlength = line_info_get_as_float(filename, node);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "line-width")) {
      info->line_width = line_info_get_as_float(filename, node);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "corner-radius")) {
      info->corner_radius = line_info_get_as_float(filename, node);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "arrows")) {
      line_info_get_arrows(filename, node, info);
    } else if (/*node->ns == shape_ns &&*/ !strcmp(node->name, "line-color")) {
      line_info_get_line_color(filename, node, info);
    }
  }

  return( info );
}




