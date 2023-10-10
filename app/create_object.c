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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "create_object.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "preferences.h"
#include "undo.h"
#include "cursor.h"
#include "highlight.h"
#include "textedit.h"
#include "parent.h"
#include "message.h"
#include "object.h"
#include "menus.h"
#include "widgets.h"
#include "dia-layer.h"

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
  DiaObject *obj;

  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  origpoint = clickedpoint;

  snap_to_grid(ddisp, &clickedpoint.x, &clickedpoint.y);

  obj = dia_object_default_create (tool->objtype, &clickedpoint,
                                   tool->user_data,
                                   &handle1, &handle2);

  tool->obj = obj; /* ensure that tool->obj is initialised in case we
		      return early. */
  if (!obj) {
    tool->moving = FALSE;
    tool->handle = NULL;
    message_error(_("'%s' creation failed"), tool->objtype ? tool->objtype->name : "NULL");
    return;
  }

  diagram_add_object(ddisp->diagram, obj);

  /* Try a connect */
  if (handle1 != NULL &&
      handle1->connect_type != HANDLE_NONCONNECTABLE) {
    ConnectionPoint *connectionpoint;
    connectionpoint =
      object_find_connectpoint_display(ddisp, &origpoint, obj, TRUE);
    if (connectionpoint != NULL) {
      (obj->ops->move)(obj, &origpoint);
    }
  }

  if (!(event->state & GDK_SHIFT_MASK)) {
    /* Not Multi-select => remove current selection */
    diagram_remove_all_selected(ddisp->diagram, TRUE);
  }
  diagram_select(ddisp->diagram, obj);

  /* Connect first handle if possible: */
  if ((handle1!= NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1, TRUE);
  }

  object_add_updates(obj, ddisp->diagram);
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);

  if (handle2 != NULL) {
    tool->handle = handle2;
    tool->moving = TRUE;
    tool->last_to = handle2->pos;

    gdk_device_grab (gdk_event_get_device ((GdkEvent*)event),
                     gtk_widget_get_window(ddisp->canvas),
                     GDK_OWNERSHIP_APPLICATION,
                     FALSE,
                     GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                     NULL, event->time);
    ddisplay_set_all_cursor_name (NULL, "move");
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
  DiaObject *obj = tool->obj;
  gboolean reset;

  GList *parent_candidates;

  g_return_if_fail (obj != NULL);
  if (!obj) /* not sure if this isn't enough */
    return; /* could be a legal invariant */

  if (tool->moving) {
    gdk_device_ungrab (gdk_event_get_device ((GdkEvent*)event), event->time);

    object_add_updates(tool->obj, ddisp->diagram);
    tool->obj->ops->move_handle(tool->obj, tool->handle, &tool->last_to,
				NULL, HANDLE_MOVE_CREATE_FINAL, 0);
    object_add_updates(tool->obj, ddisp->diagram);

  }

  parent_candidates =
    dia_layer_find_objects_containing_rectangle (obj->parent_layer,
                                                 &obj->bounding_box);

  /* whole object must be within another object to parent it */
  for (; parent_candidates != NULL; parent_candidates = g_list_next(parent_candidates)) {
    DiaObject *parent_obj = (DiaObject *) parent_candidates->data;
    if (obj != parent_obj
        && object_within_parent (obj, parent_obj)) {
      DiaChange *change = dia_parenting_change_new (ddisp->diagram, parent_obj, obj, TRUE);
      dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));
      break;
    /*
    obj->parent = parent_obj;
    parent_obj->children = g_list_append(parent_obj->children, obj);
    */
    }
  }
  g_list_free(parent_candidates);

  list = g_list_prepend(list, tool->obj);

  dia_insert_objects_change_new (ddisp->diagram, list, 1);

  if (tool->moving) {
    if (tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display(ddisp, tool->obj, tool->handle, TRUE);
      diagram_update_connections_selection(ddisp->diagram);
      diagram_flush(ddisp->diagram);
    }
    tool->moving = FALSE;
    tool->handle = NULL;
    tool->obj = NULL;
  }

  {
    /* remove position from status bar */
    GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
    guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
    gtk_statusbar_pop (statusbar, context_id);
  }

  highlight_reset_all(ddisp->diagram);
  reset = prefs.reset_tools_after_create != tool->invert_persistence;
  /* kind of backward: first starting editing to see if it is possible at all, than GUI reflection */
  if (textedit_activate_object(ddisp, obj, NULL) && reset) {
    gtk_action_activate (menus_get_action ("ToolsTextedit"));
    reset = FALSE; /* don't switch off textedit below */
  }
  diagram_update_extents(ddisp->diagram);
  diagram_modified(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);

  if (reset)
      tool_reset();
  ddisplay_set_all_cursor(default_cursor);
  ddisplay_do_update_menu_sensitivity(ddisp);
}

static void
create_object_motion(CreateObjectTool *tool, GdkEventMotion *event,
		   DDisplay *ddisp)
{
  Point to;
  ConnectionPoint *connectionpoint = NULL;
  gchar *postext;
  GtkStatusbar *statusbar;
  guint context_id;

  if (!tool->moving)
    return;

  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  /* make sure the new object is restricted to its parent */
  parent_handle_move_out_check(tool->obj, &to);

  /* Move to ConnectionPoint if near: */
  if (tool->handle != NULL &&
      tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
    connectionpoint =
      object_find_connectpoint_display(ddisp, &to, tool->obj, TRUE);

    if (connectionpoint != NULL) {
      to = connectionpoint->pos;
      highlight_object(connectionpoint->object, DIA_HIGHLIGHT_CONNECTIONPOINT, ddisp->diagram);
      ddisplay_set_all_cursor_name (NULL, "crosshair");
    }
  }

  if (connectionpoint == NULL) {
    /* No connectionpoint near, then snap to grid (if enabled) */
    snap_to_grid(ddisp, &to.x, &to.y);
    highlight_reset_all(ddisp->diagram);
    ddisplay_set_all_cursor_name (NULL, "move");
  }

  object_add_updates(tool->obj, ddisp->diagram);
  tool->obj->ops->move_handle(tool->obj, tool->handle, &to, connectionpoint,
			      HANDLE_MOVE_CREATE, 0);
  object_add_updates(tool->obj, ddisp->diagram);

  /* Put current mouse position in status bar */
  statusbar = GTK_STATUSBAR (ddisp->modified_status);
  context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");

  postext = g_strdup_printf("%.3f, %.3f - %.3f, %.3f",
			    tool->obj->bounding_box.left,
			    tool->obj->bounding_box.top,
			    tool->obj->bounding_box.right,
			    tool->obj->bounding_box.bottom);

  gtk_statusbar_pop (statusbar, context_id);
  gtk_statusbar_push (statusbar, context_id, postext);

  g_clear_pointer (&postext, g_free);

  diagram_flush (ddisp->diagram);

  tool->last_to = to;

  return;
}



Tool *
create_create_object_tool(DiaObjectType *objtype, void *user_data,
			  int invert_persistence)
{
  CreateObjectTool *tool;
  GdkPixbuf *pixbuf;
  GdkDisplay *disp;
  GdkCursor *cursor;

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

  pixbuf = pixbuf_from_resource ("/org/gnome/Dia/icons/dia-cursor-create.png");

  disp = gdk_display_get_default ();

  cursor = gdk_cursor_new_from_pixbuf (disp, pixbuf, 0, 0);

  ddisplay_set_all_cursor (cursor);

  return (Tool *) tool;
}


void
free_create_object_tool(Tool *tool)
{
  CreateObjectTool *real_tool = (CreateObjectTool *)tool;

  if (real_tool->moving) { /* should not get here, but see bug #619246 */
    // deprecated gdk_pointer_ungrab (GDK_CURRENT_TIME);
    ddisplay_set_all_cursor (default_cursor);
  }

  g_clear_pointer (&tool, g_free);
}
