#include <gtk/gtk.h>
#include "uml.h"
#include "class.h"
#include "dia-uml-attribute.h"
#include "dia-uml-operation.h"
#include "dia-uml-formal-parameter.h"

#ifndef UML_CLASS_H
#define UML_CLASS_H

G_BEGIN_DECLS

#define DIA_UML_TYPE_CLASS (dia_uml_class_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlClass, dia_uml_class, DIA_UML, CLASS, GObject)

DiaUmlClass     *dia_uml_class_new                     (UMLClass              *klass);
void             dia_uml_class_load                    (DiaUmlClass           *self,
                                                        UMLClass              *klass);
void             dia_uml_class_store                   (DiaUmlClass           *self,
                                                        UMLClass              *klass,
                                                        GList                **added,
                                                        GList                **removed,
                                                        GList                **disconnected);
gboolean         dia_uml_class_is_template             (DiaUmlClass           *klass);
GListModel      *dia_uml_class_get_attributes          (DiaUmlClass           *self);
void             dia_uml_class_remove_attribute        (DiaUmlClass           *self,
                                                        DiaUmlAttribute       *attribute);
void             dia_uml_class_insert_attribute        (DiaUmlClass           *self,
                                                        DiaUmlAttribute       *attribute,
                                                        int                    index);
GListModel      *dia_uml_class_get_operations          (DiaUmlClass           *self);
void             dia_uml_class_remove_operation        (DiaUmlClass           *self,
                                                        DiaUmlOperation       *operation);
void             dia_uml_class_insert_operation        (DiaUmlClass           *self,
                                                        DiaUmlOperation       *operation,
                                                        int                    index);
GListModel      *dia_uml_class_get_formal_parameters   (DiaUmlClass           *self);
void             dia_uml_class_remove_formal_parameter (DiaUmlClass           *self,
                                                        DiaUmlFormalParameter *formal);
void             dia_uml_class_insert_formal_parameter (DiaUmlClass           *self,
                                                        DiaUmlFormalParameter *formal,
                                                        int                    index);

G_END_DECLS

#endif