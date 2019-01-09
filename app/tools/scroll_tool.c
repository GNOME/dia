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

#include "scroll_tool.h"
#include "handle_ops.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "message.h"
#include "cursor.h"

static void scroll_button_press   (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);
static void scroll_button_release (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);
static void scroll_motion         (DiaTool        *tool,
                                   GdkEventMotion *event,
                                   DiaDisplay     *ddisp);
static void scroll_double_click   (DiaTool        *tool,
                                   GdkEventButton *event,
                                   DiaDisplay     *ddisp);

G_DEFINE_TYPE (DiaScrollTool, dia_scroll_tool, DIA_TYPE_TOOL)

static void
activate (DiaTool *tool)
{
  DiaScrollTool *self = DIA_SCROLL_TOOL (tool);

  self->scrolling = FALSE;
  self->use_hand = TRUE;

  dia_display_set_all_cursor (get_cursor (CURSOR_GRAB));
}

static void
deactivate (DiaTool *tool)
{
  dia_display_set_all_cursor (default_cursor);
}

static void
dia_scroll_tool_class_init (DiaScrollToolClass *klass)
{
  DiaToolClass *tool_class = DIA_TOOL_CLASS (klass);

  tool_class->activate = activate;
  tool_class->deactivate = deactivate;

  tool_class->button_press = scroll_button_press;
  tool_class->button_release = scroll_button_release;
  tool_class->motion = scroll_motion;
  tool_class->double_click = scroll_double_click;
}

static void
dia_scroll_tool_init (DiaScrollTool *self)
{
}

static void
scroll_double_click (DiaTool        *tool,
                     GdkEventButton *event,
                     DiaDisplay     *ddisp)
{
  /* Do nothing */
}

static void
scroll_button_press (DiaTool        *tool,
                     GdkEventButton *event,
                     DiaDisplay     *ddisp)
{
  DiaScrollTool *self = DIA_SCROLL_TOOL (tool);
  Point clickedpoint;

  self->use_hand = (event->state & GDK_SHIFT_MASK) == 0;
  if (self->use_hand)
    dia_display_set_all_cursor (get_cursor (CURSOR_GRABBING));
  else
    dia_display_set_all_cursor (get_cursor (CURSOR_SCROLL));

  dia_display_untransform_coords (ddisp,
                                  (int)event->x, (int)event->y,
                                  &clickedpoint.x, &clickedpoint.y);

  self->scrolling = TRUE;
  self->last_pos = clickedpoint;
}

static void
scroll_motion (DiaTool        *tool,
               GdkEventMotion *event,
               DiaDisplay     *ddisp)
{
  DiaScrollTool *self = DIA_SCROLL_TOOL (tool);
  Point to;
  Point delta;

  /* set the cursor appropriately, and change use_hand if needed */
  if (!self->scrolling) {
    /* try to minimise the number of cursor type changes */
    if ((event->state & GDK_SHIFT_MASK) == 0) {
      if (!self->use_hand) {
        self->use_hand = TRUE;
        dia_display_set_all_cursor (get_cursor (CURSOR_GRAB));
      }
    } else
      if (self->use_hand) {
        self->use_hand = FALSE;
        dia_display_set_all_cursor (get_cursor (CURSOR_SCROLL));
      }
    return;
  }

  dia_display_untransform_coords (ddisp, event->x, event->y, &to.x, &to.y);

  if (self->use_hand) {
    delta = self->last_pos;
    point_sub(&delta, &to);

    self->last_pos = to;
    point_add(&self->last_pos, &delta);

    /* we use this so you can scroll past the edge of the image */
    point_add(&delta, &ddisp->origo);
    dia_display_set_origo (ddisp, delta.x, delta.y);
    dia_display_update_scrollbars (ddisp);
    dia_display_add_update_all(ddisp);
  } else {
    delta = to;
    point_sub(&delta, &self->last_pos);
    point_scale(&delta, 0.5);

    dia_display_scroll (ddisp, &delta);
    self->last_pos = to;
  }
  dia_display_flush (ddisp);
}

static void
scroll_button_release (DiaTool        *tool,
                       GdkEventButton *event,
                       DiaDisplay     *ddisp)
{
  DiaScrollTool *self = DIA_SCROLL_TOOL (tool);
  self->use_hand = (event->state & GDK_SHIFT_MASK) == 0;
  if (self->use_hand) {
    dia_display_set_all_cursor(get_cursor(CURSOR_GRAB));
  } else
    dia_display_set_all_cursor(get_cursor(CURSOR_SCROLL));

  self->scrolling = FALSE;
}
