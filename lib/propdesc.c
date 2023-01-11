/* Dia -- a diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * propdesc.c: This module handles operations on property descriptors and
 * property descriptor lists.
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
prop_desc_list_calculate_quarks(PropDescription *plist)
{
  guint i;

  for (i = 0; plist[i].name != NULL; i++) {
    if (plist[i].quark == 0)
      plist[i].quark = g_quark_from_static_string(plist[i].name);
    if (plist[i].type_quark == 0)
      plist[i].type_quark = g_quark_from_static_string(plist[i].type);
    if (!plist[i].ops)
      plist[i].ops = prop_type_get_ops(plist[i].type);
  }
}

const PropDescription *
prop_desc_list_find_prop(const PropDescription *plist, const gchar *name)
{
  gint i = 0;
  GQuark name_quark = g_quark_from_string(name);

  while (plist[i].name != NULL) {
    if (plist[i].quark == name_quark)
      return &plist[i];
    i++;
  }
  return NULL;
}

/* finds the real handler in case there are several levels of indirection */
PropEventHandler
prop_desc_find_real_handler(const PropDescription *pdesc)
{
  PropEventHandler ret = pdesc->event_handler;
  const PropEventHandlerChain *chain = &pdesc->chain_handler;
  if (!chain->handler) return ret;
  while (chain) {
    if (chain->handler) ret = chain->handler;
    chain = chain->chain;
  }
  return ret;
}


/* free a handler indirection list */
void
prop_desc_free_handler_chain (PropDescription *pdesc)
{
  if (pdesc) {
    PropEventHandlerChain *chain = pdesc->chain_handler.chain;
    while (chain) {
      PropEventHandlerChain *next = chain->chain;
      g_clear_pointer (&chain, g_free);
      chain = next;
    }
    pdesc->chain_handler.chain = NULL;
    pdesc->chain_handler.handler = NULL;
  }
}


/* free a handler indirection list in a list of descriptors */
void
prop_desc_list_free_handler_chain(PropDescription *pdesc)
{
  if (!pdesc) return;
  while (pdesc->name) {
    prop_desc_free_handler_chain(pdesc);
    pdesc++;
  }
}

/* insert an event handler */
void
prop_desc_insert_handler(PropDescription *pdesc,
                         PropEventHandler handler)
{
  if ((pdesc->chain_handler.handler) || (pdesc->chain_handler.chain)) {
    /* not the first level. Push things forward. */
    PropEventHandlerChain *pushed = g_new0(PropEventHandlerChain,1);
    *pushed = pdesc->chain_handler;
    pdesc->chain_handler.chain = pushed;
  }
  pdesc->chain_handler.handler = pdesc->event_handler;
  pdesc->event_handler = handler;
}

static const PropDescription null_prop_desc = { NULL };

PropDescription *
prop_desc_lists_union(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;

  /* make sure the array is allocated */
  /* FIXME: this doesn't seem to do anything useful if an error occurs. */
  g_array_append_val(arr, null_prop_desc);
  g_array_remove_index(arr, 0);

  /* Each element in the list is a GArray of PropDescription,
     terminated by a NULL element. */
  for (tmp = plists; tmp; tmp = tmp->next) {
    PropDescription *plist = tmp->data;
    int i;

    for (i = 0; plist[i].name != NULL; i++) {
      guint j;

      if (plist[i].flags & PROP_FLAG_DONT_MERGE)
        continue; /* exclude anything that can't be merged */

      /* Check to see if this PropDescription is already included in
	 the union. */
      for (j = 0; j < arr->len; j++)
	if (g_array_index(arr, PropDescription, j).quark == plist[i].quark)
	  break;

      /* Add to the union if it isn't already present. */
      if (j == arr->len)
	g_array_append_val(arr, plist[i]);
    }
  }

  /* Get the actually array and free the GArray wrapper. */
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

gboolean
propdescs_can_be_merged(const PropDescription *p1,
                        const PropDescription *p2) {
  PropEventHandler peh1 = prop_desc_find_real_handler(p1);
  PropEventHandler peh2 = prop_desc_find_real_handler(p2);

  if (p1->ops != p2->ops) return FALSE;
  if ((p1->flags|p2->flags) & PROP_FLAG_DONT_MERGE) return FALSE;
  if (peh1 != peh2) return FALSE;
  if ((p1->ops->can_merge && !(p1->ops->can_merge(p1,p2))) ||
      (p2->ops->can_merge && !(p2->ops->can_merge(p2,p1)))) return FALSE;

  return TRUE;
}

PropDescription *
prop_desc_lists_intersection(GList *plists)
{
  GArray *arr = g_array_new(TRUE, TRUE, sizeof(PropDescription));
  PropDescription *ret;
  GList *tmp;
  gint i;

  /* make sure the array is allocated */
  g_array_append_val(arr, null_prop_desc);
  g_array_remove_index(arr, 0);

  if (plists) {

    /* initialise the intersection with the first list. */
    ret = plists->data;
    for (i = 0; ret[i].name != NULL; i++)
      g_array_append_val(arr, ret[i]);

    /* check each PropDescription list for intersection */
    for (tmp = plists->next; tmp; tmp = tmp->next) {
      ret = tmp->data;

      /* go through array in reverse so that removals don't stuff things up */
      for (i = arr->len - 1; i >= 0; i--) {
	gint j;
	/* FIXME: we can avoid a copy here by making cand a pointer to
	   the data in the GArray. */
        PropDescription cand = g_array_index(arr,PropDescription,i);
        gboolean remove = TRUE;
	for (j = 0; ret[j].name != NULL; j++) {
	  if (cand.quark == ret[j].quark) {
            remove = !(propdescs_can_be_merged(&ret[j],&cand));
            break;
          }
        }
	if (remove) g_array_remove_index(arr, i);
      }
    }
  }
  ret = (PropDescription *)arr->data;
  g_array_free(arr, FALSE);
  return ret;
}

