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

static PropDescription umlformalparameter_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Type"), NULL, NULL },

  PROP_DESC_END
};

static PropOffset umlformalparameter_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(UMLFormalParameter, name) },
  { "type", PROP_TYPE_STRING, offsetof(UMLFormalParameter, type) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umlformalparameter_extra = {
  { umlformalparameter_props, umlformalparameter_offsets, "umlformalparameter" },
  (NewRecordFunc)uml_formalparameter_new,
  (FreeRecordFunc)uml_formalparameter_destroy
};

UMLFormalParameter *
uml_formalparameter_new(void)
{
  UMLFormalParameter *param;

  param = g_new0(UMLFormalParameter, 1);
  param->name = g_strdup("");
  param->type = NULL;

  return param;
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
uml_formalparameter_destroy(UMLFormalParameter *param)
{
  g_free(param->name);
  if (param->type != NULL)
    g_free(param->type);
  g_free(param);
}

void
uml_formalparameter_write(AttributeNode attr_node, UMLFormalParameter *param,
			  DiaContext *ctx)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "umlformalparameter", ctx);

  data_add_string(composite_add_attribute(composite, "name"),
		  param->name, ctx);
  data_add_string(composite_add_attribute(composite, "type"),
		  param->type, ctx);
}

char *
uml_get_formalparameter_string (UMLFormalParameter *parameter)
{
  int len;
  char *str;

  /* Calculate length: */
  len = parameter->name ? strlen (parameter->name) : 0;
  
  if (parameter->type != NULL) {
    len += 1 + strlen (parameter->type);
  }

  /* Generate string: */
  str = g_malloc (sizeof (char) * (len + 1));
  strcpy (str, parameter->name ? parameter->name : "");
  if (parameter->type != NULL) {
    strcat (str, ":");
    strcat (str, parameter->type);
  }

  g_assert (strlen (str) == len);

  return str;
}

