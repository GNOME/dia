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
 */

#include "dia-guide.h"

G_DEFINE_BOXED_TYPE (DiaGuide, dia_guide, dia_guide_copy, dia_guide_free)


DiaGuide *
dia_guide_copy (DiaGuide * self)
{
  DiaGuide *new;


  g_return_val_if_fail (self != NULL, NULL);

  new = g_new0 (DiaGuide, 1);

  new->orientation = self->orientation;
  new->position = self->position;

  return new;
}


void
dia_guide_free (DiaGuide * self)
{
  g_clear_pointer (&self, g_free);
}


DiaGuide *
dia_guide_new (GtkOrientation orientation,
               double         position)
{
  DiaGuide *self = g_new0 (DiaGuide, 1);

  self->orientation = orientation;
  self->position = position;

  return self;
}
