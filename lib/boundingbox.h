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
  double start_long, start_trans;
  double middle_trans;
  double end_long, end_trans;
};

/*!
 * \brief Line bounding box extras
 * \ingroup ObjectBBox
 */
struct _LineBBExtras {
  double start_long, start_trans;
  double end_long, end_trans;
};

/*!
 * \brief Element bounding box extras
 * \ingroup ObjectBBox
 */
struct _ElementBBExtras {
  double border_trans;
};

void bicubicbezier2D_bbox(const Point *p0,const Point *p1,
                          const Point *p2,const Point *p3,
                          const PolyBBExtras *extra,
                          DiaRectangle       *rect);

/*!
 * \brief Bounding box calculation for a straight line
 * The calculation includes line width and arrows with the right extra
 * \ingroup ObjectBBox
 */
void line_bbox(const Point *p1, const Point *p2,
               const LineBBExtras *extra,
               DiaRectangle       *rect);

/*!
 * \brief Bounding box calculation for a rectangle
 * The calculation includes line width with the right extra
 * \ingroup ObjectBBox
 */
void rectangle_bbox(const DiaRectangle    *rin,
                    const ElementBBExtras *extra,
                    DiaRectangle          *rout);

/*!
 * \brief Bounding box calculation for an ellipse
 * The calculation includes line width with the right extra
 * \ingroup ObjectBBox
 */
void ellipse_bbox (const Point           *centre,
                   double                 width,
                   double                 height,
                   const ElementBBExtras *extra,
                   DiaRectangle          *rect);
/*!
 * \brief Bounding box calculation for a polyline
 * The calculation includes line width and arrows with the right extra
 * \ingroup ObjectBBox
 */
void polyline_bbox(const Point *pts, int numpoints,
                   const PolyBBExtras *extra, gboolean closed,
                   DiaRectangle       *rect);
/*!
 * \brief Bounding box calculation for a bezier
 * The calculation includes line width and arrows with the right extra
 * \ingroup ObjectBBox
 */
void polybezier_bbox(const BezPoint *pts, int numpoints,
                     const PolyBBExtras *extra, gboolean closed,
                     DiaRectangle       *rect);

/* helpers for bezier curve calculation */
void   bernstein_develop   (const double  p[4],
                            double       *A,
                            double       *B,
                            double       *C,
                            double       *D);
double bezier_eval         (const double  p[4],
                            double        u);
double bezier_eval_tangent (const double  p[4],
                            double        u);

#endif /* BOUNDINGBOX_H */
