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

/*!
 * \defgroup ObjectBBox Bounding box
 * \brief Calculations considering line width, caps and joins
 * \ingroup ObjectParts
 */

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "diatypes.h"
#include "geometry.h"

/*!
 * \brief Polygon/Polyline bounding box extras
 * \ingroup ObjectBBox
 */
struct _PolyBBExtras {
  real start_long, start_trans;
  real middle_trans;
  real end_long, end_trans;
};

/*!
 * \brief Line bounding box extras
 * \ingroup ObjectBBox
 */
struct _LineBBExtras {
  real start_long, start_trans;
  real end_long, end_trans;
};

/*!
 * \brief Element bounding box extras
 * \ingroup ObjectBBox
 */
struct _ElementBBExtras {
  real border_trans;
};

void bicubicbezier2D_bbox(const Point *p0,const Point *p1,
                          const Point *p2,const Point *p3,
                          const PolyBBExtras *extra,
                          Rectangle *rect);

/*!
 * \brief Bounding box calculation for a straight line
 * The calcualtion includes line width and arrwos with the right extra
 * \ingroup ObjectBBox
 */
void line_bbox(const Point *p1, const Point *p2,
               const LineBBExtras *extra,
               Rectangle *rect);

/*!
 * \brief Bounding box calculation for a rectangle
 * The calcualtion includes line width with the right extra
 * \ingroup ObjectBBox
 */
void rectangle_bbox(const Rectangle *rin,
                    const ElementBBExtras *extra,
                    Rectangle *rout);

/*!
 * \brief Bounding box calculation for an ellipse
 * The calcualtion includes line width with the right extra
 * \ingroup ObjectBBox
 */
void ellipse_bbox(const Point *centre, real width, real height,
                  const ElementBBExtras *extra,
                  Rectangle *rect);
/*!
 * \brief Bounding box calculation for a polyline
 * The calcualtion includes line width and arrwos with the right extra
 * \ingroup ObjectBBox
 */
void polyline_bbox(const Point *pts, int numpoints,
                   const PolyBBExtras *extra, gboolean closed,
                   Rectangle *rect);
/*!
 * \brief Bounding box calculation for a bezier
 * The calcualtion includes line width and arrwos with the right extra
 * \ingroup ObjectBBox
 */
void polybezier_bbox(const BezPoint *pts, int numpoints,
                     const PolyBBExtras *extra, gboolean closed,
                     Rectangle *rect);

/* helpers for bezier curve calculation */
void bernstein_develop(const real p[4],real *A,real *B,real *C,real *D);
real bezier_eval(const real p[4],real u);
real bezier_eval_tangent(const real p[4],real u);

#endif /* BOUNDINGBOX_H */
