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

#include <gdk/gdk.h>

#define DIA_CURSOR GDK_LAST_CURSOR

#include "display.h"
#include "cursor.h"

#include "pixmaps/hand-open-data.xbm"
#include "pixmaps/hand-open-mask.xbm"
#include "pixmaps/hand-closed-data.xbm"
#include "pixmaps/hand-closed-mask.xbm"
#include "pixmaps/magnify-plus-data.xbm"
#include "pixmaps/magnify-plus-mask.xbm"
#include "pixmaps/magnify-minus-data.xbm"
#include "pixmaps/magnify-minus-mask.xbm"

static struct {
  /* Can't use a union because it can't be statically initialized
     (except for the first element) */
  int gdk_cursor_number;
  gchar *data;
  int width;
  int height;
  gchar *mask;
  int hot_x;
  int hot_y;
  GdkCursor *cursor;
} cursors[MAX_CURSORS] = {
  { GDK_LEFT_PTR },
  { GDK_FLEUR },
  { DIA_CURSOR,
    hand_open_data_bits,
    hand_open_data_width, hand_open_data_height,
    hand_open_mask_bits,
    hand_open_data_width/2, hand_open_data_height/2},
  { DIA_CURSOR, 
    hand_closed_data_bits,
    hand_closed_data_width, hand_closed_data_height,
    hand_closed_mask_bits,
    hand_closed_data_width/2, hand_closed_data_height/2},
  { DIA_CURSOR,
    magnify_minus_data_bits,
    magnify_minus_data_width, magnify_minus_data_height,
    magnify_minus_mask_bits,
    magnify_minus_data_x_hot, magnify_minus_data_y_hot},
  { DIA_CURSOR,
    magnify_plus_data_bits,
    magnify_plus_data_width, magnify_plus_data_height,
    magnify_plus_mask_bits,
    magnify_plus_data_x_hot, magnify_plus_data_y_hot},
  { GDK_CROSS_REVERSE },
  { GDK_XTERM },
};

GdkCursor *
get_cursor(DiaCursorType ctype) {
  if (ctype >= MAX_CURSORS || ctype < 0) {
    return NULL;
  }
  if (cursors[ctype].cursor == NULL) {
    GdkCursor *new_cursor = NULL;

    if (cursors[ctype].gdk_cursor_number != DIA_CURSOR) {
      new_cursor = gdk_cursor_new(cursors[ctype].gdk_cursor_number);
    } else {
      DDisplay *active_display = ddisplay_active (); 
      if (active_display != NULL) 
	new_cursor = create_cursor(active_display->canvas->window,
				   cursors[ctype].data,
				   cursors[ctype].width,
				   cursors[ctype].height,
				   cursors[ctype].mask,
				   cursors[ctype].hot_x,
				   cursors[ctype].hot_y);
    }
    cursors[ctype].cursor = new_cursor;
  }

  return cursors[ctype].cursor;
}

GdkCursor *
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

  /* For some odd reason, black and white is inverted */
  gdk_color_black(gdk_window_get_colormap(window), &white);
  gdk_color_white(gdk_window_get_colormap(window), &black);

  cursor = gdk_cursor_new_from_pixmap(dbit, mbit, &white, &black, hot_x,hot_y);
  g_assert(cursor != NULL);

  gdk_bitmap_unref(dbit);
  gdk_bitmap_unref(mbit);

  return cursor;
}
