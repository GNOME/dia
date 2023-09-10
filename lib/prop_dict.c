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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>
#define WIDGET GtkWidget
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
  g_clear_pointer (&prop, g_free);
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
dictprop_load(DictProperty *prop, AttributeNode attr, DataNode data, DiaContext *ctx)
{
  DataNode kv;
  guint nvals = attribute_num_data(attr);
  if (!nvals)
    return;

  kv = attribute_first_data (data);
  while (kv) {
    xmlChar *key = xmlGetProp(kv, (const xmlChar *)"name");

    if (key) {
      gchar *value = data_string(attribute_first_data (kv), ctx);
      if (value)
        g_hash_table_insert (prop->dict, g_strdup((gchar *)key), value);
      xmlFree (key);
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
typedef struct
{
  ObjectNode  node;
  DiaContext *ctx;
} DictUserData;
static void
_keyvalue_save (gpointer key,
                gpointer value,
                gpointer user_data)
{
  DictUserData *ud = (DictUserData *)user_data;
  gchar *name = (gchar *)key;
  gchar *val = (gchar *)value;
  ObjectNode node = ud->node;
  DiaContext *ctx = ud->ctx;

  data_add_string(new_attribute(node, name), val, ctx);
}
static void
dictprop_save(DictProperty *prop, AttributeNode attr, DiaContext *ctx)
{
  DictUserData ud;
  ud.node = data_add_composite(attr, "dict", ctx);
  ud.ctx = ctx;
  if (prop->dict)
    g_hash_table_foreach (prop->dict, _keyvalue_save, &ud);
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

/* GUI stuff */
#define TREE_MODEL_KEY "dict-tree-model"
enum {
  KEY_COLUMN,
  VALUE_COLUMN,
  IS_EDITABLE_COLUMN,
  NUM_COLUMNS
};

static void
_keyvalue_fill_model (gpointer key,
                      gpointer value,
                      gpointer user_data)
{
  gchar *name = (gchar *)key;
  gchar *val = (gchar *)value;
  GtkTreeStore *model = (GtkTreeStore *)user_data;
  GtkTreeIter iter;

  gtk_tree_store_append (model, &iter, NULL);
  gtk_tree_store_set (model, &iter,
                      KEY_COLUMN, name,
		      VALUE_COLUMN, val,
		      IS_EDITABLE_COLUMN, TRUE,
		      -1);
}

static GtkTreeModel *
_create_model (DictProperty *prop)
{
  GtkTreeStore *model;

  model = gtk_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN);

  return GTK_TREE_MODEL (model);
}
static void
edited (GtkCellRendererText *cell,
	gchar               *path_string,
	gchar               *new_text,
	gpointer             data)
{
  GtkTreeModel *model = GTK_TREE_MODEL (data);
  GtkTreeIter iter;
  GtkTreePath *path = gtk_tree_path_new_from_string (path_string);

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_store_set (GTK_TREE_STORE (model), &iter, VALUE_COLUMN, new_text, -1);
  g_object_set_data (G_OBJECT (model), "modified", GINT_TO_POINTER (1));
  gtk_tree_path_free (path);
}

static GtkWidget *
_create_view (GtkTreeModel *model)
{
  GtkWidget *widget;
  GtkWidget *tree_view;
  GtkTreeViewColumn *col;
  GtkCellRenderer *renderer;

  widget = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_set_vexpand (widget, TRUE);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (widget), GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (widget), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  tree_view = gtk_tree_view_new_with_model (model);
  gtk_widget_set_vexpand (tree_view, TRUE);
  gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree_view), TRUE);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree_view), TRUE);

  col = gtk_tree_view_column_new_with_attributes(
		   _("Key"), gtk_cell_renderer_text_new (),
		   "text", KEY_COLUMN, NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);
  gtk_tree_view_column_set_sort_column_id (col, KEY_COLUMN);

  renderer = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes(
		   _("Value"), renderer,
		   "text", VALUE_COLUMN,
		   "editable", IS_EDITABLE_COLUMN, NULL);
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), model);
  gtk_tree_view_append_column (GTK_TREE_VIEW (tree_view), col);
  gtk_tree_view_column_set_sort_column_id (col, VALUE_COLUMN);

  gtk_container_add (GTK_CONTAINER (widget), tree_view);

  g_object_set_data (G_OBJECT (widget), TREE_MODEL_KEY, model);

  return widget;
}
static GtkWidget *
dictprop_get_widget (DictProperty *prop, PropDialog *dialog)
{
  GtkWidget *ret;
  ret = _create_view (_create_model (prop));
  gtk_widget_show_all (ret);
  /* prophandler_connect(&prop->common, G_OBJECT(ret), "value_changed");
   * It's not so easy, we are maintaining our own changed state via edited signal
   */
  return ret;
}
static void
dictprop_reset_widget(DictProperty *prop, GtkWidget *widget)
{
  GtkTreeModel *model = g_object_get_data (G_OBJECT (widget), TREE_MODEL_KEY);
  GtkTreeIter iter;
  WellKnownKeys *wkk;

  /* should it be empty */
  gtk_tree_store_clear (GTK_TREE_STORE (model));

  /* add everything we have */
  if (!prop->dict)
    prop->dict = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  g_hash_table_foreach (prop->dict, _keyvalue_fill_model, model);

  g_object_set_data (G_OBJECT (model), "modified", GINT_TO_POINTER (0));

  /* also add the well known ? */
  for (wkk = _well_known; wkk->name != NULL; ++wkk) {
    gchar *val;

    if (g_hash_table_lookup (prop->dict, wkk->name))
      continue;

    gtk_tree_store_append (GTK_TREE_STORE (model), &iter, NULL);
    val = g_hash_table_lookup (prop->dict, wkk->name);
    gtk_tree_store_set (GTK_TREE_STORE (model), &iter,
                        KEY_COLUMN, wkk->name,
			VALUE_COLUMN, val ? val : "",
			IS_EDITABLE_COLUMN, TRUE,
			-1);
  }
}
static void
dictprop_set_from_widget(DictProperty *prop, GtkWidget *widget)
{
  GtkTreeModel *model = g_object_get_data (G_OBJECT (widget), TREE_MODEL_KEY);
  GtkTreeIter   iter;

  if (gtk_tree_model_get_iter_first (model, &iter)) {
    gchar *key, *val;

    do {
      gtk_tree_model_get (model, &iter,
                          KEY_COLUMN, &key,
			  VALUE_COLUMN, &val,
			  -1);

      if (key && val) {
	if (!prop->dict)
	  prop->dict = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

	if (strlen (val))
          g_hash_table_insert (prop->dict, key, val);
	else /* delete stuff which has no value any longer */
	  g_hash_table_remove (prop->dict, key);
	/* enough to replace prophandler_connect() ? */
	if (g_object_get_data (G_OBJECT (model), "modified"))
	  prop->common.experience &= ~PXP_NOTSET;
      }
    } while (gtk_tree_model_iter_next (model, &iter));
  }
}
static const PropertyOps dictprop_ops = {
  (PropertyType_New) dictprop_new,
  (PropertyType_Free) dictprop_free,
  (PropertyType_Copy) dictprop_copy,
  (PropertyType_Load) dictprop_load,
  (PropertyType_Save) dictprop_save,
  (PropertyType_GetWidget) dictprop_get_widget,
  (PropertyType_ResetWidget) dictprop_reset_widget,
  (PropertyType_SetFromWidget) dictprop_set_from_widget,

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
data_dict (DataNode data, DiaContext *ctx)
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
        val = data_string (attribute_first_data (kv), ctx);
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
data_add_dict (AttributeNode attr, GHashTable *data, DiaContext *ctx)
{
  DictUserData ud;
  ud.node = data_add_composite(attr, "dict", ctx);
  ud.ctx = ctx;

  g_hash_table_foreach (data, _keyvalue_save, &ud);
}
