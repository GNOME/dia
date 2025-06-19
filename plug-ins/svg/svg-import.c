/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998-2002 Alexander Larsson
 *
 * svg-import.c: SVG import filter for dia
 * Copyright (C) 2002 Steffen Macke
 * Copyright (C) 2005-2014 Hans Breuer
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

#define G_LOG_DOMAIN "DiaSVG"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>

#include "geometry.h"
#include "filter.h"
#include "object.h"
#include "properties.h"
#include "propinternals.h"
#include "dia_svg.h"
#include "create.h"
#include "group.h"
#include "font.h"
#include "attributes.h"
#include "pattern.h"
#include "dia-graphene.h"
#include "dia-io.h"
#include "dia-layer.h"

/**
 * SECTION:svg-import
 *
 * Import SVG
 *
 * Based on \ref DiaSvg this plug-in translates SVG to Dia \ref StandardObjects.
 *
 * Sometimes SVG import is the only way to create complex objects ...
 */

static gboolean import_svg (xmlDocPtr doc, DiagramData *dia, DiaContext *ctx, void* user_data);
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

/*!
 * \brief SVG Default Scale
 * Dia default scale is 20px per cm; the user scale *should* be dynamic but this would
 * involve a  much more complicated approach than implemented in this importer, e.g.
 * full transformations and dynamic scale to view port (viewBox).
 * \ingroup SvgImport
 */
const gdouble DEFAULT_SVG_SCALE = 20.0;
static gdouble user_scale = 20.0;

/*!
 * Read a numeric value from a string taking unit into account.
 * The signature is the same as g_ascii_strtod but also reads
 * the unit if there is one
 * \ingroup SvgImport
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
        val *= 2.54, endp+=2;
    else if (strncmp(endp, "pt", 2) == 0)
        val *= (2.54/72.0), endp+=2;
    else if (strncmp(endp, "pc", 2) == 0)
        val *= (2.54/6.0), endp+=2;
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
    { "line_join", PROP_TYPE_ENUM },
    { "line_caps", PROP_TYPE_ENUM },
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

  props = prop_list_from_descs (_arrow_prop_descs, pdtpp_true);
  g_return_if_fail (props->len == 2);
  for (i = 0; i < 2; ++i) {
    ((ArrowProperty *) g_ptr_array_index (props, 0))->arrow_data.type = ARROW_NONE;
  }
  dia_object_set_properties (obj, props);
  prop_list_free (props);
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


static real
_node_get_real (xmlNodePtr node, const char *name, real defval)
{
    real val = defval;
    xmlChar *str = xmlGetProp(node, (const xmlChar *)name);

    if (str) {
        val = get_value_as_cm((char *) str, NULL);
        xmlFree(str);
    }

    return val;
}


static void
_transform_object (DiaObject *obj, DiaMatrix *m, DiaContext *ctx)
{
  if (!dia_object_transform (obj, m)) {
    dia_context_add_message (ctx,
                             _("Failed to apply transformation for '%s'"),
                             obj->type->name);
  }
}


/*!
 * \brief Translate an existing object to a new position
 * The tag 'use' has x and y attributes to position the used object
 * \ingroup SvgImport
 */
static void
use_position (DiaObject *obj, xmlNodePtr node, DiaContext *ctx)
{
  Point pos = {0, 0};
  xmlChar *str;
  Point delta = obj->position;

  pos.x = _node_get_real (node, "x", 0.0);
  pos.y = _node_get_real (node, "y", 0.0);
  /* not assuming the original is at 0,0 */
  pos.x += delta.x;
  pos.y += delta.y;
  dia_object_move (obj, &pos);

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    graphene_matrix_t *graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);
    DiaMatrix *m = g_new0 (DiaMatrix, 1);

    dia_matrix_from_graphene (m, graphene_matrix);

    if (m) {
      if (obj->ops->transform) {
        _transform_object (obj, m, ctx);
      } else {
        GPtrArray *props = g_ptr_array_new ();

        PointProperty *pp;
        RealProperty  *pr;

        /* setting obj_pos is pointless, it is read-only in all objects */
        prop_list_add_point (props, "obj_pos", &pos);
        prop_list_add_point (props, "elem_corner", &pos);
        prop_list_add_real (props, "elem_width", 1.0);
        prop_list_add_real (props, "elem_height", 1.0);

        obj->ops->get_props (obj, props);
        /* XXX: try to transform the object without the full matrix  */
        pp = g_ptr_array_index (props, 0);
        pp->point_data.x +=  m->x0;
        pp->point_data.y +=  m->y0;
        /* XXX: set position a second time, now for non-elements */
        pp = g_ptr_array_index (props, 1);
        pp->point_data.x +=  m->x0;
        pp->point_data.y +=  m->y0;

        pr = g_ptr_array_index (props, 2);
        pr->real_data *= m->xx;
        pr = g_ptr_array_index (props, 3);
        pr->real_data *= m->yy;

        dia_object_set_properties (obj, props);
        prop_list_free (props);
      }
      g_clear_pointer (&m, g_free);
    }
    xmlFree(str);
  }
}


/*!
 * \brief Lookup and apply CSS style
 *
 * This is a slow initial version of CSS support. It parse the style
 * string for every use rather than for every definition. A better/faster
 * variant might be to parse the style on definition time into a list
 * of stdprops which simply could be applied in the right order on use.
 *
 * Also missing support for:
 *  - class lists (which would make it even slower)
 *  - referenced style definitions (e.g. url())
 *  - fill-rule, overflow, clip, ....
 *
 * \ingroup SvgImport
 */
static void
_css_parse_style (DiaSvgStyle *s,
                  double       user_scale_arg,
                  char        *tag,
                  char        *klass,
                  char        *id,
                  GHashTable  *style_ht)
{
  char *style = NULL;

  /* always try and apply '*' */
  style = g_hash_table_lookup (style_ht, "*");
  if (style) {
    dia_svg_parse_style_string (s, user_scale_arg, style);
    style = NULL;
  }

  /* also type only style */
  style = g_hash_table_lookup (style_ht, tag);
  if (style) {
    dia_svg_parse_style_string (s, user_scale_arg, style);
    style = NULL;
  }

  /* build the key in order of importance */
  /* tag.class#id */
  if (id && klass) {
    char *key = g_strdup_printf ("%s.%s#%s", tag, klass, id);

    style = g_hash_table_lookup (style_ht, key);
    g_clear_pointer (&key, g_free);
    if (!style) {
      /* class#id */
      key = g_strdup_printf (".%s#%s", klass, id);
      style = g_hash_table_lookup (style_ht, key);
      g_clear_pointer (&key, g_free);
    }
  }
  if (!style && klass) {
    char *key = g_strdup_printf (".%s", klass);
    style = g_hash_table_lookup (style_ht, key);
    g_clear_pointer (&key, g_free);
  }

  /* apply most specific style from class lookup */
  if (style) {
    dia_svg_parse_style_string (s, user_scale_arg, style);
    style = NULL;
  }

  /* apply style from id */
  if (id) {
    char *key = g_strdup_printf ("#%s", id);
    style = g_hash_table_lookup (style_ht, key);
    if (style) {
      dia_svg_parse_style_string (s, user_scale_arg, style);
    }
    g_clear_pointer (&key, g_free);
    key = g_strdup_printf ("%s#%s", tag, id);
    style = g_hash_table_lookup (style_ht, key);
    if (style) {
      dia_svg_parse_style_string (s, user_scale_arg, style);
    }
    g_clear_pointer (&key, g_free);
  }
}


/*!
 * \brief from the given node derive the CSS style if any
 * \ingroup SvgImport
 */
static void
_node_css_parse_style (xmlNodePtr   node,
                       DiaSvgStyle *gs,
                       real         user_scale_arg,
                       GHashTable  *style_ht)
{
  if (g_hash_table_size (style_ht) > 0) {
    /* only do all these expensive variants if we have some style at all */
    xmlChar *id = xmlGetProp (node, (xmlChar *) "id");
    xmlChar *klass = xmlGetProp (node, (xmlChar *) "class");

    if (klass) {
      char **klasses = g_regex_split_simple ("[\\s,;]+",
                                             (char *) klass, 0, 0);
      int i = 0;
      while (klasses[i]) {
        _css_parse_style (gs, user_scale_arg, (char *) node->name,
                          klasses[i], (char *) id, style_ht);
        ++i;
      }
      g_strfreev (klasses);
    } else {
      _css_parse_style (gs, user_scale_arg, (char *) node->name,
                        (char *) klass, (char *) id, style_ht);
    }

    if (id) {
      xmlFree (id);
    }

    if (klass) {
      xmlFree (klass);
    }
  }
}


static void
_set_pattern_from_key (DiaObject    *obj,
                       BoolProperty *bprop,
                       GHashTable   *pattern_ht,
                       const char   *key)
{
  DiaObjectChange *change = NULL;
  DiaPattern *pattern = g_hash_table_lookup (pattern_ht, key);

  if (pattern) {
    change = dia_object_set_pattern (obj, pattern);
    /* activate "show_background" */
    bprop->bool_data = TRUE;
  }

  /* throw it away, no one needs it here  */
  g_clear_pointer (&change, dia_object_change_unref);
}


/**
 * apply_style
 *
 * Apply SVG style to object
 *
 * Styling with SVG is a complicated thing. The style can be given with:
 *  - single attributes on the node
 *  - a style attribute on the node
 *  - accumulation from parent objects/groups
 *  - a reference to further information in single or style attribute
 *  - inheritance from object type, class, id or a combination thereof
 *
 * This method uses all of this information to apply the best style
 * approximation possible with Dia's rendering model.
 */
static void
apply_style (DiaObject   *obj,
             xmlNodePtr   node,
             DiaSvgStyle *parent_style,
             GHashTable  *style_ht,
             GHashTable  *pattern_ht,
             gboolean     init)
{
  DiaSvgStyle *gs;
  GPtrArray *props;
  LinestyleProperty *lsprop;
  ColorProperty *cprop;
  RealProperty *rprop;
  BoolProperty *bprop;
  EnumProperty *eprop;
  real scale = 1.0;


  xmlChar *str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    graphene_matrix_t *graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);
    DiaMatrix *m = g_new0 (DiaMatrix, 1);

    dia_matrix_from_graphene (m, graphene_matrix);

    if (m) {
      transform_length (&scale, m);
      g_clear_pointer (&m, g_free);
      g_clear_pointer (&graphene_matrix, graphene_matrix_free);
    }

    xmlFree(str);
  }
  gs = g_new0(DiaSvgStyle, 1);
  /* SVG defaults */
  dia_svg_style_init (gs, parent_style);
  _node_css_parse_style (node, gs, user_scale, style_ht);
  dia_svg_parse_style(node, gs, user_scale);
  props = prop_list_from_descs(svg_style_prop_descs, pdtpp_true);
  g_assert(props->len == 7);

  cprop = g_ptr_array_index(props,0);
  if (gs->stroke == DIA_SVG_COLOUR_DEFAULT) {
    if (init) /* no stroke */
      cprop->color_data = get_colour(0xFFFFFF, 0.0);
    else /* leave it alone */
      cprop->common.experience |= PXP_NOTSET; /* no overwrite */
  } else if (gs->stroke == DIA_SVG_COLOUR_NONE) /* transparent */
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
  if (gs->linestyle != DIA_LINE_STYLE_DEFAULT) {
    lsprop->style = gs->linestyle;
  } else if (init) {
    lsprop->style = DIA_LINE_STYLE_SOLID;
  } else {
    lsprop->common.experience |= PXP_NOTSET; /* no overwrite */
  }
  lsprop->dash = gs->dashlength * scale;

  cprop = g_ptr_array_index(props,3);
  if (gs->fill == DIA_SVG_COLOUR_DEFAULT) {
    if (init)
      cprop->color_data = get_colour(0x000000, gs->fill_opacity); /* black */
    else
      cprop->common.experience |= PXP_NOTSET; /* no overwrite */
  } else if (gs->fill == DIA_SVG_COLOUR_NONE) /* transparent */
    cprop->color_data = get_colour(0x000000, 0.0);
  else if (gs->fill == DIA_SVG_COLOUR_FOREGROUND)
    cprop->color_data = attributes_get_foreground();
  else if (gs->fill == DIA_SVG_COLOUR_BACKGROUND)
    cprop->color_data = attributes_get_background();
  else
    cprop->color_data = get_colour(gs->fill, gs->fill_opacity);

  bprop = g_ptr_array_index(props,4);
  if(gs->fill == DIA_SVG_COLOUR_NONE || gs->fill_opacity == 0) {
    bprop->bool_data = FALSE;
  } else {
    if (init)
      bprop->bool_data = TRUE;
    else
      bprop->common.experience |= PXP_NOTSET; /* no overwrite */
  }

  /* apply pattern, gradient if any */
  str = xmlGetProp(node, (const xmlChar *)"fill");
  if (str) {
    const char *left = strstr ((const char*)str, "url(#");
    const char *right = strrchr ((const char*)str, ')');
    if (left && right) {
      char *key = g_strndup (left + 5, right - left - 5);
      _set_pattern_from_key (obj, bprop, pattern_ht, key);
      g_clear_pointer (&key, g_free);
    }
    xmlFree(str);
  } else if (gs->fill == DIA_SVG_COLOUR_NONE) {
    /* check the style again, it might contain a pattern */
    str = xmlGetProp(node, (const xmlChar*)"style");
    if (str) {
      const char *left = strstr ((const char*)str, "fill:url(#");
      const char *right = left ? strrchr (left, ')') : NULL;
      if (left && right) {
        char *key = g_strndup (left + 10, right - left - 10);
        _set_pattern_from_key (obj, bprop, pattern_ht, key);
        g_clear_pointer (&key, g_free);
      }
      xmlFree (str);
    }
  }

  eprop = g_ptr_array_index(props,5);
  if (gs->linejoin != DIA_LINE_JOIN_DEFAULT)
    eprop->enum_data = gs->linejoin;
  else
    eprop->common.experience |= PXP_NOTSET;

  eprop = g_ptr_array_index(props,6);
  if (gs->linecap != DIA_LINE_CAPS_DEFAULT)
    eprop->enum_data = gs->linecap;
  else
    eprop->common.experience |= PXP_NOTSET;

  dia_object_set_properties (obj, props);

  g_clear_object (&gs->font);
  g_clear_pointer (&gs, g_free);
}


/*!
 * \brief Elements (poly, path) can be closed style, too.
 *
 * Even though there is a dedicated type for polygon and a specific path data
 * element to close a path ('z') there are variants to close _Polyline and path
 * without them. This function checks the given styles to enable this closing.
 *
 * \ingroup SvgImport
 */
static gboolean
_node_closed_by_style (xmlNodePtr node, DiaSvgStyle *parent_style)
{
  xmlChar *str;
  gboolean closed;

  if (parent_style && parent_style->fill > 0 && !xmlHasProp (node, (const xmlChar *)"fill"))
    return TRUE;
  str = xmlGetProp (node, (const xmlChar *)"fill");
  if (!str)
    return FALSE;
  closed = xmlStrcmp(str, (const xmlChar *)"none") != 0;
  xmlFree (str);

  return closed;
}


/**
 * read_path_svg:
 *
 * Read a SVG path element
 *
 * This function parses a SVG path element into a Dia object. All
 * the heavy lifting is done with dia_svg_parse_path() which is called multiple
 * times but it's result is accumulated into a single object.
 *
 * The result of this method is a path represented in Dia as _Bezierline,
 * _Beziergon or _StdPath
 *
 * All the basic SVG elements allow their own transformation which
 * is not directly supported by Dia and also not especially useful.
 * For path elements it is just an easy transformation of all the
 * single points, so it is done here.
 */
static GList *
read_path_svg (xmlNodePtr   node,
               DiaSvgStyle *parent_style,
               GHashTable  *style_ht,
               GHashTable  *pattern_ht,
               GList       *list,
               DiaContext  *ctx)
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
  graphene_matrix_t *graphene_matrix = NULL;
  Point current_point = {0.0, 0.0};
  gboolean use_stdpath = FALSE;
  gboolean closed_by_style;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  closed_by_style = _node_closed_by_style (node, parent_style);
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
      closed = closed_by_style;
      /* if we close it here add an explicit line-to */
      if (closed) {
        BezPoint bp;

        bp.type = BEZ_LINE_TO;
        bp.p1 = g_array_index(bezpoints, BezPoint, first).p1;
        g_array_append_val (bezpoints, bp);
      }
    }
    if (unparsed) {
      use_stdpath = TRUE;
    } else if (bezpoints && bezpoints->len > 0) {
      /* A stray 'z' can produce extra runs without adding any new BEZ_MOVE_TO.
      * To have the optimum representation with Dia's objects we check again.
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
      } else if (!closed) {
        otype = object_get_type("Standard - BezierLine");
      } else if (bezpoints->len < 3) { /* error path: invalid input or parsing error? */
        /* Our beziergon can not handle less than three points
        * So line-to the first point again...
        */
        BezPoint bpz = g_array_index(bezpoints, BezPoint, 0);
        bpz.type = BEZ_LINE_TO;
        g_array_append_val(bezpoints, bpz);
        otype = object_get_type("Standard - Beziergon");
      } else {
        otype = object_get_type("Standard - Beziergon");
      }

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
      g_clear_pointer (&bcd, g_free);
      apply_style(new_obj, node, parent_style, style_ht, pattern_ht, TRUE);
      list = g_list_append (list, new_obj);

      g_array_set_size (bezpoints, 0);
    }
    pathdata = unparsed;
    unparsed = NULL;
  } while (pathdata);

  if (bezpoints) {
    g_array_free (bezpoints, TRUE);
  }

  if (matrix) {
    g_clear_pointer (&matrix, g_free);
  }

  xmlFree (str);

  return list;
}


/**
 * read_text_svg:
 *
 * Read a SVG text element
 */
static GList *
read_text_svg (xmlNodePtr   node,
               DiaSvgStyle *parent_style,
               GHashTable  *style_ht,
               GHashTable  *pattern_ht,
               GList       *list,
               DiaContext  *ctx)
{
  DiaObjectType *otype = object_get_type ("Standard - Text");
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
  graphene_matrix_t *graphene_matrix = NULL;
  real font_height = 0.0;
  gboolean preserve_space = xmlNodeGetSpacePreserve (node) > 0;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  gs = g_new(DiaSvgStyle, 1);
  dia_svg_style_init (gs, parent_style);

  point.x = _node_get_real (node, "x", 0.0);
  point.y = _node_get_real (node, "y", 0.0);

  /* text property handling is special, don't use apply_style() */
  _node_css_parse_style (node, gs, user_scale, style_ht);

  /* font-size can be given in the style (with absolute unit) or
    * with it's own attribute. The latter is preferred - also by
    * Dia's own export.
    */
  str = xmlGetProp(node, (const xmlChar *)"font-size");
  if (str) {
    font_height = get_value_as_cm ((char *) str, NULL);
    dia_clear_xml_string (&str);
  }

  /* parse from <text/> before looking at the first <tspan/> */
  dia_svg_parse_style(node, gs, user_scale);

  {
    xmlNode *tspan = node->children;
    GString *paragraph = g_string_sized_new(512);
    while (tspan) {
      if (xmlStrcmp (tspan->name, (const xmlChar*) "tspan") == 0) {
        xmlChar *line = xmlNodeGetContent (tspan);

        if (any_tspan) {
          /* every other line needs separation */
          g_string_append (paragraph, "\n");
        } else {
          /* only first time with user scale - but w/o matrix - that shall be
           * in effect from the context? */
          dia_svg_parse_style (tspan, gs, user_scale);
          point.x = _node_get_real (tspan, "x", point.x);
          point.y = _node_get_real (tspan, "y", point.y);
        }

        g_string_append (paragraph, (char *) line);
        dia_clear_xml_string (&line);
        any_tspan = TRUE;
      }
      tspan = tspan->next;
    }

    multiline = g_string_free (paragraph, FALSE);
  }

  if (!any_tspan) {
    str = xmlNodeGetContent (node);
    /* so no valid multiline */
    g_clear_pointer (&multiline, g_free);
  }

  if (str || multiline) {
    new_obj = otype->ops->create(&point, otype->default_user_data, &h1, &h2);
    list = g_list_append (list, new_obj);

    props = prop_list_from_descs(svg_text_prop_descs, pdtpp_true);
    g_assert(props->len == 1);

    if(gs->font == NULL) {
      gs->font = dia_font_new_from_legacy_name("Courier");
    }
    prop = g_ptr_array_index(props, 0);
    g_clear_pointer (&prop->text_data, g_free);
    prop->text_data = str ? g_strdup((char *) str) : multiline;
    if (!preserve_space) {
      prop->text_data = g_strstrip (prop->text_data); /* modifies in-place */
    }
    xmlFree(str);
    prop->attr.alignment = gs->alignment;
    prop->attr.position.x = point.x;
    prop->attr.position.y = point.y;
    /* FIXME: looks like a leak but without this an imported svg is
      * crashing on release */
    prop->attr.font = g_object_ref (gs->font);
    if (font_height > 0.0) {
      /* font-size should be the line-height according to SVG spec,
      * but see node_set_text_style() - round-trip first */
      real font_scale = dia_font_get_height (prop->attr.font) / dia_font_get_size (prop->attr.font);
      prop->attr.height = font_height * font_scale;
    } else {
      prop->attr.height = gs->font_height;
    }
    /* when operating with default values foreground and background are intentionally swapped
      * to avoid getting white text by default */
    switch (gs->fill) {
      case DIA_SVG_COLOUR_NONE :
        /* don't use -1 which would be almost white */
      case DIA_SVG_COLOUR_TEXT :
      case DIA_SVG_COLOUR_BACKGROUND :
        prop->attr.color = attributes_get_foreground();
        break;
      case DIA_SVG_COLOUR_DEFAULT: /* black */
        prop->attr.color = get_colour(0x000000, gs->fill_opacity);
        break;
      case DIA_SVG_COLOUR_FOREGROUND :
        prop->attr.color = attributes_get_background();
        break;
      default:
        prop->attr.color = get_colour (gs->fill, gs->fill_opacity);
        break;
    }
    dia_object_set_properties (new_obj, props);
    prop_list_free (props);
    if (matrix) {
      _transform_object (new_obj, matrix, ctx);
    }
  }
  g_clear_object (&gs->font);
  g_clear_pointer (&gs, g_free);
  g_clear_pointer (&matrix, g_free);

  return list;
}


/**
 * read_poly_svg:
 *
 * Read a polygon or a polyline
 *
 * Create a #Polyline or #Polygon from the SVG element at node.
 */
static GList *
read_poly_svg (xmlNodePtr   node,
               DiaSvgStyle *parent_style,
               GHashTable  *style_ht,
               GHashTable  *pattern_ht,
               GList       *list,
               char        *object_type)
{
  DiaObjectType *otype;
  DiaObject *new_obj;
  Handle *h1, *h2;
  MultipointCreateData *pcd;
  Point *points;
  GArray *arr = g_array_new (FALSE, FALSE, sizeof (double));
  double val, *rarr;
  xmlChar *str;
  char *tmp;
  int i;
  DiaMatrix *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);
    xmlFree (str);
  }

  /* Uh, oh, no : apparently a fill="" in a group above make this a polygon */
  if (_node_closed_by_style (node, parent_style))
    otype = object_get_type("Standard - Polygon");
  else
    otype = object_get_type(object_type);

  str = xmlGetProp(node, (const xmlChar *)"points");
  if (!str) {
    g_warning ("SVG: '%s' without points", node->name);
    g_clear_pointer (&matrix, g_free);
    return list;
  }
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

  /* If an odd number of coordinates is provided, then the element is in error, cut off below */
  points = g_new0 (Point, arr->len / 2);

  pcd = g_new0 (MultipointCreateData, 1);
  pcd->num_points = arr->len / 2;
  rarr = (double *) arr->data;
  for (i = 0; i < pcd->num_points; i++) {
    points[i].x = rarr[2*i];
    points[i].y = rarr[2*i+1];
    if (matrix) {
      transform_point (&points[i], matrix);
    }
  }
  g_array_free (arr, TRUE);
  g_clear_pointer (&matrix, g_free);

  pcd->points = points;
  new_obj = otype->ops->create (NULL, pcd, &h1, &h2);
  reset_arrows (new_obj);
  apply_style(new_obj, node, parent_style, style_ht, pattern_ht, TRUE);
  list = g_list_append (list, new_obj);
  g_clear_pointer (&points, g_free);
  g_clear_pointer (&pcd, g_free);

  return list;
}


/**
 * read_ellipse_svg:
 *
 * Read an ellipse or circle from SVG
 *
 * Creates only _Ellipse objects, but could set the 'circle property'.
 */
static GList *
read_ellipse_svg (xmlNodePtr   node,
                  DiaSvgStyle *parent_style,
                  GHashTable  *style_ht,
                  GHashTable  *pattern_ht,
                  GList       *list,
                  DiaContext  *ctx)
{
  xmlChar *str;
  real width, height;
  DiaObjectType *otype = object_get_type ("Standard - Ellipse");
  DiaObject *new_obj;
  Handle *h1, *h2;
  GPtrArray *props;
  Point start;
  DiaMatrix *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  start.x = _node_get_real (node, "cx", 0.0);
  start.y = _node_get_real (node, "cy", 0.0);

  width = _node_get_real (node, "rx", 0.0) * 2;
  height = _node_get_real (node, "ry", 0.0) * 2;
  /* not part of ellipse attributes, just here for circle */
  str = xmlGetProp(node, (const xmlChar *)"r");
  if (str) {
    width = height = get_value_as_cm((char *) str, NULL)*2;
    xmlFree(str);
  }
  /* A negative value is an error [...]. A value of zero disables rendering of the element. */
  if (width <= 0.0 || height <= 0.0)
    return list;
  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);
  apply_style(new_obj, node, parent_style, style_ht, pattern_ht, TRUE);

  props = make_element_props(start.x-(width/2), start.y-(height/2),
			     width, height);
  dia_object_set_properties(new_obj, props);
  if (matrix) {
    _transform_object (new_obj, matrix, ctx);
    g_clear_pointer (&matrix, g_free);
  }
  prop_list_free(props);
  return g_list_append (list, new_obj);
}


/**
 * read_line_svg:
 *
 * Read a line element from SVG
 */
static GList *
read_line_svg (xmlNodePtr   node,
               DiaSvgStyle *parent_style,
               GHashTable  *style_ht,
               GHashTable  *pattern_ht,
               GList       *list,
               DiaContext  *ctx)
{
  xmlChar *str;
  DiaObjectType *otype = object_get_type ("Standard - Line");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  GPtrArray *props;
  Point start, end;
  DiaMatrix *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  start.x = _node_get_real (node, "x1", 0.0);
  start.y = _node_get_real (node, "y1", 0.0);
  end.x = _node_get_real (node, "x2", start.x);
  end.y = _node_get_real (node, "y2", start.y);

  new_obj = otype->ops->create(&start, otype->default_user_data,
				 &h1, &h2);

  props = prop_list_from_descs(svg_line_prop_descs,pdtpp_true);
  g_assert(props->len == 2);

  ptprop = g_ptr_array_index(props,0);
  ptprop->point_data = start;

  ptprop = g_ptr_array_index(props,1);
  ptprop->point_data = end;

  dia_object_set_properties (new_obj, props);
  reset_arrows (new_obj);

  prop_list_free(props);

  apply_style(new_obj, node, parent_style, style_ht, pattern_ht, TRUE);

  if (matrix) {
    _transform_object (new_obj, matrix, ctx);
    g_clear_pointer (&matrix, g_free);
  }

  return g_list_append (list, new_obj);
}


/**
 * read_rect_svg:
 *
 * Read a rectangle element from SVG
 */
static GList *
read_rect_svg (xmlNodePtr   node,
               DiaSvgStyle *parent_style,
               GHashTable  *style_ht,
               GHashTable  *pattern_ht,
               GList       *list,
               DiaContext  *ctx)
{
  xmlChar *str;
  double width, height;
  DiaObjectType *otype = object_get_type("Standard - Box");
  DiaObject *new_obj;
  Handle *h1, *h2;
  PointProperty *ptprop;
  RealProperty *rprop;
  GPtrArray *props;
  Point start,end;
  double corner_radius = 0.0;
  DiaMatrix *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  start.x = _node_get_real (node, "x", 0.0);
  start.y = _node_get_real (node, "y", 0.0);
  width = _node_get_real (node, "width", 0.0);
  height = _node_get_real (node, "height", 0.0);

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

  end.x = start.x + width;
  end.y = start.y + height;

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

  ptprop = g_ptr_array_index(props,1);
  ptprop->point_data = end;

  rprop = g_ptr_array_index(props,2);
  rprop->real_data = corner_radius;

  dia_object_set_properties (new_obj, props);
  prop_list_free (props);
  props = make_element_props (start.x,start.y,width,height);
  dia_object_set_properties (new_obj, props);

  apply_style(new_obj, node, parent_style, style_ht, pattern_ht, TRUE);
  prop_list_free(props);

  if (matrix) {
    _transform_object (new_obj, matrix, ctx);
    g_clear_pointer (&matrix, g_free);
  }
  return list;
}


/**
 * read_image_svg:
 *
 * Read SVG image element
 *
 * The result is an #Image object in the list.
 */
static GList *
read_image_svg (xmlNodePtr   node,
                DiaSvgStyle *parent_style,
                GHashTable  *style_ht,
                GHashTable  *pattern_ht,
                GList       *list,
                const char  *filename_svg,
                DiaContext  *ctx)
{
  xmlChar *str;
  real x, y, width, height;
  DiaObject *new_obj = NULL;
  DiaMatrix *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;

  str = xmlGetProp(node, (const xmlChar *)"transform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);

    xmlFree (str);
  }

  x = _node_get_real (node, "x", 0.0);
  y = _node_get_real (node, "y", 0.0);
  width = _node_get_real (node, "width", 0.0);
  height = _node_get_real (node, "height", 0.0);

  /* TODO: aspect ratio? */

  str = xmlGetNsProp (node, (const xmlChar *)"href", (const xmlChar *)"http://www.w3.org/1999/xlink");
  if (str) {
    if (strncmp ((char *)str, "data:image/", 11) == 0) {
      /* inline data - skip format description like: data:image/png;base64, */
      const char* data = strchr((char *)str, ',');

      if (data) {
        GdkPixbuf *pixbuf = pixbuf_decode_base64 (data+1);

        if (pixbuf) {
          DiaObjectChange *change;

          new_obj = create_standard_image (x, y, width, height, NULL);
          change = dia_object_set_pixbuf (new_obj, pixbuf);

          /* throw it away, no one needs it here  */
          g_clear_pointer (&change, dia_object_change_unref);
          g_clear_object (&pixbuf);
        }
      }
    } else {
      char *filename = g_filename_from_uri ((char *) str, NULL, NULL);
      if (!filename || !g_path_is_absolute (filename)) {
        /* if image file path is relative use the main path */
        char *dir = g_path_get_dirname (filename_svg);
        char *absfn = g_build_path (G_DIR_SEPARATOR_S,
                                    dir, filename ? filename : (char *) str, NULL);

        g_clear_pointer (&filename, g_free);
        g_clear_pointer (&dir, g_free);
        filename = absfn;
      }

      /* Importing svg as "Misc - Diagram" should produce better results than GdkPixbuf rendering */
      if (filename && g_strrstr (filename, ".svg")) {
        Handle *h1, *h2;
        Point point = {x, y};
        DiaObjectType *otype = object_get_type ("Misc - Diagram");

        if (otype) {
          GPtrArray *props = g_ptr_array_new ();
          new_obj = otype->ops->create (&point, otype->default_user_data, &h1, &h2);
          prop_list_add_filename (props, "diagram_file", filename);
          if (new_obj) {
            dia_object_set_properties (new_obj, props);
            point.x += width;
            point.y += height;
            dia_object_move_handle (new_obj, h2, &point, NULL, HANDLE_MOVE_USER_FINAL, 0);
          }
          prop_list_free (props);
        }
      } else {
        new_obj = create_standard_image (x, y, width, height, filename ? filename : "<broken>");
      }
      g_clear_pointer (&filename, g_free);
    }
    xmlFree(str);
  }

  if (matrix && new_obj) {
    _transform_object (new_obj, matrix, ctx);
    g_clear_pointer (&matrix, g_free);
  }

  if (new_obj)
    return g_list_append (list, new_obj);

  return list;
}

/*!
 * \brief Parse gradient including stop-colors
 *
 * Parse a radial or linear gradient into a DiaPattern.
 * Missing attribute handling for:
 *  - gradients via CSS (??)
 *
 * \ingroup SvgImport
 */
static DiaPattern *
read_gradient (xmlNodePtr node, DiaSvgStyle *parent_gs, GHashTable  *pattern_ht, DiaContext *ctx)
{
  DiaPattern *pat;
  xmlNode    *child;
  xmlChar    *str;
  guint       flags = 0;
  real        old_scale = user_scale;
  DiaMatrix  *matrix = NULL;
  graphene_matrix_t *graphene_matrix = NULL;
  DiaSvgStyle gradient_gs;

  str = xmlGetProp (node, (const xmlChar *)"gradientUnits");
  if (str) {
    if (xmlStrcmp(str, (const xmlChar *)"userSpaceOnUse")==0)
      flags |= DIA_PATTERN_USER_SPACE;
    xmlFree (str);
  }
  str = xmlGetProp (node, (const xmlChar *)"spreadMethod");
  if (str) {
    if (xmlStrcmp(str, (const xmlChar *)"pad")==0)
      flags |= DIA_PATTERN_EXTEND_PAD;
    else if (xmlStrcmp(str, (const xmlChar *)"reflect")==0)
      flags |= DIA_PATTERN_EXTEND_REFLECT;
    if (xmlStrcmp(str, (const xmlChar *)"repeat")==0)
      flags |= DIA_PATTERN_EXTEND_REPEAT;
    xmlFree (str);
  }
  /* this should not be user_scaled */
  if ((flags & DIA_PATTERN_USER_SPACE) == 0) {
    user_scale = 1.0;
  }

  str = xmlGetProp (node, (const xmlChar *)"gradientTransform");
  if (str) {
    matrix = g_new0 (DiaMatrix, 1);
    graphene_matrix = dia_svg_parse_transform ((char *) str, user_scale);

    dia_matrix_from_graphene (matrix, graphene_matrix);
  }

  if (xmlStrcmp(node->name, (const xmlChar *)"linearGradient")==0) {
    Point p1 = {_node_get_real (node, "x1", 0.0), _node_get_real (node, "y1", 0.0)};
    Point p2 = {_node_get_real (node, "x2", 1.0), _node_get_real (node, "y2", 0.0)};
    if (matrix) {
      transform_point (&p1, matrix);
      transform_point (&p2, matrix);
    }
    pat = dia_pattern_new (DIA_LINEAR_GRADIENT, flags, p1.x, p1.y);
    dia_pattern_set_point (pat, p2.x, p2.y);
  } else {
    Point c = {_node_get_real (node, "cx", 0.5), _node_get_real (node, "cy", 0.5)};
    Point f = {_node_get_real (node, "fx", c.x), _node_get_real (node, "fy", c.y)};
    real r = _node_get_real (node, "r", 0.5);
    if (matrix) {
      transform_point (&c, matrix);
      transform_point (&f, matrix);
      transform_length (&r, matrix);
    }
    pat = dia_pattern_new (DIA_RADIAL_GRADIENT, flags, c.x, c.y);
    dia_pattern_set_radius (pat, r);
    dia_pattern_set_point (pat, f.x, f.y);
  }
  /* restore previous user scale */
  user_scale = old_scale;

  /* if there are single stop value on the gradient
   *  - use it as default stop-color and stop-opacity
   *  - parse it to initialize currentColor
   */
  dia_svg_style_init (&gradient_gs, parent_gs);
  dia_svg_parse_style (node, &gradient_gs, user_scale);

  /* stops and focal point can be defined by reference */
  str = xmlGetNsProp (node, (const xmlChar *)"href", (const xmlChar *)"http://www.w3.org/1999/xlink");
  if (str) {
    DiaPattern *pattern = g_hash_table_lookup (pattern_ht, (const char*)str+1);
    if (pattern)
      dia_pattern_set_pattern (pat, pattern);
    xmlFree (str);
  }

  child = node->children;
  while (child) {
    if (xmlStrcmp(child->name, (const xmlChar *)"stop")==0) {
      DiaSvgStyle gs;
      Color color;
      real offset = 0.0;

      dia_svg_style_init (&gs, &gradient_gs);
      dia_svg_parse_style (child, &gs, user_scale);

      str = xmlGetProp(child, (const xmlChar *)"offset");
      if (str) {
	if (strrchr ((const char*)str, '%'))
	  offset = g_ascii_strtod ((const char*)str, NULL) / 100.0;
	else
	  offset = g_ascii_strtod ((const char*)str, NULL);
	xmlFree(str);
      }
      color = get_colour (gs.stop_color, gs.stop_opacity);
      dia_pattern_add_color (pat, offset, &color);
    }
    child = child->next;
  }
  g_clear_pointer (&matrix, g_free);

  return pat;
}


/*!
 * \brief Parse the CSS style block of the SVG
 *
 * Extract style information from the given node into a map for later use.
 *
 * @param node  containing the style
 * @param ht    hash table with style key and style string
 *
 * \ingroup SvgImport
 */
static void
read_style (xmlNodePtr node, GHashTable *ht)
{

  xmlChar *style_type = xmlGetProp (node, (const xmlChar *)"type");

  if (!style_type || xmlStrcmp(style_type, (const xmlChar*)"text/css") == 0) {
    xmlChar *str = xmlNodeGetContent(node);
    GRegex *regex = g_regex_new ("\\s*([^\\s+{]+)\\s*{([^}]*)}", G_REGEX_MULTILINE, 0, NULL);
    GMatchInfo *info;

    g_regex_match (regex, (gchar *)str, 0, &info);
    while (g_match_info_matches (info)) {
      /* Use the _last_ key before { and val between {}
       * To the left there might be referenced parents in the tree restricting
       * the effect of the key but that's just too complicated for Dia's SVG ...
       */
      char *key = g_match_info_fetch (info, 1);
      char *val = g_match_info_fetch (info, 2);
      const char *former;

      /* multiple occurrences chain up to a single style string */
      former = g_hash_table_lookup (ht, key);
      if (former) {
        char *tmp = val;
        val = g_strdup_printf ("%s;%s", former, val);
        g_clear_pointer (&tmp, g_free);
      }
      /* hashtable taking ownership */
      g_hash_table_insert (ht, key, val);
      g_match_info_next (info, NULL);
    }
    g_match_info_free (info);
    g_regex_unref (regex);
    xmlFree (str);
  }

  if (style_type)
    xmlFree (style_type);
}


/* GFunc for foreach */
static void
add_def (gpointer       data,
         gpointer       user_data)
{
  DiaObject  *obj = (DiaObject *)data;
  GHashTable *defs_ht = (GHashTable *)user_data;
  char *id = dia_object_get_meta (obj, "id");
  if (id) { /* pass ownership of name and object */
    g_hash_table_insert (defs_ht, id, obj);
  } else {
    obj->ops->destroy (obj);
    g_clear_pointer (&obj, g_free);
  }
}


/*!
 * \brief Parse and set global user scale
 *
 * This function is modifying the global user scale. If it's effect
 * should be temporary user_scale needs to be saved before.
 *
 * @param root  element containing the viewBox attribute
 * @param mat   out parameter for optional matrix transform
 *
 * \ingroup SvgImport
 */
static void
_node_read_viewbox (xmlNodePtr root, DiaMatrix **mat)
{
  xmlChar *swidth = xmlGetProp(root, (const xmlChar *)"width");
  xmlChar *sheight = xmlGetProp(root, (const xmlChar *)"height");
  xmlChar *sviewbox = xmlGetProp(root, (const xmlChar *)"viewBox");

  if (swidth && sheight && sviewbox) {
    real width, height;
    gchar *remains = NULL;
    gboolean percent;
    gchar **vals;

    width = get_value_as_cm ((const char *)swidth, &remains);
    percent = (remains && *remains == '%');
    height = get_value_as_cm ((const char *)sheight, &remains);
    percent |= (remains && *remains == '%');
    vals = g_regex_split_simple ("[\\s,;]+", (const char *)sviewbox, 0, 0);

    if (vals && vals[0] && vals[1] && vals[2] && vals[3]) {
      real x1, y1, x2, y2;
      real xs, ys;
      x1 = g_ascii_strtod (vals[0], NULL);
      y1 = g_ascii_strtod (vals[1], NULL);
      x2 = g_ascii_strtod (vals[2], NULL);
      y2 = g_ascii_strtod (vals[3], NULL);
      g_debug ("%s: viewBox(%f %f %f %f) = (%f,%f)",
               G_STRLOC,
               x1,
               y1,
               x2,
               y2,
               width,
               height);
      /* some basic sanity check */
      if (x2 > x1 && y2 > y1 && width > 0 && height > 0) {
        if (!percent) {
          /* viwBox is x-min y-min width height - so only use the latter for scaling */
          xs = (real) x2 / width;
          ys = (real) y2 / height;
        } else {
          xs = ys = DEFAULT_SVG_SCALE * (100.0 / sqrt(width*height));
        }
        /* plausibility check, strictly speaking these are not required to be the same
        * /or/ are they and Dia is writing a bogus viewBox?
        */
        if (fabs ((fabs (xs/ys) - 1.0) < 0.1) && fabs ((fabs (ys/xs) - 1.0) < 0.1)) {
          user_scale = xs;
          g_debug ("%s: viewBox(%f %f %f %f) scaling (%f,%f) -> %f",
                   G_STRLOC,
                   x1,
                   y1,
                   x2,
                   y2,
                   xs,
                   ys,
                   user_scale);
        } else {
          /* the bigger the scale the smaller the objects */
          user_scale = MAX (xs, ys);
      #if 0 /* need to think about - might fly with preserveAspectRatio handling */
          if (mat) {
            DiaMatrix *m = g_new0 (DiaMatrix, 1);
            m->xx = xs > ys ? xs/ys : 1.0;
            m->yy = ys > xs ? ys/xs : 1.0;
            *mat = m;
          }
      #endif
        }
      }
    }
    g_strfreev (vals);
  }

  if (swidth)
    xmlFree (swidth);
  if (sheight)
    xmlFree (sheight);
  if (sviewbox)
    xmlFree (sviewbox);
}

/*!
 * Fill a GList* with objects which is to be put in a
 * diagram or a group by the caller.
 * Can be called recursively to allow groups in groups.
 *
 * @param startnode the XML node to dive into
 * @param parent_gs the graphic style inherited by parent
 * @param defs_ht a map of objects filled from 'defs' to use as templates for 'use'
 * @param style_ht map of styles
 * @param pattern_ht map of patterns
 * @param filename_svg SVG filename for better error messages
 * @param ctx context to keep error messages grouped
 * @return a list of _DiaObject
 *
 * \ingroup SvgImport
 */
static GList*
read_items (xmlNodePtr   startnode,
            DiaSvgStyle *parent_gs,
            GHashTable  *defs_ht,
            GHashTable  *style_ht,
            GHashTable  *pattern_ht,
            const char  *filename_svg,
            DiaContext  *ctx)
{
  xmlNodePtr node;
  GList *items = NULL;
  char *comment = NULL;


  for (node = startnode; node != NULL; node = node->next) {
    /* Points to the *first* object created by this node (used to be the last one).
     * Dia may split a single SVG element into multiple DiaObjects. If so these objects
     * should be grouped or at least inherit all the same node properties
     */
    DiaObject *obj = NULL;

    if (xmlIsBlankNode(node)) continue;
    if (node->type == XML_COMMENT_NODE) {
      if (!comment) {
        comment = g_strdup ((char *) node->content);
      } else {
        char *prev = comment;
        comment = g_strjoin ("\n", comment, (char *) node->content, NULL);
        g_clear_pointer (&prev, g_free);
      }
      continue;
    }
    if (node->type != XML_ELEMENT_NODE) continue;

    if (!xmlStrcmp(node->name, (const xmlChar *)"g")) {
      GList *moreitems;
      DiaSvgStyle *group_gs;
      DiaMatrix *matrix = NULL;
      graphene_matrix_t *graphene_matrix = NULL;
      xmlChar *trans;

      /* We need to have/apply the groups style before the objects style */
      group_gs = g_new0 (DiaSvgStyle, 1);
      dia_svg_style_init (group_gs, parent_gs);
      _node_css_parse_style (node, group_gs, user_scale, style_ht);
      dia_svg_parse_style (node, group_gs, user_scale);

      trans = xmlGetProp (node, (xmlChar *)"transform");
      if (trans) {
        matrix = g_new0 (DiaMatrix, 1);
        graphene_matrix = dia_svg_parse_transform ((char *) trans, user_scale);

        dia_matrix_from_graphene (matrix, graphene_matrix);

        xmlFree (trans);
      }

      moreitems = read_items (node->xmlChildrenNode, group_gs,
			      defs_ht, style_ht, pattern_ht,
			      filename_svg, ctx);

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
      g_clear_object (&group_gs->font);
      g_clear_pointer (&group_gs, g_free);
      g_clear_pointer (&matrix, g_free);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"symbol")) {
      /* ignore 'viewBox' and 'preserveAspectRatio' */
      GList *moreitems = read_items (node->xmlChildrenNode, parent_gs,
				     defs_ht, style_ht, pattern_ht,
				     filename_svg, ctx);

      /* only one object or create a group */
      if (g_list_length (moreitems) > 1)
	obj = group_create (moreitems);
      else if (moreitems)
	obj = g_list_last(moreitems)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"rect")) {
      items = read_rect_svg(node, parent_gs, style_ht, pattern_ht, items, ctx);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"line")) {
      items = read_line_svg(node, parent_gs, style_ht, pattern_ht, items, ctx);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"ellipse") || !xmlStrcmp(node->name, (const xmlChar *)"circle")) {
      items = read_ellipse_svg(node, parent_gs, style_ht, pattern_ht, items, ctx);
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polyline")) {
      items = read_poly_svg(node, parent_gs, style_ht, pattern_ht, items, "Standard - PolyLine");
      if (items)
	obj = g_list_last(items)->data;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polygon")) {
      items = read_poly_svg(node, parent_gs, style_ht, pattern_ht, items, "Standard - Polygon");
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"text")) {
      items = read_text_svg(node, parent_gs, style_ht, pattern_ht, items, ctx);
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"path")) {
      /* the path element might be split into multiple objects */
      int first = g_list_length (items);
      items = read_path_svg(node, parent_gs, style_ht, pattern_ht, items, ctx);
      if (items && g_list_nth(items, first))
	obj = g_list_nth(items, first)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"image")) {
      items = read_image_svg(node, parent_gs, style_ht, pattern_ht, items, filename_svg, ctx);
      if (items)
	obj = g_list_last(items)->data;
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"linearGradient") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"radialGradient") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"style") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"pattern") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"mask") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"defs")) {
      /* read_defs was already handling these, mostly;) */
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"use")) {
      xmlChar *key = xmlGetNsProp (node, (const xmlChar *)"href", (const xmlChar *)"http://www.w3.org/1999/xlink");

      if (key && key[0] == '#') {
	DiaObject *otemp = g_hash_table_lookup (defs_ht, key+1);

	if (otemp) {
	  /* The new object can be referenced again by use or
	   * be target for meta info, comment or something. */
	  obj = otemp->ops->copy (otemp);

	  use_position (obj, node, ctx);
	  /* this should only be styled from the containing group,
	   * if it has no style on it's own. Sorry Dia can't create
	   * objects w/o style so we have two options beside complete
	   * rewrite:
	   *  - use the style from the group and hope it is on defaults
	        (i.e. pass in parent_gs with init=FALSE)
	   *  - use a style from scratch instead of parent_gs and hope
	   *    the object is already styled correctly
	   * Or maybe we should remember the explicit style set during
	   * creation, store it with the template as meta info and use
	   * that to give NULL or parent_gs here?
	   */
	  apply_style (obj, node, parent_gs, style_ht, pattern_ht, FALSE);
	  items = g_list_append (items, obj);
	}
      }
      if (key)
	xmlFree (key);
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"svg")) {
      /* A subsequent svg node is turned into a group to simplify offsetting
       * it and maybe later honor additional attributes like viewBox, width,
       * height and preserveAspectRatio with clipping and scaling
       */
      GList *moreitems;
      Point pos;
      /* put the global user scale on the stack */
      real user_scale_prev = user_scale;
      DiaMatrix *matrix = NULL;
      DiaSvgStyle subsvg_gs;

      _node_read_viewbox (node, &matrix);

      /* there may be specific style attached to this node, too */
      dia_svg_style_init (&subsvg_gs, parent_gs);
      dia_svg_parse_style (node, &subsvg_gs, user_scale);

      pos.x = _node_get_real (node, "x", 0.0);
      pos.y = _node_get_real (node, "y", 0.0);
      moreitems = read_items (node->xmlChildrenNode, &subsvg_gs,
			      defs_ht, style_ht, pattern_ht,
			      filename_svg, ctx);
      if (moreitems) {
	DiaObject *group;

	if (matrix) /* eats list and matrix */
	  group = group_create_with_matrix (moreitems, matrix);
	else
	  group = group_create (moreitems);
	/* remember for meta */
	obj = group;
	if (pos.x != 0.0 || pos.y != 0.0) {
	  pos.x += obj->position.x;
	  pos.y += obj->position.y;
	  obj->ops->move (obj, &pos);
	}
        items = g_list_append (items, group);
      }
      /* restore scale */
      user_scale = user_scale_prev;
    } else {
      /* everything else is treated like a group _without grouping_, i.e. we dive
       * into unknown stuff. This allows to import stuff generated by 'dot' with
       * e.g. links for the nodes.
       */
      GList *moreitems;
      /* one of the non-grouping elements is <a>, extract possible links */
      xmlChar *href = xmlGetNsProp (node, (const xmlChar *)"href", (const xmlChar *)"http://www.w3.org/1999/xlink");

      moreitems = read_items (node->xmlChildrenNode, parent_gs,
			      defs_ht, style_ht, pattern_ht,
			      filename_svg, ctx);
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
      if (comment) {
        dia_object_set_meta (obj, "comment", (char *)comment);
        g_clear_pointer (&comment, g_free);
      }
    }
  }
  /* just to be sure */
  g_clear_pointer (&comment, g_free);
  return items;
}

/*!
 * \brief Parse definitions, i.e. everything which does not have direct drawing
 *
 * This function shall be called before read_items to allow referencing of the
 * definitions in the latter.
 *
 * \ingroup SvgImport
 */
static void
read_defs (xmlNodePtr   startnode,
	    DiaSvgStyle *parent_gs,
	    GHashTable  *defs_ht,
	    GHashTable  *style_ht,
	    GHashTable  *pattern_ht,
	    const gchar *filename_svg,
	    DiaContext  *ctx)
{
  xmlNodePtr node;

  for (node = startnode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node) || node->type == XML_COMMENT_NODE)
      continue;

    if(!xmlStrcmp(node->name, (const xmlChar *)"linearGradient") ||
       !xmlStrcmp(node->name, (const xmlChar *)"radialGradient")) {
      xmlChar *val = xmlGetProp (node, (const xmlChar *)"id");
      if (val) {
	DiaPattern *pat = read_gradient (node, parent_gs, pattern_ht, ctx);
	if (pat)
	  g_hash_table_insert (pattern_ht, g_strdup((gchar *)val), pat);
	xmlFree (val);
      }
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"defs")) {
      /* everything below must have a name to make a difference */
      GList *list, *defs = read_items (node->xmlChildrenNode, parent_gs,
				       defs_ht, style_ht, pattern_ht,
				       filename_svg, ctx);

      /* Commonly seen in <defs/> are
       *   clipPath, font, filter, linearGradient, mask, marker, pattern, radialGradient, style
       * mostly not supported as of this writing.
       * Less commonly used are normal objects which could be supported here.
       */
      for (list = defs; list != NULL; list = g_list_next (list)) {
        DiaObject *otemp = list->data;
        char *id;

        id = dia_object_get_meta (otemp, "id");
        if (id) {
          /* pass ownership of name and object */
          g_hash_table_insert (defs_ht, id, otemp);
        } else if (IS_GROUP (otemp)) {
          /* defs in _unnamed_ groups, I don't get the
          * benefit but must have seen it in the wild.
          */
          GList *moredefs = group_objects (otemp);

          g_list_foreach (moredefs, add_def, defs_ht);
          group_destroy_shallow (otemp);
        } else {
          /* just loose the object */
          otemp->ops->destroy (otemp);
          g_clear_pointer (&otemp, g_free);
          list->data = NULL;
        }
      }
      /* kind of greedy */
      read_defs (node->xmlChildrenNode, parent_gs,
		 defs_ht, style_ht, pattern_ht,
		 filename_svg, ctx);
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"style")) {
      /* Prepare the third variant to apply style to the objects.
       * The final style is similar to what we have in the style
       * attribute, but we have to do complicated key lookup to
       * apply styles to the right objects.
       */
      read_style (node, style_ht);
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"pattern")) {
      /* Patterns could be considered as groups, too. But Dia does not
       * have the facility to apply them (yet?). */
    } else if(!xmlStrcmp(node->name, (const xmlChar *)"g") ||
	      !xmlStrcmp(node->name, (const xmlChar *)"a")) {
      /* just dive into */
      DiaSvgStyle group_gs;
      dia_svg_style_init (&group_gs, parent_gs);
      _node_css_parse_style (node, &group_gs, user_scale, style_ht);
      dia_svg_parse_style (node, &group_gs, user_scale);

      read_defs (node->xmlChildrenNode, &group_gs,
                 defs_ht, style_ht, pattern_ht,
                 filename_svg, ctx);

      if (group_gs.font) {
        g_clear_object (&group_gs.font);
      }
    }
  }
}

/*!
 * \defgroup ShapeImport Shape Import
 * \ingroup SvgPlugin
 * \brief Import SVG from Dia shape file
 */

/*!
 * \brief Parse a shape connection point node
 * \ingroup ShapeImport
 */
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
    *mcp = (sm ? strcmp ((const char *)sm, "yes") == 0 : FALSE);
    ret = TRUE;
  }
  if (sx) xmlFree (sx);
  if (sy) xmlFree (sy);
  if (sm) xmlFree (sm);
  return ret;
}

/*!
 * \brief Import a Dia .shape file
 * The Dia Shape file format consists of simple SVG plus some additional information
 * in the shape name space.
 * \ingroup ShapeImport
 */
static gboolean
import_shape_info (xmlNodePtr start_node, DiagramData *dia, DiaContext *ctx)
{
  const DiaObjectType *ot_cp = object_get_type ("Shape Design - Connection Point");
  const DiaObjectType *ot_mp = object_get_type ("Shape Design - Main Connection Point");
  xmlNodePtr node;
  DiaLayer *layer;

  if (!ot_cp || !ot_mp) {
    dia_context_add_message (ctx, _("'Shape Design' shapes missing."));
    return FALSE;
  }
  layer = dia_layer_new ("Shape Design", dia);
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
            dia_layer_add_object (layer, o);
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

  g_clear_object (&layer);

  return TRUE;
}

/*!
 * \brief Import an SVG file from the given memory block
 * \ingroup SvgImport
 */
static gboolean
import_memory_svg (const guchar *p, guint size, DiagramData *dia,
		   DiaContext *ctx, void *user_data)
{
  xmlDocPtr doc = xmlParseMemory ((const char *)p, size);

  if (!doc) {
    const xmlError *err = xmlGetLastError ();

    dia_context_add_message(ctx, _("Parse error for memory block.\n%s"), err->message);
    return FALSE;
  }
  return import_svg (doc, dia, ctx, user_data);
}


/*!
 * \brief Imports the SVG file given by file name
 * @return TRUE if successful
 * \ingroup SvgImport
 */
static gboolean
import_file_svg (const char  *filename,
                 DiagramData *dia,
                 DiaContext  *ctx,
                 void        *user_data)
{
  xmlDocPtr doc = dia_io_load_document (filename, ctx, NULL);

  if (!doc) {
    return FALSE;
  }

  return import_svg (doc, dia, ctx, user_data);
}


static gboolean
import_svg (xmlDocPtr    doc,
            DiagramData *dia,
            DiaContext  *ctx,
            void        *user_data)
{
  xmlNsPtr svg_ns;
  xmlNodePtr root;
  xmlNodePtr shape_root = NULL;
  GList *items, *item;
  guint num_items = 0;
  gboolean groups_to_layers = TRUE;

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
    dia_context_add_message(ctx, _("Expected SVG name-space not found in file"));
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

  /* the following calls rely on the fact that no one messed with the original scale */
  if (shape_root)
    user_scale = 1.0;
  else
    user_scale = DEFAULT_SVG_SCALE;

  /* if the svg root element contains width, height and viewBox calculate our user scale from it */
  if (!shape_root)
    _node_read_viewbox (root, NULL);

  {
    GHashTable *defs_ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
    GHashTable *style_ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    GHashTable *pattern_ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
    DiaSvgStyle root_gs;
    /* first read all definitions ... */
    read_defs (root->xmlChildrenNode, NULL, defs_ht, style_ht, pattern_ht,
	       dia_context_get_filename(ctx), ctx);
    /* ... to have the available for the rendered objects */
    /* also parse the style from the root element to have potential defaults */
    dia_svg_style_init (&root_gs, NULL);
    dia_svg_parse_style (root, &root_gs, user_scale);
    items = read_items (root->xmlChildrenNode, &root_gs, defs_ht, style_ht, pattern_ht,
		        dia_context_get_filename(ctx), ctx);
    g_hash_table_destroy (pattern_ht);
    g_hash_table_destroy (style_ht);
    g_hash_table_destroy (defs_ht);
  }
  /* Every top level item which is a group with a name/id we are converting
   * back to a layer. This is consistent with our SVG export and does
   * also work with layers from Sodipodi/Inkscape/iDraw/...
   * But if there is just one item - even if a group - it is put into the active layer.
   */
  num_items = g_list_length (items);
  /* We have to convert _all_ the groups to layers or not at all to
   * preserve the drawing order. Also we should keep the groups intact
   * to preserve potential transformations.
   */
  if (num_items == 1) {
    groups_to_layers = FALSE;
  }

  for (item = items; groups_to_layers && item != NULL; item = g_list_next (item)) {
    DiaObject *obj = (DiaObject *)item->data;
    char *name;

    if (IS_GROUP(obj) && ((name = dia_object_get_meta (obj, "id")) != NULL))
      g_clear_pointer (&name, g_free);
    else
      groups_to_layers = FALSE;
  }
  for (item = items; item != NULL; item = g_list_next (item)) {
    DiaObject *obj = (DiaObject *)item->data;

    if (groups_to_layers) {
      char *name = dia_object_get_meta (obj, "id");
      DiaLayer *layer = dia_layer_new (name, dia);

      g_clear_pointer (&name, g_free);

      /* keep the group for potential transformation */
      dia_layer_add_object (layer, obj);
      data_add_layer (dia, layer);

      g_clear_object (&layer);
    } else {
      DiaLayer *active = dia_diagram_data_get_active_layer (dia);
      /* Just as before: throw it in the active layer */
      dia_layer_add_object (active, obj);
      dia_layer_update_extents (active);
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

static const char *extensions[] = { "svg", "svgz", NULL };
DiaImportFilter svg_import_filter = {
  N_("Scalable Vector Graphics"),
  extensions,
  import_file_svg,
  NULL, /* user_data */
  "dia-svg",
  0, /* flags */
  import_memory_svg
};
