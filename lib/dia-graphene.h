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
 * Copyright 2020-2026 Zander Brown <zbrown@gnome.org>
 */

#pragma once

#include <graphene.h>

#include "geometry.h"

G_BEGIN_DECLS


static inline void
dia_matrix_from_graphene (DiaMatrix               *matrix,
                          const graphene_matrix_t *graphene)
{
  matrix->xx = graphene_matrix_get_value (graphene, 0, 0);
  matrix->yx = graphene_matrix_get_value (graphene, 0, 1);
  matrix->xy = graphene_matrix_get_value (graphene, 1, 0);
  matrix->yy = graphene_matrix_get_value (graphene, 1, 1);
  matrix->x0 = graphene_matrix_get_x_translation (graphene);
  matrix->y0 = graphene_matrix_get_y_translation (graphene);
}


static inline void
dia_matrix_to_graphene (const DiaMatrix   *matrix,
                        graphene_matrix_t *graphene)
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
dia_rectangle_from_graphene (DiaRectangle          *rect,
                             const graphene_rect_t *graphene)
{
  graphene_point_t p;

  graphene_rect_get_top_left (graphene, &p);
  rect->top = p.y;
  rect->left = p.x;

  graphene_rect_get_bottom_right (graphene, &p);
  rect->bottom = p.y;
  rect->right = p.x;
}


static inline void
dia_rectangle_to_graphene (const DiaRectangle *rect,
                           graphene_rect_t    *graphene)
{
  graphene_rect_t tmp;
  graphene_point_t bottom_right =
    GRAPHENE_POINT_INIT (rect->right, rect->bottom);

  graphene_rect_init (&tmp, rect->left, rect->top, 0.0, 0.0);
  graphene_rect_expand (&tmp, &bottom_right, graphene);
}

G_END_DECLS
