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

#pragma once

#include <glib-object.h>

#ifndef __GTK_DOC_IGNORE__
// TODO: Fix the cycle
typedef struct _DiaObject DiaObject;
#endif

G_BEGIN_DECLS

GType dia_object_change_get_type (void);
#define DIA_TYPE_OBJECT_CHANGE dia_object_change_get_type ()

typedef struct _DiaObjectChange DiaObjectChange;
typedef struct _DiaObjectChangeClass DiaObjectChangeClass;

#define DIA_TYPE_IS_OBJECT_CHANGE(type)     (G_TYPE_FUNDAMENTAL (type) == DIA_TYPE_OBJECT_CHANGE)
#define DIA_OBJECT_CHANGE(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DIA_TYPE_OBJECT_CHANGE, DiaObjectChange))
#define DIA_OBJECT_CHANGE_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DIA_TYPE_OBJECT_CHANGE, DiaObjectChangeClass))
#define DIA_IS_OBJECT_CHANGE(object)        (G_TYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE ((object), DIA_TYPE_OBJECT_CHANGE))
#define DIA_IS_OBJECT_CHANGE_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DIA_TYPE_OBJECT_CHANGE))
#define DIA_OBJECT_CHANGE_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DIA_TYPE_OBJECT_CHANGE, DiaObjectChangeClass))
#define DIA_OBJECT_CHANGE_TYPE(object)      (G_TYPE_FROM_INSTANCE (object))
#define DIA_OBJECT_CHANGE_TYPE_NAME(object) (g_type_name (DIA_OBJECT_CHANGE_TYPE (object)))
#define DIA_OBJECT_CHANGE_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define DIA_OBJECT_CHANGE_CLASS_NAME(class) (g_type_name (DIA_OBJECT_CHANGE_CLASS_TYPE (class)))
#define G_VALUE_HOLDS_OBJECT_CHANGE(value)  (G_TYPE_CHECK_VALUE_TYPE ((value), DIA_TYPE_OBJECT_CHANGE))


/**
 * DIA_DEFINE_CHANGE:
 * @TypeName: CamelCase name of the type
 * @type_name: python_case name of the type
 *
 * G_DEFINE_TYPE() wrapper for implementing #DiaObjectChange types, however
 * unlike G_DEFINE_TYPE() the _init and _class_init are implemented for you.
 * Instead you provide apply, revert & free functions
 *
 * |[<!-- language="C" -->
 * DIA_DEFINE_OBJECT_CHANGE (SomeChange, some_change)
 *
 * static void
 * some_change_apply (DiaObjectChange *change,
 *                    DiaObject       *object)
 * {
 * }
 *
 * static void
 * some_change_revert (DiaObjectChange *change,
 *                     DiaObject       *object)
 * {
 * }
 *
 * static void
 * some_change_free (DiaObjectChange *change)
 * {
 * }
 * ]|
 *
 * Since: 0.98
 */
#define DIA_DEFINE_OBJECT_CHANGE(TypeName, type_name)                        \
  G_DEFINE_TYPE (TypeName, type_name, DIA_TYPE_OBJECT_CHANGE)                \
                                                                             \
  static void type_name##_apply             (DiaObjectChange *change,        \
                                             DiaObject       *object);       \
  static void type_name##_revert            (DiaObjectChange *change,        \
                                             DiaObject       *object);       \
  static void type_name##_free              (DiaObjectChange *change);       \
                                                                             \
  static void                                                                \
  type_name##_class_init (TypeName##Class *klass)                            \
  {                                                                          \
    DiaObjectChangeClass *change_class = DIA_OBJECT_CHANGE_CLASS (klass);    \
                                                                             \
    change_class->apply = type_name##_apply;                                 \
    change_class->revert = type_name##_revert;                               \
    change_class->free = type_name##_free;                                   \
  }                                                                          \
                                                                             \
  static void                                                                \
  type_name##_init (TypeName *klass)                                         \
  {                                                                          \
  }


struct _DiaObjectChange {
  GTypeInstance g_type_instance;

  grefcount refs;

  DiaObjectChange *next;
  DiaObjectChange *prev;
};


/**
 * DiaObjectChangeClass:
 * @apply: do the change
 * @revert: undo the effects of @apply
 * @free: clear fields (called during destruction)
 *
 * Since: 0.98
 *
 * Stability: Stable
 */
struct _DiaObjectChangeClass {
  GTypeClass parent;

  void (*apply)  (DiaObjectChange *change,
                  DiaObject       *dia);
  void (*revert) (DiaObjectChange *change,
                  DiaObject       *dia);
  void (*free)   (DiaObjectChange *change);
};


void     dia_object_change_unref  (gpointer         self);
gpointer dia_object_change_ref    (gpointer         self);
gpointer dia_object_change_new    (GType            type);
void     dia_object_change_apply  (DiaObjectChange *self,
                                   DiaObject       *object);
void     dia_object_change_revert (DiaObjectChange *self,
                                   DiaObject       *object);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DiaObjectChange, dia_object_change_unref)

G_END_DECLS
