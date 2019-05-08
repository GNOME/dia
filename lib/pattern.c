/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * pattern.h - implementation of DiaPattern object
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
#include <config.h> 
#include "diatypes.h"
#include "pattern.h"
#include "color.h"
#include <glib-object.h>

typedef struct _DiaPatternClass DiaPatternClass;
#define DIA_TYPE_PATTERN (_dia_pattern_get_type ())
static GType _dia_pattern_get_type (void) G_GNUC_CONST;

typedef struct _ColorStop
{
  Color color;
  real  offset;
} ColorStop;

struct _DiaPattern
{
  GObject parent_instance;
  /*< private >*/
  DiaPatternType type;
  Point          start;
  real           radius;
  Point          other;
  GArray        *stops;

  guint          flags;
};

struct _DiaPatternClass
{
  GObjectClass parent_class;
};

G_DEFINE_TYPE (DiaPattern, _dia_pattern, G_TYPE_OBJECT);
static gpointer parent_class = NULL;

static void
_dia_pattern_finalize(GObject *object)
{
  DiaPattern *pattern = (DiaPattern *)object;

  g_array_free (pattern->stops, TRUE);

  G_OBJECT_CLASS (parent_class)->finalize (object);
};

static void
_dia_pattern_class_init(DiaPatternClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_peek_parent (klass);
  object_class->finalize = _dia_pattern_finalize;
}
static void
_dia_pattern_init(DiaPattern *self)
{
  self->other.x = 1.0;
  self->other.y = 0.0;
  self->radius  = 0.5;
  self->stops = g_array_new (FALSE, FALSE, sizeof(ColorStop));
}

/*!
 * \brief Create a new gradient object
 *
 * Create a default initialized pattern object. For gradients at least
 * dia_pattern_add_color() should be called on it.
 *
 * \ingroup DiaPattern
 */
DiaPattern *
dia_pattern_new (DiaPatternType pt, guint flags, real x, real y)
{
  DiaPattern *pat = g_object_new (DIA_TYPE_PATTERN, NULL);
  pat->type = pt;
  pat->start.x = x;
  pat->start.y = y;
  pat->flags = flags;

  return pat;
}

/*!
 * \brief Set the other point of the pattern
 *
 * The meaning of the point depends on the type of the pattern.
 * For linear gradient it is the second point giving the direction.
 * With a radial gradient it is the focal point.
 *
 * \ingroup DiaPattern
 */
void
dia_pattern_set_point (DiaPattern *self, real x, real y)
{
  self->other.x = x;
  self->other.y = y;
  /* with radial we have to ensure the point is in circle */
  if (self->type == DIA_RADIAL_GRADIENT) {
    real dist = distance_ellipse_point (&self->start, self->radius*2, self->radius*2, 0.0, &self->other);
    if (dist > 0) {
      /* If the point defined by �fx� and �fy� lies outside the circle defined
       * by �cx�, �cy� and �r�, then the user agent shall set the focal point to
       * the intersection of the line from (�cx�, �cy�) to (�fx�, �fy�) with the
       * circle defined by �cx�, �cy� and �r�
       */
      Point p1 = self->start;
      Point p2 = self->other;
      point_sub (&p2, &p1);
      point_normalize (&p2);
      point_add_scaled (&p1, &p2, self->radius);
      self->other = p1;
    }
  }
}
/*!
 * \brief Set the radius of a radial gradient
 * \ingroup DiaPattern
 */
void
dia_pattern_set_radius (DiaPattern *self, real r)
{
  g_return_if_fail (self != NULL);
  self->radius = r;
}
/*!
 * \brief Get construction parameters of the pattern
 * \ingroup DiaPattern
 */
void
dia_pattern_get_settings (DiaPattern *self, DiaPatternType *type, guint *flags)
{
  g_return_if_fail (self != NULL);

  if (type)
    *type = self->type;
  if (flags)
    *flags = self->flags;
}
/*!
 * \brief Get both points of the pattern definition
 * The meaning of p2 is gradient type specific. 
 * \ingroup DiaPattern
 */
void
dia_pattern_get_points (DiaPattern *self, Point *p1, Point *p2)
{
  g_return_if_fail (self != NULL);

  if (p1)
    *p1 = self->start;
  if (p2)
    *p2 = self->other;
}
/*!
 * \brief Get the radius of a radial gradient
 * \ingroup DiaPattern
 */
void
dia_pattern_get_radius (DiaPattern *self, real *r)
{
  g_return_if_fail (self != NULL);

  if (r)
    *r = self->radius;
}

/*!
 * \brief Add a color stop to the gradient
 * \ingroup DiaPattern
 */
void
dia_pattern_add_color (DiaPattern *self, real pos, const Color *color)
{
  ColorStop stop;
  real former = 0.0;

  g_return_if_fail (self != NULL && color != NULL);
  stop.color  = *color;
  if (self->stops->len > 0)
    former = g_array_index (self->stops, ColorStop, self->stops->len - 1).offset;
  stop.offset = MAX(pos, former);
  g_array_append_val (self->stops, stop);
}

/*!
 * \brief Inherit color stops from one gradient to the other
 * \ingroup DiaPattern
 */
void
dia_pattern_set_pattern (DiaPattern *self, DiaPattern *pat)
{
  gsize i;
 
  g_return_if_fail (self != NULL && pat != NULL);

  for (i = 0; i < pat->stops->len; ++i) {
    ColorStop *stop = &g_array_index (pat->stops, ColorStop, i);
    g_array_append_val (self->stops, *stop);
  }
}

/*!
 * \brief Get a good color to emulate the gradient with a solid fill
 * \ingroup DiaPattern
 */
void
dia_pattern_get_fallback_color (DiaPattern *self, Color *color)
{
  g_return_if_fail (self != NULL && color != NULL);
  if (self->stops->len > 0)
    *color = g_array_index (self->stops, ColorStop, 0).color;
  else
    *color = color_black;
}

/*!
 * \brief Call the given callback for each color stop
 * \ingroup DiaPattern
 */
void
dia_pattern_foreach (DiaPattern *pattern, DiaColorStopFunc fn, gpointer user_data)
{
  gsize i;
  g_return_if_fail (pattern != NULL && fn != NULL);

  for (i = 0; i < pattern->stops->len; ++i) {
    ColorStop *stop = &g_array_index (pattern->stops, ColorStop, i);

    (*fn)(stop->offset, &stop->color, user_data);
  }
}
