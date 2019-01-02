#include <gtk/gtk.h>
#include "uml.h"
#include "dia-uml-class.h"

#ifndef UML_CLASS_EDITOR_H
#define UML_CLASS_EDITOR_H

G_BEGIN_DECLS

#define DIA_UML_TYPE_CLASS_EDITOR (dia_uml_class_editor_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlClassEditor, dia_uml_class_editor, DIA_UML, CLASS_EDITOR, GtkScrolledWindow)

GtkWidget   *dia_uml_class_editor_new       (DiaUmlClass       *klass);
DiaUmlClass *dia_uml_class_editor_get_class (DiaUmlClassEditor *self);

G_END_DECLS

#endif