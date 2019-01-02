#include <gtk/gtk.h>
#include "uml.h"
#include "dia-uml-formal-parameter.h"

#define DIA_UML_TYPE_FORMAL_PARAMETER_DIALOG (dia_uml_formal_parameter_dialog_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlFormalParameterDialog, dia_uml_formal_parameter_dialog, DIA_UML, FORMAL_PARAMETER_DIALOG, GtkDialog)

struct _DiaUmlFormalParameterDialog {
  GtkDialog parent;

  GtkWidget *name;
  GtkWidget *type;

  DiaUmlFormalParameter *parameter;
};

GtkWidget *dia_uml_formal_parameter_dialog_new (GtkWindow             *parent,
                                                DiaUmlFormalParameter *param);
