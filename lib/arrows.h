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
#include "render.h"

/* NOTE: Add new arrow types at the end, or the enums
   will change order leading to file incompatibilities. */
   
typedef enum {
  ARROW_NONE,
  ARROW_LINES,
  ARROW_HOLLOW_TRIANGLE,
  ARROW_FILLED_TRIANGLE,
  ARROW_HOLLOW_DIAMOND,
  ARROW_FILLED_DIAMOND,
  ARROW_HALF_HEAD,
  ARROW_SLASHED_CROSS,
  ARROW_FILLED_ELLIPSE,
  ARROW_HOLLOW_ELLIPSE
} ArrowType;

typedef struct {
  ArrowType type;
  real length;
  real width;
} Arrow;

void arrow_draw(Renderer *renderer, ArrowType type,
		Point *to, Point *from,
		real length, real width, real linewidth,
		Color *fg_color, Color *bg_color);

#endif /* ARROWS_H */
