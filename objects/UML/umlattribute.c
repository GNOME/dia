/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * umlattribute.c : refactored from uml.c, class.c to final use StdProps
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

extern PropEnumData _uml_visibilities[];

static PropDescription umlattribute_props[] = {
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Name"), NULL, NULL },
  { "type", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Type"), NULL, NULL },
  { "value", PROP_TYPE_STRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Value"), NULL, NULL },
  { "comment", PROP_TYPE_MULTISTRING, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Comment"), NULL, NULL },
  { "visibility", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Visibility"), NULL, _uml_visibilities },
  /* Kept for backward compatibility, not sure what it is meant to be --hb */
  { "abstract", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Abstract"), NULL, NULL },
  { "class_scope", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
  N_("Scope"), NULL, N_("Class scope (C++ static class variable)") },

  PROP_DESC_END
};

static PropOffset umlattribute_offsets[] = {
  { "name", PROP_TYPE_STRING, offsetof(UMLAttribute, name) },
  { "type", PROP_TYPE_STRING, offsetof(UMLAttribute, type) },
  { "value", PROP_TYPE_STRING, offsetof(UMLAttribute, value) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(UMLAttribute, comment) },
  { "visibility", PROP_TYPE_ENUM, offsetof(UMLAttribute, visibility) },
  { "abstract", PROP_TYPE_BOOL, offsetof(UMLAttribute, abstract) },
  { "class_scope", PROP_TYPE_BOOL, offsetof(UMLAttribute, class_scope) },
  { NULL, 0, 0 },
};


PropDescDArrayExtra umlattribute_extra = {
  { umlattribute_props, umlattribute_offsets, "umlattribute" },
  (NewRecordFunc) uml_attribute_new,
  (FreeRecordFunc) uml_attribute_unref
};

G_DEFINE_BOXED_TYPE (UMLAttribute, uml_attribute, uml_attribute_ref, uml_attribute_unref)


UMLAttribute *
uml_attribute_new (void)
{
  UMLAttribute *attr;
  static gint next_id = 1;

  attr = g_rc_box_new0 (UMLAttribute);
  attr->internal_id = next_id++;
  attr->name = NULL;
  attr->type = NULL;
  attr->value = NULL;
  attr->comment = NULL;
  attr->visibility = DIA_UML_PUBLIC;
  attr->abstract = FALSE;
  attr->class_scope = FALSE;
#if 0 /* setup elsewhere */
  attr->left_connection = g_new0(ConnectionPoint, 1);
  attr->right_connection = g_new0(ConnectionPoint, 1);
#endif
  return attr;
}


/** Copy the data of an attribute into another, but not the connections.
 * Frees up any strings in the attribute being copied into. */
void
uml_attribute_copy_into (UMLAttribute *attr, UMLAttribute *newattr)
{
  newattr->internal_id = attr->internal_id;

  if (newattr->name != NULL) {
    g_clear_pointer (&newattr->name, g_free);
  }
  newattr->name = g_strdup (attr->name);

  if (newattr->type != NULL) {
    g_clear_pointer (&newattr->type, g_free);
  }
  newattr->type = g_strdup (attr->type);

  if (newattr->value != NULL) {
     g_clear_pointer (&newattr->value, g_free);
  }
  newattr->value = g_strdup (attr->value);

  if (newattr->comment != NULL) {
    g_clear_pointer (&newattr->comment, g_free);
  }
  newattr->comment = g_strdup (attr->comment);

  newattr->visibility = attr->visibility;
  newattr->abstract = attr->abstract;
  newattr->class_scope = attr->class_scope;
}


/** Copy an attribute's content.
 */
UMLAttribute *
uml_attribute_copy (UMLAttribute *attr)
{
  UMLAttribute *newattr;

  newattr = uml_attribute_new ();

  uml_attribute_copy_into (attr, newattr);

  return newattr;
}


UMLAttribute *
uml_attribute_ref (UMLAttribute *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  return g_rc_box_acquire (self);
}


static void
attribute_destroy (UMLAttribute *attr)
{
  g_clear_pointer (&attr->name, g_free);
  g_clear_pointer (&attr->type, g_free);
  g_clear_pointer (&attr->value, g_free);
  g_clear_pointer (&attr->comment, g_free);
#if 0 /* free'd elsewhere */
  g_clear_pointer (&attr->left_connection, g_free);
  g_clear_pointer (&attr->right_connection, g_free);
#endif
}


void
uml_attribute_unref (UMLAttribute *self)
{
  g_rc_box_release_full (self, (GDestroyNotify) attribute_destroy);
}


void
uml_attribute_write (AttributeNode  attr_node,
                     UMLAttribute  *attr,
                     DiaContext    *ctx)
{
  DataNode composite;

  composite = data_add_composite (attr_node, "umlattribute", ctx);

  data_add_string (composite_add_attribute (composite, "name"),
                   attr->name, ctx);
  data_add_string (composite_add_attribute (composite, "type"),
                   attr->type, ctx);
  data_add_string (composite_add_attribute (composite, "value"),
                   attr->value, ctx);
  data_add_string (composite_add_attribute (composite, "comment"),
                   attr->comment, ctx);
  data_add_enum (composite_add_attribute (composite, "visibility"),
                 attr->visibility, ctx);
  data_add_boolean (composite_add_attribute (composite, "abstract"),
                    attr->abstract, ctx);
  data_add_boolean (composite_add_attribute (composite, "class_scope"),
                    attr->class_scope, ctx);
}


/* Warning, the following *must* be strictly ASCII characters (or fix the
   following code for UTF-8 cleanliness */

char visible_char[] = { '+', '-', '#', ' ' };

char *
uml_attribute_get_string (UMLAttribute *attribute)
{
  int len;
  char *str;

  len = 1 + (attribute->name ? strlen (attribute->name) : 0)
          + (attribute->type ? strlen (attribute->type) : 0);
  if (attribute->name && attribute->name[0] &&
      attribute->type && attribute->type[0]) {
    len += 2;
  }
  if (attribute->value != NULL && attribute->value[0] != '\0') {
    len += 3 + strlen (attribute->value);
  }

  str = g_new0 (char, len + 1);

  str[0] = visible_char[(int) attribute->visibility];
  str[1] = 0;

  strcat (str, attribute->name ? attribute->name : "");
  if (attribute->name && attribute->name[0] &&
      attribute->type && attribute->type[0]) {
    strcat (str, ": ");
  }
  strcat (str, attribute->type ? attribute->type : "");
  if (attribute->value != NULL && attribute->value[0] != '\0') {
    strcat (str, " = ");
    strcat (str, attribute->value);
  }

  g_assert (strlen (str) == len);

  return str;
}


/*!
 * The ownership of these connection points is quite complicated. Instead of being part of
 * the UMLAttribute as one may expect at first, they are somewhat in between the DiaObject
 * (see: DiaObject::connections and the concrete user, here UMLClass) and the UMLAttribute.
 *
 * But with taking undo state mangement into account it gets even worse. Deleted (to be
 * restored connection points) live inside the UMLClassChange until they get reverted back
 * to the object *or* get free'd by umlclass_change_free()
 *
 * Since the implementation of attributes/operations being settable via StdProps there are
 * more places to keep this stuff consitent. So here comes a tolerant helper.
 *
 * NOTE: Same function as uml_operation_ensure_connection_points(),
 * with C++ it would be a template function ;)
 */
void
uml_attribute_ensure_connection_points (UMLAttribute* attr, DiaObject* obj)
{
  if (!attr->left_connection)
    attr->left_connection = g_new0(ConnectionPoint,1);
  attr->left_connection->object = obj;
  if (!attr->right_connection)
    attr->right_connection = g_new0(ConnectionPoint,1);
  attr->right_connection->object = obj;
}
