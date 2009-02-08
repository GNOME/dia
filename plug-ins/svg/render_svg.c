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
#include "textline.h"

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
  
  /* track the parents while grouping in draw_object() */
  GQueue *parents;
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
static void draw_string       (DiaRenderer *self,
	                       const char *text,
			       Point *pos, Alignment alignment,
			       Color *colour);
static void draw_text_line    (DiaRenderer *self, TextLine *text_line,
	                       Point *pos, Alignment alignment, Color *colour);
static void draw_text         (DiaRenderer *self, Text *text);

static void svg_renderer_class_init (SvgRendererClass *klass);

static gpointer parent_class = NULL;

/* constructor */
static void
svg_renderer_init (GTypeInstance *self, gpointer g_class)
{
  SvgRenderer *renderer = SVG_RENDERER (self);

  renderer->parents = g_queue_new ();
}

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
	svg_renderer_init /* init */
      };

      object_type = g_type_register_static (DIA_TYPE_SVG_RENDERER,
                                            "SvgRenderer",
                                            &object_info, 0);
    }
  
  return object_type;
}

static void
begin_render (DiaRenderer *self)
{
  SvgRenderer *renderer = SVG_RENDERER (self);
  g_assert (g_queue_is_empty (renderer->parents));
  DIA_RENDERER_CLASS (parent_class)->begin_render (DIA_RENDERER (self));
}

static void
end_render (DiaRenderer *self)
{
  SvgRenderer *renderer = SVG_RENDERER (self);
  g_assert (g_queue_is_empty (renderer->parents));
  DIA_RENDERER_CLASS (parent_class)->end_render (DIA_RENDERER (self));
}

/* destructor */
static void
svg_renderer_finalize (GObject *object)
{
  SvgRenderer *svg_renderer = SVG_RENDERER (object);

  g_queue_free (svg_renderer->parents);

  G_OBJECT_CLASS (parent_class)->finalize (object);  
}

static void
svg_renderer_class_init (SvgRendererClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaRendererClass *renderer_class = DIA_RENDERER_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);

  object_class->finalize = svg_renderer_finalize;

  renderer_class->begin_render = begin_render;
  renderer_class->end_render = end_render;
  renderer_class->draw_object = draw_object;
  renderer_class->draw_rounded_rect = draw_rounded_rect;
  renderer_class->fill_rounded_rect = fill_rounded_rect;
  renderer_class->draw_string  = draw_string;
  renderer_class->draw_text  = draw_text;
  renderer_class->draw_text_line  = draw_text_line;
}


static DiaSvgRenderer *
new_svg_renderer(DiagramData *data, const char *filename)
{
  DiaSvgRenderer *renderer;
  SvgRenderer *svg_renderer;
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
  /* apparently most svg readers don't like small values, especially not in the viewBox attribute */
  renderer->scale = 20.0;

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

  /* add namespaces to make strict parsers happy, e.g. Firefox */
  svg_renderer = SVG_RENDERER (renderer);

  /* set the extents of the SVG document */
  extent = &data->extents;
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->right - extent->left)));
  xmlSetProp(renderer->root, (const xmlChar *)"width", (xmlChar *) buf);
  g_snprintf(buf, sizeof(buf), "%dcm",
	     (int)ceil((extent->bottom - extent->top)));
  xmlSetProp(renderer->root, (const xmlChar *)"height", (xmlChar *) buf);
  g_snprintf(buf, sizeof(buf), "%d %d %d %d",
	     (int)floor(extent->left  * renderer->scale), (int)floor(extent->top * renderer->scale),
	     (int)ceil((extent->right - extent->left) * renderer->scale),
	     (int)ceil((extent->bottom - extent->top) * renderer->scale));
  xmlSetProp(renderer->root, (const xmlChar *)"viewBox", (xmlChar *) buf);
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns", (const xmlChar *)"http://www.w3.org/2000/svg");
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns", (const xmlChar *)"http://www.w3.org/2000/svg");
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns:xlink", (const xmlChar *)"http://www.w3.org/1999/xlink");

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
draw_object(DiaRenderer *self,
            DiaObject *object) 
{
  /* wrap in  <g></g> 
   * We could try to be smart and count the objects we using for the object.
   * If it is only one the grouping is superfluous and should be removed.
   */
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  SvgRenderer *svg_renderer = SVG_RENDERER (self);
  int n_children = 0;
  xmlNodePtr child, group;

  g_queue_push_tail (svg_renderer->parents, renderer->root);
  /* modifying the root pointer so everything below us gets into the new node */
  renderer->root = group = xmlNewNode (renderer->svg_name_space, (const xmlChar *)"g");

  object->ops->draw(object, DIA_RENDERER (renderer));
  
  /* no easy way to count? */
  child = renderer->root->children;
  while (child != NULL) {
    child = child->next;
    ++n_children;
  }
  renderer->root = g_queue_pop_tail (svg_renderer->parents);
  /* if there is only one element added to the group node unpack it again  */
  if (1 == n_children) {
    xmlAddChild (renderer->root, group->children);
    xmlUnlinkNode (group); /* dont free the children */
    xmlFree (group);
  } else {
    xmlAddChild (renderer->root, group);
  }
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

  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->x * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", ul_corner->y * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->x - ul_corner->x) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->y - ul_corner->y) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf),"%g", (rounding * renderer->scale));
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

  g_ascii_formatd(buf, sizeof(buf), "%g", (ul_corner->x * renderer->scale));
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (ul_corner->y * renderer->scale));
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->x - ul_corner->x) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"width", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf), "%g", (lr_corner->y - ul_corner->y) * renderer->scale);
  xmlSetProp(node, (const xmlChar *)"height", (xmlChar *) buf);
  g_ascii_formatd(buf, sizeof(buf),"%g", (rounding * renderer->scale));
  xmlSetProp(node, (const xmlChar *)"rx", (xmlChar *) buf);
  xmlSetProp(node, (const xmlChar *)"ry", (xmlChar *) buf);
}

#define dia_svg_dtostr(buf,d) \
  g_ascii_formatd(buf,sizeof(buf),"%g",(d)*renderer->scale)

static void
node_set_text_style (xmlNodePtr      node,
                     DiaSvgRenderer *renderer,
		     const DiaFont  *font,
		     real            font_height,
                     Alignment       alignment,
		     Color          *colour)
{
  char *style, *tmp;
  real saved_width;
  gchar d_buf[G_ASCII_DTOSTR_BUF_SIZE];
  DiaSvgRendererClass *svg_renderer_class = DIA_SVG_RENDERER_GET_CLASS (renderer);
  /* SVG font-size is the (line-) height, from SVG Spec:
   * ... property refers to the size of the font from baseline to baseline when multiple lines of text are set ...
  so we should be able to use font_height directly instead of:
   */
  real font_size = dia_font_get_size (font) * (font_height / dia_font_get_height (font));
  /* ... but at least Inkscape and Firefox would produce the wrong font-size */
  const gchar *family = dia_font_get_family(font);

  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  style = (char*)svg_renderer_class->get_fill_style(renderer, colour);
  /* return value must not be freed */
  renderer->linewidth = saved_width;
  /* This is going to break for non-LTR texts, as SVG thinks 'start' is
   * 'right' for those.
   */
  switch (alignment) {
  case ALIGN_LEFT:
    style = g_strconcat(style, ";text-anchor:start", NULL);
    break;
  case ALIGN_CENTER:
    style = g_strconcat(style, ";text-anchor:middle", NULL);
    break;
  case ALIGN_RIGHT:
    style = g_strconcat(style, ";text-anchor:end", NULL);
    break;
  }
  tmp = g_strdup_printf("%s;font-size:%s", style,
			dia_svg_dtostr(d_buf, font_size) );
  g_free (style);
  style = tmp;

  if (font) {
     tmp = g_strdup_printf("%s;font-family:%s;font-style:%s;"
                           "font-weight:%s",style,
                           strcmp(family, "sans") == 0 ? "sanserif" : family,
                           dia_font_get_slant_string(font),
                           dia_font_get_weight_string(font));
     g_free(style);
     style = tmp;
  }

  /* have to do something about fonts here ... */

  xmlSetProp(node, (xmlChar *)"style", (xmlChar *)style);
  g_free(style);
}

static void
draw_string(DiaRenderer *self,
	    const char *text,
	    Point *pos, Alignment alignment,
	    Color *colour)
{    
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  gchar d_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (xmlChar *)"text", (xmlChar *)text);

  node_set_text_style(node, renderer, self->font, self->font_height, alignment, colour);
  
  dia_svg_dtostr(d_buf, pos->x);
  xmlSetProp(node, (xmlChar *)"x", (xmlChar *)d_buf);
  dia_svg_dtostr(d_buf, pos->y);
  xmlSetProp(node, (xmlChar *)"y", (xmlChar *)d_buf);
}

static void
draw_text_line(DiaRenderer *self, TextLine *text_line,
	       Point *pos, Alignment alignment, Color *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  DiaFont *font = text_line_get_font(text_line); /* no reference? */
  real font_height = text_line_get_height(text_line);
  gchar d_buf[G_ASCII_DTOSTR_BUF_SIZE];
  
  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text", 
		     (xmlChar *) text_line_get_string(text_line));

  /* not using the renderers font but the textlines */
  node_set_text_style(node, renderer, font, font_height, alignment, colour);

  dia_svg_dtostr(d_buf, pos->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, pos->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, text_line_get_width(text_line));
  xmlSetProp(node, (const xmlChar*)"textLength", (xmlChar *) d_buf);
}

static void
draw_text (DiaRenderer *self, Text *text)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  Point pos = text->position;
  int i;
  xmlNodePtr node_text, node_tspan;
  gchar d_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node_text = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text", NULL);
  /* text 'global' properties  */
  node_set_text_style(node_text, renderer, text->font, text->height, text->alignment, &text->color);
  dia_svg_dtostr(d_buf, pos.x);
  xmlSetProp(node_text, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, pos.y);
  xmlSetProp(node_text, (const xmlChar *)"y", (xmlChar *) d_buf);
  
  pos = text->position;
  for (i=0;i<text->numlines;i++) {
    TextLine *text_line = text->lines[i];

    node_tspan = xmlNewChild(node_text, renderer->svg_name_space, (const xmlChar *)"tspan",
                             (const xmlChar *)text_line_get_string(text_line));
    dia_svg_dtostr(d_buf, pos.x);
    xmlSetProp(node_tspan, (const xmlChar *)"x", (xmlChar *) d_buf);
    dia_svg_dtostr(d_buf, pos.y);
    xmlSetProp(node_tspan, (const xmlChar *)"y", (xmlChar *) d_buf);
    
    pos.y += text->height;
  }
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
