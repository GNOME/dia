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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

/* the dots per centimetre to render this diagram at */
/* this matches the setting `100%' setting in dia. */
#define DPCM 20

#define CONNECTION_POINT_SHAPE "Shape Design - Connection Point"
#define MAIN_CONNECTION_POINT_SHAPE "Shape Design - Main Connection Point"

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h> /* xmlStrdup */
#include "dia_xml.h"
#include "geometry.h"
#include "diasvgrenderer.h"
#include "filter.h"
#include "diagramdata.h"
#include "object.h"
#include "dia-io.h"


G_BEGIN_DECLS

#define SHAPE_TYPE_RENDERER           (shape_renderer_get_type ())
#define SHAPE_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), SHAPE_TYPE_RENDERER, ShapeRenderer))
#define SHAPE_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), SHAPE_TYPE_RENDERER, ShapeRendererClass))
#define SHAPE_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SHAPE_TYPE_RENDERER))
#define SHAPE_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SHAPE_TYPE_RENDERER, ShapeRendererClass))

typedef struct _ShapeRenderer ShapeRenderer;
typedef struct _ShapeRendererClass ShapeRendererClass;

/*!
 * \brief Shape export for use as new _Custom object
 *
 * Render to the SVG Shape dialect including connection points and icon information.
 * The SVG output is unscaled - that is with untransformed diagram coordinates.
 *
 * \sa Shapes
 *
 * \extends _DiaSvgRenderer
 */
struct _ShapeRenderer
{
  DiaSvgRenderer parent_instance;

  xmlNodePtr connection_root;
  /* True, if Shape_Design connection points are used */
  gboolean design_connection;
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
static void
draw_object(DiaRenderer *self,
            DiaObject   *object,
	    DiaMatrix   *matrix);
static void draw_polyline(DiaRenderer *self,
			  Point *points, int num_points,
			  Color *line_colour);
static void draw_polygon(DiaRenderer *self,
			 Point *points, int num_points,
			 Color *fill, Color *stroke);
static void draw_rect(DiaRenderer *self,
		      Point *ul_corner, Point *lr_corner,
		      Color *fill, Color *stroke);
static void draw_rounded_rect (DiaRenderer *self,
			       Point *ul_corner, Point *lr_corner,
			       Color *fill, Color *stroke, real rounding);
static void draw_ellipse(DiaRenderer *self,
			 Point *center,
			 real width, real height,
			 Color *fill, Color *stroke);

/* helper functions */
static void add_connection_point(ShapeRenderer *renderer,
                                 Point *point, gboolean design_connection,
                                 gboolean main_point);
static void add_rectangle_connection_points(ShapeRenderer *renderer,
                                            Point *ul_corner, Point *lr_corner, real r);
static void add_ellipse_connection_points(ShapeRenderer *renderer,
                                          Point *center,
                                          real width, real height);

/* Moved to reduce confusion of Doxygen */
GType shape_renderer_get_type (void) G_GNUC_CONST;

/*!
 * \brief Create a shape renderer and initialize it from the diagram
 *
 * \relates _DiaSvgRenderer
 */
static DiaSvgRenderer *
new_shape_renderer(DiagramData *data, const char *filename)
{
  ShapeRenderer *shape_renderer;
  DiaSvgRenderer *renderer;
  char *point;
  xmlNodePtr xml_node_ptr;
  gint i;
  gchar *png_filename;
  char *shapename, *dirname, *fullname;
  char *sheetname;
  char *basename;

  shape_renderer = g_object_new(SHAPE_TYPE_RENDERER, NULL);
  renderer = DIA_SVG_RENDERER (shape_renderer);

  renderer->filename = g_strdup(filename);

  /* keep everything unscaled, i.e. in Dia's scale default */
  renderer->scale = 1.0;

  /* set up the root node */
  renderer->doc = xmlNewDoc((const xmlChar *)"1.0");
  renderer->doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  renderer->root = xmlNewDocNode(renderer->doc, NULL, (const xmlChar *)"shape", NULL);
  xmlNewNs(renderer->root,
                        (const xmlChar *)"http://www.daa.com.au/~james/dia-shape-ns", NULL);

  renderer->svg_name_space = xmlNewNs(renderer->root,
                                      (const xmlChar *)"http://www.w3.org/2000/svg", (const xmlChar *)"svg");
  xmlNewNs(renderer->root, (const xmlChar *)"http://www.w3.org/1999/xlink", (const xmlChar *)"xlink");
  renderer->doc->xmlRootNode = renderer->root;

  dirname = g_path_get_dirname(filename);
  sheetname = g_path_get_basename(dirname);
  basename = g_path_get_basename(filename);
  shapename = g_strndup(basename, strlen(basename)-6);
  g_clear_pointer (&basename, g_free);
  fullname = g_strdup_printf ("%s - %s", sheetname, shapename);
  g_clear_pointer (&dirname, g_free);
  g_clear_pointer (&sheetname, g_free);
  g_clear_pointer (&shapename, g_free);

  xmlNewChild(renderer->root, NULL, (const xmlChar *)"name", (xmlChar *) fullname);
  g_clear_pointer (&fullname, g_free);
  point = strrchr(filename, '.');
  i = (int)(point-filename);
  point = g_strndup(filename, i);
  png_filename = g_strdup_printf("%s.png",point);
  g_clear_pointer (&point, g_free);
  basename = g_path_get_basename(png_filename);
  xmlNewChild(renderer->root, NULL, (const xmlChar *)"icon", (xmlChar *) basename);
  g_clear_pointer (&basename, g_free);
  g_clear_pointer (&png_filename, g_free);
  shape_renderer->connection_root = xmlNewChild(renderer->root, NULL, (const xmlChar *)"connections", NULL);
  shape_renderer->design_connection = FALSE;
  xml_node_ptr = xmlNewChild(renderer->root, NULL, (const xmlChar *)"aspectratio",NULL);
  xmlSetProp(xml_node_ptr, (const xmlChar *)"type", (const xmlChar *)"fixed");
  renderer->root = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"svg", NULL);

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
  renderer_class->draw_object = draw_object;
  renderer_class->draw_polyline = draw_polyline;
  renderer_class->draw_polygon = draw_polygon;
  renderer_class->draw_rect = draw_rect;
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->draw_ellipse = draw_ellipse;
}

/* member implementations */
/*!
 * \brief Render an object
 *
 * Most objects rendering is delegated to the base class. Only special
 * objects representing connection points are not rendered, but instead
 * translated into shape connection points.
 *
 * \memberof _ShapeRenderer
 */
static void
draw_object(DiaRenderer *self,
            DiaObject   *object,
	    DiaMatrix   *matrix)
{
  Point center;
  ShapeRenderer *renderer = SHAPE_RENDERER(self);
  gboolean main_point = (0 == strncmp(MAIN_CONNECTION_POINT_SHAPE,
  	               object->type->name, strlen(MAIN_CONNECTION_POINT_SHAPE)));

  if ((0 == strncmp(CONNECTION_POINT_SHAPE, object->type->name,
  	 strlen(CONNECTION_POINT_SHAPE))) || main_point) {
	  /* add user defined connection point */
	  center.x = (object->bounding_box.left + object->bounding_box.right)/2;
	  center.y = (object->bounding_box.top + object->bounding_box.bottom)/2;
	  add_connection_point(renderer, &center, TRUE, main_point);
  } else {
  	/* use base class implementation */
    DIA_RENDERER_CLASS(parent_class)->draw_object (self, object, matrix);
  }
}


static void
end_render (DiaRenderer *self)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  DiaContext *ctx = dia_context_new (_("Shape Export"));

  g_clear_pointer (&renderer->linestyle, g_free);
  renderer->linestyle = NULL;

  dia_context_set_filename (ctx, renderer->filename);
  dia_io_save_document (renderer->filename, renderer->doc, FALSE, ctx);

  g_clear_pointer (&renderer->filename, g_free);
  g_clear_pointer (&renderer->doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);
}


/*!
 * \brief Add a connection point to the shape
 * \protected \memberof _ShapeRenderer
 */
static void
add_connection_point (ShapeRenderer *renderer,
                      Point *point, gboolean design_connection,
                      gboolean main)
{
  xmlNodePtr node;
  xmlChar *propx, *propy;
  gchar bufx[G_ASCII_DTOSTR_BUF_SIZE], bufy[G_ASCII_DTOSTR_BUF_SIZE];

  /* Use connection points, drop the auto ones */
  if(design_connection && !renderer->design_connection) {
	  renderer->design_connection = design_connection;
	  node = renderer->connection_root->parent;
	  xmlUnlinkNode(renderer->connection_root);
	  xmlFree(renderer->connection_root);
	  renderer->connection_root = xmlNewChild(node, NULL, (const xmlChar *)"connections", NULL);
  }
  if(!design_connection && renderer->design_connection)
  	return;

  g_ascii_formatd(bufx, sizeof(bufx), "%g", point->x);
  g_ascii_formatd(bufy, sizeof(bufy), "%g", point->y);

  /* Remove duplicates */
  for (node = renderer->connection_root->children; node; node = node->next) {
    if ((XML_ELEMENT_NODE == node->type) && xmlStrEqual((const xmlChar *)"point", node->name)) {
      propx = xmlGetProp(node, (const xmlChar *)"x");
      propy = xmlGetProp(node, (const xmlChar *)"y");
      if (xmlStrEqual((const xmlChar *)propx, (const xmlChar *)bufx) &&
          xmlStrEqual((const xmlChar *)propy, (const xmlChar *)bufy)) {
	if(propx) xmlFree(propx);
        if(propy) xmlFree(propy);
        return;
      }
      if(propx) xmlFree(propx);
      if(propy) xmlFree(propy);
    }
  }

  node = xmlNewChild(renderer->connection_root, NULL, (const xmlChar *)"point", NULL);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) bufx);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) bufy);
  if (main) {
	  xmlSetProp(node, (const xmlChar *)"main", (xmlChar *)"yes");
  }

}

/*!
 * \brief Draw a line and add three connection points
 * \memberof _ShapeRenderer
 */
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
  add_connection_point(renderer, start, FALSE, FALSE);
  add_connection_point(renderer, end, FALSE, FALSE);
  center.x = (start->x + end->x)/2;
  center.y = (start->y + end->y)/2;
  add_connection_point(renderer, &center, FALSE, FALSE);

}

/*!
 * \brief Draw a polyline and add connection points
 *
 * \note Complete overwrite, code duplication with base class
 * \memberof _ShapeRenderer
 */
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
  gchar px_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar py_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polyline", NULL);

  xmlSetProp(node, (const xmlChar *)"style",
             (xmlChar *) DIA_SVG_RENDERER_GET_CLASS(renderer)->get_draw_style(renderer, NULL, line_colour));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_append_printf(str, "%s,%s ",
			   g_ascii_formatd(px_buf, sizeof(px_buf), "%g", points[i].x),
			   g_ascii_formatd(py_buf, sizeof(py_buf), "%g", points[i].y) );
    add_connection_point(SHAPE_RENDERER(self), &points[i], FALSE, FALSE);
  }
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(SHAPE_RENDERER(renderer), &center, FALSE, FALSE);
  }
}


/*!
 * \brief Add a polygon including it's connection points
 *
 * \note complete overwrite, necessary code duplication?
 * \memberof _ShapeRenderer
 */
static void
draw_polygon(DiaRenderer *self,
	      Point *points, int num_points,
	      Color *fill, Color *stroke)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  int i;
  xmlNodePtr node;
  GString *str;
  Point center;
  gchar px_buf[G_ASCII_DTOSTR_BUF_SIZE];
  gchar py_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"polygon", NULL);

  xmlSetProp(node, (const xmlChar *)"style",
            (xmlChar *) DIA_SVG_RENDERER_GET_CLASS(renderer)->get_draw_style(renderer, fill, stroke));

  str = g_string_new(NULL);
  for (i = 0; i < num_points; i++) {
    g_string_append_printf(str, "%s,%s ",
			   g_ascii_formatd(px_buf, sizeof(px_buf), "%g", points[i].x),
			   g_ascii_formatd(py_buf, sizeof(py_buf), "%g", points[i].y) );
    add_connection_point(SHAPE_RENDERER(self), &points[i], FALSE, FALSE);
  }
  for(i = 1; i < num_points; i++) {
    center.x = (points[i].x + points[i-1].x)/2;
    center.y = (points[i].y + points[i-1].y)/2;
    add_connection_point(SHAPE_RENDERER(self), &center, FALSE, FALSE);
  }
  xmlSetProp(node, (const xmlChar *)"points", (xmlChar *) str->str);
  g_string_free(str, TRUE);
}

/*!
 * \brief Add nine connection points for a rectangle
 *
 * \protected \memberof _ShapeRenderer
 */
static void
add_rectangle_connection_points (ShapeRenderer *renderer,
                                 Point *ul_corner, Point *lr_corner, real r)
{
  Point pos;
  real width, height;

  width = lr_corner->x - ul_corner->x;
  height = lr_corner->y - ul_corner->y;
  r = MIN(width/2, r);
  r = MIN(height/2, r);
  r *= (1-M_SQRT1_2);

  /* connection points in the same order as the handles are */
  pos.x = ul_corner->x + r; /* NW */
  pos.y = ul_corner->y + r;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + width/2; /* N */
  pos.y = ul_corner->y;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + width - r; /* NE */
  pos.y = ul_corner->y + r;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + width; /* E */
  pos.y = ul_corner->y + height/2;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + width - r; /* SE */
  pos.y = ul_corner->y + height - r;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + width/2; /* S */
  pos.y = ul_corner->y + height;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x + r; /* SW */
  pos.y = ul_corner->y + height - r;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = ul_corner->x; /* W */
  pos.y = ul_corner->y + height/2;
  add_connection_point(renderer, &pos, FALSE, FALSE);
  pos.x = (ul_corner->x + lr_corner->x)/2; /* center */
  pos.y = (ul_corner->y + lr_corner->y)/2;
  add_connection_point(renderer, &pos, FALSE, FALSE);
}

/*!
 * \brief Draw a rectangle, if stroked with connection points
 *
 * \memberof _ShapeRenderer
 */
static void
draw_rect (DiaRenderer *self,
           Point *ul_corner, Point *lr_corner,
           Color *fill, Color *stroke)
{
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_rect (self, ul_corner, lr_corner, fill, stroke);
  /* do our own stuff */
  if (stroke)
    add_rectangle_connection_points(renderer, ul_corner, lr_corner, 0.0);
}

/*!
 * \brief Draw a rounded rectangle, if stroked with connection points
 *
 * \memberof _ShapeRenderer
 */
static void
draw_rounded_rect (DiaRenderer *self,
		   Point *ul_corner, Point *lr_corner,
		   Color *fill, Color *stroke, real rounding)
{
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_rounded_rect (self, ul_corner, lr_corner, fill, stroke, rounding);
  /* do our own stuff */
  if (stroke)
    add_rectangle_connection_points(renderer, ul_corner, lr_corner, rounding);
}

/*!
 * \brief Add connection points for an ellipse
 *
 * \protected \memberof _ShapeRenderer
 */
static void
add_ellipse_connection_points (ShapeRenderer *renderer,
                               Point *center,
                               real width, real height)
{
  Point connection;

  connection.x = center->x;
  connection.y = center->y + height/2;
  add_connection_point(renderer, &connection, FALSE, FALSE);
  connection.y = center->y - height/2;
  add_connection_point(renderer, &connection, FALSE, FALSE);

  connection.y = center->y;
  connection.x = center->x - width/2;
  add_connection_point(renderer, &connection, FALSE, FALSE);
  connection.x = center->x + width/2;
  add_connection_point(renderer, &connection, FALSE, FALSE);
}

/*!
 * \brief Draw an ellipse, if stroked with connection points
 *
 * \memberof _ShapeRenderer
 */
static void
draw_ellipse(DiaRenderer *self,
             Point *center,
             real width, real height,
             Color *fill, Color *stroke)
{
  ShapeRenderer *renderer = SHAPE_RENDERER(self);

  /* use base class implementation */
  DIA_RENDERER_CLASS(parent_class)->draw_ellipse (self, center, width, height, fill, stroke);

  /* do our own stuff */
  if (stroke)
    add_ellipse_connection_points(renderer, center, width, height);
}

static gboolean
export_shape(DiagramData *data, DiaContext *ctx,
	     const gchar *filename, const gchar *diafilename,
	     void* user_data)
{
    DiaSvgRenderer *renderer;
    int i;
    gchar *point;
    gchar *png_filename = NULL;
    DiaExportFilter *exportfilter;
    gfloat old_scaling;
    DiaRectangle *ext = &data->extents;
    gfloat scaling_x, scaling_y;

    /* create the png preview shown in the toolbox */
    point = strrchr(filename, '.');
    if (point == NULL || strcmp(point, ".shape") != 0) {
	dia_context_add_message(ctx, _("Shape files must end in .shape, or they cannot be loaded by Dia"));
	return FALSE;
    }
    i = (int)(point-filename);
    point = g_strndup(filename, i);
    png_filename = g_strdup_printf("%s.png",point);
    g_clear_pointer (&point, g_free);
    exportfilter = filter_guess_export_filter(png_filename);

    if (!exportfilter) {
      dia_context_add_message (ctx, _("Can't export PNG icon without export plugin!"));
    } else {
      /* get the scaling right */
      old_scaling = data->paper.scaling;
      scaling_x = 22 / ((ext->right - ext->left) * 20);
      scaling_y = 22 / ((ext->bottom - ext->top) * 20);
      data->paper.scaling = MIN (scaling_x, scaling_y);
      exportfilter->export_func (data,
                                 ctx,
                                 png_filename,
                                 diafilename,
                                 exportfilter->user_data);
      data->paper.scaling = old_scaling;
    }
    g_clear_pointer (&png_filename, g_free);

    /* create the shape */
    if((renderer = new_shape_renderer (data, filename))) {
      data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

      g_clear_object (&renderer);

      return TRUE;
    }

    /* Create a sheet entry if applicable (../../sheets exists) */

    return FALSE;
}


static const gchar *extensions[] = { "shape", NULL };
DiaExportFilter shape_export_filter = {
    N_("Dia Shape File"),
    extensions,
    export_shape
};
