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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>

#include "intl.h"
#include "grid.h"
#include "preferences.h"

/** Calculate the width (in cm) of the gap between grid lines in dynamic
 * grid mode.
 */
static real
calculate_dynamic_grid(DDisplay *ddisp, real *width_x, real *width_y)
{
  real zoom = ddisplay_untransform_length(ddisp, 1.0);
  /* Twiddle zoom to make change-over appropriate */
  zoom *= 5;
  return pow(10, ceil(log10(zoom)));
}

static void
grid_draw_horizontal_lines(DDisplay *ddisp, Rectangle *update, real length) 
{
  int x, y;
  real pos;
  int height, width;
  guint major_lines = ddisp->diagram->data->grid.major_lines;
  DiaRenderer *renderer = ddisp->renderer;
  DiaInteractiveRendererInterface *irenderer;
  int major_count;

  irenderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);

  pos = ceil( update->top / length ) * length;
  ddisplay_transform_coords(ddisp, update->left, pos, &x, &y);
  ddisplay_transform_coords(ddisp, update->right, update->bottom, &width, &height);

  if (major_lines) {
    major_count = pos/length;
    major_count %= major_lines;
  }

  while (y < height) {
    if (major_lines) {
      if (major_count == 0)
	DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
      else
	DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_DOTTED);
      major_count = (major_count+1)%major_lines;
    }
    irenderer->draw_pixel_line(renderer, x, y, width, y,
			       &ddisp->diagram->data->grid.colour);
    pos += length;
    ddisplay_transform_coords(ddisp, update->left, pos, &x, &y);
  }
}

static void
grid_draw_vertical_lines(DDisplay *ddisp, Rectangle *update, real length) 
{
  int x = 0, y = 0;
  real pos;
  int height, width;
  guint major_lines = ddisp->diagram->data->grid.major_lines;
  DiaRenderer *renderer = ddisp->renderer;
  DiaInteractiveRendererInterface *irenderer;
  int major_count;

  irenderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);

  pos = ceil( update->left / length ) * length;
  ddisplay_transform_coords(ddisp, update->right, update->bottom, &width, &height);

  if (major_lines) {
    major_count = pos/length;
    major_count %= major_lines;
  }

  while (x < width) {
    ddisplay_transform_coords(ddisp, pos, update->top, &x, &y);
    if (major_lines) {
      if (major_count == 0)
	DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
      else
	DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_DOTTED);
      major_count = (major_count+1)%major_lines;
    }
    irenderer->draw_pixel_line(renderer, x, y, x, height,
			       &ddisp->diagram->data->grid.colour);
    pos += length;
  }
}

void
grid_draw(DDisplay *ddisp, Rectangle *update)
{
  Grid *grid = &ddisp->grid;
  DiaRenderer *renderer = ddisp->renderer;
  int major_lines;

  if (grid->visible) {
    int width = dia_renderer_get_width_pixels(ddisp->renderer);
    int height = dia_renderer_get_height_pixels(ddisp->renderer);
    /* distance between visible grid lines */
    real width_x = ddisp->diagram->data->grid.width_x;
    real width_y = ddisp->diagram->data->grid.width_y;
    if (ddisp->diagram->data->grid.dynamic) {
      width_x = width_y = calculate_dynamic_grid(ddisp, &width_x, &width_y);
    } else {
      width_x = ddisp->diagram->data->grid.width_x *
	ddisp->diagram->data->grid.visible_x;
      width_y = ddisp->diagram->data->grid.width_y *
	ddisp->diagram->data->grid.visible_y;
    }

    DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, 0.0);
    DIA_RENDERER_GET_CLASS(renderer)->set_dashlength(renderer,
						     ddisplay_untransform_length(ddisp, 31));
    
    if (ddisplay_transform_length(ddisp, width_y) >= 2.0 &&
	ddisplay_transform_length(ddisp, width_x) >= 2.0) {
      /* Vertical lines: */
      grid_draw_vertical_lines(ddisp, update, width_x);
      /* Horizontal lines: */
      grid_draw_horizontal_lines(ddisp, update, width_y);
    }
  }
}

void
pagebreak_draw(DDisplay *ddisp, Rectangle *update)
{
  DiaRenderer *renderer = ddisp->renderer;
  DiaInteractiveRendererInterface *irenderer;

  int width = dia_renderer_get_width_pixels(ddisp->renderer);
  int height = dia_renderer_get_height_pixels(ddisp->renderer);
  
  irenderer = DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer);
  if (prefs.pagebreak.visible) {
    Diagram *dia = ddisp->diagram;
    real origx = 0, origy = 0, pos;
    real pwidth = dia->data->paper.width;
    real pheight = dia->data->paper.height;
    int x,y;

    DIA_RENDERER_GET_CLASS(renderer)->set_linewidth(renderer, 0.0);
    DIA_RENDERER_GET_CLASS(renderer)->set_dashlength(renderer,
				    ddisplay_untransform_length(ddisp, 31));
    if (prefs.pagebreak.solid)
      DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_SOLID);
    else
      DIA_RENDERER_GET_CLASS(renderer)->set_linestyle(renderer, LINESTYLE_DOTTED);

    if (dia->data->paper.fitto) {
      origx = dia->data->extents.left;
      origy = dia->data->extents.top;
    }

    /* vertical lines ... */
    pos = origx + ceil((update->left - origx) / pwidth) * pwidth;
    while (pos <= update->right) {
      ddisplay_transform_coords(ddisp, pos,0,&x,&y);
      irenderer->draw_pixel_line(renderer,
                                 x, 0, x, height,
				 &dia->data->pagebreak_color);
      pos += pwidth;
    }
    /* Horizontal lines: */
    pos = origy + ceil((update->top - origy) / pheight) * pheight;
    while (pos <= update->bottom) {
      ddisplay_transform_coords(ddisp, 0,pos,&x,&y);
      irenderer->draw_pixel_line(renderer,
				 0, y, width, y,
				 &dia->data->pagebreak_color);
      pos += pheight;
    }
  }
}

void
snap_to_grid(DDisplay *ddisp, coord *x, coord *y)
{
  if (ddisp->grid.snap) {
    real width_x = ddisp->diagram->data->grid.width_x;
    real width_y = ddisp->diagram->data->grid.width_y;
    if (prefs.grid.dynamic) {
      calculate_dynamic_grid(ddisp, &width_x, &width_y);
    }
    *x = ROUND((*x) / width_x) * width_x;
    *y = ROUND((*y) / width_y) * width_y;
  }
}
