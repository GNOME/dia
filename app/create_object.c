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

#include "create_object.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "preferences.h"
#include "undo.h"
#include "cursor.h"

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
  Point clickedpoint, origpoint;
  Handle *handle1;
  Handle *handle2;
  Object *obj;
  Object *parent_obj;
  real click_distance;
  GList *avoid = NULL;

  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  origpoint = clickedpoint;

  snap_to_grid(ddisp, &clickedpoint.x, &clickedpoint.y);

  click_distance = ddisplay_untransform_length(ddisp, 3.0);

  obj = dia_object_default_create (tool->objtype, &clickedpoint,
                                   tool->user_data,
                                   &handle1, &handle2);

  diagram_add_object(ddisp->diagram, obj);

  /* Make sure to not parent to itself (!) */
  avoid = g_list_prepend(avoid, obj);
  /* Try a connect */
  if (handle1 != NULL &&
      handle1->connect_type != HANDLE_NONCONNECTABLE) {
    ConnectionPoint *connectionpoint;
    connectionpoint =
      object_find_connectpoint_display(ddisp, &origpoint, obj);
    if (connectionpoint != NULL) {
      (obj->ops->move)(obj, &origpoint);
      /* Make sure to not parent to the object connected to */
      avoid = g_list_prepend(avoid, connectionpoint->object);
    }
  }
  

  parent_obj = diagram_find_clicked_object_except(ddisp->diagram, 
						  &clickedpoint,
						  click_distance,
						  avoid);

  g_list_free(avoid);

  if (parent_obj && parent_obj->can_parent) /* starting point is within another object */
  {
    Change *change = undo_parenting(ddisp->diagram, parent_obj, obj, TRUE);
    (change->apply)(change, ddisp->diagram);
    /*
    obj->parent = parent_obj;
    parent_obj->children = g_list_append(parent_obj->children, obj);
    */
  }

  if (!(event->state & GDK_SHIFT_MASK)) {
    /* Not Multi-select => remove current selection */
    diagram_remove_all_selected(ddisp->diagram, TRUE);
  }
  diagram_select(ddisp->diagram, obj);

  obj->ops->selectf(obj, &clickedpoint,
		ddisp->renderer);

  tool->obj = obj;

  /* Connect first handle if possible: */
  if ((handle1!= NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1);
  }

  object_add_updates(obj, ddisp->diagram);
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);
  
  if (handle2 != NULL) {
    tool->handle = handle2;
    tool->moving = TRUE;
    tool->last_to = handle2->pos;
    
    gdk_pointer_grab (ddisp->canvas->window, FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, event->time);
    ddisplay_set_all_cursor(get_cursor(CURSOR_SCROLL));
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
  GList *list = NULL;
  if (tool->moving) {
    gdk_pointer_ungrab (event->time);

    object_add_updates(tool->obj, ddisp->diagram);
    tool->obj->ops->move_handle(tool->obj, tool->handle, &tool->last_to,
				NULL, HANDLE_MOVE_CREATE_FINAL, 0);
    object_add_updates(tool->obj, ddisp->diagram);

  }

  list = g_list_prepend(list, tool->obj);

  undo_insert_objects(ddisp->diagram, list, 1); 

  if (tool->moving) {
    if (tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display(ddisp, tool->obj, tool->handle);
      diagram_update_connections_selection(ddisp->diagram);
      diagram_flush(ddisp->diagram);
    }
    tool->moving = FALSE;
    tool->handle = NULL;
    tool->obj = NULL;
  }
  
  highlight_reset_all(ddisp->diagram);
  diagram_update_extents(ddisp->diagram);
  diagram_modified(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
  
  if (prefs.reset_tools_after_create != tool->invert_persistence)
      tool_reset();
  ddisplay_set_all_cursor(default_cursor);
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

  /* make sure the new object is restricted to its parent */
  parent_handle_move_out_check(tool->obj, &to);

  /* Move to ConnectionPoint if near: */
  if ((tool->handle != NULL &&
       tool->handle->connect_type != HANDLE_NONCONNECTABLE) &&
      ((connectionpoint =
	object_find_connectpoint_display(ddisp, &to, tool->obj)) != NULL)) {
    to = connectionpoint->pos;
    highlight_object(connectionpoint->object, NULL, ddisp->diagram);
    ddisplay_set_all_cursor(get_cursor(CURSOR_CONNECT));
  } else {
    /* No connectionopoint near, then snap to grid (if enabled) */
    snap_to_grid(ddisp, &to.x, &to.y);
    highlight_reset_all(ddisp->diagram);
    ddisplay_set_all_cursor(get_cursor(CURSOR_SCROLL));
  }

  object_add_updates(tool->obj, ddisp->diagram);
  tool->obj->ops->move_handle(tool->obj, tool->handle, &to, connectionpoint,
			      HANDLE_MOVE_CREATE, 0);
  object_add_updates(tool->obj, ddisp->diagram);
  
  diagram_flush(ddisp->diagram);

  tool->last_to = to;
  
  return;
}



Tool *
create_create_object_tool(ObjectType *objtype, void *user_data,
			  int invert_persistence)
{
  CreateObjectTool *tool;

  tool = g_new0(CreateObjectTool, 1);
  tool->tool.type = CREATE_OBJECT_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &create_object_button_press;
  tool->tool.button_release_func = (ButtonReleaseFunc) &create_object_button_release;
  tool->tool.motion_func = (MotionFunc) &create_object_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &create_object_double_click;
    
  tool->objtype = objtype;
  tool->user_data = user_data;
  tool->moving = FALSE;
  tool->invert_persistence = invert_persistence;
  
  return (Tool *) tool;
}

void free_create_object_tool(Tool *tool)
{
  g_free(tool);
}
