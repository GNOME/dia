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

/** This files handles which text elements are currently eligible to get
 * the input focus, and moving back and forth between them.  Objects can
 * add their texts to the list with request_focus (more than one can be
 * added), which doesn't give them focus outright, but makes them part of 
 * the focus chain.  Actual handling of when focus goes where is handled
 * in app/disp_callbacks.c
 */

#include <config.h>

#include "focus.h"

static Focus *active_focus_ptr = NULL;
static GList *text_foci = NULL;

/** Request that the give focus become active.
 * Also adds the focus to the list of available foci.
 * Eventually, this will only add the focus to the list. */
void
request_focus(Focus *focus)
{
  /* Only add to focus list if not already there, and don't snatch focus. */
  if (!g_list_find(text_foci, focus)) {
    text_foci = g_list_append(text_foci, focus);
  }
  return;
  if (active_focus_ptr != NULL) {
    active_focus_ptr->has_focus = FALSE;
  }
  active_focus_ptr = focus;
  active_focus_ptr->has_focus = TRUE;
  text_foci = g_list_append(text_foci, focus);
}

/** Return the currently active focus */
Focus *
active_focus(void)
{
  return active_focus_ptr;
}

void
give_focus(Focus *focus)
{
  if (active_focus_ptr != NULL) {
    active_focus_ptr->has_focus = FALSE;
  }
  active_focus_ptr = focus;
  active_focus_ptr->has_focus = TRUE;
}

gboolean
give_focus_to_object(DiaObject *obj)
{
  GList *tmplist = text_foci;

  for (; tmplist != NULL; tmplist = g_list_next(tmplist) ) {
    Focus *focus = (Focus*)tmplist->data;
    if (focus->obj == obj) {
      give_focus(focus);
      return TRUE;
    }
  }
  return FALSE;
}

/** Shift to the next available focus, if one is already active.
 */
void
focus_next(void)
{
  if (text_foci != NULL && active_focus_ptr != NULL) {
    GList *listelem = g_list_find(text_foci, active_focus_ptr);
    listelem = g_list_next(listelem);
    if (listelem == NULL) listelem = text_foci;
    give_focus((Focus *)listelem->data);
  }
}

/** Shift to the previous available focus, if one is already active.
 */
void
focus_previous(void)
{
  if (text_foci != NULL && active_focus_ptr != NULL) {
    GList *listelem = g_list_find(text_foci, active_focus_ptr);
    listelem = g_list_previous(listelem);
    if (listelem == NULL) listelem = g_list_last(text_foci);
    give_focus((Focus *)listelem->data);
  }
}

/** Remove the current focus */
void
remove_focus(void)
{
  if (active_focus_ptr != NULL) {
    active_focus_ptr->has_focus = FALSE;
  }
  active_focus_ptr = NULL;
}

/** Reset the list of currently available foci */
void
reset_foci(void)
{
  if (active_focus_ptr != NULL) {
    remove_focus();
  }
  g_list_free(text_foci);
  text_foci = NULL;
}

/** Removes all foci owned by the object.
 * Returns TRUE if the object had the active focus.
 */
gboolean
remove_focus_object(DiaObject *obj)
{
  GList *tmplist = text_foci;
  gboolean active = FALSE;

  for (; tmplist != NULL; ) {
    Focus *focus = (Focus*)tmplist->data;
    GList *link = tmplist;
    tmplist = g_list_next(tmplist);
    if (focus->obj == obj) {
      text_foci = g_list_delete_link(text_foci, link);
      if (focus == active_focus_ptr) {
	active = TRUE;
      }
    }
  }
  return active;
}
