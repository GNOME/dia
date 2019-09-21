#ifndef DIA_PS_RENDERER_H
#define DIA_PS_RENDERER_H

#include <stdio.h>
#include "color.h"

#include "diarenderer.h"

/* Distinguish between variants of postscript.
 * EPS needs bounding box, EPSI also needs preview.
 */
#define PSTYPE_PS 0
#define PSTYPE_EPS 1
#define PSTYPE_EPSI 2

G_BEGIN_DECLS

#define DIA_TYPE_PS_RENDERER           (dia_ps_renderer_get_type ())
#define DIA_PS_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_PS_RENDERER, DiaPsRenderer))
#define DIA_PS_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_PS_RENDERER, DiaPsRendererClass))
#define DIA_IS_PS_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_PS_RENDERER))
#define DIA_PS_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_PS_RENDERER, DiaPsRendererClass))

GType dia_ps_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaPsRenderer DiaPsRenderer;
typedef struct _DiaPsRendererClass DiaPsRendererClass;

struct _DiaPsRenderer
{
  DiaRenderer parent_instance;

  /** Need this if we're doing preview */
  DiagramData *diagram;

  FILE *file;

  guint pstype;
  guint pagenum;

  Color lcolor;

  gchar *title;
  gchar *paper;
  gboolean is_portrait;
  double scale;
  DiaRectangle extent;

  DiaContext *ctx;

  DiaFont *font;
  double font_height;
};

struct _DiaPsRendererClass
{
  DiaRendererClass parent_class;

  /* postscript specific renderer functions */
  void (*begin_prolog) (DiaPsRenderer *renderer);
  void (*dump_fonts) (DiaPsRenderer *renderer);
  void (*end_prolog) (DiaPsRenderer *renderer);
};

void lazy_setcolor(DiaPsRenderer *renderer, Color *color);

G_END_DECLS

#endif /* DIA_PS_RENDERER_H */
