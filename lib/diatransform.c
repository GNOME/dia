/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diatransform.c - scaling indirection to connect display
 *                  and renderers
 * Copyright (C) 2002 Hans Breuer (refactoring)
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

#include "diatransform.h"


struct _DiaTransform {
  GObject parent_instance;
  /*< private >*/
  DiaRectangle *visible; /* pointer to original rectangle for transform_coords */
  double       *factor;  /* pointer to original factor for transform_length */
};

G_DEFINE_TYPE (DiaTransform, dia_transform, G_TYPE_OBJECT)


static void
dia_transform_finalize (GObject *object)
{
  /* don't free the fields, we don't own them */

  G_OBJECT_CLASS (dia_transform_parent_class)->finalize (object);
}


static void
dia_transform_class_init (DiaTransformClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = dia_transform_finalize;
}


static void
dia_transform_init (DiaTransform *self)
{
}


DiaTransform *
dia_transform_new (DiaRectangle *rect, double* zoom)
{
  DiaTransform *t = g_object_new (DIA_TYPE_TRANSFORM, NULL);
  t->visible = rect;
  t->factor = zoom;

  return t;
}


double
dia_transform_length (DiaTransform *t, double len)
{
  g_return_val_if_fail (DIA_IS_TRANSFORM (t), len);
  g_return_val_if_fail (t != NULL && *t->factor != 0.0, len);

  return (len * *(t->factor));
}


/* Takes pixel length and returns real length */
double
dia_untransform_length (DiaTransform *t, double len)
{
  g_return_val_if_fail (DIA_IS_TRANSFORM (t), len);
  g_return_val_if_fail (t != NULL && *t->factor != 0.0, len);

  return len / *(t->factor);
}


void
dia_transform_coords (DiaTransform *t,
                      double        x,
                      double        y,
                      int          *xi,
                      int          *yi)
{
  g_return_if_fail (DIA_IS_TRANSFORM (t));
  g_return_if_fail (t != NULL && t->factor != NULL);

  *xi = ROUND ( (x - t->visible->left) * *(t->factor));
  *yi = ROUND ( (y - t->visible->top) * *(t->factor));
}


void
dia_transform_coords_double (DiaTransform *t,
                             double        x,
                             double        y,
                             double       *xd,
                             double       *yd)
{
  g_return_if_fail (DIA_IS_TRANSFORM (t));
  g_return_if_fail (t != NULL && t->factor != NULL);

  *xd = ((x - t->visible->left) * *(t->factor));
  *yd = ((y - t->visible->top) * *(t->factor));
}
