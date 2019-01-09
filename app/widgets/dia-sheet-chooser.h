#include <gtk/gtk.h>

#include "sheet.h"

G_BEGIN_DECLS

#define DIA_TYPE_SHEET_CHOOSER_POPOVER (dia_sheet_chooser_popover_get_type ())
G_DECLARE_FINAL_TYPE (DiaSheetChooserPopover, dia_sheet_chooser_popover, DIA, SHEET_CHOOSER_POPOVER, GtkPopover)

void       dia_sheet_chooser_popover_set_model (DiaSheetChooserPopover *self,
                                                GListModel             *model,
                                                DiaSheet               *current);


#define DIA_TYPE_SHEET_CHOOSER (dia_sheet_chooser_get_type ())
G_DECLARE_FINAL_TYPE (DiaSheetChooser, dia_sheet_chooser, DIA, SHEET_CHOOSER, GtkMenuButton)

GtkWidget *dia_sheet_chooser_new               ();
void       dia_sheet_chooser_set_model         (DiaSheetChooser *self,
                                                GListModel      *model,
                                                DiaSheet        *current);

G_END_DECLS
