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

#include "handle_ops.h"
#include "handle.h"
#include "color.h"

/* This value is best left odd so that the handles are centered. */
#define HANDLE_SIZE 9

static const Color handle_color[NUM_HANDLE_TYPES<<1] =
{
  { 0.0, 0.0, 0.5, 1.0 }, /* HANDLE_NON_MOVABLE */
  { 0.0, 1.0, 0.0, 1.0 }, /* HANDLE_MAJOR_CONTROL */
  { 1.0, 0.6, 0.0, 1.0 }, /* HANDLE_MINOR_CONTROL */
  /* dim down the color if the handle is in a group of selected objects */
  { 0.0, 0.0, 0.5, 1.0 }, /* HANDLE_NON_MOVABLE */
  { 0.0, 0.7, 0.0, 1.0 }, /* HANDLE_MAJOR_CONTROL */
  { 0.7, 0.4, 0.0, 1.0 }, /* HANDLE_MINOR_CONTROL */
};

static const Color handle_color_connected[NUM_HANDLE_TYPES<<1] =
{
  { 0.0, 0.0, 0.5, 1.0 }, /* HANDLE_NON_MOVABLE */
  { 1.0, 0.0, 0.0, 1.0 }, /* HANDLE_MAJOR_CONTROL */
  { 1.0, 0.4, 0.0, 1.0 }, /* HANDLE_MINOR_CONTROL */
  /* dim down the color if the handle is in a group of selected objects */
  { 0.0, 0.0, 0.5, 1.0 }, /* HANDLE_NON_MOVABLE */
  { 0.7, 0.0, 0.0, 1.0 }, /* HANDLE_MAJOR_CONTROL */
  { 0.7, 0.3, 0.0, 1.0 }, /* HANDLE_MINOR_CONTROL */
};

void
handle_draw(Handle *handle, DDisplay *ddisp)
{
  gboolean some_selected;
  int x,y;
  DiaRenderer *renderer = ddisp->renderer;
  DiaInteractiveRendererInterface *irenderer =
    DIA_GET_INTERACTIVE_RENDERER_INTERFACE (ddisp->renderer);
  const Color *color;

  ddisplay_transform_coords(ddisp, handle->pos.x, handle->pos.y, &x, &y);
  /* change handle color to reflect different behaviour for multiple selected */
  /* this code relies on the fact that only selected objects get their handles drawn */
  some_selected = g_list_length (ddisp->diagram->data->selected) > 1;

  if  (handle->connected_to != NULL) {
    color = &handle_color_connected[handle->type + (some_selected ? NUM_HANDLE_TYPES : 0)];
  } else {
    color = &handle_color[handle->type + (some_selected ? NUM_HANDLE_TYPES : 0)];
  }

  DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, 0.0);
  DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID, 0.0);
  DIA_RENDERER_GET_CLASS(renderer)->set_linejoin(renderer, LINEJOIN_MITER);
  DIA_RENDERER_GET_CLASS(renderer)->set_fillstyle(renderer, FILLSTYLE_SOLID);
  

  irenderer->fill_pixel_rect(renderer,
			     x - HANDLE_SIZE/2 + 1,
			     y - HANDLE_SIZE/2 + 1,
			     HANDLE_SIZE-2, HANDLE_SIZE-2,
			     /* it does not change the color, but does not reflect taht in the signature */
			     (Color *)color);
  
  irenderer->draw_pixel_rect(renderer,
			     x - HANDLE_SIZE/2,
			     y - HANDLE_SIZE/2,
			     HANDLE_SIZE-1, HANDLE_SIZE-1,
			     &color_black);

    
  
  if (handle->connect_type != HANDLE_NONCONNECTABLE) {
    irenderer->draw_pixel_line
                         (renderer,
			  x - HANDLE_SIZE/2, y - HANDLE_SIZE/2,
			  x + HANDLE_SIZE/2, y + HANDLE_SIZE/2,
			  &color_black);
    irenderer->draw_pixel_line
                         (renderer,
			  x - HANDLE_SIZE/2, y + HANDLE_SIZE/2,
			  x + HANDLE_SIZE/2, y - HANDLE_SIZE/2,
			  &color_black);
  }
}

void
handle_add_update(Handle *handle, Diagram *dia)
{
  g_return_if_fail (handle != NULL);
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

