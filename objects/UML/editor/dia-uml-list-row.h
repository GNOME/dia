#include <gtk/gtk.h>
#include "dia-uml-list-data.h"
#include "dia-uml-list-store.h"

G_BEGIN_DECLS

#define DIA_UML_TYPE_LIST_ROW (dia_uml_list_row_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlListRow, dia_uml_list_row, DIA_UML, LIST_ROW, GtkListBoxRow)

GtkWidget      *dia_uml_list_row_new      (DiaUmlListData  *data,
                                           DiaUmlListStore *model);
DiaUmlListData *dia_uml_list_row_get_data (DiaUmlListRow   *self);

G_END_DECLS
