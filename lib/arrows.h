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
#ifndef ARROWS_H
#define ARROWS_H

#include "geometry.h"

/* NOTE: Add new arrow types at the end, or the enums
   will change order leading to file incompatibilities. */

/* Comments in curly braces mention ISO 10303-AP201 names */

typedef enum {
  ARROW_NONE,
  ARROW_LINES,             /* {open arrow} */
  ARROW_HOLLOW_TRIANGLE,   /* {blanked arrow} */
  ARROW_FILLED_TRIANGLE,   /* {filled arrow} */
  ARROW_HOLLOW_DIAMOND,    
  ARROW_FILLED_DIAMOND,
  ARROW_HALF_HEAD,
  ARROW_SLASHED_CROSS,     /* Vertical + diagonal line */
  ARROW_FILLED_ELLIPSE,
  ARROW_HOLLOW_ELLIPSE,
  ARROW_DOUBLE_HOLLOW_TRIANGLE,
  ARROW_DOUBLE_FILLED_TRIANGLE,
  ARROW_UNFILLED_TRIANGLE,       /* {unfilled arrow} */
  ARROW_FILLED_DOT,              /* {filled dot} Ellipse + vertical line */ 
  ARROW_DIMENSION_ORIGIN,        /* {dimension origin} Ellipse + vert line */ 
  ARROW_BLANKED_DOT,             /* {blanked dot} Empty ellipse + vert line */
  ARROW_FILLED_BOX,              /* {filled box} Box + vertical line */
  ARROW_BLANKED_BOX,             /* {blanked box} Box + vertical line */
  ARROW_SLASH_ARROW,             /* {slash arrow} Vertical + diagonal line*/
  ARROW_INTEGRAL_SYMBOL,         /* {integral symbol} Vertical + integral */
  ARROW_CROW_FOOT,
  ARROW_CROSS                    /* Vertical line */
} ArrowType;

typedef struct {
  ArrowType type;
  real length;
  real width;
} Arrow;

/* Can't include it earlier as it uses the Arrow def */
#include "render.h"

void arrow_draw(Renderer *renderer, ArrowType type,
		Point *to, Point *from,
		real length, real width, real linewidth,
		Color *fg_color, Color *bg_color);

void
calculate_arrow_point(Arrow *arrow, Point *to, Point *from, Point *move,
		      real linewidth);

/* Transforms 'start' to be at the back end of the arrow, and puts the
 * tip of the arrow into 'arrowtip'.
 */
void arrow_transform_points(Arrow *arrow, Point *start, Point *to,
                       int linewidth, Point *arrowtip);

#endif /* ARROWS_H */
