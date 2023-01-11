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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>

#include "uml.h"
#include "properties.h"

static PropEnumData _uml_parameter_kinds[] = {
  { N_("Undefined"), DIA_UML_UNDEF_KIND} ,
  { N_("In"), DIA_UML_IN },
  { N_("Out"), DIA_UML_OUT },
  { N_("In & Out"), DIA_UML_INOUT },
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
  { "name", PROP_TYPE_STRING, offsetof(UMLParameter, name) },
  { "type", PROP_TYPE_STRING, offsetof(UMLParameter, type) },
  { "value", PROP_TYPE_STRING, offsetof(UMLParameter, value) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(UMLParameter, comment) },
  { "kind", PROP_TYPE_ENUM, offsetof(UMLParameter, kind) },
  { NULL, 0, 0 },
};

PropDescDArrayExtra umlparameter_extra = {
  { umlparameter_props, umlparameter_offsets, "umlparameter" },
  (NewRecordFunc) uml_parameter_new,
  (FreeRecordFunc) uml_parameter_unref
};

G_DEFINE_BOXED_TYPE (UMLParameter, uml_parameter, uml_parameter_ref, uml_parameter_unref)


UMLParameter *
uml_parameter_new (void)
{
  UMLParameter *param;

  param = g_rc_box_new0 (UMLParameter);
  param->name = g_strdup ("");
  param->type = g_strdup ("");
  param->value = g_strdup ("");
  param->comment = g_strdup ("");
  param->kind = DIA_UML_UNDEF_KIND;

  return param;
}


UMLParameter *
uml_parameter_copy (UMLParameter *param)
{
  UMLParameter *new;

  g_return_val_if_fail (param != NULL, NULL);

  new = uml_parameter_new ();

  g_clear_pointer (&new->name, g_free);
  g_clear_pointer (&new->type, g_free);
  g_clear_pointer (&new->value, g_free);
  g_clear_pointer (&new->comment, g_free);

  new->name = g_strdup (param->name);
  new->type = g_strdup (param->type);
  new->value = g_strdup (param->value);
  new->comment = g_strdup (param->comment);

  new->kind = param->kind;

  return new;
}


UMLParameter *
uml_parameter_ref (UMLParameter *param)
{
  g_return_val_if_fail (param != NULL, NULL);

  return g_rc_box_acquire (param);
}


static void
parameter_free (UMLParameter *param)
{
  g_clear_pointer (&param->name, g_free);
  g_clear_pointer (&param->type, g_free);
  g_clear_pointer (&param->value, g_free);
  g_clear_pointer (&param->comment, g_free);
}


void
uml_parameter_unref (UMLParameter *param)
{
  g_rc_box_release_full (param, (GDestroyNotify) parameter_free);
}


char *
uml_parameter_get_string (UMLParameter *param)
{
  int len;
  char *str;

  /* Calculate length: */
  len = strlen (param->name);

  if (param->type && strlen (param->type) > 0) {
    len += 1 + strlen (param->type);
  }

  if (param->value && strlen (param->value) > 0) {
    len += 1 + strlen (param->value);
  }

  switch (param->kind) {
    case DIA_UML_UNDEF_KIND:
      break;
    case DIA_UML_IN:
      len += 3;
      break;
    case DIA_UML_OUT:
      len += 4;
      break;
    case DIA_UML_INOUT:
      len += 6;
      break;
    default:
      g_return_val_if_reached (NULL);
  }

  /* Generate string: */
  str = g_new0 (char, len + 1);

  strcpy (str, "");

  switch (param->kind) {
    case DIA_UML_UNDEF_KIND:
      break;
    case DIA_UML_IN:
      strcat (str, "in ");
      break;
    case DIA_UML_OUT:
      strcat (str, "out ");
      break;
    case DIA_UML_INOUT:
      strcat (str, "inout ");
      break;
    default:
      g_return_val_if_reached (NULL);
  }

  strcat (str, param->name);

  if (param->type && strlen (param->type) > 0) {
    strcat (str, ":");
    strcat (str, param->type);
  }

  if (param->value && strlen (param->value) > 0) {
    strcat (str, "=");
    strcat (str, param->value);
  }

  g_assert (strlen (str) == len);

  return str;
}

