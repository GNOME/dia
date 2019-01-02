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

#ifndef UML_H
#define UML_H

#include <glib.h>
#include <gtk/gtk.h>
#include "intl.h"
#include "connectionpoint.h"
#include "dia_xml.h"

/* TODO: enums as GEnum for _spec_enum ext */

/** the visibility (allowed acces) of (to) various UML sub elements */
typedef enum _UMLVisibility {
  UML_PUBLIC, /**< everyone can use it */
  UML_PRIVATE, /**< only accessible inside the class itself */
  UML_PROTECTED, /**< the class and its inheritants ca use this */
  UML_IMPLEMENTATION /**< ?What's this? Means implementation decision */
} UMLVisibility;

typedef struct _UMLFormalParameter UMLFormalParameter;

/** \brief A list of UMLFormalParameter is contained in DiaUmlOperation
 * Some would call them template parameters ;)
 */
struct _UMLFormalParameter {
  gchar *name; /**< name */
  gchar *type; /**< Can be NULL => Type parameter */
};

/* Characters used to start/end stereotypes: */
/** start stereotype symbol(like \xab) for local locale */
#define UML_STEREOTYPE_START _("<<")
/** end stereotype symbol(like \xbb) for local locale */
#define UML_STEREOTYPE_END _(">>")

/** calculated the 'formated' representation */
gchar *uml_get_formalparameter_string(UMLFormalParameter *parameter);
UMLFormalParameter *uml_formalparameter_copy(UMLFormalParameter *param);
void uml_formalparameter_destroy(UMLFormalParameter *param);

UMLFormalParameter *uml_formalparameter_new(void);

void uml_formalparameter_write(AttributeNode attr_node, UMLFormalParameter *param, DiaContext *ctx);

void list_box_separators (GtkListBoxRow *row,
                          GtkListBoxRow *before,
                          gpointer       user_data);

#endif /* UML_H */

