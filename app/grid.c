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

void
grid_draw(DDisplay *ddisp, Rectangle *update)
{
  Grid *grid = &ddisp->grid;
  Renderer *renderer = ddisp->renderer;
  int width = ddisp->renderer->pixel_width;
  int height = ddisp->renderer->pixel_height;
  /* distance between visible grid lines */
  real width_x = ddisp->diagram->data->grid.width_x *
    ddisp->diagram->data->grid.visible_x;
  real width_y = ddisp->diagram->data->grid.width_y *
    ddisp->diagram->data->grid.visible_y;

  if ( (ddisplay_transform_length(ddisp, width_x) <= 1.0) ||
       (ddisplay_transform_length(ddisp, width_y) <= 1.0) ) {
    return;
  }
  
  if (grid->visible) {
    real pos;
    int x,y;

    (renderer->ops->set_linewidth)(renderer, 0.0);
    if (prefs.grid.solid) {
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_SOLID);
    } else {
      (renderer->ops->set_dashlength)(renderer,
      		    ddisplay_untransform_length(ddisp, 31));
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_DOTTED);
    }
    
    /* Vertical lines: */
    pos = ceil( update->left / width_x ) * width_x;
    while (pos <= update->right) {
      ddisplay_transform_coords(ddisp, pos,0,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   x, 0,
						   x, height,
						   &prefs.grid.colour);
      pos += width_x;
    }

    /* Horizontal lines: */
    pos = ceil( update->top / width_y ) * width_y;
    while (pos <= update->bottom) {
      ddisplay_transform_coords(ddisp, 0,pos,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   0, y,
						   width, y,
						   &prefs.grid.colour);
      pos += width_y;
    }
  }

  if (prefs.pagebreak.visible) {
    Diagram *dia = ddisp->diagram;
    real origx = 0, origy = 0, pos;
    real pwidth = dia->data->paper.width;
    real pheight = dia->data->paper.height;
    int x,y;

    (renderer->ops->set_linewidth)(renderer, 0.0);
    (renderer->ops->set_dashlength)(renderer,
				    ddisplay_untransform_length(ddisp, 31));
    if (prefs.pagebreak.solid)
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_SOLID);
    else
      (renderer->ops->set_linestyle)(renderer, LINESTYLE_DOTTED);

    if (dia->data->paper.fitto) {
      origx = dia->data->extents.left;
      origy = dia->data->extents.top;
    }

    /* vertical lines ... */
    pos = origx + ceil((update->left - origx) / pwidth) * pwidth;
    while (pos <= update->right) {
      ddisplay_transform_coords(ddisp, pos,0,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   x, 0,
						   x, height,
						   &prefs.pagebreak.colour);
      pos += pwidth;
    }
    /* Horizontal lines: */
    pos = origy + ceil((update->top - origy) / pheight) * pheight;
    while (pos <= update->bottom) {
      ddisplay_transform_coords(ddisp, 0,pos,&x,&y);
      (renderer->interactive_ops->draw_pixel_line)(renderer,
						   0, y,
						   width, y,
						   &prefs.pagebreak.colour);
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

    *x = ROUND((*x) / width_x) * width_x;
    *y = ROUND((*y) / width_y) * width_y;
  }
}
