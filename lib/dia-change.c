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
 * Copyright © 2019 Zander Brown <zbrown@gnome.org>
 */

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "dia-change.h"
#include "diagramdata.h"


static void
dia_change_real_apply (DiaChange   *self,
                       DiagramData *diagram)
{
  g_critical ("%s doesn't implement apply", DIA_CHANGE_TYPE_NAME (self));
}


static void
dia_change_real_revert (DiaChange   *self,
                        DiagramData *diagram)
{
  g_critical ("%s doesn't implement revert", DIA_CHANGE_TYPE_NAME (self));
}


static void
dia_change_real_free (DiaChange *self)
{
}


static void
dia_change_base_class_init (DiaChangeClass *klass)
{
  klass->apply = dia_change_real_apply;
  klass->revert = dia_change_real_revert;
  klass->free = dia_change_real_free;
}


static void
dia_change_class_finalize (DiaChangeClass *klass)
{

}


static void
dia_change_do_class_init (DiaChangeClass *klass)
{
}


static void
dia_change_init (DiaChange      *self,
                 DiaChangeClass *klass)
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
    dia_change_unref (value->data[0].v_pointer);
  }
}


static void
g_value_change_copy_value (const GValue *src_value,
                           GValue       *dest_value)
{
  if (src_value->data[0].v_pointer) {
    dest_value->data[0].v_pointer = dia_change_ref (src_value->data[0].v_pointer);
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
    DiaChange *change = collect_values[0].v_pointer;

    if (change->g_type_instance.g_class == NULL) {
      return g_strconcat ("invalid unclassed change pointer for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
    } else if (!g_value_type_compatible (DIA_CHANGE_TYPE (change), G_VALUE_TYPE (value))) {
      return g_strconcat ("invalid change type '",
                          DIA_CHANGE_TYPE_NAME (change),
                          "' for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = dia_change_ref (change);
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
  DiaChange **change_p = collect_values[0].v_pointer;

  if (!change_p) {
    return g_strdup_printf ("value location for '%s' passed as NULL",
                            G_VALUE_TYPE_NAME (value));
  }

  if (!value->data[0].v_pointer) {
    *change_p = NULL;
  } else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) {
    *change_p = value->data[0].v_pointer;
  } else {
    *change_p = dia_change_ref (value->data[0].v_pointer);
  }

  return NULL;
}


static void
g_value_change_transform_value (const GValue *src_value,
                                GValue       *dest_value)
{
  if (src_value->data[0].v_pointer &&
      g_type_is_a (DIA_CHANGE_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value))) {
    dest_value->data[0].v_pointer = dia_change_ref (src_value->data[0].v_pointer);
  } else {
    dest_value->data[0].v_pointer = NULL;
  }
}


GType
dia_change_get_type (void)
{
  static GType type_id = 0;

  if (g_once_init_enter (&type_id)) {
    static const GTypeFundamentalInfo finfo = {
      G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE,
    };
    GTypeInfo info = {
      sizeof (DiaChangeClass),
      (GBaseInitFunc) dia_change_base_class_init,
      (GBaseFinalizeFunc) dia_change_class_finalize,
      (GClassInitFunc) dia_change_do_class_init,
      NULL,                         /* class_destroy */
      NULL,                         /* class_data */
      sizeof (DiaChange),
      0,                            /* n_preallocs */
      (GInstanceInitFunc) dia_change_init,
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
                                        g_intern_static_string ("DiaChange"),
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
 * dia_change_ref:
 * @self: the #DiaChange
 *
 * Returns: (transfer full): a new reference
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_change_ref (gpointer self)
{
  g_return_val_if_fail (self != NULL, NULL);

  g_ref_count_inc (&((DiaChange *) self)->refs);

  return self;
}


/**
 * dia_change_unref:
 * @self: (transfer full): the #DiaChange
 *
 * Decrease the ref count of @self potentially freeing it
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_change_unref (gpointer self)
{
  g_return_if_fail (self != NULL);

  if (g_ref_count_dec (&((DiaChange *) self)->refs)) {
    DIA_CHANGE_GET_CLASS (self)->free (self);

    g_type_free_instance (self);
  }
}


/**
 * dia_change_new:
 * @type: the #DiaChange derived #GType to instantiate
 *
 * Create an instance of @type, it's expected this will be used inside a
 * constructor for @type rather than in general code
 *
 * Returns: (transfer full): A new #DiaChange instance
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_change_new (GType type)
{
  g_return_val_if_fail (DIA_TYPE_IS_CHANGE (type), NULL);

  return g_type_create_instance (type);
}


/**
 * dia_change_apply:
 * @self: a #DiaChange
 * @diagram: the #DiagramData to apply @self to
 *
 * Do a change
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_change_apply (DiaChange   *self,
                  DiagramData *diagram)
{
  g_return_if_fail (self && DIA_IS_CHANGE (self));
  g_return_if_fail (diagram && DIA_IS_DIAGRAM_DATA (diagram));

  DIA_CHANGE_GET_CLASS (self)->apply (self, diagram);
}


/**
 * dia_change_revert:
 * @self: a #DiaChange
 * @diagram: the #DiagramData to revert @self from
 *
 * Undo a change
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_change_revert (DiaChange   *self,
                   DiagramData *diagram)
{
  g_return_if_fail (self && DIA_IS_CHANGE (self));
  g_return_if_fail (diagram && DIA_IS_DIAGRAM_DATA (diagram));

  DIA_CHANGE_GET_CLASS (self)->revert (self, diagram);
}
