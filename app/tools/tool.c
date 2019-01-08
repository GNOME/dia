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

#include "tool.h"
#include "create_object.h"
#include "magnify.h"
#include "modify_tool.h"
#include "scroll_tool.h"
#include "textedit_tool.h"
#include "interface.h"
#include "defaults.h"
#include "object.h"

DiaTool *active_tool = NULL;
DiaTool *transient_tool = NULL;
static GtkWidget *active_button = NULL;
static GtkWidget *former_button = NULL;


G_DEFINE_TYPE (DiaTool, dia_tool, G_TYPE_OBJECT)

enum {
  ACTIVATE,
  DEACTIVATE,
  BUTTON_PRESS,
  BUTTON_HOLD,
  DOUBLE_CLICK,
  BUTTON_RELEASE,
  MOTION,
  LAST_SIGNAL
};
static guint signals[LAST_SIGNAL] = { 0 };

static void
dia_tool_class_init (DiaToolClass *klass)
{
  klass->activate = NULL;
  klass->deactivate = NULL;
  klass->button_press = NULL;
  klass->button_hold = NULL;
  klass->double_click = NULL;
  klass->button_release = NULL;
  klass->motion = NULL;

  signals[ACTIVATE] = g_signal_new ("activate",
                                    G_TYPE_FROM_CLASS (klass),
                                    G_SIGNAL_RUN_FIRST,
                                    G_STRUCT_OFFSET (DiaToolClass, activate),
                                    NULL, NULL, NULL,
                                    G_TYPE_NONE, 0);

  signals[DEACTIVATE] = g_signal_new ("deactivate",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET (DiaToolClass, activate),
                                        NULL, NULL, NULL,
                                        G_TYPE_NONE, 0);

  signals[BUTTON_PRESS] = g_signal_new ("button-press",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET (DiaToolClass, button_press),
                                        NULL, NULL, NULL,
                                        G_TYPE_NONE, 2,
                                        GDK_TYPE_EVENT,
                                        DIA_TYPE_DISPLAY);

  signals[BUTTON_HOLD] = g_signal_new ("button-hold",
                                       G_TYPE_FROM_CLASS (klass),
                                       G_SIGNAL_RUN_FIRST,
                                       G_STRUCT_OFFSET (DiaToolClass, button_hold),
                                       NULL, NULL, NULL,
                                       G_TYPE_NONE, 2,
                                       GDK_TYPE_EVENT,
                                       DIA_TYPE_DISPLAY);

  signals[DOUBLE_CLICK] = g_signal_new ("double-click",
                                        G_TYPE_FROM_CLASS (klass),
                                        G_SIGNAL_RUN_FIRST,
                                        G_STRUCT_OFFSET (DiaToolClass, double_click),
                                        NULL, NULL, NULL,
                                        G_TYPE_NONE, 2,
                                        GDK_TYPE_EVENT,
                                        DIA_TYPE_DISPLAY);

  signals[BUTTON_RELEASE] = g_signal_new ("button-release",
                                          G_TYPE_FROM_CLASS (klass),
                                          G_SIGNAL_RUN_FIRST,
                                          G_STRUCT_OFFSET (DiaToolClass, button_release),
                                          NULL, NULL, NULL,
                                          G_TYPE_NONE, 2,
                                          GDK_TYPE_EVENT,
                                          DIA_TYPE_DISPLAY);

  signals[MOTION] = g_signal_new ("motion",
                                  G_TYPE_FROM_CLASS (klass),
                                  G_SIGNAL_RUN_FIRST,
                                  G_STRUCT_OFFSET (DiaToolClass, motion),
                                  NULL, NULL, NULL,
                                  G_TYPE_NONE, 2,
                                  GDK_TYPE_EVENT,
                                  DIA_TYPE_DISPLAY);
}

static void
dia_tool_init (DiaTool *self)
{

}

void
dia_tool_activate (DiaTool *self)
{
  g_signal_emit (self, signals[ACTIVATE], 0);
}

void
dia_tool_deactivate (DiaTool *self)
{
  g_signal_emit (self, signals[DEACTIVATE], 0);
}

void
dia_tool_button_press (DiaTool        *self,
                       GdkEventButton *event,
                       DDisplay       *ddisp)
{
  g_signal_emit (self, signals[BUTTON_PRESS], 0, event, display_box_new (ddisp));
}

void
dia_tool_button_hold (DiaTool        *self,
                      GdkEventButton *event,
                      DDisplay       *ddisp)
{
  g_signal_emit (self, signals[BUTTON_HOLD], 0, event, display_box_new (ddisp));

}

void
dia_tool_double_click (DiaTool        *self,
                       GdkEventButton *event,
                       DDisplay       *ddisp)
{
  g_signal_emit (self, signals[DOUBLE_CLICK], 0, event, display_box_new (ddisp));

}

void
dia_tool_button_release (DiaTool        *self,
                         GdkEventButton *event,
                         DDisplay       *ddisp)
{
  g_signal_emit (self, signals[BUTTON_RELEASE], 0, event, display_box_new (ddisp));

}

void
dia_tool_motion (DiaTool        *self,
                 GdkEventMotion *event,
                 DDisplay       *ddisp)
{
  g_signal_emit (self, signals[MOTION], 0, event, display_box_new (ddisp));
}

void 
tool_select_former(void) 
{
  if (former_button) {
    g_signal_emit_by_name(G_OBJECT(former_button), "clicked",
                          GTK_BUTTON(former_button), NULL);
  }
}

void
tool_reset(void)
{
  g_signal_emit_by_name(G_OBJECT(modify_tool_button), "clicked",
			GTK_BUTTON(modify_tool_button), NULL);
}

void
tool_get(ToolState *state)
{
  state->button = active_button;
  if (state->type == DIA_TYPE_CREATE_TOOL) {
    state->user_data = DIA_CREATE_TOOL (active_tool)->user_data;
    state->extra_data = DIA_CREATE_TOOL (active_tool)->objtype->name;
    state->invert_persistence = DIA_CREATE_TOOL (active_tool)->invert_persistence;
  }
  else
  {
    state->user_data = NULL;
    state->extra_data = NULL;
    state->invert_persistence = 0;
  }
}

void
tool_restore(const ToolState *state)
{
  tool_select (state->type, state->extra_data, state->user_data, state->button,
               state->invert_persistence);
}

void 
tool_select (ToolType   type,
             gpointer   extra_data, 
             gpointer   user_data,
             GtkWidget *button,
             gboolean   invert_persistence)
{
  if (button)
    former_button = active_button;

  dia_tool_deactivate (active_tool);
  g_object_unref (active_tool);
  
  switch(type) {
    case MODIFY_TOOL:
      active_tool = g_object_new (DIA_TYPE_MODIFY_TOOL, NULL);
      break;
    case CREATE_OBJECT_TOOL:
      active_tool = dia_create_tool_new (object_get_type ((char *) extra_data),
                                        invert_persistence,
                                        (void *) user_data);
      break;
    case MAGNIFY_TOOL:
      active_tool = g_object_new (DIA_TYPE_MAGNIFY_TOOL, NULL);
      break;
    case SCROLL_TOOL:
      active_tool = g_object_new (DIA_TYPE_SCROLL_TOOL, NULL);
      break;
    case TEXTEDIT_TOOL :
      active_tool = g_object_new (DIA_TYPE_TEXT_EDIT_TOOL, NULL);
      break;
    default:
      g_assert_not_reached();
  }

  dia_tool_activate (active_tool);

  if (button)
    active_button = button;
}

void
tool_options_dialog_show (GType      type,
                          gpointer   extra_data, 
                          gpointer   user_data,
                          GtkWidget *button,
                          gboolean   invert_persistence) 
{
  DiaObjectType *objtype;

  if (!G_TYPE_CHECK_INSTANCE_TYPE (active_tool, type)) 
    tool_select (type, extra_data, user_data, button, invert_persistence);

  if (DIA_IS_CREATE_TOOL (active_tool)) {
    objtype = object_get_type ((char *) extra_data);
    defaults_show (objtype, user_data);
  }
}
