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
Color color_black = { 0.0f, 0.0f, 0.0f };
Color color_white = { 1.0f, 1.0f, 1.0f };
GdkColor color_gdk_black, color_gdk_white;

void 
color_init(void)
{
  GdkVisual *visual = gdk_visual_get_system (); 
  colormap = gdk_colormap_new (visual, FALSE); 

  color_convert(&color_black, &color_gdk_black);
  color_convert(&color_white, &color_gdk_white);
}

void
color_convert(Color *color, GdkColor *gdkcolor)
{
  gdkcolor->red = color->red*65535;
  gdkcolor->green = color->green*65535;
  gdkcolor->blue = color->blue*65535;

  if (!gdk_colormap_alloc_color (colormap, gdkcolor, TRUE, TRUE))
    g_warning ("color_convert failed.");
}

gboolean
color_equals(Color *color1, Color *color2)
{
  return (color1->red == color2->red) &&
    (color1->green == color2->green) &&
    (color1->blue == color2->blue);
}
