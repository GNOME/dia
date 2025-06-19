/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-render-script-import.c - plugin for dia
 * Copyright (C) 2009, 2014 Hans Breuer, <hans@breuer.org>
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

#define G_LOG_DOMAIN "DiaRS"

/*!  \file dia-render-script-import.c import of dia-render-script either to
 * diagram with objects or maybe as one object */
#include "config.h"

#include <glib/gi18n-lib.h>

#include "geometry.h"
#include "color.h"
#include "diagramdata.h"
#include "group.h"
#include "diaimportrenderer.h"
#include "dia-io.h"
#include "dia-layer.h"

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "dia-render-script.h"

/*!
 * \defgroup DiaRenderScriptImport Dia Render Script Import
 * \ingroup ImportFilters
 * \brief Importing _Layer, _DiaObject from their XML representation saved with _DrsRenderer
 */

static real
_parse_real (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp(node, (const xmlChar *)attrib);
  real val = 0;
  if (str) {
    val = g_ascii_strtod ((gchar *)str, NULL);
    xmlFree(str);
  }
  return val;
}
static Point
_parse_point (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp(node, (const xmlChar *)attrib);
  Point pt = { 0, 0 };
  if (str) {
    gchar *ep = NULL;
    pt.x = g_ascii_strtod ((gchar *)str, &ep);
    if (ep) {
      ++ep;
      pt.y = g_ascii_strtod (ep, NULL);
    }
    xmlFree(str);
  }
  return pt;
}
static GArray *
_parse_points (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp(node, (const xmlChar *)attrib);
  GArray *arr = NULL;

  if (str) {
    gint i;
    gchar **split = g_strsplit ((gchar *)str, " ", -1);
    gchar *val, *ep = NULL;
    arr = g_array_new(FALSE, TRUE, sizeof(Point));
    for (i = 0; split[i] != NULL; ++i)
      /* count them */;
    g_array_set_size (arr, i);
    for (i = 0; split[i] != 0; ++i) {
      Point *pt = &g_array_index(arr, Point, i);
      val = split[i];
      pt->x = g_ascii_strtod (val, &ep);
      pt->y = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
    }
    g_strfreev(split);
    xmlFree(str);
  }
  return arr;
}
static GArray *
_parse_bezpoints (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp(node, (const xmlChar *)attrib);
  GArray *arr = NULL;

  if (str) {
    gint i;
    gchar **split = g_strsplit ((gchar *)str, " ", -1);
    gchar *val, *ep = NULL;
    arr = g_array_new(FALSE, TRUE, sizeof(BezPoint));
    for (i = 0; split[i] != NULL; ++i)
      /* count them */;
    g_array_set_size (arr, i);
    for (i = 0; split[i] != 0; ++i) {
      BezPoint *pt = &g_array_index(arr, BezPoint, i);
      val = split[i];
      pt->type = val[0] == 'M' ? BEZ_MOVE_TO : (val[0] == 'L' ? BEZ_LINE_TO : BEZ_CURVE_TO);
      ep = (gchar *)val + 1;

      pt->p1.x = ep ? g_ascii_strtod (ep, &ep) : 0;
      pt->p1.y = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
      if (pt->type == BEZ_CURVE_TO) {
	pt->p2.x = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
	pt->p2.y = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
	pt->p3.x = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
	pt->p3.y = ep ? ++ep, g_ascii_strtod (ep, &ep) : 0;
      }
    }
    g_strfreev(split);
    xmlFree(str);
  }
  return arr;
}
static Color *
_parse_color (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp(node, (const xmlChar *)attrib);
  Color *val = NULL;

  if (str) {
    int r, g, b, a = 255;
    int n = sscanf ((gchar *)str, "#%02x%02x%02x%02x", &r, &g, &b, &a);

    if (n > 2) {
      val = g_new (Color, 1);
      val->red   = r / 255.0;
      val->green = g / 255.0;
      val->blue  = b / 255.0;
      val->alpha = a / 255.0;
    }
    xmlFree(str);
  }
  return val;
}


static DiaLineStyle
_parse_linestyle (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp (node, (const xmlChar *) attrib);
  DiaLineStyle val = DIA_LINE_STYLE_SOLID;

  if (str) {
    if (g_strcmp0 ((const char *) str, "dashed") == 0) {
      val = DIA_LINE_STYLE_DASHED;
    } else if  (g_strcmp0 ((const char *) str, "dash-dot") == 0) {
      val = DIA_LINE_STYLE_DASH_DOT;
    } else if  (g_strcmp0 ((const char *) str, "dash-dot-dot") == 0) {
      val = DIA_LINE_STYLE_DASH_DOT_DOT;
    } else if (g_strcmp0 ((const char *) str, "dotted") == 0) {
      val = DIA_LINE_STYLE_DOTTED;
    } else if (g_strcmp0 ((const char *) str, "solid") != 0) {
      g_warning ("DRS: unknown linestyle: %s", str);
    }
  }

  dia_clear_xml_string (&str);

  return val;
}


static DiaLineCaps
_parse_linecaps (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp (node, (const xmlChar *) attrib);
  DiaLineCaps val = DIA_LINE_CAPS_BUTT;

  if (str) {
    if (g_strcmp0 ((const char *) str, "round") == 0) {
      val = DIA_LINE_CAPS_ROUND;
    } else if (g_strcmp0 ((const char *) str, "projecting") == 0) {
      val = DIA_LINE_CAPS_PROJECTING;
    } else if (g_strcmp0 ((const char *) str, "butt") != 0) {
      g_warning ("unknown linecaps: %s", str);
    }
  }

  dia_clear_xml_string (&str);

  return val;
}


static DiaLineJoin
_parse_linejoin (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp (node, (const xmlChar *) attrib);
  DiaLineJoin val =DIA_LINE_JOIN_MITER;

  if (str) {
    if (g_strcmp0 ((const char *) str, "round") == 0) {
      val = DIA_LINE_JOIN_ROUND;
    } else if (g_strcmp0 ((const char *) str, "bevel") == 0) {
      val = DIA_LINE_JOIN_BEVEL;
    } else if (g_strcmp0 ((const char *) str, "miter") != 0) {
      g_warning ("unknown linejoin: %s", str);
    }
  }

  dia_clear_xml_string (&str);

  return val;
}


static DiaFillStyle
_parse_fillstyle (xmlNodePtr node, const char *attrib)
{
  /* ToDo: complain about everything but */
  return DIA_FILL_STYLE_SOLID;
}


static DiaAlignment
_parse_alignment (xmlNodePtr node, const char *attrib)
{
  xmlChar *str = xmlGetProp (node, (const xmlChar *) attrib);
  DiaAlignment val = DIA_ALIGN_LEFT;

  if (str) {
    if (g_strcmp0 ((const char *) str, "center") == 0) {
      val = DIA_ALIGN_CENTRE;
    } else if (g_strcmp0 ((const char *) str, "right") == 0) {
      val = DIA_ALIGN_RIGHT;
    } else if (g_strcmp0 ((const char *) str, "left") != 0) {
      g_warning ("unknown alignment: %s", str);
    }
  }

  dia_clear_xml_string (&str);

  return val;
}


static DiaFont *
_parse_font (xmlNodePtr node)
{
  DiaFont *font = NULL;
  xmlChar *str;
  xmlChar *fam = xmlGetProp(node, (const xmlChar *)"family");
  real size = _parse_real (node, "size");

  if (size <= 0.0)
    size = 1.0;
  if (fam)
    font = dia_font_new ((const char *)fam, 0, size);
  if (!font)
    font = dia_font_new_from_style (DIA_FONT_SANS, size);
  str = xmlGetProp(node, (const xmlChar *)"weight");
  if (str) {
    dia_font_set_weight_from_string (font, (const char*)str);
    xmlFree (str);
  }
  str = xmlGetProp(node, (const xmlChar *)"slant");
  if (str) {
    dia_font_set_slant_from_string (font, (const char*)str);
    xmlFree (str);
  }

  if (fam)
    xmlFree (fam);

  return font;
}
/*!
 * \brief Create a _DiaObject from it's rendering XML representation
 * \ingroup DiaRenderScriptImport
 */
static DiaObject *
_render_object (xmlNodePtr render, DiaContext *ctx)
{
  DiaRenderer *ir = g_object_new (DIA_TYPE_IMPORT_RENDERER, NULL);
  DiaObject *o;
  xmlNodePtr node;

  for (node = render->children; node; node = node->next) {
    if (xmlStrcmp (node->name, (const xmlChar *) "set-linewidth") == 0) {
      dia_renderer_set_linewidth (ir, _parse_real (node, "width"));
    } else if (xmlStrcmp (node->name, (const xmlChar *) "set-linestyle") == 0) {
      dia_renderer_set_linestyle (ir,
                                  _parse_linestyle (node, "mode"),
                                  _parse_real (node, "dash-length"));
    } else if (xmlStrcmp (node->name, (const xmlChar *) "set-linecaps") == 0) {
      dia_renderer_set_linecaps (ir, _parse_linecaps (node, "mode"));
    } else if (xmlStrcmp (node->name, (const xmlChar *) "set-linejoin") == 0) {
      dia_renderer_set_linejoin (ir, _parse_linejoin (node, "mode"));
    } else if (xmlStrcmp (node->name, (const xmlChar *) "set-fillstyle") == 0) {
      dia_renderer_set_fillstyle (ir, _parse_fillstyle (node, "mode"));
    /* ToDo: set-linejoin, set-fillstyle */
    } else if (xmlStrcmp (node->name, (const xmlChar *) "set-font") == 0) {
      DiaFont *font = _parse_font (node);
      dia_renderer_set_font (ir, font, _parse_real (node, "height"));
      g_clear_object (&font);
    } else {
      Color *stroke = _parse_color (node, "stroke");
      Color *fill = _parse_color (node, "fill");

      if (xmlStrcmp (node->name, (const xmlChar *)"line") == 0) {
        Point p1 = _parse_point (node, "start");
        Point p2 = _parse_point (node, "end");
        if (stroke) {
          dia_renderer_draw_line (ir, &p1, &p2, stroke);
        }
      } else if (   xmlStrcmp (node->name, (const xmlChar *) "polyline") == 0
                 || xmlStrcmp (node->name, (const xmlChar *) "rounded-polyline") == 0) {
        GArray *path = _parse_points (node, "points");
        real r = _parse_real (node,"r");
        if (path) {
          if (stroke) {
            dia_renderer_draw_rounded_polyline (ir, &g_array_index (path,Point,0), path->len, stroke, r);
          }
          g_array_free (path, TRUE);
        }
      } else if (   xmlStrcmp (node->name, (const xmlChar *) "polygon") == 0) {
        GArray *path = _parse_points (node, "points");
        if (path) {
          if (fill || stroke) {
            dia_renderer_draw_polygon (ir, &g_array_index (path,Point,0), path->len, fill, stroke);
          }
          g_array_free (path, TRUE);
        }
      } else if (xmlStrcmp (node->name, (const xmlChar *)"arc") == 0) {
        Point center = _parse_point (node, "center");
        if (fill) {
          dia_renderer_fill_arc (ir,
                                 &center,
                                 _parse_real (node, "width"),
                                 _parse_real (node, "height"),
                                 _parse_real (node, "angle1"),
                                 _parse_real (node, "angle2"),
                                 fill);
        }
        if (stroke) {
          dia_renderer_draw_arc (ir,
                                 &center,
                                 _parse_real (node, "width"),
                                 _parse_real (node, "height"),
                                 _parse_real (node, "angle1"),
                                 _parse_real (node, "angle2"),
                                 stroke);
        }
      } else if (xmlStrcmp (node->name, (const xmlChar *) "ellipse") == 0) {
        Point center = _parse_point (node, "center");
        dia_renderer_draw_ellipse (ir,
                                   &center,
                                   _parse_real (node, "width"),
                                   _parse_real (node, "height"),
                                   fill,
                                   stroke);
      } else if (xmlStrcmp (node->name, (const xmlChar *) "bezier") == 0) {
        GArray *path = _parse_bezpoints (node, "bezpoints");
        if (path) {
          if (fill && stroke) {
            dia_renderer_draw_beziergon (ir, &g_array_index (path,BezPoint,0), path->len, fill, stroke);
          } else if (fill) {
            dia_renderer_draw_beziergon (ir, &g_array_index (path,BezPoint,0), path->len, fill, NULL);
          } else if (stroke) {
            dia_renderer_draw_bezier (ir, &g_array_index (path,BezPoint,0), path->len, stroke);
          }
          g_array_free (path, TRUE);
        }
      } else if (   xmlStrcmp (node->name, (const xmlChar *) "rect") == 0
                 || xmlStrcmp (node->name, (const xmlChar *) "rounded-rect") == 0) {
        Point ul = _parse_point (node, "lefttop");
        Point lr = _parse_point (node, "rightbottom");
        real r = _parse_real (node,"r");
        if (fill || stroke) {
          dia_renderer_draw_rounded_rect (ir, &ul, &lr, fill, stroke, r);
        }
      } else if (xmlStrcmp (node->name, (const xmlChar *) "string") == 0) {
        Point pos = _parse_point (node, "pos");
        DiaAlignment align = _parse_alignment (node, "alignment");
        xmlChar *text = xmlNodeGetContent (node);
        if (text) {
          dia_renderer_draw_string (ir, (const char*) text, &pos, align, fill);
          xmlFree (text);
        }
      } else if (node->type == XML_ELEMENT_NODE) {
        g_warning ("%s not handled", node->name);
      }
      g_clear_pointer (&fill, g_free);
      g_clear_pointer (&stroke, g_free);
    }
  }

  o = dia_import_renderer_get_objects (ir);

  g_clear_object (&ir);

  return o;
}


static xmlNodePtr
find_child_named (xmlNodePtr node, const char *name)
{
  for (node = node->children; node; node = node->next) {
    if (xmlStrcmp (node->name, (const xmlChar *)name) == 0) {
      return node;
    }
  }
  return NULL;
}

/*!
 * \brief Parse _DiaObject from the given node
 * Fill a GList* with objects which is to be put in a
 * diagram or a group by the caller.
 * Can be called recursively to allow groups in groups.
 * This is only using the render branch of the file, if the
 * object type is not registered with Dia. Otherwise the objects
 * are just created from their type and properties.
 * \ingroup DiaRenderScriptImport
 */
static GList*
read_items (xmlNodePtr startnode, DiaContext *ctx)
{
  xmlNodePtr node;
  GList *items = NULL;

  for (node = startnode; node != NULL; node = node->next) {
    if (xmlIsBlankNode(node))
      continue;
    if (node->type != XML_ELEMENT_NODE)
      continue;
    if (!xmlStrcmp(node->name, (const xmlChar *)"object")) {
      xmlChar *sType = xmlGetProp(node, (const xmlChar *)"type");
      const DiaObjectType *ot = object_get_type ((gchar *)sType);
      xmlNodePtr props = NULL, render = NULL;

      props = find_child_named (node, "properties");
      render = find_child_named (node, "render");

      if (ot && !ot->ops) {
	GList *moreitems;
        /* FIXME: 'render' is also the grouping element */
	moreitems = read_items (render->children, ctx);
	if (moreitems) {
	  DiaObject *group = group_create (moreitems);
	    /* apply group props, e.g. transform */
	  object_load_props (group, props, ctx);
	    /* group eats list */
	  items = g_list_append (items, group);
	}
      } else if (ot) {
        Point startpoint = {0.0,0.0};
        Handle *handle1,*handle2;
	DiaObject *o;

	o = ot->ops->create(&startpoint,
                            ot->default_user_data,
			    &handle1,&handle2);
	if (o) {
	  object_load_props (o, props, ctx);
	  items = g_list_append (items, o);
	}
      } else if (render) {
        DiaObject *o = _render_object (render, ctx);
        if (o) {
          items = g_list_append (items, o);
        }
      } else {
        g_debug ("%s: DRS-Import: %s?", G_STRLOC, node->name);
      }
    } else {
      g_debug ("%s: DRS-Import: %s?", G_STRLOC, node->name);
    }
  }
  return items;
}

/*!
 * \brief Imports the given DRS file
 * Restore a diagram from it's Dia Render Script representation. This function
 * is handling diagram and layers creation itself and delegates the object creation
 * to read_items().
 * \ingroup DiaRenderScriptImport
 */
gboolean
import_drs (const gchar *filename, DiagramData *dia, DiaContext *ctx, void* user_data)
{
  GList *item, *items;
  xmlDocPtr doc = dia_io_load_document (filename, ctx, NULL);
  xmlNodePtr root = NULL, node;
  DiaLayer *active_layer = NULL;

  for (node = doc->children; node; node = node->next)
    if (xmlStrcmp (node->name, (const xmlChar *) "drs") == 0)
      root = node;

  if (!root || !(root = find_child_named (root, "diagram"))) {
    dia_context_add_message (ctx, _("Broken file?"));
    return FALSE;
  }

  for (node = root->children; node != NULL; node = node->next) {
    if (xmlStrcmp (node->name, (const xmlChar *) "layer") == 0) {
      xmlChar *str;
      xmlChar *name = xmlGetProp (node, (const xmlChar *) "name");
      DiaLayer *layer = dia_layer_new (name ? (char *) name : _("Layer"), dia);

      dia_clear_xml_string (&name);

      str = xmlGetProp (node, (const xmlChar *) "active");
      if (xmlStrcmp (str, (const xmlChar *) "true")) {
        g_set_object (&active_layer, layer);
      }
      dia_clear_xml_string (&str);

      items = read_items (node->children, ctx);
      for (item = items; item != NULL; item = g_list_next (item)) {
        DiaObject *obj = DIA_OBJECT (item->data);
        dia_layer_add_object (layer, obj);
      }
      g_list_free (items);
      data_add_layer (dia, layer);

      g_clear_object (&layer);
    }
  }

  if (active_layer) {
    data_set_active_layer (dia, active_layer);
  }

  xmlFreeDoc(doc);
  g_clear_object (&active_layer);

  return TRUE;
}
