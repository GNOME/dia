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

/** This file handles the display part of text edit stuff: Making text
 * edit start and stop at the right time and highlighting the edits.
 * lib/text.c and lib/focus.c handles internal things like who can have
 * focus and how to enter text.  app/disp_callbacks.c handles the actual
 * keystrokes.
 *
 * There's an invariant that all objects in the focus list must be selected.
 *
 * When starting to edit a particular text, we force entering text edit mode,
 * and when leaving text edit mode, we force stopping editing the text.
 * However, when changing between which texts are edited, we don't leave
 * textedit mode just to enter it again.  However, due to the text edit
 * tool, it is possible to be in text edit mode without editing any
 * particular text.
 */

#include <config.h>

#include "object.h"
#include "focus.h"
#include "display.h"
#include "highlight.h"
#include "textedit.h"
#include "object_ops.h"
#include "text.h"

typedef struct TextEditChange {
  Change obj_change;

  /** The text before editing began */
  gchar *orig_text;
  /** The text after editing finished */
  gchar *new_text;
  /** The Text item that this change happened to (in case there's more than
   *  one on an object). */
  Text *text;
} TextEditChange;

static void textedit_end_edit(DDisplay *ddisp, Focus *focus);

/** Returns TRUE if the given display is currently in text-edit mode. */
gboolean
textedit_mode(DDisplay *ddisp)
{
  return ddisplay_active_focus(ddisp) != NULL;
}

/** Perform the necessary changes to the display according to whether
 *  or not we are in text edit mode.  While normally called from
 *  textedit_enter and textedit_exit, this is exposed in order to allow
 *  for switching between displays.

 * @param ddisp The display to set according to mode.
 */
static void
textedit_display_change(DDisplay *ddisp)
{
}

/** Start editing text.  This brings Dia into text-edit mode, which
 * changes some menu items.  We may or may not already be in text-edit mode,
 * but at the return of this function we are known to be in text-edit mode.
 * @param ddisp The display that editing happens in.
 */
static void
textedit_enter(DDisplay *ddisp)
{
  if (textedit_mode(ddisp)) {
    return;
  }
  /* Set textedit menus */
  /* Set textedit key-event handler */
  textedit_display_change(ddisp);
}

/** Stop editing text.  Whether or not we already are in text-edit mode,
 * this function leaves us in object-edit mode.  This function will call
 * textedit_end_edit if necessary.
 *
 * @param ddisp The display that editing happens in.
 */
static void
textedit_exit(DDisplay *ddisp)
{
  if (!textedit_mode(ddisp)) {
    return;
  }
  if (ddisplay_active_focus(ddisp) != NULL) {
    textedit_end_edit(ddisp, ddisplay_active_focus(ddisp));
  }
  /* Set object-edit menus */
  /* Set object-edit key-event handler */
  textedit_display_change(ddisp);
}


/** Begin editing a particular text focus.  This function will call
 * textedit_enter if necessary.  By return from this function, we will
 * be in textedit mode.
 * @param ddisp The display in use
 * @param focus The text focus to edit 
 */
static void
textedit_begin_edit(DDisplay *ddisp, Focus *focus)
{
  g_assert(dia_object_is_selected(focus_get_object(focus)));
  if (!textedit_mode(ddisp)) {
    textedit_enter(ddisp);
  }
  ddisplay_set_active_focus(ddisp, focus);
  highlight_object(focus->obj, DIA_HIGHLIGHT_TEXT_EDIT, ddisp->diagram);
  object_add_updates(focus->obj, ddisp->diagram);
}

/** Stop editing a particular text focus.  This must only be called in
 * text-edit mode.  This handles the object-specific changes required to
 * edit it.
 * @param ddisp The display in use
 * @param focus The text focus to stop editing
 */
static void
textedit_end_edit(DDisplay *ddisp, Focus *focus) 
{
  /* During destruction of the diagram the display may already be gone */
  if (!ddisp)
    return;

  g_assert(textedit_mode(ddisp));

  /* Leak of focus highlight color here, but should it be handled
     by highlight or by us?
  */
  highlight_object_off(focus->obj, ddisp->diagram);
  object_add_updates(focus->obj, ddisp->diagram);
  diagram_object_modified(ddisp->diagram, focus->obj);
  ddisplay_set_active_focus(ddisp, NULL);
}

/** Move the text edit focus either backwards or forwards. */
Focus *
textedit_move_focus(DDisplay *ddisp, Focus *focus, gboolean forwards)
{
  g_return_val_if_fail (ddisp != NULL, NULL);
  if (focus != NULL) {
    textedit_end_edit(ddisp, focus);
  }
  if (forwards) {
    Focus *new_focus = focus_next_on_diagram((DiagramData *) ddisp->diagram);
    if (new_focus != NULL) give_focus(new_focus);    
  } else {
    Focus *new_focus = focus_previous_on_diagram((DiagramData *) ddisp->diagram);
    if (new_focus != NULL) give_focus(new_focus);    
  }
  focus = get_active_focus((DiagramData *) ddisp->diagram);

  if (focus != NULL) {
    textedit_begin_edit(ddisp, focus);
  }
  diagram_flush(ddisp->diagram);
  return focus;
}

/** Call when something recieves an actual focus (not to be confused
 * with doing request_focus(), which merely puts one in the focus list).
 */
void
textedit_activate_focus(DDisplay *ddisp, Focus *focus, Point *clicked)
{
  Focus *old_focus = get_active_focus((DiagramData *) ddisp->diagram);
  if (old_focus != NULL) {
    textedit_end_edit(ddisp, old_focus);
  }
  if (clicked) {
      text_set_cursor(focus->text, clicked, ddisp->renderer);
  }
  textedit_begin_edit(ddisp, focus);
  give_focus(focus);
  diagram_flush(ddisp->diagram);
}

/** Call when an object is chosen for activation (e.g. due to creation).
 * Calling this function will put us into text-edit mode if there is
 * text to edit, otherwise it will take us out of text-edit mode.
 *
 * Returns true if there is something to text edit.
 */
gboolean
textedit_activate_object(DDisplay *ddisp, DiaObject *obj, Point *clicked)
{
  Focus *new_focus;

  new_focus = focus_get_first_on_object(obj);
  if (new_focus != NULL) {
    Focus *focus = get_active_focus((DiagramData *) ddisp->diagram);
    if (focus != NULL) {
      textedit_end_edit(ddisp, focus);
    }
    give_focus(new_focus); 
    if (clicked) {
      text_set_cursor(new_focus->text, clicked, ddisp->renderer);
    }
    textedit_begin_edit(ddisp, new_focus);
    diagram_flush(ddisp->diagram);
    return TRUE;
  } else {
    textedit_exit(ddisp);
    return FALSE;
  }
}

/** Call to activate the first editable selected object.
 * Deactivates the old edit.
 * Calling this function will put us into text-edit mode if there is
 * text to edit, otherwise it will take us out of text-edit mode.
 */
gboolean
textedit_activate_first(DDisplay *ddisp)
{
  Focus *new_focus = NULL;
  GList *tmp, *selected = diagram_get_sorted_selected(ddisp->diagram);
  Focus *focus = get_active_focus((DiagramData *) ddisp->diagram);

  if (focus != NULL) {
    textedit_end_edit(ddisp, focus);
  }
  tmp = selected;
  while (new_focus == NULL && tmp != NULL) {
    DiaObject *obj = (DiaObject*) selected->data;
    new_focus = focus_get_first_on_object(obj);
    tmp = g_list_next(tmp);
  }
  g_list_free (selected);
  if (new_focus != NULL) {
    give_focus(new_focus); 
    textedit_begin_edit(ddisp, new_focus);
    diagram_flush(ddisp->diagram);
    return TRUE;
  } else {
    textedit_exit(ddisp);
    return FALSE;
  }
}

/** Call when something causes the text focus to disappear.
 * Does not remove objects from the focus list, but removes the
 * focus highlight and stuff.
 * Calling remove_focus on the active object or remove_focus_all
 * implies deactivating the focus.
 * Calling this takes us out of textedit mode.
 */
void
textedit_deactivate_focus(void)
{
  if (ddisplay_active() != NULL) {
    DiagramData *dia = (DiagramData *) ddisplay_active()->diagram;
    Focus *focus = get_active_focus(dia);
    if (focus != NULL) {
      remove_focus_on_diagram(dia);
    }
    /* This also ends the edit */
    textedit_exit(ddisplay_active());
  }
}

/** Call when something should be removed from the focus list.
 * Calling this takes us out of textedit mode.
 */
void
textedit_remove_focus(DiaObject *obj, Diagram *diagram)
{
  Focus *old_focus = get_active_focus((DiagramData *) diagram);
  if (remove_focus_object(obj)) {
    /* TODO: make sure the focus is deactivated */
  }
  /* This also ends the edit */
  if (ddisplay_active() != NULL) {
    textedit_exit(ddisplay_active());
  }
}

/** Call when the entire list of focusable texts gets reset.
 * Calling this takes us out of textedit mode.
 */
void
textedit_remove_focus_all(Diagram *diagram)
{
  Focus *focus = get_active_focus((DiagramData *) diagram);
  if (focus != NULL) {
    /* TODO: make sure the focus is deactivated */
  }
  reset_foci_on_diagram((DiagramData *) diagram);
  /* This also ends the edit */
  if (ddisplay_active() != NULL) {
    textedit_exit(ddisplay_active());
  }
}

/* *************** Textedit-mode related Undo ******************* */

/* Each edit of a text part of an object counts as one undo after it's
 * done.  While editing, full undo is available, but afterwards the
 * changes get merged into one.  This is done by sticking a TextEditChange
 * on the undo stack at the beginning with the original text, and then
 * at the end removing every change after that one.  This is why non-text-edit
 * changes are not allowed in text edit mode:  It would break the undo.
 */

