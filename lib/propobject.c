/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Propobject.c: routines which deal with Dia Objects and affect their
 * properties.
 *
 * Most of these routines used to exist in code before the restructuration.
 * They've lost most of their meat, in favour for more modularity.
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

#include <config.h>

#include <string.h>

#include <glib.h>
#include "properties.h"
#include "propinternals.h"
#include "object.h"

const PropDescription *
object_get_prop_descriptions(const DiaObject *obj) {
  const PropDescription *pdesc;
  if (!obj->ops->describe_props) return NULL;

  pdesc = obj->ops->describe_props((DiaObject *)obj); /* Yes... */
  if (!pdesc) return NULL;

  if (pdesc[0].quark != 0) return pdesc;

  prop_desc_list_calculate_quarks((PropDescription *)pdesc); /* Yes again... */
  return pdesc;
}

const PropDescription *
object_list_get_prop_descriptions(GList *objects, PropMergeOption option)
{
  GList *descs = NULL, *tmp;
  const PropDescription *pdesc;

  for (tmp = objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;
    const PropDescription *desc = object_get_prop_descriptions(obj);

    if (desc) descs = g_list_append(descs, (gpointer)desc);
  }

  /* use intersection for single object's list because it is more
   * complete than union. The latter does not include PROP_FLAG_DONT_MERGE */
  if (option == PROP_UNION && g_list_length(objects)!=1)
    pdesc = prop_desc_lists_union(descs);
  else
    pdesc = prop_desc_lists_intersection(descs);

  /* Important: Do not destroy the actual descriptions returned by the
     objects. We don't own them. */
  g_list_free(descs);

  return pdesc;
}

const PropDescription *
object_describe_props (DiaObject *obj)
{
  const PropDescription *props = obj->type->prop_descs;

  g_return_val_if_fail (props != NULL, NULL);

  if (props[0].quark == 0)
    prop_desc_list_calculate_quarks((PropDescription *)props);
  return props;
}

void
object_get_props(DiaObject *obj, GPtrArray *props)
{
  const PropOffset *offsets = obj->type->prop_offsets;

  g_return_if_fail (offsets != NULL);

  object_get_props_from_offsets(obj, (PropOffset *)offsets, props);
}

/* ------------------------------------------------------ */
/* Change management                                      */

/* an ObjectChange structure for setting of properties */
typedef struct _ObjectPropChange ObjectPropChange;
struct _ObjectPropChange {
  ObjectChange obj_change;

  DiaObject *obj;
  GPtrArray *saved_props;
};

static void
object_prop_change_apply_revert(ObjectPropChange *change, DiaObject *obj)
{
  GPtrArray *old_props;

  old_props = prop_list_copy_empty(change->saved_props);

  if (change->obj->ops->get_props)
    change->obj->ops->get_props(change->obj, old_props);

  /* set saved property values */
  if (change->obj->ops->set_props)
    change->obj->ops->set_props(change->obj, change->saved_props);

  /* move old props to saved properties */
  prop_list_free(change->saved_props);
  change->saved_props = old_props;
}

static void
object_prop_change_free(ObjectPropChange *change)
{
  prop_list_free(change->saved_props);
}

ObjectChange *
object_apply_props(DiaObject *obj, GPtrArray *props)
{
  ObjectPropChange *change;
  GPtrArray *old_props;

  change = g_new0(ObjectPropChange, 1);

  change->obj_change.apply =
    (ObjectChangeApplyFunc) object_prop_change_apply_revert;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) object_prop_change_apply_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) object_prop_change_free;

  change->obj = obj;

  /* create new properties structure with current values */
  old_props = prop_list_copy_empty(props);

  if (obj->ops->get_props)
    obj->ops->get_props(obj, old_props);

  /* set saved property values */
  if (obj->ops->set_props)
    obj->ops->set_props(obj, props);

  change->saved_props = old_props;

  return (ObjectChange *)change;
}

/*!
 * Toggle a boolean property including change management
 */
ObjectChange *
object_toggle_prop (DiaObject *obj, const char *pname, gboolean val)
{
  Property *prop = make_new_prop (pname, PROP_TYPE_BOOL, 0);
  GPtrArray *plist = prop_list_from_single (prop);

  ((BoolProperty *)prop)->bool_data = val;
  return object_apply_props (obj, plist);
}

/* Get/Set routines */

gboolean
object_get_props_from_offsets(DiaObject *obj, PropOffset *offsets,
                              GPtrArray *props)
{
  prop_offset_list_calculate_quarks(offsets);
  do_get_props_from_offsets(obj,props,offsets);

  return TRUE;
}

gboolean
object_set_props_from_offsets(DiaObject *obj, PropOffset *offsets,
                              GPtrArray *props)
{
  prop_offset_list_calculate_quarks(offsets);
  do_set_props_from_offsets(obj,props,offsets);

  return TRUE;
}


WIDGET *
object_create_props_dialog(DiaObject *obj, gboolean is_default)
{
  GList *list = NULL;
  list = g_list_append(list, obj);
  return object_list_create_props_dialog(list, is_default);
}

WIDGET *
object_list_create_props_dialog(GList *objects, gboolean is_default)
{
  return prop_dialog_new(objects, is_default)->widget;
}


ObjectChange *
object_apply_props_from_dialog(DiaObject *obj, WIDGET *dialog_widget)
{
  ObjectChange *change;
  PropDialog *dialog = prop_dialog_from_widget(dialog_widget);
  GPtrArray *props = g_ptr_array_new ();
  guint i;

  prop_get_data_from_widgets(dialog);
  /* only stuff which is actually changed */
  for (i = 0; i < dialog->props->len; ++i) {
    Property *p = g_ptr_array_index (dialog->props, i);
    if (p->descr->flags & PROP_FLAG_WIDGET_ONLY)
      continue;
    if ((p->experience & PXP_NOTSET) == 0)
      g_ptr_array_add(props, p);
  }
  /* with an empty list there is no change at all but simply
   * returning NULL is against the contract ...
   */
  if (!obj->ops->apply_properties_list) {
    g_warning("using a fallback function to apply properties;"
              " undo may not work correctly");
    change = object_apply_props(obj, props);
  } else {
    change = obj->ops->apply_properties_list(obj, props);
  }
  g_ptr_array_free(props, TRUE);
  return change;
}

gboolean
objects_comply_with_stdprop(GList *objects)
{
  GList *tmp = objects;

  for (; tmp != NULL; tmp = tmp->next) {
    const DiaObject *obj = (const DiaObject*)tmp->data;
    if (!object_complies_with_stdprop(obj))
      return FALSE;
  }

  return TRUE;
}

gboolean
object_complies_with_stdprop(const DiaObject *obj)
{
  if (obj->ops->set_props == NULL) {
    g_warning("No set_props !");
    return FALSE;
  }
  if (obj->ops->get_props == NULL) {
    g_warning("No get_props !");
    return FALSE;
  }
  if (obj->ops->describe_props == NULL) {
    g_warning("No describe_props !");
    return FALSE;
  }
  if (object_get_prop_descriptions(obj) == NULL) {
    g_warning("No properties !");
    return FALSE;
  }
  return TRUE;
}

void
object_list_get_props(GList *objects, GPtrArray *props)
{
  GList *tmp = objects;

  for (; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = (DiaObject*)tmp->data;
    obj->ops->get_props(obj,props);
  }
}

static gboolean
pdtpp_do_save_no_standard_default (const PropDescription *pdesc)
{
  return pdtpp_do_save_no_standard(pdesc) && pdtpp_defaults (pdesc);
}

void
object_copy_props(DiaObject *dest, const DiaObject *src, gboolean is_default)
{
  GPtrArray *props;

  g_return_if_fail(src != NULL);
  g_return_if_fail(dest != NULL);
  g_return_if_fail(strcmp(src->type->name,dest->type->name)==0);
  g_return_if_fail(src->ops == dest->ops);
  g_return_if_fail(object_complies_with_stdprop(src));
  g_return_if_fail(object_complies_with_stdprop(dest));

  props = prop_list_from_descs(object_get_prop_descriptions(src),
                               (is_default?pdtpp_do_save_no_standard_default:
				pdtpp_do_save));

  src->ops->get_props((DiaObject *)src, props); /* FIXME: really should make
                                                get_props' first argument
                                                a (const DiaObject *) */
  dest->ops->set_props(dest, props);

  prop_list_free(props);
}

void
object_load_props(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  GPtrArray *props;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(obj_node != NULL);
  g_return_if_fail(object_complies_with_stdprop(obj));

  props = prop_list_from_descs(object_get_prop_descriptions(obj),
                               pdtpp_do_load);

  if (!prop_list_load(props,obj_node, ctx)) {
    /* context already has the message */
  }

  obj->ops->set_props(obj, props);
  prop_list_free(props);
}

void
object_save_props(DiaObject *obj, ObjectNode obj_node, DiaContext *ctx)
{
  GPtrArray *props;

  g_return_if_fail(obj != NULL);
  g_return_if_fail(obj_node != NULL);
  g_return_if_fail(object_complies_with_stdprop(obj));

  props = prop_list_from_descs(object_get_prop_descriptions(obj),
                               pdtpp_do_save);

  obj->ops->get_props(obj, props);
  prop_list_save(props,obj_node,ctx);
  prop_list_free(props);
}

Property *
object_prop_by_name_type(DiaObject *obj, const char *name, const char *type)
{
  const PropDescription *pdesc;
  GQuark name_quark = g_quark_from_string(name);

  if (!object_complies_with_stdprop(obj)) return NULL;

  for (pdesc = object_get_prop_descriptions(obj);
       pdesc->name != NULL;
       pdesc++) {
    if (name_quark == 0 || (pdesc->quark == name_quark)) {
      Property *prop;
      static GPtrArray *plist = NULL;

      if (type && (0 != strcmp(pdesc->type,type))) continue;

      if (!plist) {
        plist = g_ptr_array_new();
        g_ptr_array_set_size(plist,1);
      }
      prop = pdesc->ops->new_prop(pdesc,pdtpp_from_object);
      g_ptr_array_index(plist,0) = prop;
      obj->ops->get_props(obj,plist);
      return prop;
    }
  }
  return NULL;
}

Property *
object_prop_by_name(DiaObject *obj, const char *name)
{
  return object_prop_by_name_type(obj,name,NULL);
}

/*!
 * \brief Modification of the objects 'pixbuf' property
 *
 * @param object object to modify
 * @param pixbuf the pixbuf to set
 * @return an object change or NULL
 *
 * If the object does not have a pixbuf property nothing
 * happens. If there is a pixbuf property and the passed
 * in pixbuf is identical an empty change is returned.
 *
 * \memberof _DiaObject
 * \ingroup StdProps
 */
ObjectChange *
dia_object_set_pixbuf (DiaObject *object,
		       GdkPixbuf *pixbuf)
{
  ObjectChange *change;
  GPtrArray *props;
  PixbufProperty *pp;
  Property *prop = object_prop_by_name_type (object, "pixbuf", PROP_TYPE_PIXBUF);

  if (!prop)
    return NULL;
  pp = (PixbufProperty *)prop;
  if (pp->pixbuf == pixbuf)
    return change_list_create ();
  if (pp->pixbuf)
    g_object_unref (pp->pixbuf);
  pp->pixbuf = g_object_ref (pixbuf);
  props = prop_list_from_single (prop);
  change = object_apply_props (object, props);
  prop_list_free (props);
  return change;
}

/*!
 * \brief Modification of the objects 'pattern' property
 *
 * @param object object to modify
 * @param pattern the pattern to set
 * @return an object change or NULL
 *
 * If the object does not have a pattern property nothing
 * happens. If there is a pattern property and the passed
 * in pattern is identical an empty change is returned.
 *
 * \memberof _DiaObject
 * \ingroup StdProps
 */
ObjectChange *
dia_object_set_pattern (DiaObject  *object,
			DiaPattern *pattern)
{
  ObjectChange *change;
  GPtrArray *props;
  PatternProperty *pp;
  Property *prop = object_prop_by_name_type (object, "pattern", PROP_TYPE_PATTERN);

  if (!prop)
    return NULL;
  pp = (PatternProperty *)prop;
  if (pp->pattern == pattern)
    return change_list_create ();
  if (pp->pattern)
    g_object_unref (pp->pattern);
  pp->pattern = g_object_ref (pattern);
  props = prop_list_from_single (prop);
  change = object_apply_props (object, props);
  prop_list_free (props);
  return change;
}

/*!
 * \brief Modify the objects string property
 * @param object the object to modify
 * @param name the name of the string property (NULL for any)
 * @param value the value to set, NULL to delete
 * @return object change on sucess, NULL if not found
 *
 * Usually you should not pass NULL for the name, the facility
 * was added for convenience of the unit test.
 *
 * \memberof _DiaObject
 * \ingroup StdProps
 */
ObjectChange *
dia_object_set_string (DiaObject *object,
		       const char *name,
		       const char *value)
{
  ObjectChange *change;
  GPtrArray *props = NULL;
  Property *prop = object_prop_by_name_type (object, name, PROP_TYPE_STRING);

  if (!prop)
    prop = object_prop_by_name_type (object, name, PROP_TYPE_FILE);
  if (prop) {
    StringProperty *pp = (StringProperty *)prop;
    g_free (pp->string_data);
    pp->string_data = g_strdup (value);
    props = prop_list_from_single (prop);
  } else if ((prop = object_prop_by_name_type (object, name, PROP_TYPE_TEXT)) != NULL) {
    TextProperty *pp = (TextProperty *)prop;
    g_free (pp->text_data);
    pp->text_data = g_strdup (value);
    props = prop_list_from_single (prop);
  }
  if (!props)
    return NULL;

  change = object_apply_props (object, props);
  prop_list_free (props);
  return change;
}
