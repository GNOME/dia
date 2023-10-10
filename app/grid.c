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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <glib.h>

#include "grid.h"
#include "preferences.h"
#include "diainteractiverenderer.h"


/**
 * calculate_dynamic_grid:
 * @ddisp: the #DDisplay
 * @width_x: (out): the grid width x
 * @width_y: (out): the grid width y
 *
 * Calculate the width (in cm) of the gap between grid lines in dynamic
 * grid mode.
 */
static void
calculate_dynamic_grid (DDisplay *ddisp, double *width_x, double *width_y)
{
  real zoom = ddisplay_untransform_length(ddisp, 1.0);
  real ret, tmp;
  /* Twiddle zoom to make change-over appropriate */
  zoom *= 5;
  ret = pow(10, ceil(log10(zoom)));
  /* don't make it too small or huge (this is in pixels) */
  tmp = ddisplay_transform_length(ddisp, ret);
  if (tmp < 10.0)
    ret *= 2.0;
  else if (tmp > 35.0)
    ret /= 2.0;
  *width_x = ret;
  *width_y = ret;
}

gboolean
grid_step (DDisplay *ddisp, GtkOrientation orientation,
	   real *start, int *ipos, gboolean *is_major)
{
  real  length;
  real  pos;
  guint major_lines = ddisp->diagram->grid.major_lines;
  int   x, y;
  int   major_count = 1;

  /* length from the diagram settings - but always dynamic */
  calculate_dynamic_grid (ddisp, &length, &length);
  pos = ROUND(*start / length) * length;
  pos += length;
  if (major_lines) {
    major_count = ROUND (pos/length);
    if(major_count < 0) major_count -= major_lines * major_count;
    major_count %= major_lines;
  }
  ddisplay_transform_coords(ddisp,
			    orientation == GTK_ORIENTATION_HORIZONTAL ? pos : 0,
			    orientation == GTK_ORIENTATION_VERTICAL ? pos : 0,
			    &x, &y);

  *start = pos;
  *ipos = (orientation == GTK_ORIENTATION_HORIZONTAL ? x : y);
  *is_major = (major_count == 0);

  return TRUE;
}

static void
grid_draw_horizontal_lines(DDisplay *ddisp, DiaRectangle *update, real length)
{
  int x, y;
  real pos;
  int height, width;
  guint major_lines = ddisp->diagram->grid.major_lines;
  DiaRenderer *renderer = ddisp->renderer;
  int major_count = 0;

  pos = ceil ( update->top / length ) * length;
  ddisplay_transform_coords (ddisp, update->left, pos, &x, &y);
  ddisplay_transform_coords (ddisp, update->right, update->bottom, &width, &height);

  /*
    Explanatory note from Lawrence Withers (lwithers@users.sf.net):
    Think about it in terms of the maths and the modulo arithmetic; we
    have a number P such that P < 0, and we want to find a number Q
    such that Q >= 0, but (P % major_count) == (Q % major_count). To
    do this, we say Q = P - (P * major_count), which is guaranteed to
    give a correct answer if major_count > 0 (since the addition of
    anf multiple of major_count won't alter the modulo).
  */

  if (major_lines) {
    major_count = ROUND (pos/length);
    if (major_count < 0) major_count -= major_lines * major_count;
    major_count %= major_lines;
  }

  while (y < height) {
    if (major_lines) {
      if (major_count == 0) {
        dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
      } else {
        dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DOTTED,
                                    ddisplay_untransform_length (ddisp, 31));
      }
      major_count = (major_count + 1) % major_lines;
    }
    dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                              x,
                                              y,
                                              width,
                                              y,
                                              &ddisp->diagram->grid.colour);
    pos += length;
    ddisplay_transform_coords (ddisp, update->left, pos, &x, &y);
  }
}

static void
grid_draw_vertical_lines (DDisplay *ddisp, DiaRectangle *update, real length)
{
  int x = 0, y = 0;
  real pos;
  int height, width;
  guint major_lines = ddisp->diagram->grid.major_lines;
  DiaRenderer *renderer = ddisp->renderer;
  int major_count = 0;

  pos = ceil ( update->left / length ) * length;
  ddisplay_transform_coords (ddisp, update->right, update->bottom, &width, &height);

  if (major_lines) {
    major_count = ROUND (pos/length);
    if(major_count < 0) major_count -= major_lines * major_count;
    major_count %= major_lines;
  }

  while (x < width) {
    ddisplay_transform_coords (ddisp, pos, update->top, &x, &y);
    if (major_lines) {
      if (major_count == 0) {
        dia_renderer_set_linestyle (renderer,
                                    DIA_LINE_STYLE_SOLID,
                                    0.0);
      } else {
        dia_renderer_set_linestyle (renderer,
                                    DIA_LINE_STYLE_DOTTED,
                                    ddisplay_untransform_length (ddisp, 31));
      }
      major_count = (major_count + 1) % major_lines;
    }
    dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                              x,
                                              y,
                                              x,
                                              height,
                                              &ddisp->diagram->grid.colour);
    pos += length;
  }
}

static void
grid_draw_hex (DDisplay *ddisp, DiaRectangle *update, real length)
{
  real horiz_pos, vert_pos;
  int to_x, to_y, x, y;
  DiaRenderer *renderer = ddisp->renderer;

  /*
   * Let's not lock up the app trying to draw a invisible grid
   *
   * Assumption: We don't support increments smaller than 0.1 thus < 0.1 is 0
   * whilst allowing for floating point quirks
   */
  if (length < 0.1) {
    return;
  }

  /* First horizontal lines: */
  vert_pos = ceil( update->top / (length * sqrt(3)) ) * length * sqrt(3);
  while (vert_pos <= update->bottom) {
    horiz_pos = ceil( (update->left) / (3 * length) ) * length * 3 - length * 2.5;
    while (horiz_pos <= update->right) {
      ddisplay_transform_coords (ddisp, horiz_pos, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos + length, vert_pos, &to_x, &y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                y,
                                                &ddisp->diagram->grid.colour);
      horiz_pos += 3 * length;
    }

    vert_pos += sqrt(3) * length;
  }

  /*  Second horizontal lines: */
  vert_pos = ceil( update->top / (length * sqrt(3)) ) * length * sqrt(3) - 0.5 * sqrt(3) * length;
  while (vert_pos <= update->bottom) {
    horiz_pos = ceil( (update->left) / (3 * length) ) * length * 3 - length;
    while (horiz_pos <= update->right) {
      ddisplay_transform_coords (ddisp, horiz_pos, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos+length, vert_pos, &to_x, &y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                y,
                                                &ddisp->diagram->grid.colour);
      horiz_pos += 3 * length;
    }

    vert_pos += sqrt(3) * length;
  }

  /* First \'s and /'s */
  vert_pos = ceil ( update->top / (length * sqrt(3)) ) * length * sqrt(3) - length * sqrt(3);
  while (vert_pos <= update->bottom) {
    horiz_pos = ceil ( (update->left) / (3 * length) ) * length * 3 - length * 2.5;
    while (horiz_pos <= update->right) {
      ddisplay_transform_coords (ddisp, horiz_pos + length, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos + 1.5 * length, vert_pos + length * sqrt(3) * 0.5, &to_x, &to_y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                to_y,
                                                &ddisp->diagram->grid.colour);

      ddisplay_transform_coords (ddisp, horiz_pos, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos - 0.5 * length, vert_pos + length * sqrt(3) * 0.5, &to_x, &to_y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                to_y,
                                                &ddisp->diagram->grid.colour);
                                                horiz_pos += 3 * length;
    }

    vert_pos += sqrt(3) * length;
  }

  /*  Second \'s and /'s */
  vert_pos = ceil ( update->top / (length * sqrt(3)) ) * length * sqrt(3) - 0.5 * sqrt(3) * length;
  while (vert_pos <= update->bottom) {
    horiz_pos = ceil ( (update->left) / (3 * length) ) * length * 3 - length;
    while (horiz_pos <= update->right) {
      ddisplay_transform_coords (ddisp, horiz_pos, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos - 0.5 * length, vert_pos + 0.5 * sqrt(3) * length, &to_x, &to_y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                to_y,
                                                &ddisp->diagram->grid.colour);

      ddisplay_transform_coords (ddisp, horiz_pos + length, vert_pos, &x, &y);
      ddisplay_transform_coords (ddisp, horiz_pos + 1.5 * length, vert_pos + 0.5 * sqrt(3) * length, &to_x, &to_y);

      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x,
                                                y,
                                                to_x,
                                                to_y,
                                                &ddisp->diagram->grid.colour);
      horiz_pos += 3 * length;
    }

    vert_pos += sqrt(3) * length;
  }

}

void
grid_draw (DDisplay *ddisp, DiaRectangle *update)
{
  Grid *grid = &ddisp->grid;
  DiaRenderer *renderer = ddisp->renderer;

  if (grid->visible) {
    /* distance between visible grid lines */
    real width_x = ddisp->diagram->grid.width_x;
    real width_y = ddisp->diagram->grid.width_y;
    if (ddisp->diagram->grid.dynamic) {
      calculate_dynamic_grid(ddisp, &width_x, &width_y);
    } else {
      width_x = ddisp->diagram->grid.width_x *
                ddisp->diagram->grid.visible_x;
      width_y = ddisp->diagram->grid.width_y *
                ddisp->diagram->grid.visible_y;
    }

    dia_renderer_set_linewidth (renderer, 0.0);

    if (ddisp->diagram->grid.hex) {
      grid_draw_hex (ddisp, update, ddisp->diagram->grid.hex_size);
    } else {
      if (ddisplay_transform_length (ddisp, width_y) >= 2.0 &&
          ddisplay_transform_length (ddisp, width_x) >= 2.0) {
        /* Vertical lines: */
        grid_draw_vertical_lines (ddisp, update, width_x);
        /* Horizontal lines: */
        grid_draw_horizontal_lines (ddisp, update, width_y);
      }
    }
  }
}

void
pagebreak_draw (DDisplay *ddisp, DiaRectangle *update)
{
  DiaRenderer *renderer = ddisp->renderer;

  int width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  int height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  if (prefs.pagebreak.visible) {
    Diagram *dia = ddisp->diagram;
    real origx = 0, origy = 0, pos;
    real pwidth = dia->data->paper.width;
    real pheight = dia->data->paper.height;
    int x, y;

    dia_renderer_set_linewidth (renderer, 0.0);
    if (prefs.pagebreak.solid) {
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    } else {
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DOTTED,
                                  ddisplay_untransform_length (ddisp, 31));
    }

    if (dia->data->paper.fitto) {
      origx = dia->data->extents.left;
      origy = dia->data->extents.top;
    }

    /* vertical lines ... */
    pos = origx + ceil ((update->left - origx) / pwidth) * pwidth;
    while (pos <= update->right) {
      ddisplay_transform_coords (ddisp, pos, 0, &x, &y);
      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                x, 0, x, height,
                                                &dia->pagebreak_color);
      pos += pwidth;
    }
    /* Horizontal lines: */
    pos = origy + ceil ((update->top - origy) / pheight) * pheight;
    while (pos <= update->bottom) {
      ddisplay_transform_coords (ddisp, 0,pos,&x,&y);
      dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                0, y, width, y,
                                                &dia->pagebreak_color);
      pos += pheight;
    }
  }
}


void
guidelines_draw (DDisplay *ddisp, DiaRectangle *update)
{
  Diagram *dia = ddisp->diagram;
  DiaRenderer *renderer = ddisp->renderer;
  GList *list;
  real line_width;

  int width = dia_interactive_renderer_get_width_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));
  int height = dia_interactive_renderer_get_height_pixels (DIA_INTERACTIVE_RENDERER (ddisp->renderer));

  Color guideline_color = dia->guide_color;

  if (!dia) {
    return;
  }

  /* Make the line width a little bigger than hairline. */
  line_width = ddisplay_untransform_length (ddisp, 2);

  dia_renderer_set_linewidth (renderer, line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  if (ddisp->guides_visible) {
    list = dia->guides;
    while (list) {
      int x;
      int y;
      DiaGuide *guide = list->data;

      switch (guide->orientation) {
        case GTK_ORIENTATION_HORIZONTAL:
          ddisplay_transform_coords (ddisp, 0, guide->position, &x, &y);
          dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                    0, y, width, y, &guideline_color);
          break;

        case GTK_ORIENTATION_VERTICAL:
          ddisplay_transform_coords (ddisp, guide->position, 0, &x, &y);
          dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                    x, 0, x, height, &guideline_color);
          break;

        default:
          g_print ("Should not have reached this.\n");
          break;
      }

      list = g_list_next (list);
    }
  }

  /* NOTE: We can still drag new guides even if guide visibility
   * is set to off (like in GIMP). */
  if (ddisp->is_dragging_new_guideline) {
    int x;
    int y;

    switch (ddisp->dragged_new_guideline_orientation) {
      case GTK_ORIENTATION_HORIZONTAL:
        ddisplay_transform_coords (ddisp, 0, ddisp->dragged_new_guideline_position, &x, &y);
        dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                  0, y, width, y, &guideline_color);
        break;

      case GTK_ORIENTATION_VERTICAL:
        ddisplay_transform_coords (ddisp, ddisp->dragged_new_guideline_position, 0, &x, &y);
        dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                                  x, 0, x, height, &guideline_color);
        break;

      default:
        g_print ("Should not have reached this.\n");
        break;
    }
  }
}


/* For guides. */
#define  FUNSCALEX(s,x)   ((x) / (s)->zoom_factor)
#define  FUNSCALEY(s,y)   ((y) / (s)->zoom_factor)


void
snap_to_grid (DDisplay *ddisp, double *x, double *y)
{
  /* First snap to guides - only if they are visible and the setting is
   * turned on. */
  if (ddisp->guides_snap && ddisp->guides_visible) {
    DiaGuide *guide_h;
    DiaGuide *guide_v;
    const int snap_distance = prefs.snap_distance;

    guide_h = dia_diagram_pick_guide_h (ddisp->diagram, *x, *y,
                                        FUNSCALEX (ddisp, snap_distance),
                                        FUNSCALEY (ddisp, snap_distance));

    guide_v = dia_diagram_pick_guide_v (ddisp->diagram, *x, *y,
                                        FUNSCALEX (ddisp, snap_distance),
                                        FUNSCALEY (ddisp, snap_distance));

    if (guide_h) {
      *y = guide_h->position;
    }

    if (guide_v) {
      *x = guide_v->position;
    }

    /* Assume this takes priority over grid. */
    if (guide_h || guide_v) {
      return;
    }
  }

  /* Snap to grid. */
  if (ddisp->grid.snap) {
    if (ddisp->diagram->grid.hex) {
      real width_x = ddisp->diagram->grid.hex_size;
      real x_mod = (*x - 1 * width_x) - floor ((*x - 1 * width_x) / (3 * width_x)) * 3 * width_x;
      real y_mod = (*y - 0.25 * sqrt (3) * width_x) -
        floor ((*y - 0.25 * sqrt (3) * width_x) / (sqrt (3) * width_x)) * sqrt (3) * width_x;

      if ( x_mod < (1.5 * width_x) ) {
        if ( y_mod < 0.5 * sqrt (3) * width_x ) {
          *x = floor ((*x + 0.5 * width_x) / (3 * width_x)) * 3 * width_x + 2 * width_x;
          *y = floor ((*y - 0.25 * sqrt (3) * width_x) / (sqrt (3)*width_x)) * sqrt (3) * width_x + 0.5 * sqrt (3) * width_x;
        } else {
          *x = floor ((*x + 0.5 * width_x) / (3 * width_x)) * 3 * width_x + 1.5 * width_x;
          *y = floor ((*y - 0.25 * sqrt (3) * width_x) / (sqrt (3)*width_x)) * sqrt (3) * width_x + sqrt (3) * width_x;
        }
      } else {
        if ( y_mod < 0.5 * sqrt (3) * width_x ) {
          *x = floor ((*x + 0.5 * width_x) / (3 * width_x)) * 3 * width_x ;
          *y = floor ((*y - 0.25 * sqrt (3) * width_x) / (sqrt (3) * width_x)) * sqrt (3) * width_x + 0.5 * sqrt (3) * width_x;
        } else {
          *x = floor ((*x + 0.5 * width_x) / (3 * width_x)) * 3 * width_x + 0.5 * width_x;
          *y = floor ((*y - 0.25 * sqrt (3) * width_x) / (sqrt (3) * width_x)) * sqrt (3) * width_x + sqrt (3) * width_x;
        }
      }
    } else {
      real width_x = ddisp->diagram->grid.width_x;
      real width_y = ddisp->diagram->grid.width_y;
      if (ddisp->diagram->grid.dynamic) {
          calculate_dynamic_grid (ddisp, &width_x, &width_y);
      }
      *x = ROUND ((*x) / width_x) * width_x;
      *y = ROUND ((*y) / width_y) * width_y;
    }
  }
}
