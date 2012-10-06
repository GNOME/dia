/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998-2002 Alexander Larsson
 *
 * svg-import.c: SVG import filter for dia
 * Copyright (C) 2002 Steffen Macke
 * Copyright (C) 2005 Hans Breuer
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
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>

#include "intl.h"
#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "dia_xml_libxml.h"
#include "intl.h"
#include "dia_svg.h"
#include "create.h"
#include "group.h"
#include "font.h"
#include "attributes.h"

static gboolean import_svg (xmlDocPtr doc, DiagramData *dia, DiaContext *ctx, void* user_data);
static GList *read_ellipse_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list);
static GList *read_rect_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list);
static GList *read_line_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list);
static GList *read_poly_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list, char *object_type);
static GList *read_text_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list);
static GList *read_path_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list, DiaContext *ctx);
static GPtrArray *make_element_props(real xpos, real ypos, real width, real height);

/* TODO: use existing implementation in dia source */
static Color 
get_colour(gint32 c, real opacity)
{
    Color colour;
    colour.red   = ((c & 0xff0000) >> 16) / 255.0;
    colour.green = ((c & 0x00ff00) >> 8) / 255.0;
    colour.blue  =  (c & 0x0000ff) / 255.0;
    colour.alpha = opacity;

    return colour;
}

/* Dia default scale is 20px per cm; the user scale *should* be dynamic but this would involve a 
 *  much more complicated approach than implemented in this imported (transformations!)  
 */
const gdouble DEFAULT_SVG_SCALE = 20.0;
static gdouble user_scale = 20.0;

/*!
 * Read a numeric value from a string taking unit into account. 
 * The signature is the same as g_ascii_strtod but also reads 
 * the unit if there is one
 */
static gdouble
get_value_as_cm (const gchar *nptr,
		 gchar      **endptr)
{
    gchar *endp = NULL;
    gdouble val = 0.0;

    g_return_val_if_fail (nptr != NULL, 0.0);
    
    val = g_ascii_strtod (nptr, &endp);
    if (!endp || '\0' == *endp || ' ' == *endp || ',' == *endp || ';' == *endp)
        val /= user_scale;
    else if (strncmp(endp, "px", 2) == 0)
        val /= user_scale, endp+=2;
    else if (strncmp(endp, "cm", 2) == 0)
        val *= 1.0, endp+=2; /* nothing to scale */
    else if (strncmp(endp, "mm", 2) == 0)
        val /= 10.0, endp+=2;
    else if (strncmp(endp, "in", 2) == 0)
        val /= 2.54, endp+=2;
    else if (strncmp(endp, "pt", 2) == 0)
        val *= 0.03528, endp+=2;
    /* the rest can't really be resolved here, passing unit to caller (who will just ignore at the moment) */
    
    if (endptr)
        *endptr = endp;
	
    return val;
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
    { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH },
    { "line_style", PROP_TYPE_LINESTYLE},
    { "fill_colour", PROP_TYPE_COLOUR },
    { "show_background", PROP_TYPE_BOOL },
    PROP_DESC_END};

static PropDescription svg_element_prop_descs[] = {
    { "elem_corner", PROP_TYPE_POINT },
    { "elem_width", PROP_TYPE_REAL },
    { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END};

static PropDescription _arrow_prop_descs[] = {
    PROP_STD_START_ARROW,
    PROP_STD_END_ARROW,
    PROP_DESC_END
};

static void
reset_arrows (DiaObject *obj)
{
    GPtrArray *props;
    int i;

    props = prop_list_from_descs(_arrow_prop_descs,pdtpp_true);
    g_assert(props->len == 2);
    for (i = 0; i < 2; ++i)
        ((ArrowProperty *)g_ptr_array_index(props, 0))->arrow_data.type = ARROW_NONE;
    obj->ops->set_props(obj, props);
    prop_list_free(props);
}

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


/* <use/> has x and y attributes, use to position */
static void
use_position (DiaObject *obj, xmlNodePtr node)
{
    Point pos = {0, 0};
    xmlChar *str;
    Point delta = obj->position;

    str = xmlGetProp(node, (const xmlChar *)"x");
    if (str) {
        pos.x = get_value_as_cm((char *) str, NULL);
        xmlFree(str);
    }

    str = xmlGetProp(node, (const xmlChar *)"y");
    if (str) {
        pos.y = get_value_as_cm((char *) str, NULL);
        xmlFree(str);
    }
    /* not assuming the original is at 0,0 */
    pos.x += delta.x;
    pos.y += delta.y;
    obj->ops->move(obj, &pos);

    str = xmlGetProp(node, (const xmlChar *)"transform");
    if (str) {
        DiaMatrix *m = dia_svg_parse_transform ((char *)str, user_scale);

	if (m) {
	    GPtrArray *props = g_ptr_array_new ();
	    PointProperty *pp;
	    RealProperty  *pr;

	    prop_list_add_point (props, "obj_pos", &pos); 
	    prop_list_add_point (props, "elem_corner", &pos); 
	    prop_list_add_real (props, "elem_width", 1.0);
	    prop_list_add_real (props, "elem_height", 1.0);
	    prop_list_add_real (props, PROP_STDNAME_LINE_WIDTH, 0.1);
	    obj->ops->get_props (obj, props);
	    /* try to transform the object without the full matrix  */
	    pp = g_ptr_array_index (props, 0);
	    pp->point_data.x +=  m->x0;
	    pp->point_data.y +=  m->y0;
	    /* set position a second time, now for non-elements */
	    pp = g_ptr_array_index (props, 1);
	    pp->point_data.x +=  m->x0;
	    pp->point_data.y +=  m->y0;

	    pr = g_ptr_array_index (props, 2);
	    pr->real_data *= m->xx;
	    pr = g_ptr_array_index (props, 3);
	    pr->real_data *= m->yy;
	    pr = g_ptr_array_index (props, 4);
	    pr->real_data *= m->yy;
	    obj->ops->set_props (obj, props);

	    prop_list_free (props);
	    g_free (m);
	}
        xmlFree(str);
    }
}

/* apply SVG style to object */
static void
apply_style(DiaObject *obj, xmlNodePtr node, DiaSvgStyle *parent_style) 
{
      DiaSvgStyle *gs;
      GPtrArray *props;
      LinestyleProperty *lsprop;
      ColorProperty *cprop;
      RealProperty *rprop;
      BoolProperty *bprop;
      real scale = 1.0;

      xmlChar *str = xmlGetProp(node, (const xmlChar *)"transform");
      if (str) {
	  DiaMatrix *m = dia_svg_parse_transform ((char *)str, user_scale);
	  if (m) {
	    scale = m->xx;
	    g_free (m);
	  }
	  xmlFree(str);
      }
      gs = g_new0(DiaSvgStyle, 1);
      /* SVG defaults */
      dia_svg_style_init (gs, parent_style);
            
      dia_svg_parse_style(node, gs, user_scale);
      props = prop_list_from_descs(svg_style_prop_descs, pdtpp_true);
      g_assert(props->len == 5);
  
      cprop = g_ptr_array_index(props,0);
      if(gs->stroke == DIA_SVG_COLOUR_NONE) /* transparent */
	cprop->color_data = get_colour(0xFFFFFF, 0.0);
      else if (gs->stroke == DIA_SVG_COLOUR_FOREGROUND)
	cprop->color_data = attributes_get_foreground();
      else if (gs->stroke == DIA_SVG_COLOUR_BACKGROUND)
	cprop->color_data = attributes_get_background();
      else
        cprop->color_data = get_colour(gs->stroke, gs->stroke_opacity);

      rprop = g_ptr_array_index(props,1);
      rprop->real_data = gs->line_width * scale;
  
      lsprop = g_ptr_array_index(props,2);
      lsprop->style = gs->linestyle != DIA_SVG_LINESTYLE_DEFAULT ? gs->linestyle : LINESTYLE_SOLID;
      lsprop->dash = gs->dashlength * scale;

      cprop = g_ptr_array_index(props,3);
      if(gs->fill == DIA_SVG_COLOUR_NONE) /* transparent */
	cprop->color_data = get_colour(0x000000, 0.0);
      else if (gs->fill == DIA_SVG_COLOUR_FOREGROUND)
	cprop->color_data = attributes_get_foreground();
      else if (gs->fill == DIA_SVG_COLOUR_BACKGROUND)
	cprop->color_data = attributes_get_background();
      else
        cprop->color_data = get_colour(gs->fill, gs->fill_opacity);
      
      bprop = g_ptr_array_index(props,4);
      if(gs->fill == DIA_SVG_COLOUR_NONE) {
        bprop->bool_data = FALSE;
      } else {
	bprop->bool_data = TRUE;
      }

      obj->ops->set_props(obj, props);
      
      if (gs->font)
        dia_font_unref (gs->font);
      g_free(gs);
}

/* all the basic SVG elements allow their own transformation which
 * is not directly supported by Dia and also not especially useful 
 */
/* read a path */
static GList *
read_path_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list, DiaContext *ctx)
{
    DiaObjectType *otype;
    DiaObject *new_obj;
    Handle *h1, *h2;
    BezierCreateData *bcd;
    xmlChar *str;
    char *pathdata, *unparsed = NULL;
    GArray *bezpoints = NULL;
    gboolean closed = FALSE;
    gint i;
    DiaMatrix *matrix = NULL;
    Point current_point = {0.0, 0.0};
    gboolean use_stdpath = FALSE;

    str = xmlGetProp(node, (const xmlChar *)"transform");
    if (str) {
      matrix = dia_svg_parse_transform ((char *)str, user_scale);
      xmlFree (str);
    }

    str = xmlGetProp(node, (const xmlChar *)"d");
    pathdata = (char *)str;
    bezpoints = g_array_new(FALSE, FALSE, sizeof(BezPoint));
    g_array_set_size(bezpoints, 0);
    do {
      int first = bezpoints->len;
      if (!dia_svg_parse_path (bezpoints, pathdata, &unparsed, &closed, &current_point))
        break;

      if (!closed) {
	/* expensive way to possibly close the path */
	DiaSvgStyle *gs = g_new0(DiaSvgStyle, 1);
	BezPoint bp;

	dia_svg_style_init (gs, NULL);
	dia_svg_parse_style(node, gs, user_scale);
	if (gs->font)
          dia_font_unref (gs->font);
	closed = (gs->fill != DIA_SVG_COLOUR_NONE);
	/* if we close it here add an explicit line-to */
	if (closed) {
	  bp.type = BEZ_LINE_TO;
	  bp.p1 = g_array_index(bezpoints, BezPoint, first).p1;
	  g_array_append_val (bezpoints, bp);
	}
        g_free(gs);
      }
      if (unparsed) {
        use_stdpath = TRUE;
      } else if (bezpoints && bezpoints->len > 0) {
	/* A stray 'z' can produce extra runs without adding any new BEZ_MOVE_TO.
	 * To have the optimum representaion with Dia's objects we check again.
	 */
	if (use_stdpath) {
	  int move_tos = 0;
	  for (i = 0; i < bezpoints->len; ++i)
	    if (g_array_index(bezpoints, BezPoint, i).type == BEZ_MOVE_TO)
	      ++move_tos;
	  use_stdpath = (move_tos > 1);
	}
        if (g_array_index(bezpoints, BezPoint, 0).type != BEZ_MOVE_TO) {
          dia_context_add_message(ctx, _("Invalid path data.\n"
					 "svg:path data must start with moveto."));
	  break;
	} else if (use_stdpath) {
	  otype = object_get_type("Standard - Path");
        } else if (!closed)
	  otype = object_get_type("Standard - BezierLine");
        else
	  otype = object_get_type("Standard - Beziergon");

	if (otype == NULL){
	  dia_context_add_message(ctx, _("Can't find standard object"));
	  break;
        }
	bcd = g_new(BezierCreateData, 1);
	bcd->num_points = bezpoints->len;
	bcd->points = &(g_array_index(bezpoints, BezPoint, 0));
	/* dia_svg_parse_path does not scale to the user coordinate system, do it here */
	for (i = 0; i < bcd->num_points; ++i) {
	  bcd->points[i].p1.x /= user_scale;
	  bcd->points[i].p1.y /= user_scale;
	  bcd->points[i].p2.x /= user_scale;
	  bcd->points[i].p2.y /= user_scale;
	  bcd->points[i].p3.x /= user_scale;
	  bcd->points[i].p3.y /= user_scale;
	  if (matrix)
	    transform_bezpoint (&bcd->points[i], matrix);
	}
	new_obj = otype->ops->create(NULL, bcd, &h1, &h2);
	if (!closed)
	  reset_arrows (new_obj);
	g_free(bcd);
	apply_style(new_obj, node, parent_style);
	list = g_list_append (list, new_obj);

        g_array_set_size (bezpoints, 0);
      }
      pathdata = unparsed;
      unparsed = NULL;
    } while (pathdata);

    if (bezpoints)
      g_array_free (bezpoints, TRUE);
    if (matrix)
      g_free (matrix);

    xmlFree (str);

    return list;
}

/* read a text */
static GList *
read_text_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list) 
{
    DiaObjectType *otype = object_get_type("Standard - Text");
    DiaObject *new_obj;
    Handle *h1, *h2;
    Point point;
    GPtrArray *props;
    TextProperty *prop;
    xmlChar *str = NULL;
    gchar *multiline = NULL;
    DiaSvgStyle *gs;
    gboolean any_tspan = FALSE;
    DiaMatrix *matrix = NULL;
    real font_height = 0.0;

    str = xmlGetProp(node, (const xmlChar *)"transform");
    if (str) {
      matrix = dia_svg_parse_transform ((char *)str, user_scale);
      xmlFree (str);
    }

    gs = g_new(DiaSvgStyle, 1);
    dia_svg_style_init (gs, parent_style);

    point.x = 0;
    point.y = 0;

    str = xmlGetProp(node, (const xmlChar *)"x");
    if (str) {
      point.x = get_value_as_cm((char *) str, NULL);
      xmlFree(str);
    }

    str = xmlGetProp(node, (const xmlChar *)"y");
    if (str) {
      point.y = get_value_as_cm((char *) str, NULL);
      xmlFree(str);
    }

    /* font-size can be given in the style (with absolute unit) or
     * with it's own attribute. The latter is preferred - also by
     * Dia's own export.
     */
    str = xmlGetProp(node, (const xmlChar *)"font-size");
    if (str) {
      font_height = get_value_as_cm((char *) str, NULL);
      xmlFree(str);
    }

    str = xmlGetProp(node, (const xmlChar *)"text-anchor");
    if (str) {
      if (xmlStrcmp(str, (const xmlChar*)"middle") == 0)
        gs->alignment = ALIGN_CENTER;
      else if (xmlStrcmp(str, (const xmlChar*)"end") == 0)
        gs->alignment = ALIGN_RIGHT;
      else if (xmlStrcmp(str, (const xmlChar*)"start") == 0)
        gs->alignment = ALIGN_LEFT;
      xmlFree(str);
    }

    {
      xmlNode *tspan = node->children;
      GString *paragraph = g_string_sized_new(512);
      while (tspan) {
        if (xmlStrcmp (tspan->name, (const xmlChar*)"tspan") == 0) {
          xmlChar *line = xmlNodeGetContent(tspan);
          if (any_tspan) /* every other line needs separation */
	    g_string_append(paragraph, "\n");
	  else /* only first time */
	    dia_svg_parse_style(tspan, gs, user_scale);
          g_string_append(paragraph, (gchar*)line);
	  xmlFree(line);
          any_tspan = TRUE;
        }        
        tspan = tspan->next;
      }
      multiline = paragraph->str;
      g_string_free (paragraph, FALSE);
      str = NULL;
    }
    if (!any_tspan) {
      str = xmlNodeGetContent(node);
    }
    if(str || multiline) {
      if (matrix) {
        /* TODO: transform the text, too - when it is supported */
	transform_point (&point, matrix);
	g_free (matrix);
      }
      new_obj = otype->ops->create(&point, otype->default_user_data,
				 &h1, &h2);
      list = g_list_append (list, new_obj);

      props = prop_list_from_descs(svg_text_prop_descs, pdtpp_true);
      g_assert(props->len == 1);

      dia_svg_parse_style(node, gs, user_scale);    
      if(gs->font == NULL) {
	gs->font = dia_font_new_from_legacy_name("Courier");
      }
      prop = g_ptr_array_index(props, 0);
      g_free(prop->text_data);
      prop->text_data = str ? g_strdup((char *) str) : multiline;
      xmlFree(str);
      prop->attr.alignment = gs->alignment;
      prop->attr.position.x = point.x;
      prop->attr.position.y = point.y;
      /* FIXME: looks like a leak but without this an imported svg is 
       * crashing on release */
      prop->attr.font = dia_font_ref (gs->font);
      if (font_height > 0.0) {
        /* font-size should be the line-height according to SVG spec,
	 * but see node_set_text_style() - round-trip first */
	real font_scale = dia_font_get_height (prop->attr.font) / dia_font_get_size (prop->attr.font); 
        prop->attr.height = font_height * font_scale;
      } else
        prop->attr.height = gs->font_height;
      /* when operating with default values foreground and background are intentionally swapped
       * to avoid getting white text by default */
      switch (gs->fill) {
      case DIA_SVG_COLOUR_NONE :
        /* don't use -1 which would be almost white */
      case DIA_SVG_COLOUR_TEXT :
      case DIA_SVG_COLOUR_BACKGROUND :
        prop->attr.color = attributes_get_foreground();
	break;
      case DIA_SVG_COLOUR_FOREGROUND :
        prop->attr.color = attributes_get_background();
	break;
      default :
        prop->attr.color = get_colour (gs->fill, gs->fill_opacity);
	break;
      }
      new_obj->ops->set_props(new_obj, props);
      prop_list_free(props);
    }
    if (gs->font)
      dia_font_unref (gs->font);
    g_free(gs);

    return list;
}

/* read a polygon or a polyline */
static GList *
read_poly_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list, char *object_type) 
{
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
    DiaMatrix *matrix = NULL;

    str = xmlGetProp(node, (const xmlChar *)"transform");
    if (str) {
      matrix = dia_svg_parse_transform ((char *)str, user_scale);
      xmlFree (str);
    }
    
    str = xmlGetProp(node, (const xmlChar *)"points");
    tmp = (char *) str;
    while (tmp[0] != '\0') {
      /* skip junk */
      while (tmp[0] != '\0' && !g_ascii_isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
	tmp++;
      if (tmp[0] == '\0') break;
      val = get_value_as_cm(tmp, &tmp);
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
      if (matrix)
        transform_point (&points[i], matrix);
    }
    g_array_free(arr, TRUE);
    if (matrix)
      g_free (matrix);
  
    pcd->points = points;
    new_obj = otype->ops->create(NULL, pcd,
				 &h1, &h2);
    reset_arrows (new_obj);
    apply_style(new_obj, node, parent_style);
    list = g_list_append (list, new_obj);
    g_free(points);
    g_free(pcd);

    return list;
}

/* read an ellipse or circle */
static GList *
read_ellipse_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list) 
{
  xmlChar *str;
  real width = 0.0, height = 0.0;
  DiaObjectType *otype = object_get_type("Standard - Ellipse");
  DiaObject *new_obj;
  Handle *h1, *h2;
  GPtrArray *props;
  Point start = {0, 0};
  DiaMatrix *matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = dia_svg_parse_transform ((char *)str, user_scale);
    xmlFree (str);
  }
  
  str = xmlGetProp(node, (const xmlChar *)"cx");
  if (str) {
    start.x = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"cy");
  if (str) {
    start.y = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"rx");
  if (str) {
    width = get_value_as_cm((char *) str, NULL)*2;
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"ry");
  if (str) {
    height = get_value_as_cm((char *) str, NULL)*2;
    xmlFree(str);
  }
  /* not part of ellipse attributes, just here for circle */
  str = xmlGetProp(node, (const xmlChar *)"r");
  if (str) {
    width = height = get_value_as_cm((char *) str, NULL)*2;
    xmlFree(str);
  }
  if (matrix) {
    /* TODO: transform angle - when it is supported */
    Point wh = {width, height};
    transform_point (&wh, matrix);
    width = wh.x;
    height = wh.y;
    transform_point (&start, matrix);
    g_free (matrix);
  }
  /* A negative value is an error [...]. A value of zero disables rendering of the element. */
  if (width <= 0.0 || height <= 0.0)
    return list;
  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  apply_style(new_obj, node, parent_style);			
	 
  props = make_element_props(start.x-(width/2), start.y-(height/2),
                             width, height);
  new_obj->ops->set_props(new_obj, props);
  prop_list_free(props);
  return g_list_append (list, new_obj);
}

/* read a line */
static GList *
read_line_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list) 
{
  xmlChar *str;
  DiaObjectType *otype = object_get_type("Standard - Line");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  GPtrArray *props;
  Point start, end;
  DiaMatrix *matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = dia_svg_parse_transform ((char *)str, user_scale);
    xmlFree (str);
  }

  str = xmlGetProp(node, (const xmlChar *)"x1");
  if (str) {
    start.x = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else
    start.x = 0.0;
  str = xmlGetProp(node, (const xmlChar *)"y1");
  if (str) {
    start.y = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else
    start.y = 0.0;
  str = xmlGetProp(node, (const xmlChar *)"x2");
  if (str) {
    end.x = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else
    end.x = start.x;
  str = xmlGetProp(node, (const xmlChar *)"y2");
  if (str) {
    end.y = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else
    end.y = start.y;

  if (matrix) {
    transform_point (&start, matrix);
    transform_point (&end, matrix);
    g_free (matrix);
  }

  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  
  props = prop_list_from_descs(svg_line_prop_descs,pdtpp_true);
  g_assert(props->len == 2);

  ptprop = g_ptr_array_index(props,0);
  ptprop->point_data = start;

  ptprop = g_ptr_array_index(props,1);
  ptprop->point_data = end;

  new_obj->ops->set_props(new_obj, props);
  reset_arrows (new_obj);

  prop_list_free(props);

  apply_style(new_obj, node, parent_style);
  
  return g_list_append (list, new_obj);
}

/* read a rectangle */
static GList *
read_rect_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list) 
{
  xmlChar *str;
  real width = 0.0, height = 0.0;
  DiaObjectType *otype = object_get_type("Standard - Box");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  RealProperty *rprop;
  GPtrArray *props;
  Point start,end;
  real corner_radius = 0.0;
  DiaMatrix *matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = dia_svg_parse_transform ((char *)str, user_scale);
    xmlFree (str);
  }

  str = xmlGetProp(node, (const xmlChar *)"x");
  if (str) {
    start.x = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else 
    start.x = 0.0;
  str = xmlGetProp(node, (const xmlChar *)"y");
  if (str) {
    start.y = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } else 
    start.y = 0.0;
  str = xmlGetProp(node, (const xmlChar *)"width");
  if (str) {
    width = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"height");
  if (str) {
    height = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"rx");
  if (str) {
    corner_radius = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"ry");
  if (str) {
    if(corner_radius != 0.0) {
      /* calculate the mean value of rx and ry */
      corner_radius = (corner_radius+get_value_as_cm((char *) str, NULL))/2;
    } else {
      corner_radius = get_value_as_cm((char *) str, NULL);
    }
    xmlFree(str);
  }

  if (matrix) {
    /* TODO: for rotated rects we would need to create a polygon */
    transform_point (&start, matrix);
    transform_point (&end, matrix);
    g_free (matrix);
  }
  /* A negative value is an error [...]. A value of zero disables rendering of the element. */
  if (width <= 0.0 || height <= 0.0)
    return list; /* just ignore it w/o much complaints */
  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  list = g_list_append (list, new_obj);
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
  
  apply_style(new_obj, node, parent_style);
  prop_list_free(props);

  return list;
}

static GList *
read_image_svg(xmlNodePtr node, DiaSvgStyle *parent_style, GList *list, const gchar *filename_svg)
{
  xmlChar *str;
  real x = 0, y = 0, width = 0, height = 0;
  DiaObject *new_obj;
  DiaMatrix *matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = dia_svg_parse_transform ((char *)str, user_scale);
    xmlFree (str);
  }

  str = xmlGetProp(node, (const xmlChar *)"x");
  if (str) {
    x = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } 
  str = xmlGetProp(node, (const xmlChar *)"y");
  if (str) {
    y = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  } 
  str = xmlGetProp(node, (const xmlChar *)"width");
  if (str) {
    width = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  str = xmlGetProp(node, (const xmlChar *)"height");
  if (str) {
    height = get_value_as_cm((char *) str, NULL);
    xmlFree(str);
  }
  /* TODO: aspect ratio? */

  if (matrix) {
    /* TODO: transform angle - when it is supported */
    Point xy = {x, y};
    Point wh = {width, height};

    transform_point (&xy, matrix);
    transform_point (&wh, matrix);
    width = wh.x;
    height = wh.y;
    x = xy.x;
    y = xy.y;

    g_free (matrix);
  }

  str = xmlGetProp(node, (const xmlChar *)"xlink:href");
  if (!str) /* this doesn't look right but it appears to work w/o namespace --hb */
    str = xmlGetProp(node, (const xmlChar *)"href");
  if (str) {
    if (strncmp ((char *)str, "data:image/", 11) == 0) {
      /* inline data - skip format description like: data:image/png;base64, */
      const char* data = strchr((char *)str, ',');
      
      if (data) {
        GdkPixbuf *pixbuf = pixbuf_decode_base64 (data+1);

	if (pixbuf) {
	  ObjectChange *change;

	  new_obj = create_standard_image(x, y, width, height, NULL);
	  change = dia_object_set_pixbuf (new_obj, pixbuf);
	  if (change) /* throw it away, noone needs it here  */
	    change->free (change);
	  g_object_unref (pixbuf);
	}
      }
    } else {
      gchar *filename = g_filename_from_uri((gchar *) str, NULL, NULL);
      if (!filename || !g_path_is_absolute (filename)) {
        /* if image file path is relative use the main path */
        gchar *dir = g_path_get_dirname (filename_svg);
        gchar *absfn = g_build_path (G_DIR_SEPARATOR_S, 
	                             dir, filename ? filename : (gchar *)str, NULL);

        g_free (filename);
        g_free (dir);
        filename = absfn;
      }
      new_obj = create_standard_image(x, y, width, height, filename ? filename : "<broken>");
      g_free(filename);
    }
    xmlFree(str);
  }

  return g_list_append (list, new_obj);
}

/* GFunc for foreach */
static void
add_def (gpointer       data,
         gpointer       user_data)
{
  DiaObject  *obj = (DiaObject *)data;
  GHashTable *defs_ht = (GHashTable *)user_data;
  gchar *id = dia_object_get_meta (obj, "id");
  if (id) { /* pass ownership of name and object */
    g_hash_table_insert (defs_ht, id, obj);
  } else {
    obj->ops->destroy (obj);
    g_free (obj);
  }
}

/*!
 * Fill a GList* with objects which is to be put in a
 * diagram or a group by the caller. 
 * Can be called recusively to allow groups in groups.
 *
 * @param startnode the XML node to dive into
 * @param parent_gs the graphic style inherited by parent
 * @param defs_ht a map of objects filled from 'defs' to use as templates for 'use'
 * @param filename_svg SVG filename for better error messages
 * @param ctx context to keep error messages grouped
 * @return a list of _DiaObject
 */
static GList*
read_items (xmlNodePtr   startnode, 
	    DiaSvgStyle *parent_gs,
	    GHashTable  *defs_ht,
	    const gchar *filename_svg,
	    DiaContext  *ctx)
{
  xmlNodePtr node;
  GList *items = NULL;


  for (node = startnode; node != NULL; node = node->next) {
    /* Points to the *first* object created by this node (used to be the last one).
     * Dia may split a single SVG element into multiple DiaObjects. If so these objects
     * should be grouped or at least inherit all the same node properties
     */
    DiaObject *obj = NULL;

    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE) continue;

    if (!xmlStrcmp(node->name, (const xmlChar *)"g")) {
      GList *moreitems;
      DiaSvgStyle *group_gs;
      DiaMatrix *matrix = NULL;
      xmlChar *trans;

      /* We need to have/apply the groups style before the objects style */
      group_gs = g_new0 (DiaSvgStyle, 1);
      dia_svg_style_init (group_gs, parent_gs);
      dia_svg_parse_style (node, group_gs, user_scale);

      trans = xmlGetProp (node, (xmlChar *)"transform");
      if (trans) {
        matrix = dia_svg_parse_transform ((char *)trans, user_scale);
	xmlFree (trans);
      }

      moreitems = read_items (node->xmlChildrenNode, group_gs, defs_ht, filename_svg, ctx);

      if (moreitems) {
        DiaObject *group;

	if (matrix) {
	  group = group_create_with_matrix (moreitems, matrix);
	  matrix = NULL;
	} else
	  group = group_create (moreitems);
	/* group eats list */
        items = g_list_append (items, group);
	/* remember for meta */
	obj = group;
      }
      if (group_gs->font)
        dia_font_unref (group_gs->font);
      g_free (group_gs);
      g_free (matrix);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"symbol")) {
      /* ignore ‘viewBox’ and ‘preserveAspectRatio’ */
      GList *moreitems = read_items (node->xmlChildrenNode, parent_gs, defs_ht, filename_svg, ctx);

      /* only one object or create a group */
      if (g_list_length (moreitems) > 1)
	obj = group_create (moreitems);
      else if (moreitems)
	obj = g_list_last(moreitems)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"rect")) {
      items = read_rect_svg(node, parent_gs, items);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"line")) {
      items = read_line_svg(node, parent_gs, items);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"ellipse") || !xmlStrcmp(node->name, (const xmlChar *)"circle")) {
      items = read_ellipse_svg(node, parent_gs, items);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polyline")) {
      /* Uh, oh, no : apparently a fill="" in a group above make this a polygon */
      items = read_poly_svg(node, parent_gs, items, parent_gs && parent_gs->fill >= 0 ?
                            "Standard - Polygon" : "Standard - PolyLine");
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polygon")) {
      items = read_poly_svg(node, parent_gs, items, "Standard - Polygon");
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"text")) {
      items = read_text_svg(node, parent_gs, items);
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"path")) {
      /* the path element might be split into multiple objects */
      int first = g_list_length (items);
      items = read_path_svg(node, parent_gs, items, ctx);
      if (items && g_list_nth(items, first))
	obj = g_list_nth(items, first)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"image")) {
      items = read_image_svg(node, parent_gs, items, filename_svg);
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"defs")) {
      /* everything below must have a name to make a difference */
      DiaObject *otemp;
      GList *list, *defs = read_items (node->xmlChildrenNode, parent_gs, defs_ht, filename_svg, ctx);

      for (list = defs; list != NULL; list = g_list_next (list)) {
#if 0
	gchar *id;

	otemp = list->data;
	id = dia_object_get_meta (otemp, "id");
	if (id) {
	  /* pass ownership of name and object */
	  g_hash_table_insert (defs_ht, id, otemp);
	} else if (IS_GROUP (otemp)) {
	  /* defs in groups, I don't get the benefit */
	  GList *moredefs = group_objects (otemp);

	  g_list_foreach (moredefs, add_def, defs_ht);
	  group_destroy_shallow (otemp);
	} else {
	  /* just loose the object */
	  otemp->ops->destroy (otemp);
	  g_free (otemp);
	  list->data = NULL;
	}
#endif
      }
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"use")) {
      xmlChar *key = xmlGetProp (node, (const xmlChar *)"xlink:href");
      
      if (!key) /* this doesn't look right but ... */
        key = xmlGetProp(node, (const xmlChar *)"href");

      if (key && key[0] == '#') {
	DiaObject *otemp = g_hash_table_lookup (defs_ht, key+1);

	if (otemp) {
	  DiaObject *onew = otemp->ops->copy (otemp);

	  use_position (onew, node);
	  apply_style (onew, node, parent_gs);
	  items = g_list_append (items, onew);
	}
	xmlFree (key);
      }
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"pattern")) {
      /* Patterns could be considered as groups, too But Dia does not
       * have the facility to apply them (yet?). */
    } else {
      /* everything else is treated like a group _without grouping_, i.e. we dive into unknown stuff */
      /* this allows to import stuff generated by 'dot' with links for the nodes */
      GList *moreitems;
      /* on of the non-grouping elements is <a>, extract possible links */
      xmlChar *href = xmlGetProp (node, (const xmlChar *)"href");

      moreitems = read_items (node->xmlChildrenNode, parent_gs, defs_ht, filename_svg, ctx);
      if (moreitems) {
	if (href) {
	  GList *subs;

	  for (subs = moreitems; subs != NULL; subs = g_list_next(subs)) {
	    DiaObject *sub = subs->data;

	    dia_object_set_meta (sub, "url", (char *)href);
	  }
	}
	obj = moreitems->data;
        items = g_list_concat (items, moreitems);
      }
      if (href)
	xmlFree (href);
    }
    /* remember some additional stuff of the current object */
    if (g_list_find (items, obj) && g_list_length (g_list_find (items, obj)) > 1)
      g_print ("Todo: group or set props ...\n");
    if (obj) {
      xmlChar *val = xmlGetProp (node, (const xmlChar *)"id");
      if (val) {
	dia_object_set_meta (obj, "id", (char *)val);
	if (defs_ht) /* FIXME: adding everything with id */
	  g_hash_table_insert (defs_ht, g_strdup ((char *)val), obj);
	xmlFree (val);
      }
    }
  }
  return items;
}

static gboolean
_parse_shape_cp (xmlNodePtr node, real *x, real *y, gboolean *mcp)
{
  xmlChar *sx = xmlGetProp (node, (const xmlChar *)"x");
  xmlChar *sy = xmlGetProp (node, (const xmlChar *)"y");
  xmlChar *sm = xmlGetProp (node, (const xmlChar *)"main");
  gboolean ret = FALSE;

  if (sx && sy) {
    *x = g_ascii_strtod ((const char *)sx, NULL);
    *y = g_ascii_strtod ((const char *)sy, NULL);
    *mcp = (sm ? strcmp (sm, "yes") == 0 : FALSE);
    return TRUE;
  }
  if (sx) xmlFree (sx);
  if (sy) xmlFree (sy);
  if (sm) xmlFree (sm);
  return ret;
}

static gboolean
import_shape_info (xmlNodePtr start_node, DiagramData *dia, DiaContext *ctx)
{
  const DiaObjectType *ot_cp = object_get_type ("Shape Design - Connection Point");
  const DiaObjectType *ot_mp = object_get_type ("Shape Design - Main Connection Point");
  xmlNodePtr node;
  Layer *layer;

  if (!ot_cp || !ot_mp) {
    dia_context_add_message (ctx, _("'Shape Design' shapes missing."));
    return FALSE;
  }
  layer = new_layer (g_strdup ("Shape Design"), dia);
  data_add_layer (dia, layer);

  for (node = start_node; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE) continue;
    if (!xmlStrcmp(node->name, (const xmlChar *)"name")) {
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"icon")) {
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"aspectratio")) {
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"connections")) {
      xmlNodePtr cp_node;

      for (cp_node = node->xmlChildrenNode; cp_node != NULL; cp_node = cp_node->next) {
	Point pos;
	gboolean is_main = FALSE;

	if (_parse_shape_cp (cp_node, &pos.x, &pos.y, &is_main)) {
	  Handle *h1, *h2;
	  const DiaObjectType *ot = is_main ? ot_mp : ot_cp;
	  DiaObject *o;
	  
	  o = ot->ops->create (&pos, ot->default_user_data, &h1, &h2);

	  if (o) {
	    /* we have to adjust the position to be centered */
	    g_return_val_if_fail (o->num_handles == 8, FALSE);
	    pos.x -= (o->handles[1]->pos.x - o->handles[0]->pos.x);
	    pos.y -= (o->handles[3]->pos.y - o->handles[0]->pos.y);
	    o->ops->move (o, &pos);
	    layer_add_object (layer, o);
	  } else {
	    dia_context_add_message (ctx, _("Object '%s' creation failed"), ot->name);
	  }
	}
      }
    } else if (   !xmlStrcmp(node->name, (const xmlChar *)"default-width")
	       || !xmlStrcmp(node->name, (const xmlChar *)"default-height")) {
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"textbox")) {
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"can-parent")) {
    }
  }
  return TRUE;
}

gboolean
import_memory_svg (const guchar *p, guint size, DiagramData *dia,
		   DiaContext *ctx, void *user_data)
{
  xmlDocPtr doc = xmlParseMemory (p, size);

  if (!doc) {
    xmlErrorPtr err = xmlGetLastError ();

    dia_context_add_message(ctx, _("Parse error for memory block.\n%s"), err->message);
    return FALSE;
  }
  return import_svg (doc, dia, ctx, user_data);
}

/* imports the given SVG file, returns TRUE if successful */
gboolean
import_file_svg(const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data) 
{
  xmlDocPtr doc = xmlDoParseFile(filename);

  if (!doc) {
    dia_context_add_message(ctx, _("Parse error for %s"), 
		            dia_context_get_filename (ctx));
    return FALSE;
  }
  return import_svg (doc, dia, ctx, user_data);
}

gboolean
import_svg (xmlDocPtr doc, DiagramData *dia,
	    DiaContext *ctx, void *user_data)
{
  xmlNsPtr svg_ns;
  xmlNodePtr root;
  xmlNodePtr shape_root = NULL;
  GList *items, *item;
  guint num_items = 0;

  /* skip (emacs) comments */
  root = doc->xmlRootNode;
  while (root && (root->type != XML_ELEMENT_NODE)) root = root->next;
  if (!root) return FALSE;
  if (xmlIsBlankNode(root)) return FALSE;

  if (xmlSearchNsByHref(doc, root, (const xmlChar *)"http://www.daa.com.au/~james/dia-shape-ns") != NULL)
    shape_root = root;

  if (!(svg_ns = xmlSearchNsByHref(doc, root, (const xmlChar *)"http://www.w3.org/2000/svg"))) {
    /* correct filetype vs. robust import */
#if 0
    xmlFreeDoc(doc);
    return FALSE;
#else
    dia_context_add_message(ctx, _("Expected SVG Namespace not found in file"));
#endif
  }
  /* search for some svg in the file, this allows us to read the
   * svg part of our own shape file ...
   */
  if (svg_ns && root->ns != svg_ns) {
    xmlNodePtr node = root->xmlChildrenNode;

    while (node) {
      if (node->ns == svg_ns)
        break;
      node = node->next;
    }
    /* changing 'root' to svg */
    if (node)
      root = node;
  }

  if (root->ns != svg_ns && 0 != xmlStrcmp(root->name, (const xmlChar *)"svg")) {
    dia_context_add_message(ctx, _("root element was '%s' -- expecting 'svg'."), root->name);
    xmlFreeDoc(doc);
    return FALSE;
  }

  /* the following calls rely on the fact that noone messed with the original scale */
  if (shape_root)
    user_scale = 1.0;
  else
    user_scale = DEFAULT_SVG_SCALE;

  /* if the svg root element contains width, height and viewBox calculate our user scale from it */
  if (shape_root)
  {
    xmlChar *swidth = xmlGetProp(root, (const xmlChar *)"width");
    xmlChar *sheight = xmlGetProp(root, (const xmlChar *)"height");
    xmlChar *sviewbox = xmlGetProp(root, (const xmlChar *)"viewBox");

    if (swidth && sheight && sviewbox) {
      real width = get_value_as_cm ((const char *)swidth, NULL);
      real height = get_value_as_cm ((const char *)sheight, NULL);
      gint x1 = 0, y1 = 0, x2 = 0, y2 = 0;
      
      if (4 == sscanf ((const char *)sviewbox, "%d %d %d %d", &x1, &y1, &x2, &y2)) {
        real xs, ys;
	g_debug ("viewBox(%d %d %d %d) = (%f,%f)\n", x1, y1, x2, y2, width, height);
        /* some basic sanity check */
	if (x2 > x1 && y2 > y1 && width > 0 && height > 0) {
	  xs = ((real)x2 - x1) / width;
	  ys = ((real)y2 - y1) / height;
	  /* plausibility check, strictly speaking these are not required to be the same 
	       * /or/ are they and Dia is writting a bogus viewBox?
	       */
	  if (fabs((fabs (xs/ys) - 1.0) < 0.1)) {
	    user_scale = xs;
	    g_debug ("viewBox(%d %d %d %d) scaling (%f,%f) -> %f\n", x1, y1, x2, y2, xs, ys, user_scale);
	  }
        }
      }
    }
    
    if (swidth)
      xmlFree (swidth);
    if (sheight)
      xmlFree (sheight);
    if (sviewbox)
      xmlFree (sviewbox);
  }

  {
    GHashTable *defs_ht = g_hash_table_new (g_str_hash, g_str_equal);
    items = read_items (root->xmlChildrenNode, NULL, defs_ht, dia_context_get_filename(ctx), ctx);
    g_hash_table_destroy (defs_ht);
  }
  /* Every top level item which is a group with a name/id we are converting
   * back to a layer. This is consistent with our SVG export and does
   * also work with layers from Sodipodi/Inkscape/iDraw/...
   * But if there is just one item - even if a group - it is put into the active layer.
   */
  num_items = g_list_length (items);
  for (item = items; item != NULL; item = g_list_next (item)) {
    DiaObject *obj = (DiaObject *)item->data;
    gchar *name = NULL;

    if (num_items > 1 && IS_GROUP(obj) && ((name = dia_object_get_meta (obj, "id")) != NULL)) {
      DiaObject *group = (DiaObject *)item->data;
      /* new_layer() is taking ownership of the name */
      Layer *layer = new_layer (g_strdup (name), dia);

      /* layer_add_objects() is taking ownersip of the list */
      layer_add_objects (layer, g_list_copy (group_objects (group)));
      data_add_layer (dia, layer);
      group_destroy_shallow (group);
      g_free (name);
    } else {
      /* Just as before: throw it in the active layer */
      DiaObject *obj = (DiaObject *)item->data;
      layer_add_object(dia->active_layer, obj);
      layer_update_extents(dia->active_layer);
    }

  }

  g_list_free (items);

  if (shape_root)
    import_shape_info (shape_root->xmlChildrenNode, dia, ctx);

  xmlFreeDoc(doc);
  /* set 'display' setting */
  g_object_set_data (G_OBJECT(dia), "show-connection-points", GINT_TO_POINTER(-1));

  return TRUE;
}

/* interface from filter.h */

static const gchar *extensions[] = {"svg", NULL };
DiaImportFilter svg_import_filter = {
	N_("Scalable Vector Graphics"),
	extensions,
	import_file_svg,
	NULL, /* user_data */
	"dia-svg",
	0, /* flags */
	import_memory_svg
};
