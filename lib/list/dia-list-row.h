#include <gtk/gtk.h>
#include "dia-list-data.h"
#include "dia-list-store.h"

G_BEGIN_DECLS

#define DIA_TYPE_LIST_ROW (dia_list_row_get_type ())
G_DECLARE_FINAL_TYPE (DiaListRow, dia_list_row, DIA, LIST_ROW, GtkListBoxRow)

GtkWidget   *dia_list_row_new      (DiaListData  *data,
                                    DiaListStore *model);
DiaListData *dia_list_row_get_data (DiaListRow   *self);

G_END_DECLS
