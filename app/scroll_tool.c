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

#include "pixmaps/hand-open-data.xbm"
#include "pixmaps/hand-open-mask.xbm"
#include "pixmaps/hand-closed-data.xbm"
#include "pixmaps/hand-closed-mask.xbm"

static void scroll_button_press(ScrollTool *tool, GdkEventButton *event,
				 DDisplay *ddisp);
static void scroll_button_release(ScrollTool *tool, GdkEventButton *event,
				  DDisplay *ddisp);
static void scroll_motion(ScrollTool *tool, GdkEventMotion *event,
			  DDisplay *ddisp);
static void scroll_double_click(ScrollTool *tool, GdkEventButton *event,
				DDisplay *ddisp);

static GdkCursor *scroll_cursor = NULL;
static GdkCursor *open_hand_cursor = NULL;
static GdkCursor *closed_hand_cursor = NULL;

static GdkCursor *
create_cursor(GdkWindow *window,
	      const gchar *data, int width, int height,
	      const gchar *mask, int hot_x, int hot_y)
{
  GdkBitmap *dbit, *mbit;
  GdkColor black, white;
  GdkCursor *cursor;

  g_return_val_if_fail(window != NULL, NULL);

  dbit = gdk_bitmap_create_from_data(window, data, width, height);
  mbit = gdk_bitmap_create_from_data(window, mask, width, height);
  g_assert(dbit != NULL && mbit != NULL);

  gdk_color_black(gdk_window_get_colormap(window), &black);
  gdk_color_white(gdk_window_get_colormap(window), &white);

  cursor = gdk_cursor_new_from_pixmap(dbit, mbit, &white,&black, hot_x,hot_y);
  g_assert(cursor != NULL);

  gdk_bitmap_unref(dbit);
  gdk_bitmap_unref(mbit);

  return cursor;
}
	      

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
  tool->use_hand = FALSE;

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

  tool->use_hand = (event->state & GDK_SHIFT_MASK) != 0;
  if (tool->use_hand) {
    if (!closed_hand_cursor)
      closed_hand_cursor = create_cursor(ddisp->canvas->window,
					 hand_closed_data_bits,
					 hand_closed_data_width,
					 hand_closed_data_height,
					 hand_closed_mask_bits,
					 hand_closed_data_width/2,
					 hand_closed_data_height/2);
    ddisplay_set_all_cursor(closed_hand_cursor);
  } else
    ddisplay_set_all_cursor(scroll_cursor);

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

  /* set the cursor appropriately, and change use_hand if needed */
  if (!tool->scrolling) {
    /* try to minimise the number of cursor type changes */
    if ((event->state & GDK_SHIFT_MASK) != 0) {
      if (!tool->use_hand) {
	tool->use_hand = TRUE;
	if (!open_hand_cursor)
	  open_hand_cursor = create_cursor(ddisp->canvas->window,
					   hand_open_data_bits,
					   hand_open_data_width,
					   hand_open_data_height,
					   hand_open_mask_bits,
					   hand_open_data_width/2,
					   hand_open_data_height/2);
	ddisplay_set_all_cursor(open_hand_cursor);
      }
    } else
      if (tool->use_hand) {
	tool->use_hand = FALSE;
	ddisplay_set_all_cursor(scroll_cursor);
      }
    return;
  }

  ddisplay_untransform_coords(ddisp, event->x, event->y, &to.x, &to.y);

  if (tool->use_hand) {
    delta = tool->last_pos;
    point_sub(&delta, &to);

    tool->last_pos = to;
    point_add(&tool->last_pos, &delta);

    /* we use this so you can scroll past the edge of the image */
    point_add(&delta, &ddisp->origo);
    ddisplay_set_origo(ddisp, delta.x, delta.y);
    ddisplay_update_scrollbars(ddisp);
    ddisplay_add_update_all(ddisp);
  } else {
    delta = to;
    point_sub(&delta, &tool->last_pos);
    point_scale(&delta, 0.5);

    ddisplay_scroll(ddisp, &delta);
    tool->last_pos = to;
  }
  ddisplay_flush(ddisp);
}


static void
scroll_button_release(ScrollTool *tool, GdkEventButton *event,
		      DDisplay *ddisp)
{
  tool->use_hand = (event->state & GDK_SHIFT_MASK) != 0;
  if (tool->use_hand) {
    if (!open_hand_cursor)
      open_hand_cursor = create_cursor(ddisp->canvas->window,
				       hand_open_data_bits,
				       hand_open_data_width,
				       hand_open_data_height,
				       hand_open_mask_bits,
				       hand_open_data_width/2,
				       hand_open_data_height/2);
    ddisplay_set_all_cursor(open_hand_cursor);
  } else
    ddisplay_set_all_cursor(scroll_cursor);

  tool->scrolling = FALSE;
}

