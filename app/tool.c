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

#include "tool.h"
#include "create_object.h"
#include "magnify.h"
#include "modify_tool.h"
#include "scroll_tool.h"
#include "textedit_tool.h"
#include "interface.h"
#include "defaults.h"
#include "object.h"
#include "dia-guide-tool.h"

Tool *active_tool = NULL;
Tool *transient_tool = NULL;
static GtkWidget *active_button = NULL;
static GtkWidget *former_button = NULL;

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
  state->type = active_tool->type;
  state->button = active_button;
  if (state->type == CREATE_OBJECT_TOOL) {
    state->user_data = ((CreateObjectTool *)active_tool)->user_data;
    state->extra_data = ((CreateObjectTool *)active_tool)->objtype->name;
    state->invert_persistence = ((CreateObjectTool *)active_tool)->invert_persistence;
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
  tool_select(state->type, state->extra_data, state->user_data, state->button,
              state->invert_persistence);
}

void
tool_free(Tool *tool)
{
  switch(tool->type) {
  case MODIFY_TOOL:
    free_modify_tool(tool);
    break;
  case CREATE_OBJECT_TOOL:
    free_create_object_tool(tool);
    break;
  case MAGNIFY_TOOL:
    free_magnify_tool(tool);
    break;
  case SCROLL_TOOL:
    free_scroll_tool(tool);
    break;
  case TEXTEDIT_TOOL :
    free_textedit_tool(tool);
    break;
  case GUIDE_TOOL:
    free_guide_tool(tool);
    break;
  default:
    g_assert(0);
  }
}

void
tool_select(ToolType type, gpointer extra_data,
            gpointer user_data, GtkWidget *button,
            int invert_persistence)
{
  if (button)
    former_button = active_button;

  tool_free(active_tool);
  switch(type) {
  case MODIFY_TOOL:
    active_tool = create_modify_tool();
    break;
  case CREATE_OBJECT_TOOL:
    active_tool =
      create_create_object_tool(object_get_type((char *)extra_data),
				(void *) user_data, invert_persistence);
    break;
  case MAGNIFY_TOOL:
    active_tool = create_magnify_tool();
    break;
  case SCROLL_TOOL:
    active_tool = create_scroll_tool();
    break;
  case TEXTEDIT_TOOL :
    active_tool = create_textedit_tool();
    break;
  case GUIDE_TOOL :
    active_tool = create_guide_tool ();
    guide_tool_set_guide (active_tool, extra_data);
    guide_tool_set_orientation (active_tool, GPOINTER_TO_INT(user_data));
    break;
  default:
    g_assert(0);
  }
  if (button)
    active_button = button;
}


void
tool_options_dialog_show (ToolType   type,
                          gpointer   extra_data,
                          gpointer   user_data,
                          GtkWidget *button,
                          int        invert_persistence)
{
  DiaObjectType *objtype;

  if (active_tool->type != type) {
    tool_select (type, extra_data, user_data, button, invert_persistence);
  }

  switch (type) {
    case CREATE_OBJECT_TOOL:
      objtype = object_get_type ((char *) extra_data);
      defaults_show (objtype, user_data);
      break;
    case MODIFY_TOOL:
    case MAGNIFY_TOOL:
    case SCROLL_TOOL:
    case TEXTEDIT_TOOL:
    case GUIDE_TOOL:
    default:
      break;
  }
}
