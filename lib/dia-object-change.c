/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "dia-object-change.h"
#include "object.h"


/**
 * SECTION:dia-object-change
 *
 * Forming the basic of undo support to be implemented in objects
 *
 * Object implementations need some effort to support undo/redo
 *
 * Return value of object changing functions and methods of DiaObject
 *
 * FIXME: #DiaObjectChange functions should not require the changed object
 * as an argument. Every change object should keep track of the
 * relevant object instead. The second argument in the above typedefs
 * is deprecated and should not be relied on.
 *
 * Replaces ObjectChange
 */


static void
dia_object_change_real_apply (DiaObjectChange *self,
                              DiaObject       *object)
{
  g_critical ("%s doesn't implement apply", DIA_OBJECT_CHANGE_TYPE_NAME (self));
}


static void
dia_object_change_real_revert (DiaObjectChange *self,
                               DiaObject       *object)
{
  g_critical ("%s doesn't implement revert", DIA_OBJECT_CHANGE_TYPE_NAME (self));
}


static void
dia_object_change_real_free (DiaObjectChange *self)
{
}


static void
dia_object_change_base_class_init (DiaObjectChangeClass *klass)
{
  klass->apply = dia_object_change_real_apply;
  klass->revert = dia_object_change_real_revert;
  klass->free = dia_object_change_real_free;
}


static void
dia_object_change_class_finalize (DiaObjectChangeClass *klass)
{

}


static void
dia_object_change_do_class_init (DiaObjectChangeClass *klass)
{
}


static void
dia_object_change_init (DiaObjectChange      *self,
                 DiaObjectChangeClass *klass)
{
  g_ref_count_init (&self->refs);
}


static void
g_value_change_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}


static void
g_value_change_free_value (GValue *value)
{
  if (value->data[0].v_pointer) {
    dia_object_change_unref (value->data[0].v_pointer);
  }
}


static void
g_value_change_copy_value (const GValue *src_value,
                           GValue       *dest_value)
{
  if (src_value->data[0].v_pointer) {
    dest_value->data[0].v_pointer = dia_object_change_ref (src_value->data[0].v_pointer);
  } else {
    dest_value->data[0].v_pointer = NULL;
  }
}


static gpointer
g_value_change_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}


static char *
g_value_change_collect_value (GValue      *value,
                              guint        n_collect_values,
                              GTypeCValue *collect_values,
                              guint        collect_flags)
{
  if (collect_values[0].v_pointer) {
    DiaObjectChange *change = collect_values[0].v_pointer;

    if (change->g_type_instance.g_class == NULL) {
      return g_strconcat ("invalid unclassed change pointer for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
    } else if (!g_value_type_compatible (DIA_OBJECT_CHANGE_TYPE (change), G_VALUE_TYPE (value))) {
      return g_strconcat ("invalid change type '",
                          DIA_OBJECT_CHANGE_TYPE_NAME (change),
                          "' for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = dia_object_change_ref (change);
    }
  } else {
    value->data[0].v_pointer = NULL;
  }

  return NULL;
}


static char *
g_value_change_lcopy_value (const GValue *value,
                            guint         n_collect_values,
                            GTypeCValue  *collect_values,
                            guint         collect_flags)
{
  DiaObjectChange **change_p = collect_values[0].v_pointer;

  if (!change_p) {
    return g_strdup_printf ("value location for '%s' passed as NULL",
                            G_VALUE_TYPE_NAME (value));
  }

  if (!value->data[0].v_pointer) {
    *change_p = NULL;
  } else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) {
    *change_p = value->data[0].v_pointer;
  } else {
    *change_p = dia_object_change_ref (value->data[0].v_pointer);
  }

  return NULL;
}


static void
g_value_change_transform_value (const GValue *src_value,
                                GValue       *dest_value)
{
  if (src_value->data[0].v_pointer &&
      g_type_is_a (DIA_OBJECT_CHANGE_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value))) {
    dest_value->data[0].v_pointer = dia_object_change_ref (src_value->data[0].v_pointer);
  } else {
    dest_value->data[0].v_pointer = NULL;
  }
}


GType
dia_object_change_get_type (void)
{
  static GType type_id = 0;

  if (g_once_init_enter (&type_id)) {
    static const GTypeFundamentalInfo finfo = {
      G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE,
    };
    GTypeInfo info = {
      sizeof (DiaObjectChangeClass),
      (GBaseInitFunc) dia_object_change_base_class_init,
      (GBaseFinalizeFunc) dia_object_change_class_finalize,
      (GClassInitFunc) dia_object_change_do_class_init,
      NULL,                         /* class_destroy */
      NULL,                         /* class_data */
      sizeof (DiaObjectChange),
      0,                            /* n_preallocs */
      (GInstanceInitFunc) dia_object_change_init,
      NULL,                         /* value_table */
    };
    static const GTypeValueTable value_table = {
      g_value_change_init,          /* value_init */
      g_value_change_free_value,    /* value_free */
      g_value_change_copy_value,    /* value_copy */
      g_value_change_peek_pointer,  /* value_peek_pointer */
      "p",                          /* collect_format */
      g_value_change_collect_value, /* collect_value */
      "p",                          /* lcopy_format */
      g_value_change_lcopy_value,   /* lcopy_value */
    };
    GType type;

    info.value_table = &value_table;
    type = g_type_register_fundamental (g_type_fundamental_next (),
                                        g_intern_static_string ("DiaObjectChange"),
                                        &info,
                                        &finfo,
                                        0);
    g_value_register_transform_func (type,
                                     type,
                                     g_value_change_transform_value);

    g_once_init_leave (&type_id, type);
  }

  return type_id;
}


/**
 * dia_object_change_ref:
 * @self: the #DiaObjectChange
 *
 * Returns: (transfer full): a new reference
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_object_change_ref (gpointer self)
{
  g_return_val_if_fail (self != NULL, NULL);

  g_ref_count_inc (&((DiaObjectChange *) self)->refs);

  return self;
}


/**
 * dia_object_change_unref:
 * @self: (transfer full): the #DiaObjectChange
 *
 * Decrease the ref count of @self potentially freeing it
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_object_change_unref (gpointer self)
{
  g_return_if_fail (self != NULL);

  if (g_ref_count_dec (&((DiaObjectChange *) self)->refs)) {
    DIA_OBJECT_CHANGE_GET_CLASS (self)->free (self);

    g_type_free_instance (self);
  }
}


/**
 * dia_object_change_new:
 * @type: the #DiaObjectChange derived #GType to instantiate
 *
 * Create an instance of @type, it's expected this will be used inside a
 * constructor for @type rather than in general code
 *
 * Returns: (transfer full): A new #DiaObjectChange instance
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_object_change_new (GType type)
{
  g_return_val_if_fail (DIA_TYPE_IS_OBJECT_CHANGE (type), NULL);

  return g_type_create_instance (type);
}


/**
 * dia_object_change_apply:
 * @self: a #DiaObjectChange
 * @object: the #DiaObject to apply @self to
 *
 * Do a change
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_object_change_apply (DiaObjectChange *self,
                         DiaObject       *object)
{
  g_return_if_fail (self && DIA_IS_OBJECT_CHANGE (self));

  DIA_OBJECT_CHANGE_GET_CLASS (self)->apply (self, object);
}


/**
 * dia_object_change_revert:
 * @self: a #DiaObjectChange
 * @object: the #DiaObject to revert @self from
 *
 * Undo a change
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_object_change_revert (DiaObjectChange *self,
                          DiaObject       *object)
{
  g_return_if_fail (self && DIA_IS_OBJECT_CHANGE (self));

  DIA_OBJECT_CHANGE_GET_CLASS (self)->revert (self, object);
}
