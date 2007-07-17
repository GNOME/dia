/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_svg.c - an SVG renderer for dia, based on render_eps.c
 * Copyright (C) 1999, 2000 James Henstridge
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
#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <glib.h>
#include <glib/gstdio.h>

#include <libxml/entities.h>
#include <libxml/tree.h>
#include <libxml/xmlmemory.h>

#include "geometry.h"
#include "diasvgrenderer.h"
#include "filter.h"
#include "intl.h"
#include "message.h"
#include "diagramdata.h"
#include "dia_xml_libxml.h"
#include "object.h"

G_BEGIN_DECLS

#define SVG_TYPE_RENDERER           (svg_renderer_get_type ())
#define SVG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVG_TYPE_RENDERER, SvgRenderer))
#define SVG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), SVG_TYPE_RENDERER, SvgRendererClass))
#define SVG_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVG_TYPE_RENDERER))
#define SVG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SVG_TYPE_RENDERER, SvgRendererClass))

GType svg_renderer_get_type (void) G_GNUC_CONST;

typedef struct _SvgRenderer SvgRenderer;
typedef struct _SvgRendererClass SvgRendererClass;

struct _SvgRenderer
{
  DiaSvgRenderer parent_instance;
};

struct _SvgRendererClass
{
  DiaSvgRendererClass parent_class;
};

G_END_DECLS

static DiaSvgRenderer *new_svg_renderer(DiagramData *data, const char *filename);

static void draw_object       (DiaRenderer *renderer,
                               DiaObject *object);
static void draw_rounded_rect (DiaRenderer *renderer, 
                               Point *ul_corner, Point *lr_corner,
                               Color *colour, real rounding);
static void fill_rounded_rect (DiaRenderer *renderer, 
                               Point *ul_corner, Point *lr_corner,
                               Color *colour, real rounding);

static void svg_renderer_class_init (SvgRendererClass *klass);

static gpointer parent_class = NULL;

GType
svg_renderer_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
        sizeof (SvgRendererClass),
        (GBaseInitFunc) NULL,
        (GBaseFinalizeFunc) NULL,
        (GClassInitFunc) svg_renderer_class_init,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        sizeof (SvgRenderer),
        0,              /* n_preallocs */
	NULL            /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_SVG_RENDERER,
                                            "SvgRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
svg_renderer_finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
svg_renderer_class_init (SvgRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = svg_renderer_finalize;

  renderer_class->draw_object = draw_object;
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->fill_rounded_rect = fill_rounded_rect;
}


static DiaSvgRenderer *
new_svg_renderer(DiagramData *data, const char *filename)
{
  DiaSvgRenderer *renderer;
  FILE *file;
  gchar buf[512];
  time_t time_now;
  Rectangle *extent;
  const char *name;
  xmlDtdPtr dtd;
 
  file = g_fopen(filename, "w");

  if (file==NULL) {
    message_error(_("Can't open output file %s: %s\n"), 
		  dia_message_filename(filename), strerror(errno));
    return NULL;
  }
  fclose(file);

  /* we need access to our base object */
  renderer = DIA_SVG_RENDERER (g_object_new(SVG_TYPE_RENDERER, NULL));

  renderer->filename = g_strdup(filename);

  renderer->dash_length = 1.0;
  renderer->dot_length = 0.2;
  renderer->saved_line_style = LINESTYLE_SOLID;

  /* set up the root node */
  renderer->doc = xmlNewDoc((const xmlChar *)"1.0");
  renderer->doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  renderer->doc->standalone = FALSE;
  dtd = xmlCreateIntSubset(renderer->doc, (const xmlChar *)"svg",
		     (const xmlChar *)"-//W3C//DTD SVG 1.0//EN",
		     (const xmlChar *)"http://www.w3.org/TR/2001/PR-SVG-20010719/DTD/svg10.dtd");
  xmlAddChild((xmlNodePtr) renderer->doc, (xmlNodePtr) dtd);
  renderer->root = xmlNewDocNode(renderer->doc, NULL, (const xmlChar *)"svg", NULL);
  xmlAddSibling(renderer->doc->children, (xmlNodePtr) renderer->root);

  /* set the extents of the SVG document */
  extent = &data->extents;
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->right - extent->left)));
  xmlSetProp(renderer->root, (const xmlChar *)"width", (xmlChar *) buf);
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->bottom - extent->top)));
  xmlSetProp(renderer->root, (const xmlChar *)"height", (xmlChar *) buf);
  g_snprintf(buf, sizeof(buf), "%d %d %d %d",
	     (int)floor(extent->left), (int)floor(extent->top),
	     (int)ceil(extent->right - extent->left),
	     (int)ceil(extent->bottom - extent->top));
  xmlSetProp(renderer->root, (const xmlChar *)"viewBox", (xmlChar *) buf);
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns", (const xmlChar *)"http://www.w3.org/2000/svg");
  
  time_now = time(NULL);
  name = g_get_user_name();

#if 0
  /* some comments at the top of the file ... */
  xmlAddChild(renderer->root, xmlNewText("\n"));
  xmlAddChild(renderer->root, xmlNewComment("Dia-Version: "VERSION));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "File: %s", dia->filename);
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "Date: %s", ctime(&time_now));
  buf[strlen(buf)-1] = '\0'; /* remove the trailing new line */
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n"));
  g_snprintf(buf, sizeof(buf), "For: %s", name);
  xmlAddChild(renderer->root, xmlNewComment(buf));
  xmlAddChild(renderer->root, xmlNewText("\n\n"));

  xmlNewChild(renderer->root, NULL, "title", dia->filename);
#endif
  
  return renderer;
}

static void 
draw_object(DiaRenderer *renderer,
            DiaObject *object) 
{
  /* TODO: wrap in  <g></g> */
  object->ops->draw(object, renderer);
}

static void
draw_rounded_rect(DiaRenderer *self, 
                  Point *ul_corner, Point *lr_corner,
                  Color *colour, real rounding)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
 
  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style", 
             (const xmlChar *) DIA_SVG_RENDERER_GET_CLASS(self)->get_draw_style(renderer, colour));

  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf),"%g", rounding);
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) buf);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) buf);
}

static void
fill_rounded_rect(DiaRenderer *self, 
                  Point *ul_corner, Point *lr_corner,
                  Color *colour, real rounding)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, NULL, (const xmlChar *)"rect", NULL);

  xmlSetProp(node, (const xmlChar *)"style", 
             (const xmlChar *) DIA_SVG_RENDERER_GET_CLASS(self)->get_fill_style(renderer, colour));

  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", lr_corner->x - ul_corner->x);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", lr_corner->y - ul_corner->y);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf),"%g", rounding);
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) buf);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) buf);
}

static void
export_svg(DiagramData *data, const gchar *filename, 
           const gchar *diafilename, void* user_data)
{
  DiaSvgRenderer *renderer;

  if ((renderer = new_svg_renderer(data, filename))) {
    data_render(data, DIA_RENDERER(renderer), NULL, NULL, NULL);
    g_object_unref(renderer);
  }
}

static const gchar *extensions[] = { "svg", NULL };
DiaExportFilter svg_export_filter = {
  N_("Scalable Vector Graphics"),
  extensions,
  export_svg
};
