/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998-2002 Alexander Larsson
 *
 * svg-import.c: SVG import filter for dia
 * Copyright (C) 2002 Steffen Macke
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <locale.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>

#include "intl.h"
#include "message.h"
#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "dia_xml_libxml.h"
#include "intl.h"
#include "dia_svg.h"
#include "create.h"
#include "font.h"

gboolean import_svg(const gchar *filename, DiagramData *dia, void* user_data);
static void read_ellipse_svg(xmlNodePtr node, DiagramData *dia);
static void read_rect_svg(xmlNodePtr node, DiagramData *dia);
static void read_line_svg(xmlNodePtr node, DiagramData *dia);
static void read_poly_svg(xmlNodePtr node, DiagramData *dia, char *object_type);
static void read_text_svg(xmlNodePtr node, DiagramData *dia);
static void read_path_svg(xmlNodePtr node, DiagramData *dia);
static GPtrArray *make_element_props(real xpos, real ypos, real width, real height);

/* TODO: use existing implementation in dia source */
static Color 
get_colour(gint32 c)
{
    Color colour;
    colour.red   = ((c & 0xff0000) >> 16) / 255.0;
    colour.green = ((c & 0x00ff00) >> 8) / 255.0;
    colour.blue  =  (c & 0x0000ff) / 255.0;

    return colour;
}

static PropDescription svg_line_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    PROP_DESC_END};
    
static PropDescription svg_rect_prop_descs[] = {
    { "start_point", PROP_TYPE_POINT },
    { "end_point", PROP_TYPE_POINT },
    { "corner_radius", PROP_TYPE_REAL},
    PROP_DESC_END};

static PropDescription svg_style_prop_descs[] = {
    { "line_colour", PROP_TYPE_COLOUR },
    { "line_width", PROP_TYPE_REAL },
    { "line_style", PROP_TYPE_LINESTYLE},
    { "fill_colour", PROP_TYPE_COLOUR },
    { "show_background", PROP_TYPE_BOOL },
    PROP_DESC_END};

static PropDescription svg_element_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END};

static GPtrArray *
make_element_props(real xpos, real ypos,
                   real width, real height)
{
    GPtrArray *props;
    PointProperty *pprop;
    RealProperty *rprop;

    props = prop_list_from_descs(svg_element_prop_descs,pdtpp_true);
    g_assert(props->len == 3);

    pprop = g_ptr_array_index(props,0);
    pprop->point_data.x = xpos;
    pprop->point_data.y = ypos;
    rprop = g_ptr_array_index(props,1);
    rprop->real_data = width;
    rprop = g_ptr_array_index(props,2);
    rprop->real_data = height;

    return props;
}

static PropDescription svg_text_prop_descs[] = {
    { "text", PROP_TYPE_TEXT },
    PROP_DESC_END};

/* apply SVG style to object */
static void
apply_style(xmlNodePtr node, DiaObject *obj) {
      DiaSvgGraphicStyle *gs;
      GPtrArray *props;
      LinestyleProperty *lsprop;
      ColorProperty *cprop;
      RealProperty *rprop;
      BoolProperty *bprop;
      
      gs = g_new(DiaSvgGraphicStyle, 1);
      /* SVG defaults */
      gs->stroke = (-1);
      gs->line_width = 0.0;
      gs->linestyle = LINESTYLE_SOLID;
      gs->dashlength = 1;
      gs->fill = (-1);
      
      dia_svg_parse_style(node, gs);
      props = prop_list_from_descs(svg_style_prop_descs, pdtpp_true);
      g_assert(props->len == 5);
  
      cprop = g_ptr_array_index(props,0);
      if(gs->stroke != (-1)) {
        cprop->color_data = get_colour(gs->stroke);
      } else {
	if(gs->fill == (-1)) {
	  cprop->color_data = get_colour(0x000000);
	} else {
	  cprop->color_data = get_colour(gs->stroke);
	}
      }
      rprop = g_ptr_array_index(props,1);
      rprop->real_data = gs->line_width;
  
      lsprop = g_ptr_array_index(props,2);
      lsprop->style = gs->linestyle;
      lsprop->dash = gs->dashlength;

      cprop = g_ptr_array_index(props,3);
      cprop->color_data = get_colour(gs->fill);
      
      bprop = g_ptr_array_index(props,4);
      if(gs->fill == (-1)) {
        bprop->bool_data = FALSE;
      } else {
	bprop->bool_data = TRUE;
      }

      obj->ops->set_props(obj, props);
      
      g_free(gs);
}

/* read a path */
static void
read_path_svg(xmlNodePtr node, DiagramData *dia) 
{
    DiaObjectType *otype;
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;
    gchar *str, *pathdata, *unparsed = NULL;
    GList *tmp;
    GArray *bezpoints;
    gboolean closed = FALSE;
    
    pathdata = str = xmlGetProp(node, "d");
    do {
      bezpoints = dia_svg_parse_path (pathdata, &unparsed, &closed);

      if (bezpoints && bezpoints->len > 0) {
        if (!closed)
	  otype = object_get_type("Standard - BezierLine");
        else
	  otype = object_get_type("Standard - Beziergon");

	if (otype == NULL){
	  message_error(_("Can't find standard object"));
	  break;
        }
	bcd = g_new(BezierCreateData, 1);
	bcd->num_points = bezpoints->len;
	bcd->points = &(g_array_index(bezpoints, BezPoint, 0));	
	new_obj = otype->ops->create(NULL, bcd, &h1, &h2);
	g_free(bcd);
	apply_style(node, new_obj);
	layer_add_object(dia->active_layer, new_obj);

        g_array_set_size (bezpoints, 0);
      }
      pathdata = unparsed;
      unparsed = NULL;
    } while (pathdata);

    g_array_free(bezpoints, TRUE);
    xmlFree (str);
}


/* read a text */
void
read_text_svg(xmlNodePtr node, DiagramData *dia) {
    DiaObjectType *otype = object_get_type("Standard - Text");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;
    TextProperty *prop;
    xmlChar *str;
    char *old_locale;
    DiaSvgGraphicStyle *gs;

    gs = g_new(DiaSvgGraphicStyle, 1);
    gs->font = NULL;
    gs->font_height = 1.0;
    gs->alignment = ALIGN_CENTER;

    point.x = 0;
    point.y = 0;

    str = xmlGetProp(node, "x");
    if (str) {
      old_locale = setlocale(LC_NUMERIC, "C");
      point.x = strtod(str, NULL);
      setlocale(LC_NUMERIC, old_locale);
      xmlFree(str);
    }

    str = xmlGetProp(node, "y");
    if (str) {
      old_locale = setlocale(LC_NUMERIC, "C");
      point.y = strtod(str, NULL);
      setlocale(LC_NUMERIC, old_locale);
      xmlFree(str);
    }

    str = xmlNodeGetContent(node);
    if(str) {
      new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
      layer_add_object(dia->active_layer, new_obj);

      props = prop_list_from_descs(svg_text_prop_descs, pdtpp_true);
      g_assert(props->len == 1);

      dia_svg_parse_style(node, gs);
    
      if(gs->font == NULL) {
	gs->font = dia_font_new_from_legacy_name(_("Courier"));
      }
      prop = g_ptr_array_index(props, 0);
      g_free(prop->text_data);
      prop->text_data = g_strdup(str);
      xmlFree(str);
      prop->attr.alignment = gs->alignment;
      prop->attr.position.x = point.x;
      prop->attr.position.y = point.y;
      prop->attr.font = gs->font;
      prop->attr.height = gs->font_height;
      new_obj->ops->set_props(new_obj, props);
      prop_list_free(props);
    }
    g_free(gs);
}

/* read a polygon or a polyline */
void
read_poly_svg(xmlNodePtr node, DiagramData *dia, char *object_type) {
    DiaObjectType *otype = object_get_type(object_type);
    DiaObject *new_obj;
    Handle *h1, *h2;
    MultipointCreateData *pcd;
    Point *points;
    GArray *arr = g_array_new(FALSE, FALSE, sizeof(real));
    real val, *rarr;
    xmlChar *str;
    char *tmp;
    int i;
    char *old_locale;
    
    tmp = str = xmlGetProp(node, "points");
    while (tmp[0] != '\0') {
      /* skip junk */
      while (tmp[0] != '\0' && !g_ascii_isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
	tmp++;
      if (tmp[0] == '\0') break;
      old_locale = setlocale(LC_NUMERIC, "C");
      val = strtod(tmp, &tmp);
      setlocale(LC_NUMERIC, old_locale);
      g_array_append_val(arr, val);
    }
    xmlFree(str);
    val = 0;
    if (arr->len % 2 == 1)
      g_array_append_val(arr, val);
    points = g_malloc0(arr->len/2*sizeof(Point));

    pcd = g_new(MultipointCreateData, 1);
    pcd->num_points = arr->len/2;
    rarr = (real *)arr->data;
    for (i = 0; i < pcd->num_points; i++) {
      points[i].x = rarr[2*i];
      points[i].y = rarr[2*i+1];
    }
    g_array_free(arr, TRUE);
  
    pcd->points = points;
    new_obj = otype->ops->create(NULL, pcd,
				 &h1, &h2);
    apply_style(node, new_obj);
    layer_add_object(dia->active_layer, new_obj);
    g_free(pcd);
}

/* read an ellipse */
void
read_ellipse_svg(xmlNodePtr node, DiagramData *dia) {
xmlChar *str;
  real width, height;
  DiaObjectType *otype = object_get_type("Standard - Ellipse");
  DiaObject *new_obj;
  Handle *h1, *h2;
  GPtrArray *props;
  Point start;
  char *old_locale;
  
  old_locale = setlocale(LC_NUMERIC, "C");
  str = xmlGetProp(node, "cx");
  if (str) {
    start.x = strtod(str, NULL);
    xmlFree(str);
  }
  else {
    setlocale(LC_NUMERIC, old_locale);
    return;
  }
  str = xmlGetProp(node, "cy");
  if (str) {
    start.y = strtod(str, NULL);
    xmlFree(str);
  }
  else {
    setlocale(LC_NUMERIC, old_locale);
    return;
  }
  str = xmlGetProp(node, "rx");
  if (str) {
    width = strtod(str, NULL)*2;
    xmlFree(str);
  }
  else {
    setlocale(LC_NUMERIC, old_locale);
    return;
  }
  str = xmlGetProp(node, "ry");
  if (str) {
    height = strtod(str, NULL)*2;
    xmlFree(str);
  }
  else {
    setlocale(LC_NUMERIC, old_locale);
    return;
  }
  setlocale(LC_NUMERIC, old_locale);

  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  apply_style(node, new_obj);			
	 
  props = make_element_props(start.x-(width/2), start.y-(height/2),
                             width, height);
  new_obj->ops->set_props(new_obj, props);
  prop_list_free(props);
  layer_add_object(dia->active_layer, new_obj);
}

/* read a line */
void
read_line_svg(xmlNodePtr node, DiagramData *dia) {
  xmlChar *str;
  DiaObjectType *otype = object_get_type("Standard - Line");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  GPtrArray *props;
  Point start, end;
  char *old_locale;

  str = xmlGetProp(node, "x1");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    start.x = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "y1");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    start.y = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "x2");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    end.x = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "y2");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    end.y = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;

  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  
  props = prop_list_from_descs(svg_line_prop_descs,pdtpp_true);
  g_assert(props->len == 2);

  ptprop = g_ptr_array_index(props,0);
  ptprop->point_data = start;

  ptprop = g_ptr_array_index(props,1);
  ptprop->point_data = end;

  new_obj->ops->set_props(new_obj, props);
  
  prop_list_free(props);

  apply_style(node, new_obj);
  
  layer_add_object(dia->active_layer, new_obj);
}

/* read a rectangle */
void
read_rect_svg(xmlNodePtr node, DiagramData *dia) {
  xmlChar *str;
  real width, height;
  DiaObjectType *otype = object_get_type("Standard - Box");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  RealProperty *rprop;
  GPtrArray *props;
  Point start,end;
  char *old_locale;
  real corner_radius = 0.0;

  str = xmlGetProp(node, "x");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    start.x = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "y");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    start.y = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "width");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    width = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "height");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    height = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  else return;
  str = xmlGetProp(node, "rx");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    corner_radius = strtod(str, NULL);
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  str = xmlGetProp(node, "ry");
  if (str) {
    old_locale = setlocale(LC_NUMERIC, "C");
    if(corner_radius != 0.0) {
      /* calculate the mean value of rx and ry */
      corner_radius = (corner_radius+strtod(str, NULL))/2;
    } else {
      corner_radius = strtod(str, NULL);
    }
    setlocale(LC_NUMERIC, old_locale);
    xmlFree(str);
  }
  
  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  layer_add_object(dia->active_layer, new_obj);
  props = prop_list_from_descs(svg_rect_prop_descs, pdtpp_true);
  g_assert(props->len == 3);

  ptprop = g_ptr_array_index(props,0);
  ptprop->point_data = start;

  end.x = start.x + width;
  end.y = start.y + height;
  ptprop = g_ptr_array_index(props,1);
  ptprop->point_data = end;

  rprop = g_ptr_array_index(props,2);
  rprop->real_data = corner_radius;
  
  new_obj->ops->set_props(new_obj, props);
  prop_list_free(props);
  props = make_element_props(start.x,start.y,width,height);
  new_obj->ops->set_props(new_obj, props);
  
  apply_style(node, new_obj);
  prop_list_free(props);
}

/* imports the given SVG file, returns TRUE if successful */
gboolean
import_svg(const gchar *filename, DiagramData *dia, void* user_data) {
  xmlDocPtr doc = xmlDoParseFile(filename);
  xmlNsPtr svg_ns;
  xmlNodePtr node, root;

  if (!doc) {
    g_warning("parse error for %s", filename);
    return FALSE;
  }
  /* skip (emacs) comments */
  root = doc->xmlRootNode;
  while (root && (root->type != XML_ELEMENT_NODE)) root = root->next;
  if (!root) return FALSE;
  if (xmlIsBlankNode(root)) return FALSE;

  if (!(svg_ns = xmlSearchNsByHref(doc, root, "http://www.w3.org/2000/svg"))) {
   g_warning(_("Could not find SVG namespace."));
    /* correct filetype vs. robust import */
    /*xmlFreeDoc(doc);
    return FALSE;*/
  }
  if (root->ns != svg_ns && 0 != strcmp(root->name, "svg")) {
    g_warning(_("root element was '%s' -- expecting 'svg'."), root->name);
    xmlFreeDoc(doc);
    return FALSE;
  }

  for (node = root->xmlChildrenNode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE) continue;
    if (!strcmp(node->name, "rect")) {
      read_rect_svg(node, dia);
      continue;
    }
    if (!strcmp(node->name, "line")) {
      read_line_svg(node, dia);
      continue;
    }
    if (!strcmp(node->name, "ellipse")) {
      read_ellipse_svg(node, dia);
      continue;
    }
    if (!strcmp(node->name, "polyline")) {
      read_poly_svg(node, dia, "Standard - PolyLine");
      continue;
    }
    if (!strcmp(node->name, "polygon")) {
      read_poly_svg(node, dia, "Standard - Polygon");
      continue;
    }
    if(!strcmp(node->name, "text")) {
      read_text_svg(node, dia);
      continue;
    }
    if(!strcmp(node->name, "path")) {
      read_path_svg(node, dia);
      continue;
    }
  }
  xmlFreeDoc(doc);
  return TRUE;
}

/* interface from filter.h */

static const gchar *extensions[] = {"svg", NULL };
DiaImportFilter svg_import_filter = {
	N_("Scalable Vector Graphics"),
	extensions,
	import_svg
};
