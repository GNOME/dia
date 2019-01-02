#include <gtk/gtk.h>
#include "uml.h"

G_BEGIN_DECLS

#define DIA_UML_TYPE_OPERATION_ROW (dia_uml_operation_row_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlOperationRow, dia_uml_operation_row, DIA_UML, OPERATION_ROW, GtkListBoxRow)

struct _DiaUmlOperationRow {
  GtkListBoxRow parent;

  GtkWidget *title;

  DiaUmlOperation *operation;
};

GtkWidget       *dia_uml_operation_row_new           (DiaUmlOperation    *operation);
DiaUmlOperation *dia_uml_operation_row_get_operation (DiaUmlOperationRow *self);

G_END_DECLS
