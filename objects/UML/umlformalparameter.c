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

#include "config.h"

#include <glib/gi18n-lib.h>

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
  (NewRecordFunc) uml_formal_parameter_new,
  (FreeRecordFunc) uml_formal_parameter_unref
};

G_DEFINE_BOXED_TYPE (UMLFormalParameter, uml_formal_parameter, uml_formal_parameter_ref, uml_formal_parameter_unref)


UMLFormalParameter *
uml_formal_parameter_new (void)
{
  UMLFormalParameter *param;

  param = g_rc_box_new0 (UMLFormalParameter);
  param->name = NULL;
  param->type = NULL;

  return param;
}


UMLFormalParameter *
uml_formal_parameter_copy (UMLFormalParameter *param)
{
  UMLFormalParameter *newparam;

  newparam = uml_formal_parameter_new ();

  newparam->name = g_strdup (param->name);
  newparam->type = g_strdup (param->type);

  return newparam;
}


UMLFormalParameter *
uml_formal_parameter_ref (UMLFormalParameter *param)
{
  g_return_val_if_fail (param != NULL, NULL);

  return g_rc_box_acquire (param);
}


static void
formal_parameter_destroy (UMLFormalParameter *param)
{
  g_clear_pointer (&param->name, g_free);
  g_clear_pointer (&param->type, g_free);
}


void
uml_formal_parameter_unref (UMLFormalParameter *param)
{
  g_rc_box_release_full (param, (GDestroyNotify) formal_parameter_destroy);
}


void
uml_formal_parameter_write (AttributeNode       attr_node,
                            UMLFormalParameter *param,
                            DiaContext         *ctx)
{
  DataNode composite;

  composite = data_add_composite (attr_node, "umlformalparameter", ctx);

  data_add_string (composite_add_attribute (composite, "name"),
                   param->name, ctx);
  data_add_string (composite_add_attribute (composite, "type"),
                   param->type, ctx);
}


char *
uml_formal_parameter_get_string (UMLFormalParameter *parameter)
{
  int len;
  char *str;

  /* Calculate length: */
  len = parameter->name ? strlen (parameter->name) : 0;

  if (parameter->type != NULL) {
    len += 1 + strlen (parameter->type);
  }

  /* Generate string: */
  str = g_new0 (char, len + 1);
  strcpy (str, parameter->name ? parameter->name : "");
  if (parameter->type != NULL) {
    strcat (str, ":");
    strcat (str, parameter->type);
  }

  g_assert (strlen (str) == len);

  return str;
}

