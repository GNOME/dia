/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-render-script-renderer.c - plugin for dia
 * Copyright (C) 2009, Hans Breuer, <Hans@Breuer.Org>
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

#include <config.h>

#define G_LOG_DOMAIN "DiaRenderScript"
#include <glib.h>

#include "intl.h"
#include "diarenderer.h"

#include "object.h" /* object->type->name */
#include "dia_image.h" /* dia_image_filename */
#include "properties.h" /* object_save_props */

#include <pango/pango.h>

#include "dia-render-script-renderer.h"

static gpointer parent_class = NULL;

/* constructor */
static void
drs_renderer_init (GTypeInstance *self, gpointer g_class)
{
  DrsRenderer *renderer = DRS_RENDERER (self);

  renderer->parents = g_queue_new ();
  renderer->save_props = FALSE;
}

/* destructor */
static void
drs_renderer_finalize (GObject *object)
{
  DrsRenderer *renderer = DRS_RENDERER (object);

  g_queue_free (renderer->parents);

  G_OBJECT_CLASS (parent_class)->finalize (object);  
}

static void drs_renderer_class_init (DrsRendererClass *klass);

GType
drs_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (DrsRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) drs_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (DrsRenderer),
        0,              /* n_preallocs */
	drs_renderer_init /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_RENDERER,
                                            "DrsRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

/* 
 * renderer methods 
 */ 
static void 
draw_object(DiaRenderer *self,
            DiaObject *object) 
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  g_queue_push_tail (renderer->parents, renderer->root);
  renderer->root = node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"object", NULL);
  xmlSetProp(node, (const xmlChar *)"type", (xmlChar *)object->type->name);
  /* if it looks like intdata store it as well */
  if ((int)object->type->default_user_data > 0 && (int)object->type->default_user_data < 0xFF) {
    gchar buffer[30];
    g_snprintf(buffer, sizeof(buffer), "%d", (int)object->type->default_user_data);
    xmlSetProp(node, (const xmlChar *)"intdata", (xmlChar *)buffer);
  }
  if (renderer->save_props) {
    xmlNodePtr props_node;
    
    props_node = xmlNewChild(node, NULL, (const xmlChar *)"properties", NULL);
    object_save_props (object, props_node);
  }
  /* TODO: special handling for group object? */
  {
    g_queue_push_tail (renderer->parents, renderer->root);
    renderer->root = node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"render", NULL);
    object->ops->draw(object, DIA_RENDERER (renderer));
    renderer->root = g_queue_pop_tail (renderer->parents);
  }
  renderer->root = g_queue_pop_tail (renderer->parents);
}

static void
begin_render(DiaRenderer *self)
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


static void
_node_set_color (xmlNodePtr node, const char *name, const Color *color)
{
  gchar *value;
  
  value = g_strdup_printf ("#%02x%02x%02x",
		           (int)ceil(255*color->red), (int)ceil(255*color->green),
		           (int)ceil(255*color->blue));
  xmlSetProp(node, (const xmlChar *)name, (xmlChar *)value);
  g_free (value);
}
static void
_node_set_real (xmlNodePtr node, const char *name, real v)
{
  gchar value[G_ASCII_DTOSTR_BUF_SIZE];

  g_ascii_formatd (value, sizeof(value), "%g", v);
  xmlSetProp (node, (const xmlChar *)name, (xmlChar *)value);
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
  
  for (i = 0; i < num_points; ++i)
    _string_append_point (str, &points[i], i == 0);
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);

  g_string_free(str, TRUE);
}
static void
_node_set_bezpoints (xmlNodePtr node, BezPoint *points, int num_points)
{
  GString *str = NULL;
  int i;

  str = g_string_new (NULL);
  
  for (i = 0; i < num_points; ++i) {
    BezPoint *bpt = &points[i];
    if (i != 0)
      g_string_append (str, " ");
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
  xmlSetProp(node, (const xmlChar *)"bezpoints", (xmlChar *) str->str);

  g_string_free(str, TRUE);
}

static void
set_linewidth(DiaRenderer *self, real width)
{  
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-linewidth", NULL);
  _node_set_real (node, "width", width);
}

static void
set_linecaps(DiaRenderer *self, LineCaps mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *value = NULL;

  switch(mode) {
  case LINECAPS_BUTT:
    value = "butt";
    break;
  case LINECAPS_ROUND:
    value = "round";
    break;
  case LINECAPS_PROJECTING:
    value = "projecting";
    break;
  /* intentionally no default to let 'good' compilers warn about new constants*/
  }
  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-linecaps", NULL);
  xmlSetProp(node, (const xmlChar *)"mode", 
             value ? (xmlChar *)value : (xmlChar *)"?");
}

static void
set_linejoin(DiaRenderer *self, LineJoin mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *value = NULL;

  switch(mode) {
  case LINEJOIN_MITER:
    value = "miter";
    break;
  case LINEJOIN_ROUND:
    value = "round";
    break;
  case LINEJOIN_BEVEL:
    value = "bevel";
    break;
  /* intentionally no default to let 'good' compilers warn about new constants*/
  }
  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-linejoin", NULL);
  xmlSetProp(node, (const xmlChar *)"mode", 
             value ? (xmlChar *)value : (xmlChar *)"?");
}

static void
set_linestyle(DiaRenderer *self, LineStyle mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const gchar *value = NULL;

  /* line type */
  switch (mode) {
  case LINESTYLE_SOLID:
    value = "solid";
    break;
  case LINESTYLE_DASHED:
    value = "dashed";
    break;
  case LINESTYLE_DASH_DOT:
    value = "dash-dot";
    break;
  case LINESTYLE_DASH_DOT_DOT:
    value = "dash-dot-dot";
    break;
  case LINESTYLE_DOTTED:
    value = "dotted";
    break;
  /* intentionally no default to let 'good' compilers warn about new constants*/
  }
  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-linestyle", NULL);
  xmlSetProp(node, (const xmlChar *)"mode", 
             value ? (xmlChar *)value : (xmlChar *)"?");
}

static void
set_dashlength(DiaRenderer *self, real length)
{  
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  
  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-dashlength", NULL);
  _node_set_real (node, "length", length);
}

static void
set_fillstyle(DiaRenderer *self, FillStyle mode)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const gchar *value = NULL;

  switch(mode) {
  case FILLSTYLE_SOLID:
    value = "solid";
    break;
  /* intentionally no default to let 'good' compilers warn about new constants*/
  }
  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-fillstyle", NULL);
  xmlSetProp(node, (const xmlChar *)"mode", 
             value ? (xmlChar *)value : (xmlChar *)"?");
}

static void
set_font(DiaRenderer *self, DiaFont *font, real height)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  const PangoFontDescription *pfd = dia_font_get_description (font);
  char *desc = pango_font_description_to_string (pfd);

  node =  xmlNewChild(renderer->root, NULL, (const xmlChar *)"set-font", NULL);
  xmlSetProp(node, (const xmlChar *)"description", (xmlChar *)desc);
  
  xmlSetProp(node, (const xmlChar *)"family", (xmlChar *)dia_font_get_family (font));
  xmlSetProp(node, (const xmlChar *)"weight", (xmlChar *)dia_font_get_weight_string (font));
  xmlSetProp(node, (const xmlChar *)"slant", (xmlChar *)dia_font_get_slant_string (font));
  _node_set_real (node, "size", dia_font_get_size (font));
  _node_set_real (node, "height", height);

  g_free(desc);
}

static void
draw_line(DiaRenderer *self, 
          Point *start, Point *end, 
          Color *color)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"draw-line", NULL);
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

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"draw-polyline", NULL);
  _node_set_points (node, points, num_points);
  _node_set_color (node, "stroke", color);
}

static void
_polygon(DiaRenderer *self, 
         Point *points, int num_points, 
         Color *color,
         gboolean fill)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  
  g_return_if_fail(1 < num_points);

  node = xmlNewChild(renderer->root, NULL, 
                     (const xmlChar *)(fill ? "fill-polygon" : "draw-polygon"), NULL);
  _node_set_points (node, points, num_points);
  if (fill)
    _node_set_color (node, "fill", color);
  else
    _node_set_color (node, "stroke", color);
}
static void
draw_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color)
{
  _polygon (self, points, num_points, color, FALSE);
}

static void
fill_polygon(DiaRenderer *self, 
             Point *points, int num_points, 
             Color *color)
{
  _polygon (self, points, num_points, color, TRUE);
}

static void
_rounded_rect(DiaRenderer *self, 
              Point *lefttop, Point *rightbottom,
              Color *color, real *rounding, gboolean fill)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;

  if (rounding)
    node = xmlNewChild(renderer->root, NULL, 
                       (const xmlChar *)(fill ? "fill-rounded-rect" : "draw-rounded-rect"), NULL);
  else
    node = xmlNewChild(renderer->root, NULL, 
                       (const xmlChar *)(fill ? "fill-rect" : "draw-rect"), NULL);

  _node_set_point (node, "lefttop", lefttop);
  _node_set_point (node, "rightbottom", rightbottom);
  if (rounding)
    _node_set_real (node, "r", *rounding);
  if (fill)
    _node_set_color (node, "fill", color);
  else
    _node_set_color (node, "stroke", color);
}
static void
draw_rect(DiaRenderer *self, 
          Point *lefttop, Point *rightbottom,
          Color *color)
{
  _rounded_rect(self, lefttop, rightbottom, color, NULL, FALSE);
}
static void
fill_rect(DiaRenderer *self, 
          Point *lefttop, Point *rightbottom,
          Color *color)
{
  _rounded_rect(self, lefttop, rightbottom, color, NULL, TRUE);
}
static void
draw_rounded_rect(DiaRenderer *self, 
                  Point *lefttop, Point *rightbottom,
                  Color *color, real rounding)
{
  _rounded_rect(self, lefttop, rightbottom, color, &rounding, FALSE);
}
static void
fill_rounded_rect(DiaRenderer *self, 
                  Point *lefttop, Point *rightbottom,
                  Color *color, real rounding)
{
  _rounded_rect(self, lefttop, rightbottom, color, &rounding, FALSE);
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
  
  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)(fill ? "fill-arc" : "draw-arc"), NULL);
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
_ellipse(DiaRenderer *self, 
         Point *center,
         real width, real height,
         Color *color,
         gboolean fill)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  
  node = xmlNewChild(renderer->root, NULL, 
                     (const xmlChar *)(fill ? "fill-ellipse" : "draw-ellipse"), NULL);
  _node_set_point (node, "center", center);
  _node_set_real (node, "width", width);
  _node_set_real (node, "height", height);
  if (fill)
    _node_set_color (node, "fill", color);
  else
    _node_set_color (node, "stroke", color);
}
static void
draw_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color)
{
  _ellipse (self, center, width, height, color, FALSE);
}
static void
fill_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *color)
{
  _ellipse (self, center, width, height, color, TRUE);
}

static void
_bezier(DiaRenderer *self, 
        BezPoint *points,
        int numpoints,
        Color *color,
        gboolean fill)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  
  node = xmlNewChild (renderer->root, NULL, 
                      (const xmlChar *)(fill ? "fill-bezier" : "draw-bezier"), NULL);
  _node_set_bezpoints (node, points, numpoints);
  if (fill)
    _node_set_color (node, "fill", color);
  else
    _node_set_color (node, "stroke", color);
}
static void
draw_bezier(DiaRenderer *self, 
            BezPoint *points,
            int numpoints,
            Color *color)
{
  _bezier (self, points, numpoints, color, FALSE);
}
static void
fill_bezier(DiaRenderer *self, 
            BezPoint *points,
            int numpoints,
            Color *color)
{
  _bezier (self, points, numpoints, color, TRUE);
}

static void 
draw_rounded_polyline (DiaRenderer *self,
                       Point *points, int num_points,
                       Color *color, real radius )
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  
  node = xmlNewChild (renderer->root, NULL, (const xmlChar *)"draw-rounded-polyline", NULL);
  _node_set_points (node, points, num_points);
  _node_set_color (node, "stroke", color);
  _node_set_real (node, "r", radius);
}

static void
draw_string(DiaRenderer *self,
            const char *text,
            Point *pos, Alignment alignment,
            Color *color)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *align = NULL;
  
  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"draw-string", NULL);
  _node_set_point (node, "pos", pos);
  _node_set_color (node, "fill", color);
  switch (alignment) {
  case ALIGN_LEFT :
    align = "left";
    break;
  case ALIGN_RIGHT :
    align = "right";
    break;
  case ALIGN_CENTER :
    align = "center";
    break;
  /* intentionally no default */
  }
  if (align)
    xmlSetProp(node, (const xmlChar *)"alignment", (xmlChar *) align);

  xmlNodeSetContent (node, (const xmlChar *)text);
}

static void
draw_image(DiaRenderer *self,
           Point *point,
           real width, real height,
           DiaImage image)
{
  DrsRenderer *renderer = DRS_RENDERER (self);
  xmlNodePtr node;
  gchar *uri;

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"draw-image", NULL);
  _node_set_point (node, "point", point);
  _node_set_real (node, "width", width);
  _node_set_real (node, "height", height);
  /* TODO: also save the data? */
  uri = g_filename_to_uri (dia_image_filename(image), NULL, NULL);
  if (uri)
    xmlSetProp (node, (const xmlChar *)"uri", (xmlChar *) uri);
  g_free (uri);
}

static void
drs_renderer_class_init (DrsRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = drs_renderer_finalize;

  /* renderer members */
  renderer_class->begin_render = begin_render;
  renderer_class->end_render   = end_render;

  renderer_class->draw_object = draw_object;

  renderer_class->set_linewidth  = set_linewidth;
  renderer_class->set_linecaps   = set_linecaps;
  renderer_class->set_linejoin   = set_linejoin;
  renderer_class->set_linestyle  = set_linestyle;
  renderer_class->set_dashlength = set_dashlength;
  renderer_class->set_fillstyle  = set_fillstyle;

  renderer_class->set_font  = set_font;

  renderer_class->draw_line    = draw_line;
  renderer_class->fill_polygon = fill_polygon;
  renderer_class->draw_rect    = draw_rect;
  renderer_class->fill_rect    = fill_rect;
  renderer_class->draw_arc     = draw_arc;
  renderer_class->fill_arc     = fill_arc;
  renderer_class->draw_ellipse = draw_ellipse;
  renderer_class->fill_ellipse = fill_ellipse;

  renderer_class->draw_string  = draw_string;
  renderer_class->draw_image   = draw_image;

  /* medium level functions */
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_polyline  = draw_polyline;
  renderer_class->draw_polygon   = draw_polygon;

  renderer_class->draw_bezier   = draw_bezier;
  renderer_class->fill_bezier   = fill_bezier;

  renderer_class->draw_rounded_polyline = draw_rounded_polyline;
  /* highest level functions */
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->fill_rounded_rect = fill_rounded_rect;
#if 0
  renderer_class->draw_text  = draw_text;
  renderer_class->draw_text_line  = draw_text_line;
  /* TODO: more to come ... */
#endif
}
