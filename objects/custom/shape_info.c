/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
 *
 * Non-uniform scaling/subshape support by Marcel Toele.
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

#define G_LOG_DOMAIN "DiaCustom"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdlib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <float.h>
#include <string.h>
#include "shape_info.h"
#include "custom_util.h"
#include "custom_object.h"
#include "dia_image.h"
#include "prop_pixbuf.h" /* pixbuf_decode_base64() */
#include "message.h"
#include "prefs.h"
#include "boundingbox.h"
#include "dia-io.h"

#include "units.h"

#define FONT_HEIGHT_DEFAULT 1
#define TEXT_ALIGNMENT_DEFAULT DIA_ALIGN_CENTRE

static ShapeInfo *load_shape_info(const gchar *filename, ShapeInfo *preload);
static void update_bounds(ShapeInfo *info);

static GHashTable *name_to_info = NULL;

ShapeInfo *
shape_info_load(const gchar *filename)
{
  ShapeInfo *info = load_shape_info(filename, NULL);

  if (!info)
    return NULL;
  shape_info_register (info);
  return info;
}

void
shape_info_register (ShapeInfo *info)
{
  if (!name_to_info)
    name_to_info = g_hash_table_new(g_str_hash, g_str_equal);
  g_hash_table_insert(name_to_info, info->name, info);
}

ShapeInfo *
shape_info_get(ObjectNode obj_node)
{
  ShapeInfo *info = NULL;
  xmlChar *str;

  str = xmlGetProp(obj_node, (const xmlChar *)"type");
  if (str && name_to_info) {
    info = g_hash_table_lookup(name_to_info, (gchar *) str);
    if (!info->loaded)
      load_shape_info (info->filename, info);

    dia_clear_xml_string (&str);
  }
  return info;
}

ShapeInfo *
shape_info_getbyname(const gchar *name)
{
  if (name && name_to_info) {
    ShapeInfo *info = g_hash_table_lookup(name_to_info,name);
    if (!info->loaded)
      load_shape_info (info->filename, info);
    return info;
  }
  return NULL;
}

/*!
 * \brief After loading and before drawing ShapeInfo needs to be realised
 * @param : the shape to realise
 *
 * Puts the ShapeInfo into a form suitable for actual use (lazy loading)
 *
 * \extends _ShapeInfo
 */
void
shape_info_realise(ShapeInfo* info)
{
  GList* tmp;

  for (tmp = info->display_list; tmp != NULL; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    if (el->type == GE_TEXT) {
          /* set default values for text style */
      if (!el->text.s.font_height)
        el->text.s.font_height = FONT_HEIGHT_DEFAULT;
      if (!el->text.s.font)
        el->text.s.font = dia_font_new_from_style(DIA_FONT_SANS,1.0);
      if (el->text.s.alignment == -1)
        el->text.s.alignment = TEXT_ALIGNMENT_DEFAULT;
      if (!el->text.object) {
        el->text.object = new_text(el->text.string,
                                   el->text.s.font,
                                   el->text.s.font_height,
                                   &el->text.anchor,
                                   &color_black,
                                   el->text.s.alignment);
      }
      text_calc_boundingbox(el->text.object, &el->text.text_bounds);
    }
  }
}

real
shape_info_get_default_width(ShapeInfo *info)
{
  if (info->default_width == 0.0)
    info->default_width = DEFAULT_WIDTH;

  return( info->default_width );
}

real
shape_info_get_default_height(ShapeInfo *info)
{
  if (info->default_height == 0.0)
    info->default_height = DEFAULT_HEIGHT;

  return( info->default_height );
}

static void
parse_path(ShapeInfo *info, const char *path_str, DiaSvgStyle *s, const char* filename)
{
  GArray *points;
  gchar *pathdata = (gchar *)path_str, *unparsed;
  gboolean closed = FALSE;
  Point current_point = {0.0, 0.0};

  points = g_array_new(FALSE, FALSE, sizeof(BezPoint));
  g_array_set_size(points, 0);
  do {
    if (!dia_svg_parse_path (points, pathdata, &unparsed, &closed, &current_point))
      break;

    if (points->len > 0) {
      if (g_array_index(points, BezPoint, 0).type != BEZ_MOVE_TO) {
        message_warning (_("The file '%s' has invalid path data.\n"
	                   "svg:path data must start with moveto."),
			   dia_message_filename(filename));
      } else if (closed) {
        /* if there is some unclosed commands, add them as a GE_SHAPE */
        GraphicElementPath *el = dia_new_with_extra (sizeof (GraphicElementPath),
                                                     points->len,
                                                     sizeof (BezPoint));
        el->type = GE_SHAPE;
        dia_svg_style_init (&el->s, s);
        el->npoints = points->len;
        memcpy ((char *) el->points, points->data, points->len * sizeof(BezPoint));
        info->display_list = g_list_append (info->display_list, el);
      } else {
        /* if there is some unclosed commands, add them as a GE_PATH */
        GraphicElementPath *el = dia_new_with_extra (sizeof (GraphicElementPath),
                                                     points->len,
                                                     sizeof (BezPoint));
        el->type = GE_PATH;
        dia_svg_style_init (&el->s, s);
        el->npoints = points->len;
        memcpy ((char *) el->points, points->data, points->len * sizeof (BezPoint));
        info->display_list = g_list_append(info->display_list, el);
      }
      g_array_set_size (points, 0);
    }
    pathdata = unparsed;
    unparsed = NULL;
  } while (pathdata);

  g_array_free (points, TRUE);
}

static gboolean
is_subshape(xmlNode* node)
{
  gboolean res = FALSE;

  if (xmlHasProp(node, (const xmlChar*)"subshape")) {
    xmlChar* value = xmlGetProp(node, (const xmlChar*)"subshape");

    if (!strcmp((const char*)value, "true"))
      res = TRUE;

    dia_clear_xml_string (&value);
  }

  return res;
}

/*!
 * \brief Parse the SVG node from a shape file
 *
 * Fill the ShapeInfo display list with GraphicElement each got from
 * a single node within the shape's SVG part.
 *
 * \extends _ShapeInfo
 */
static void
parse_svg_node(ShapeInfo *info, xmlNodePtr node, xmlNsPtr svg_ns,
               DiaSvgStyle *style, const gchar *filename)
{
  xmlChar *str;

      /* walk SVG node ... */
  for (node = node->xmlChildrenNode; node != NULL; node = node->next) {
    GraphicElement *el = NULL;
    DiaSvgStyle s;

    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE || node->ns != svg_ns)
      continue;
    dia_svg_style_init (&s, style);
    dia_svg_parse_style(node, &s, -1);
    if (!xmlStrcmp(node->name, (const xmlChar *)"line")) {
      GraphicElementLine *line = g_new0(GraphicElementLine, 1);

      el = (GraphicElement *)line;
      line->type = GE_LINE;
      str = xmlGetProp(node, (const xmlChar *)"x1");
      if (str) {
        line->p1.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"y1");
      if (str) {
        line->p1.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"x2");
      if (str) {
        line->p2.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"y2");
      if (str) {
        line->p2.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polyline")) {
      GraphicElementPoly *poly;
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(real));
      real val, *rarr;
      gchar *tmp;
      int i;

      str = xmlGetProp(node, (const xmlChar *)"points");
      tmp = (gchar *) str;
      while (tmp[0] != '\0') {
            /* skip junk */
        while (tmp[0] != '\0' && !g_ascii_isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
          tmp++;
        if (tmp[0] == '\0') break;
        val = g_ascii_strtod(tmp, &tmp);
        g_array_append_val(arr, val);
      }
      xmlFree(str);
      val = 0;
      if (arr->len % 2 == 1)
        g_array_append_val(arr, val);
      poly = dia_new_with_extra (sizeof (GraphicElementPoly),
                                 arr->len / 2,
                                 sizeof (Point));
      el = (GraphicElement *)poly;
      poly->type = GE_POLYLINE;
      poly->npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < poly->npoints; i++) {
        poly->points[i].x = rarr[2*i];
        poly->points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"polygon")) {
      GraphicElementPoly *poly;
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(real));
      real val, *rarr;
      char *tmp;
      int i;

      str = xmlGetProp(node, (const xmlChar *)"points");
      tmp = (char *) str;
      while (tmp[0] != '\0') {
            /* skip junk */
        while (tmp[0] != '\0' && !g_ascii_isdigit(tmp[0]) && tmp[0]!='.'&&tmp[0]!='-')
          tmp++;
        if (tmp[0] == '\0') break;
        val = g_ascii_strtod(tmp, &tmp);
        g_array_append_val(arr, val);
      }
      xmlFree(str);
      val = 0;
      if (arr->len % 2 == 1)
        g_array_append_val(arr, val);
      poly = dia_new_with_extra (sizeof (GraphicElementPoly),
                                 arr->len / 2,
                                 sizeof (Point));
      el = (GraphicElement *)poly;
      poly->type = GE_POLYGON;
      poly->npoints = arr->len / 2;
      rarr = (real *)arr->data;
      for (i = 0; i < poly->npoints; i++) {
        poly->points[i].x = rarr[2*i];
        poly->points[i].y = rarr[2*i+1];
      }
      g_array_free(arr, TRUE);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"rect")) {
      GraphicElementRect *rect = g_new0(GraphicElementRect, 1);
      real corner_radius = 0.0;

      el = (GraphicElement *)rect;
      rect->type = GE_RECT;
      str = xmlGetProp(node, (const xmlChar *)"x");
      if (str) {
        rect->corner1.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      } else rect->corner1.x = 0;
      str = xmlGetProp(node, (const xmlChar *)"y");
      if (str) {
        rect->corner1.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      } else rect->corner1.y = 0;
      str = xmlGetProp(node, (const xmlChar *)"width");
      if (str) {
        rect->corner2.x = rect->corner1.x + g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"height");
      if (str) {
        rect->corner2.y = rect->corner1.y + g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"rx");
      if (str) {
        corner_radius = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"ry");
      if (str) {
        if(corner_radius != 0.0) {
          /* calculate the mean value of rx and ry */
          corner_radius = (corner_radius+g_ascii_strtod((gchar *) str, NULL))/2;
        } else {
          corner_radius = g_ascii_strtod((gchar *) str, NULL);
        }
        xmlFree(str);
      }
      rect->corner_radius = corner_radius;
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"text")) {

      GraphicElementText *text = g_new(GraphicElementText, 1);
      el = (GraphicElement *)text;
      text->type = GE_TEXT;
      text->object = NULL;
      str = xmlGetProp(node, (const xmlChar *)"x");
      if (str) {
        text->anchor.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      } else text->anchor.x = 0;
      str = xmlGetProp(node, (const xmlChar *)"y");
      if (str) {
        text->anchor.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      } else text->anchor.y = 0;

      str = xmlGetProp(node, (const xmlChar *)"font-size");
      if (str) {
        text->s.font_height = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      } else text->s.font_height = 0.8;

      str = xmlNodeGetContent(node);
      if (str) {
        text->string = g_strdup((gchar *) str);
        xmlFree(str);
      } else text->string = g_strdup("");
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"circle")) {
      GraphicElementEllipse *ellipse = g_new0(GraphicElementEllipse, 1);

      el = (GraphicElement *)ellipse;
      ellipse->type = GE_ELLIPSE;
      str = xmlGetProp(node, (const xmlChar *)"cx");
      if (str) {
        ellipse->center.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"cy");
      if (str) {
        ellipse->center.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"r");
      if (str) {
        ellipse->width = ellipse->height = 2 * g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"ellipse")) {
      GraphicElementEllipse *ellipse = g_new0(GraphicElementEllipse, 1);

      el = (GraphicElement *)ellipse;
      ellipse->type = GE_ELLIPSE;
      str = xmlGetProp(node, (const xmlChar *)"cx");
      if (str) {
        ellipse->center.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"cy");
      if (str) {
        ellipse->center.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"rx");
      if (str) {
        ellipse->width = 2 * g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"ry");
      if (str) {
        ellipse->height = 2 * g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"path")) {
      str = xmlGetProp(node, (const xmlChar *)"d");
      if (str) {
        parse_path(info, (char *) str, &s, filename);
        xmlFree(str);
      }
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"image")) {
      GraphicElementImage *image = g_new0(GraphicElementImage, 1);

      el = (GraphicElement *)image;
      image->type = GE_IMAGE;
      str = xmlGetProp(node, (const xmlChar *)"x");
      if (str) {
        image->topleft.x = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"y");
      if (str) {
        image->topleft.y = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"width");
      if (str) {
        image->width = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"height");
      if (str) {
        image->height = g_ascii_strtod((gchar *) str, NULL);
        xmlFree(str);
      }
      str = xmlGetProp(node, (const xmlChar *)"xlink:href");
      if (!str) /* this doesn't look right but it appears to work w/o namespace --hb */
        str = xmlGetProp(node, (const xmlChar *)"href");
      if (str) {
        char *imgfn = NULL;
        const char* data = strchr ((char *) str, ',');

        /* first check for inlined data */
        if (data) {
          GdkPixbuf *pixbuf = pixbuf_decode_base64 (data + 1);

          if (pixbuf) {
            image->image = dia_image_new_from_pixbuf (pixbuf);
            g_clear_object (&pixbuf);
          }
        } else {
          imgfn = g_filename_from_uri ((char *) str, NULL, NULL);
          if (!imgfn) {
            /* despite it's name it ensures an absolute filename */
            imgfn = custom_get_relative_filename (filename, (char *) str);
          }

          image->image = dia_image_load (imgfn);
        }

        if (!image->image) {
          g_debug ("%s: failed to load image file %s",
                   G_STRLOC,
                   imgfn ? imgfn : "(data:)");
        }

        g_clear_pointer (&imgfn, g_free);
        xmlFree (str);
      }
      /* w/o the image we would crash later */
      if (!image->image)
        image->image = dia_image_get_broken();
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"g")) {
      if (!is_subshape(node)) {
          /* add elements from the group element */
        parse_svg_node(info, node, svg_ns, &s, filename);
      } else {
          /* add elements from the group element, but make it a subshape */
        GraphicElementSubShape *subshape = g_new0(GraphicElementSubShape, 1);
        ShapeInfo* tmpinfo = g_new0(ShapeInfo, 1);
        xmlChar *v_anchor_attr = xmlGetProp(node, (const xmlChar*)"v_anchor");
        xmlChar *h_anchor_attr = xmlGetProp(node, (const xmlChar*)"h_anchor");

        parse_svg_node(tmpinfo, node, svg_ns, &s, filename);

        tmpinfo->shape_bounds.top = DBL_MAX;
        tmpinfo->shape_bounds.left = DBL_MAX;
        tmpinfo->shape_bounds.bottom = -DBL_MAX;
        tmpinfo->shape_bounds.right = -DBL_MAX;

        update_bounds( tmpinfo );
        update_bounds( info );

        subshape->half_width = (tmpinfo->shape_bounds.right-tmpinfo->shape_bounds.left) / 2.0;
        subshape->half_height = (tmpinfo->shape_bounds.bottom-tmpinfo->shape_bounds.top) / 2.0;
        subshape->center.x = tmpinfo->shape_bounds.left + subshape->half_width;
        subshape->center.y = tmpinfo->shape_bounds.top + subshape->half_height;

        subshape->type = GE_SUBSHAPE;
        subshape->display_list = tmpinfo->display_list;
        subshape->v_anchor_method = OFFSET_METHOD_FIXED;
        subshape->h_anchor_method = OFFSET_METHOD_FIXED;
        subshape->default_scale = 0.0;

        if (!v_anchor_attr || !strcmp((const char*)v_anchor_attr,"fixed.top"))
          subshape->v_anchor_method = OFFSET_METHOD_FIXED;
        else if (v_anchor_attr && !strcmp((const char*)v_anchor_attr,"fixed.bottom"))
          subshape->v_anchor_method = -OFFSET_METHOD_FIXED;
        else if (v_anchor_attr && !strcmp((const char*)v_anchor_attr,"proportional"))
          subshape->v_anchor_method = OFFSET_METHOD_PROPORTIONAL;
        else {
          fprintf (stderr,
                   "illegal v_anchor “%s”, defaulting to fixed.top\n",
                   v_anchor_attr);
        }

        if (!h_anchor_attr || !strcmp((const char*)h_anchor_attr,"fixed.left"))
          subshape->h_anchor_method = OFFSET_METHOD_FIXED;
        else if (h_anchor_attr && !strcmp((const char*)h_anchor_attr,"fixed.right"))
          subshape->h_anchor_method = -OFFSET_METHOD_FIXED;
        else if (h_anchor_attr && !strcmp((const char*)h_anchor_attr,"proportional"))
          subshape->h_anchor_method = OFFSET_METHOD_PROPORTIONAL;
        else {
          fprintf (stderr,
                   "illegal h_anchor “%s”, defaulting to fixed.left\n",
                   h_anchor_attr);
        }

        info->subshapes = g_list_append(info->subshapes, subshape);

        /* gfree( tmpinfo );*/
        xmlFree(v_anchor_attr);
        xmlFree(h_anchor_attr);

        el = (GraphicElement *)subshape;
      }
    }
    if (el) {
      el->any.s = s;
      if (el->any.s.font) {
        el->any.s.font = g_object_ref (s.font);
      }
      info->display_list = g_list_append (info->display_list, el);
    }
    if (s.font) {
      g_clear_object (&s.font);
    }
  }
}

static void
check_point(ShapeInfo *info, Point *pt)
{
  if (pt->x < info->shape_bounds.left)
    info->shape_bounds.left = pt->x;
  if (pt->x > info->shape_bounds.right)
    info->shape_bounds.right = pt->x;
  if (pt->y < info->shape_bounds.top)
    info->shape_bounds.top = pt->y;
  if (pt->y > info->shape_bounds.bottom)
    info->shape_bounds.bottom = pt->y;
}


static void
update_bounds (ShapeInfo *info)
{
  GList *tmp;
  Point pt;
  for (tmp = info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    int i;

    switch (el->type) {
      case GE_SUBSHAPE:
        /* subshapes are not supposed to have an influence on bounds */
        break;
      case GE_LINE:
        check_point (info, &(el->line.p1));
        check_point (info, &(el->line.p2));
        break;
      case GE_POLYLINE:
        for (i = 0; i < el->polyline.npoints; i++) {
          check_point (info, &(el->polyline.points[i]));
        }
        break;
      case GE_POLYGON:
        for (i = 0; i < el->polygon.npoints; i++) {
          check_point (info, &(el->polygon.points[i]));
        }
        break;
      case GE_RECT:
        check_point (info, &(el->rect.corner1));
        check_point (info, &(el->rect.corner2));
        /* el->rect.corner_radius has no infulence on the bounding rectangle */
        break;
      case GE_TEXT:
        check_point (info, &(el->text.anchor));
        break;
      case GE_ELLIPSE:
        pt = el->ellipse.center;
        pt.x -= el->ellipse.width / 2.0;
        pt.y -= el->ellipse.height / 2.0;
        check_point (info, &pt);
        pt.x += el->ellipse.width;
        pt.y += el->ellipse.height;
        check_point (info, &pt);
        break;
      case GE_PATH:
      case GE_SHAPE:
#if 1
        {
          DiaRectangle bbox;
          PolyBBExtras extra = { 0, };

          polybezier_bbox (&el->path.points[0],el->path.npoints,
                           &extra,el->type == GE_SHAPE,&bbox);
          rectangle_union (&info->shape_bounds, &bbox);
        }
#else
        for (i = 0; i < el->path.npoints; i++) {
          switch (el->path.points[i].type) {
            case BEZ_CURVE_TO:
              check_point(info, &el->path.points[i].p3);
              check_point(info, &el->path.points[i].p2);
            case BEZ_MOVE_TO:
            case BEZ_LINE_TO:
              check_point(info, &el->path.points[i].p1);
          }
        }
#endif
        break;
      case GE_IMAGE:
        check_point (info, &(el->image.topleft));
        pt.x = el->image.topleft.x + el->image.width;
        pt.y = el->image.topleft.y + el->image.height;
        check_point (info, &pt);
        break;
      default:
        g_return_if_reached ();
    }
  }

  {
    real width = info->shape_bounds.right-info->shape_bounds.left;
    real height = info->shape_bounds.bottom-info->shape_bounds.top;

    if (info->default_width > 0.0 && info->default_height == 0.0) {
      info->default_height = (info->default_width / width) * height;
    } else if (info->default_height > 0.0 && info->default_width == 0.0) {
      info->default_width = (info->default_height / height) * width;
    }
  }
}


/**
 * load_shape_info:
 *
 * Contructor for ShapeInfo from file
 *
 * Load the full shape info from file potentially reusing the preloaded
 * ShapeInfo loaded by shape_typeinfo_load()
 */
static ShapeInfo *
load_shape_info (const char *filename, ShapeInfo *preload)
{
  DiaContext *ctx = dia_context_new (_("Load Custom Shape"));
  xmlDocPtr doc = dia_io_load_document (filename, ctx, NULL);
  xmlNsPtr shape_ns, svg_ns;
  xmlNodePtr node, root, ext_node = NULL;
  ShapeInfo *info = NULL;
  char *tmp = NULL;
  int i;

  dia_context_set_filename (ctx, filename);

  if (!doc) {
    dia_context_add_message (ctx,
                             _("Loading Custom Shape from %s failed"),
                             filename);

    goto out;
  }

  /* skip (emacs) comments */
  root = doc->xmlRootNode;
  while (root && (root->type != XML_ELEMENT_NODE)) root = root->next;
  if (!root || xmlIsBlankNode(root)) {
    goto out;
  }

  if (!(shape_ns = xmlSearchNsByHref (doc,
                                      root,
                                      (const xmlChar *) "http://www.daa.com.au/~james/dia-shape-ns"))) {
    dia_context_add_message (ctx, _("could not find shape namespace"));

    goto out;
  }

  if (!(svg_ns = xmlSearchNsByHref (doc,
                                    root,
                                    (const xmlChar *) "http://www.w3.org/2000/svg"))) {
    dia_context_add_message (ctx, _("could not find svg namespace"));

    goto out;
  }

  if (root->ns != shape_ns ||
      xmlStrcmp (root->name, (const xmlChar *) "shape")) {
    dia_context_add_message (ctx,
                             _("root element was %s — expecting shape"),
                             root->name);

    goto out;
  }

  if (preload)
    info = preload;
  else
    info = g_new0(ShapeInfo, 1);
  info->loaded = TRUE;
  info->shape_bounds.top = DBL_MAX;
  info->shape_bounds.left = DBL_MAX;
  info->shape_bounds.bottom = -DBL_MAX;
  info->shape_bounds.right = -DBL_MAX;
  info->aspect_type = SHAPE_ASPECT_FREE;
  info->default_width = 0.0;
  info->default_height = 0.0;
  info->main_cp = -1;
  info->object_flags = 0;

  i = 0;
  for (node = root->xmlChildrenNode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node)) continue;
    if (node->type != XML_ELEMENT_NODE) continue;
    if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"name")) {
      tmp = (gchar *) xmlNodeGetContent(node);
      if (preload) {
        if (strcmp (tmp, info->name) != 0)
          g_warning ("Shape(preload) '%s' can't change name '%s'", info->name, tmp);
        /* the key name is already used as key in name_to_info */
      } else {
        g_clear_pointer (&info->name, g_free);
        info->name = g_strdup (tmp);
      }
      dia_clear_xml_string (&tmp);
    } else if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"icon")) {
      tmp = (char *) xmlNodeGetContent(node);
      if (preload) {
        if (strstr (info->icon, tmp) == NULL) /* the left including the absolute path */
          g_warning ("Shape(preload) '%s' can't change icon '%s'", info->icon, tmp);
        /* the key name is already used as key in name_to_info */
      } else {
        g_clear_pointer (&info->icon, g_free);
        info->icon = custom_get_relative_filename (filename, tmp);
      }
      dia_clear_xml_string (&tmp);
    } else if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"connections")) {
      GArray *arr = g_array_new(FALSE, FALSE, sizeof(Point));
      xmlNodePtr pt_node;

      for (pt_node = node->xmlChildrenNode;
           pt_node != NULL;
           pt_node = pt_node->next) {
        if (xmlIsBlankNode(pt_node)) {
          continue;
        }

        if (pt_node->ns == shape_ns && !xmlStrcmp(pt_node->name, (const xmlChar *)"point")) {
          Point pt = { 0.0, 0.0 };
          xmlChar *str;

          str = xmlGetProp (pt_node, (const xmlChar *) "x");
          if (str) {
            pt.x = g_ascii_strtod ((char *) str, NULL);
            dia_clear_xml_string (&str);
          }

          str = xmlGetProp (pt_node, (const xmlChar *) "y");
          if (str) {
            pt.y = g_ascii_strtod ((char *) str, NULL);
            dia_clear_xml_string (&str);
          }

          g_array_append_val (arr, pt);

          str = xmlGetProp (pt_node, (const xmlChar *) "main");
          if (str && str[0] != '\0') {
            if (info->main_cp != -1) {
              message_warning ("More than one main connection point in %s.  Only the first one will be used.\n",
                               info->name);
            } else {
              info->main_cp = i;
            }
          }
          dia_clear_xml_string (&str);
        }
        i++;
      }
      info->nconnections = arr->len;
      info->connections = (Point *)arr->data;
      g_array_free(arr, FALSE);
    } else if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"can-parent")) {
      info->object_flags |= DIA_OBJECT_CAN_PARENT;
    } else if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"textbox")) {
      xmlChar *str;

      str = xmlGetProp (node, (const xmlChar *) "x1");
      if (str) {
        info->text_bounds.left = g_ascii_strtod ((char *) str, NULL);
        dia_clear_xml_string (&str);
      }

      str = xmlGetProp (node, (const xmlChar *) "y1");
      if (str) {
        info->text_bounds.top = g_ascii_strtod ((char *) str, NULL);
        dia_clear_xml_string (&str);
      }

      str = xmlGetProp (node, (const xmlChar *) "x2");
      if (str) {
        info->text_bounds.right = g_ascii_strtod ((char *) str, NULL);
        dia_clear_xml_string (&str);
      }

      str = xmlGetProp (node, (const xmlChar *) "y2");
      if (str) {
        info->text_bounds.bottom = g_ascii_strtod ((char *) str, NULL);
        dia_clear_xml_string (&str);
      }

      info->resize_with_text = TRUE;
      str = xmlGetProp (node, (const xmlChar *) "resize");
      if (str) {
        info->resize_with_text = TRUE;
        if (!xmlStrcmp (str, (const xmlChar *) "no")) {
          info->resize_with_text = FALSE;
        }
      }
      dia_clear_xml_string (&str);
      info->text_align = DIA_ALIGN_CENTRE;
      str = xmlGetProp (node, (const xmlChar *) "align");
      if (str) {
        if (!xmlStrcmp (str, (const xmlChar *) "left")) {
          info->text_align = DIA_ALIGN_LEFT;
        } else if (!xmlStrcmp (str, (const xmlChar *) "right")) {
          info->text_align = DIA_ALIGN_RIGHT;
        }
      }
      dia_clear_xml_string (&str);
      info->has_text = TRUE;
    } else if (node->ns == shape_ns && !xmlStrcmp(node->name, (const xmlChar *)"aspectratio")) {
      tmp = (gchar *) xmlGetProp(node, (const xmlChar *)"type");
      if (tmp) {
        if (!strcmp (tmp, "free")) {
          info->aspect_type = SHAPE_ASPECT_FREE;
        } else if (!strcmp (tmp, "fixed")) {
          info->aspect_type = SHAPE_ASPECT_FIXED;
        } else if (!strcmp (tmp, "range")) {
          xmlChar *str;

          info->aspect_type = SHAPE_ASPECT_RANGE;
          info->aspect_min = 0.0;
          info->aspect_max = G_MAXFLOAT;

          str = xmlGetProp (node, (const xmlChar *) "min");
          if (str) {
            info->aspect_min = g_ascii_strtod ((char *) str, NULL);
            dia_clear_xml_string (&str);
          }

          str = xmlGetProp (node, (const xmlChar *) "max");
          if (str) {
            info->aspect_max = g_ascii_strtod ((char *) str, NULL);
            dia_clear_xml_string (&str);
          }

          if (info->aspect_max < info->aspect_min) {
            double asp = info->aspect_max;
            info->aspect_max = info->aspect_min;
            info->aspect_min = asp;
          }
        }

        dia_clear_xml_string (&tmp);
      }
    } else if (node->ns == shape_ns && (!xmlStrcmp(node->name, (const xmlChar *)"default-width") || !xmlStrcmp(node->name, (const xmlChar *) "default-height"))) {
      double val = 0.0;
      int unit_ssize = 0;
      int ssize = 0;

      tmp = (char *) xmlNodeGetContent (node);
      ssize = strlen (tmp);

      val = g_ascii_strtod (tmp, NULL);

      for (int j = 0; j < DIA_LAST_UNIT; j++) {
        unit_ssize = strlen (dia_unit_get_symbol (j));
        if (ssize > unit_ssize &&
            !g_strcmp0 (tmp + (ssize - unit_ssize),
                        dia_unit_get_symbol (j))) {
          val *= (dia_unit_get_factor (j) / 28.346457);
          break;
        }
      }

      if (!xmlStrcmp (node->name, (const xmlChar *) "default-width")) {
        info->default_width = val;
      } else {
        info->default_height = val;
      }

      dia_clear_xml_string (&tmp);
    } else if (node->ns == svg_ns && !xmlStrcmp(node->name, (const xmlChar *)"svg")) {
      DiaSvgStyle s = {
        1.0,
        DIA_SVG_COLOUR_FOREGROUND,
        1.0,
        DIA_SVG_COLOUR_NONE,
        1.0,
        DIA_LINE_CAPS_DEFAULT,
        DIA_LINE_JOIN_DEFAULT,
        DIA_LINE_STYLE_DEFAULT,
        1.0
      };

      dia_svg_parse_style(node, &s, -1);
      parse_svg_node(info, node, svg_ns, &s, filename);
      update_bounds(info);
    } else if (!xmlStrcmp(node->name, (const xmlChar *)"ext_attributes")) {
      ext_node = node;
    }
  }
  /*MC 11/03 parse ext attributes if any & prepare prop tables */
  custom_setup_properties (info, ext_node);

out:
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);

  return info;
}


void
shape_info_print (ShapeInfo *info)
{
  GList *tmp;
  int i;

  g_print ("Name        : %s\n", info->name);
  g_print ("Connections :\n");
  for (i = 0; i < info->nconnections; i++) {
    g_print("  (%g, %g)\n", info->connections[i].x, info->connections[i].y);
  }
  g_print ("Shape bounds: (%g, %g) - (%g, %g)\n",
           info->shape_bounds.left, info->shape_bounds.top,
           info->shape_bounds.right, info->shape_bounds.bottom);
  if (info->has_text) {
    g_print ("Text bounds : (%g, %g) - (%g, %g)\n",
             info->text_bounds.left, info->text_bounds.top,
             info->text_bounds.right, info->text_bounds.bottom);
  }
  g_print ("Aspect ratio: ");
  switch (info->aspect_type) {
    case SHAPE_ASPECT_FREE:
      g_print ("free\n");
      break;
    case SHAPE_ASPECT_FIXED:
      g_print ("fixed\n");
      break;
    case SHAPE_ASPECT_RANGE:
      g_print ("range %g - %g\n", info->aspect_min, info->aspect_max);
      break;
    default:
      g_return_if_reached ();
  }
  g_print ("Display list:\n");
  for (tmp = info->display_list; tmp; tmp = tmp->next) {
    GraphicElement *el = tmp->data;
    int j;

    switch (el->type) {
      case GE_LINE:
        g_print ("  line: (%g, %g) (%g, %g)\n", el->line.p1.x, el->line.p1.y,
                 el->line.p2.x, el->line.p2.y);
        break;
      case GE_POLYLINE:
        g_print ("  polyline:");
        for (j = 0; j < el->polyline.npoints; j++) {
          g_print (" (%g, %g)", el->polyline.points[j].x,
                   el->polyline.points[j].y);
        }
        g_print ("\n");
        break;
      case GE_POLYGON:
        g_print ("  polygon:");
        for (j = 0; j < el->polygon.npoints; j++) {
          g_print (" (%g, %g)", el->polygon.points[j].x,
                   el->polygon.points[j].y);
        }
        g_print ("\n");
        break;
      case GE_RECT:
        g_print ("  rect: (%g, %g) (%g, %g)\n",
                 el->rect.corner1.x, el->rect.corner1.y,
                 el->rect.corner2.x, el->rect.corner2.y);
        break;
      case GE_TEXT:
        g_print ("  text: (%g, %g)\n",
                 el->text.anchor.x, el->text.anchor.y);
        break;
      case GE_ELLIPSE:
        g_print ("  ellipse: center=(%g, %g) width=%g height=%g\n",
                 el->ellipse.center.x, el->ellipse.center.y,
                 el->ellipse.width, el->ellipse.height);
        break;
      case GE_PATH:
        g_print ("  path:");
        for (j = 0; j < el->path.npoints; j++) {
          switch (el->path.points[j].type) {
            case BEZ_MOVE_TO:
              g_print (" M (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y);
              break;
            case BEZ_LINE_TO:
              g_print (" L (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y);
              break;
            case BEZ_CURVE_TO:
              g_print (" C (%g, %g) (%g, %g) (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y, el->path.points[j].p2.x,
                       el->path.points[j].p2.y, el->path.points[j].p3.x,
                       el->path.points[j].p3.y);
              break;
            default:
              g_return_if_reached ();
          }
        }
        break;
      case GE_SHAPE:
        g_print("  shape:");
        for (j = 0; j < el->path.npoints; j++) {
          switch (el->path.points[j].type) {
            case BEZ_MOVE_TO:
              g_print (" M (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y);
              break;
            case BEZ_LINE_TO:
              g_print (" L (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y);
              break;
            case BEZ_CURVE_TO:
              g_print (" C (%g, %g) (%g, %g) (%g, %g)", el->path.points[j].p1.x,
                       el->path.points[j].p1.y, el->path.points[j].p2.x,
                       el->path.points[j].p2.y, el->path.points[j].p3.x,
                       el->path.points[j].p3.y);
              break;
            default:
              g_return_if_reached ();
          }
        }
        break;
      case GE_IMAGE:
        g_print ("  image topleft=(%g, %g) width=%g height=%g file=%s\n",
                 el->image.topleft.x, el->image.topleft.y,
                 el->image.width, el->image.height,
                 el->image.image ? dia_image_filename(el->image.image) : "(nil)");
        break;
      case GE_SUBSHAPE:
        g_print ("   subshape\n");
        break;
      default:
        break;
    }
  }
  g_print ("\n");
}
