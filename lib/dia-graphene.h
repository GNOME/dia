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
