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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

GType dia_part_get_type (void);
#define DIA_TYPE_PART (dia_part_get_type ())

typedef struct _DiaPart DiaPart;
typedef struct _DiaPartClass DiaPartClass;

#define DIA_TYPE_IS_PART(type)     (G_TYPE_FUNDAMENTAL (type) == DIA_TYPE_PART)
#define DIA_PART(object)           (G_TYPE_CHECK_INSTANCE_CAST ((object), DIA_TYPE_PART, DiaPart))
#define DIA_PART_CLASS(class)      (G_TYPE_CHECK_CLASS_CAST ((class), DIA_TYPE_PART, DiaPartClass))
#define DIA_IS_PART(object)        (G_TYPE_CHECK_INSTANCE_FUNDAMENTAL_TYPE ((object), DIA_TYPE_PART))
#define DIA_IS_PART_CLASS(class)   (G_TYPE_CHECK_CLASS_TYPE ((class), DIA_TYPE_PART))
#define DIA_PART_GET_CLASS(object) (G_TYPE_INSTANCE_GET_CLASS ((object), DIA_TYPE_PART, DiaPartClass))
#define DIA_PART_TYPE(object)      (G_TYPE_FROM_INSTANCE (object))
#define DIA_PART_TYPE_NAME(object) (g_type_name (DIA_PART_TYPE (object)))
#define DIA_PART_CLASS_TYPE(class) (G_TYPE_FROM_CLASS (class))
#define DIA_PART_CLASS_NAME(class) (g_type_name (DIA_PART_CLASS_TYPE (class)))
#define G_VALUE_HOLDS_PART(value)  (G_TYPE_CHECK_VALUE_TYPE ((value), DIA_TYPE_PART))


struct _DiaPart {
  GTypeInstance g_type_instance;

  gatomicrefcount refs;
};


struct _DiaPartClass {
  GTypeClass parent;

  void (*clear)   (DiaPart   *part);
};


gpointer dia_part_alloc  (GType        type);
gpointer dia_part_ref    (gpointer     self);
void     dia_part_unref  (gpointer     self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (DiaPart, dia_part_unref)

#define dia_clear_part(part_ptr) g_clear_pointer ((part_ptr), dia_part_unref)

G_END_DECLS
