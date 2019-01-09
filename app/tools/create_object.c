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
#include "highlight.h"
#include "textedit.h"
#include "parent.h"
#include "message.h"
#include "object.h"
#include "intl.h"
#include "menus.h"

static void create_button_press   (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);
static void create_button_release (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);
static void create_motion         (DiaTool        *tool,
                                   GdkEventMotion *event,
                                   DiaDisplay     *ddisp);
static void create_double_click   (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);

G_DEFINE_TYPE (DiaCreateTool, dia_create_tool, DIA_TYPE_TOOL)

enum {
  PROP_INVERT_PERSISTENCE = 1,
  N_PROPS
};
static GParamSpec* properties[N_PROPS];

static void
activate (DiaTool *tool)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (tool);

  self->moving = FALSE;

  dia_display_set_all_cursor (get_cursor (CURSOR_CREATE));
}

static void
deactivate (DiaTool *tool)
{
  DiaCreateTool *real_tool = DIA_CREATE_TOOL (tool);

  if (real_tool->moving) { /* should not get here, but see bug #619246 */
    gdk_pointer_ungrab (GDK_CURRENT_TIME);
    dia_display_set_all_cursor (default_cursor);
  }
}

static void
dia_create_tool_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (object);

  switch (property_id) {
    case PROP_INVERT_PERSISTENCE:
      self->invert_persistence = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_create_tool_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (object);

  switch (property_id) {
    case PROP_INVERT_PERSISTENCE:
      g_value_set_boolean (value, self->invert_persistence);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_create_tool_class_init (DiaCreateToolClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  DiaToolClass *tool_class = DIA_TOOL_CLASS (klass);

  object_class->set_property = dia_create_tool_set_property;
  object_class->get_property = dia_create_tool_get_property;

  properties[PROP_INVERT_PERSISTENCE] = g_param_spec_boolean ("invert-persistence",
                                                              NULL, NULL,
                                                              FALSE,
                                                              G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class,
                                     N_PROPS,
                                     properties);

  tool_class->activate = activate;
  tool_class->deactivate = deactivate;

  tool_class->button_press = create_button_press;
  tool_class->button_release = create_button_release;
  tool_class->motion = create_motion;
  tool_class->double_click = create_double_click;
}

static void
dia_create_tool_init (DiaCreateTool *self)
{
}

DiaTool *
dia_create_tool_new (DiaObjectType *objtype,
                     gboolean       invert_persistence,
                     void          *user_data)
{
  DiaCreateTool *self = g_object_new (DIA_TYPE_CREATE_TOOL,
                                      "invert-persistence", invert_persistence);

  /* TODO: As properties */
  self->objtype = objtype;
  self->user_data = user_data;

  return DIA_TOOL (self);
}

static void
create_button_press (DiaTool        *tool,
                     GdkEventButton *event,
                     DiaDisplay     *ddisp)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (tool);
  Point clickedpoint, origpoint;
  Handle *handle1;
  Handle *handle2;
  DiaObject *obj;

  dia_display_untransform_coords (ddisp,
                                  (int)event->x, (int)event->y,
                                  &clickedpoint.x, &clickedpoint.y);

  origpoint = clickedpoint;

  snap_to_grid (ddisp, &clickedpoint.x, &clickedpoint.y);

  obj = dia_object_default_create (self->objtype, &clickedpoint,
                                   self->user_data,
                                   &handle1, &handle2);

  self->obj = obj; /* ensure that tool->obj is initialised in case we
		      return early. */
  if (!obj) {
    self->moving = FALSE;
    self->handle = NULL;
    message_error(_("'%s' creation failed"), self->objtype ? self->objtype->name : "NULL");
    return;
  }

  diagram_add_object (ddisp->diagram, obj);

  /* Try a connect */
  if (handle1 != NULL &&
      handle1->connect_type != HANDLE_NONCONNECTABLE) {
    ConnectionPoint *connectionpoint;
    connectionpoint = object_find_connectpoint_display (ddisp,
                                                        &origpoint, obj, TRUE);
    if (connectionpoint != NULL) {
      (obj->ops->move)(obj, &origpoint);
    }
  }
  
  if (!(event->state & GDK_SHIFT_MASK)) {
    /* Not Multi-select => remove current selection */
    diagram_remove_all_selected (ddisp->diagram, TRUE);
  }
  diagram_select (ddisp->diagram, obj);

  /* Connect first handle if possible: */
  if ((handle1!= NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display (ddisp, obj, handle1, TRUE);
  }

  object_add_updates (obj, ddisp->diagram);
  dia_display_do_update_menu_sensitivity (ddisp);
  diagram_flush (ddisp->diagram);
  
  if (handle2 != NULL) {
    self->handle = handle2;
    self->moving = TRUE;
    self->last_to = handle2->pos;
    
    gdk_pointer_grab (gtk_widget_get_window (ddisp->canvas), FALSE,
		      GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		      NULL, NULL, event->time);
    dia_display_set_all_cursor (get_cursor (CURSOR_SCROLL));
  } else {
    diagram_update_extents (ddisp->diagram);
    self->moving = FALSE;
  }

}

static void
create_double_click (DiaTool        *tool,
                     GdkEventButton *event,
                     DiaDisplay     *ddisp)
{
}

static void
create_button_release (DiaTool        *tool,
                       GdkEventButton *event,
                       DiaDisplay     *ddisp)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (tool);
  GList *list = NULL;
  DiaObject *obj = self->obj;
  gboolean reset;

  GList *parent_candidates;

  g_return_if_fail (obj != NULL);
  if (!obj) /* not sure if this isn't enough */
    return; /* could be a legal invariant */

  if (self->moving) {
    gdk_pointer_ungrab (event->time);

    object_add_updates (self->obj, ddisp->diagram);
    self->obj->ops->move_handle (self->obj,
                                 self->handle,
                                 &self->last_to,
                                 NULL,
                                 HANDLE_MOVE_CREATE_FINAL,
                                 0);
    object_add_updates (self->obj, ddisp->diagram);
  }

  parent_candidates = layer_find_objects_containing_rectangle (obj->parent_layer,
                                                               &obj->bounding_box);

  /* whole object must be within another object to parent it */
  for (; parent_candidates != NULL; parent_candidates = g_list_next(parent_candidates)) {
    DiaObject *parent_obj = (DiaObject *) parent_candidates->data;
    if (obj != parent_obj && object_within_parent (obj, parent_obj)) {
      Change *change = undo_parenting (ddisp->diagram, parent_obj, obj, TRUE);
      (change->apply)(change, ddisp->diagram);
      break;
    /*
    obj->parent = parent_obj;
    parent_obj->children = g_list_append(parent_obj->children, obj);
    */
    }
  }
  g_list_free(parent_candidates);

  list = g_list_prepend (list, self->obj);

  undo_insert_objects (ddisp->diagram, list, 1); 

  if (self->moving) {
    if (self->handle->connect_type != HANDLE_NONCONNECTABLE) {
      object_connect_display (ddisp, self->obj, self->handle, TRUE);
      diagram_update_connections_selection (ddisp->diagram);
      diagram_flush (ddisp->diagram);
    }
    self->moving = FALSE;
    self->handle = NULL;
    self->obj = NULL;
  }
  
  {
    /* remove position from status bar */
    GtkStatusbar *statusbar = GTK_STATUSBAR (ddisp->modified_status);
    guint context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
    gtk_statusbar_pop (statusbar, context_id);
  }
  
  highlight_reset_all (ddisp->diagram);
  reset = prefs.reset_tools_after_create != self->invert_persistence;
  /* kind of backward: first starting editing to see if it is possible at all, than GUI reflection */
  if (textedit_activate_object (ddisp, obj, NULL) && reset) {
    gtk_action_activate (menus_get_action ("ToolsTextedit"));
    reset = FALSE; /* don't switch off textedit below */
  }
  diagram_update_extents (ddisp->diagram);
  diagram_modified (ddisp->diagram);

  undo_set_transactionpoint (ddisp->diagram->undo);
  
  if (reset)
    tool_reset();
  dia_display_set_all_cursor (default_cursor);
  dia_display_do_update_menu_sensitivity (ddisp);
}

static void
create_motion (DiaTool        *tool,
               GdkEventMotion *event,
               DiaDisplay     *ddisp)
{
  DiaCreateTool *self = DIA_CREATE_TOOL (tool);
  Point to;
  ConnectionPoint *connectionpoint = NULL;
  gchar *postext;
  GtkStatusbar *statusbar;
  guint context_id;

  if (!self->moving)
    return;
  
  dia_display_untransform_coords (ddisp, event->x, event->y, &to.x, &to.y);

  /* make sure the new object is restricted to its parent */
  parent_handle_move_out_check(self->obj, &to);

  /* Move to ConnectionPoint if near: */
  if (self->handle != NULL &&
      self->handle->connect_type != HANDLE_NONCONNECTABLE) {
    connectionpoint = object_find_connectpoint_display (ddisp,
                                                        &to, self->obj, TRUE);
    
    if (connectionpoint != NULL) {
      to = connectionpoint->pos;
      highlight_object(connectionpoint->object, DIA_HIGHLIGHT_CONNECTIONPOINT, ddisp->diagram);
      dia_display_set_all_cursor(get_cursor(CURSOR_CONNECT));
    }
  }
  
  if (connectionpoint == NULL) {
    /* No connectionopoint near, then snap to grid (if enabled) */
    snap_to_grid (ddisp, &to.x, &to.y);
    highlight_reset_all (ddisp->diagram);
    dia_display_set_all_cursor (get_cursor (CURSOR_SCROLL));
  }
      
  object_add_updates (self->obj, ddisp->diagram);
  self->obj->ops->move_handle (self->obj, self->handle, &to, connectionpoint,
                               HANDLE_MOVE_CREATE, 0);
  object_add_updates (self->obj, ddisp->diagram);

  /* Put current mouse position in status bar */
  statusbar = GTK_STATUSBAR (ddisp->modified_status);
  context_id = gtk_statusbar_get_context_id (statusbar, "ObjectPos");
    
  postext = g_strdup_printf ("%.3f, %.3f - %.3f, %.3f",
                             self->obj->bounding_box.left,
                             self->obj->bounding_box.top,
                             self->obj->bounding_box.right,
                             self->obj->bounding_box.bottom);

  gtk_statusbar_pop (statusbar, context_id); 
  gtk_statusbar_push (statusbar, context_id, postext);

  g_free(postext);
  
  diagram_flush (ddisp->diagram);

  self->last_to = to;
  
  return;
}
