/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Copyright (C) 2009 Hans Breuer
 *
 * Property types for dictonaries.
 * These dictionaries are simple key/value pairs both of type string.
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

#include <gtk/gtk.h>
#define WIDGET GtkWidget
#include "widgets.h"
#include "properties.h"
#include "propinternals.h"

typedef struct _WellKnownKeys {
  const gchar *name;
  const gchar *display_name;
} WellKnownKeys;

/* a list of wel known keys with their display name */
static WellKnownKeys _well_known[] = {
  { "author", N_("Author") },
  { "id", N_("Identifier") },
  { "creation", N_("Creation date") },
  { "modification", N_("Modification date") },
  { "url", N_("URL") },
  { NULL, NULL }
};
 
static DictProperty *
dictprop_new(const PropDescription *pdesc, PropDescToPropPredicate reason)
{
  DictProperty *prop = g_new0(DictProperty,1);

  initialize_property(&prop->common, pdesc, reason);  
  prop->dict = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  return prop;
}

static void
dictprop_free(DictProperty *prop) 
{
  if (prop->dict)
    g_hash_table_destroy(prop->dict);
  g_free(prop);
} 

static void
_keyvalue_copy (gpointer key,
                gpointer value,
                gpointer user_data)
{
  gchar *name = (gchar *)key;
  gchar *val = (gchar *)value;
  GHashTable *dest = (GHashTable *)user_data;
  
  g_hash_table_insert (dest, g_strdup (name), g_strdup (val));
}
static DictProperty *
dictprop_copy(DictProperty *src) 
{
  DictProperty *prop = 
    (DictProperty *)src->common.ops->new_prop(src->common.descr,
                                              src->common.reason);
  if (src->dict)
    g_hash_table_foreach (src->dict, _keyvalue_copy, prop->dict);
  
  return prop;
}

static void 
dictprop_load(DictProperty *prop, AttributeNode attr, DataNode data)
{
  DataNode kv;
  guint nvals = attribute_num_data(attr);
  if (!nvals)
    return;

  kv = attribute_first_data (data);
  while (kv) {
    xmlChar *key = xmlGetProp(kv, (const xmlChar *)"name");

    if (key) {
      gchar *value = data_string(attribute_first_data (kv));
      if (value)
        g_hash_table_insert (prop->dict, g_strdup((gchar *)key), value);
    } else {
      g_warning ("Dictionary key missing");
    }
    kv = data_next(kv);
  }
}

static GHashTable *
_hash_dup (const GHashTable *src)
{
  GHashTable *dest = NULL;
  if (src) {
    dest = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    g_hash_table_foreach ((GHashTable *)src, _keyvalue_copy, dest);
  }
  return dest;  
}

static void
_keyvalue_save (gpointer key,
                gpointer value,
                gpointer user_data)
{
  gchar *name = (gchar *)key;
  gchar *val = (gchar *)value;
  ObjectNode node = (ObjectNode)user_data;

  data_add_string(new_attribute(node, name), val);
}
static void 
dictprop_save(DictProperty *prop, AttributeNode attr) 
{
  ObjectNode composite = data_add_composite(attr, "dict");
  if (prop->dict)
    g_hash_table_foreach (prop->dict, _keyvalue_save, composite); 
}

static void 
dictprop_get_from_offset(DictProperty *prop,
                         void *base, guint offset, guint offset2) 
{
  prop->dict = _hash_dup (struct_member(base,offset,GHashTable *));
}

static void 
dictprop_set_from_offset(DictProperty *prop,
                         void *base, guint offset, guint offset2)
{
  GHashTable *dest = struct_member(base,offset,GHashTable *);
  if (dest)
    g_hash_table_destroy (dest);
  struct_member(base,offset, GHashTable *) = _hash_dup (prop->dict);
}

static const PropertyOps dictprop_ops = {
  (PropertyType_New) dictprop_new,
  (PropertyType_Free) dictprop_free,
  (PropertyType_Copy) dictprop_copy,
  (PropertyType_Load) dictprop_load,
  (PropertyType_Save) dictprop_save,
  (PropertyType_GetWidget) noopprop_get_widget,
  (PropertyType_ResetWidget) noopprop_reset_widget,
  (PropertyType_SetFromWidget) noopprop_set_from_widget,

  (PropertyType_CanMerge) noopprop_can_merge,
  (PropertyType_GetFromOffset) dictprop_get_from_offset,
  (PropertyType_SetFromOffset) dictprop_set_from_offset
};

void 
prop_dicttypes_register(void)
{
  prop_type_register(PROP_TYPE_DICT, &dictprop_ops);
}

GHashTable *
data_dict (DataNode data)
{
  GHashTable *ht = NULL;
  int nvals = attribute_num_data (data);
  
  if (nvals) {
    DataNode kv = attribute_first_data (data);
    gchar *val = NULL;

    ht = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
    while (kv) {
      xmlChar *key = xmlGetProp(kv, (const xmlChar *)"name");

      if (key) {
        val = data_string (attribute_first_data (kv));
        if (val)
          g_hash_table_insert (ht, g_strdup ((gchar *)key), val);
	xmlFree (key);
      }
      kv = data_next (kv);
    }
  }
  return ht;
}

void
data_add_dict (AttributeNode attr, GHashTable *data)
{
  ObjectNode composite = data_add_composite(attr, "dict");

  g_hash_table_foreach (data, _keyvalue_save, composite); 
}
