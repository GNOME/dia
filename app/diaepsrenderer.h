#ifndef DIA_EPS_RENDERER_H
#define DIA_EPS_RENDERER_H

#include "diarenderer.h"

/* We use an instance of color in the struct, so include it */
#include "color.h"

G_BEGIN_DECLS

#define DIA_TYPE_EPS_RENDERER           (dia_eps_renderer_get_type ())
#define DIA_EPS_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_EPS_RENDERER, DiaEpsRenderer))
#define DIA_EPS_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_EPS_RENDERER, DiaEpsRendererClass))
#define DIA_IS_EPS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_EPS_RENDERER))
#define DIA_EPS_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_EPS_RENDERER, DiaEpsRendererClass))

GType dia_eps_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaEpsRenderer DiaEpsRenderer;
typedef struct _DiaEpsRendererClass DiaEpsRendererClass;

struct _DiaEpsRenderer
{
  DiaRenderer parent_instance;

  /*< protected >*/
  FILE *file;
  int is_ps;
  int pagenum;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;
  Color lcolor;

  real scale;

#ifdef HAVE_FREETYPE
  DiaFont *current_font;
  real current_height;
  PangoContext *context;
#endif
};

struct _DiaEpsRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

#endif /* DIA_EPS_RENDERER_H */
