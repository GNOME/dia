#ifndef DIA_LIBART_RENDERER_H
#define DIA_LIBART_RENDERER_H

#include "diatypes.h"
#include "geometry.h"
#include "diarenderer.h"
#include "diatransform.h"

#ifdef HAVE_LIBART
#include <libart_lgpl/art_vpath.h>
#include <libart_lgpl/art_vpath_dash.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_svp_vpath_stroke.h>
#endif

G_BEGIN_DECLS

typedef struct _DiaLibartRenderer DiaLibartRenderer;
typedef struct _DiaLibartRendererClass DiaLibartRendererClass;

#define DIA_TYPE_LIBART_RENDERER           (dia_libart_renderer_get_type ())
#define DIA_LIBART_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_LIBART_RENDERER, DiaLibartRenderer))
#define DIA_LIBART_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_LIBART_RENDERER, DiaLibartRendererClass))
#define DIA_IS_LIBART_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_LIBART_RENDERER))
#define DIA_LIBART_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_LIBART_RENDERER, DiaLibartRendererClass))

GType dia_libart_renderer_get_type (void) G_GNUC_CONST;

struct _DiaLibartRenderer
{
  DiaRenderer parent_instance;

  /*< private >*/
  DiaTransform *transform;        /* Our link to the display settings */
#ifdef HAVE_LIBART
  int pixel_width, pixel_height;
  guint8 *rgb_buffer;
  int clip_rect_empty;
  IntRectangle clip_rect;
  
  /* line attributes: */
  double line_width;
  ArtPathStrokeCapType cap_style;
  ArtPathStrokeJoinType join_style;

  int dash_enabled;
  ArtVpathDash dash;
  Color *highlight_color;
#endif
  DiaContext *ctx;
};

struct _DiaLibartRendererClass
{
  DiaRendererClass parent_class;
};

void dia_libart_renderer_iface_init (DiaInteractiveRendererInterface* iface);

G_END_DECLS

#endif /* DIA_LIBART_RENDERER_H */
