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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>

#include "uml.h"
#include "dia-uml-attribute.h"
#include "list/dia-list-data.h"
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
  { "name", PROP_TYPE_STRING, offsetof(DiaUmlAttribute, name) },
  { "type", PROP_TYPE_STRING, offsetof(DiaUmlAttribute, type) },
  { "value", PROP_TYPE_STRING, offsetof(DiaUmlAttribute, value) },
  { "comment", PROP_TYPE_MULTISTRING, offsetof(DiaUmlAttribute, comment) },
  { "visibility", PROP_TYPE_ENUM, offsetof(DiaUmlAttribute, visibility) },
  { "abstract", PROP_TYPE_BOOL, offsetof(DiaUmlAttribute, abstract) },
  { "class_scope", PROP_TYPE_BOOL, offsetof(DiaUmlAttribute, class_scope) },
  { NULL, 0, 0 },
};


PropDescDArrayExtra umlattribute_extra = {
  { umlattribute_props, umlattribute_offsets, "umlattribute" },
  (NewRecordFunc) dia_uml_attribute_new,
  (FreeRecordFunc) g_object_unref
};

static void
dia_uml_attribute_list_data_init (DiaListDataInterface *iface);

G_DEFINE_TYPE_WITH_CODE (DiaUmlAttribute, dia_uml_attribute, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (DIA_TYPE_LIST_DATA,
                                                dia_uml_attribute_list_data_init))

enum {
  PROP_NAME = 1,
  PROP_TYPE,
  PROP_VALUE,
  PROP_COMMENT,
  PROP_VISIBILITY,
  PROP_CLASS_SCOPE,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];


static void
dia_uml_attribute_finalize (GObject *object)
{
  DiaUmlAttribute *self = DIA_UML_ATTRIBUTE (object);

  g_free (self->name);
  g_free (self->type);
  g_free (self->value);
  g_free (self->comment);

  /* free'd elsewhere */
  /*
  g_free(attr->left_connection);
  g_free(attr->right_connection);
  */
}


static void
dia_uml_attribute_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DiaUmlAttribute *self = DIA_UML_ATTRIBUTE (object);

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
    case PROP_VALUE:
      self->value = g_value_dup_string (value);
      g_object_notify_by_pspec (object, properties[PROP_VALUE]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    case PROP_COMMENT:
      self->comment = g_value_dup_string (value);
      g_object_notify_by_pspec (object, properties[PROP_COMMENT]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    case PROP_VISIBILITY:
      self->visibility = g_value_get_int (value);
      g_object_notify_by_pspec (object, properties[PROP_VISIBILITY]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    case PROP_CLASS_SCOPE:
      self->class_scope = g_value_get_boolean (value);
      g_object_notify_by_pspec (object, properties[PROP_CLASS_SCOPE]);
      dia_list_data_changed (DIA_LIST_DATA (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_uml_attribute_get_property (GObject    *object,
                                guint       property_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  DiaUmlAttribute *self = DIA_UML_ATTRIBUTE (object);

  switch (property_id) {
    case PROP_NAME:
      g_value_set_string (value, self->name);
      break;
    case PROP_TYPE:
      g_value_set_string (value, self->type);
      break;
    case PROP_VALUE:
      g_value_set_string (value, self->type);
      break;
    case PROP_COMMENT:
      g_value_set_string (value, self->comment);
      break;
    case PROP_VISIBILITY:
      g_value_set_int (value, self->visibility);
      break;
    case PROP_CLASS_SCOPE:
      g_value_set_boolean (value, self->class_scope);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static const gchar *
format (DiaListData *self)
{
  return dia_uml_attribute_format (DIA_UML_ATTRIBUTE (self));
}

static void
dia_uml_attribute_list_data_init (DiaListDataInterface *iface)
{
  iface->format = format;
}

static void
dia_uml_attribute_class_init (DiaUmlAttributeClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_uml_attribute_finalize;
  object_class->set_property = dia_uml_attribute_set_property;
  object_class->get_property = dia_uml_attribute_get_property;

  properties[PROP_NAME] = g_param_spec_string ("name",
                                               "Name",
                                               "Function name",
                                               "",
                                               G_PARAM_READWRITE);
  properties[PROP_TYPE] = g_param_spec_string ("type",
                                               "Type",
                                               "Attribute type",
                                               "",
                                               G_PARAM_READWRITE);
  properties[PROP_VALUE] = g_param_spec_string ("value",
                                                "Value",
                                                "Default value",
                                                "",
                                                G_PARAM_READWRITE);
  properties[PROP_COMMENT] = g_param_spec_string ("comment",
                                                  "Comment",
                                                  "Comment",
                                                  "",
                                                  G_PARAM_READWRITE);
  properties[PROP_VISIBILITY] = g_param_spec_int ("visibility",
                                                  "Visibility",
                                                  "Visibility",
                                                  UML_PUBLIC,
                                                  UML_IMPLEMENTATION,
                                                  UML_PUBLIC,
                                                  G_PARAM_READWRITE);
  properties[PROP_CLASS_SCOPE] = g_param_spec_boolean ("class-scope",
                                                       "Class scope",
                                                       "Class scope",
                                                       FALSE,
                                                       G_PARAM_READWRITE);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);
}

static void
dia_uml_attribute_init (DiaUmlAttribute *self)
{
  static gint next_id = 1;

  self->internal_id = next_id++;
  self->name = g_strdup("");
  self->type = g_strdup("");
  self->value = NULL;
  self->comment = g_strdup("");
  self->visibility = UML_PUBLIC;
  self->abstract = FALSE;
  self->class_scope = FALSE;
  /* setup elsewhere */
  /*
  attr->left_connection = g_new0(ConnectionPoint, 1);
  attr->right_connection = g_new0(ConnectionPoint, 1);
  */
}

DiaUmlAttribute *
dia_uml_attribute_new ()
{
  return g_object_new (DIA_UML_TYPE_ATTRIBUTE, NULL);
}

/** Copy the data of an attribute into another, but not the connections. 
 * Frees up any strings in the attribute being copied into. */
DiaUmlAttribute *
dia_uml_attribute_copy (DiaUmlAttribute *self)
{
  DiaUmlAttribute *newattr = g_object_new (DIA_UML_TYPE_ATTRIBUTE, NULL);

  newattr->internal_id = self->internal_id;
  g_free(newattr->name);
  newattr->name = g_strdup(self->name);
  g_free(newattr->type);
  newattr->type = g_strdup(self->type);

  g_free(newattr->value);
  newattr->value = g_strdup(self->value);
  
  g_free(newattr->comment);
  newattr->comment = g_strdup (self->comment);
  
  newattr->visibility = self->visibility;
  newattr->abstract = self->abstract;
  newattr->class_scope = self->class_scope;

  return newattr;
}

void
uml_attribute_write(AttributeNode attr_node, DiaUmlAttribute *attr, DiaContext *ctx)
{
  DataNode composite;

  composite = data_add_composite(attr_node, "umlattribute", ctx);

  data_add_string(composite_add_attribute(composite, "name"),
		  attr->name, ctx);
  data_add_string(composite_add_attribute(composite, "type"),
		  attr->type, ctx);
  data_add_string(composite_add_attribute(composite, "value"),
		  attr->value, ctx);
  data_add_string(composite_add_attribute(composite, "comment"),
		  attr->comment, ctx);
  data_add_enum(composite_add_attribute(composite, "visibility"),
		attr->visibility, ctx);
  data_add_boolean(composite_add_attribute(composite, "abstract"),
		  attr->abstract, ctx);
  data_add_boolean(composite_add_attribute(composite, "class_scope"),
		  attr->class_scope, ctx);
}

/* Warning, the following *must* be strictly ASCII characters (or fix the 
   following code for UTF-8 cleanliness */

char visible_char[] = { '+', '-', '#', ' ' };

char *
dia_uml_attribute_format (DiaUmlAttribute *attribute)
{
  int len;
  char *str;

  len = 1 + (attribute->name ? strlen (attribute->name) : 0) 
          + (attribute->type ? strlen (attribute->type) : 0);
  if (attribute->name && attribute->name[0] && attribute->type && attribute->type[0]) {
    len += 2;
  }
  if (attribute->value != NULL && attribute->value[0] != '\0') {
    len += 3 + strlen (attribute->value);
  }
  
  str = g_malloc (sizeof (char) * (len + 1));

  str[0] = visible_char[(int) attribute->visibility];
  str[1] = 0;

  strcat (str, attribute->name ? attribute->name : "");
  if (attribute->name && attribute->name[0] && attribute->type && attribute->type[0]) {
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
 * the DiaUmlAttribute as one may expect at first, they are somewhat in between the DiaObject
 * (see: DiaObject::connections and the concrete user, here UMLClass) and the DiaUmlAttribute.
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
dia_uml_attribute_ensure_connection_points (DiaUmlAttribute* attr, DiaObject* obj)
{
  if (!attr->left_connection)
    attr->left_connection = g_new0(ConnectionPoint,1);
  attr->left_connection->object = obj;
  if (!attr->right_connection)
    attr->right_connection = g_new0(ConnectionPoint,1);
  attr->right_connection->object = obj;
}
