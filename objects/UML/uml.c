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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "intl.h"
#include "object.h"
#include "uml.h"
#include "plug-ins.h"

extern ObjectType umlclass_type;
extern ObjectType note_type;
extern ObjectType dependency_type;
extern ObjectType realizes_type;
extern ObjectType generalization_type;
extern ObjectType association_type;
extern ObjectType implements_type;
extern ObjectType constraint_type;
extern ObjectType smallpackage_type;
extern ObjectType largepackage_type;
extern ObjectType actor_type;
extern ObjectType usecase_type;
extern ObjectType lifeline_type;
extern ObjectType objet_type;
extern ObjectType umlobject_type;
extern ObjectType message_type;
extern ObjectType component_type;
extern ObjectType classicon_type;
extern ObjectType state_type;
extern ObjectType node_type;
extern ObjectType branch_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "UML",
			    _("Unified Modelling Language diagram objects"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&umlclass_type);
  object_register_type(&note_type);
  object_register_type(&dependency_type);
  object_register_type(&realizes_type);
  object_register_type(&generalization_type);
  object_register_type(&association_type);
  object_register_type(&implements_type);
  object_register_type(&constraint_type);
  object_register_type(&smallpackage_type);
  object_register_type(&largepackage_type);
  object_register_type(&actor_type);
  object_register_type(&usecase_type);
  object_register_type(&lifeline_type);
  object_register_type(&objet_type);
  object_register_type(&umlobject_type);
  object_register_type(&message_type);  
  object_register_type(&component_type);
  object_register_type(&classicon_type);
  object_register_type(&state_type);    
  object_register_type(&node_type);    
  object_register_type(&branch_type);    

  return DIA_PLUGIN_INIT_OK;
}

/* Warning, the following *must* be strictly ASCII characters (or fix the 
   following code for UTF-8 cleanliness */

utfchar visible_char[] = { '+', '-', '#', ' ' };

utfchar *
uml_get_attribute_string (UMLAttribute *attribute)
{
  int len;
  utfchar *str;

  len = 1 + strlen (attribute->name) + strlen (attribute->type);
  if (attribute->name[0] && attribute->type[0]) {
    len += 2;
  }
  if (attribute->value != NULL) {
    len += 3 + strlen (attribute->value);
  }
  
  str = g_malloc (sizeof (utfchar) * (len + 1));

  str[0] = visible_char[(int) attribute->visibility];
  str[1] = 0;

  strcat (str, attribute->name);
  if (attribute->name[0] && attribute->type[0]) {
    strcat (str, ": ");
  }
  strcat (str, attribute->type);
  if (attribute->value != NULL) {
    strcat (str, " = ");
    strcat (str, attribute->value);
  }
    
  assert (strlen (str) == len);

  return str;
}

utfchar *
uml_get_operation_string (UMLOperation *operation)
{
  int len;
  utfchar *str;
  GList *list;
  UMLParameter *param;

  /* Calculate length: */
  len = 1 + strlen (operation->name)  + 1;
  if(operation->stereotype != NULL) {
    len += 5 + strlen (operation->stereotype);
  }   
  
  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
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
    len += strlen (param->name) + strlen (param->type);
    if (param->type[0] && param->name[0]) {
      len += 1;
    }
    if (param->value != NULL) {
      len += 1 + strlen (param->value);
    }
    
    if (list != NULL) {
      len += 1; /* ',' */
    }
  }

  len += 1; /* ')' */
  if (operation->type != NULL) {
    len += 2 + strlen (operation->type);
  }
  if(operation->query != 0) {
    len += 6;
  }

  /* generate string: */
  str = g_malloc (sizeof (utfchar) * (len + 1));

  str[0] = visible_char[(int) operation->visibility];
  str[1] = 0;

  if(operation->stereotype != NULL) {
    strcat(str, UML_STEREOTYPE_START);
    strcat(str, operation->stereotype);
    strcat(str, UML_STEREOTYPE_END);
    strcat(str, " ");
  }

  strcat (str, operation->name);
  strcat (str, "(");
  
  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
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
    strcat (str, param->name);
    if (param->type[0] && param->name[0]) {
      strcat (str, ":");
    }
    strcat (str, param->type);
    
    if (param->value != NULL) {
      strcat (str, "=");
      strcat (str, param->value);
    }

    if (list != NULL) {
      strcat (str, ",");
    }
  }
  strcat (str, ")");

  if (operation->type != NULL) {
    strcat (str, ": ");
    strcat (str, operation->type);
  }
 
  if (operation->query != 0) {
    strcat(str, " const");
  }

  assert (strlen (str) == len);
  
  return str;
}

utfchar *
uml_get_parameter_string (UMLParameter *param)
{
  int len;
  utfchar *str;

  /* Calculate length: */
  len = strlen (param->name) + 1 + strlen (param->type);
  
  if (param->value != NULL) {
    len += 1 + strlen (param->value) ;
  }

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

  /* Generate string: */
  str = g_malloc (sizeof (utfchar) * (len + 1));

  strcpy(str, "");

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
  

  strcat (str, param->name);
  strcat (str, ":");
  strcat (str, param->type);
  if (param->value != NULL) {
    strcat (str, "=");
    strcat (str, param->value);
  }
  
  assert (strlen (str) == len);

  return str;
}

utfchar *
uml_get_formalparameter_string (UMLFormalParameter *parameter)
{
  int len;
  utfchar *str;

  /* Calculate length: */
  len = strlen (parameter->name);
  
  if (parameter->type != NULL) {
    len += 1 + strlen (parameter->type);
  }

  /* Generate string: */
  str = g_malloc (sizeof (utfchar) * (len + 1));
  strcpy (str, parameter->name);
  if (parameter->type != NULL) {
    strcat (str, ":");
    strcat (str, parameter->type);
  }

  assert (strlen (str) == len);

  return str;
}


UMLAttribute *
uml_attribute_copy(UMLAttribute *attr)
{
  UMLAttribute *newattr;

  newattr = g_new0(UMLAttribute, 1);
  newattr->name = g_strdup(attr->name);
  newattr->type = g_strdup(attr->type);
  if (attr->value != NULL) {
    newattr->value = g_strdup(attr->value);
  } else {
    newattr->value = NULL;
  }
  newattr->visibility = attr->visibility;
  newattr->abstract = attr->abstract;
  newattr->class_scope = attr->class_scope;

  newattr->left_connection = attr->left_connection;
  newattr->right_connection = attr->right_connection;
  
  return newattr;
}

UMLOperation *
uml_operation_copy(UMLOperation *op)
{
  UMLOperation *newop;
  UMLParameter *param;
  UMLParameter *newparam;
  GList *list;
  
  newop = g_new0(UMLOperation, 1);
  newop->name = g_strdup(op->name);
  if (op->type != NULL) {
    newop->type = g_strdup(op->type);
  } else {
    newop->type = NULL;
  }
  if(op->stereotype != NULL) {
    newop->stereotype = g_strdup(op->stereotype);
  } else {
    newop->stereotype = NULL;
  }
    
  newop->visibility = op->visibility;
  newop->class_scope = op->class_scope;
  newop->inheritance_type = op->inheritance_type;
  newop->query = op->query;


  newop->left_connection = op->left_connection;
  newop->right_connection = op->right_connection;
  
  newop->parameters = NULL;
  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;

    newparam = g_new0(UMLParameter, 1);
    newparam->name = g_strdup(param->name);
    newparam->type = g_strdup(param->type);
    if (param->value != NULL)
      newparam->value = g_strdup(param->value);
    else
      newparam->value = NULL;
    newparam->kind = param->kind;
    
    newop->parameters = g_list_append(newop->parameters, newparam);
    
    list = g_list_next(list);
  }

  return newop;
}

UMLFormalParameter *
uml_formalparameter_copy(UMLFormalParameter *param)
{
  UMLFormalParameter *newparam;

  newparam = g_new0(UMLFormalParameter, 1);

  newparam->name = g_strdup(param->name);
  if (param->type != NULL) {
    newparam->type = g_strdup(param->type);
  } else {
    newparam->type = NULL;
  }
  
  return newparam;
}

void
uml_attribute_destroy(UMLAttribute *attr)
{
  g_free(attr->name);
  g_free(attr->type);
  if (attr->value != NULL)
    g_free(attr->value);
  g_free(attr);
}

void
uml_operation_destroy(UMLOperation *op)
{
  GList *list;
  UMLParameter *param;
  
  g_free(op->name);
  if (op->type != NULL)
    g_free(op->type);
  if (op->stereotype != NULL)
    g_free(op->stereotype);

  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;
    uml_parameter_destroy(param);
    list = g_list_next(list);
  }
  g_free(op);
}

void
uml_parameter_destroy(UMLParameter *param)
{
  g_free(param->name);
  g_free(param->type);
  if (param->value != NULL) 
    g_free(param->value);
  g_free(param);
}

void
uml_formalparameter_destroy(UMLFormalParameter *param)
{
  g_free(param->name);
  if (param->type != NULL)
    g_free(param->type);
  g_free(param);
}

UMLAttribute *
uml_attribute_new(void)
{
  UMLAttribute *attr;
  
  attr = g_new0(UMLAttribute, 1);
  attr->name = g_strdup("");
  attr->type = g_strdup("");
  attr->value = NULL;
  attr->visibility = UML_PUBLIC;
  attr->abstract = FALSE;
  attr->class_scope = FALSE;
  
  attr->left_connection = NULL;
  attr->right_connection = NULL;
  return attr;
}

UMLOperation *
uml_operation_new(void)
{
  UMLOperation *op;

  op = g_new0(UMLOperation, 1);
  op->name = g_strdup("");
  op->type = NULL;
  op->stereotype = NULL;
  op->visibility = UML_PUBLIC;
  op->class_scope = FALSE;
  op->inheritance_type = UML_POLYMORPHIC;
  op->query = FALSE;

  op->parameters = NULL;

  op->left_connection = NULL;
  op->right_connection = NULL;
  return op;
}

UMLParameter *
uml_parameter_new(void)
{
  UMLParameter *param;

  param = g_new0(UMLParameter, 1);
  param->name = g_strdup("");
  param->type = g_strdup("");
  param->value = NULL;
  param->kind = UML_UNDEF_KIND;

  return param;
}

UMLFormalParameter *
uml_formalparameter_new(void)
{
  UMLFormalParameter *param;

  param = g_new0(UMLFormalParameter, 1);
  param->name = g_strdup("");
  param->type = NULL;

  return param;
}

void
uml_attribute_write(AttributeNode attr_node, UMLAttribute *attr)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "umlattribute");

  data_add_string(composite_add_attribute(composite, "name"),
		  attr->name);
  data_add_string(composite_add_attribute(composite, "type"),
		  attr->type);
  data_add_string(composite_add_attribute(composite, "value"),
		  attr->value);
  data_add_enum(composite_add_attribute(composite, "visibility"),
		attr->visibility);
  data_add_boolean(composite_add_attribute(composite, "abstract"),
		  attr->abstract);
  data_add_boolean(composite_add_attribute(composite, "class_scope"),
		  attr->class_scope);
}

void
uml_operation_write(AttributeNode attr_node, UMLOperation *op)
{
  GList *list;
  UMLParameter *param;
  DataNode composite;
  DataNode composite2;
  AttributeNode attr_node2;

  composite = data_add_composite(attr_node, "umloperation");

  data_add_string(composite_add_attribute(composite, "name"),
		  op->name);
  data_add_string(composite_add_attribute(composite, "stereotype"),
		  op->stereotype);
  data_add_string(composite_add_attribute(composite, "type"),
		  op->type);
  data_add_enum(composite_add_attribute(composite, "visibility"),
		op->visibility);
  /* Backward compatibility */
  data_add_boolean(composite_add_attribute(composite, "abstract"),
		   op->inheritance_type == UML_ABSTRACT);
  data_add_enum(composite_add_attribute(composite, "inheritance_type"),
		op->inheritance_type);
  data_add_boolean(composite_add_attribute(composite, "query"),
		   op->query);
  data_add_boolean(composite_add_attribute(composite, "class_scope"),
		   op->class_scope);
  
  attr_node2 = composite_add_attribute(composite, "parameters");
  
  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *) list->data;

    composite2 = data_add_composite(attr_node2, "umlparameter");

    data_add_string(composite_add_attribute(composite2, "name"),
		    param->name);
    data_add_string(composite_add_attribute(composite2, "type"),
		    param->type);
    data_add_string(composite_add_attribute(composite2, "value"),
		    param->value);
    data_add_enum(composite_add_attribute(composite2, "kind"),
		  param->kind);
    list = g_list_next(list);
  }
}

void
uml_formalparameter_write(AttributeNode attr_node, UMLFormalParameter *param)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "umlformalparameter");

  data_add_string(composite_add_attribute(composite, "name"),
		  param->name);
  data_add_string(composite_add_attribute(composite, "type"),
		  param->type);
}

UMLAttribute *
uml_attribute_read(DataNode composite)
{
  UMLAttribute *attr;
  AttributeNode attr_node;
  
  attr = g_new0(UMLAttribute, 1);

  attr->name = NULL;
  attr_node = composite_find_attribute(composite, "name");
  if (attr_node != NULL)
    attr->name =  data_string( attribute_first_data(attr_node) );

  attr->type = NULL;
  attr_node = composite_find_attribute(composite, "type");
  if (attr_node != NULL)
    attr->type =  data_string( attribute_first_data(attr_node) );

  attr->value = NULL;
  attr_node = composite_find_attribute(composite, "value");
  if (attr_node != NULL)
    attr->value =  data_string( attribute_first_data(attr_node) );
  
  attr->visibility = FALSE;
  attr_node = composite_find_attribute(composite, "visibility");
  if (attr_node != NULL)
    attr->visibility =  data_enum( attribute_first_data(attr_node) );
  
  attr->abstract = FALSE;
  attr_node = composite_find_attribute(composite, "abstract");
  if (attr_node != NULL)
    attr->abstract =  data_boolean( attribute_first_data(attr_node) );
  
  attr->class_scope = FALSE;
  attr_node = composite_find_attribute(composite, "class_scope");
  if (attr_node != NULL)
    attr->class_scope =  data_boolean( attribute_first_data(attr_node) );
  
  attr->left_connection = NULL;
  attr->right_connection = NULL;

  return attr;
}

UMLOperation *
uml_operation_read(DataNode composite)
{
  UMLOperation *op;
  UMLParameter *param;
  AttributeNode attr_node;
  AttributeNode attr_node2;
  DataNode composite2;
  int i, num;

  op = g_new0(UMLOperation, 1);

  op->name = NULL;
  attr_node = composite_find_attribute(composite, "name");
  if (attr_node != NULL)
    op->name =  data_string( attribute_first_data(attr_node) );

  op->type = NULL;
  attr_node = composite_find_attribute(composite, "type");
  if (attr_node != NULL)
    op->type =  data_string( attribute_first_data(attr_node) );

  op->stereotype = NULL;
  attr_node = composite_find_attribute(composite, "stereotype");
  if (attr_node != NULL)
    op->stereotype = data_string( attribute_first_data(attr_node) );

  op->visibility = FALSE;
  attr_node = composite_find_attribute(composite, "visibility");
  if (attr_node != NULL)
    op->visibility =  data_enum( attribute_first_data(attr_node) );
  
  op->inheritance_type = UML_POLYMORPHIC;
  /* Backward compatibility */
  attr_node = composite_find_attribute(composite, "abstract");
  if (attr_node != NULL)
    if(data_boolean( attribute_first_data(attr_node) ))
      op->inheritance_type = UML_ABSTRACT;
  
  attr_node = composite_find_attribute(composite, "inheritance_type");
  if (attr_node != NULL)
    op->inheritance_type = data_enum( attribute_first_data(attr_node) );
  
  attr_node = composite_find_attribute(composite, "query");
  if (attr_node != NULL)
    op->query =  data_boolean( attribute_first_data(attr_node) );
  
  op->class_scope = FALSE;
  attr_node = composite_find_attribute(composite, "class_scope");
  if (attr_node != NULL)
    op->class_scope =  data_boolean( attribute_first_data(attr_node) );

  op->parameters = NULL;
  attr_node2 = composite_find_attribute(composite, "parameters");
  num = attribute_num_data(attr_node2);
  composite2 = attribute_first_data(attr_node2);
  for (i=0;i<num;i++) {
    param = g_new0(UMLParameter, 1);
    
    param->name = NULL;
    attr_node = composite_find_attribute(composite2, "name");
    if (attr_node != NULL)
      param->name =  data_string( attribute_first_data(attr_node) );
    
    param->type = NULL;
    attr_node = composite_find_attribute(composite2, "type");
    if (attr_node != NULL)
      param->type =  data_string( attribute_first_data(attr_node) );
    
    param->value = NULL;
    attr_node = composite_find_attribute(composite2, "value");
    if (attr_node != NULL)
      param->value =  data_string( attribute_first_data(attr_node) );
    
    param->kind = UML_UNDEF_KIND;
    attr_node = composite_find_attribute(composite2, "kind");
    if (attr_node != NULL)
      param->kind =  data_enum( attribute_first_data(attr_node) );
    
    op->parameters = g_list_append(op->parameters, param);
    composite2 = data_next(composite2);
  }

  op->left_connection = NULL;
  op->right_connection = NULL;

  return op;
}

UMLFormalParameter *
uml_formalparameter_read(DataNode composite)
{
  UMLFormalParameter *param;
  AttributeNode attr_node;
  
  param = g_new0(UMLFormalParameter, 1);

  param->name = NULL;
  attr_node = composite_find_attribute(composite, "name");
  if (attr_node != NULL)
    param->name =  data_string( attribute_first_data(attr_node) );

  param->type = NULL;
  attr_node = composite_find_attribute(composite, "type");
  if (attr_node != NULL)
    param->type =  data_string( attribute_first_data(attr_node) );

  return param;
}

