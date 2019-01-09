#ifndef __DIACANVAS_H__
#define __DIACANVAS_H__

#include <glib.h>
#include <gtk/gtk.h>
#include "display.h"

G_BEGIN_DECLS

#define DIA_TYPE_CANVAS (dia_canvas_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaCanvas, dia_canvas, DIA, CANVAS, GtkDrawingArea)

struct _DiaCanvasClass {
  GtkDrawingAreaClass parent_class;
};

GtkWidget       *dia_canvas_new                 (DiaDisplay *ddisp);
DiaDisplay      *dia_canvas_get_display         (DiaCanvas  *self);

G_END_DECLS

#endif
