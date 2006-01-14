/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef UML_H
#define UML_H

#include <glib.h>
#include "intl.h"
#include "connectionpoint.h"

typedef struct _UMLAttribute UMLAttribute;
typedef struct _UMLOperation UMLOperation;
typedef struct _UMLParameter UMLParameter;
typedef struct _UMLFormalParameter UMLFormalParameter;

typedef enum _UMLVisibility {
  UML_PUBLIC,
  UML_PRIVATE,
  UML_PROTECTED,
  UML_IMPLEMENTATION /* ?What's this? Means implementation decision */
} UMLVisibility;

typedef enum _UMLInheritanceType {
  UML_ABSTRACT, /* Pure virtual method: an object of this class cannot be instanciated */
  UML_POLYMORPHIC, /* Virtual method : could be reimplemented in derivated classes */
  UML_LEAF /* Final method: can't be redefined in subclasses */
} UMLInheritanceType;

typedef enum _UMLParameterKind {
  UML_UNDEF_KIND,
  UML_IN,
  UML_OUT,
  UML_INOUT
} UMLParameterKind;

typedef gchar * UMLStereotype;

struct _UMLAttribute {
  gint internal_id; /* Arbitrary integer to recognize attributes after 
		     * the user has shuffled them in the dialog. */
  gchar *name;
  gchar *type;
  gchar *value; /* Can be NULL => No default value */
  gchar *comment;
  UMLVisibility visibility;
  int abstract;
  int class_scope;
  
  ConnectionPoint* left_connection;
  ConnectionPoint* right_connection;
};

struct _UMLOperation {
  gint internal_id; /* Arbitrary integer to recognize operations after
		     * the user has shuffled them in the dialog. */
  gchar *name;
  gchar *type; /* Return type, NULL => No return type */
  gchar *comment;   
  UMLStereotype stereotype;
  UMLVisibility visibility;
  UMLInheritanceType inheritance_type;
  int query; /* Do not modify the object */
  int class_scope;
  GList *parameters; /* List of UMLParameter */

  ConnectionPoint* left_connection;
  ConnectionPoint* right_connection;
};

struct _UMLParameter {
  gchar *name;
  gchar *type;
  gchar *value; /* Can be NULL => No default value */
  gchar *comment;
  UMLParameterKind kind; /* Not currently used */
};

struct _UMLFormalParameter {
  gchar *name;
  gchar *type; /* Can be NULL => Type parameter */
};

/* Characters used to start/end stereotypes: */
/* start stereotype symbol(like \xab) for local locale */
#define UML_STEREOTYPE_START _("<<")
/* end stereotype symbol(like \xbb) for local locale */
#define UML_STEREOTYPE_END _(">>")

extern gchar *uml_get_attribute_string (UMLAttribute *attribute);
extern gchar *uml_get_operation_string(UMLOperation *operation);
extern gchar *uml_get_parameter_string(UMLParameter *param);
extern gchar *uml_get_formalparameter_string(UMLFormalParameter *parameter);
extern void uml_attribute_copy_into(UMLAttribute *srcattr, UMLAttribute *destattr);
extern UMLAttribute *uml_attribute_copy(UMLAttribute *attr);
extern void uml_operation_copy_into(UMLOperation *srcop, UMLOperation *destop);
extern UMLOperation *uml_operation_copy(UMLOperation *op);
extern UMLFormalParameter *uml_formalparameter_copy(UMLFormalParameter *param);
extern void uml_attribute_destroy(UMLAttribute *attribute);
extern void uml_operation_destroy(UMLOperation *op);
extern void uml_parameter_destroy(UMLParameter *param);
extern void uml_formalparameter_destroy(UMLFormalParameter *param);
extern UMLAttribute *uml_attribute_new(void);
extern UMLOperation *uml_operation_new(void);
extern UMLParameter *uml_parameter_new(void);
extern UMLFormalParameter *uml_formalparameter_new(void);

extern void uml_attribute_ensure_connection_points (UMLAttribute *attr, DiaObject* obj);
extern void uml_operation_ensure_connection_points (UMLOperation *oper, DiaObject* obj);

extern void uml_attribute_write(AttributeNode attr_node, UMLAttribute *attr);
extern void uml_operation_write(AttributeNode attr_node, UMLOperation *op);
extern void uml_formalparameter_write(AttributeNode attr_node, UMLFormalParameter *param);

#endif /* UML_H */

