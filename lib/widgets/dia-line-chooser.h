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

/* --------------- DiaLineChooser -------------------------------- */

#define DIA_TYPE_LINE_CHOOSER (dia_line_chooser_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaLineChooser, dia_line_chooser, DIA, LINE_CHOOSER, GtkMenuButton)

struct _DiaLineChooserClass {
  GtkMenuButtonClass parent_class;
};

GtkWidget *dia_line_chooser_new            (void);
void       dia_line_chooser_get_line_style (DiaLineChooser *as,
                                            LineStyle      *linestyle, 
                                            gdouble        *dashlength);
void       dia_line_chooser_set_line_style (DiaLineChooser *as,
                                            LineStyle       linestyle,
                                            gdouble         dashlength);

G_END_DECLS

#endif
