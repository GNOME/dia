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

static GdkColormap *colormap = NULL;

#ifndef G_OS_WIN32 
Color color_black = { 0.0f, 0.0f, 0.0f, 1.0f };
Color color_white = { 1.0f, 1.0f, 1.0f, 1.0f };
#endif

gboolean _color_initialized = FALSE;

/** Initialize color access (gdk) and set up default colors.
 */
void 
color_init(void)
{
  if (!_color_initialized) {
#if !defined(GDK_WINDOWING_QUARTZ)
    GdkVisual *visual = gtk_widget_get_default_visual();
    colormap = gdk_colormap_new (visual, FALSE); 
#else
    /* gdk/quartz does not implement gdk_colormap_new () */
    colormap = gdk_screen_get_system_colormap (gdk_screen_get_default ());
#endif

    _color_initialized = TRUE;
  }
}

/** Allocate a new color object wtih the given values.
 * Initializes alpha component to 1.0
 * @param r Red component (0 <= r <= 1)
 * @param g Green component (0 <= g <= 1)
 * @param b Blue component (0 <= b <= 1)
 * @returns A newly allocated color object.  This should be freed after use.
 */
Color *
color_new_rgb(float r, float g, float b)
{
  Color *col = g_new(Color, 1);
  col->red = r;
  col->green = g;
  col->blue = b;
  col->alpha = 1.0;
  return col;
}

/** Allocate a new color object wtih the given values.
 * @param r Red component (0 <= r <= 1)
 * @param g Green component (0 <= g <= 1)
 * @param b Blue component (0 <= b <= 1)
 * @param alpha Alpha component (0 <= alpha <= 1)
 * @returns A newly allocated color object.  This should be freed after use.
 */
Color *
color_new_rgba(float r, float g, float b, float alpha)
{
  Color *col = g_new(Color, 1);
  col->red = r;
  col->green = g;
  col->blue = b;
  col->alpha = alpha;
  return col;
}

/** Convert a Dia color object to GDK style, including handling allocation.
 * @param color A color object.  This will not be kept by this function.
 * @param gdkcolor Return value: GDK color object to fill in.
 */
void
color_convert(const Color *color, GdkColor *gdkcolor)
{
  gdkcolor->red = (guint16)(color->red*65535);
  gdkcolor->green = (guint16)(color->green*65535);
  gdkcolor->blue = (guint16)(color->blue*65535);

  if (_color_initialized) {
    if (!gdk_colormap_alloc_color (colormap, gdkcolor, TRUE, TRUE))
      g_warning ("color_convert failed.");
  } else {
    g_warning("Can't color_convert in non-interactive app (w/o color_init())");
  }
}

/** Compare two color objects.
 * @param color1 One color object
 * @param color2 Another color object.
 * @returns TRUE if the color objects are the same color.
 */
gboolean
color_equals(const Color *color1, const Color *color2)
{
  return (color1->red == color2->red) &&
    (color1->green == color2->green) &&
    (color1->blue == color2->blue) &&
    (color1->alpha == color2->alpha);
}
