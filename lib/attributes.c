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

#include "attributes.h"
#include "intl.h"

static Color attributes_foreground = { 0.0f, 0.0f, 0.0f };
static Color attributes_background = { 1.0f, 1.0f, 1.0f };

static real attributes_default_linewidth = 0.1;

static Arrow attributes_start_arrow = { ARROW_NONE, 0.8, 0.8 };
static Arrow attributes_end_arrow = { ARROW_NONE, 0.8, 0.8 };

static LineStyle attributes_linestyle = LINESTYLE_SOLID;
static real attributes_dash_length = 1.0;

static DiaFont *attributes_font = NULL;
static real attributes_font_height = 0.8;

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

Arrow
attributes_get_default_start_arrow(void)
{
  return attributes_start_arrow;
}
void
attributes_set_default_start_arrow(Arrow arrow)
{
  attributes_start_arrow = arrow;
}

Arrow
attributes_get_default_end_arrow(void)
{
  return attributes_end_arrow;
}
void
attributes_set_default_end_arrow(Arrow arrow)
{
  attributes_end_arrow = arrow;
}

void
attributes_get_default_line_style(LineStyle *style, real *dash_length)
{
  if (style)
    *style = attributes_linestyle;
  if (dash_length)
    *dash_length = attributes_dash_length;
}
void
attributes_set_default_line_style(LineStyle style, real dash_length)
{
  attributes_linestyle = style;
  attributes_dash_length = dash_length;
}

void
attributes_get_default_font(DiaFont **font, real *font_height)
{
	if (!attributes_font) {
		/* choose default font name for your locale. see also font_data structure
		   in lib/font.c. if "Courier" works for you, it would be better.  */
		attributes_font = font_getfont(_("Courier"));
	}
	if (font)
		*font = attributes_font;
	if (font_height)
		*font_height = attributes_font_height;
}

void
attributes_set_default_font(DiaFont *font, real font_height)
{
  attributes_font = font;
  attributes_font_height = font_height;
}
