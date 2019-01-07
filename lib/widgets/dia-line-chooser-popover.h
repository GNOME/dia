#ifndef __DIALSSPOPOVER_H__
#define __DIALSSPOPOVER_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "dia-enums.h"

G_BEGIN_DECLS

#define DIA_TYPE_LINE_CHOOSER_POPOVER (dia_line_chooser_popover_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaLineChooserPopover, dia_line_chooser_popover, DIA, LINE_CHOOSER_POPOVER, GtkPopover)

struct _DiaLineChooserPopoverClass {
  GtkPopoverClass parent_class;
};

GtkWidget *dia_line_chooser_popover_new                  (void);
LineStyle  dia_line_chooser_popover_get_line_style       (DiaLineChooserPopover *self,
                                                          gdouble               *length);
void       dia_line_chooser_popover_set_line_style       (DiaLineChooserPopover *self,
                                                          LineStyle              line_style);
void       dia_line_chooser_popover_set_length           (DiaLineChooserPopover *self,
                                                          gdouble                length);

G_END_DECLS

#endif
