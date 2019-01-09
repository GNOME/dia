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
#include <config.h>

#include "textedit_tool.h"

#include "diagram.h"
#include "textedit.h"
#include "cursor.h"
#include "object_ops.h"

G_DEFINE_TYPE (DiaTextEditTool, dia_text_edit_tool, DIA_TYPE_TOOL)

/** The text edit tool.  This tool allows the user to switch to a mode where
 * clicking on an editable text will start text edit mode.  Clicking outside
 * of editable text will revert to selection tool.  Note that clicking this
 * tool doesn't enter text edit mode immediately, just allows it to be entered
 * by clicking an object.
 */
static DiaObject *
click_select_object (DiaDisplay     *ddisp,
                     Point          *clickedpoint,
                     GdkEventButton *event)
{
  real click_distance = dia_display_untransform_length(ddisp, 3.0);
  Diagram *diagram = ddisp->diagram;
  DiaObject *obj;
  
  dia_display_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint->x, &clickedpoint->y);

  obj = diagram_find_clicked_object (diagram, clickedpoint, click_distance);
  
  if (obj) {
    /* Selected an object. */
    GList *already;
    /*printf("Selected object!\n");*/
      
    already = g_list_find(diagram->data->selected, obj);
    if (already == NULL) { /* Not already selected */
      if (!(event->state & GDK_SHIFT_MASK)) {
        /* Not Multi-select => remove current selection */
        diagram_remove_all_selected (diagram, TRUE);
      }
      diagram_select (diagram, obj);
    }
    dia_display_do_update_menu_sensitivity (ddisp);
    object_add_updates_list (diagram->data->selected, diagram);
    diagram_flush (diagram);

    return obj;
  }

  return obj;
}

static void
text_edit_button_press (DiaTool        *tool,
                        GdkEventButton *event,
                        DiaDisplay     *ddisp)
{
  DiaTextEditTool *self = DIA_TEXT_EDIT_TOOL (tool);
  Point clickedpoint;
  Diagram *diagram = ddisp->diagram;
  DiaObject *obj = click_select_object (ddisp, &clickedpoint, event);

  if (obj) {
    if (obj != self->object)
      textedit_deactivate_focus ();

    /*  set cursor position */
    if (textedit_activate_object (ddisp, obj, &clickedpoint)) {
      self->object = obj;
      self->start_at = clickedpoint;
      self->state = STATE_TEXT_SELECT;
    } else {
      /* Clicked outside of editable object, stop editing */
      tool_reset ();
    }
  } else {
    textedit_deactivate_focus ();
    diagram_remove_all_selected (diagram, TRUE);
    tool_reset ();
  }
}

static void
text_edit_button_release (DiaTool        *tool,
                          GdkEventButton *event,
                          DiaDisplay     *ddisp)
{
  Point clickedpoint;
  DiaObject *obj = click_select_object (ddisp, &clickedpoint, event);
  
  if (obj) {
    dia_display_do_update_menu_sensitivity (ddisp);

    DIA_TEXT_EDIT_TOOL (tool)->state = STATE_TEXT_EDIT;
    /* no selection in the text editing yes */
  } else {
    /* back to modifying if we dont have an object */
    textedit_deactivate_focus ();
    tool_reset ();
  }
}

static void
text_edit_motion (DiaTool        *tool,
                  GdkEventMotion *event,
                  DiaDisplay     *ddisp)
{
  /* if we implement text selection here we could update the visual feedback */
}

static void
text_edit_double_click (DiaTool        *tool,
                        GdkEventButton *event,
                        DiaDisplay     *ddisp)
{
  /* if we implment text selection this should select a word */
}

static void
activate (DiaTool *tool)
{
  DiaDisplay *ddisp;

  dia_display_set_all_cursor (get_cursor (CURSOR_XTERM));

  ddisp = dia_display_active();
  if (ddisp) {
    if (textedit_activate_first (ddisp)) {
      /*  set the focus to the canvas area  */
      gtk_widget_grab_focus (ddisp->canvas);
    }
    dia_display_flush (ddisp);
    /* the above may have entered the textedit mode, just update in any case */
    dia_display_do_update_menu_sensitivity (ddisp);
  }
}

static void
deactivate (DiaTool *tool)
{
  DiaDisplay *ddisp = dia_display_active ();
  if (ddisp) {
    textedit_deactivate_focus ();
    dia_display_flush (ddisp);
  }
  dia_display_set_all_cursor (default_cursor);
}

static void
dia_text_edit_tool_class_init (DiaTextEditToolClass *klass)
{
  DiaToolClass *tool_class = DIA_TOOL_CLASS (klass);

  tool_class->activate = activate;
  tool_class->deactivate = deactivate;

  tool_class->button_press = text_edit_button_press;
  tool_class->button_release = text_edit_button_release;
  tool_class->motion = text_edit_motion;
  tool_class->double_click = text_edit_double_click;
}

static void
dia_text_edit_tool_init (DiaTextEditTool *self)
{
}
