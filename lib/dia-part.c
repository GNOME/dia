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
 * Copyright 2026 Zander Brown <zbrown@gnome.org>
 */

#include <glib-object.h>
#include <gobject/gvaluecollector.h>

#include "dia-part.h"


static void
dia_part_real_clear (DiaPart *self)
{
}


static void
dia_part_base_class_init (DiaPartClass *klass)
{
  klass->clear = dia_part_real_clear;
}


static void
dia_part_class_finalize (DiaPartClass *klass)
{

}


static void
dia_part_do_class_init (DiaPartClass *klass)
{
}


static void
dia_part_init (DiaPart      *self,
               DiaPartClass *klass)
{
  g_atomic_ref_count_init (&self->refs);
}


static void
g_value_part_init (GValue *value)
{
  value->data[0].v_pointer = NULL;
}


static void
g_value_part_free_value (GValue *value)
{
  if (value->data[0].v_pointer) {
    dia_part_unref (value->data[0].v_pointer);
  }
}


static void
g_value_part_copy_value (const GValue *src_value,
                         GValue       *dest_value)
{
  if (src_value->data[0].v_pointer) {
    dest_value->data[0].v_pointer = dia_part_ref (src_value->data[0].v_pointer);
  } else {
    dest_value->data[0].v_pointer = NULL;
  }
}


static gpointer
g_value_part_peek_pointer (const GValue *value)
{
  return value->data[0].v_pointer;
}


static char *
g_value_part_collect_value (GValue      *value,
                            guint        n_collect_values,
                            GTypeCValue *collect_values,
                            guint        collect_flags)
{
  if (collect_values[0].v_pointer) {
    DiaPart *part = collect_values[0].v_pointer;

    if (part->g_type_instance.g_class == NULL) {
      return g_strconcat ("invalid unclassed part pointer for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
    } else if (!g_value_type_compatible (DIA_PART_TYPE (part), G_VALUE_TYPE (value))) {
      return g_strconcat ("invalid part type '",
                          DIA_PART_TYPE_NAME (part),
                          "' for value type '",
                          G_VALUE_TYPE_NAME (value),
                          "'",
                          NULL);
      /* never honour G_VALUE_NOCOPY_CONTENTS for ref-counted types */
      value->data[0].v_pointer = dia_part_ref (part);
    }
  } else {
    value->data[0].v_pointer = NULL;
  }

  return NULL;
}


static char *
g_value_part_lcopy_value (const GValue *value,
                          guint         n_collect_values,
                          GTypeCValue  *collect_values,
                          guint         collect_flags)
{
  DiaPart **part_p = collect_values[0].v_pointer;

  if (!part_p) {
    return g_strdup_printf ("value location for '%s' passed as NULL",
                            G_VALUE_TYPE_NAME (value));
  }

  if (!value->data[0].v_pointer) {
    *part_p = NULL;
  } else if (collect_flags & G_VALUE_NOCOPY_CONTENTS) {
    *part_p = value->data[0].v_pointer;
  } else {
    *part_p = dia_part_ref (value->data[0].v_pointer);
  }

  return NULL;
}


static void
g_value_part_transform_value (const GValue *src_value,
                              GValue       *dest_value)
{
  if (src_value->data[0].v_pointer &&
      g_type_is_a (DIA_PART_TYPE (src_value->data[0].v_pointer), G_VALUE_TYPE (dest_value))) {
    dest_value->data[0].v_pointer = dia_part_ref (src_value->data[0].v_pointer);
  } else {
    dest_value->data[0].v_pointer = NULL;
  }
}


GType
dia_part_get_type (void)
{
  static GType type_id = 0;

  if (g_once_init_enter (&type_id)) {
    static const GTypeFundamentalInfo finfo = {
      G_TYPE_FLAG_CLASSED | G_TYPE_FLAG_INSTANTIATABLE | G_TYPE_FLAG_DERIVABLE,
    };
    GTypeInfo info = {
      sizeof (DiaPartClass),
      (GBaseInitFunc) dia_part_base_class_init,
      (GBaseFinalizeFunc) dia_part_class_finalize,
      (GClassInitFunc) dia_part_do_class_init,
      NULL,                         /* class_destroy */
      NULL,                         /* class_data */
      sizeof (DiaPart),
      0,                            /* n_preallocs */
      (GInstanceInitFunc) dia_part_init,
      NULL,                         /* value_table */
    };
    static const GTypeValueTable value_table = {
      g_value_part_init,          /* value_init */
      g_value_part_free_value,    /* value_free */
      g_value_part_copy_value,    /* value_copy */
      g_value_part_peek_pointer,  /* value_peek_pointer */
      "p",                        /* collect_format */
      g_value_part_collect_value, /* collect_value */
      "p",                        /* lcopy_format */
      g_value_part_lcopy_value,   /* lcopy_value */
    };
    GType type;

    info.value_table = &value_table;
    type = g_type_register_fundamental (g_type_fundamental_next (),
                                        g_intern_static_string ("DiaPart"),
                                        &info,
                                        &finfo,
                                        0);
    g_value_register_transform_func (type,
                                     type,
                                     g_value_part_transform_value);

    g_once_init_leave (&type_id, type);
  }

  return type_id;
}


/**
 * dia_part_alloc:
 * @type: the #DiaPart derived #GType to instantiate
 *
 * Allocate an instance of @type, it's expected this will be used inside a
 * constructor for @type rather than in general code
 *
 * Returns: (transfer full): A new #DiaPart instance
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_part_alloc (GType type)
{
  g_return_val_if_fail (DIA_TYPE_IS_PART (type), NULL);

  return g_type_create_instance (type);
}


/**
 * dia_part_ref:
 * @self: the #DiaPart
 *
 * Returns: (transfer full): a new reference
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
gpointer
dia_part_ref (gpointer self)
{
  g_return_val_if_fail (self != NULL, NULL);

  g_atomic_ref_count_inc (&((DiaPart *) self)->refs);

  return self;
}


/**
 * dia_part_unref:
 * @self: (transfer full): the #DiaPart
 *
 * Decrease the ref count of @self potentially freeing it
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
void
dia_part_unref (gpointer self)
{
  g_return_if_fail (self != NULL);

  if (g_atomic_ref_count_dec (&((DiaPart *) self)->refs)) {
    DIA_PART_GET_CLASS (self)->clear (self);

    g_type_free_instance (self);
  }
}
