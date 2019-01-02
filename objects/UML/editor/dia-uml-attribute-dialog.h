#include <gtk/gtk.h>
#include "uml.h"
#include "dia-uml-attribute.h"

#define DIA_UML_TYPE_ATTRIBUTE_DIALOG (dia_uml_attribute_dialog_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlAttributeDialog, dia_uml_attribute_dialog, DIA_UML, ATTRIBUTE_DIALOG, GtkDialog)

struct _DiaUmlAttributeDialog {
  GtkDialog parent;

  GtkWidget *name;
  GtkWidget *type;
  GtkWidget *value;
  GtkWidget *visibility;
  GtkWidget *scope;
  GtkTextBuffer *comment;

  DiaUmlAttribute *attribute;
};

GtkWidget *dia_uml_attribute_dialog_new (GtkWindow       *parent,
                                         DiaUmlAttribute *attr);
