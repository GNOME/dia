/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * BezierConn  Copyright (C) 1999 James Henstridge
 * BezierShape Copyright (C) 2000 James Henstridge
 *
 * Major restructuring to share more code
 * Copyright (C) 2012 Hans Breuer
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
#ifndef BEZIER_COMMON_H
#define BEZIER_COMMON_H

#include "diatypes.h"
#include "geometry.h" /* for real */

typedef enum {
  BEZ_CORNER_SYMMETRIC,
  BEZ_CORNER_SMOOTH,
  BEZ_CORNER_CUSP
} BezCornerType;


typedef struct _BezierCommon BezierCommon;

struct _BezierCommon {
  /*! The number of point in points */
  int            num_points;
  /*! Array of _BezPoint to operate on */
  BezPoint      *points;
  /*! The corner types for manual adjustment */
  BezCornerType *corner_types;
};

void beziercommon_set_points (BezierCommon *bezier, int num, const BezPoint *pts);
void beziercommon_copy (BezierCommon *from, BezierCommon *to);
int  beziercommon_closest_segment (BezierCommon *bezier, const Point *point, real line_width);

void bezier_draw_control_lines (int num_points, BezPoint *pts, DiaRenderer *renderer);

#endif