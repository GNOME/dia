/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-render-script-renderer.c - plugin for dia
 * Copyright (C) 2009, 2014 Hans Breuer <hans@breuer.org>
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

/*! A DiaRenderer mapping every renderer method to some metafile representation */

#define G_LOG_DOMAIN "DiaRenderScript"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib.h>

#include "diarenderer.h"

#include "object.h" /* object->type->name */
#include "dia_image.h" /* dia_image_filename */
#include "properties.h" /* object_save_props */

#include <pango/pango.h>

#include "dia-render-script-renderer.h"

#include "diatransformrenderer.h"
#include "group.h"

G_DEFINE_TYPE (DrsRenderer, drs_renderer, DIA_TYPE_RENDERER);

enum {
  PROP_0,
  PROP_FONT,
  PROP_FONT_HEIGHT,
  LAST_PROP
};


static void
_node_set_real (xmlNodePtr node, const char *name, real v)
{
  gchar value[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd (value, sizeof (value), "%g", v);
  xmlSetProp (node, (const xmlChar *) name, (xmlChar *) value);
}


static void
drs_renderer_set_font (DiaRenderer *self, DiaFont *font, double height)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const PangoFontDescription *pfd = dia_font_get_description (font);
  char *desc = pango_font_description_to_string (pfd);

  g_set_object (&renderer->font, font);
  renderer->font_height = height;

  node =  xmlNewChild (renderer->root, NULL, (const xmlChar *) "set-font", NULL);
  xmlSetProp (node, (const xmlChar *) "description", (xmlChar *) desc);

  xmlSetProp (node, (const xmlChar *) "family", (xmlChar *) dia_font_get_family (font));
  xmlSetProp (node, (const xmlChar *) "weight", (xmlChar *) dia_font_get_weight_string (font));
  xmlSetProp (node, (const xmlChar *) "slant", (xmlChar *) dia_font_get_slant_string (font));
  _node_set_real (node, "size", dia_font_get_size (font));
  _node_set_real (node, "height", height);

  g_clear_pointer (&desc, g_free);
}


static void
drs_renderer_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  DrsRenderer *self = DRS_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      drs_renderer_set_font (DIA_RENDERER (self),
                             DIA_FONT (g_value_get_object (value)),
                             self->font_height);
      break;
    case PROP_FONT_HEIGHT:
      drs_renderer_set_font (DIA_RENDERER (self),
                             self->font,
                             g_value_get_double (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
drs_renderer_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  DrsRenderer *self = DRS_RENDERER (object);

  switch (property_id) {
    case PROP_FONT:
      g_value_set_object (value, self->font);
      break;
    case PROP_FONT_HEIGHT:
      g_value_set_double (value, self->font_height);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


/* destructor */
static void
drs_renderer_finalize (GObject *object)
{
  DrsRenderer *renderer = DRS_RENDERER (object);

  g_clear_object (&renderer->font);

  g_queue_free (renderer->parents);
  g_queue_free (renderer->matrices);
  g_clear_object (&renderer->ctx);

  G_OBJECT_CLASS (drs_renderer_parent_class)->finalize (object);
}


/*
 * renderer methods
 */
static void
draw_object (DiaRenderer *self,
             DiaObject   *object,
             DiaMatrix   *matrix)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  DiaMatrix *m = g_queue_peek_tail (renderer->matrices);
  xmlNodePtr node;

  g_queue_push_tail (renderer->parents, renderer->root);
  renderer->root = node = xmlNewChild (renderer->root,
                                       NULL,
                                       (const xmlChar *) "object",
                                       NULL);
  xmlSetProp (node, (const xmlChar *) "type",
              (xmlChar *) object->type->name);
  /* if it looks like intdata store it as well */
  if (GPOINTER_TO_INT (object->type->default_user_data) > 0 &&
      GPOINTER_TO_INT (object->type->default_user_data) < 0xFF) {
    gchar buffer[30];
    g_snprintf (buffer, sizeof (buffer), "%d",
                GPOINTER_TO_INT (object->type->default_user_data));
    xmlSetProp (node, (const xmlChar *) "intdata", (xmlChar *) buffer);
  }

  if (renderer->save_props) {
    xmlNodePtr props_node;

    props_node = xmlNewChild (node, NULL,
                              (const xmlChar *) "properties", NULL);
    object_save_props (object, props_node, renderer->ctx);
  }

  if (matrix) {
    DiaMatrix *m2 = g_new0 (DiaMatrix, 1);
    if (m) {
      dia_matrix_multiply (m2, matrix, m);
    } else {
      *m2 = *matrix;
    }
    g_queue_push_tail (renderer->matrices, m2);
    /* lazy creation of our transformer */
    if (!renderer->transformer) {
      renderer->transformer = dia_transform_renderer_new (self);
    }
  }
  /* special handling for group objects:
   *  - for the render branch use DiaTransformRenderer, but not it's draw_object,
   *    to see all the children's draw_object ourself
   *  - for the object branch we rely on this draw_object being called so need
   *    to inline group_draw here
   *  - to maintain the correct transform build our own queue of matrices like
   *    the DiaTransformRenderer would do through it's draw_object
   */
  {
    g_queue_push_tail (renderer->parents, renderer->root);
    renderer->root = node = xmlNewChild (renderer->root, NULL,
                                         (const xmlChar *) "render", NULL);
    if (renderer->transformer) {
      DiaMatrix *cm = g_queue_peek_tail (renderer->matrices);

      if (IS_GROUP (object)) {
        /* reimplementation of group_draw to use this draw_object method */
        GList *list;
        DiaObject *obj;

        list = group_objects (object);
        while (list != NULL) {
          obj = (DiaObject *) list->data;

          dia_renderer_draw_object (self, obj, cm);
          list = g_list_next (list);
        }
      } else {
        /* just the leaf */
        dia_renderer_draw_object (renderer->transformer, object, cm);
      }
    } else {
      dia_object_draw (object, DIA_RENDERER (renderer));
    }
    renderer->root = g_queue_pop_tail (renderer->parents);
  }
  renderer->root = g_queue_pop_tail (renderer->parents);

  if (matrix) {
    g_queue_pop_tail (renderer->matrices);
  }

  /* one lost demand destruction */
  if (renderer->transformer && g_queue_is_empty (renderer->matrices)) {
    g_clear_object (&renderer->transformer);
    renderer->transformer = NULL;
  }
}

static void
begin_render(DiaRenderer *self, const DiaRectangle *update)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  renderer->root = node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"diagram", NULL);
#if 0
  _node_set_color (node, "bg_color", &data->bg_color);
#endif
}
static void
end_render(DiaRenderer *self)
{
  DrsRenderer *renderer = DRS_RENDERER (self);

  renderer->root = g_queue_pop_tail (renderer->parents);
}
static gboolean
is_capable_to (DiaRenderer *self, RenderCapability cap)
{
  if (RENDER_ALPHA == cap)
    return TRUE;

  return FALSE;
}

static void
_node_set_color (xmlNodePtr node, const char *name, const Color *color)
{
  gchar *value;

  value = g_strdup_printf ("#%02x%02x%02x%02x",
                           (int) (255 * color->red),
                           (int) (255 * color->green),
                           (int) (255 * color->blue),
                           (int) (255 * color->alpha));
  xmlSetProp (node, (const xmlChar *) name, (xmlChar *) value);
  g_clear_pointer (&value, g_free);
}

static void
_string_append_point (GString *str, Point *pt, gboolean first)
{
  gchar value[G_ASCII_DTOSTR_BUF_SIZE];

  if (!first)
    g_string_append (str, " ");
  g_ascii_formatd (value, sizeof(value), "%g", pt->x);
  g_string_append (str, value);
  g_string_append (str, ",");
  g_ascii_formatd (value, sizeof(value), "%g", pt->y);
  g_string_append (str, value);
}
static void
_node_set_point (xmlNodePtr node, const char *name, Point *point)
{
  GString *str;

  str = g_string_new (NULL);
  _string_append_point (str, point, TRUE);
  xmlSetProp(node, (const xmlChar *)name, (xmlChar *) str->str);

  g_string_free(str, TRUE);
}


static void
_node_set_points (xmlNodePtr node, Point *points, int num_points)
{
  GString *str;
  int i;

  str = g_string_new (NULL);

  for (i = 0; i < num_points; ++i) {
    _string_append_point (str, &points[i], i == 0);
  }
  xmlSetProp (node, (const xmlChar *) "points", (xmlChar *) str->str);

  g_string_free (str, TRUE);
}


static void
_node_set_bezpoints (xmlNodePtr node, BezPoint *points, int num_points)
{
  GString *str = NULL;
  int i;

  str = g_string_new (NULL);

  for (i = 0; i < num_points; ++i) {
    BezPoint *bpt = &points[i];

    if (i != 0) {
      g_string_append (str, " ");
    }

    switch (bpt->type) {
      case BEZ_MOVE_TO:
        g_string_append (str, "M");
        break;
      case BEZ_LINE_TO:
        g_string_append (str, "L");
        break;
      case BEZ_CURVE_TO:
        g_string_append (str, "C");
        break;
      default:
        g_return_if_reached ();
    }

    _string_append_point (str, &points[i].p1, TRUE);

    if (bpt->type == BEZ_CURVE_TO) {
      /* no space between bez-points, simplifies parsing */
      g_string_append (str, ";");
      _string_append_point (str, &points[i].p2, TRUE);
      g_string_append (str, ";");
      _string_append_point (str, &points[i].p3, TRUE);
    }
  }
  xmlSetProp (node, (const xmlChar *) "bezpoints", (xmlChar *) str->str);

  g_string_free (str, TRUE);
}


static void
set_linewidth (DiaRenderer *self, real width)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *) "set-linewidth", NULL);
  _node_set_real (node, "width", width);
}


static void
set_linecaps (DiaRenderer *self, DiaLineCaps mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  char *value = NULL;

  switch (mode) {
    case DIA_LINE_CAPS_DEFAULT:
    case DIA_LINE_CAPS_BUTT:
      value = "butt";
      break;
    case DIA_LINE_CAPS_ROUND:
      value = "round";
      break;
    case DIA_LINE_CAPS_PROJECTING:
      value = "projecting";
      break;
    default:
      g_return_if_reached ();
  }

  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *) "set-linecaps", NULL);
  xmlSetProp (node, (const xmlChar *) "mode",
              value ? (xmlChar *) value : (xmlChar *) "?");
}


static void
set_linejoin (DiaRenderer *self, DiaLineJoin mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  char *value = NULL;

  switch (mode) {
    case DIA_LINE_JOIN_DEFAULT:
    case DIA_LINE_JOIN_MITER:
      value = "miter";
      break;
    case DIA_LINE_JOIN_ROUND:
      value = "round";
      break;
    case DIA_LINE_JOIN_BEVEL:
      value = "bevel";
      break;
    default:
      g_return_if_reached ();
  }
  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *) "set-linejoin", NULL);
  xmlSetProp (node, (const xmlChar *) "mode",
              value ? (xmlChar *) value : (xmlChar *) "?");
}


static void
set_linestyle (DiaRenderer *self, DiaLineStyle mode, double dash_length)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const gchar *value = NULL;

  /* line type */
  switch (mode) {
    case DIA_LINE_STYLE_DEFAULT:
    case DIA_LINE_STYLE_SOLID:
      value = "solid";
      break;
    case DIA_LINE_STYLE_DASHED:
      value = "dashed";
      break;
    case DIA_LINE_STYLE_DASH_DOT:
      value = "dash-dot";
      break;
    case DIA_LINE_STYLE_DASH_DOT_DOT:
      value = "dash-dot-dot";
      break;
    case DIA_LINE_STYLE_DOTTED:
      value = "dotted";
      break;
    default:
      g_return_if_reached ();
  }
  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *) "set-linestyle", NULL);
  xmlSetProp (node, (const xmlChar *) "mode",
              value ? (xmlChar *) value : (xmlChar *) "?");
  if (mode != DIA_LINE_STYLE_SOLID) {
    _node_set_real (node, "dash-length", dash_length);
  }
}


static void
set_fillstyle (DiaRenderer *self, DiaFillStyle mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const char *value = NULL;

  switch (mode) {
    case DIA_FILL_STYLE_SOLID:
      value = "solid";
      break;
    default:
      g_return_if_reached ();
  }

  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *) "set-fillstyle", NULL);
  xmlSetProp (node, (const xmlChar *) "mode",
              value ? (xmlChar *) value : (xmlChar *) "?");
}


static void
draw_line (DiaRenderer *self,
           Point       *start,
           Point       *end,
           Color       *color)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild (renderer->root, NULL, (const xmlChar *) "line", NULL);
  _node_set_point (node, "start", start);
  _node_set_point (node, "end", end);
  _node_set_color (node, "stroke", color);
}

static void
draw_polyline(DiaRenderer *self,
              Point *points, int num_points,
              Color *color)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"polyline", NULL);
  _node_set_points (node, points, num_points);
  _node_set_color (node, "stroke", color);
}

static void
draw_polygon (DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  g_return_if_fail(1 < num_points);

  node = xmlNewChild(renderer->root, NULL,
                     (const xmlChar *)"polygon", NULL);
  _node_set_points (node, points, num_points);
  if (fill)
    _node_set_color (node, "fill", fill);
  if (stroke)
    _node_set_color (node, "stroke", stroke);
}

static void
_rounded_rect(DiaRenderer *self,
              Point *lefttop, Point *rightbottom,
              Color *fill, Color *stroke, real *rounding)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  if (rounding)
    node = xmlNewChild(renderer->root, NULL,
                       (const xmlChar *)"rounded-rect", NULL);
  else
    node = xmlNewChild(renderer->root, NULL,
                       (const xmlChar *)"rect", NULL);

  _node_set_point (node, "lefttop", lefttop);
  _node_set_point (node, "rightbottom", rightbottom);
  if (rounding)
    _node_set_real (node, "r", *rounding);
  if (fill)
    _node_set_color (node, "fill", fill);
  if (stroke)
    _node_set_color (node, "stroke", stroke);
}
static void
draw_rect(DiaRenderer *self,
          Point *lefttop, Point *rightbottom,
          Color *fill, Color *stroke)
{
  _rounded_rect(self, lefttop, rightbottom, fill, stroke, NULL);
}

static void
draw_rounded_rect(DiaRenderer *self,
                  Point *lefttop, Point *rightbottom,
                  Color *fill, Color *stroke, real rounding)
{
  _rounded_rect(self, lefttop, rightbottom, fill, stroke, &rounding);
}

static void
_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color,
	 gboolean fill)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"arc", NULL);
  _node_set_point (node, "center", center);
  _node_set_real (node, "width", width);
  _node_set_real (node, "height", height);
  _node_set_real (node, "angle1", angle1);
  _node_set_real (node, "angle2", angle2);
  if (fill)
    _node_set_color (node, "fill", color);
  else
    _node_set_color (node, "stroke", color);
}
static void
draw_arc(DiaRenderer *self,
	 Point *center,
	 real width, real height,
	 real angle1, real angle2,
	 Color *color)
{
  _arc (self, center, width, height, angle1, angle2, color, FALSE);
}
static void
fill_arc(DiaRenderer *self,
         Point *center,
         real width, real height,
         real angle1, real angle2,
         Color *color)
{
  _arc (self, center, width, height, angle1, angle2, color, TRUE);
}

static void
draw_ellipse(DiaRenderer *self,
             Point *center,
             real width, real height,
             Color *fill, Color *stroke)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild(renderer->root, NULL,
                     (const xmlChar *)"ellipse", NULL);
  _node_set_point (node, "center", center);
  _node_set_real (node, "width", width);
  _node_set_real (node, "height", height);
  if (fill)
    _node_set_color (node, "fill", fill);
  if (stroke)
    _node_set_color (node, "stroke", stroke);
}

static void
_bezier(DiaRenderer *self,
	BezPoint *points,
	int numpoints,
	Color *fill,
	Color *stroke)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild (renderer->root, NULL,
                      (const xmlChar *)"bezier", NULL);
  _node_set_bezpoints (node, points, numpoints);
  if (fill)
    _node_set_color (node, "fill", fill);
  if (stroke)
    _node_set_color (node, "stroke", stroke);
}
static void
draw_bezier(DiaRenderer *self,
            BezPoint *points,
            int numpoints,
            Color *color)
{
  _bezier (self, points, numpoints, NULL, color);
}
static void
draw_beziergon (DiaRenderer *self,
		BezPoint *points,
		int numpoints,
		Color *fill,
		Color *stroke)
{
  if (!fill && stroke) { /* maybe this is too clever: close path by existence of fill attribute */
    Color transparent = { 0, };
    _bezier (self, points, numpoints, &transparent, stroke);
  } else {
    _bezier (self, points, numpoints, fill, stroke);
  }
}

static void
draw_rounded_polyline (DiaRenderer *self,
                       Point *points, int num_points,
                       Color *color, real radius )
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild (renderer->root, NULL, (const xmlChar *)"rounded-polyline", NULL);
  _node_set_points (node, points, num_points);
  _node_set_color (node, "stroke", color);
  _node_set_real (node, "r", radius);
}


static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *color)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *align = NULL;

  node = xmlNewChild (renderer->root, NULL, (const xmlChar *) "string", NULL);
  _node_set_point (node, "pos", pos);
  _node_set_color (node, "fill", color);
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      align = "left";
      break;
    case DIA_ALIGN_RIGHT:
      align = "right";
      break;
    case DIA_ALIGN_CENTRE:
      align = "center";
      break;
    default:
      g_return_if_reached ();
  }

  if (align) {
    xmlSetProp (node, (const xmlChar *) "alignment", (xmlChar *) align);
  }

  xmlNodeSetContent (node, (const xmlChar *) text);
}


static void
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage *image)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *uri;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"image", NULL);
  _node_set_point (node, "point", point);
  _node_set_real (node, "width", width);
  _node_set_real (node, "height", height);
  /* TODO: also save the data? */
  uri = g_filename_to_uri (dia_image_filename(image), NULL, NULL);
  if (uri)
    xmlSetProp (node, (const xmlChar *)"uri", (xmlChar *) uri);
  g_clear_pointer (&uri, g_free);
}

static void
drs_renderer_class_init (DrsRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  drs_renderer_parent_class = g_type_class_peek_parent (klass);

  object_class->set_property = drs_renderer_set_property;
  object_class->get_property = drs_renderer_get_property;
  object_class->finalize = drs_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->draw_object = draw_object;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->draw_line    = draw_line;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect      = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;

  renderer_class->draw_bezier    = draw_bezier;
  renderer_class->draw_beziergon = draw_beziergon;

  renderer_class->draw_rounded_polyline = draw_rounded_polyline;
  /* highest level functions */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
#if 0
  renderer_class->draw_text  = draw_text;
  renderer_class->draw_text_line  = draw_text_line;
  /* TODO: more to come ... */
#endif
  /* other */
  renderer_class->is_capable_to = is_capable_to;

  g_object_class_override_property (object_class, PROP_FONT, "font");
  g_object_class_override_property (object_class, PROP_FONT_HEIGHT, "font-height");
}

/* constructor */
static void
drs_renderer_init (DrsRenderer *renderer)
{
  renderer->parents = g_queue_new ();
  renderer->save_props = FALSE;
  renderer->matrices = g_queue_new ();
}
