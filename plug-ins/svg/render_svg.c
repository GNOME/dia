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

#include "config.h"

#include <glib/gi18n-lib.h>

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
#include "diagramdata.h"
#include "object.h"
#include "group.h"
#include "textline.h"
#include "dia_svg.h"
#include "dia-layer.h"
#include "dia-graphene.h"

G_BEGIN_DECLS

#define SVG_TYPE_RENDERER           (svg_renderer_get_type ())
#define SVG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), SVG_TYPE_RENDERER, SvgRenderer))
#define SVG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), SVG_TYPE_RENDERER, SvgRendererClass))
#define SVG_IS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SVG_TYPE_RENDERER))
#define SVG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SVG_TYPE_RENDERER, SvgRendererClass))

typedef struct _SvgRenderer SvgRenderer;
typedef struct _SvgRendererClass SvgRendererClass;

/*!
 * \defgroup SvgExport SVG Export
 * \ingroup SvgPlugin
 * \brief SVG renderer written in C
 *
 * The export to SVG is based on the _SvgRenderer. It is meant to create SVG
 * as close as possible reflecting the visual and logical appearance of the
 * diagram:
 * - layers are represented as groups with the layer name as id
 * - every object is represented as group, too.
 */

/*!
 * \brief Renderer for SVG export
 *
 * This renderer is used for SVG export. It extends the base class with
 * special handling of multi-line text, grouping of object rendering into
 * object specific groups and grouping of layers.
 *
 * \extends _DiaSvgRenderer
 */
struct _SvgRenderer
{
  DiaSvgRenderer parent_instance;

  /*! track the parents while grouping in draw_object() */
  GQueue *parents;
};

struct _SvgRendererClass
{
  DiaSvgRendererClass parent_class;
};

G_END_DECLS

/* Moved because it disturbs Doxygen */
GType svg_renderer_get_type (void) G_GNUC_CONST;

static DiaSvgRenderer *new_svg_renderer(DiagramData *data, const char *filename);

static void draw_layer (DiaRenderer  *self,
                        DiaLayer     *layer,
                        gboolean      active,
                        DiaRectangle *update);
static void draw_object       (DiaRenderer *renderer,
                               DiaObject   *object,
			       DiaMatrix   *matrix);
static void draw_string        (DiaRenderer  *self,
                                const char   *text,
                                Point        *pos,
                                DiaAlignment  alignment,
                                Color        *colour);
static void draw_text_line     (DiaRenderer  *self,
                                TextLine     *text_line,
                                Point        *pos,
                                DiaAlignment  alignment,
                                Color        *colour);
static void draw_text         (DiaRenderer *self, Text *text);
static void draw_rotated_text (DiaRenderer *self, Text *text, Point *center, real angle);
static void draw_rotated_image (DiaRenderer *self, Point *point,
				real width, real height,
				real angle, DiaImage *image);

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
begin_render (DiaRenderer *self, const DiaRectangle *update)
{
  SvgRenderer *renderer = SVG_RENDERER (self);
  g_assert (g_queue_is_empty (renderer->parents));
  DIA_RENDERER_CLASS (parent_class)->begin_render (DIA_RENDERER (self), NULL);
}

static void
end_render (DiaRenderer *self)
{
  SvgRenderer *renderer = SVG_RENDERER (self);
  g_assert (g_queue_is_empty (renderer->parents));
  DIA_RENDERER_CLASS (parent_class)->end_render (DIA_RENDERER (self));
}

/*!
 * \brief Advertise special capabilities
 *
 * Some objects drawing adapts to capabilities advertised by the respective
 * renderer. Usually there is a fallback, but generally the real thing should
 * be better. The SVG renderer preserves holes, transparency, handles affine
 * transformation and also gradients.
 *
 * \memberof _SvgRenderer
 */
static gboolean
is_capable_to (DiaRenderer *renderer, RenderCapability cap)
{
  if (RENDER_HOLES == cap)
    return TRUE;
  else if (RENDER_ALPHA == cap)
    return TRUE;
  else if (RENDER_AFFINE == cap)
    return TRUE;
  else if (RENDER_PATTERN == cap)
    return TRUE;
  return FALSE;
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
  renderer_class->draw_layer = draw_layer;
  renderer_class->draw_object = draw_object;
  renderer_class->draw_string  = draw_string;
  renderer_class->draw_text  = draw_text;
  renderer_class->draw_text_line  = draw_text_line;
  renderer_class->draw_rotated_text  = draw_rotated_text;
  renderer_class->draw_rotated_image  = draw_rotated_image;
  renderer_class->is_capable_to = is_capable_to;
}

/*!
 * \brief Creation and initialization of the SvgRenderer
 *
 * Using the same base class as the Shape renderer, but with slightly
 * different parameters. Here we want to be as compatible as feasible
 * with the SVG specification to support proper diagram exchange.
 *
 * \relates _SvgRenderer
 */
static DiaSvgRenderer *
new_svg_renderer(DiagramData *data, const char *filename)
{
  DiaSvgRenderer *renderer;
  gchar buf[512];
  DiaRectangle *extent;
  xmlDtdPtr dtd;

  /* we need access to our base object */
  renderer = DIA_SVG_RENDERER (g_object_new(SVG_TYPE_RENDERER, NULL));

  renderer->filename = g_strdup(filename);

  /* apparently most svg readers don't like small values, especially not in the viewBox attribute */
  renderer->scale = 20.0;

  /* set up the root node */
  renderer->doc = xmlNewDoc((const xmlChar *)"1.0");
  renderer->doc->encoding = xmlStrdup((const xmlChar *)"UTF-8");
  renderer->doc->standalone = FALSE;
  dtd = xmlCreateIntSubset(renderer->doc, (const xmlChar *)"svg",
		     (const xmlChar *)"-//W3C//DTD SVG 1.1//EN",
		     (const xmlChar *)"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd");
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
	     (int)floor(extent->left  * renderer->scale), (int)floor(extent->top * renderer->scale),
	     (int)ceil((extent->right - extent->left) * renderer->scale),
	     (int)ceil((extent->bottom - extent->top) * renderer->scale));
  xmlSetProp(renderer->root, (const xmlChar *)"viewBox", (xmlChar *) buf);
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns", (const xmlChar *)"http://www.w3.org/2000/svg");
  xmlSetProp(renderer->root,(const xmlChar *)"xmlns:xlink", (const xmlChar *)"http://www.w3.org/1999/xlink");

  return renderer;
}

/*!
 * \brief Wrap every layer into it's own group
 * This method intercepts DiaRenderer::draw_layer() to wrap every layer's
 * object into their own named group. This seems to be the common way to
 * transport layer information via SVG.
 * \memberof _SvgRenderer
 */
static void
draw_layer (DiaRenderer  *self,
            DiaLayer     *layer,
            gboolean      active,
            DiaRectangle *update)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  SvgRenderer *svg_renderer = SVG_RENDERER (self);
  xmlNodePtr layer_group;

  g_queue_push_tail (svg_renderer->parents, renderer->root);

  /* modifying the root pointer so everything below us gets into the new node */
  renderer->root = layer_group = xmlNewNode (renderer->svg_name_space, (const xmlChar *)"g");

  if (dia_layer_get_name (layer)) {
    xmlSetProp (renderer->root,
                (const xmlChar *) "id",
                (xmlChar *) dia_layer_get_name (layer));
  }

  DIA_RENDERER_CLASS (parent_class)->draw_layer (self, layer, active, update);

  renderer->root = g_queue_pop_tail (svg_renderer->parents);
  xmlAddChild (renderer->root, layer_group);
}
/*!
 * \brief Wrap every object in \<g\>\</g\> and apply transformation
 *
 * We could try to be smart and count the objects we using for the object.
 * If it is only one the grouping is superfluous and should be removed.
 *
 * \memberof _SvgRenderer
 */
static void
draw_object(DiaRenderer *self,
            DiaObject   *object,
	    DiaMatrix   *matrix)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  SvgRenderer *svg_renderer = SVG_RENDERER (self);
  int n_children = 0;
  xmlNodePtr child, group;

  g_queue_push_tail (svg_renderer->parents, renderer->root);

  /* modifying the root pointer so everything below us gets into the new node */
  renderer->root = group = xmlNewNode (renderer->svg_name_space, (const xmlChar *)"g");

  if (IS_GROUP (object) && !matrix) {
    /* group_draw() is applying it's matrix to every child, but we can do
     * better with inline version of group drawing
     */
    const DiaMatrix *gm = group_get_transform ((Group *)object);
    GList *objs = group_objects (object);

    if (gm) {
      char *s;
      graphene_matrix_t graphene_matrix;

      dia_graphene_from_matrix (&graphene_matrix, gm);

      s = dia_svg_from_matrix (&graphene_matrix, renderer->scale);

      xmlSetProp (renderer->root, (const xmlChar *) "transform", (xmlChar *) s);

      g_clear_pointer (&s, g_free);
    }

    while (objs) {
      DiaObject *obj = (DiaObject *)objs->data;

      dia_object_draw (obj, DIA_RENDERER (renderer));
      objs = objs->next;
    }
    renderer->root = g_queue_pop_tail (svg_renderer->parents);
    xmlAddChild (renderer->root, group);
  } else {
    if (matrix) {
      char *s;
      graphene_matrix_t graphene_matrix;

      dia_graphene_from_matrix (&graphene_matrix, matrix);

      s = dia_svg_from_matrix (&graphene_matrix, renderer->scale);

      xmlSetProp(renderer->root, (const xmlChar *)"transform", (xmlChar *) s);

      g_clear_pointer (&s, g_free);
    }

    object->ops->draw(object, DIA_RENDERER (renderer));

    /* no easy way to count? */
    child = renderer->root->children;
    while (child != NULL) {
      child = child->next;
      ++n_children;
    }
    renderer->root = g_queue_pop_tail (svg_renderer->parents);
    /* if there is only one element added to the group node unpack it again  */
    if (1 == n_children && !matrix) {
      xmlAddChild (renderer->root, group->children);
      xmlUnlinkNode (group); /* dont free the children */
      xmlFree (group);
    } else {
      xmlAddChild (renderer->root, group);
    }
  }
}

#define dia_svg_dtostr(buf,d) \
  g_ascii_formatd(buf,sizeof(buf),"%g",(d)*renderer->scale)


static void
node_set_text_style (xmlNodePtr      node,
                     DiaSvgRenderer *renderer,
                     DiaFont        *font,
                     double          font_height,
                     DiaAlignment    alignment,
                     Color          *colour)
{
  double saved_width;
  char d_buf[G_ASCII_DTOSTR_BUF_SIZE];
  DiaSvgRendererClass *svg_renderer_class = DIA_SVG_RENDERER_GET_CLASS (renderer);
  GString *style;
  /* SVG font-size is the (line-) height, from SVG Spec:
   * ... property refers to the size of the font from baseline to baseline when multiple lines of text are set ...
  so we should be able to use font_height directly instead of:
   */
  double font_size = dia_font_get_size (font) * (font_height / dia_font_get_height (font));
  /* ... but at least Inkscape and Firefox would produce the wrong font-size */
  const char *family = dia_font_get_family (font);

  saved_width = renderer->linewidth;
  renderer->linewidth = 0.001;
  /* return value of get_draw_style must not be freed */
  style = g_string_new (svg_renderer_class->get_draw_style(renderer, colour, NULL));
  renderer->linewidth = saved_width;
  /* This is going to break for non-LTR texts, as SVG thinks 'start' is
   * 'right' for those.
   */
  switch (alignment) {
    case DIA_ALIGN_LEFT:
      g_string_append (style, ";text-anchor:start");
      break;
    case DIA_ALIGN_CENTRE:
      g_string_append (style, ";text-anchor:middle");
      break;
    case DIA_ALIGN_RIGHT:
      g_string_append (style, ";text-anchor:end");
      break;
    default:
      g_return_if_reached ();
  }
#if 0 /* would need a unit according to https://bugzilla.mozilla.org/show_bug.cgi?id=707071#c4 */
  tmp = g_strdup_printf("%s;font-size:%s", style,
			dia_svg_dtostr(d_buf, font_size) );
  g_clear_pointer (&style, g_free);
  style = tmp;
#else
  /* font-size as attribute can work like the other length w/o unit */
  dia_svg_dtostr (d_buf, font_size);
  xmlSetProp (node, (const xmlChar *) "font-size", (xmlChar *) d_buf);
#endif

  if (font) {
     g_string_append_printf (style,
                             ";font-family:%s;font-style:%s;font-weight:%s",
                             strcmp (family, "sans") == 0 ? "sans-serif" : family,
                             dia_font_get_slant_string (font),
                             dia_font_get_weight_string (font));
  }
  xmlSetProp (node, (xmlChar *) "style", (xmlChar *) style->str);
  g_string_free (style, TRUE);
}


/*!
 * To minimize impact of this deprecated attribute we set it as locally as
 * possible and only if it is needed at all.
 */
static void
_adjust_space_preserve (xmlNodePtr node,
			const char *str)
{
  gunichar uc;
  if (!strlen(str))
    return;
  uc = g_utf8_get_char_validated (str, -1);
  if (g_unichar_isspace (uc))
    xmlSetProp (node, (const xmlChar *)"xml:space", (const xmlChar *)"preserve");
}


/*!
 * \brief Support rendering of raw text
 *
 * This is the only function in the renderer interface using the
 * font passed in by set_font() method.
 *
 * \memberof _SvgRenderer
 */
static void
draw_string (DiaRenderer  *self,
             const char   *text,
             Point        *pos,
             DiaAlignment  alignment,
             Color        *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  char d_buf[G_ASCII_DTOSTR_BUF_SIZE];
  DiaFont *font;
  double font_height;

  font = dia_renderer_get_font (self, &font_height);

  node = xmlNewChild (renderer->root,
                      renderer->svg_name_space,
                      (xmlChar *) "text",
                      (xmlChar *) text);
  _adjust_space_preserve (node, text);

  node_set_text_style (node, renderer, font, font_height, alignment, colour);

  dia_svg_dtostr (d_buf, pos->x);
  xmlSetProp (node, (xmlChar *) "x", (xmlChar *)d_buf);
  dia_svg_dtostr (d_buf, pos->y);
  xmlSetProp (node, (xmlChar *) "y", (xmlChar *)d_buf);
}


/*
 * Support rendering of the #TextLine object
 */
static void
draw_text_line (DiaRenderer  *self,
                TextLine     *text_line,
                Point        *pos,
                DiaAlignment  alignment,
                Color        *colour)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  xmlNodePtr node;
  DiaFont *font = text_line_get_font (text_line); /* no reference? */
  double font_height = text_line_get_height (text_line);
  char d_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text",
		     (xmlChar *) text_line_get_string(text_line));
  _adjust_space_preserve (node, text_line_get_string(text_line));

  /* not using the renderers font but the textlines */
  node_set_text_style(node, renderer, font, font_height, alignment, colour);

  dia_svg_dtostr(d_buf, pos->x);
  xmlSetProp(node, (const xmlChar *)"x", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, pos->y);
  xmlSetProp(node, (const xmlChar *)"y", (xmlChar *) d_buf);
  dia_svg_dtostr(d_buf, text_line_get_width(text_line));
  xmlSetProp(node, (const xmlChar*)"textLength", (xmlChar *) d_buf);
}

/*!
 * \brief multi-line text creation
 *
 * The most high-level member function for text support. Still the
 * others have to be implemented because some _DiaObject implementations
 * use the more low-level variants.
 *
 * \memberof _SvgRenderer
 */
static void
draw_text (DiaRenderer *self, Text *text)
{
  draw_rotated_text (self, text, NULL, 0.0);
}

static void
draw_rotated_text (DiaRenderer *self, Text *text, Point *center, real angle)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);
  Point pos = text->position;
  int i;
  xmlNodePtr node_text, node_tspan;
  char d_buf[G_ASCII_DTOSTR_BUF_SIZE];

  node_text = xmlNewChild(renderer->root, renderer->svg_name_space, (const xmlChar *)"text", NULL);
  /* text 'global' properties  */
  node_set_text_style(node_text, renderer, text->font, text->height, text->alignment, &text->color);

  if (angle != 0) {
    char x_buf0[G_ASCII_DTOSTR_BUF_SIZE];
    char y_buf0[G_ASCII_DTOSTR_BUF_SIZE];
    char x_buf1[G_ASCII_DTOSTR_BUF_SIZE];
    char y_buf1[G_ASCII_DTOSTR_BUF_SIZE];
    char *trans;
     if (center)
       pos = *center;
     g_ascii_formatd (d_buf, sizeof(d_buf), "%g", angle);
     dia_svg_dtostr(x_buf0, pos.x);
     dia_svg_dtostr(y_buf0, pos.y);
     dia_svg_dtostr(x_buf1, -pos.x);
     dia_svg_dtostr(y_buf1, -pos.y);
     trans = g_strdup_printf ("translate(%s,%s) rotate(%s) translate(%s,%s)",
			      x_buf0, y_buf0, d_buf, x_buf1, y_buf1);
     xmlSetProp(node_text, (const xmlChar *)"transform", (xmlChar *) trans);
     g_clear_pointer (&trans, g_free);
  } else {
    dia_svg_dtostr(d_buf, pos.x);
    xmlSetProp(node_text, (const xmlChar *)"x", (xmlChar *) d_buf);
    dia_svg_dtostr(d_buf, pos.y);
    xmlSetProp(node_text, (const xmlChar *)"y", (xmlChar *) d_buf);
  }
  pos = text->position;
  for (i=0;i<text->numlines;i++) {
    TextLine *text_line = text->lines[i];

    node_tspan = xmlNewTextChild(node_text, renderer->svg_name_space, (const xmlChar *)"tspan",
                                 (const xmlChar *)text_line_get_string(text_line));
    _adjust_space_preserve (node_tspan, text_line_get_string(text_line));
    dia_svg_dtostr(d_buf, pos.x);
    xmlSetProp(node_tspan, (const xmlChar *)"x", (xmlChar *) d_buf);
    dia_svg_dtostr(d_buf, pos.y);
    xmlSetProp(node_tspan, (const xmlChar *)"y", (xmlChar *) d_buf);

    pos.y += text->height;
  }
}

static void
draw_rotated_image (DiaRenderer *self,
		    Point *point,
		    real width, real height,
		    real angle,
		    DiaImage *image)
{
  DiaSvgRenderer *renderer = DIA_SVG_RENDERER (self);

  /* delegate to base class ... */
  DIA_RENDERER_CLASS (parent_class)->draw_image (self, point, width, height, image);
  /* ... and modify the image node to transform */
  if (angle != 0.0) {
    xmlNodePtr node = xmlFirstElementChild (renderer->root);
    gchar d_buf[G_ASCII_DTOSTR_BUF_SIZE];
    gchar x_buf0[G_ASCII_DTOSTR_BUF_SIZE];
    gchar y_buf0[G_ASCII_DTOSTR_BUF_SIZE];
    gchar x_buf1[G_ASCII_DTOSTR_BUF_SIZE];
    gchar y_buf1[G_ASCII_DTOSTR_BUF_SIZE];
    gchar *trans;
    Point pos = { point->x + width/2, point->y + height/2 }; /* center */

    g_return_if_fail (node != NULL && xmlStrcmp (node->name, (const xmlChar *)"image") == 0);

    g_ascii_formatd (d_buf, sizeof(d_buf), "%g", angle);
    dia_svg_dtostr(x_buf0, pos.x);
    dia_svg_dtostr(y_buf0, pos.y);
    dia_svg_dtostr(x_buf1, -pos.x);
    dia_svg_dtostr(y_buf1, -pos.y);
    trans = g_strdup_printf ("translate(%s,%s) rotate(%s) translate(%s,%s)",
                             x_buf0, y_buf0, d_buf, x_buf1, y_buf1);
    xmlSetProp (node, (const xmlChar *)"transform", (xmlChar *) trans);
    g_clear_pointer (&trans, g_free);
  }
}


/*!
 * \brief Callback function registered for export
 * \ingroup SvgExport
 */
static gboolean
export_svg (DiagramData *data,
            DiaContext  *ctx,
            const char  *filename,
            const char  *diafilename,
            void        *user_data)
{
  DiaSvgRenderer *renderer;

  if ((renderer = new_svg_renderer (data, filename))) {
    data_render (data, DIA_RENDERER (renderer), NULL, NULL, NULL);

    g_clear_object (&renderer);

    return TRUE;
  }

  return FALSE;
}


static const gchar *extensions[] = { "svg", NULL };
DiaExportFilter svg_export_filter = {
  N_("Scalable Vector Graphics"),
  extensions,
  export_svg,
  NULL, /* user_data */
  "dia-svg"
};
