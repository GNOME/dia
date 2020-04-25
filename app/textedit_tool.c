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


/**
 * click_select_object:
 * @ddisp: the #DDisplay
 * @clickedpoint: the #Point clicked
 * @event: the #GdkEventButton
 *
 * The text edit tool.  This tool allows the user to switch to a mode where
 * clicking on an editable text will start text edit mode.  Clicking outside
 * of editable text will revert to selection tool.  Note that clicking this
 * tool doesn't enter text edit mode immediately, just allows it to be entered
 * by clicking an object.
 *
 * Since: dawn-of-time
 */
static DiaObject *
click_select_object (DDisplay       *ddisp,
                     Point          *clickedpoint,
                     GdkEventButton *event)
{
  double click_distance = ddisplay_untransform_length (ddisp, 3.0);
  Diagram *diagram = ddisp->diagram;
  DiaObject *obj;

  ddisplay_untransform_coords(ddisp,
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
	diagram_remove_all_selected(diagram, TRUE);
      }
      diagram_select(diagram, obj);
    }
    ddisplay_do_update_menu_sensitivity(ddisp);
    object_add_updates_list(diagram->data->selected, diagram);
    diagram_flush(diagram);

    return obj;
  }

  return obj;
}

static void
textedit_button_press(TexteditTool *tool, GdkEventButton *event,
		      DDisplay *ddisp)
{
  Point clickedpoint;
  Diagram *diagram = ddisp->diagram;
  DiaObject *obj = click_select_object (ddisp, &clickedpoint, event);

  if (obj) {
    if (obj != tool->object)
      textedit_deactivate_focus ();

    /*  set cursor position */
    if (textedit_activate_object(ddisp, obj, &clickedpoint)) {
      tool->object = obj;
      tool->start_at = clickedpoint;
      tool->state = STATE_TEXT_SELECT;
    } else {
      /* Clicked outside of editable object, stop editing */
      tool_reset();
    }
  } else {
    textedit_deactivate_focus ();
    diagram_remove_all_selected(diagram, TRUE);
    tool_reset();
  }
}

static void
textedit_button_release(TexteditTool *tool, GdkEventButton *event,
		        DDisplay *ddisp)
{
  Point clickedpoint;
  DiaObject *obj = click_select_object (ddisp, &clickedpoint, event);

  if (obj) {
    ddisplay_do_update_menu_sensitivity(ddisp);

    tool->state = STATE_TEXT_EDIT;
    /* no selection in the text editing yes */
  } else {
    /* back to modifying if we dont have an object */
    textedit_deactivate_focus();
    tool_reset ();
  }
}

static void
textedit_motion(TexteditTool *tool, GdkEventMotion *event,
	        DDisplay *ddisp)
{
  /* if we implement text selection here we could update the visual feedback */
}

static void
textedit_double_click(TexteditTool *tool, GdkEventButton *event,
		      DDisplay *ddisp)
{
  /* if we implement text selection this should select a word */
}

Tool *
create_textedit_tool(void)
{
  TexteditTool *tool;
  DDisplay *ddisp;

  tool = g_new0(TexteditTool, 1);
  tool->tool.type = TEXTEDIT_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &textedit_button_press;
  tool->tool.button_release_func = (ButtonReleaseFunc) &textedit_button_release;
  tool->tool.motion_func = (MotionFunc) &textedit_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &textedit_double_click;

  ddisplay_set_all_cursor_name (NULL, "text");

  ddisp = ddisplay_active();
  if (ddisp) {
    if (textedit_activate_first (ddisp)) {
      /*  set the focus to the canvas area  */
      gtk_widget_grab_focus (ddisp->canvas);
    }
    ddisplay_flush(ddisp);
    /* the above may have entered the textedit mode, just update in any case */
    ddisplay_do_update_menu_sensitivity(ddisp);
  }

  return (Tool *)tool;
}


void
free_textedit_tool (Tool *tool)
{
  DDisplay *ddisp = ddisplay_active();
  if (ddisp) {
    textedit_deactivate_focus ();
    ddisplay_flush(ddisp);
  }
  ddisplay_set_all_cursor(default_cursor);

  g_clear_pointer (&tool, g_free);
}
