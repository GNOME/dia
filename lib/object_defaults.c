/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * object_defaults.c : manage default properties of dia objects
 *	The serialization is done with standard object methods in
 *	a diagram compatible format.
 *
 * Copyright (C) 2002 Hans Breuer
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

#include <stdlib.h> /* atoi() */
#include <string.h>

#include <glib.h>

#include <libxml/tree.h>
#include "dia_xml.h"
#include "object.h"
#include "diacontext.h"
#include "dia_dirs.h"
#include "propinternals.h"
#include "dia-io.h"

static GHashTable *defaults_hash = NULL;
static gboolean object_default_create_lazy = FALSE;

static void
_obj_create (gpointer key,
             gpointer value,
             gpointer user_data)
{
  gchar *name = (gchar *)key;
  DiaObjectType *type = (DiaObjectType *)value;
  GHashTable *ht = (GHashTable *) user_data;
  DiaObject *obj;
  Point startpoint = {0.0,0.0};
  Handle *handle1,*handle2;

  g_assert (g_hash_table_lookup (ht, name) == NULL);

  /* at least 'Group' has no ops */
  if (!type->ops)
    return;

  /* the custom objects needs extra_data */
  obj = type->ops->create(&startpoint, type->default_user_data, &handle1,&handle2);
  if (!obj)
    g_warning ("Failed to create default object for '%s'", name);
  else if (0 != strcmp (obj->type->name, name))
    object_destroy (obj); /* Skip over 'compatibility' names/objects */
  else
    g_hash_table_insert(ht, obj->type->name, obj);
}


static void
_obj_destroy (gpointer val)
{
  DiaObject *obj = (DiaObject *)val;

  object_destroy (obj);

  g_clear_pointer (&obj, g_free);
}


/**
 * dia_object_defaults_load:
 * @filename: the file to load from or NULL for default
 * @create_lazy: if %FALSE creates default objects for
 *             every known type. Otherwise default objects
 *             are created on demand
 * @ctx: The context in which this function is called
 *
 * Create all the default objects.
 */
gboolean
dia_object_defaults_load (const gchar *filename, gboolean create_lazy, DiaContext *ctx)
{
  xmlDocPtr doc;
  xmlNsPtr name_space;
  ObjectNode obj_node, layer_node;

  object_default_create_lazy = create_lazy;

  if (!defaults_hash) {
    defaults_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                           NULL, _obj_destroy);

    if (!create_lazy) {
      object_registry_foreach (_obj_create, defaults_hash);
    }
  }


  /* overload properties from file */
  if (!filename) {
    char *default_filename = dia_config_filename ("defaults.dia");

    dia_context_set_filename (ctx, default_filename);
    if (g_file_test (default_filename, G_FILE_TEST_EXISTS)) {
      doc = dia_io_load_document (default_filename, ctx, NULL);
    } else {
      doc = NULL;
    }
    g_clear_pointer (&default_filename, g_free);
  } else {
    dia_context_set_filename (ctx, filename);
    doc = dia_io_load_document (filename, ctx, NULL);
  }

  if (!doc) {
    return FALSE;
  }

  name_space = xmlSearchNs (doc, doc->xmlRootNode, (const xmlChar *) "dia");
  if (xmlStrcmp (doc->xmlRootNode->name, (const xmlChar *) "diagram")
      || (name_space == NULL)) {
    dia_context_add_message (ctx,
                             _("Error loading defaults '%s'.\n"
                            "Not a Dia diagram file."),
                            filename);
    xmlFreeDoc (doc);
    return FALSE;
  }

  layer_node = doc->xmlRootNode->xmlChildrenNode;
  while (layer_node) {
    if (   !xmlIsBlankNode (layer_node)
        && 0 == xmlStrcmp (layer_node->name, (const xmlChar *) "layer")) {
      obj_node = layer_node->xmlChildrenNode;

      while (obj_node) {
        if (!xmlIsBlankNode (obj_node)
            && 0 == xmlStrcmp (obj_node->name, (const xmlChar *) "object")) {
          char *typestr = (char *) xmlGetProp(obj_node, (const xmlChar *) "type");
          char *version = (char *) xmlGetProp(obj_node, (const xmlChar *) "version");
          if (typestr) {
            DiaObject *obj = g_hash_table_lookup (defaults_hash, typestr);
            if (!obj) {
              if (!create_lazy) {
                g_warning ("Unknown object '%s' while reading '%s'",
                           typestr, filename);
              } else {
                DiaObjectType *type = object_get_type (typestr);
                if (type) {
                  obj = type->ops->load (obj_node,
                                         version ? atoi(version) : 0,
                                         ctx);
                }
                if (obj) {
                  g_hash_table_insert (defaults_hash,
                                       obj->type->name,
                                       obj);
                }
              }
            } else {
#if 0 /* lots of complaining about missing attributes */
              object_load_props(obj, obj_node, ctx); /* leaks ?? */
#else
              DiaObject *def_obj;
              def_obj = obj->type->ops->load (obj_node,
                                              version ? atoi(version) : 0,
                                              ctx);
              object_copy_props (obj, def_obj, TRUE);
              object_destroy (def_obj);
      #endif
            }
            if (version) {
              xmlFree (version);
            }
            xmlFree (typestr);
          }
        }
        obj_node = obj_node->next;
      }
    }
    layer_node = layer_node->next;
  }
  xmlFreeDoc (doc);
  return TRUE;
}

/**
 * dia_object_default_get :
 * @param type The type of the object for which you want the defaults object.
 * @param user_data unused
 *
 * Allows to edit one defaults object properties
 */
DiaObject *
dia_object_default_get (const DiaObjectType *type, gpointer user_data)
{
  DiaObject *obj;

  obj = g_hash_table_lookup (defaults_hash, type->name);
  if (!obj && object_default_create_lazy)
    {
      Point startpoint = {0.0,0.0};
      Handle *handle1,*handle2;

      /* at least 'Group' has no ops */
      if (!type->ops)
	return NULL;

      /* the custom objects needs extra_data */
      obj = type->ops->create(&startpoint,
                              type->default_user_data,
			      &handle1,&handle2);
      if (obj)
        g_hash_table_insert (defaults_hash, obj->type->name, obj);
    }

  return obj;
}

static gboolean
pdtpp_standard_or_defaults (const PropDescription *pdesc)
{
  return (   (pdesc->flags & PROP_FLAG_NO_DEFAULTS) == 0
          && (pdesc->flags & PROP_FLAG_STANDARD) == 0);
}


/**
 * dia_object_default_create:
 * @type: The objects type
 * @startpoint: The left upper corner
 * @user_data: ???
 * @handle1: ???
 * @handle2: ???
 *
 * Create an object respecting defaults if available
 *
 * Returns: A newly created object.
 *
 * Since: dawn-of-time
 */
DiaObject *
dia_object_default_create (const DiaObjectType  *type,
                           Point                *startpoint,
                           void                 *user_data,
                           Handle              **handle1,
                           Handle              **handle2)
{
  const DiaObject *def_obj;
  DiaObject *obj;

  g_return_val_if_fail (type != NULL, NULL);

  /* don't use dia_object_default_get() as it would insert the object into the hashtable (store defaults without being asked for it) */
  def_obj = g_hash_table_lookup (defaults_hash, type->name);
  if (def_obj) {
    /* copy properties to new object, but keep position */
    obj = type->ops->create (startpoint, user_data, handle1, handle2);
    if (obj) {
      GPtrArray *props = prop_list_from_descs (object_get_prop_descriptions (def_obj),
                                               pdtpp_standard_or_defaults);
      dia_object_get_properties ((DiaObject *) def_obj, props);
      dia_object_set_properties (obj, props);
      dia_object_move (obj, startpoint);
      prop_list_free (props);
    }
  } else {
    obj = type->ops->create (startpoint, user_data, handle1, handle2);
  }

  return obj;
}

typedef struct _MyLayerInfo MyLayerInfo;
struct _MyLayerInfo
{
  Point      pos;
  xmlNodePtr node;
};

typedef struct _MyRootInfo MyRootInfo;
struct _MyRootInfo
{
  xmlNodePtr  node;
  gchar      *filename;
  GHashTable *layer_hash;
  xmlNs      *name_space;
  gint        obj_nr;
  DiaContext *ctx;
};

static void
_obj_store (gpointer key,
            gpointer value,
            gpointer user_data)
{
  gchar *name = (gchar *)key;
  DiaObject *obj = (DiaObject *)value;
  MyRootInfo *ri = (MyRootInfo *)user_data;
  ObjectNode obj_node;
  gchar *layer_name;
  gchar buffer[31];
  gchar *p;
  MyLayerInfo *li;

  /* fires if you have messed up the hash keys,
   * e.g. by using non permanent memory */
  g_assert (0 == strcmp (obj->type->name, name));

  p = strstr (name, " - ");
  if (p) {
    if (p > name)
      layer_name = g_strndup (name, p - name);
    else
      layer_name = g_strdup("NULL");
  }
  else
    layer_name = g_strdup ("default");

  li = g_hash_table_lookup (ri->layer_hash, layer_name);
  if (!li) {
    li = g_new0 (MyLayerInfo, 1);
    li->node = xmlNewChild(ri->node, ri->name_space, (const xmlChar *)"layer", NULL);
    xmlSetProp(li->node, (const xmlChar *)"name", (xmlChar *)layer_name);
    xmlSetProp(li->node, (const xmlChar *)"visible", (const xmlChar *)"false");
    li->pos.x = li->pos.y = 0.0;
    g_hash_table_insert (ri->layer_hash, layer_name, li);
  } else {
    g_clear_pointer (&layer_name, g_free);
  }

  obj_node = xmlNewChild(li->node, NULL, (const xmlChar *)"object", NULL);
  xmlSetProp(obj_node, (const xmlChar *)"type", (xmlChar *) obj->type->name);

  g_snprintf(buffer, 30, "%d", obj->type->version);
  xmlSetProp(obj_node, (const xmlChar *)"version", (xmlChar *)buffer);

  g_snprintf(buffer, 30, "O%d", ri->obj_nr++);
  xmlSetProp(obj_node, (const xmlChar *)"id", (xmlChar *)buffer);

  /* if it looks like intdata store it as well */
  if (   GPOINTER_TO_INT(obj->type->default_user_data) > 0
      && GPOINTER_TO_INT(obj->type->default_user_data) < 0xFF) {
    g_snprintf(buffer, 30, "%d", GPOINTER_TO_INT(obj->type->default_user_data));
    xmlSetProp(obj_node, (const xmlChar *)"intdata", (xmlChar *)buffer);
  }

  dia_object_move (obj, &(li->pos));
  /* saving every property of the object */
  obj->type->ops->save (obj, obj_node, ri->ctx);

  /* arrange following objects below */
  li->pos.y += (obj->bounding_box.bottom - obj->bounding_box.top + 1.0);
}


/**
 * dia_object_defaults_save:
 *
 * Saves all the currently created default objects into a
 * valid diagram file. All the objects are placed into
 * separate invisible layers.
 */
gboolean
dia_object_defaults_save (const char *filename, DiaContext *ctx)
{
  MyRootInfo ni;
  xmlDocPtr doc;
  gboolean ret;
  char *real_filename;

  if (!filename) {
    real_filename = dia_config_filename ("defaults.dia");
  } else {
    real_filename = g_strdup (filename);
  }

  doc = xmlNewDoc ((const xmlChar *) "1.0");
  doc->encoding = xmlStrdup ((const xmlChar *) "UTF-8");
  doc->xmlRootNode = xmlNewDocNode (doc, NULL, (const xmlChar *) "diagram", NULL);

  ni.name_space = xmlNewNs (doc->xmlRootNode,
                            (const xmlChar *) DIA_XML_NAME_SPACE_BASE,
                            (const xmlChar *) "dia");
  xmlSetNs (doc->xmlRootNode, ni.name_space);

  ni.obj_nr = 0;
  ni.node = doc->xmlRootNode;
  ni.filename = real_filename;
  ni.ctx = ctx;
  ni.layer_hash = g_hash_table_new_full (g_str_hash, g_str_equal,
                                         g_free, g_free);

  g_hash_table_foreach (defaults_hash, _obj_store, &ni);

  ret = dia_io_save_document (real_filename, doc, FALSE, ctx);

  g_clear_pointer (&real_filename, g_free);
  g_clear_pointer (&doc, xmlFreeDoc);

  g_hash_table_destroy (ni.layer_hash);

  return ret;
}
