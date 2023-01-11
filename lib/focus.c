/* Dia -- a diagram creation/manipulation program
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

/**
 * SECTION:focus
 *
 * This files handles which text elements are currently eligible to get
 * the input focus, and moving back and forth between them.  Objects can
 * add their texts to the list with request_focus (more than one can be
 * added), which doesn't give them focus outright, but makes them part of
 * the focus chain.  Actual handling of when focus goes where is handled
 * in app/disp_callbacks.c
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "text.h"
#include "focus.h"
#include "diagramdata.h"
#include "object.h"
#include "dia-layer.h"

/** Returns the list of foci for the given diagram */
static GList *
get_text_foci(DiagramData *dia)
{
  return dia->text_edits;
}


/**
 * get_active_focus:
 * @dia: the #DiagramData
 *
 * Get the currently active focus for the given diagram, if any
 *
 * Since: dawn-of-time
 */
Focus *
get_active_focus (DiagramData *dia)
{
  return dia->active_text_edit;
}


/**
 * set_active_focus:
 * @dia: #DiagramData to set active focus for.
 * @focus: #Focus to be active, or %NULL if no focus should be active.
 *
 * Set which focus is active for the given diagram.
 *
 * The focus should be on the get_text_foci list.
 *
 * Since: dawn-of-time
 */
static void
set_active_focus (DiagramData *dia, Focus *focus)
{
  if (dia->active_text_edit != NULL) {
    dia->active_text_edit->has_focus = FALSE;
  }
  dia->active_text_edit = focus;
  if (focus != NULL) {
    focus->has_focus = TRUE;
  }
}


/**
 * add_text_focus:
 * @dia: #DiagramData to add #Focus to
 * @focus: #Focus to add
 *
 * Add a focus to the list of foci that can be activated for the given
 * diagram.
 *
 * Since: dawn-of-time
 */
static void
add_text_focus (DiagramData *dia, Focus *focus)
{
  dia->text_edits = g_list_append (dia->text_edits, focus);
}


/**
 * set_text_foci:
 * @dia: #DiagramData to set list for
 * @foci: the new list
 *
 * Set the list of foci for a diagram. The old list, if any, will be
 * returned, not freed.
 *
 * Since: dawn-of-time
 */
static GList *
set_text_foci (DiagramData *dia, GList *foci)
{
  GList *old_foci = get_text_foci (dia);
  dia->text_edits = NULL;
  return old_foci;
}


/**
 * request_focus:
 * @focus: the #Focus to make active
 *
 * Request that the give focus become active.
 * Also adds the focus to the list of available foci.
 * Eventually, this will only add the focus to the list.
 *
 * Since: dawn-of-time
 */
void
request_focus(Focus *focus)
{
  DiagramData *dia = dia_layer_get_parent_diagram (focus->obj->parent_layer);
  GList *text_foci = get_text_foci (dia);
  /* Only add to focus list if not already there, and don't snatch focus. */
  if (!g_list_find (text_foci, focus)) {
    add_text_focus (dia, focus);
  }
  return;
}


void
give_focus(Focus *focus)
{
  DiagramData *dia = dia_layer_get_parent_diagram (focus->obj->parent_layer);

  if (get_active_focus(dia) != NULL) {
    get_active_focus(dia)->has_focus = FALSE;
  }
  set_active_focus(dia, focus);
  focus->has_focus = TRUE;
}

/* Return the first focus on the given object
 */
Focus *
focus_get_first_on_object(DiaObject *obj)
{
  GList *tmplist = get_text_foci (dia_layer_get_parent_diagram (obj->parent_layer));

  for (; tmplist != NULL; tmplist = g_list_next(tmplist) ) {
    Focus *focus = (Focus*)tmplist->data;
    if (focus_get_object(focus) == obj) {
      return focus;
    }
  }
  return NULL;
}


/**
 * focus_get_object:
 * @focus: the #Focus
 *
 * Return the object that this focus belongs to.  Note that each
 * object may have more than one #Text associated with it, the
 * focus will be on one of those.
 *
 * Since: dawn-of-time
 */
DiaObject*
focus_get_object (Focus *focus)
{
  return focus->obj;
}


Focus *
focus_next_on_diagram(DiagramData *dia)
{
  if (get_text_foci(dia) != NULL && get_active_focus(dia) != NULL) {
    GList *listelem = g_list_find(get_text_foci(dia), get_active_focus(dia));
    listelem = g_list_next(listelem);
    if (listelem == NULL) listelem = get_text_foci(dia);
    return ((Focus*)listelem->data);
  }
  return NULL;
}


Focus *
focus_previous_on_diagram(DiagramData *dia)
{
  GList *text_foci = get_text_foci(dia);
  if (text_foci != NULL && get_active_focus(dia) != NULL) {
    GList *listelem = g_list_find(text_foci, get_active_focus(dia));
    listelem = g_list_previous(listelem);
    if (listelem == NULL)
      listelem = g_list_last(text_foci);
    return (Focus *)listelem->data;
  }
  return NULL;
}

void
remove_focus_on_diagram(DiagramData *dia)
{
  if (get_active_focus(dia) != NULL) {
    set_active_focus(dia, NULL);
  }
}

void
reset_foci_on_diagram(DiagramData *dia)
{
  GList *old_foci;
  remove_focus_on_diagram(dia);
  old_foci = set_text_foci(dia, NULL);
  g_list_free(old_foci);
}


/**
 * remove_focus_object:
 * @obj: the #DiaObject
 *
 * Removes all foci owned by the object.
 *
 * Returns: %TRUE if the object had the active focus for its diagram.
 */
gboolean
remove_focus_object (DiaObject *obj)
{
  DiagramData *dia = dia_layer_get_parent_diagram (obj->parent_layer);
  GList *tmplist = get_text_foci (dia);
  gboolean active = FALSE;
  Focus *next_focus = NULL;
  Focus *active_focus = get_active_focus (dia);

  for (; tmplist != NULL; ) {
    Focus *focus = (Focus*) tmplist->data;
    GList *link = tmplist;
    tmplist = g_list_next (tmplist);
    if (focus_get_object (focus) == obj) {
      if (focus == active_focus) {
        next_focus = focus_next_on_diagram (dia);
        active = TRUE;
      }
      set_text_foci (dia, g_list_delete_link (get_text_foci (dia), link));
    }
  }

  if (next_focus != NULL && get_text_foci (dia) != NULL) {
    give_focus (next_focus);
  } else {
    if (get_text_foci (dia) == NULL) {
      set_active_focus (dia, NULL);
    }
  }

  return active;
}

