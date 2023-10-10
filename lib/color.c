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

#include <config.h>

#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "color.h"


G_DEFINE_BOXED_TYPE (Color, dia_colour, dia_colour_copy, dia_colour_free)


/**
 * dia_colour_copy:
 * @self: source #Color
 *
 * Allocate a new color equal to @self
 *
 * Since: 0.98
 */
Color *
dia_colour_copy (Color *self)
{
  Color *new;

  g_return_val_if_fail (self != NULL, NULL);

  new = g_new0 (Color, 1);

  new->red = self->red;
  new->green = self->green;
  new->blue = self->blue;
  new->alpha = self->alpha;

  return new;
}


/**
 * dia_colour_free:
 * @self: the #Color
 *
 * Free a colour object
 *
 * Since: 0.98
 */
void
dia_colour_free (Color *self)
{
  g_clear_pointer (&self, g_free);
}


/**
 * dia_colour_parse:
 * @self: the #Color
 * @str: string to parse
 *
 * Set @self according to @str
 *
 * @str can be of the form
 *
 * \#RRGGBB or \#RRGGBBAA
 *
 * Since: 0.98
 */
void
dia_colour_parse (Color       *self,
                  const char  *str)
{
  int r = 0, g = 0, b = 0, a = 255;

  switch (strlen (str)) {
    case 7:
      if (sscanf (str, "#%2x%2x%2x", &r, &g, &b) != 3) {
        g_return_if_reached ();
      }
      break;
    case 9:
      if (sscanf (str, "#%2x%2x%2x%2x", &r, &g, &b, &a) != 4) {
        g_return_if_reached ();
      }
      break;
    default:
      g_return_if_reached ();
  }

  self->red = r / 255.0;
  self->green = g / 255.0;
  self->blue = b / 255.0;
  self->alpha = a / 255.0;
}


/**
 * dia_colour_to_string:
 * @self: the #Color
 *
 * Generate a string representation of @self
 *
 * Since: 0.98
 *
 * Returns: @self as \#RRGGBBAA
 */
char *
dia_colour_to_string (Color *self)
{
  return g_strdup_printf ("#%02X%02X%02X%02X",
                          (guint) (CLAMP (self->red, 0.0, 1.0) * 255),
                          (guint) (CLAMP (self->green, 0.0, 1.0) * 255),
                          (guint) (CLAMP (self->blue, 0.0, 1.0) * 255),
                          (guint) (CLAMP (self->alpha, 0.0, 1.0) * 255));
}


/**
 * color_new_rgb:
 * @r: Red component (0 <= r <= 1)
 * @g: Green component (0 <= g <= 1)
 * @b: Blue component (0 <= b <= 1)
 *
 * Allocate a new color object with the given values.
 * Initializes alpha component to 1.0
 *
 * Returns: A newly allocated color object.  This should be freed after use.
 */
Color *
color_new_rgb (float r, float g, float b)
{
  Color *col = g_new (Color, 1);
  col->red = r;
  col->green = g;
  col->blue = b;
  col->alpha = 1.0;
  return col;
}

/**
 * color_new_rgba:
 * @r: Red component (0 <= r <= 1)
 * @g: Green component (0 <= g <= 1)
 * @b: Blue component (0 <= b <= 1)
 * @alpha: Alpha component (0 <= alpha <= 1)
 *
 * Allocate a new color object with the given values.
 *
 * Returns: A newly allocated color object.  This should be freed after use.
 */
Color *
color_new_rgba (float r, float g, float b, float alpha)
{
  Color *col = g_new (Color, 1);
  col->red = r;
  col->green = g;
  col->blue = b;
  col->alpha = alpha;
  return col;
}

/**
 * color_convert:
 * @color: A #Color object. This will not be kept by this function.
 * @gdkcolor: (out): GDK RGBA object to fill in.
 *
 * Convert a Dia color object to GDK style.
 */
void
color_convert (const Color *color, GdkRGBA *gdkcolor)
{
  gdkcolor->red = color->red;
  gdkcolor->green = color->green;
  gdkcolor->blue = color->blue;
  gdkcolor->alpha = color->alpha;
}

/**
 * color_equals:
 * @color1: One #Color object
 * @color2: Another #Color object.
 *
 * Compare two colour objects.
 *
 * Returns: %TRUE if the colour objects are the same colour.
 */
gboolean
color_equals (const Color *color1, const Color *color2)
{
  return (color1->red == color2->red) &&
         (color1->green == color2->green) &&
         (color1->blue == color2->blue) &&
         (color1->alpha == color2->alpha);
}
