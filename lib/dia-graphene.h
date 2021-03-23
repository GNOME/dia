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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright © 2020 Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <graphene.h>

#include "geometry.h"

G_BEGIN_DECLS

static inline void
dia_matrix_from_graphene (DiaMatrix         *matrix,
                          graphene_matrix_t *graphene)
{
  matrix->xx = graphene_matrix_get_value (graphene, 0, 0);
  matrix->yx = graphene_matrix_get_value (graphene, 0, 1);
  matrix->xy = graphene_matrix_get_value (graphene, 1, 0);
  matrix->yy = graphene_matrix_get_value (graphene, 1, 1);
  matrix->x0 = graphene_matrix_get_x_translation (graphene);
  matrix->y0 = graphene_matrix_get_y_translation (graphene);
}


static inline void
dia_graphene_from_matrix (graphene_matrix_t *graphene,
                          const DiaMatrix   *matrix)
{
  graphene_matrix_init_from_2d (graphene,
                                matrix->xx,
                                matrix->yx,
                                matrix->xy,
                                matrix->yy,
                                matrix->x0,
                                matrix->y0);
}


static inline void
dia_graphene_to_rectangle (const graphene_rect_t *rect,
                           DiaRectangle          *bounds)
{
  graphene_point_t point;

  graphene_rect_get_top_left (rect, &point);
  bounds->top = point.y;
  bounds->left = point.x;

  graphene_rect_get_bottom_right (rect, &point);
  bounds->bottom = point.y;
  bounds->right = point.x;
}


static inline void
dia_rectangle_to_graphene (const DiaRectangle *bounds,
                           graphene_rect_t    *rect)
{
  graphene_rect_init (rect,
                      bounds->left,
                      bounds->top,
                      bounds->right - bounds->left,
                      bounds->bottom - bounds->top);
}


static inline void
dia_graphene_dump_rect (const graphene_rect_t *self)
{
  graphene_point_t point;

  graphene_rect_get_top_left (self, &point);
  g_print (" \\ %10f\n", point.y);
  g_print (" %10f\n", point.x);
  g_print ("   %10f X %10f\n",
           graphene_rect_get_width (self),
           graphene_rect_get_height (self));

  graphene_rect_get_bottom_right (self, &point);
  g_print ("                \\ %10f\n", point.x);
  g_print ("                %10f\n", point.y);
}


static inline void
dia_graphene_to_point (const graphene_point_t *src,
                       Point                  *dest)
{
  dest->x = src->x;
  dest->y = src->y;
}


static inline void
dia_point_to_graphene (const Point      *src,
                       graphene_point_t *dest)
{
  dest->x = src->x;
  dest->y = src->y;
}


static inline void
dia_point_to_vec2 (const Point     *src,
                   graphene_vec2_t *dest)
{
  graphene_vec2_init (dest, src->x, src->y);
}


static inline void
dia_vec2_to_point (const graphene_vec2_t *src,
                   Point                 *dest)
{
  dest->x = graphene_vec2_get_x (src);
  dest->y = graphene_vec2_get_y (src);
}


/**
 * dia_vec2_add_scaled:
 * @a: the first #graphene_vec2_t
 * @b: the second #graphene_vec2_t
 * @scale: scale to apply to @b
 * @res: (out caller-allocates): where to store the result
 *
 * See graphene_vec2_add()/graphene_vec2_scale()
 */
static inline void
dia_vec2_add_scaled (const graphene_vec2_t *a,
                     const graphene_vec2_t *b,
                     float                  scale,
                     graphene_vec2_t       *res)
{
  graphene_vec2_t tmp;
  graphene_vec2_scale (b, scale, &tmp);
  graphene_vec2_add (a, &tmp, res);
}


static inline void
dia_vec2_perpendicular (const graphene_vec2_t *src, graphene_vec2_t *res)
{
  float y = graphene_vec2_get_x (src);
  float x = -graphene_vec2_get_y (src);

  graphene_vec2_init (res, x, y);
}


G_END_DECLS
