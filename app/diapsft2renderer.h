#ifndef DIA_PS_FT2_RENDERER_H
#define DIA_PS_FT2_RENDERER_H

#include "diapsrenderer.h"

G_BEGIN_DECLS

#define DIA_TYPE_PS_FT2_RENDERER           (dia_ps_ft2_renderer_get_type ())
#define DIA_PS_FT2_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_PS_FT2_RENDERER, DiaPsFt2Renderer))
#define DIA_PS_FT2_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_PS_FT2_RENDERER, DiaPsFt2RendererClass))
#define DIA_IS_PS_FT2_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_PS_FT2_RENDERER))
#define DIA_PS_FT2_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_PS_FT2_RENDERER, DiaPsFt2RendererClass))

GType dia_ps_ft2_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaPsFt2Renderer DiaPsFt2Renderer;
typedef struct _DiaPsFt2RendererClass DiaPsFt2RendererClass;

struct _DiaPsFt2Renderer
{
  DiaPsRenderer parent_instance;

  DiaFont *current_font;
  real current_height;
};

struct _DiaPsFt2RendererClass
{
  DiaPsRendererClass parent_class;
};

G_END_DECLS

#endif /* DIA_PS_FT2_RENDERER_H */
