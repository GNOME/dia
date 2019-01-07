#include <glib-object.h>
#include "uml.h"

#ifndef UML_FP_H
#define UML_FP_H

#define DIA_UML_TYPE_FORMAL_PARAMETER (dia_uml_formal_parameter_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlFormalParameter, dia_uml_formal_parameter, DIA_UML, FORMAL_PARAMETER, GObject)

/** \brief A list of UMLFormalParameter is contained in DiaUmlOperation
 * Some would call them template parameters ;)
 */
struct _DiaUmlFormalParameter {
  GObject parent;

  gchar *name; /**< name */
  gchar *type; /**< Can be NULL => Type parameter */
};

DiaUmlFormalParameter *dia_uml_formal_parameter_new    (void);
/** calculated the 'formated' representation */
gchar                 *dia_uml_formal_parameter_format (DiaUmlFormalParameter *self);
DiaUmlFormalParameter *dia_uml_formal_parameter_copy   (DiaUmlFormalParameter *self);

void uml_formalparameter_write(AttributeNode attr_node, DiaUmlFormalParameter *param, DiaContext *ctx);

#endif
