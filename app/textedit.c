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
 */

#include <config.h>

#include "object.h"
#include "focus.h"
#include "display.h"
#include "highlight.h"
#include "textedit.h"
#include "object_ops.h"
#include "text.h"

static void
textedit_end_edit(DDisplay *ddisp, Focus *focus) 
{
  /* During destruction of the diagram the display may already be gone */
  if (!ddisp)
    return;

  /* Leak of focus highlight color here, but should it be handled
     by highlight or by us?
  */
  highlight_object_off(focus->obj, ddisp->diagram);
  object_add_updates(focus->obj, ddisp->diagram);
}

static void
textedit_begin_edit(DDisplay *ddisp, Focus *focus)
{
  Color *focus_col = color_new_rgb(1.0, 1.0, 0.0);
  
  g_assert(dia_object_is_selected(focus_get_object(focus)));
  highlight_object(focus->obj, focus_col, ddisp->diagram);
  object_add_updates(focus->obj, ddisp->diagram);
}

/** Move the text edit focus either backwards or forwards. */
Focus *
textedit_move_focus(DDisplay *ddisp, Focus *focus, gboolean forwards)
{
  if (focus != NULL) {
    textedit_end_edit(ddisp, focus);
  }
  if (forwards) {
    Focus *new_focus = focus_next();
    if (new_focus != NULL) give_focus(new_focus);    
  } else {
    Focus *new_focus = focus_previous();
    if (new_focus != NULL) give_focus(new_focus);    
  }
  focus = active_focus();

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
  if (active_focus()) {
    Focus *old_focus = active_focus();
    textedit_end_edit(ddisp, old_focus);
  }
  if (clicked) {
      text_set_cursor((Text*)focus->user_data, clicked, ddisp->renderer);
  }
  textedit_begin_edit(ddisp, focus);
  give_focus(focus);
  diagram_flush(ddisp->diagram);
}

/** Call when an object is chosen for activation (e.g. due to creation).
 */
void
textedit_activate_object(DDisplay *ddisp, DiaObject *obj, Point *clicked)
{
  Focus *new_focus;

  if (active_focus()) {
    Focus *focus = active_focus();
    textedit_end_edit(ddisp, focus);
  }
  new_focus = focus_get_first_on_object(obj);
  if (new_focus != NULL) {
    give_focus(new_focus); 
    if (clicked) {
      text_set_cursor((Text*)new_focus->user_data, clicked, ddisp->renderer);
    }
    textedit_begin_edit(ddisp, new_focus);
    diagram_flush(ddisp->diagram);
  }
}

/** Call to activate the first editable selected object.
 * Deactivates the old edit.
 */
void
textedit_activate_first(DDisplay *ddisp)
{
  Focus *new_focus = NULL;
  GList *selected = diagram_get_sorted_selected(ddisp->diagram);

  if (active_focus()) {
    Focus *focus = active_focus();
    textedit_end_edit(ddisp, focus);
  }
  while (new_focus != NULL && selected != NULL) {
    DiaObject *obj = (DiaObject*) selected->data;
    new_focus = focus_get_first_on_object(obj);
  }
  if (new_focus != NULL) {
    give_focus(new_focus); 
    textedit_begin_edit(ddisp, new_focus);
    diagram_flush(ddisp->diagram);
  }  
}

/** Call when something causes the text focus to disappear.
 * Does not remove objects from the focus list, but removes the
 * focus highlight and stuff.
 * Calling remove_focus on the active object or remove_focus_all
 * implies deactivating the focus. */
void
textedit_deactivate_focus(void)
{
  Focus *focus = active_focus();
  if (focus != NULL) {
    textedit_end_edit(ddisplay_active(), focus);
    remove_focus();
  }
}

/** Call when something should be removed from the focus list */
void
textedit_remove_focus(DiaObject *obj, Diagram *diagram)
{
  Focus *old_focus = active_focus();
  if (remove_focus_object(obj)) {
    /* TODO: make sure the focus is deactivated */
    textedit_end_edit(ddisplay_active(), old_focus);
  }
  g_free(old_focus);
}

/** Call when the entire list of focusable texts gets reset. */
void
textedit_remove_focus_all(Diagram *diagram)
{
  Focus *focus = active_focus();
  if (focus != NULL) {
    /* TODO: make sure the focus is deactivated */
    textedit_end_edit(ddisplay_active(), focus);
  }
  reset_foci();
}
