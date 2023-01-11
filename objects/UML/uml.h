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

/** \file objects/UML/uml.h  Objects contained  in 'UML - Class' type also and helper functions */

#pragma once

#include <glib.h>

#include "connectionpoint.h"
#include "dia_xml.h"

G_BEGIN_DECLS

typedef struct _UMLAttribute UMLAttribute;
typedef struct _UMLOperation UMLOperation;
typedef struct _UMLParameter UMLParameter;
typedef struct _UMLFormalParameter UMLFormalParameter;

/** the visibility (allowed acces) of (to) various UML sub elements */
typedef enum /*< enum >*/ {
  DIA_UML_PUBLIC, /**< everyone can use it */
  DIA_UML_PRIVATE, /**< only accessible inside the class itself */
  DIA_UML_PROTECTED, /**< the class and its inheritants ca use this */
  DIA_UML_IMPLEMENTATION /**< ?What's this? Means implementation decision */
} DiaUmlVisibility;

/** In some languages there are different kinds of class inheritances */
typedef enum /*< enum >*/ {
  DIA_UML_ABSTRACT, /**< Pure virtual method: an object of this class cannot be instanciated */
  DIA_UML_POLYMORPHIC, /**< Virtual method : could be reimplemented in derivated classes */
  DIA_UML_LEAF /**< Final method: can't be redefined in subclasses */
} DiaUmlInheritanceType;

/** describes the data flow between caller and callee */
typedef enum /*< enum >*/ {
  DIA_UML_UNDEF_KIND, /**< not defined */
  DIA_UML_IN, /**< by value */
  DIA_UML_OUT, /**< by ref, can be passed in uninitialized */
  DIA_UML_INOUT /**< by ref */
} DiaUmlParameterKind;

typedef char * UMLStereotype;

/**
 * UMLAttribute:
 *
 * A list of #UMLAttribute is contained in #UMLClass
 * Some would call them member variables ;)
 */
struct _UMLAttribute {
  int internal_id; /**< Arbitrary integer to recognize attributes after
                    * the user has shuffled them in the dialog. */
  char *name; /**< the member variables name */
  char *type; /**< the return value */
  char *value; /**< default parameter : Can be NULL => No default value */
  char *comment; /**< comment */
  DiaUmlVisibility visibility; /**< attributes visibility */
  int abstract; /**< not sure if this applicable */
  int class_scope; /**< in C++ : static member */

  ConnectionPoint *left_connection; /**< left */
  ConnectionPoint *right_connection; /**< right */
};


/**
 * UMLOperation:
 *
 * A list of #UMLOperation is contained in #UMLClass
 * Some would call them member functions ;)
 */
struct _UMLOperation {
  int internal_id; /**< Arbitrary integer to recognize operations after
                     * the user has shuffled them in the dialog. */
  char *name; /**< the function name */
  char *type; /**< Return type, NULL => No return type */
  char *comment; /**< comment */
  UMLStereotype stereotype; /**< just some string */
  DiaUmlVisibility visibility; /**< allowed access */
  DiaUmlInheritanceType inheritance_type;
  int query; /**< Do not modify the object, in C++ this is a const function */
  int class_scope;
  GList *parameters; /**< List of UMLParameter */

  ConnectionPoint* left_connection; /**< left */
  ConnectionPoint* right_connection; /**< right */

  gboolean needs_wrapping; /** Whether this operation needs wrapping */
  int wrap_indent; /** The amount of indentation in chars */
  GList *wrappos; /** Absolute wrapping positions */
  double ascent; /** The ascent amount used for line distance in wrapping */
};


/**
 * UMLParameter:
 *
 * A list of #UMLParameter is contained in #UMLOperation
 * Some would call them functions parameters ;)
 */
struct _UMLParameter {
  char *name; /**<  name*/
  char *type; /**< return value */
  char *value; /**< Initialization,  can be NULL => No default value */
  char *comment; /**< comment */
  DiaUmlParameterKind kind; /**< Not currently used */
};


/**
 * UMLFormalParameter:
 *
 * A list of #UMLFormalParameter is contained in #UMLOperation
 * Some would call them template parameters ;)
 */
struct _UMLFormalParameter {
  char *name; /**< name */
  char *type; /**< Can be NULL => Type parameter */
};

/* Characters used to start/end stereotypes: */
/** start stereotype symbol(like \xab) for local locale */
#define UML_STEREOTYPE_START _("<<")
/** end stereotype symbol(like \xbb) for local locale */
#define UML_STEREOTYPE_END _(">>")



#define DIA_UML_TYPE_PARAMETER uml_parameter_get_type ()

UMLParameter       *uml_parameter_new                      (void);
GType               uml_parameter_get_type                 (void);
UMLParameter       *uml_parameter_copy                     (UMLParameter *param);
UMLParameter       *uml_parameter_ref                      (UMLParameter *param);
void                uml_parameter_unref                    (UMLParameter *param);
/** calculated the 'formated' representation */
gchar              *uml_parameter_get_string               (UMLParameter *param);



#define DIA_UML_TYPE_OPERATION uml_operation_get_type ()

UMLOperation       *uml_operation_new                      (void);
GType               uml_operation_get_type                 (void);
UMLOperation       *uml_operation_copy                     (UMLOperation  *op);
void                uml_operation_copy_into                (UMLOperation  *srcop,
                                                            UMLOperation  *destop);
UMLOperation       *uml_operation_ref                      (UMLOperation  *self);
void                uml_operation_unref                    (UMLOperation  *self);
void                uml_operation_write                    (AttributeNode  attr_node,
                                                            UMLOperation  *op,
                                                            DiaContext    *ctx);
void                uml_operation_ensure_connection_points (UMLOperation  *oper,
                                                            DiaObject     *obj);
/** calculated the 'formated' representation */
char               *uml_get_operation_string               (UMLOperation  *operation);



#define DIA_UML_TYPE_ATTRIBUTE uml_attribute_get_type ()

UMLAttribute       *uml_attribute_new                      (void);
GType               uml_attribute_get_type                 (void);
UMLAttribute       *uml_attribute_copy                     (UMLAttribute  *attr);
void                uml_attribute_copy_into                (UMLAttribute  *srcattr,
                                                            UMLAttribute  *destattr);
UMLAttribute       *uml_attribute_ref                      (UMLAttribute  *attribute);
void                uml_attribute_unref                    (UMLAttribute  *attribute);
void                uml_attribute_write                    (AttributeNode  attr_node,
                                                            UMLAttribute  *attr,
                                                            DiaContext    *ctx);
void                uml_attribute_ensure_connection_points (UMLAttribute  *attr,
                                                            DiaObject     *obj);
/** calculated the 'formated' representation */
char               *uml_attribute_get_string               (UMLAttribute  *attribute);



#define DIA_UML_TYPE_FORMAL_PARAMETER uml_formal_parameter_get_type ()

UMLFormalParameter *uml_formal_parameter_new               (void);
GType               uml_formal_parameter_get_type          (void);
UMLFormalParameter *uml_formal_parameter_copy              (UMLFormalParameter *param);
UMLFormalParameter *uml_formal_parameter_ref               (UMLFormalParameter *param);
void                uml_formal_parameter_unref             (UMLFormalParameter *param);
void                uml_formal_parameter_write             (AttributeNode       attr_node,
                                                            UMLFormalParameter *param,
                                                            DiaContext         *ctx);
/** calculated the 'formated' representation */
char               *uml_formal_parameter_get_string        (UMLFormalParameter *parameter);

G_END_DECLS
