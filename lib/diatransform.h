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
 */

#pragma once

#include <glib-object.h>

#include "geometry.h"

G_BEGIN_DECLS

#define DIA_TYPE_TRANSFORM dia_transform_get_type ()
G_DECLARE_FINAL_TYPE (DiaTransform, dia_transform, DIA, TRANSFORM, GObject)

DiaTransform *dia_transform_new           (DiaRectangle *rect,
                                           double       *zoom);
double        dia_transform_length        (DiaTransform *transform,
                                           double        len);
void          dia_transform_coords        (DiaTransform *transform,
                                           double        x,
                                           double        y,
                                           int          *xi,
                                           int          *yi);
void          dia_transform_coords_double (DiaTransform *transform,
                                           double        x,
                                           double        y,
                                           double       *xd,
                                           double       *yd);
double        dia_untransform_length      (DiaTransform *t,
                                           double        len);

G_END_DECLS
