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
#include "tool.h"
#include "create_object.h"
#include "magnify.h"
#include "modify_tool.h"
#include "scroll_tool.h"
#include "interface.h"
#include "defaults.h"
#include "intl.h"

#include "pixmaps.h"

Tool *active_tool = NULL;

ToolButton tool_data[] =
{
  { (char **) arrow_xpm,
    N_("Modify object(s)"),
    { MODIFY_TOOL, NULL, NULL}
  },
  { (char **) magnify_xpm,
    N_("Magnify"),
    { MAGNIFY_TOOL, NULL, NULL}
  },
  { (char **) scroll_xpm,
    N_("Scroll around the diagram"),
    { SCROLL_TOOL, NULL, NULL}
  },
  { NULL,
    N_("Create Text"),
    { CREATE_TEXT_TOOL, "Standard - Text", NULL }
  },
  { NULL,
    N_("Create Box"),
    { CREATE_BOX_TOOL, "Standard - Box", NULL }
  },
  { NULL,
    N_("Create Ellipse"),
    { CREATE_ELLIPSE_TOOL, "Standard - Ellipse", NULL }
  },
  { NULL,
    N_("Create Line"),
    { CREATE_LINE_TOOL, "Standard - Line", NULL }
  },
  { NULL,
    N_("Create Arc"),
    { CREATE_ARC_TOOL, "Standard - Arc", NULL }
  },
  { NULL,
    N_("Create Zigzagline"),
    { CREATE_ZIGZAG_TOOL, "Standard - ZigZagLine", NULL }
  },
  { NULL,
    N_("Create Polyline"),
    { CREATE_POLYLINE_TOOL, "Standard - PolyLine", NULL }
  },
  { NULL,
    N_("Create Image"),
    { CREATE_IMAGE_TOOL, "Standard - Image", NULL }
  }
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (ToolButton))
int num_tool_data = NUM_TOOLS;

void 
tool_select(ToolType type, gpointer extra_data, gpointer user_data)
{
  switch(active_tool->type) {
  case MODIFY_TOOL:
    free_modify_tool(active_tool);
    break;
  case CREATE_TEXT_TOOL:
  case CREATE_BOX_TOOL:
  case CREATE_ELLIPSE_TOOL:
  case CREATE_LINE_TOOL:
  case CREATE_ARC_TOOL:
  case CREATE_ZIGZAG_TOOL:
  case CREATE_POLYLINE_TOOL:
  case CREATE_IMAGE_TOOL:
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
  case CREATE_TEXT_TOOL:
  case CREATE_BOX_TOOL:
  case CREATE_ELLIPSE_TOOL:
  case CREATE_LINE_TOOL:
  case CREATE_ARC_TOOL:
  case CREATE_ZIGZAG_TOOL:
  case CREATE_POLYLINE_TOOL:
  case CREATE_IMAGE_TOOL:
    active_tool =
      create_create_object_tool(object_get_type((char *)extra_data),
				type, (void *) user_data);
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
    case CREATE_TEXT_TOOL:
    case CREATE_BOX_TOOL:
    case CREATE_ELLIPSE_TOOL:
    case CREATE_LINE_TOOL:
    case CREATE_ARC_TOOL:
    case CREATE_ZIGZAG_TOOL:
    case CREATE_POLYLINE_TOOL:
    case CREATE_IMAGE_TOOL:
      free_create_object_tool(active_tool);
      break;
    case MAGNIFY_TOOL:
      free_magnify_tool(active_tool);
      break;
    case SCROLL_TOOL:
      free_scroll_tool(active_tool);
      break;
    default:
      g_warning("Can't free tool %d!\n", type);
    }
    switch(type) {
    case MODIFY_TOOL:
      active_tool = create_modify_tool();
      break;
    case CREATE_TEXT_TOOL:
    case CREATE_BOX_TOOL:
    case CREATE_ELLIPSE_TOOL:
    case CREATE_LINE_TOOL:
    case CREATE_ARC_TOOL:
    case CREATE_ZIGZAG_TOOL:
    case CREATE_POLYLINE_TOOL:
    case CREATE_IMAGE_TOOL:
      active_tool =
	create_create_object_tool(object_get_type((char *)extra_data),
				  type, (void *) user_data);
      break;
    case MAGNIFY_TOOL:
      active_tool = create_magnify_tool();
      break;
    case SCROLL_TOOL:
      active_tool = create_scroll_tool();
      break;
    default:
      g_warning("Can't create tool %d!\n", type);
    }
  }
  switch(type) {
  case MODIFY_TOOL:
      break;
  case CREATE_TEXT_TOOL:
  case CREATE_BOX_TOOL:
  case CREATE_ELLIPSE_TOOL:
  case CREATE_LINE_TOOL:
  case CREATE_ARC_TOOL:
  case CREATE_ZIGZAG_TOOL:
  case CREATE_POLYLINE_TOOL:
  case CREATE_IMAGE_TOOL:
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
tool_set(ToolType type)
{
  GtkWidget *tool_widget = NULL;
  int i;
  
  tool_select(type, tool_data[type].callback_data.extra_data, tool_data[type].callback_data.user_data);


  for (i=0;i<num_tool_data;i++) {
    if (tool_data[i].callback_data.type == type) {
      gtk_widget_activate(tool_data[i].tool_widget);
      break;
    }
  }
}
