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
#include <string.h>
#include <stdio.h>
#include <assert.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "sheet.h"

#include "uml.h"

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
extern ObjectType message_type;
extern ObjectType component_type;
extern ObjectType classicon_type;

void register_objects(void) {
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
  object_register_type(&message_type);  
  object_register_type(&component_type);
  object_register_type(&classicon_type);
}

extern SheetObject umlclass_sheetobj;
extern SheetObject umlclass_template_sheetobj;
extern SheetObject note_sheetobj;
extern SheetObject dependency_sheetobj;
extern SheetObject realizes_sheetobj;
extern SheetObject generalization_sheetobj;
extern SheetObject association_sheetobj;
extern SheetObject aggregation_sheetobj;
extern SheetObject implements_sheetobj;
extern SheetObject constraint_sheetobj;
extern SheetObject smallpackage_sheetobj;
extern SheetObject largepackage_sheetobj;
extern SheetObject actor_sheetobj;
extern SheetObject usecase_sheetobj;
extern SheetObject lifeline_sheetobj;
extern SheetObject objet_sheetobj;
extern SheetObject message_sheetobj;
extern SheetObject component_sheetobj;
extern SheetObject classicon_sheetobj;

int get_version(void) {
  return 0;
}

void register_sheets(void) {
  Sheet *sheet;
  
  sheet = new_sheet(_("UML"),
		    _("Editor for UML Static Structure Diagrams."));
  sheet_append_sheet_obj(sheet, &umlclass_sheetobj);
  sheet_append_sheet_obj(sheet, &umlclass_template_sheetobj);
  sheet_append_sheet_obj(sheet, &note_sheetobj);
  sheet_append_sheet_obj(sheet, &dependency_sheetobj);
  sheet_append_sheet_obj(sheet, &realizes_sheetobj);
  sheet_append_sheet_obj(sheet, &generalization_sheetobj);
  sheet_append_sheet_obj(sheet, &association_sheetobj);
  sheet_append_sheet_obj(sheet, &aggregation_sheetobj);
  sheet_append_sheet_obj(sheet, &implements_sheetobj);
  sheet_append_sheet_obj(sheet, &constraint_sheetobj);
  sheet_append_sheet_obj(sheet, &smallpackage_sheetobj);
  sheet_append_sheet_obj(sheet, &largepackage_sheetobj);
  sheet_append_sheet_obj(sheet, &actor_sheetobj);
  sheet_append_sheet_obj(sheet, &usecase_sheetobj);
  sheet_append_sheet_obj(sheet, &lifeline_sheetobj);
  sheet_append_sheet_obj(sheet, &objet_sheetobj);
  sheet_append_sheet_obj(sheet, &message_sheetobj);
  sheet_append_sheet_obj(sheet, &component_sheetobj);
  sheet_append_sheet_obj(sheet, &classicon_sheetobj);

  register_sheet(sheet);
}

char visible_char[] = { '+', '-', '#', ' ' };

char *
uml_get_attribute_string(UMLAttribute *attribute)
{
  int len;
  char *str;

  len = 1 + strlen(attribute->name)  + 2 + strlen(attribute->type);
  if (attribute->value != NULL) {
    len += 3 + strlen(attribute->value);
  }

  str = g_malloc(sizeof(char)*(len+1));

  str[0] = visible_char[(int) attribute->visibility];
  str[1] = 0;
  
  strcat(str, attribute->name);
  strcat(str, ": ");
  strcat(str, attribute->type);
  if (attribute->value != NULL) {
    strcat(str, " = ");
    strcat(str, attribute->value);
  }

  assert(strlen(str)==len);

  return str;
}

char *
uml_get_operation_string(UMLOperation *operation)
{
  int len;
  char *str;
  GList *list;
  UMLParameter *param;

  /* Calculate length: */
  len = 1 + strlen(operation->name)  + 1;
  
  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
    list = g_list_next(list);

    len += strlen(param->name) + 1 + strlen(param->type);
    if (param->value != NULL) {
      len += 1 + strlen(param->value);
    }

    if (list != NULL) {
      len += 1; /* ',' */
    }
  }
  len += 1; /* ')' */
  if (operation->type != NULL) {
    len += 2 + strlen(operation->type);
  }
  
  /* generate string: */
  str = g_malloc(sizeof(char)*(len+1));

  str[0] = visible_char[(int) operation->visibility];
  str[1] = 0;

  strcat(str, operation->name);
  strcat(str, "(");
  
  list = operation->parameters;
  while (list != NULL) {
    param = (UMLParameter  *) list->data;
    list = g_list_next(list);

    strcat(str, param->name);
    strcat(str, ":");
    strcat(str, param->type);
    
    if (param->value != NULL) {
      strcat(str, "=");
      strcat(str, param->value);
    }

    if (list != NULL) {
      strcat(str, ",");
    }
  }
  strcat(str, ")");

  if (operation->type != NULL) {
    strcat(str, ": ");
    strcat(str, operation->type);
  }

  assert(strlen(str)==len);
  
  return str;
}

char *
uml_get_parameter_string(UMLParameter *param)
{
  int len;
  char *str;

  /* Calculate length: */
  len = strlen(param->name) + 1 + strlen(param->type);
  
  if (param->value != NULL) {
    len += 1 + strlen(param->value) ;
  }

  /* Generate string: */
  str = g_malloc(sizeof(char)*(len+1));
  strcpy(str, param->name);
  strcat(str, ":");
  strcat(str, param->type);
  if (param->value != NULL) {
    strcat(str, "=");
    strcat(str, param->value);
  }
  
  assert(strlen(str)==len);

  return str;
}

char *
uml_get_formalparameter_string(UMLFormalParameter *parameter)
{
  int len;
  char *str;

  /* Calculate length: */
  len = strlen(parameter->name);
  
  if (parameter->type != NULL) {
    len += 1 + strlen(parameter->type);
  }

  /* Generate string: */
  str = g_malloc(sizeof(char)*(len+1));
  strcpy(str, parameter->name);
  if (parameter->type != NULL) {
    strcat(str, ":");
    strcat(str, parameter->type);
  }

  assert(strlen(str)==len);

  return str;
}


UMLAttribute *
uml_attribute_copy(UMLAttribute *attr)
{
  UMLAttribute *newattr;

  newattr = g_new(UMLAttribute, 1);
  newattr->name = strdup(attr->name);
  newattr->type = strdup(attr->type);
  if (attr->value != NULL) {
    newattr->value = strdup(attr->value);
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
  
  newop = g_new(UMLOperation, 1);
  newop->name = strdup(op->name);
  if (op->type != NULL) {
    newop->type = strdup(op->type);
  } else {
    newop->type = NULL;
  }
  newop->visibility = op->visibility;
  newop->abstract = op->abstract;
  newop->class_scope = op->class_scope;

  newop->left_connection = op->left_connection;
  newop->right_connection = op->right_connection;
  
  newop->parameters = NULL;
  list = op->parameters;
  while (list != NULL) {
    param = (UMLParameter *)list->data;

    newparam = g_new(UMLParameter, 1);
    newparam->name = strdup(param->name);
    newparam->type = strdup(param->type);
    if (param->value != NULL)
      newparam->value = strdup(param->value);
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

  newparam = g_new(UMLFormalParameter, 1);

  newparam->name = strdup(param->name);
  if (param->type != NULL) {
    newparam->type = strdup(param->type);
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
  
  attr = g_new(UMLAttribute, 1);
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

  op = g_new(UMLOperation, 1);
  op->name = g_strdup("");
  op->type = NULL;
  op->visibility = UML_PUBLIC;
  op->abstract = FALSE;
  op->class_scope = FALSE;
  op->parameters = NULL;

  op->left_connection = NULL;
  op->right_connection = NULL;
  return op;
}

UMLParameter *
uml_parameter_new(void)
{
  UMLParameter *param;

  param = g_new(UMLParameter, 1);
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

  param = g_new(UMLFormalParameter, 1);
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
  data_add_string(composite_add_attribute(composite, "type"),
		  op->type);
  data_add_enum(composite_add_attribute(composite, "visibility"),
		op->visibility);
  data_add_boolean(composite_add_attribute(composite, "abstract"),
		   op->abstract);
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
  
  attr = g_new(UMLAttribute, 1);

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

  op = g_new(UMLOperation, 1);

  op->name = NULL;
  attr_node = composite_find_attribute(composite, "name");
  if (attr_node != NULL)
    op->name =  data_string( attribute_first_data(attr_node) );

  op->type = NULL;
  attr_node = composite_find_attribute(composite, "type");
  if (attr_node != NULL)
    op->type =  data_string( attribute_first_data(attr_node) );

  op->visibility = FALSE;
  attr_node = composite_find_attribute(composite, "visibility");
  if (attr_node != NULL)
    op->visibility =  data_enum( attribute_first_data(attr_node) );
  
  op->abstract = FALSE;
  attr_node = composite_find_attribute(composite, "abstract");
  if (attr_node != NULL)
    op->abstract =  data_boolean( attribute_first_data(attr_node) );
  
  op->class_scope = FALSE;
  attr_node = composite_find_attribute(composite, "class_scope");
  if (attr_node != NULL)
    op->class_scope =  data_boolean( attribute_first_data(attr_node) );

  op->parameters = NULL;
  attr_node2 = composite_find_attribute(composite, "parameters");
  num = attribute_num_data(attr_node2);
  composite2 = attribute_first_data(attr_node2);
  for (i=0;i<num;i++) {
    param = g_new(UMLParameter, 1);
    
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
  
  param = g_new(UMLFormalParameter, 1);

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

