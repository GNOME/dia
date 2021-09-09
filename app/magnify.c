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
#include <stdlib.h>
#include <math.h>

#include "magnify.h"
#include "cursor.h"
#include "diainteractiverenderer.h"

struct _MagnifyTool {
  Tool tool;
  int box_active;
  int moved;
  int x, y;
  int oldx, oldy;

  gboolean zoom_out;
};

static void
magnify_button_press(MagnifyTool *tool, GdkEventButton *event,
		     DDisplay *ddisp)
{
  tool->x = tool->oldx = event->x;
  tool->y = tool->oldy = event->y;
  tool->box_active = TRUE;
  tool->moved = FALSE;

  gdk_device_grab (gdk_event_get_device ((GdkEvent*)event),
                   gtk_widget_get_window(ddisp->canvas),
                   GDK_OWNERSHIP_APPLICATION,
                   FALSE,
                   GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
                   NULL, event->time);
}

static void
magnify_button_release (MagnifyTool    *tool,
                        GdkEventButton *event,
                        DDisplay       *ddisp)
{
  DiaRectangle *visible;
  Point p1, p2, tl;
  real diff;
  int idiff;
  real factor;

  tool->box_active = FALSE;

  dia_interactive_renderer_set_selection (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                          FALSE, 0, 0, 0, 0);
  ddisplay_flush (ddisp);


  visible = &ddisp->visible;

  ddisplay_untransform_coords(ddisp, tool->x, tool->y, &p1.x, &p1.y);
  ddisplay_untransform_coords(ddisp, event->x, event->y, &p2.x, &p2.y);

  tl.x = MIN(p1.x, p2.x);
  tl.y = MIN(p1.y, p2.y);

  diff = MAX(fabs(p2.x - p1.x), fabs(p2.y - p1.y));
  idiff = MAX(abs(tool->x - event->x), abs(tool->y - event->y));

  if (tool->moved) {
    if (idiff <= 4) {
      ddisplay_add_update_all(ddisp);
      ddisplay_flush(ddisp);
    } else if (!(event->state & GDK_CONTROL_MASK)) {
      /* the whole zoom rect should be visible, not just it's square equivalent */
      real fh = fabs(p2.y - p1.y) / (visible->bottom - visible->top);
      real fw = fabs(p2.x - p1.x) / (visible->right - visible->left);
      factor = 1.0 / MAX(fh, fw);
      tl.x += (visible->right - visible->left)/(2.0*factor);
      tl.y += (visible->bottom - visible->top)/(2.0*factor);
      ddisplay_zoom(ddisp, &tl, factor);
    } else {
      factor = diff / (visible->right - visible->left);
      tl.x = tl.x * factor + tl.x;
      tl.y = tl.y * factor + tl.y;
      ddisplay_zoom(ddisp, &tl, factor);
    }
  } else {
    if (event->state & GDK_SHIFT_MASK)
      ddisplay_zoom(ddisp, &tl, 0.5);
    else
      ddisplay_zoom(ddisp, &tl, 2.0);
  }

  gdk_device_ungrab (gdk_event_get_device((GdkEvent*)event), event->time);
}

typedef struct intPoint { int x,y; } intPoint;

static void
magnify_motion(MagnifyTool *tool, GdkEventMotion *event,
	       DDisplay *ddisp)
{
  intPoint tl, br;

  if (tool->box_active) {
    tool->moved = TRUE;

    tl.x = MIN (tool->x, event->x); tl.y = MIN (tool->y, event->y);
    br.x = MAX (tool->x, event->x); br.y = MAX (tool->y, event->y);

    dia_interactive_renderer_set_selection (DIA_INTERACTIVE_RENDERER (ddisp->renderer),
                                            TRUE,
                                            tl.x, tl.y, br.x - tl.x, br.y - tl.y);
    ddisplay_flush (ddisp);
  }
}

void
set_zoom_out(Tool *tool)
{
  ((MagnifyTool *)tool)->zoom_out = TRUE;
  ddisplay_set_all_cursor_name (NULL, "zoom-out");
}

void
set_zoom_in(Tool *tool)
{
  ((MagnifyTool *)tool)->zoom_out = FALSE;
  ddisplay_set_all_cursor_name (NULL, "zoom-in");
}

Tool *
create_magnify_tool(void)
{
  MagnifyTool *tool;

  tool = g_new0(MagnifyTool, 1);
  tool->tool.type = MAGNIFY_TOOL;
  tool->tool.button_press_func = (ButtonPressFunc) &magnify_button_press;
  tool->tool.button_release_func = (ButtonPressFunc) &magnify_button_release;
  tool->tool.motion_func = (MotionFunc) &magnify_motion;
  tool->tool.double_click_func = NULL;

  tool->box_active = FALSE;
  tool->zoom_out = FALSE;

  ddisplay_set_all_cursor_name (NULL, "zoom-in");

  return (Tool *) tool;
}


void
free_magnify_tool (Tool *tool)
{
  g_clear_pointer (&tool, g_free);
  ddisplay_set_all_cursor (default_cursor);
}
