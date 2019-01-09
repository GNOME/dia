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

struct _DiaMagnifyTool {
  DiaTool tool;

  int box_active;
  int moved;
  int x, y;
  int oldx, oldy;

  gboolean zoom_out;
};

G_DEFINE_TYPE (DiaMagnifyTool, dia_magnify_tool, DIA_TYPE_TOOL)

static void
magnify_button_press (DiaTool        *self,
                      GdkEventButton *event,
                      DiaDisplay    *ddisp)
{
  DiaMagnifyTool *tool = DIA_MAGNIFY_TOOL (self);
  tool->x = tool->oldx = event->x;
  tool->y = tool->oldy = event->y;
  tool->box_active = TRUE;
  tool->moved = FALSE;
  gdk_pointer_grab (gtk_widget_get_window(ddisp->canvas), FALSE,
		    GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
		    NULL, NULL, event->time);
}

static void
magnify_button_release (DiaTool        *self,
                        GdkEventButton *event,
                        DiaDisplay    *ddisp)
{
  DiaMagnifyTool *tool = DIA_MAGNIFY_TOOL (self);
  Rectangle *visible;
  Point p1, p2, tl;
  real diff;
  int idiff;
  real factor;

  tool->box_active = FALSE;

  dia_interactive_renderer_set_selection (ddisp->renderer,
                                          FALSE, 0, 0, 0, 0);
  dia_display_flush (ddisp);


  visible = &ddisp->visible;
  
  dia_display_untransform_coords (ddisp, tool->x, tool->y, &p1.x, &p1.y);
  dia_display_untransform_coords (ddisp, event->x, event->y, &p2.x, &p2.y);

  tl.x = MIN(p1.x, p2.x);
  tl.y = MIN(p1.y, p2.y);

  diff = MAX(fabs(p2.x - p1.x), fabs(p2.y - p1.y));
  idiff = MAX(abs(tool->x - event->x), abs(tool->y - event->y));

  if (tool->moved) {
    if (idiff <= 4) {
      dia_display_add_update_all (ddisp);
      dia_display_flush (ddisp);
    } else if (!(event->state & GDK_CONTROL_MASK)) {
      /* the whole zoom rect should be visible, not just it's square equivalent */
      real fh = fabs(p2.y - p1.y) / (visible->bottom - visible->top);
      real fw = fabs(p2.x - p1.x) / (visible->right - visible->left);
      factor = 1.0 / MAX(fh, fw);
      tl.x += (visible->right - visible->left)/(2.0*factor);
      tl.y += (visible->bottom - visible->top)/(2.0*factor);
      dia_display_zoom (ddisp, &tl, factor);
    } else {
      factor = diff / (visible->right - visible->left);
      tl.x = tl.x * factor + tl.x;
      tl.y = tl.y * factor + tl.y;
      dia_display_zoom (ddisp, &tl, factor);
    }
  } else {
    if (event->state & GDK_SHIFT_MASK)
      dia_display_zoom (ddisp, &tl, 0.5);
    else
      dia_display_zoom (ddisp, &tl, 2.0);
  }

  gdk_pointer_ungrab (event->time);
}

typedef struct intPoint { int x,y; } intPoint;

static void
magnify_motion (DiaMagnifyTool *self,
                GdkEventMotion *event,
                DiaDisplay    *ddisp)
{
  DiaMagnifyTool *tool = DIA_MAGNIFY_TOOL (self);
  intPoint tl, br;

  if (tool->box_active) {
    tool->moved = TRUE;
    
    tl.x = MIN (tool->x, event->x); tl.y = MIN (tool->y, event->y);
    br.x = MAX (tool->x, event->x); br.y = MAX (tool->y, event->y);

    dia_interactive_renderer_set_selection (ddisp->renderer,
                                            TRUE,
                                            tl.x, tl.y, br.x - tl.x, br.y - tl.y);
    dia_display_flush (ddisp);
  }
}

void
set_zoom_out (DiaTool *tool)
{
  ((DiaMagnifyTool *)tool)->zoom_out = TRUE;
  dia_display_set_all_cursor(get_cursor(CURSOR_ZOOM_OUT));
}

void
set_zoom_in (DiaTool *tool)
{
  ((DiaMagnifyTool *)tool)->zoom_out = FALSE;
  dia_display_set_all_cursor(get_cursor(CURSOR_ZOOM_IN));
}

static void
activate (DiaTool *self)
{
  DIA_MAGNIFY_TOOL (self)->box_active = FALSE;
  DIA_MAGNIFY_TOOL (self)->zoom_out = FALSE;

  dia_display_set_all_cursor(get_cursor(CURSOR_ZOOM_IN));
}

static void
deactivate (DiaTool *tool)
{
  dia_display_set_all_cursor(default_cursor);
}

static void
dia_magnify_tool_class_init (DiaMagnifyToolClass *klass)
{
  DiaToolClass *tool_class = DIA_TOOL_CLASS (klass);

  tool_class->activate = activate;
  tool_class->deactivate = deactivate;
  tool_class->button_press = magnify_button_press;
  tool_class->button_release = magnify_button_release;
  tool_class->motion = magnify_motion;
}

static void
dia_magnify_tool_init (DiaMagnifyTool *self)
{
}
