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

#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>

#include "modify_tool.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "message.h"
#include "properties-dialog.h"
#include "select.h"
#include "preferences.h"
#include "cursor.h"
#include "highlight.h"
#include "textedit.h"
#include "textline.h"
#include "menus.h"
#include "diainteractiverenderer.h"
#include "dia-layer.h"

#include "parent.h"
#include "prop_text.h"
#include "object.h"

#include "dia-guide-tool.h"

static DiaObject *click_select_object(DDisplay *ddisp, Point *clickedpoint,
				   GdkEventButton *event);
static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
				Point *clickedpoint,
				GdkEventButton *event);
static void modify_button_press(ModifyTool *tool, GdkEventButton *event,
				 DDisplay *ddisp);
static void modify_button_hold(ModifyTool *tool, GdkEventButton *event,
				 DDisplay *ddisp);
static void modify_button_release(ModifyTool *tool, GdkEventButton *event,
				  DDisplay *ddisp);
static void modify_motion(ModifyTool *tool, GdkEventMotion *event,
			  DDisplay *ddisp);
static void modify_double_click(ModifyTool *tool, GdkEventButton *event,
				DDisplay *ddisp);

struct _ModifyTool {
  Tool tool;

  enum ModifyToolState state;
  int break_connections;
  Point move_compensate;
  DiaObject *object;
  Handle *handle;
  Point last_to;
  Point start_at;
  time_t start_time;
  int x1, y1, x2, y2;
  Point start_box;
  Point end_box;

  gboolean auto_scrolled; /* TRUE if the diagram auto scrolled last time
                             modify_motion was called */
  /* Undo info: */
  Point *orig_pos;

  /* Guide info: */
  DiaGuide *guide;
};


Tool *
create_modify_tool(void)
{
  ModifyTool *tool;

  tool = g_new0(ModifyTool, 1);
  tool->tool.type = MODIFY_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &modify_button_press;
  tool->tool.button_hold_func = (ButtonHoldFunc) &modify_button_hold;
  tool->tool.button_release_func = (ButtonReleaseFunc) &modify_button_release;
  tool->tool.motion_func = (MotionFunc) &modify_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &modify_double_click;
  tool->state = STATE_NONE;
  tool->break_connections = FALSE;
  tool->auto_scrolled = FALSE;

  tool->orig_pos = NULL;

  return (Tool *)tool;
}

static ModifierKeys
gdk_event_to_dia_ModifierKeys(guint event_state)
{
  ModifierKeys mod = MODIFIER_NONE;
#if defined GDK_WINDOWING_QUARTZ
  static guint last_state = 0;

  if (last_state != event_state) {
    last_state = event_state;
    g_printerr ("%s%s%s,M%s%s%s%s%s,B%s%s%s%s%s,%s%s%s\n",
	     event_state & GDK_SHIFT_MASK ? "Sh" : "  ",
	     event_state & GDK_LOCK_MASK ? "Lo" : "  ",
	     event_state & GDK_CONTROL_MASK ? "Co" : "  ",
	     event_state & GDK_MOD1_MASK ? "1" : " ",
	     event_state & GDK_MOD2_MASK ? "2" : " ",
	     event_state & GDK_MOD3_MASK ? "3" : " ",
	     event_state & GDK_MOD4_MASK ? "4" : " ",
	     event_state & GDK_MOD5_MASK ? "5" : " ",
	     event_state & GDK_BUTTON1_MASK ? "1" : " ",
	     event_state & GDK_BUTTON2_MASK ? "2" : " ",
	     event_state & GDK_BUTTON3_MASK ? "3" : " ",
	     event_state & GDK_BUTTON4_MASK ? "4" : " ",
	     event_state & GDK_BUTTON5_MASK ? "5" : " ",
	     event_state & GDK_SUPER_MASK ? "Su" : "  ",
	     event_state & GDK_HYPER_MASK ? "Hy" : "  ",
	     event_state & GDK_META_MASK ? "Me" : "  ");
  }
  /* Kludge against GTK/Quartz returning bogus values,
   * see https://bugzilla.gnome.org/show_bug.cgi?id=722815
   */
  if (event_state == (GDK_CONTROL_MASK|GDK_MOD1_MASK|GDK_MOD2_MASK |GDK_MOD3_MASK|GDK_BUTTON1_MASK))
    event_state = GDK_BUTTON1_MASK;
#endif

  if (event_state & GDK_SHIFT_MASK)
    mod |= MODIFIER_SHIFT;
  if (event_state & GDK_CONTROL_MASK)
    mod |= MODIFIER_CONTROL;
  if (event_state & GDK_MOD1_MASK)
    mod |= MODIFIER_ALT;

  return mod;
}


void
free_modify_tool (Tool *tool)
{
  ModifyTool *mtool = (ModifyTool *)tool;
  g_clear_pointer (&mtool, g_free);
}


static DiaObject *
click_select_object(DDisplay *ddisp, Point *clickedpoint,
		    GdkEventButton *event)
{
  Diagram *diagram;
  real click_distance;
  DiaObject *obj;

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

      diagram_select(diagram, obj);
      /* To be removed once text edit mode is stable.  By then,
       * we don't want to automatically edit selected objects.
	 textedit_activate_object(ddisp, obj, clickedpoint);
      */

      ddisplay_do_update_menu_sensitivity(ddisp);
      object_add_updates_list(diagram->data->selected, diagram);
      diagram_flush(diagram);

      return obj;
    } else { /* Clicked on already selected. */
      /*printf("Already selected\n");*/
      /* To be removed once text edit mode is stable.  By then,
       * we don't want to automatically edit selected objects.
      textedit_activate_object(ddisp, obj, clickedpoint);
      */
      object_add_updates_list(diagram->data->selected, diagram);
      diagram_flush(diagram);

      if (event->state & GDK_SHIFT_MASK) { /* Multi-select */
	/* Remove the selected selected */
	ddisplay_do_update_menu_sensitivity(ddisp);
	diagram_unselect_object(diagram, (DiaObject *)already->data);
	diagram_flush(ddisp->diagram);
      } else {
	return obj;
      }
    }
  } /* Else part moved to allow union/intersection select */

  return NULL;
}


static glong
time_micro (void)
{
  return g_get_real_time ();
}


static int do_if_clicked_handle(DDisplay *ddisp, ModifyTool *tool,
				Point *clickedpoint, GdkEventButton *event)
{
  DiaObject *obj;
  Handle *handle;

  handle = NULL;
  diagram_find_closest_handle(ddisp->diagram, &handle, &obj, clickedpoint);
  if  (handle_is_clicked(ddisp, handle, clickedpoint)) {
    tool->state = STATE_MOVE_HANDLE;
    tool->break_connections = TRUE;
    tool->last_to = handle->pos;
    tool->handle = handle;
    tool->object = obj;
    gdk_device_grab (gdk_event_get_device ((GdkEvent*)event),
                     gtk_widget_get_window(ddisp->canvas),
                     GDK_OWNERSHIP_APPLICATION,
                     FALSE,
                     GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                     NULL, event->time);
    tool->start_at = handle->pos;
    tool->start_time = time_micro();
    ddisplay_set_all_cursor_name (NULL, "move");
    return TRUE;
  }
  return FALSE;
}


#define  FUNSCALEX(s,x)   ((x) / (s)->zoom_factor)
#define  FUNSCALEY(s,y)   ((y) / (s)->zoom_factor)


static void
modify_button_press (ModifyTool     *tool,
                     GdkEventButton *event,
                     DDisplay       *ddisp)
{
  Point clickedpoint;
  DiaObject *clicked_obj;
  gboolean some_selected;
  DiaGuide *guide;
  const int pick_guide_snap_distance = 20;	/* Margin of error for selecting a guide. */

  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  tool->guide = NULL;

  /* don't got to single handle movement if there is more than one object selected */
  some_selected = g_list_length (ddisp->diagram->data->selected) > 1;
  if (!some_selected && do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
    return;

  clicked_obj = click_select_object(ddisp, &clickedpoint, event);
  if (!some_selected && do_if_clicked_handle(ddisp, tool, &clickedpoint, event))
    return;

  if ( clicked_obj != NULL ) {
    tool->state = STATE_MOVE_OBJECT;
    tool->object = clicked_obj;
    tool->move_compensate = clicked_obj->position;
    point_sub(&tool->move_compensate, &clickedpoint);
    tool->break_connections = TRUE; /* unconnect when not grabbing handles, just setting to
				      * FALSE is not enough. Need to refine the move op, too. */
    gdk_device_grab (gdk_event_get_device ((GdkEvent*)event),
                     gtk_widget_get_window(ddisp->canvas),
                     GDK_OWNERSHIP_APPLICATION,
                     FALSE,
                     GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                     NULL, event->time);
    tool->start_at = clickedpoint;
    tool->start_time = time_micro();
    ddisplay_set_all_cursor_name (NULL, "move");
  } else {
    /* If there is a guide nearby, then drag it.
     * Note: We can only drag guides if they are visible (like in GIMP). */
    if (ddisp->guides_visible) {
      guide = dia_diagram_pick_guide (ddisp->diagram, clickedpoint.x, clickedpoint.y,
      FUNSCALEX (ddisp, pick_guide_snap_distance ),
      FUNSCALEY (ddisp, pick_guide_snap_distance ));

      if (guide) {
        tool->guide = guide;
        guide_tool_start_edit (ddisp, guide);
        return;
      }
    }

    /* Box select. */
    tool->state = STATE_BOX_SELECT;
    tool->start_box = clickedpoint;
    tool->end_box = clickedpoint;
    tool->x1 = tool->x2 = (int) event->x;
    tool->y1 = tool->y2 = (int) event->y;

    dia_interactive_renderer_set_selection (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                            TRUE,
                                            tool->x1,
                                            tool->y1,
                                            tool->x2 - tool->x1,
                                            tool->y2 - tool->y1);
    ddisplay_flush (ddisp);

    gdk_device_grab (gdk_event_get_device ((GdkEvent*)event),
                     gtk_widget_get_window(ddisp->canvas),
                     GDK_OWNERSHIP_APPLICATION,
                     FALSE,
                     GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                     NULL, event->time);
  }
}


static void
modify_button_hold(ModifyTool *tool, GdkEventButton *event,
		DDisplay *ddisp)
{
  Point clickedpoint;

  switch (tool->state) {
  case STATE_MOVE_OBJECT:
    /* A button hold is as if user was moving object - if it is
     * a text object and can be edited, then the move is cancelled */
    ddisplay_untransform_coords(ddisp,
				(int)event->x, (int)event->y,
				&clickedpoint.x, &clickedpoint.y);

    if (tool->object != NULL &&
	diagram_is_selected(ddisp->diagram, tool->object)) {
      if (textedit_activate_object(ddisp, tool->object, &clickedpoint)) {
	/* Return tool to normal state - object is text and is in edit */
	gdk_device_ungrab (gdk_event_get_device((GdkEvent*)event), event->time);
	tool->orig_pos = NULL;
	tool->state = STATE_NONE;
	/* Activate Text Edit */
	gtk_action_activate (menus_get_action ("ToolsTextedit"));
      }
    }
    break;
  case STATE_MOVE_HANDLE:
    break;
  case STATE_BOX_SELECT:
    break;
  case STATE_NONE:
    break;
  default:
    message_error("Internal error: Strange state in modify_tool (button_hold)\n");
  }
}

static void
modify_double_click(ModifyTool *tool, GdkEventButton *event,
		    DDisplay *ddisp)
{
  Point clickedpoint;
  DiaObject *clicked_obj;

  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  clicked_obj = click_select_object(ddisp, &clickedpoint, event);

  if ( clicked_obj != NULL ) {
    object_list_properties_show(ddisp->diagram, ddisp->diagram->data->selected);
  } else { /* No object selected */
    /*printf("didn't select object\n");*/
    if (!(event->state & GDK_SHIFT_MASK)) {
      /* Not Multi-select => Remove all selected */
      ddisplay_do_update_menu_sensitivity(ddisp);
      diagram_remove_all_selected(ddisp->diagram, TRUE);
      diagram_flush(ddisp->diagram);
    }
  }
}

#define MIN_PIXELS 10

/*
 * Makes sure that objects aren't accidentally moved when double-clicking
 * for properties.  Objects do not move unless double click time has passed
 * or the move is 'significant'.  Allowing the 'significant' move makes a
 * regular grab-and-move less jerky.
 *
 * There's a slight chance that the user could move outside the
 * minimum movement limit and back in within the double click time
 * (normally .25 seconds), but that's very, very unlikely.  If
 * it happens, the cursor wouldn't reset just right.
 */

static gboolean
modify_move_already(ModifyTool *tool, DDisplay *ddisp, Point *to)
{
  static gboolean settings_taken = FALSE;
  static int double_click_time = 250;
  real dist;

  if (!settings_taken) {
    /* One could argue that if the settings were updated while running,
       we should re-read them.  But I don't see any way to get notified,
       and I don't want to do this whole thing for each bit of the
       move --Lars */
    GtkSettings *settings = gtk_settings_get_default();
    if (settings == NULL) {
      g_message(_("Couldn't get GTK+ settings"));
    } else {
      g_object_get(G_OBJECT(settings),
		   "gtk-double-click-time", &double_click_time, NULL);
    }
    settings_taken = TRUE;
  }
  if (tool->start_time < time_micro()-double_click_time*1000) {
    return TRUE;
  }
  dist = distance_point_point_manhattan(&tool->start_at, to);
  if (ddisp->grid.snap) {
    real grid_x = ddisp->diagram->grid.width_x;
    real grid_y = ddisp->diagram->grid.width_y;
    if (dist > grid_x || dist > grid_y) {
      return TRUE;
    }
  }
  if (ddisplay_transform_length(ddisp, dist) > MIN_PIXELS) {
    return (ddisplay_transform_length(ddisp, dist) > MIN_PIXELS);
  } else {
    return FALSE;
  }
}

static void
modify_motion (ModifyTool     *tool,
               GdkEventMotion *event,
               DDisplay       *ddisp)
{
  Point to;
  Point now, delta, full_delta;
  gboolean auto_scroll, vertical = FALSE;
  ConnectionPoint *connectionpoint = NULL;
  DiaObjectChange *objchange = NULL;

  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  if (tool->state==STATE_NONE) {
    DiaObject *obj = NULL;
    Handle *handle = NULL;

    diagram_find_closest_handle (ddisp->diagram, &handle, &obj, &to);
    if  (handle && handle->type != HANDLE_NON_MOVABLE
      && handle->id >= HANDLE_RESIZE_NW && handle->id <= HANDLE_RESIZE_SE
      && handle_is_clicked(ddisp, handle, &to)
      && g_list_length (ddisp->diagram->data->selected) == 1)
      ddisplay_set_all_cursor (get_direction_cursor (CURSOR_DIRECTION_0 + handle->id));
    else
      ddisplay_set_all_cursor_name (NULL, "default");
    return; /* Fast path... */
  }
  auto_scroll = ddisplay_autoscroll(ddisp, event->x, event->y);

  if (!modify_move_already(tool, ddisp, &to)) return;

  switch (tool->state) {
  case STATE_MOVE_OBJECT:

    if (tool->orig_pos == NULL) {
      GList *list, *pla;
      int i;
      DiaObject *obj;

      /* consider non-selected children affected */
      pla = list = parent_list_affected(ddisp->diagram->data->selected);
      tool->orig_pos = g_new(Point, g_list_length(list));
      i=0;
      while (list != NULL) {
        obj = (DiaObject *)  list->data;
        tool->orig_pos[i] = obj->position;
        list = g_list_next(list); i++;
      }
      g_list_free (pla);
    }

    if (tool->break_connections)
      diagram_unconnect_selected(ddisp->diagram); /* Pushes UNDO info */

    if (gdk_event_to_dia_ModifierKeys (event->state) & MODIFIER_CONTROL) {
      full_delta = to;
      point_sub(&full_delta, &tool->start_at);
      vertical = (fabs(full_delta.x) < fabs(full_delta.y));
    }

    point_add(&to, &tool->move_compensate);
    snap_to_grid(ddisp, &to.x, &to.y);

    now = tool->object->position;

    delta = to;
    point_sub(&delta, &now);

    if (gdk_event_to_dia_ModifierKeys (event->state) & MODIFIER_CONTROL) {
      /* Up-down or left-right */
      if (vertical) {
       delta.x = tool->start_at.x + tool->move_compensate.x - now.x;
      } else {
       delta.y = tool->start_at.y + tool->move_compensate.y - now.y;
      }
    }

    object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);
    objchange = object_list_move_delta(ddisp->diagram->data->selected, &delta);
    if (objchange != NULL) {
      dia_object_change_change_new (ddisp->diagram, tool->object, objchange);
    }
    object_add_updates_list(ddisp->diagram->data->selected, ddisp->diagram);

    object_add_updates(tool->object, ddisp->diagram);

    /* Put current mouse position in status bar */
    {
      gchar *postext;
      GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
      guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
      gtk_statusbar_pop (statusbar, context_id);
      postext = g_strdup_printf("%.3f, %.3f - %.3f, %.3f",
			        tool->object->bounding_box.left,
			        tool->object->bounding_box.top,
			        tool->object->bounding_box.right,
			        tool->object->bounding_box.bottom);

      gtk_statusbar_pop (statusbar, context_id);
      gtk_statusbar_push (statusbar, context_id, postext);

      g_clear_pointer (&postext, g_free);
    }

    diagram_update_connections_selection(ddisp->diagram);
    diagram_flush(ddisp->diagram);
    break;
  case STATE_MOVE_HANDLE:
    full_delta = to;
    point_sub(&full_delta, &tool->start_at);
    /* make sure resizing is restricted to its parent */

    /* if resize was blocked by parent, that means the resizing was
      outward, thus it won't bother the children so we don't have to
      check the children */
    if (!parent_handle_move_out_check(tool->object, &to))
      parent_handle_move_in_check(tool->object, &to, &tool->start_at);

    if (gdk_event_to_dia_ModifierKeys (event->state) & MODIFIER_CONTROL)
      vertical = (fabs(full_delta.x) < fabs(full_delta.y));

    highlight_reset_all(ddisp->diagram);
    if ((tool->handle->connect_type != HANDLE_NONCONNECTABLE)) {
      /* Move to ConnectionPoint if near: */
      connectionpoint = object_find_connectpoint_display (ddisp,
                                                          &to,
                                                          tool->object, TRUE);
      if (connectionpoint != NULL) {
        DiaHighlightType type;
        to = connectionpoint->pos;
        if (connectionpoint->flags & CP_FLAGS_MAIN) {
          type = DIA_HIGHLIGHT_CONNECTIONPOINT_MAIN;
        } else {
          type = DIA_HIGHLIGHT_CONNECTIONPOINT;
        }
        highlight_object(connectionpoint->object, type, ddisp->diagram);
        ddisplay_set_all_cursor_name (NULL, "crosshair");
      }
    }
    if (connectionpoint == NULL) {
      /* No connectionpoint near, then snap to grid (if enabled) */
      snap_to_grid(ddisp, &to.x, &to.y);
      ddisplay_set_all_cursor_name (NULL, "move");
    }

    if (tool->break_connections) {
      /* break connections to the handle currently selected. */
      if (tool->handle->connected_to!=NULL) {
        DiaChange *change = dia_unconnect_change_new (ddisp->diagram,
                                                      tool->object,
                                                      tool->handle);

        dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));
      }
    }

    if (gdk_event_to_dia_ModifierKeys (event->state) & MODIFIER_CONTROL) {
      /* Up-down or left-right */
      if (vertical) {
       to.x = tool->start_at.x;
      } else {
       to.y = tool->start_at.y;
      }
    }

    if (tool->orig_pos == NULL) {
      tool->orig_pos = g_new(Point, 1);
      *tool->orig_pos = tool->handle->pos;
    }

    /* Put current mouse position in status bar */
    {
      gchar *postext;
      GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
      guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");

      if (tool->object) { /* play safe */
        real w = tool->object->bounding_box.right - tool->object->bounding_box.left;
        real h = tool->object->bounding_box.bottom - tool->object->bounding_box.top;
        postext = g_strdup_printf("%.3f, %.3f (%.3fx%.3f)", to.x, to.y, w, h);
      } else {
        postext = g_strdup_printf("%.3f, %.3f", to.x, to.y);
      }

      gtk_statusbar_pop (statusbar, context_id);
      gtk_statusbar_push (statusbar, context_id, postext);

      g_clear_pointer (&postext, g_free);
    }

    object_add_updates (tool->object, ddisp->diagram);

    /* Handle undo */
    if (tool->object) {
      objchange = dia_object_move_handle (tool->object,
                                          tool->handle,
                                          &to,
                                          connectionpoint,
                                          HANDLE_MOVE_USER,
                                          gdk_event_to_dia_ModifierKeys (event->state));
    }

    if (objchange != NULL) {
      dia_object_change_change_new (ddisp->diagram, tool->object, objchange);
    }
    object_add_updates(tool->object, ddisp->diagram);

    diagram_update_connections_selection(ddisp->diagram);
    diagram_flush(ddisp->diagram);
    break;
  case STATE_BOX_SELECT:
    tool->end_box = to;

    ddisplay_transform_coords (ddisp,
                               MIN (tool->start_box.x, tool->end_box.x),
                               MIN (tool->start_box.y, tool->end_box.y),
                               &tool->x1, &tool->y1);
    ddisplay_transform_coords (ddisp,
                               MAX (tool->start_box.x, tool->end_box.x),
                               MAX (tool->start_box.y, tool->end_box.y),
                               &tool->x2, &tool->y2);

    dia_interactive_renderer_set_selection (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                            TRUE,
                                            tool->x1,
                                            tool->y1,
                                            tool->x2 - tool->x1,
                                            tool->y2 - tool->y1);
    ddisplay_flush (ddisp);

    break;
  case STATE_NONE:
    break;
  default:
    message_error("Internal error: Strange state in modify_tool\n");
  }

  tool->last_to = to;
  tool->auto_scrolled = auto_scroll;
}

/** Find the list of objects selected by current rubberbanding.
 * The list should be freed after use. */
static GList *
find_selected_objects(DDisplay *ddisp, ModifyTool *tool)
{
  DiaRectangle r;
  r.left = MIN(tool->start_box.x, tool->end_box.x);
  r.right = MAX(tool->start_box.x, tool->end_box.x);
  r.top = MIN(tool->start_box.y, tool->end_box.y);
  r.bottom = MAX(tool->start_box.y, tool->end_box.y);

  if (prefs.reverse_rubberbanding_intersects &&
      tool->start_box.x > tool->end_box.x) {
    return
      dia_layer_find_objects_intersecting_rectangle (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (ddisp->diagram)), &r);
  } else {
    return
      dia_layer_find_objects_in_rectangle (dia_diagram_data_get_active_layer (DIA_DIAGRAM_DATA (ddisp->diagram)), &r);
  }
}


static void
modify_button_release (ModifyTool     *tool,
                       GdkEventButton *event,
                       DDisplay       *ddisp)
{
  Point *dest_pos, to;
  GList *list;
  int i;
  DiaObjectChange *objchange;

  tool->break_connections = FALSE;
  ddisplay_set_all_cursor(default_cursor);

  /* remove position from status bar */
  {
    GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
    guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
    gtk_statusbar_pop (statusbar, context_id);
  }
  switch (tool->state) {
  case STATE_MOVE_OBJECT:
    /* Return to normal state */
    gdk_device_ungrab (gdk_event_get_device((GdkEvent*)event), event->time);

    ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);
    if (!modify_move_already(tool, ddisp, &to)) {
      tool->orig_pos = NULL;
      tool->state = STATE_NONE;
      return;
    }

    diagram_update_connections_selection(ddisp->diagram);

    if (tool->orig_pos != NULL) {
      /* consider the non-selected children affected */
      list = parent_list_affected(ddisp->diagram->data->selected);
      dest_pos = g_new(Point, g_list_length(list));
      i=0;
      while (list != NULL) {
	DiaObject *obj = (DiaObject *)  list->data;
	dest_pos[i] = obj->position;
	list = g_list_next(list); i++;
      }

      dia_move_objects_change_new (ddisp->diagram, tool->orig_pos, dest_pos,
                                   parent_list_affected (ddisp->diagram->data->selected));
    }

    ddisplay_connect_selected(ddisp); /* pushes UNDO info */
    diagram_update_extents(ddisp->diagram);
    diagram_modified(ddisp->diagram);
    diagram_flush(ddisp->diagram);

    undo_set_transactionpoint(ddisp->diagram->undo);

    tool->orig_pos = NULL;
    tool->state = STATE_NONE;
    break;
  case STATE_MOVE_HANDLE:
    gdk_device_ungrab (gdk_event_get_device((GdkEvent*)event), event->time);
    tool->state = STATE_NONE;

    if (tool->orig_pos != NULL) {
      dia_move_handle_change_new (ddisp->diagram,
                                  tool->handle,
                                  tool->object,
                                  *tool->orig_pos,
                                  tool->last_to,
                                  gdk_event_to_dia_ModifierKeys (event->state));
    }

    /* Final move: */
    object_add_updates(tool->object, ddisp->diagram);
    objchange = dia_object_move_handle (tool->object,
                                        tool->handle,
                                        &tool->last_to,
                                        NULL,
                                        HANDLE_MOVE_USER_FINAL,
                                        gdk_event_to_dia_ModifierKeys (event->state));
    if (objchange != NULL) {
      dia_object_change_change_new (ddisp->diagram, tool->object, objchange);
    }

    object_add_updates(tool->object, ddisp->diagram);

    /* Connect if possible: */
    if (tool->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display(ddisp, tool->object, tool->handle, TRUE); /* pushes UNDO info */
      diagram_update_connections_selection(ddisp->diagram);
    }

    highlight_reset_all(ddisp->diagram);
    diagram_flush(ddisp->diagram);

    diagram_modified(ddisp->diagram);
    diagram_update_extents(ddisp->diagram);

    undo_set_transactionpoint(ddisp->diagram->undo);

    g_clear_pointer (&tool->orig_pos, g_free);

    break;
  case STATE_BOX_SELECT:

    gdk_device_ungrab (gdk_event_get_device((GdkEvent*)event), event->time);
    /* Remove last box: */
    dia_interactive_renderer_set_selection (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                            FALSE, 0, 0, 0, 0);

    {
      GList *selected, *list_to_free;

      selected = list_to_free = find_selected_objects (ddisp, tool);

      if (selection_style == SELECT_REPLACE &&
          !(event->state & GDK_SHIFT_MASK)) {
        /* Not Multi-select => Remove all selected */
        diagram_remove_all_selected (ddisp->diagram, TRUE);
      }

      if (selection_style == SELECT_INTERSECTION) {
        GList *intersection = NULL;

        while (selected != NULL) {
          DiaObject *obj = DIA_OBJECT (selected->data);

          if (diagram_is_selected (ddisp->diagram, obj)) {
            intersection = g_list_append (intersection, obj);
          }

          selected = g_list_next (selected);
        }
        selected = intersection;
        diagram_remove_all_selected (ddisp->diagram, TRUE);
        while (selected != NULL) {
          DiaObject *obj = DIA_OBJECT (selected->data);

          diagram_select (ddisp->diagram, obj);

          selected = g_list_next (selected);
        }
        g_list_free (intersection);
      } else {
        while (selected != NULL) {
          DiaObject *obj = DIA_OBJECT (selected->data);

          if (selection_style == SELECT_REMOVE) {
            if (diagram_is_selected (ddisp->diagram, obj))
              diagram_unselect_object (ddisp->diagram, obj);
          } else if (selection_style == SELECT_INVERT) {
            if (diagram_is_selected (ddisp->diagram, obj))
              diagram_unselect_object (ddisp->diagram, obj);
            else
              diagram_select (ddisp->diagram, obj);
          } else {
            if (!diagram_is_selected (ddisp->diagram, obj))
              diagram_select (ddisp->diagram, obj);
          }

          selected = g_list_next (selected);
        }
      }

      g_list_free (list_to_free);
    }

    ddisplay_do_update_menu_sensitivity (ddisp);
    ddisplay_flush (ddisp);

    tool->state = STATE_NONE;
    break;
  case STATE_NONE:
    break;
  default:
    message_error("Internal error: Strange state in modify_tool\n");

  }
}

