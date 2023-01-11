/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#include <stdio.h>

#include "cut_n_paste.h"
#include "object.h"
#include "object_ops.h"

static GList *stored_list = NULL;
static int stored_generation = 0;

static void free_stored(void)
{
  if (stored_list != NULL) {
    destroy_object_list(stored_list);
    stored_list = NULL;
  }
}

void
cnp_store_objects(GList *object_list, int generation)
{
  free_stored();
  stored_list = object_list;
  stored_generation = generation;
}

GList *
cnp_get_stored_objects(int* generation)
{
  GList *copied_list;
  copied_list = object_copy_list(stored_list);
  *generation = stored_generation;
  ++stored_generation;
  return copied_list;
}

gboolean
cnp_exist_stored_objects(void)
{
  return (stored_list != NULL);
}
