/* Dia -- an diagram creation/manipulation program
 * Support for computing bounding boxes 
 * Copyright (C) 2001 Cyrille Chepelov
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

/** \file boundingbox.h Boundingbox calculation (helpers) */

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "diatypes.h"
#include "geometry.h"

struct _PolyBBExtras {
  real start_long, start_trans;
  real middle_trans;
  real end_long, end_trans;
};

struct _LineBBExtras {
  real start_long, start_trans;
  real end_long, end_trans;
};

struct _ElementBBExtras {
  real border_trans;
};

void bicubicbezier2D_bbox(const Point *p0,const Point *p1,
                          const Point *p2,const Point *p3,
                          const PolyBBExtras *extra,
                          Rectangle *rect);

void line_bbox(const Point *p1, const Point *p2,
               const LineBBExtras *extra,
               Rectangle *rect);

void rectangle_bbox(const Rectangle *rin,
                    const ElementBBExtras *extra,
                    Rectangle *rout);

void circle_bbox(const Point *centre, real radius, 
                 Rectangle *rect);

void ellipse_bbox(const Point *centre, real width, real height,
                  const ElementBBExtras *extra,
                  Rectangle *rect);
void polyline_bbox(const Point *pts, int numpoints,
                   const PolyBBExtras *extra, gboolean closed,
                   Rectangle *rect);
void polybezier_bbox(const BezPoint *pts, int numpoints,
                     const PolyBBExtras *extra, gboolean closed,
                     Rectangle *rect);

#endif /* BOUNDINGBOX_H */
