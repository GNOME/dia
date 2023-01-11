/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * A Registry of property types.
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
#include "properties.h"
#include "propinternals.h"

static GHashTable *props_hash = NULL;

static void check_props_hash(void)
{
  if (!props_hash) {
    props_hash = g_hash_table_new(g_str_hash,g_str_equal);
  }
}

void
prop_type_register(PropertyType type, const PropertyOps *ops)
{
  check_props_hash();
  g_hash_table_insert(props_hash,(gpointer)type,(gpointer)ops);
}

const PropertyOps *
prop_type_get_ops(PropertyType type)
{
  check_props_hash();
  return g_hash_table_lookup(props_hash,type);
}
