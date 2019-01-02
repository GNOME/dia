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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <math.h>
#include <string.h>

#include "uml.h"
#include "properties.h"
#include "dia-uml-operation.h"
#include "editor/dia-uml-list-data.h"

static void
dia_uml_operation_list_data_init (DiaUmlListDataInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DiaUmlOperation, dia_uml_operation, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (DIA_UML_TYPE_LIST_DATA,
                                                dia_uml_operation_list_data_init))

enum {
  UML_OP_NAME = 1,
  UML_OP_TYPE,
  UML_OP_COMMENT,
  UML_OP_STEREOTYPE,
  UML_OP_VISIBILITY,
  UML_OP_INHERITANCE_TYPE,
  UML_OP_QUERY,
  UML_OP_CLASS_SCOPE,
  UML_OP_N_PROPS
};
static GParamSpec* uml_op_properties[UML_OP_N_PROPS];

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
  { "name", PROP_TYPE_STRING, offsetof(DiaUmlOperation, name) },
  { "type", PROP_TYPE_STRING, offsetof(DiaUmlOperation, type) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(DiaUmlOperation, comment) },
  { "stereotype", PROP_TYPE_STRING, offsetof(DiaUmlOperation, stereotype) },
  { "visibility", PROP_TYPE_ENUM, offsetof(DiaUmlOperation, visibility) },
  { "inheritance_type", PROP_TYPE_ENUM, offsetof(DiaUmlOperation, inheritance_type) },
  { "query", PROP_TYPE_BOOL, offsetof(DiaUmlOperation, query) },
  { "class_scope", PROP_TYPE_BOOL, offsetof(DiaUmlOperation, class_scope) },
  { "parameters", PROP_TYPE_DARRAY, offsetof(DiaUmlOperation, parameters) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umloperation_extra = {
  { umloperation_props, umloperation_offsets, "umloperation" },
  (NewRecordFunc) dia_uml_operation_new,
  (FreeRecordFunc) g_object_unref
};

DiaUmlOperation *
dia_uml_operation_copy (DiaUmlOperation *srcop)
{
  DiaUmlOperation *destop;
  DiaUmlParameter *param;
  DiaUmlParameter *newparam;
  GList *list;
  
  destop = g_object_new (DIA_UML_TYPE_OPERATION, NULL);

  destop->internal_id = srcop->internal_id;

  if (destop->name != NULL) {
    g_free(destop->name);
  }
  destop->name = g_strdup(srcop->name);
  
  if (destop->type != NULL) {
    g_free(destop->type);
  }
  if (srcop->type != NULL) {
    destop->type = g_strdup(srcop->type);
  } else {
    destop->type = NULL;
  }

  if (destop->stereotype != NULL) {
    g_free(destop->stereotype);
  }
  if(srcop->stereotype != NULL) {
    destop->stereotype = g_strdup(srcop->stereotype);
  } else {
    destop->stereotype = NULL;
  }
  
  if (destop->comment != NULL) {
    g_free(destop->comment);
  }
  if (srcop->comment != NULL) {
    destop->comment = g_strdup(srcop->comment);
  } else {
    destop->comment = NULL;
  }

  destop->visibility = srcop->visibility;
  destop->class_scope = srcop->class_scope;
  destop->inheritance_type = srcop->inheritance_type;
  destop->query = srcop->query;

  list = destop->parameters;
  while (list != NULL) {
    param = (DiaUmlParameter *)list->data;
    dia_uml_operation_remove_parameter (destop, param);
    list = g_list_next (list);
  }
  destop->parameters = NULL;
  list = srcop->parameters;
  while (list != NULL) {
    param = (DiaUmlParameter *)list->data;

    newparam = dia_uml_parameter_new ();
    newparam->name = g_strdup(param->name);
    newparam->type = g_strdup(param->type);
    newparam->comment = g_strdup(param->comment);
    newparam->value = g_strdup(param->value);
    newparam->kind = param->kind;

    dia_uml_operation_insert_parameter (destop, newparam, -1);
    
    list = g_list_next(list);
  }

  return destop;

#if 0 /* setup elsewhere */
  newop->left_connection = g_new0(ConnectionPoint,1);
  *newop->left_connection = *op->left_connection;
  newop->left_connection->object = NULL; /* must be setup later */

  newop->right_connection = g_new0(ConnectionPoint,1);
  *newop->right_connection = *op->right_connection;
  newop->right_connection->object = NULL; /* must be setup later */
#endif
}

void
uml_operation_write(AttributeNode attr_node, DiaUmlOperation *op, DiaContext *ctx)
{
  GList *list;
  DiaUmlParameter *param;
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
		   op->inheritance_type == UML_ABSTRACT, ctx);
  data_add_enum(composite_add_attribute(composite, "inheritance_type"),
		op->inheritance_type, ctx);
  data_add_boolean(composite_add_attribute(composite, "query"),
		   op->query, ctx);
  data_add_boolean(composite_add_attribute(composite, "class_scope"),
		   op->class_scope, ctx);
  
  attr_node2 = composite_add_attribute(composite, "parameters");
  
  list = op->parameters;
  while (list != NULL) {
    param = (DiaUmlParameter *) list->data;

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
dia_uml_operation_format (DiaUmlOperation *operation)
{
  int len;
  char *str;
  GList *list;
  DiaUmlParameter *param;

  /* Calculate length: */
  len = 1 + (operation->name ? strlen (operation->name) : 0) + 1;
  if(operation->stereotype != NULL && operation->stereotype[0] != '\0') {
    len += 5 + strlen (operation->stereotype);
  }   
  
  list = operation->parameters;
  while (list != NULL) {
    param = (DiaUmlParameter  *) list->data;
    list = g_list_next (list);
    
    switch(param->kind)
      {
      case UML_UNDEF_KIND:
	break;  
      case UML_IN:
	len += 3;
	break;
      case UML_OUT:
	len += 4;
	break;
      case UML_INOUT:
	len += 6;
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
  str = g_malloc (sizeof (char) * (len + 1));

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
    param = (DiaUmlParameter  *) list->data;
    list = g_list_next (list);
    
    switch(param->kind)
      {
      case UML_UNDEF_KIND:
	break;
      case UML_IN:
	strcat (str, "in ");
	break;
      case UML_OUT:
	strcat (str, "out ");
	break;
      case UML_INOUT:
	strcat (str, "inout ");
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

  g_assert_cmpint (strlen (str), ==, len);
  
  return str;
}

/*!
 * The ownership of these connection points is quite complicated. Instead of being part of
 * the DiaUmlOperation as one may expect at first, they are somewhat in between the DiaObject
 * (see: DiaObject::connections and the concrete user, here UMLClass) and the DiaUmlOperation.
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
dia_uml_operation_ensure_connection_points (DiaUmlOperation* op, DiaObject* obj)
{
  if (!op->l_connection)
    op->l_connection = g_new0(ConnectionPoint,1);
  op->l_connection->object = obj;
  if (!op->r_connection)
    op->r_connection = g_new0(ConnectionPoint,1);
  op->r_connection->object = obj;
}

static void
dia_uml_operation_finalize (GObject *object)
{
  DiaUmlOperation *self = DIA_UML_OPERATION (object);

  g_free (self->name);
  g_free (self->type);
  g_free (self->stereotype);
  g_free (self->comment);

  g_list_free_full (self->parameters, g_object_unref);
  if (self->wrappos) {
    g_list_free (self->wrappos);
  }

  /* freed elsewhere */
  /* These are merely temporary reminders, don't need to unconnect */
  /*
  g_free(op->left_connection);
  g_free(op->right_connection);
  */
}

static void
dia_uml_operation_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DiaUmlOperation *self = DIA_UML_OPERATION (object);

  switch (property_id) {
    case UML_OP_NAME:
      self->name = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_NAME]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_TYPE:
      self->type = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_TYPE]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_COMMENT:
      self->comment = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_COMMENT]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_STEREOTYPE:
      self->stereotype = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_STEREOTYPE]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_VISIBILITY:
      self->visibility = g_value_get_int (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_VISIBILITY]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_INHERITANCE_TYPE:
      self->inheritance_type = g_value_get_int (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_INHERITANCE_TYPE]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_QUERY:
      self->query = g_value_get_boolean (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_QUERY]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    case UML_OP_CLASS_SCOPE:
      self->class_scope = g_value_get_boolean (value);
      g_object_notify_by_pspec (object, uml_op_properties[UML_OP_CLASS_SCOPE]);
      dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_operation_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  DiaUmlOperation *self = DIA_UML_OPERATION (object);

  switch (property_id) {
    case UML_OP_NAME:
      g_value_set_string (value, self->name);
      break;
    case UML_OP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case UML_OP_COMMENT:
      g_value_set_string (value, self->comment);
      break;
    case UML_OP_STEREOTYPE:
      g_value_set_string (value, self->stereotype);
      break;
    case UML_OP_VISIBILITY:
      g_value_set_int (value, self->visibility);
      break;
    case UML_OP_INHERITANCE_TYPE:
      g_value_set_int (value, self->inheritance_type);
      break;
    case UML_OP_QUERY:
      g_value_set_boolean (value, self->query);
      break;
    case UML_OP_CLASS_SCOPE:
      g_value_set_boolean (value, self->class_scope);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static const gchar *
format (DiaUmlListData *self)
{
  return dia_uml_operation_format (DIA_UML_OPERATION (self));
}

static void
dia_uml_operation_list_data_init (DiaUmlListDataInterface *iface)
{
  iface->format = format;
}

static void
dia_uml_operation_class_init (DiaUmlOperationClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_operation_finalize;
  object_class->set_property = dia_uml_operation_set_property;
  object_class->get_property = dia_uml_operation_get_property;

  uml_op_properties[UML_OP_NAME] = g_param_spec_string ("name",
                                                        "Name",
                                                        "Function name",
                                                        "",
                                                        G_PARAM_READWRITE);
  uml_op_properties[UML_OP_TYPE] = g_param_spec_string ("type",
                                                        "Type",
                                                        "Return type",
                                                        "",
                                                        G_PARAM_READWRITE);
  uml_op_properties[UML_OP_COMMENT] = g_param_spec_string ("comment",
                                                           "Comment",
                                                           "Comment",
                                                           "",
                                                           G_PARAM_READWRITE);
  uml_op_properties[UML_OP_STEREOTYPE] = g_param_spec_string ("stereotype",
                                                              "Stereotype",
                                                              "Stereotype",
                                                              "",
                                                              G_PARAM_READWRITE);
  uml_op_properties[UML_OP_VISIBILITY] = g_param_spec_int ("visibility",
                                                           "Visibility",
                                                           "Visibility",
                                                           UML_PUBLIC,
                                                           UML_IMPLEMENTATION,
                                                           UML_PUBLIC,
                                                           G_PARAM_READWRITE);
  uml_op_properties[UML_OP_INHERITANCE_TYPE] = g_param_spec_int ("inheritance-type",
                                                                 "Inheritance type",
                                                                 "Inheritance type",
                                                                 UML_ABSTRACT,
                                                                 UML_LEAF,
                                                                 UML_LEAF,
                                                                 G_PARAM_READWRITE);
  uml_op_properties[UML_OP_QUERY] = g_param_spec_boolean ("query",
                                                          "Query",
                                                          "Const function",
                                                          FALSE,
                                                          G_PARAM_READWRITE);
  uml_op_properties[UML_OP_CLASS_SCOPE] = g_param_spec_boolean ("class-scope",
                                                                "Class scope",
                                                                "Class scope",
                                                                FALSE,
                                                                G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     UML_OP_N_PROPS,
                                     uml_op_properties);
}

static void
dia_uml_operation_init (DiaUmlOperation *self)
{
  static gint next_id = 1;

  self->internal_id = next_id++;

  self->name = g_strdup("");
  self->comment = g_strdup("");
  self->type = g_strdup("");
  self->stereotype = g_strdup("");
  self->visibility = UML_PUBLIC;
  self->inheritance_type = UML_LEAF;
}

DiaUmlOperation *
dia_uml_operation_new ()
{
  return g_object_new (DIA_UML_TYPE_OPERATION, NULL);
}

static void
bubble (DiaUmlParameter *para,
        DiaUmlOperation *self)
{
  dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
}

void
dia_uml_operation_insert_parameter (DiaUmlOperation *self,
                                    DiaUmlParameter *parameter,
                                    int              index)
{
  self->parameters = g_list_insert (self->parameters,
                                    g_object_ref (parameter),
                                    index);
  g_signal_connect (G_OBJECT (parameter), "changed", G_CALLBACK (bubble), self);
  dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
}

void
dia_uml_operation_remove_parameter (DiaUmlOperation *self,
                                    DiaUmlParameter *parameter)
{
  self->parameters = g_list_remove (self->parameters, parameter);
  g_signal_handlers_disconnect_by_func (G_OBJECT (parameter), G_CALLBACK (bubble), self);
  g_object_unref (parameter);
  dia_uml_list_data_changed (DIA_UML_LIST_DATA (self));
}

GList *
dia_uml_operation_get_parameters (DiaUmlOperation *self)
{
  return self->parameters;
}
