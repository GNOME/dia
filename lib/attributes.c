/* xxxxxx -- an diagram creation/manipulation program
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
#include "attributes.h"

static Color attributes_foreground = { 0.0f, 0.0f, 0.0f };
static Color attributes_background = { 1.0f, 1.0f, 1.0f };

static real attributes_default_linewidth = 0.1;

Color 
attributes_get_foreground(void)
{
  return attributes_foreground;
}
Color 
attributes_get_background(void)
{
  return attributes_background;
}
void
attributes_set_foreground(Color *color)
{
  attributes_foreground = *color;
}

void
attributes_set_background(Color *color)
{
  attributes_background = *color;
}

void
attributes_swap_fgbg(void)
{
  Color temp;
  temp = attributes_foreground;
  attributes_foreground = attributes_background;
  attributes_background = temp;
}

void
attributes_default_fgbg(void)
{
  attributes_foreground = color_black;
  attributes_background = color_white;
}

real
attributes_get_default_linewidth(void)
{
  return attributes_default_linewidth;
}

void
attributes_set_default_linewidth(real width)
{
  attributes_default_linewidth = width;
}
