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
#include "connectionpoint.h"

typedef struct _UMLAttribute UMLAttribute;
typedef struct _UMLOperation UMLOperation;
typedef struct _UMLParameter UMLParameter;
typedef struct _UMLFormalParameter UMLFormalParameter;

typedef enum _UMLVisibility {
  UML_PUBLIC,
  UML_PRIVATE,
  UML_PROTECTED,
  UML_IMPLEMENTATION /* ?What's this? */
} UMLVisibility;

typedef enum _UMLParameterKind {
  UML_UNDEF_KIND,
  UML_IN,
  UML_INOUT,
  UML_OUT
} UMLParameterKind;

struct _UMLAttribute {
  char *name;
  char *type;
  char *value; /* Can be NULL => No default value */
  UMLVisibility visibility;
  int abstract;    /* Not currently used */
  int class_scope;

  ConnectionPoint* left_connection;
  ConnectionPoint* right_connection;
};

struct _UMLOperation {
  char *name;
  char *type; /* Return type, NULL => No return type */
  UMLVisibility visibility;
  int abstract;
  int class_scope;
  GList *parameters; /* List of UMLParameter */

  ConnectionPoint* left_connection;
  ConnectionPoint* right_connection;
};

struct _UMLParameter {
  char *name;
  char *type;
  char *value; /* Can be NULL => No default value */
  UMLParameterKind kind; /* Not currently used */
};

struct _UMLFormalParameter {
  char *name;
  char *type; /* Can be NULL => Type parameter */
};

/* Characters used to start/end stereotypes: */
#define UML_STEREOTYPE_START ((char) 171)
#define UML_STEREOTYPE_END ((char) 187)

extern char *uml_get_attribute_string(UMLAttribute *attribute);
extern char *uml_get_operation_string(UMLOperation *operation);
extern char *uml_get_parameter_string(UMLParameter *param);
extern char *uml_get_formalparameter_string(UMLFormalParameter *parameter);
extern UMLAttribute *uml_attribute_copy(UMLAttribute *attr);
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

extern void uml_attribute_write(AttributeNode attr_node, UMLAttribute *attr);
extern void uml_operation_write(AttributeNode attr_node, UMLOperation *op);
extern void uml_formalparameter_write(AttributeNode attr_node, UMLFormalParameter *param);
extern UMLAttribute *uml_attribute_read(DataNode composite);
extern UMLOperation * uml_operation_read(DataNode composite);
extern UMLFormalParameter *uml_formalparameter_read(DataNode composite);

#endif /* UML_H */

