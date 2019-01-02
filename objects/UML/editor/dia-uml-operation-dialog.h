#include <gtk/gtk.h>
#include "uml.h"
#include "dia-uml-operation.h"

#define DIA_UML_TYPE_OPERATION_DIALOG (dia_uml_operation_dialog_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlOperationDialog, dia_uml_operation_dialog, DIA_UML, OPERATION_DIALOG, GtkDialog)

struct _DiaUmlOperationDialog {
  GtkDialog parent;

  GtkWidget *name;
  GtkWidget *type;
  GtkWidget *stereotype;
  GtkWidget *visibility;
  GtkWidget *inheritance;
  GtkWidget *scope;
  GtkWidget *query;
  GtkTextBuffer *comment;
  GtkWidget *list;

  DiaUmlOperation *operation;
};

GtkWidget *dia_uml_operation_dialog_new (GtkWindow       *parent,
                                         DiaUmlOperation *op);
