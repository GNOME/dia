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
#include "interface.h"
#include "defaults.h"

Tool *active_tool = NULL;

void 
tool_select(ToolType type, gpointer extra_data, gpointer user_data)
{
  switch(active_tool->type) {
  case MODIFY_TOOL:
    free_modify_tool(active_tool);
    break;
  case CREATE_OBJECT_TOOL:
    free_create_object_tool(active_tool);
    break;
  case MAGNIFY_TOOL:
    free_magnify_tool(active_tool);
    break;
  case SCROLL_TOOL:
    free_scroll_tool(active_tool);
    break;
  }
  switch(type) {
  case MODIFY_TOOL:
    active_tool = create_modify_tool();
    break;
  case CREATE_OBJECT_TOOL:
    active_tool =
      create_create_object_tool(object_get_type((char *)extra_data),
				(void *) user_data);
    break;
  case MAGNIFY_TOOL:
    active_tool = create_magnify_tool();
    break;
  case SCROLL_TOOL:
    active_tool = create_scroll_tool();
    break;
  }
}

void
tool_options_dialog_show(ToolType type, gpointer extra_data, 
			 gpointer user_data) 
{
  ObjectType *objtype;
 
  if (active_tool->type != type) {
    switch(active_tool->type) {
    case MODIFY_TOOL:
      free_modify_tool(active_tool);
      break;
    case CREATE_OBJECT_TOOL:
      free_create_object_tool(active_tool);
      break;
    case MAGNIFY_TOOL:
      free_magnify_tool(active_tool);
      break;
    case SCROLL_TOOL:
      free_scroll_tool(active_tool);
      break;
    }
    switch(type) {
    case MODIFY_TOOL:
      active_tool = create_modify_tool();
      break;
    case CREATE_OBJECT_TOOL:
      active_tool =
	create_create_object_tool(object_get_type((char *)extra_data),
				  (void *) user_data);
      break;
    case MAGNIFY_TOOL:
      active_tool = create_magnify_tool();
      break;
    case SCROLL_TOOL:
      active_tool = create_scroll_tool();
      break;
    }
  }
  switch(type) {
  case MODIFY_TOOL:
      break;
  case CREATE_OBJECT_TOOL:
    objtype = object_get_type((char *)extra_data);
    defaults_show(objtype);
    break;
  case MAGNIFY_TOOL:
    break;
  case SCROLL_TOOL:
    break;
  }
}

void
tool_reset(void)
{
  gtk_signal_emit_by_name(GTK_OBJECT(modify_tool_button), "clicked",
			  GTK_BUTTON(modify_tool_button), NULL);
}

