#ifndef DIA_SVG_RENDERER_H
#define DIA_SVG_RENDERER_H

#include "diatypes.h"
#include "diarenderer.h"

G_BEGIN_DECLS

#define DIA_TYPE_SVG_RENDERER           (dia_svg_renderer_get_type ())
#define DIA_SVG_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_SVG_RENDERER, DiaSvgRenderer))
#define DIA_SVG_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_SVG_RENDERER, DiaSvgRendererClass))
#define DIA_IS_SVG_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_SVG_RENDERER))
#define DIA_SVG_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_SVG_RENDERER, DiaSvgRendererClass))

GType dia_svg_renderer_get_type (void) G_GNUC_CONST;

struct _DiaSvgRenderer
{
  DiaRenderer parent_instance;

  /*< protected >*/
  char *filename;

  xmlDocPtr doc;
  xmlNodePtr root;
  xmlNsPtr svg_name_space;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;

  real linewidth;
  const char *linecap;
  const char *linejoin;
  char *linestyle; /* not const -- must free */
};

struct _DiaSvgRendererClass
{
  DiaRendererClass parent_class;

  const gchar* (*get_draw_style) (DiaSvgRenderer*, Color*);
  const gchar* (*get_fill_style) (DiaSvgRenderer*, Color*);
};

G_END_DECLS

#endif /* DIA_SVG_RENDERER_H */
