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
#include "scroll_tool.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "message.h"

static void scroll_button_press(ScrollTool *tool, GdkEventButton *event,
				 DDisplay *ddisp);
static void scroll_button_release(ScrollTool *tool, GdkEventButton *event,
				  DDisplay *ddisp);
static void scroll_motion(ScrollTool *tool, GdkEventMotion *event,
			  DDisplay *ddisp);
static void scroll_double_click(ScrollTool *tool, GdkEventButton *event,
				DDisplay *ddisp);

static GdkCursor *scroll_cursor = NULL;

Tool *
create_scroll_tool(void)
{
  ScrollTool *tool;

  tool = g_new(ScrollTool, 1);
  tool->tool.type = SCROLL_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &scroll_button_press;
  tool->tool.button_release_func = (ButtonReleaseFunc) &scroll_button_release;
  tool->tool.motion_func = (MotionFunc) &scroll_motion;
  tool->tool.double_click_func = (DoubleClickFunc) &scroll_double_click;

  tool->scrolling = FALSE;

  if (scroll_cursor == NULL) {
    scroll_cursor = gdk_cursor_new(GDK_FLEUR);
  }
  ddisplay_set_all_cursor(scroll_cursor);
  
  return (Tool *)tool;
}


void
free_scroll_tool(Tool *tool)
{
  g_free(tool);
  ddisplay_set_all_cursor(default_cursor);

}

static void
scroll_double_click(ScrollTool *tool, GdkEventButton *event,
		    DDisplay *ddisp)
{
  /* Do nothing */
}

static void
scroll_button_press(ScrollTool *tool, GdkEventButton *event,
		     DDisplay *ddisp)
{
  Point clickedpoint;
  
  ddisplay_untransform_coords(ddisp,
			      (int)event->x, (int)event->y,
			      &clickedpoint.x, &clickedpoint.y);

  tool->scrolling = TRUE;
  tool->last_pos = clickedpoint;
}




static void
scroll_motion(ScrollTool *tool, GdkEventMotion *event,
	      DDisplay *ddisp)
{
  Point to;
  Point delta;

  
  if (!tool->scrolling)
    return;

  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  delta = to;
  point_sub(&delta, &tool->last_pos);
  point_scale(&delta, 0.5);

  ddisplay_scroll(ddisp, &delta);
  ddisplay_flush(ddisp);

  tool->last_pos = to;
}


static void
scroll_button_release(ScrollTool *tool, GdkEventButton *event,
		      DDisplay *ddisp)
{
  tool->scrolling = FALSE;
}






