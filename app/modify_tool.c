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
#include <stdio.h>
#include <math.h>

#include "modify_tool.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "message.h"
#include "properties.h"
#include "render_gdk.h"

static Object *click_select_object(DDisplay *ddisp, Point *clickedpoint,
				   GdkEventButton *event);
static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
				Point *clickedpoint,
				GdkEventButton *event);
static void modify_button_press(ModifyTool *tool, GdkEventButton *event,
				 DDisplay *ddisp);
static void modify_button_release(ModifyTool *tool, GdkEventButton *event,
				  DDisplay *ddisp);
static void modify_motion(ModifyTool *tool, GdkEventMotion *event,
			  DDisplay *ddisp);
static void modify_double_click(ModifyTool *tool, GdkEventButton *event,
				DDisplay *ddisp);

Tool *
create_modify_tool(void)
{
  ModifyTool *tool;

  tool = g_new(ModifyTool, 1);
  tool->tool.type = MODIFY_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &modify_button_press;
  tool->tool.button_release_func = (ButtonReleaseFunc) &modify_button_release;
  tool->tool.motion_func = (MotionFunc) &modify_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &modify_double_click;
  tool->gc = NULL;
  tool->state = STATE_NONE;
  tool->break_connections = 0;

  return (Tool *)tool;
}


void
free_modify_tool(Tool *tool)
{
  g_free(tool);
}

/*
  This function is buggy. Fix it later!
static void
transitive_select(DDisplay *ddisp, Point *clickedpoint, Object *obj)
{
  guint i;
  GList *j;
  Object *obj1;

  for(i = 0; i < obj->num_connections; i++) {
    printf("%d\n", i);
    j = obj->connections[i]->connected;
    while(j != NULL && (obj1 = (Object *)j->data) != NULL) {
      diagram_add_selected(ddisp->diagram, obj1);
      obj1->ops->select(obj1, clickedpoint,
			(Renderer *)ddisp->renderer);
      transitive_select(ddisp, clickedpoint, obj1);
      j = g_list_next(j);
    }
  }
}
*/

static Object *
click_select_object(DDisplay *ddisp, Point *clickedpoint,
		    GdkEventButton *event)
{
  Diagram *diagram;
  real click_distance;
  Object *obj;
  
  diagram = ddisp->diagram;
  
  /* Find the closest object to select it: */

  click_distance = ddisplay_untransform_length(ddisp, 3.0);
  
  obj = diagram_find_clicked_object(diagram, clickedpoint,
				    click_distance);

  if (obj!=NULL) {
    /* Selected an object. */
    GList *already;
    /*printf("Selected object!\n");*/
      
    already = g_list_find(diagram->data->selected, obj);
    if (already == NULL) { /* Not already selected */
      /*printf("Not already selected\n");*/

      if (!(event->state & GDK_SHIFT_MASK)) {
	/* Not Multi-select => remove current selection */
	diagram_remove_all_selected(diagram, TRUE);
      }
      
      diagram_add_selected(diagram, obj);
      obj->ops->select(obj, clickedpoint,
		       (Renderer *)ddisp->renderer);

      /*
	This stuff is buggy, fix it later.
      if (event->state & GDK_CONTROL_MASK) {
	transitive_select(ddisp, clickedpoint, obj);
      }
      */

      diagram_update_menu_sensitivity(diagram);
      object_add_updates_list(diagram->data->selected, diagram);
      diagram_flush(diagram);

      return obj;
    } else { /* Clicked on already selected. */
      /*printf("Already selected\n");*/
      obj->ops->select(obj, clickedpoint,
		       (Renderer *)ddisp->renderer);
      object_add_updates_list(diagram->data->selected, diagram);
      diagram_flush(diagram);
      
      if (event->state & GDK_SHIFT_MASK) { /* Multi-select */
	/* Remove the selected selected */
	diagram_update_menu_sensitivity(diagram);
	diagram_remove_selected(ddisp->diagram, (Object *)already->data);
	diagram_flush(ddisp->diagram);
      } else {
	return obj;
      }
    }
  } else { /* No object selected */
    /*printf("didn't select object\n");*/
    if (!(event->state & GDK_SHIFT_MASK)) {
      /* Not Multi-select => Remove all selected */
      diagram_update_menu_sensitivity(diagram);
      diagram_remove_all_selected(diagram, TRUE);
      diagram_flush(ddisp->diagram);
    }
  }
  return NULL;
}

static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
				Point *clickedpoint, GdkEventButton *event)
{
  Object *obj;
  Handle *handle;
  real dist;
  
  handle = NULL;
  dist = diagram_find_closest_handle(ddisp->diagram, &handle,
				     &obj, clickedpoint);
  if  (handle_is_clicked(ddisp, handle, clickedpoint)) {
    tool->state = STATE_MOVE_HANDLE;
    tool->break_connections = TRUE;
    tool->last_to = handle->pos;
    tool->handle = handle;
    tool->object = obj;
    gdk_pointer_grab (ddisp->canvas->window, FALSE,
                      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                      NULL, NULL, event->time);
    tool->start_at = *clickedpoint;
    return TRUE;
  }
  return FALSE;
}

static void
modify_button_press(ModifyTool *tool, GdkEventButton *event,
		     DDisplay *ddisp)
{
  Point clickedpoint;
  Object *clicked_obj;
  
  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  if (do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
    return;

  clicked_obj = click_select_object(ddisp, &clickedpoint, event);
  
  if (do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
    return;
  
  if ( clicked_obj != NULL ) {
    tool->state = STATE_MOVE_OBJECT;
    tool->object = clicked_obj;
    tool->move_compensate = clicked_obj->position;
    point_sub(&tool->move_compensate, &clickedpoint);
    tool->break_connections = TRUE;
    gdk_pointer_grab (ddisp->canvas->window, FALSE,
                      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                      NULL, NULL, event->time);
    tool->start_at = clickedpoint;
  } else {
    tool->state = STATE_BOX_SELECT;
    tool->start_box = clickedpoint;
    tool->end_box = clickedpoint;
    tool->x1 = tool->x2 = (int) event->x;
    tool->y1 = tool->y2 = (int) event->y;
    
    if (tool->gc == NULL) {
      tool->gc = gdk_gc_new(ddisp->renderer->pixmap);
      gdk_gc_set_line_attributes(tool->gc, 1, GDK_LINE_ON_OFF_DASH, 
				 GDK_CAP_BUTT, GDK_JOIN_MITER);
      gdk_gc_set_foreground(tool->gc, &color_gdk_white);
      gdk_gc_set_function(tool->gc, GDK_XOR);
    }

    gdk_draw_rectangle (ddisp->renderer->pixmap, tool->gc, FALSE,
			tool->x1, tool->y1,
			tool->x2 - tool->x1, tool->y2 - tool->y1);
    ddisplay_add_display_area(ddisp,
			      tool->x1, tool->y1,
			      tool->x2+1, tool->y2+1);

    ddisplay_flush(ddisp);
    gdk_pointer_grab (ddisp->canvas->window, FALSE,
                      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                      NULL, NULL, event->time);
  }
}


static void
modify_double_click(ModifyTool *tool, GdkEventButton *event,
		    DDisplay *ddisp)
{
  Point clickedpoint;
  Object *clicked_obj;
  
  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  clicked_obj = click_select_object(ddisp, &clickedpoint, event);
  
  if ( clicked_obj != NULL ) {
    properties_show(ddisp->diagram, clicked_obj);
  }
}


static void
modify_motion(ModifyTool *tool, GdkEventMotion *event,
	      DDisplay *ddisp)
{
  Point to;
  Point now, delta, full_delta;
  gboolean auto_scroll, vertical;
  ConnectionPoint *connectionpoint;

  if (tool->state==STATE_NONE)
    return; /* Fast path... */

  auto_scroll = ddisplay_autoscroll(ddisp, event->x, event->y);
  
  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  switch (tool->state) {
  case STATE_MOVE_OBJECT:
    
    if (tool->break_connections)
      diagram_unconnect_selected(ddisp->diagram);
  
    if (event->state & GDK_CONTROL_MASK) {
      full_delta = to;
      point_sub(&full_delta, &tool->start_at);
      vertical = (fabs(full_delta.x) < fabs(full_delta.y));
    }

    point_add(&to, &tool->move_compensate);
    snap_to_grid(&ddisp->grid, &to.x, &to.y);
  
    now = tool->object->position;
    
    delta = to;
    point_sub(&delta, &now);
    
    if (event->state & GDK_CONTROL_MASK) {
      /* Up-down or left-right */
      if (vertical) {
	delta.x = tool->start_at.x + tool->move_compensate.x - now.x;
      } else {
	delta.y = tool->start_at.y + tool->move_compensate.y - now.y;
      }
    }

    object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);
    object_list_move_delta(ddisp->diagram->data->selected, &delta);
    object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);
  
    diagram_update_connections_selection(ddisp->diagram);
    diagram_flush(ddisp->diagram);
    break;
  case STATE_MOVE_HANDLE:
    /* Move to ConnectionPoint if near: */
    connectionpoint =
      object_find_connectpoint_display(ddisp, &to);

    if (event->state & GDK_CONTROL_MASK) {
      full_delta = to;
      point_sub(&full_delta, &tool->start_at);
      vertical = (fabs(full_delta.x) < fabs(full_delta.y));
    }

    if ( (tool->handle->connect_type != HANDLE_NONCONNECTABLE) &&
	 (connectionpoint != NULL) ) {
      to = connectionpoint->pos;
    } else {
      /* No connectionopoint near, then snap to grid (if enabled) */
      snap_to_grid(&ddisp->grid, &to.x, &to.y);
    }

    if (tool->break_connections) {
      /* break connections to the handle currently selected. */
      if (tool->handle->connected_to!=NULL) {
	object_unconnect(tool->object, tool->handle);
      }
    }

    if (event->state & GDK_CONTROL_MASK) {
      /* Up-down or left-right */
      if (vertical) {
	to.x = tool->start_at.x;
      } else {
	to.y = tool->start_at.y;
      }
    }

    object_add_updates(tool->object, ddisp->diagram);
    tool->object->ops->move_handle(tool->object, tool->handle, &to,
				   HANDLE_MOVE_USER,0);
    object_add_updates(tool->object, ddisp->diagram);
  
    diagram_update_connections_selection(ddisp->diagram);
    diagram_flush(ddisp->diagram);
    break;
  case STATE_BOX_SELECT:

    if (! auto_scroll)
    {
      gdk_draw_rectangle (ddisp->renderer->pixmap, tool->gc, FALSE,
			  tool->x1, tool->y1,
			  tool->x2 - tool->x1, tool->y2 - tool->y1);
      ddisplay_add_display_area(ddisp,
				tool->x1-1, tool->y1-1,
				tool->x2+1, tool->y2+1);
    }

    tool->end_box = to;

    ddisplay_transform_coords(ddisp,
			      MIN(tool->start_box.x, tool->end_box.x),
			      MIN(tool->start_box.y, tool->end_box.y),
			      &tool->x1, &tool->y1);
    ddisplay_transform_coords(ddisp,
			      MAX(tool->start_box.x, tool->end_box.x),
			      MAX(tool->start_box.y, tool->end_box.y),
			      &tool->x2, &tool->y2);

    gdk_draw_rectangle (ddisp->renderer->pixmap, tool->gc, FALSE,
			tool->x1, tool->y1,
			tool->x2 - tool->x1, tool->y2 - tool->y1);
    ddisplay_add_display_area(ddisp,
			      tool->x1-1, tool->y1-1,
			      tool->x2+1, tool->y2+1);

    ddisplay_flush(ddisp);
    break;
  case STATE_NONE:
    
    break;
  default:
    message_error("Internal error: Strange state in modify_tool\n");
  }

  tool->last_to = to;
}


static void
modify_button_release(ModifyTool *tool, GdkEventButton *event,
		      DDisplay *ddisp)
{
  switch (tool->state) {
  case STATE_MOVE_OBJECT:
    gdk_pointer_ungrab (event->time);
    ddisplay_connect_selected(ddisp);
    diagram_update_connections_selection(ddisp->diagram);
    diagram_update_extents(ddisp->diagram);
    diagram_modified(ddisp->diagram);
    diagram_flush(ddisp->diagram);
    tool->state = STATE_NONE;
    break;
  case STATE_MOVE_HANDLE:
    gdk_pointer_ungrab (event->time);

    /* Final move: */
    object_add_updates(tool->object, ddisp->diagram);
    tool->object->ops->move_handle(tool->object, tool->handle,
				   &tool->last_to,
				   HANDLE_MOVE_USER_FINAL,0);
    object_add_updates(tool->object, ddisp->diagram);

    /* Connect if possible: */
    if (tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display(ddisp, tool->object, tool->handle);
      diagram_update_connections_selection(ddisp->diagram);
    }
    
    diagram_flush(ddisp->diagram);
    
    diagram_modified(ddisp->diagram);
    diagram_update_extents(ddisp->diagram);
    tool->state = STATE_NONE;
    break;
  case STATE_BOX_SELECT:
    gdk_pointer_ungrab (event->time);
    /* Remove last box: */
    gdk_draw_rectangle (ddisp->renderer->pixmap, tool->gc, FALSE,
			tool->x1, tool->y1,
			tool->x2 - tool->x1, tool->y2 - tool->y1);
    ddisplay_add_display_area(ddisp,
			      tool->x1-1, tool->y1-1,
			      tool->x2+1, tool->y2+1);

    {
      Rectangle r;
      GList *list;
      Object *obj;

      r.left = MIN(tool->start_box.x, tool->end_box.x);
      r.right = MAX(tool->start_box.x, tool->end_box.x);
      r.top = MIN(tool->start_box.y, tool->end_box.y);
      r.bottom = MAX(tool->start_box.y, tool->end_box.y);

      list =
	layer_find_objects_in_rectangle(ddisp->diagram->data->active_layer, &r);
      
      while (list != NULL) {
	obj = (Object *)list->data;

	if (!diagram_is_selected(ddisp->diagram, obj))
	  diagram_add_selected(ddisp->diagram, obj);
	
	list = g_list_next(list);
      }

      g_list_free(list);
      
    }
    
    diagram_update_menu_sensitivity(ddisp->diagram);
    ddisplay_flush(ddisp);

    tool->state = STATE_NONE;
    break;
  case STATE_NONE:
    break;
  default:
    message_error("Internal error: Strange state in modify_tool\n");
      
  }
  tool->break_connections = FALSE;
}






