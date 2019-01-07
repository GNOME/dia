#include <glib-object.h>
#include "uml.h"
#include "dia-uml-parameter.h"
#include "list/dia-list-store.h"

#ifndef UML_OP_H
#define UML_OP_H

/** In some languages there are different kinds of class inheritances */
typedef enum _UMLInheritanceType {
  UML_ABSTRACT, /**< Pure virtual method: an object of this class cannot be instanciated */
  UML_POLYMORPHIC, /**< Virtual method : could be reimplemented in derivated classes */
  UML_LEAF /**< Final method: can't be redefined in subclasses */
} UMLInheritanceType;

#define DIA_UML_TYPE_OPERATION (dia_uml_operation_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlOperation, dia_uml_operation, DIA_UML, OPERATION, GObject)

/** \brief A list of DiaUmlOperation is contained in UMLClass
 * Some would call them member functions ;)
 */
struct _DiaUmlOperation {
  GObject parent;

  gint internal_id; /**< Arbitrary integer to recognize operations after
		     * the user has shuffled them in the dialog. */
  gchar *name; /**< the function name */
  gchar *type; /**< Return type, NULL => No return type */
  gchar *comment; /**< comment */  
  gchar *stereotype; /**< just some string */
  UMLVisibility visibility; /**< allowed access */
  UMLInheritanceType inheritance_type;
  int query; /**< Do not modify the object, in C++ this is a const function */
  int class_scope;
  DiaListStore *parameters; /**< List of DiaUmlParameter */
  GList *parameters_hack;

  ConnectionPoint* l_connection; /**< left */
  ConnectionPoint* r_connection; /**< right */

  gboolean needs_wrapping; /** Whether this operation needs wrapping */
  gint wrap_indent; /** The amount of indentation in chars */
  GList *wrappos; /** Absolute wrapping positions */
  double ascent; /** The ascent amount used for line distance in wrapping */
};

DiaUmlOperation *dia_uml_operation_new                      (void);
/** calculated the 'formated' representation */
gchar           *dia_uml_operation_format                   (DiaUmlOperation *operation);
/* TODO: Why */
DiaUmlOperation *dia_uml_operation_copy                     (DiaUmlOperation *op);
void             dia_uml_operation_ensure_connection_points (DiaUmlOperation *oper,
                                                             DiaObject       *obj);
void             dia_uml_operation_connection_thing         (DiaUmlOperation *self,
                                                             DiaUmlOperation *from);
void             dia_uml_operation_insert_parameter         (DiaUmlOperation *self,
                                                             DiaUmlParameter *parameter,
                                                             int              index);
void             dia_uml_operation_remove_parameter         (DiaUmlOperation *self,
                                                             DiaUmlParameter *parameter);
GListModel      *dia_uml_operation_get_parameters           (DiaUmlOperation *self);

void uml_operation_write(AttributeNode attr_node, DiaUmlOperation *op, DiaContext *ctx);


#endif