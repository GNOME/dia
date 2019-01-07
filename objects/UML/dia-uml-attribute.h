#include <glib-object.h>
#include "uml.h"

#ifndef UML_ATTR_H
#define UML_ATTR_H

#define DIA_UML_TYPE_ATTRIBUTE (dia_uml_attribute_get_type ())
G_DECLARE_FINAL_TYPE (DiaUmlAttribute, dia_uml_attribute, DIA_UML, ATTRIBUTE, GObject)

/** \brief A list of DiaUmlAttribute is contained in UMLClass
 * Some would call them member variables ;)
 */
struct _DiaUmlAttribute {
  GObject parent;
  gint internal_id; /**< Arbitrary integer to recognize attributes after 
		     * the user has shuffled them in the dialog. */
  gchar *name; /**< the member variables name */
  gchar *type; /**< the return value */
  gchar *value; /**< default parameter : Can be NULL => No default value */
  gchar *comment; /**< comment */
  UMLVisibility visibility; /**< attributes visibility */
  int abstract; /**< not sure if this applicable */
  int class_scope; /**< in C++ : static member */
  
  ConnectionPoint* left_connection; /**< left */
  ConnectionPoint* right_connection; /**< right */
};

DiaUmlAttribute *dia_uml_attribute_new                      (void);
/** calculated the 'formated' representation */
gchar           *dia_uml_attribute_format                   (DiaUmlAttribute *attribute);
DiaUmlAttribute *dia_uml_attribute_copy                     (DiaUmlAttribute *attr);
void             dia_uml_attribute_ensure_connection_points (DiaUmlAttribute *attr,
                                                             DiaObject       *obj);

void uml_attribute_write(AttributeNode attr_node, DiaUmlAttribute *attr, DiaContext *ctx);

#endif