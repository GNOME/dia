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
#include "create_object.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "preferences.h"

static void create_object_button_press(CreateObjectTool *tool, GdkEventButton *event,
				     DDisplay *ddisp);
static void create_object_button_release(CreateObjectTool *tool, GdkEventButton *event,
				       DDisplay *ddisp);
static void create_object_motion(CreateObjectTool *tool, GdkEventMotion *event,
				 DDisplay *ddisp);
static void create_object_double_click(CreateObjectTool *tool, GdkEventMotion *event,
				       DDisplay *ddisp);


static void
create_object_button_press(CreateObjectTool *tool, GdkEventButton *event,
			   DDisplay *ddisp)
{
  Point clickedpoint;
  Handle *handle1;
  Handle *handle2;
  Object *obj;
  
  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  snap_to_grid(&ddisp->grid, &clickedpoint.x, &clickedpoint.y);
  
  obj = tool->objtype->ops->create(&clickedpoint, tool->user_data,
				   &handle1, &handle2);
  diagram_add_object(ddisp->diagram, obj);
  
  if (!(event->state & GDK_SHIFT_MASK)) {
    /* Not Multi-select => remove current selection */
    diagram_remove_all_selected(ddisp->diagram, TRUE);
  }
  diagram_add_selected(ddisp->diagram, obj);

  obj->ops->select(obj, &clickedpoint,
		   (Renderer *)ddisp->renderer);

  /* Connect first handle if possible: */
  if ((handle1!= NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1);
  }

  object_add_updates(obj, ddisp->diagram);
  diagram_flush(ddisp->diagram);
  
  if (handle2 != NULL) {
    tool->obj = obj;
    tool->handle = handle2;
    tool->moving = TRUE;
    tool->last_to = handle2->pos;
    
    gdk_pointer_grab (ddisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, event->time);
  } else {
    diagram_update_extents(ddisp->diagram);
    tool->moving = FALSE;
  }
}

static void
create_object_double_click(CreateObjectTool *tool, GdkEventMotion *event,
			   DDisplay *ddisp)
{
}

static void
create_object_button_release(CreateObjectTool *tool, GdkEventButton *event,
			     DDisplay *ddisp)
{
  if (tool->moving) {
    gdk_pointer_ungrab (event->time);

    object_add_updates(tool->obj, ddisp->diagram);
    tool->obj->ops->move_handle(tool->obj, tool->handle, &tool->last_to,
				HANDLE_MOVE_USER_FINAL);
    object_add_updates(tool->obj, ddisp->diagram);


    if (tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display(ddisp, tool->obj, tool->handle);
      diagram_update_connections_selection(ddisp->diagram);
      diagram_flush(ddisp->diagram);
    }
    tool->moving = FALSE;
    tool->handle = NULL;
    tool->obj = NULL;
  }
  diagram_update_extents(ddisp->diagram);
  diagram_modified(ddisp->diagram);
  if (prefs.reset_tools_after_create)
      tool_reset();
}
static void
create_object_motion(CreateObjectTool *tool, GdkEventMotion *event,
		   DDisplay *ddisp)
{
  Point to;
  ConnectionPoint *connectionpoint;
  
  if (!tool->moving)
    return;
  
  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  /* Move to ConnectionPoint if near: */
  connectionpoint =
    object_find_connectpoint_display(ddisp, &to);
  if ( (tool->handle->connect_type != HANDLE_NONCONNECTABLE) &&
       (connectionpoint != NULL) ) {
    to = connectionpoint->pos;
  } else {
    /* No connectionopoint near, then snap to grid (if enabled) */
    snap_to_grid(&ddisp->grid, &to.x, &to.y);
  }

  object_add_updates(tool->obj, ddisp->diagram);
  tool->obj->ops->move_handle(tool->obj, tool->handle, &to,
			      HANDLE_MOVE_USER);
  object_add_updates(tool->obj, ddisp->diagram);
  
  diagram_flush(ddisp->diagram);

  tool->last_to = to;
  
  return;
}



Tool *create_create_object_tool(ObjectType *objtype, void *user_data)
{
  CreateObjectTool *tool;

  tool = g_new(CreateObjectTool, 1);
  tool->tool.type = CREATE_OBJECT_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &create_object_button_press;
  tool->tool.button_release_func = (ButtonReleaseFunc) &create_object_button_release;
  tool->tool.motion_func = (MotionFunc) &create_object_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &create_object_double_click;
    
  tool->objtype = objtype;
  tool->user_data = user_data;
  tool->moving = FALSE;
  
  return (Tool *) tool;
}

void free_create_object_tool(Tool *tool)
{
  g_free(tool);
}
