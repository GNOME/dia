#ifndef __DIALSS_H__
#define __DIALSS_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "dia-enums.h"

G_BEGIN_DECLS

/* --------------- DiaLinePreview -------------------------------- */

#define DIA_TYPE_LINE_PREVIEW (dia_line_preview_get_type ())
G_DECLARE_FINAL_TYPE (DiaLinePreview, dia_line_preview, DIA, LINE_PREVIEW, GtkWidget)

struct _DiaLinePreview
{
  GtkWidget parent;
  LineStyle lstyle;
  gdouble   length;
};

GtkWidget *dia_line_preview_new            (LineStyle       lstyle);
LineStyle  dia_line_preview_get_line_style (DiaLinePreview *self);
void       dia_line_preview_set_line_style (DiaLinePreview *self,
                                            LineStyle       lstyle,
                                            gdouble         length);

/* --------------- DiaLineStyleSelector -------------------------------- */

#define DIA_TYPE_LINE_STYLE_SELECTOR (dia_line_style_selector_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaLineStyleSelector, dia_line_style_selector, DIA, LINE_STYLE_SELECTOR, GtkMenuButton)

struct _DiaLineStyleSelectorClass {
  GtkMenuButtonClass parent_class;
};

GtkWidget *dia_line_style_selector_new            (void);
void       dia_line_style_selector_get_line_style (DiaLineStyleSelector *as,
                                                   LineStyle            *linestyle, 
                                                   gdouble              *dashlength);
void       dia_line_style_selector_set_line_style (DiaLineStyleSelector *as,
                                                   LineStyle             linestyle,
                                                   gdouble               dashlength);

G_END_DECLS

#endif
