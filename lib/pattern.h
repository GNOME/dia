/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pattern.h - interface definition for DiaPattern object
 * Copyright (C) 2013 Hans Breuer
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
#ifndef PATTERN_H
#define PATTERN_H
 
#include "diatypes.h"
#include "geometry.h"
#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  DIA_LINEAR_GRADIENT = 1,
  DIA_RADIAL_GRADIENT
} DiaPatternType;

typedef enum
{
  DIA_PATTERN_USER_SPACE     = (1<<0),
  DIA_PATTERN_EXTEND_REPEAT  = (1<<1),
  DIA_PATTERN_EXTEND_REFLECT = (1<<2),
  DIA_PATTERN_EXTEND_PAD     = (1<<3)
} DiaPatternFlags;

DiaPattern *dia_pattern_new (DiaPatternType pt, guint flags, real x, real y);

void dia_pattern_set_radius (DiaPattern *self, real r);
void dia_pattern_set_point (DiaPattern *pattern, real x, real y); 
void dia_pattern_add_color (DiaPattern *pattern, real pos, const Color *stop);
void dia_pattern_set_pattern (DiaPattern *pattern, DiaPattern *pat);

void dia_pattern_get_fallback_color (DiaPattern *pattern, Color *color);
void dia_pattern_get_points (DiaPattern *pattern, Point *p1, Point *p2);
void dia_pattern_get_radius (DiaPattern *pattern, real *r);
void dia_pattern_get_settings (DiaPattern *pattern, DiaPatternType *type, guint *flags);

typedef gboolean (*DiaColorStopFunc) (real ofs, const Color *col, gpointer user_data);
void dia_pattern_foreach (DiaPattern *pattern, DiaColorStopFunc fn, gpointer user_data);

G_END_DECLS

#endif /* PATTERN_H */
