/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * shape-export.c: shape export filter for dia
 * Copyright (C) 2000 Steffen Macke
 *
 * Major refactoring while porting to use DiaSvgRenderer 
 * Copyright (C) 2002 Hans Breuer
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
/*
 * TODO:
 *   - While porting to use DiaSvgRenderer I removved all connection point
       adding to fill_* methods with the assumption that they always have
       a corresponding draw_* method call where the connection points are 
       already added. Correct me if I'm wrong ...                    --hb 
 */
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <locale.h>

/* the dots per centimetre to render this diagram at */
/* this matches the setting `100%' setting in dia. */
#define DPCM 20

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h> /* xmlStrdup */
#include "dia_xml_libxml.h"
#include "geometry.h"
#include "diasvgrenderer.h"
#include "filter.h"
#include "intl.h"
#include "message.h"
#include "diagramdata.h"

G_BEGIN_DECLS

#define SHAPE_TYPE_RENDERER           (shape_renderer_get_type ())
#define SHAPE_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHAPE_TYPE_RENDERER, ShapeRenderer))
#define SHAPE_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), SHAPE_TYPE_RENDERER, ShapeRendererClass))
#define SHAPE_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHAPE_TYPE_RENDERER))
#define SHAPE_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SHAPE_TYPE_RENDERER, ShapeRendererClass))

GType shape_renderer_get_type (void) G_GNUC_CONST;

typedef struct _ShapeRenderer ShapeRenderer;
typedef struct _ShapeRendererClass ShapeRendererClass;

struct _ShapeRenderer
{
  DiaSvgRenderer parent_instance;

  xmlNodePtr connection_root;
};

struct _ShapeRendererClass
{
  DiaSvgRendererClass parent_class;
};

G_END_DECLS

static DiaSvgRenderer *new_shape_renderer(DiagramData *data, const char *filename);

/* DiaSvgRenderer members */
static void end_render(DiaRenderer *self);

static void draw_line(DiaRenderer *self, 
		      Point *start, Point *end, 
		      Color *line_colour);
static void draw_polyline(DiaRenderer *self, 
			  Point *points, int num_points, 
			  Color *line_colour);
static void draw_polygon(DiaRenderer *self, 
			 Point *points, int num_points, 
			 Color *line_colour);
static void draw_rect(DiaRenderer *self, 
		      Point *ul_corner, Point *lr_corner,
		      Color *colour);
static void draw_ellipse(DiaRenderer *self, 
			 Point *center,
			 real width, real height,
			 Color *colour);

/* helper functions */
static void add_connection_point(ShapeRenderer *renderer, 
                                 Point *point);
static void add_rectangle_connection_points(ShapeRenderer *renderer,
                                            Point *ul_corner, Point *lr_corner);
static void add_ellipse_connection_points(ShapeRenderer *renderer,
                                          Point *center,
                                          real width, real height);      

static DiaSvgRenderer *
new_shape_renderer(DiagramData *data, const char *filename)
{
  ShapeRenderer *shape_renderer;
  DiaSvgRenderer *renderer;
  FILE *file;
  char *point;
  xmlNsPtr name_space;
  xmlNodePtr xml_node_ptr;
  gint i;
  gchar *png_filename;
  char *shapename, *dirname, *fullname;
  const char *sheetname;

  file = fopen(filename, "w");

  if (file==NULL) {
      message_error(_("Can't open output file %s: %s\n"), filename, strerror(errno));
    return NULL;
  }
  fclose(file);

  shape_renderer = g_object_new(SHAPE_TYPE_RENDERER, NULL);
  renderer = DIA_SVG_RENDERER (shape_renderer);

  renderer->filename = g_strdup(filename);

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;

  /* set up the root node */
  renderer->doc = xmlNewDoc("1.0");
  renderer->doc->encoding = xmlStrdup("UTF-8");
  renderer->root = xmlNewDocNode(renderer->doc, NULL, "shape", NULL);
  name_space = xmlNewNs(renderer->root,
                        "http://www.daa.com.au/~james/dia-shape-ns", NULL);

  renderer->svg_name_space = xmlNewNs(renderer->root,
                                      "http://www.w3.org/2000/svg", "svg");
  renderer->doc->xmlRootNode = renderer->root;

  dirname = g_dirname(filename);
  sheetname = g_basename(dirname);
  shapename = g_strndup(g_basename(filename), strlen(g_basename(filename))-6);
  fullname = g_malloc(strlen(sheetname)+3+strlen(shapename)+1);
  sprintf(fullname, "%s - %s", sheetname, shapename);
  g_free(dirname);
  g_free(shapename);

  xmlNewChild(renderer->root, NULL, "name", fullname);
  g_free(fullname);
  point = strrchr(filename, '.');
  i = (int)(point-filename);
  point = g_strndup(filename, i);
  png_filename = g_strdup_printf("%s.png",point);
  g_free(point);
  xmlNewChild(renderer->root, NULL, "icon", g_basename(png_filename));
  g_free(png_filename);
  shape_renderer->connection_root = xmlNewChild(renderer->root, NULL, "connections", NULL);
  xml_node_ptr = xmlNewChild(renderer->root, NULL, "aspectratio",NULL);
  xmlSetProp(xml_node_ptr, "type","fixed");
  renderer->root = xmlNewChild(renderer->root, renderer->svg_name_space, "svg", NULL);
    
  return renderer;
}

/* GObject stuff */
static void shape_renderer_class_init (ShapeRendererClass *klass);

static gpointer parent_class = NULL;

GType
shape_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (ShapeRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) shape_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (ShapeRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_SVG_RENDERER,
                                            "ShapeRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
shape_renderer_finalize (GObject *object)
{
  ShapeRenderer *shape_renderer = SHAPE_RENDERER (object);

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
shape_renderer_class_init (ShapeRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = shape_renderer_finalize;

  /* dia svg renderer overwrites */
  renderer_class->end_render = end_render;
  renderer_class->draw_line = draw_line;
  renderer_class->draw_polyline = draw_polyline;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_ellipse = draw_ellipse;
}

/* member implementations */
/* full overwrite */
static void
end_render(DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int old_blanks_default = pretty_formated_xml;

  /* FIXME HACK: we always want nice readable shape files,
   *  but toggling it by a global var is ugly   --hb 
   */
  pretty_formated_xml = TRUE;

  g_free(renderer->linestyle);
  renderer->linestyle = NULL;
  
  xmlSetDocCompressMode(renderer->doc, 0);
  xmlDiaSaveFile(renderer->filename, renderer->doc);
  g_free(renderer->filename);
  renderer->filename = NULL;
  xmlFreeDoc(renderer->doc);
  pretty_formated_xml = old_blanks_default;
}

static void
add_connection_point (ShapeRenderer *renderer, 
                      Point *point) 
{
  xmlNodePtr node;
  char buf[512];
  
  node = xmlNewChild(renderer->connection_root, NULL, "point", NULL);
  g_snprintf(buf, sizeof(buf), "%g", point->x);
  xmlSetProp(node, "x", buf);
  g_snprintf(buf, sizeof(buf), "%g", point->y);
  xmlSetProp(node, "y", buf);
}

static void
draw_line(DiaRenderer *self, 
	  Point *start, Point *end, 
	  Color *line_colour)
{
  Point center;
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_line (self, start, end, line_colour);

  /* do our own stuff */
  add_connection_point(renderer, start);
  add_connection_point(renderer, end);
  center.x = (start->x + end->x)/2;
  center.y = (start->y + end->y)/2;
  add_connection_point(renderer, &center);

}

/* complete overwrite, code duplication with base class */
static void
draw_polyline(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polyline", NULL);
  
  xmlSetProp(node, "style", 
             DIA_SVG_RENDERER_GET_CLASS(renderer)->get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
    add_connection_point(SHAPE_RENDERER(self), &points[i]);
  }
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(SHAPE_RENDERER(renderer), &center);
  }
}


/* complete overwrite, necessary code duplication */
static void
draw_polygon(DiaRenderer *self, 
	      Point *points, int num_points, 
	      Color *line_colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;

  node = xmlNewChild(renderer->root, renderer->svg_name_space, "polygon", NULL);
  
  xmlSetProp(node, "style", 
             DIA_SVG_RENDERER_GET_CLASS(renderer)->get_draw_style(renderer, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_sprintfa(str, "%g,%g ", points[i].x, points[i].y);
    add_connection_point(SHAPE_RENDERER(self), &points[i]);
  }
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(SHAPE_RENDERER(self), &center);
  }
  xmlSetProp(node, "points", str->str);
  g_string_free(str, TRUE);
}

static void
add_rectangle_connection_points (ShapeRenderer *renderer,
                                 Point *ul_corner, Point *lr_corner) 
{
  Point connection;
  coord center_x, center_y;
 
  center_x = (ul_corner->x + lr_corner->x)/2;
  center_y = (ul_corner->y + lr_corner->y)/2;
    
  add_connection_point(renderer, ul_corner);
  add_connection_point(renderer, lr_corner);
  connection.x = ul_corner->x;
  connection.y = lr_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = center_y;
  add_connection_point(renderer, &connection);
  
  connection.x = lr_corner->x;
  connection.y = ul_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = center_y;
  add_connection_point(renderer, &connection);
  
  connection.x = center_x;
  connection.y = lr_corner->y;
  add_connection_point(renderer, &connection);
  connection.y = ul_corner->y;
  add_connection_point(renderer, &connection);
}   
    

static void
draw_rect (DiaRenderer *self, 
           Point *ul_corner, Point *lr_corner,
           Color *colour) 
{
  Point center;
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_rect (self, ul_corner, lr_corner, colour);
  /* do our own stuff */
  add_rectangle_connection_points(renderer, ul_corner, lr_corner);
}

static void 
add_ellipse_connection_points (ShapeRenderer *renderer,
                               Point *center, 
                               real width, real height) 
{
  Point connection;
  
  connection.x = center->x;
  connection.y = center->y + height/2;
  add_connection_point(renderer, &connection);
  connection.y = center->y - height/2;
  add_connection_point(renderer, &connection);
  
  connection.y = center->y;
  connection.x = center->x - width/2;
  add_connection_point(renderer, &connection);
  connection.x = center->x + width/2;
  add_connection_point(renderer, &connection);
}

static void
draw_ellipse(DiaRenderer *self, 
             Point *center,
             real width, real height,
             Color *colour)
{
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_ellipse (self, center, width, height, colour);

  /* do our own stuff */
  add_ellipse_connection_points(renderer, center, width, height);
}

static void
export_shape(DiagramData *data, const gchar *filename, 
             const gchar *diafilename, void* user_data)
{
    DiaSvgRenderer *renderer;
    int i;
    gchar *point;
    gchar *png_filename = NULL;
    DiaExportFilter *exportfilter;
    char *old_locale;
    gfloat old_scaling;
    Rectangle *ext = &data->extents;
    gfloat scaling_x, scaling_y;

    /* create the png preview shown in the toolbox */
    point = strrchr(filename, '.');
    i = (int)(point-filename);
    point = g_strndup(filename, i);
    png_filename = g_strdup_printf("%s.png",point);
    g_free(point);
    exportfilter = filter_guess_export_filter(png_filename);

    if (!exportfilter) {
      message_warning(_("Can't export png without libart!"));
    }
    else {
      /* get the scaling right */
      old_scaling = data->paper.scaling;
      scaling_x = 22/((ext->right - ext->left) * 20);
      scaling_y = 22/((ext->bottom - ext->top) * 20);
      data->paper.scaling = MIN(scaling_x, scaling_y);
      exportfilter->export_func(data, png_filename, diafilename, user_data);
      data->paper.scaling = old_scaling;
    }
    /* create the shape */
    old_locale = setlocale(LC_NUMERIC, "C");
    if((renderer = new_shape_renderer(data, filename))) {
      data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
      g_object_unref (renderer);
    }

    /* Create a sheet entry if applicable (../../sheets exists) */
    

    setlocale(LC_NUMERIC, old_locale);
    g_free(png_filename);
}

static const gchar *extensions[] = { "shape", NULL };
DiaExportFilter shape_export_filter = {
    N_("Dia Shape File"),
    extensions,
    export_shape
};
