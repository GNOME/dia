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
#include "handle_ops.h"
#include "color.h"

/* This value is best left odd so that the handles are centered. */
#define HANDLE_SIZE 7

static int handles_initialized = 0;
static GdkGC *handle_gc = NULL;
static GdkColor handle_color[NUM_HANDLE_TYPES];
static GdkColor handle_color_connected[NUM_HANDLE_TYPES];

void
handle_draw(Handle *handle, DDisplay *ddisp)
{
  int x,y;
  if (!handles_initialized) {
    Color col;
    
    handles_initialized = 1;

    handle_gc = gdk_gc_new(ddisp->pixmap);

    gdk_gc_set_line_attributes (handle_gc,
				1,
				GDK_LINE_SOLID,
				GDK_CAP_BUTT,
				GDK_JOIN_MITER);

    col.red = 0.0; col.green = 1.0; col.blue = 0.0;
    color_convert(&col, &handle_color[HANDLE_MAJOR_CONTROL]);
    col.red = 1.0; col.green = 0.0; col.blue = 0.0;
    color_convert(&col, &handle_color_connected[HANDLE_MAJOR_CONTROL]);

    col.red = 1.0; col.green = 0.4; col.blue = 0.0;
    color_convert(&col, &handle_color[HANDLE_MINOR_CONTROL]);
    handle_color_connected[HANDLE_MINOR_CONTROL] = 
      handle_color[HANDLE_MINOR_CONTROL];

    col.red = 0.0; col.green = 0.0; col.blue = 0.4;
    color_convert(&col, &handle_color[HANDLE_NON_MOVABLE]);
    handle_color_connected[HANDLE_NON_MOVABLE] = 
      handle_color[HANDLE_NON_MOVABLE];
    
  }

  ddisplay_transform_coords(ddisp, handle->pos.x, handle->pos.y, &x, &y);

  if  (handle->connected_to != NULL) {
    gdk_gc_set_foreground(handle_gc, &handle_color_connected[handle->type]);
  } else {
    gdk_gc_set_foreground(handle_gc, &handle_color[handle->type]);
  }
  
  gdk_draw_rectangle (ddisp->pixmap,
		      handle_gc,
		      TRUE,
		      x - HANDLE_SIZE/2 + 1, y - HANDLE_SIZE/2 + 1,
		      HANDLE_SIZE-2, HANDLE_SIZE-2);
 
  gdk_gc_set_foreground(handle_gc, &color_gdk_black);

  gdk_draw_rectangle (ddisp->pixmap,
		      handle_gc,
		      FALSE,
		      x - HANDLE_SIZE/2, y - HANDLE_SIZE/2,
		      HANDLE_SIZE-1, HANDLE_SIZE-1);

  
  if (handle->connect_type != HANDLE_NONCONNECTABLE) {
    gdk_draw_line (ddisp->pixmap,
		   handle_gc,
		   x - HANDLE_SIZE/2, y - HANDLE_SIZE/2,
		   x + HANDLE_SIZE/2, y + HANDLE_SIZE/2);
    gdk_draw_line (ddisp->pixmap,
		   handle_gc,
		   x - HANDLE_SIZE/2, y + HANDLE_SIZE/2,
		   x + HANDLE_SIZE/2, y - HANDLE_SIZE/2);
  }
}
void gdk_draw_line       (GdkDrawable  *drawable,
                          GdkGC        *gc,
                          gint          x1,
                          gint          y1,
                          gint          x2,
                          gint          y2);


void
handle_add_update(Handle *handle, Diagram *dia)
{
  diagram_add_update_pixels(dia, &handle->pos,
			    HANDLE_SIZE, HANDLE_SIZE);
}

/* Call this after diagram_find_closest_handle() */
int
handle_is_clicked(DDisplay *ddisp, Handle *handle, Point *pos)
{
  real dx, dy;
  int idx, idy;

  if (handle==NULL)
    return FALSE;
  
  dx = ABS(handle->pos.x - pos->x);
  dy = ABS(handle->pos.y - pos->y);

  idx = ddisplay_transform_length(ddisp, dx);
  idy = ddisplay_transform_length(ddisp, dy);

  return (idx<(HANDLE_SIZE+1)/2) && (idy<(HANDLE_SIZE+1)/2);
}

