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

#pragma once

/*!
 * \defgroup ObjectBBox Bounding box
 * \brief Calculations considering line width, caps and joins
 * \ingroup ObjectParts
 */

#include <graphene.h>

#include "diatypes.h"
#include "geometry.h"

G_BEGIN_DECLS

/**
 * PolyBBExtras:
 *
 * Polygon/Polyline bounding box extras
 */
struct _PolyBBExtras {
  float start_long, start_trans;
  float middle_trans;
  float end_long, end_trans;
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


void   bicubicbezier2D_bbox   (const graphene_vec2_t *p0,
                               const Point           *p1,
                               const Point           *p2,
                               const Point           *p3,
                               const PolyBBExtras    *extra,
                               graphene_rect_t       *rect);
void   line_bbox              (const graphene_vec2_t *p1,
                               const graphene_vec2_t *p2,
                               const LineBBExtras    *extra,
                               graphene_rect_t       *rect);
void   rectangle_bbox         (const graphene_rect_t *rin,
                               const ElementBBExtras *extra,
                               graphene_rect_t       *rout);
void   ellipse_bbox           (const Point           *centre,
                               float                  width,
                               float                  height,
                               const ElementBBExtras *extra,
                               graphene_rect_t       *rect);
void   polyline_bbox          (const Point           *pts,
                               int                    numpoints,
                               const PolyBBExtras    *extra,
                               gboolean               closed,
                               graphene_rect_t       *rect);
void   polybezier_bbox        (const BezPoint        *pts,
                               int                    numpoints,
                               const PolyBBExtras    *extra,
                               gboolean               closed,
                               graphene_rect_t       *rect);
/* helpers for bezier curve calculation */
void   bernstein_develop      (const double           p[4],
                               double                *A,
                               double                *B,
                               double                *C,
                               double                *D);
double bezier_eval            (const double           p[4],
                               double                 u);
double bezier_eval_tangent    (const double           p[4],
                               double                 u);

G_END_DECLS
