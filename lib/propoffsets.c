/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Propoffsets.c: routines which deal with (almost) actually setting/putting
 * data in an object's members, and offset lists.
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

#include <glib.h>
#include "properties.h"
#include "propinternals.h"

void
do_set_props_from_offsets(void *base,
                          GPtrArray *props, const PropOffset *offsets)
{
  /* Warning: the name quarks *must* all be valid */
  guint i;

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    const PropOffset *ofs;
    for (ofs = offsets; ofs->name ; ofs++) {
      if ((prop->name_quark == ofs->name_quark) &&
          (prop->type_quark == ofs->type_quark)) {
        /* beware of props not set, see PROP_FLAG_OPTIONAL */
        if ((prop->experience & PXP_NOTSET) == 0)
          prop->ops->set_from_offset(prop,base,ofs->offset,ofs->offset2);
        break;
      }
    }
  }
}

void
do_get_props_from_offsets(void *base,
                          GPtrArray *props, const PropOffset *offsets)
{
  /* Warning: the name quarks *must* all be valid */
  /* This is identical to do_set_props_from_offsets(). This sucks. */
  guint i;

  for (i = 0; i < props->len; i++) {
    Property *prop = g_ptr_array_index(props,i);
    const PropOffset *ofs;
    /* nothing set yet - may happen with a foreign property list */
    prop->experience |= PXP_NOTSET;
    for (ofs = offsets; ofs->name ; ofs++) {
      if ((prop->name_quark == ofs->name_quark) &&
          (prop->type_quark == ofs->type_quark)) {
        prop->ops->get_from_offset(prop,base,ofs->offset,ofs->offset2);
        prop->experience &= ~PXP_NOTSET;
        break;
      }
    }
  }
}

/* *************************************** */

void
prop_offset_list_calculate_quarks(PropOffset *olist)
{
  guint i;

  for (i = 0; olist[i].name != NULL; i++) {
    if (olist[i].name_quark == 0)
      olist[i].name_quark = g_quark_from_static_string(olist[i].name);
    if (olist[i].type_quark == 0)
      olist[i].type_quark = g_quark_from_static_string(olist[i].type);
    if (!olist[i].ops)
      olist[i].ops = prop_type_get_ops(olist[i].type);
  }
}

