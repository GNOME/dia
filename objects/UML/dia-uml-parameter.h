#include <glib-object.h>

#ifndef UML_PARAM_H
#define UML_PARAM_H

/** describes the data flow between caller and callee */
typedef enum _UMLParameterKind {
  UML_UNDEF_KIND, /**< not defined */
  UML_IN, /**< by value */
  UML_OUT, /**< by ref, can be passed in uninitialized */
  UML_INOUT /**< by ref */
} UMLParameterKind;

#define DIA_UML_TYPE_PARAMETER (dia_uml_parameter_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlParameter, dia_uml_parameter, DIA_UML, PARAMETER, GObject)

/** \brief A list of DiaUmlParameter is contained in DiaUmlOperation
 * Some would call them functions parameters ;)
 */
struct _DiaUmlParameter {
  GObject parent;

  gchar *name; /**<  name*/
  gchar *type; /**< return value */
  gchar *value; /**< Initialization,  can be NULL => No default value */
  gchar *comment; /**< comment */
  UMLParameterKind kind; /**< Not currently used */
};

DiaUmlParameter *dia_uml_parameter_new                      (void);
/** calculated the 'formated' representation */
gchar           *dia_uml_parameter_format                   (DiaUmlParameter *param);

#endif