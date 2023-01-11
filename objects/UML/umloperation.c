/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * umloperation.c : refactored from uml.c, class.c to final use StdProps
 *                  PROP_TYPE_DARRAY, a list where each element is a set
 *                  of properies described by the same StdPropDesc
 * Copyright (C) 2005 Hans Breuer
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

/*

classType = dia.get_object_type ("UML - Class")
operType  = dia.get_object_type ("UML - Operation")
paramType = dia.get_object_type ("UML - Parameter")
for c in theClasses :
	klass, h1, h2 = classType.create (0,0) # p.x, p.y
	for f in theFunctions :
		oper, _h1, _h2 = operType.create (0,0)
		oper.properties["name"] = f.name
		oper.properties["type"] = f.type

		for p in f.parameters :
			param, _h1, _h2 = paramType.create(0,0)
			param.properties["name"] = p.name
			param.properties["type"] = p.type

			oper.insert(param, -1)
		klass.insert(oper, -1)
	layer.add_object(klass)

 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "uml.h"
#include "properties.h"

extern PropEnumData _uml_visibilities[];
extern PropEnumData _uml_inheritances[];

static PropDescription umloperation_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Type"), NULL, NULL },
  { "comment", PROP_TYPE_MULTISTRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Stereotype"), NULL, NULL },
  /* visibility: public, protected, private (other languages?) */
  { "visibility", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visibility"), NULL, _uml_visibilities },
  { "inheritance_type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Inheritance"), NULL, _uml_inheritances },
  { "query", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Query"), NULL, N_("C++ const method") },
  { "class_scope", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Scope"), NULL, N_("Class scope (C++ static method)") },
  { "parameters", PROP_TYPE_DARRAY, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Parameters"), NULL, NULL },

  PROP_DESC_END
};

static PropOffset umloperation_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(UMLOperation, name) },
  { "type", PROP_TYPE_STRING, offsetof(UMLOperation, type) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(UMLOperation, comment) },
  { "stereotype", PROP_TYPE_STRING, offsetof(UMLOperation, stereotype) },
  { "visibility", PROP_TYPE_ENUM, offsetof(UMLOperation, visibility) },
  { "inheritance_type", PROP_TYPE_ENUM, offsetof(UMLOperation, inheritance_type) },
  { "query", PROP_TYPE_BOOL, offsetof(UMLOperation, query) },
  { "class_scope", PROP_TYPE_BOOL, offsetof(UMLOperation, class_scope) },
  { "parameters", PROP_TYPE_DARRAY, offsetof(UMLOperation, parameters) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umloperation_extra = {
  { umloperation_props, umloperation_offsets, "umloperation" },
  (NewRecordFunc) uml_operation_new,
  (FreeRecordFunc) uml_operation_unref
};

G_DEFINE_BOXED_TYPE (UMLOperation, uml_operation, uml_operation_ref, uml_operation_unref)


UMLOperation *
uml_operation_new (void)
{
  UMLOperation *op;
  static gint next_id = 1;

  op = g_rc_box_new0 (UMLOperation);
  op->internal_id = next_id++;
  op->name = g_strdup ("");
  op->type = g_strdup ("");
  op->comment = g_strdup ("");
  op->visibility = DIA_UML_PUBLIC;
  op->inheritance_type = DIA_UML_LEAF;

#if 0 /* setup elsewhere */
  op->left_connection = g_new0(ConnectionPoint, 1);
  op->right_connection = g_new0(ConnectionPoint, 1);
#endif
  return op;
}


void
uml_operation_copy_into (UMLOperation *srcop, UMLOperation *destop)
{
  UMLParameter *param;
  UMLParameter *newparam;
  GList *list;

  destop->internal_id = srcop->internal_id;

  g_clear_pointer (&destop->name, g_free);
  destop->name = g_strdup (srcop->name);

  g_clear_pointer (&destop->type, g_free);
  destop->type = g_strdup (srcop->type);

  g_clear_pointer (&destop->stereotype, g_free);
  destop->stereotype = g_strdup (srcop->stereotype);

  g_clear_pointer (&destop->comment, g_free);
  destop->comment = g_strdup (srcop->comment);

  destop->visibility = srcop->visibility;
  destop->class_scope = srcop->class_scope;
  destop->inheritance_type = srcop->inheritance_type;
  destop->query = srcop->query;

  g_list_free_full (destop->parameters, (GDestroyNotify) uml_parameter_unref);
  destop->parameters = NULL;

  list = srcop->parameters;
  while (list != NULL) {
    param = (UMLParameter *) list->data;

    // Break the link to the original
    newparam = uml_parameter_copy (param);

    destop->parameters = g_list_append (destop->parameters, newparam);

    list = g_list_next (list);
  }
}


UMLOperation *
uml_operation_copy (UMLOperation *op)
{
  UMLOperation *newop;

  newop = uml_operation_new ();

  uml_operation_copy_into (op, newop);
#if 0 /* setup elsewhere */
  newop->left_connection = g_new0(ConnectionPoint,1);
  *newop->left_connection = *op->left_connection;
  newop->left_connection->object = NULL; /* must be setup later */

  newop->right_connection = g_new0(ConnectionPoint,1);
  *newop->right_connection = *op->right_connection;
  newop->right_connection->object = NULL; /* must be setup later */
#endif
  return newop;
}


UMLOperation *
uml_operation_ref (UMLOperation *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return g_rc_box_acquire (self);
}


static void
operation_free (UMLOperation *op)
{
  g_clear_pointer (&op->name, g_free);
  g_clear_pointer (&op->type, g_free);
  g_clear_pointer (&op->stereotype, g_free);
  g_clear_pointer (&op->comment, g_free);

  g_list_free_full (op->parameters, (GDestroyNotify) uml_parameter_unref);

  if (op->wrappos) {
    g_list_free (op->wrappos);
  }

#if 0 /* freed elsewhere */
  /* These are merely temporary reminders, don't need to unconnect */
  g_clear_pointer (&op->left_connection, g_free);
  g_clear_pointer (&op->right_connection, g_free);
#endif
}


void
uml_operation_unref (UMLOperation *self)
{
  g_rc_box_release_full (self, (GDestroyNotify) operation_free);
}


void
uml_operation_write(AttributeNode attr_node, UMLOperation *op, DiaContext *ctx)
{
  GList *list;
  UMLParameter *param;
  DataNode composite;
  DataNode composite2;
  AttributeNode attr_node2;

  composite = data_add_composite(attr_node, "umloperation", ctx);

  data_add_string(composite_add_attribute(composite, "name"),
		  op->name, ctx);
  data_add_string(composite_add_attribute(composite, "stereotype"),
		  op->stereotype, ctx);
  data_add_string(composite_add_attribute(composite, "type"),
		  op->type, ctx);
  data_add_enum(composite_add_attribute(composite, "visibility"),
		op->visibility, ctx);
  data_add_string(composite_add_attribute(composite, "comment"),
		  op->comment, ctx);
  /* Backward compatibility */
  data_add_boolean(composite_add_attribute(composite, "abstract"),
		   op->inheritance_type == DIA_UML_ABSTRACT, ctx);
  data_add_enum(composite_add_attribute(composite, "inheritance_type"),
		op->inheritance_type, ctx);
  data_add_boolean(composite_add_attribute(composite, "query"),
		   op->query, ctx);
  data_add_boolean(composite_add_attribute(composite, "class_scope"),
		   op->class_scope, ctx);

  attr_node2 = composite_add_attribute(composite, "parameters");

  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *) list->data;

    composite2 = data_add_composite(attr_node2, "umlparameter", ctx);

    data_add_string(composite_add_attribute(composite2, "name"),
		    param->name, ctx);
    data_add_string(composite_add_attribute(composite2, "type"),
		    param->type, ctx);
    data_add_string(composite_add_attribute(composite2, "value"),
		    param->value, ctx);
    data_add_string(composite_add_attribute(composite2, "comment"),
		    param->comment, ctx);
    data_add_enum(composite_add_attribute(composite2, "kind"),
		  param->kind, ctx);
    list = g_list_next(list);
  }
}

extern char visible_char[];

char *
uml_get_operation_string (UMLOperation *operation)
{
  int len;
  char *str;
  GList *list;
  UMLParameter *param;

  /* Calculate length: */
  len = 1 + (operation->name ? strlen (operation->name) : 0) + 1;
  if(operation->stereotype != NULL && operation->stereotype[0] != '\0') {
    len += 5 + strlen (operation->stereotype);
  }

  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
    list = g_list_next (list);

    switch (param->kind) {
      case DIA_UML_IN:
        len += 3;
        break;
      case DIA_UML_OUT:
        len += 4;
        break;
      case DIA_UML_INOUT:
        len += 6;
        break;
      case DIA_UML_UNDEF_KIND:
      default:
        break;
      }
    len += (param->name ? strlen (param->name) : 0);
    if (param->type != NULL) {
      len += strlen (param->type);
      if (param->type[0] && (param->name != NULL && param->name[0])) {
        len += 1;
      }
    }
    if (param->value != NULL && param->value[0] != '\0') {
      len += 1 + strlen (param->value);
    }

    if (list != NULL) {
      len += 1; /* ',' */
    }
  }

  len += 1; /* ')' */
  if (operation->type != NULL && operation->type[0]) {
    len += 2 + strlen (operation->type);
  }
  if(operation->query != 0) {
    len += 6;
  }

  /* generate string: */
  str = g_new0 (char, len + 1);

  str[0] = visible_char[(int) operation->visibility];
  str[1] = 0;

  if(operation->stereotype != NULL && operation->stereotype[0] != '\0') {
    strcat(str, UML_STEREOTYPE_START);
    strcat(str, operation->stereotype);
    strcat(str, UML_STEREOTYPE_END);
    strcat(str, " ");
  }

  strcat (str, operation->name ? operation->name : "");
  strcat (str, "(");

  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
    list = g_list_next (list);

    switch (param->kind) {
      case DIA_UML_IN:
        strcat (str, "in ");
        break;
      case DIA_UML_OUT:
        strcat (str, "out ");
        break;
      case DIA_UML_INOUT:
        strcat (str, "inout ");
        break;
      case DIA_UML_UNDEF_KIND:
      default:
        break;
    }
    strcat (str, param->name ? param->name : "");

    if (param->type != NULL) {
      if (param->type[0] && (param->name != NULL && param->name[0])) {
        strcat (str, ":");
      }
      strcat (str, param->type);
    }

    if (param->value != NULL && param->value[0] != '\0') {
      strcat (str, "=");
      strcat (str, param->value);
    }

    if (list != NULL) {
      strcat (str, ",");
    }
  }
  strcat (str, ")");

  if (operation->type != NULL &&
      operation->type[0]) {
    strcat (str, ": ");
    strcat (str, operation->type);
  }

  if (operation->query != 0) {
    strcat(str, " const");
  }

  g_assert (strlen (str) == len);

  return str;
}

/*!
 * The ownership of these connection points is quite complicated. Instead of being part of
 * the UMLOperation as one may expect at first, they are somewhat in between the DiaObject
 * (see: DiaObject::connections and the concrete user, here UMLClass) and the UMLOperation.
 *
 * But with taking undo state mangement into account it gets even worse. Deleted (to be
 * restored connection points) live inside the UMLClassChange until they get reverted back
 * to the object *or* get free'd by umlclass_change_free()
 *
 * Since the implementation of attributes/operations being settable via StdProps there are
 * more places to keep this stuff consitent. So here comes a tolerant helper.
 *
 * NOTE: Same function as uml_attribute_ensure_connection_points(),
 * with C++ it would be a template function ;)
 */
void
uml_operation_ensure_connection_points (UMLOperation* op, DiaObject* obj)
{
  if (!op->left_connection)
    op->left_connection = g_new0(ConnectionPoint,1);
  op->left_connection->object = obj;
  if (!op->right_connection)
    op->right_connection = g_new0(ConnectionPoint,1);
  op->right_connection->object = obj;
}
