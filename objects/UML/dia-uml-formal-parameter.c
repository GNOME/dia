/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * umlformalparameter.c : refactored from uml.c, class.c to final use StdProps
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "uml.h"
#include "properties.h"
#include "dia-uml-formal-parameter.h"
#include "list/dia-list-data.h"

static PropDescription umlformalparameter_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Type"), NULL, NULL },

  PROP_DESC_END
};

static PropOffset umlformalparameter_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(DiaUmlFormalParameter, name) },
  { "type", PROP_TYPE_STRING, offsetof(DiaUmlFormalParameter, type) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umlformalparameter_extra = {
  { umlformalparameter_props, umlformalparameter_offsets, "umlformalparameter" },
  (NewRecordFunc) dia_uml_formal_parameter_new,
  (FreeRecordFunc) g_object_unref
};


static void
dia_uml_formal_parameter_list_data_init (DiaListDataInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DiaUmlFormalParameter, dia_uml_formal_parameter, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (DIA_TYPE_LIST_DATA,
                                                dia_uml_formal_parameter_list_data_init))

enum {
  PROP_NAME = 1,
  PROP_TYPE,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];


static void
dia_uml_formal_parameter_finalize (GObject *object)
{
  DiaUmlFormalParameter *self = DIA_UML_FORMAL_PARAMETER (object);

  g_free (self->name);
  g_free (self->type);
}


static void
dia_uml_formal_parameter_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
  DiaUmlFormalParameter *self = DIA_UML_FORMAL_PARAMETER (object);

  switch (property_id) {
    case PROP_NAME:
      self->name = g_value_dup_string (value);
      g_object_notify_by_pspec (object, properties[PROP_NAME]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    case PROP_TYPE:
      self->type = g_value_dup_string (value);
      g_object_notify_by_pspec (object, properties[PROP_TYPE]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_formal_parameter_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
  DiaUmlFormalParameter *self = DIA_UML_FORMAL_PARAMETER (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static const gchar *
format (DiaListData *self)
{
  return dia_uml_formal_parameter_format (DIA_UML_FORMAL_PARAMETER (self));
}

static void
dia_uml_formal_parameter_list_data_init (DiaListDataInterface *iface)
{
  iface->format = format;
}

static void
dia_uml_formal_parameter_class_init (DiaUmlFormalParameterClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_formal_parameter_finalize;
  object_class->set_property = dia_uml_formal_parameter_set_property;
  object_class->get_property = dia_uml_formal_parameter_get_property;

  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name",
                                               "Parameter name",
                                               "",
                                               G_PARAM_READWRITE);
  properties[PROP_TYPE] = g_param_spec_string ("type",
                                               "Type",
                                               "Formal parameter type",
                                               "",
                                               G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);
}

static void
dia_uml_formal_parameter_init (DiaUmlFormalParameter *self)
{
  self->name = g_strdup ("");
  self->type = g_strdup ("");
}

DiaUmlFormalParameter *
dia_uml_formal_parameter_new ()
{
  return g_object_new (DIA_UML_TYPE_FORMAL_PARAMETER, NULL);
}

DiaUmlFormalParameter *
dia_uml_formal_parameter_copy (DiaUmlFormalParameter *param)
{
  DiaUmlFormalParameter *newparam;

  newparam = g_object_new (DIA_UML_TYPE_FORMAL_PARAMETER, NULL);

  newparam->name = g_strdup(param->name);
  newparam->type = g_strdup(param->type);

  return newparam;
}

void
uml_formalparameter_write(AttributeNode attr_node, DiaUmlFormalParameter *param,
			  DiaContext *ctx)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "umlformalparameter", ctx);

  data_add_string(composite_add_attribute(composite, "name"),
		  param->name, ctx);
  data_add_string(composite_add_attribute(composite, "type"),
		  param->type, ctx);
}

gchar *
dia_uml_formal_parameter_format (DiaUmlFormalParameter *parameter)
{
  int len;
  char *str;

  /* Calculate length: */
  len = parameter->name ? strlen (parameter->name) : 0;
  
  if (strlen (parameter->type) > 0) {
    len += 1 + strlen (parameter->type);
  }

  /* Generate string: */
  str = g_malloc (sizeof (char) * (len + 1));
  strcpy (str, parameter->name ? parameter->name : "");
  if (strlen (parameter->type) > 0) {
    strcat (str, ":");
    strcat (str, parameter->type);
  }

  g_assert (strlen (str) == len);

  return str;
}

