#include <gtk/gtk.h>
#include "uml.h"
#include "dia-uml-list-store.h"

G_BEGIN_DECLS

#define DIA_UML_TYPE_OPERATION_PARAMETER_ROW (dia_uml_operation_parameter_row_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlOperationParameterRow, dia_uml_operation_parameter_row, DIA_UML, OPERATION_PARAMETER_ROW, GtkListBoxRow)

GtkWidget       *dia_uml_operation_parameter_row_new           (DiaUmlParameter             *parameter,
                                                                DiaUmlListStore             *model);
DiaUmlParameter *dia_uml_operation_parameter_row_get_parameter (DiaUmlOperationParameterRow *self);

G_END_DECLS
