#ifndef DIA_IMPORT_RENDERER_H
#define DIA_IMPORT_RENDERER_H

#include "diatypes.h"
#include "diarenderer.h"

G_BEGIN_DECLS

#define DIA_TYPE_IMPORT_RENDERER           (dia_import_renderer_get_type ())
#define DIA_IMPORT_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_IMPORT_RENDERER, DiaImportRenderer))
#define DIA_IMPORT_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_IMPORT_RENDERER, DiaImportRendererClass))
#define DIA_IS_IMPORT_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_IMPORT_RENDERER))
#define DIA_IMPORT_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_IMPORT_RENDERER, DiaImportRendererClass))

GType dia_import_renderer_get_type (void) G_GNUC_CONST;

typedef struct _DiaImportRenderer DiaImportRenderer;
typedef struct _DiaImportRendererClass DiaImportRendererClass;

/*!
 * \brief Creation of Dia's standard objects via renderer interface
 *
 * To be used for import plug-ins which want to support a file format with a similar
 * rendering model as Dia internal. Complex rendering models like SVG and PDF are not
 * not in the intended scope.
 *
 * \extends _DiaRenderer
 */
struct _DiaImportRenderer {
  DiaRenderer parent_instance;

  /*! \protected state variables should not be set directly but with the rendering interface */
  DiaLineStyle line_style;
  DiaFillStyle fill_style;
  double       dash_length;

  double       line_width;
  DiaLineCaps  line_caps;
  DiaLineJoin  line_join;

  /*! \private */
  /*! \private pattern set by set_pattern */
  DiaPattern *pattern;
  /*! \private all patterns seen between begin_render and end_render */
  GList      *objects;
};

struct _DiaImportRendererClass
{
  DiaRendererClass parent_class;
};

DiaObject *dia_import_renderer_get_objects (DiaRenderer *renderer);

G_END_DECLS

#endif /* DIA_IMPORT_RENDERER_H */
