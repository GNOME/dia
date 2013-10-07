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
void dia_gdk_renderer_set_dashes(DiaGdkRenderer *renderer, int offset);

/*!
 * \brief Dia's first display renderer
 *
 * The GdkRenderer is using the native windowing system drawing functions provided by the
 * respective GDK backend. With newer GTK+ versions it is deprecated, with GTK+3.0 it is gone.
 *
 * \extends _DiaRenderer
 *
 * \todo move the GdkRenderer out of the core into a plug-in (as done with LibartRenderer
 */
struct _DiaGdkRenderer
{
  DiaRenderer parent_instance;

  /*< private >*/
  DiaTransform *transform;        /*!< Our link to the display settings */
  GdkPixmap *pixmap;              /*!< The pixmap shown in this display  */
  guint32 width;                  /*!< The width of the pixmap in pixels */
  guint32 height;                 /*!< The height of the pixmap in pixels */
  GdkGC *gc;                      /*!< The Gdk graphics context used */
  GdkRegion *clip_region;         /*!< Clipping region in effect when interactive */

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

  real   current_alpha;
};

struct _DiaGdkRendererClass
{
  DiaRendererClass parent_class;
};

G_END_DECLS

#endif /* DIA_GDK_RENDERER_H */
