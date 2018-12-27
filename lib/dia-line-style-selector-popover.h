#ifndef __DIALSSPOPOVER_H__
#define __DIALSSPOPOVER_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "dia-enums.h"

G_BEGIN_DECLS

#define DIA_TYPE_LINE_STYLE_SELECTOR_POPOVER (dia_line_style_selector_popover_get_type ())
G_DECLARE_DERIVABLE_TYPE (DiaLineStyleSelectorPopover, dia_line_style_selector_popover, DIA, LINE_STYLE_SELECTOR_POPOVER, GtkPopover)

struct _DiaLineStyleSelectorPopoverClass {
  GtkPopoverClass parent_class;
};

GtkWidget *dia_line_style_selector_popover_new                  ();
LineStyle  dia_line_style_selector_popover_get_line_style       (DiaLineStyleSelectorPopover *self,
                                                                 gdouble                     *length);
void       dia_line_style_selector_popover_set_line_style       (DiaLineStyleSelectorPopover *self,
                                                                 LineStyle                    line_style);
void       dia_line_style_selector_popover_set_length           (DiaLineStyleSelectorPopover *self,
                                                                 gdouble                      length);

G_END_DECLS

#endif
