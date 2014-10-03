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

#include "dia-app-icons.h"

static struct {
  /* Can't use a union because it can't be statically initialized
     (except for the first element) */
  int gdk_cursor_number;
  const guint8 *data;
  int hot_x;
  int hot_y;
  GdkCursor *cursor;
} cursors[MAX_CURSORS] = {
  { GDK_LEFT_PTR }, /* CURSOR_POINT */
  { DIA_CURSOR, /* CURSOR_CREATE */
    dia_cursor_create,
    0, 0},  
  { GDK_FLEUR }, /* CURSOR_SCROLL */
  { DIA_CURSOR, /* CURSOR_GRAB */
    dia_cursor_hand_open,
    10, 10 },
  { DIA_CURSOR, /* CURSOR_GRABBING */
    dia_cursor_hand_closed,
    10, 10 },
  { DIA_CURSOR, /* CURSOR_ZOOM_OUT */
    dia_cursor_magnify_minus,
    8, 8 },
  { DIA_CURSOR, /* CURSOR_ZOOM_IN */
    dia_cursor_magnify_plus,
    8, 8 },
  { GDK_CROSS_REVERSE }, /* CURSOR_CONNECT */
  { GDK_XTERM }, /* CURSOR_XTERM */
  /* for safety reasons these should be last and must be in the same order HANDLE_RESIZE_* */
  { GDK_TOP_LEFT_CORNER },/* CURSOR_DIRECTION_0 + NW */
  { GDK_TOP_SIDE },/* N */
  { GDK_TOP_RIGHT_CORNER },/* NE */
  { GDK_LEFT_SIDE },/* W */
  { GDK_RIGHT_SIDE },/* E */
  { GDK_BOTTOM_LEFT_CORNER },/* SE */
  { GDK_BOTTOM_SIDE },/* S */
  { GDK_BOTTOM_RIGHT_CORNER }, /* SW */
};


static GdkCursor *create_cursor(GdkWindow *window,
				const guint8 *data, 
				int hot_x, int hot_y);

GdkCursor *
get_cursor(DiaCursorType ctype)
{
  if (ctype >= G_N_ELEMENTS (cursors)) {
    return NULL;
  }
  if (cursors[ctype].cursor == NULL) {
    GdkCursor *new_cursor = NULL;

    if (cursors[ctype].gdk_cursor_number != DIA_CURSOR) {
      new_cursor = gdk_cursor_new(cursors[ctype].gdk_cursor_number);
    } else {
      DDisplay *active_display = ddisplay_active (); 
      if (active_display != NULL) 
	new_cursor = create_cursor(gtk_widget_get_window(active_display->canvas),
				   cursors[ctype].data,
				   cursors[ctype].hot_x,
				   cursors[ctype].hot_y);
    }
    cursors[ctype].cursor = new_cursor;
  }

  return cursors[ctype].cursor;
}

GdkCursor *
create_cursor(GdkWindow *window,
	      const guint8 *data,
	      int hot_x, int hot_y)
{
  GdkPixbuf *pixbuf;
  GdkCursor *cursor;
  GdkDisplay *display;

  g_return_val_if_fail(window != NULL, NULL);
#if GTK_CHECK_VERSION (2,24,0)
  display = gdk_window_get_display (window);
#else
  display = gdk_drawable_get_display (GDK_DRAWABLE (window));
#endif

  pixbuf = gdk_pixbuf_new_from_inline(-1, data, FALSE, NULL);

  cursor = gdk_cursor_new_from_pixbuf (display, pixbuf, hot_x,hot_y);
  g_assert(cursor != NULL);

  g_object_unref(pixbuf);

  return cursor;
}
