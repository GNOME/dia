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
#include "eml.h"
#include "listfun.h"
#include "plug-ins.h"

extern ObjectType emlprocess_type;
extern ObjectType instantiation_type;
extern ObjectType interaction_type;
extern ObjectType interaction_ortho_type;

DIA_PLUGIN_CHECK_INIT

PluginInitResult
dia_plugin_init(PluginInfo *info)
{
  if (!dia_plugin_info_init(info, "EML",
			    _("Event Modelling Language diagram"),
			    NULL, NULL))
    return DIA_PLUGIN_INIT_ERROR;

  object_register_type(&emlprocess_type);
  object_register_type(&instantiation_type);
  object_register_type(&interaction_type);
  object_register_type(&interaction_ortho_type);

  return DIA_PLUGIN_INIT_OK;
}

gchar *
eml_get_parameter_string(EMLParameter *parameter)
{
  gchar *str;
  GList *list;
  gint len;
  gchar* *ary;
  gchar *param_str;
  gint idx;

  if (parameter->type != EML_RELATION)
    return g_strdup(parameter->name);

  list = parameter->relmembers;
  len = g_list_length(list);
  ary = g_new0(gchar*, len+1);
 
  for (idx = 0; list != NULL; idx++) {
    ary[idx] = g_strdup((gchar*) list->data);
    list = g_list_next(list);
  }

  param_str = g_strjoinv(", ", ary);
  g_strfreev(ary);

  str = g_strconcat(parameter->name, " = {", param_str, "}", NULL);
  g_free(param_str);

  return str;
}

gchar *
eml_get_ifmessage_string(EMLParameter *parameter)
{
  gchar *str;
  GList *list;
  gint len;
  gchar* *ary;
  gchar *param_str;
  gint idx;

  list = parameter->relmembers;
  len = g_list_length(list);
  ary = g_new0(gchar*, len+1);
 
  for (idx = 0; list != NULL; idx++) {
    ary[idx] = g_strdup((gchar*) list->data);
    list = g_list_next(list);
  }

  param_str = g_strjoinv(", ", ary);
  g_strfreev(ary);

  str = g_strconcat("{", param_str, "}", NULL);
  g_free(param_str);

  return str;
}

gchar *
eml_get_function_string(EMLFunction *function)
{
  gchar *str;
  GList *list;
  EMLParameter *param;
  gint len;
  gchar* *ary;
  gchar *param_str;
  gint idx;

  list = function->parameters;
  len = g_list_length(list);
  ary = g_new0(gchar*, len+1);
 
  for (idx = 0; list != NULL; idx++) {
    param = (EMLParameter*) list->data;
    ary[idx] = g_strdup(param->name); 
    list = g_list_next(list);
  }

  param_str = g_strjoinv(", ", ary);
  g_strfreev(ary);

  str = g_strconcat(function->module, ":", function->name, "(", param_str, ")", NULL);
  g_free(param_str);

  return str;

}

gchar *
eml_get_iffunction_string(EMLFunction *function)
{
  return g_strconcat(function->module, ":", function->name, NULL);
}

EMLParameter *
eml_parameter_copy(EMLParameter *param)
{
  EMLParameter *newparam;
  GList *newmembers;
  GList *list;

  newparam = g_new0(EMLParameter, 1);
  newparam->name = strdup(param->name);
  newparam->type = param->type;

  newmembers = NULL;
  list = param->relmembers;
  while (list != NULL) {
    newmembers = g_list_append(newmembers, g_strdup((gchar*) list->data));
    list = g_list_next(list);
  }

  newparam->relmembers = newmembers;

  newparam->left_connection = param->left_connection;
  newparam->right_connection = param->right_connection;

  return newparam;
}

EMLParameter *
eml_ifmessage_copy(EMLParameter *param)
{
  return eml_parameter_copy(param);
}

EMLFunction *
eml_function_copy(EMLFunction *fun)
{
  EMLFunction *newfun;
  EMLParameter *param;
  EMLParameter *newparam;
  GList *list;
  
  newfun = g_new0(EMLFunction, 1);
  newfun->module = g_strdup(fun->module);
  newfun->name = g_strdup(fun->name);
  newfun->left_connection = fun->left_connection;
  newfun->right_connection = fun->right_connection;
  
  newfun->parameters = NULL;
  list = fun->parameters;
  while (list != NULL) {
    param = (EMLParameter *)list->data;
    newparam = eml_parameter_copy(param);
    newfun->parameters = g_list_append(newfun->parameters, newparam);
    
    list = g_list_next(list);
  }

  return newfun;
}

EMLFunction *
eml_iffunction_copy(EMLFunction *fun)
{
  return eml_function_copy(fun);
}

void
eml_parameter_destroy(EMLParameter *param)
{
  GList *list;

  g_free(param->name);

  list = param->relmembers;
  while (list != NULL) {
    g_free(list->data);
    list = g_list_next(list);
  }
  g_free(param);
}

void
eml_ifmessage_destroy(EMLParameter *param)
{
  eml_parameter_destroy(param);
}

void
eml_function_destroy(EMLFunction *fun)
{
  GList *list;
  EMLParameter *param;
  
  g_free(fun->module);
  g_free(fun->name);

  list = fun->parameters;
  while (list != NULL) {
    param = (EMLParameter *)list->data;
    eml_parameter_destroy(param);
    list = g_list_next(list);
  }
  g_free(fun);
}

void
eml_iffunction_destroy(EMLFunction *fun)
{
  eml_function_destroy(fun);
}

EMLParameter *
eml_parameter_new(void)
{
  EMLParameter *param;
  
  param = g_new0(EMLParameter, 1);
  param->name = g_strdup("");
  param->type = EML_OTHER;
  param->relmembers = NULL;
  param->left_connection = NULL;
  param->right_connection = NULL;

  return param;
}

EMLInterface *
eml_interface_new(void)
{
  return g_new0(EMLInterface, 1);
}

EMLParameter *
eml_ifmessage_new(void)
{
  return eml_parameter_new();
}

EMLFunction *
eml_function_new(void)
{
  EMLFunction *fun;

  fun = g_new0(EMLFunction, 1);
  fun->module = g_strdup("");
  fun->name = g_strdup("");
  fun->parameters = NULL;

  fun->left_connection = NULL;
  fun->right_connection = NULL;
  return fun;
}

void
eml_parameter_write(AttributeNode param_node, EMLParameter *param)
{
  DataNode composite;
  AttributeNode param_node2;
  GList *list;

  composite = data_add_composite(param_node, "emlparameter");

  data_add_string(composite_add_attribute(composite, "name"),
		  param->name);
  data_add_enum(composite_add_attribute(composite, "type"),
		  param->type);

  param_node2 = composite_add_attribute(composite, "relmembers");

  list = param->relmembers;
  while (list != NULL) {
    data_add_string(param_node2, (gchar*) list->data);
    list = g_list_next(list);
  }

}

void
eml_ifmessage_write(AttributeNode param_node, EMLParameter *param)
{
  eml_parameter_write(param_node, param);
}

void
eml_function_write(AttributeNode param_node, EMLFunction *fun)
{
  GList *list;
  EMLParameter *param;
  DataNode composite;
  AttributeNode param_node2;

  composite = data_add_composite(param_node, "emlfunction");

  data_add_string(composite_add_attribute(composite, "name"),
		  fun->name);
  data_add_string(composite_add_attribute(composite, "module"),
		  fun->module);
  
  param_node2 = composite_add_attribute(composite, "parameters");
  
  list = fun->parameters;
  while (list != NULL) {
    param = (EMLParameter *) list->data;

    eml_parameter_write(param_node2, param);
    list = g_list_next(list);
  }
}

static
void
function_write(gpointer fun, gpointer node)
{
  eml_function_write((AttributeNode) node, (EMLFunction*) fun);
}

static
void
param_write(gpointer param, gpointer node)
{
  eml_parameter_write((AttributeNode) node, (EMLParameter*) param);
}

void
eml_interface_write(AttributeNode param_node, EMLInterface *iface)
{
  DataNode composite;
  AttributeNode param_node2;

  composite = data_add_composite(param_node, "interface");

  data_add_string(composite_add_attribute(composite, "name"),
		  iface->name);

  param_node2 = composite_add_attribute(composite, "functions");
  g_list_foreach(iface->functions, function_write, param_node2);

  param_node2 = composite_add_attribute(composite, "messages");
  g_list_foreach(iface->messages, param_write, param_node2);
}

void
eml_iffunction_write(AttributeNode param_node, EMLFunction *fun)
{
  eml_function_write(param_node, fun);
}

EMLParameter *
eml_parameter_read(DataNode composite)
{
  EMLParameter *param;
  AttributeNode param_node;
  AttributeNode param_node2;
  gint num;
  gint i;

  param = g_new0(EMLParameter, 1);

  param->name = NULL;
  param_node = composite_find_attribute(composite, "name");
  if (param_node != NULL)
    param->name =  data_string( attribute_first_data(param_node) );

  param->type = EML_OTHER;
  param_node = composite_find_attribute(composite, "type");
  if (param_node != NULL)
    param->type =  data_enum( attribute_first_data(param_node) );

  param->relmembers = NULL;
  param_node2 = composite_find_attribute(composite, "relmembers");
  num = attribute_num_data(param_node2);
  if (num > 0)
    param_node2 = attribute_first_data(param_node2);
  for (i=0;i<num;i++) {
    param->relmembers = g_list_append(param->relmembers,
                                      data_string(param_node2));

    param->left_connection = NULL;
    param->right_connection = NULL;
    param_node2 = data_next(param_node2);
  }
  return param;
}

EMLParameter *
eml_ifmessage_read(DataNode composite)
{
  return eml_parameter_read(composite);
}

EMLFunction *
eml_function_read(DataNode composite)
{
  EMLFunction *fun;
  AttributeNode param_node;
  AttributeNode param_node2;
  DataNode composite2;
  int i, num;

  fun = g_new0(EMLFunction, 1);

  fun->name = NULL;
  param_node = composite_find_attribute(composite, "name");
  if (param_node != NULL)
    fun->name =  data_string( attribute_first_data(param_node) );

  fun->module = NULL;
  param_node = composite_find_attribute(composite, "module");
  if (param_node != NULL)
    fun->module =  data_string( attribute_first_data(param_node) );

  fun->parameters = NULL;
  param_node2 = composite_find_attribute(composite, "parameters");
  num = attribute_num_data(param_node2);
  composite2 = attribute_first_data(param_node2);
  for (i=0;i<num;i++) {
    fun->parameters = g_list_append(fun->parameters, eml_parameter_read(composite2));
    composite2 = data_next(composite2);
  }
  fun->left_connection = NULL;
  fun->right_connection = NULL;

  return fun;
}

EMLInterface*
eml_interface_read(DataNode composite)
{
  AttributeNode attr_node;
  int i;
  int num;
  EMLInterface *iface;
  EMLFunction *fun;
  EMLParameter *param;
  DataNode composite2;

  iface = g_new0(EMLInterface, 1);
  iface->functions = NULL;
  iface->messages = NULL;
  iface->name = NULL;

  attr_node = composite_find_attribute(composite, "name");
  if (attr_node != NULL)
    iface->name =  data_string( attribute_first_data(attr_node) );

  attr_node = composite_find_attribute(composite, "functions");
  num = attribute_num_data(attr_node);
  composite2 = attribute_first_data(attr_node);
  for (i=0;i<num;i++) {
    fun = eml_function_read(composite2);
    iface->functions = g_list_append(iface->functions, fun);
    composite2 = data_next(composite2);
  }

  attr_node = composite_find_attribute(composite, "messages");
  num = attribute_num_data(attr_node);
  composite2 = attribute_first_data(attr_node);
  for (i=0;i<num;i++) {
    param = eml_parameter_read(composite2);
    iface->messages = g_list_append(iface->messages, param);
    composite2 = data_next(composite2);
  }

  return iface;
}

EMLFunction *
eml_iffunction_read(DataNode composite)
{
  return eml_function_read(composite);
}

void emlconnected_new(EMLConnected *conn)
{
  conn->left_connection = g_new0(ConnectionPoint, 1);
  conn->right_connection = g_new0(ConnectionPoint, 1);
}

void emlconnected_destroy(EMLConnected *conn)
{
  g_free(conn->left_connection);
  g_free(conn->right_connection);
}

void function_connections_new(EMLFunction *fun)
{
  emlconnected_new((EMLConnected*) fun);
  g_list_foreach(fun->parameters, list_foreach_fun, emlconnected_new);
}

void function_connections_destroy(EMLFunction *fun)
{
  emlconnected_destroy((EMLConnected*) fun);
  g_list_foreach(fun->parameters, list_foreach_fun, emlconnected_destroy);
}

void
interface_connections_new( EMLInterface *iface)
{
  g_list_foreach(iface->functions, list_foreach_fun,
                 (gpointer) function_connections_new);
  g_list_foreach(iface->messages, list_foreach_fun, emlconnected_new);
}

EMLInterface*
eml_interface_copy( EMLInterface *iface)
{
  EMLInterface *newiface;

  newiface = g_new0(EMLInterface, 1);

  newiface->name = g_strdup(iface->name);

  newiface->functions = list_map(iface->functions,
                                 (MapFun) eml_iffunction_copy);

  newiface->messages = list_map(iface->messages,
                               (MapFun) eml_ifmessage_copy);

  return newiface;
}

EMLInterface*
emlprocess_interface_copy( EMLInterface *iface)
{
  EMLInterface *newiface;

  newiface = eml_interface_copy(iface);

  g_list_foreach(newiface->functions, list_foreach_fun,
                 function_connections_new);

  g_list_foreach(newiface->messages, list_foreach_fun, emlconnected_new);

  return newiface;
}

void
eml_interface_destroy( EMLInterface *iface)
{
  g_free(iface->name);

  g_list_foreach(iface->functions, list_free_foreach, eml_function_destroy);
  g_list_free(iface->functions);

  g_list_foreach(iface->messages, list_free_foreach, eml_parameter_destroy);
  g_list_free(iface->messages);

  g_free(iface);
}

void
emlprocess_interface_destroy( EMLInterface *iface)
{
  g_list_foreach(iface->functions,
                 list_foreach_fun, function_connections_destroy);
  g_list_foreach(iface->messages, list_foreach_fun, emlconnected_destroy);

  eml_interface_destroy( iface);

}
