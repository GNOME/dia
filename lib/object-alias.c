/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * object-alias.c : a way to correct typos in object names
 *
 * Copyright (C) 2011 Hans Breuer
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

/* To map an old name to a new object type the alias indirection is used.
 * Starting with the old object from file the old name will persist. So there
 * even should be no issue with forward compatibility (except if the object
 * itself has issues, like an updated version).
 *
 * An extra feature would be the availability of construction only properties.
 * These would be applied to the real object after creation and should not be
 * accessible to any user of the alias name, i.e. neither be listed by
 * _alias_describe_props and _alias_get_props. But they additionally need to be
 * filtered in _alias_set_props, because e.g. for groups some properties might
 * get tried to be applied which do not come from the original set(?).
 *
 * The sheet code already relies on the correct order of type registration and
 * access, so we can assume the target type is registered before the alias is
 * created.
 */

#include <stdlib.h> /* atoi() */
#include <string.h>

#include <glib.h>

#include <libxml/tree.h>
#include "dia_xml.h"
#include "object.h"
#include "message.h"
#include "dia_dirs.h"
#include "propinternals.h"

#include "object-alias.h"

/* DiaObjectType _alias_type must be dynamically
 *
 * The hash table is mapping the alias name to the real type.
 */
GHashTable *_alias_types_ht = NULL;

/* It should be possible to modify the original object after creation to point
 * back to the alias type, at least as long as there are no dedicated object_ops
 */
#undef MODIFY_OBJECTS_TYPE

static void *
_alias_lookup (const char *name)
{
  if (!_alias_types_ht)
    return NULL;

  return g_hash_table_lookup (_alias_types_ht, name);
}

/* type_ops */
static DiaObject *
_alias_create (Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2);
static DiaObject *
_alias_load (ObjectNode obj_node, int version, const char *filename, DiaContext *ctx);
static void
_alias_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx);

static ObjectTypeOps _alias_type_ops =
{
  (CreateFunc) _alias_create,
  (LoadFunc)   _alias_load, /* can't use object_load_using_properties, signature mismatch */
  (SaveFunc)   _alias_save, /* overwrite for filename normalization */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

/*! factory function */
static DiaObject *
_alias_create (Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  DiaObject *obj;
  DiaObjectType *alias_type = (DiaObjectType *)user_data;
  DiaObjectType *real_type;

  g_return_val_if_fail (alias_type != NULL && alias_type->name != NULL, NULL);

  real_type = _alias_lookup (alias_type->name);
  if (!real_type)
    return NULL;
  g_return_val_if_fail (real_type->ops != &_alias_type_ops, NULL);

  obj = real_type->ops->create (startpoint, real_type->default_user_data, handle1, handle2);
  if (!obj)
    return NULL;
#ifdef MODIFY_OBJECTS_TYPE
  /* now modify the object for some behavior change */
  obj->type = alias_type; /* also changes the name */
#endif

  return obj;
}

static DiaObject *
_alias_load (ObjectNode obj_node, int version, const char *filename, DiaContext *ctx)
{
  DiaObject *obj = NULL;
  xmlChar *str;

  str = xmlGetProp(obj_node, (const xmlChar *)"type");
  if (str) {
    DiaObjectType *real_type = _alias_lookup ((char *)str);
    Point apoint = {0, 0};
    Handle *h1, *h2;
    /* can not use real_type->ops->load (obj_node, ...) because the
     * typename used from obj_node is wrong for the real object ...
     * just another reason to pass in the exlplicit this-pointer in every method.
     */
    obj = real_type->ops->create (&apoint, real_type->default_user_data, &h1, &h2);
    object_load_props (obj, obj_node, ctx);
#ifdef MODIFY_OBJECTS_TYPE
    /* now modify the object for some behavior change */
    obj->type = object_get_type ((char *)str); /* also changes the name */
#endif
    xmlFree(str);
  }
  return obj;
}

static void
_alias_save (DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  object_save_using_properties (obj, obj_node, ctx);
}

void
object_register_alias_type (DiaObjectType *type, ObjectNode alias_node)
{
  xmlChar *name;

  /* real type must be available before the alias can be created */
  g_return_if_fail (type != NULL && object_get_type (type->name) != NULL);

  name = xmlGetProp(alias_node, (const xmlChar *)"name");
  if (name) {
    DiaObjectType *alias_type = g_new0 (DiaObjectType, 1);

    alias_type->name = g_strdup ((char *)name);
    alias_type->ops = &_alias_type_ops;
    alias_type->version = type->version; /* really? */
    alias_type->pixmap = type->pixmap;
    alias_type->pixmap_file = type->pixmap_file ;
    alias_type->default_user_data = alias_type; /* _create has no self pointer */

    object_register_type (alias_type);

    if (!_alias_types_ht)
      _alias_types_ht = g_hash_table_new (g_str_hash, g_str_equal);
    g_hash_table_insert (_alias_types_ht, g_strdup ((char *)name), type);

    xmlFree (name);
  }
}

