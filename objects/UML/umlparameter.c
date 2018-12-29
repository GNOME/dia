/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * umlparameter.c : refactored from uml.c, class.c to final use StdProps
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

G_DEFINE_TYPE (DiaUmlParameter, dia_uml_parameter, G_TYPE_OBJECT)

enum {
  UML_PARA_NAME = 1,
  UML_PARA_TYPE,
  UML_PARA_VALUE,
  UML_PARA_COMMENT,
  UML_PARA_KIND,
  UML_PARA_N_PROPS
};
static GParamSpec* uml_para_properties[UML_PARA_N_PROPS];

static PropEnumData _uml_parameter_kinds[] = {
  { N_("Undefined"), UML_UNDEF_KIND} ,
  { N_("In"), UML_IN },
  { N_("Out"), UML_OUT },
  { N_("In & Out"), UML_INOUT },
  { NULL, 0 }
};

static PropDescription umlparameter_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Type"), NULL, NULL },
  { "value", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Value"), NULL, NULL },
  { "comment", PROP_TYPE_MULTISTRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },
  { "kind", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Kind"), NULL, _uml_parameter_kinds },

  PROP_DESC_END
};

static PropOffset umlparameter_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(DiaUmlParameter, name) },
  { "type", PROP_TYPE_STRING, offsetof(DiaUmlParameter, type) },
  { "value", PROP_TYPE_STRING, offsetof(DiaUmlParameter, value) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(DiaUmlParameter, comment) },
  { "kind", PROP_TYPE_ENUM, offsetof(DiaUmlParameter, kind) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umlparameter_extra = {
  { umlparameter_props, umlparameter_offsets, "umlparameter" },
  (NewRecordFunc) dia_uml_parameter_new,
  (FreeRecordFunc) g_object_unref
};

char *
dia_uml_parameter_format (DiaUmlParameter *param)
{
  int len;
  char *str;

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
  str = g_malloc (sizeof (char) * (len + 1));

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
  
  g_assert (strlen (str) == len);

  return str;
}

static void
dia_uml_parameter_finalize (GObject *object)
{
  DiaUmlParameter *self = DIA_UML_PARAMETER (object);

  g_free (self->name);
  g_free (self->type);
  if (self->value != NULL) 
    g_free (self->value);
  g_free (self->comment);
}

static void
dia_uml_parameter_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DiaUmlParameter *self = DIA_UML_PARAMETER (object);

  switch (property_id) {
    case UML_PARA_NAME:
      self->name = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_para_properties[UML_PARA_NAME]);
      break;
    case UML_PARA_TYPE:
      self->type = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_para_properties[UML_PARA_TYPE]);
      break;
    case UML_PARA_VALUE:
      self->value = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_para_properties[UML_PARA_VALUE]);
      break;
    case UML_PARA_COMMENT:
      self->comment = g_value_dup_string (value);
      g_object_notify_by_pspec (object, uml_para_properties[UML_PARA_COMMENT]);
      break;
    case UML_PARA_KIND:
      self->kind = g_value_get_int (value);
      g_object_notify_by_pspec (object, uml_para_properties[UML_PARA_KIND]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_parameter_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  DiaUmlParameter *self = DIA_UML_PARAMETER (object);

  switch (property_id) {
    case UML_PARA_NAME:
      g_value_set_string (value, self->name);
      break;
    case UML_PARA_TYPE:
      g_value_set_string (value, self->type);
      break;
    case UML_PARA_VALUE:
      g_value_set_string (value, self->value);
      break;
    case UML_PARA_COMMENT:
      g_value_set_string (value, self->comment);
      break;
    case UML_PARA_KIND:
      g_value_set_int (value, self->kind);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_parameter_class_init (DiaUmlParameterClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_parameter_finalize;
  object_class->set_property = dia_uml_parameter_set_property;
  object_class->get_property = dia_uml_parameter_get_property;

  uml_para_properties[UML_PARA_NAME] = g_param_spec_string ("name",
                                                            "Name",
                                                            "Function name",
                                                            "",
                                                            G_PARAM_READWRITE);
  uml_para_properties[UML_PARA_TYPE] = g_param_spec_string ("type",
                                                            "Type",
                                                            "Return type",
                                                            "",
                                                            G_PARAM_READWRITE);
  uml_para_properties[UML_PARA_VALUE] = g_param_spec_string ("value",
                                                             "Value",
                                                             "Default value",
                                                             "",
                                                             G_PARAM_READWRITE);
  uml_para_properties[UML_PARA_COMMENT] = g_param_spec_string ("comment",
                                                               "Comment",
                                                               "Comment",
                                                               "",
                                                               G_PARAM_READWRITE);
  uml_para_properties[UML_PARA_KIND] = g_param_spec_int ("kind",
                                                         "Kind",
                                                         "Parameter type",
                                                         UML_UNDEF_KIND,
                                                         UML_INOUT,
                                                         UML_UNDEF_KIND,
                                                         G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     UML_PARA_N_PROPS,
                                     uml_para_properties);
}

static void
dia_uml_parameter_init (DiaUmlParameter *self)
{
  self->name = g_strdup("");
  self->type = g_strdup("");
  self->comment = g_strdup("");
  self->value = g_strdup("");
  self->kind = UML_UNDEF_KIND;
}

DiaUmlParameter *
dia_uml_parameter_new ()
{
  return g_object_new (DIA_UML_TYPE_PARAMETER, NULL);
}

GList *
dia_uml_operation_get_parameters (DiaUmlOperation *self)
{
  return self->parameters;
}
