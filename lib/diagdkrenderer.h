#ifndef DIA_GDK_RENDERER_H
#define DIA_GDK_RENDERER_H

#include "diatypes.h"
#include <gdk/gdk.h>
#include "diarenderer.h"
#include "diatransform.h"

G_BEGIN_DECLS

#define DIA_TYPE_GDK_RENDERER           (dia_gdk_renderer_get_type ())
#define DIA_GDK_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_GDK_RENDERER, DiaGdkRenderer))
#define DIA_GDK_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_GDK_RENDERER, DiaGdkRendererClass))
#define DIA_IS_GDK_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_GDK_RENDERER))
#define DIA_GDK_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_GDK_RENDERER, DiaGdkRendererClass))

GType dia_gdk_renderer_get_type (void) G_GNUC_CONST;

struct _DiaGdkRenderer
{
  DiaRenderer parent_instance;

  /*< private >*/
  DiaTransform *transform;        /* Our link to the display settings */
  GdkPixmap *pixmap;              /* The pixmap shown in this display  */
  guint32 width;                  /* The width of the pixmap in pixels */
  guint32 height;                 /* The height of the pixmap in pixels */
  GdkGC *gc;
  GdkRegion *clip_region;

  /* line attributes: */
  int line_width;
  GdkLineStyle line_style;
  GdkCapStyle cap_style;
  GdkJoinStyle join_style;

  LineStyle saved_line_style;
  int dash_length;
  int dot_length;

  /** If non-NULL, this rendering is a highlighting with the given color. */
  Color *highlight_color;
};

struct _DiaGdkRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

#endif /* DIA_GDK_RENDERER_H */
