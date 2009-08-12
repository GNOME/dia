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
#include "persistence.h"

static Color attributes_foreground = { 0.0f, 0.0f, 0.0f, 1.0f };
static Color attributes_background = { 1.0f, 1.0f, 1.0f, 1.0f };

static real attributes_default_linewidth = 0.1;

static Arrow attributes_start_arrow = { ARROW_NONE, 
					DEFAULT_ARROW_SIZE, 
					DEFAULT_ARROW_SIZE };
static Arrow attributes_end_arrow = { ARROW_NONE, 
				      DEFAULT_ARROW_SIZE,
				      DEFAULT_ARROW_SIZE };

static LineStyle attributes_linestyle = LINESTYLE_SOLID;
static real attributes_dash_length = 1.0;

static DiaFont *attributes_font = NULL;
static real attributes_font_height = 0.8;

/** Get the foreground color attribute (lines and text)
 * @returns The current foreground color as set in the toolbox.
 */
Color 
attributes_get_foreground(void)
{
  return attributes_foreground;
}

/** Get the background color attribute (for box background and such)
 * @returns The current background color as set in the toolbox.
 */
Color 
attributes_get_background(void)
{
  return attributes_background;
}

/** Set the default foreground color for new objects.
 * @param color A color object to use for foreground color.  This object is
 * not stored by ths function and can be freed afterwards.
 */
void
attributes_set_foreground(Color *color)
{
  attributes_foreground = *color;
  persistence_set_color("fg_color", color);
}

/** Set the default background color for new objects.
 * @param color A color object to use for background color.  This object is
 * not stored by ths function and can be freed afterwards.
 */
void
attributes_set_background(Color *color)
{
  attributes_background = *color;
  persistence_set_color("bg_color", color);
}

/** Swap the current foreground and background colors
 */
void
attributes_swap_fgbg(void)
{
  Color temp;
  temp = attributes_foreground;
  attributes_set_foreground(&attributes_background);
  attributes_set_background(&temp);
}

/** Set the default foreground and background colors to black and white.
 */
void
attributes_default_fgbg(void)
{
  attributes_set_foreground(&color_black);
  attributes_set_background(&color_white);
}

/** Get the default line width as defined by the toolbox.
 * @returns A linewidth (0.0 < linewidth < 10.0) that should be used as default
 *          for new objects.
 */
real
attributes_get_default_linewidth(void)
{
  return attributes_default_linewidth;
}

/** Set the default line width.
 * @param width The line width (0.0 < linewidth < 10.0) to use for new objects.
 */
void
attributes_set_default_linewidth(real width)
{
  attributes_default_linewidth = width;
  persistence_set_real("linewidth", width);
}

/** Get the default arrow type to put at the start (origin) of a connector.
 * @returns An arrow object for the arrow type defined in the toolbox.
 */
Arrow
attributes_get_default_start_arrow(void)
{
  return attributes_start_arrow;
}

/** Set the default arrow type that the toolbox will supply for new objects.
 * @param arrow An arrow object to be used for the start of new connectors.
 */
void
attributes_set_default_start_arrow(Arrow arrow)
{
  attributes_start_arrow = arrow;
  persistence_set_string("start-arrow-type", 
			 arrow_get_name_from_type(arrow.type));
  persistence_set_real("start-arrow-width", arrow.width);
  persistence_set_real("start-arrow-length", arrow.length);
}

/** Get the default arrow type to put at the end (target) of a connector.
 * @returns An arrow object for the arrow type defined in the toolbox.
 */
Arrow
attributes_get_default_end_arrow(void)
{
  return attributes_end_arrow;
}

/** Set the default arrow type that the toolbox will supply for new objects.
 * @param arrow An arrow object to be used for the end of new connectors.
 */
void
attributes_set_default_end_arrow(Arrow arrow)
{
  attributes_end_arrow = arrow;
  persistence_set_string("end-arrow-type", 
			 arrow_get_name_from_type(arrow.type));
  persistence_set_real("end-arrow-width", arrow.width);
  persistence_set_real("end-arrow-length", arrow.length);
}

/** Get the default line style (dashes, dots etc)
 * @param style A place to return the style (number of dots and dashes)
 * @param dash_length A place to return how long a dash is 
 *                    (0.0 < dash_length < ???)
 * @see dia-enums.h for possible values for style.
 */
void
attributes_get_default_line_style(LineStyle *style, real *dash_length)
{
  if (style)
    *style = attributes_linestyle;
  if (dash_length)
    *dash_length = attributes_dash_length;
}

/** Set the default line style (dashes, dots etc)
 * @param style The style (number of dots and dashes)
 * @param dash_length The length of a dash (0.0 < dash_length < ???)
 * @see dia-enums.h for possible values for style.
 */
void
attributes_set_default_line_style(LineStyle style, real dash_length)
{
  attributes_linestyle = style;
  attributes_dash_length = dash_length;
  persistence_set_integer("line-style", style);
  persistence_set_real("dash-length", dash_length);
}

/** Get the default font.
 * Note that there is currently no way for the user to set these.
 * @param font A place to return the default font description set by the user
 * @param font_height A place to return the default font height set by the user
 */
void
attributes_get_default_font(DiaFont **font, real *font_height)
{
	if (!attributes_font) {
		attributes_font = dia_font_new_from_style(DIA_FONT_SANS,
                                              attributes_font_height);
	}
	if (font)
		*font = dia_font_ref(attributes_font);
	if (font_height)
		*font_height = attributes_font_height;
}

/** Set the default font.
 * Note that this is not currently stored persistently.
 * @param font The font to set as the new default.
 *        This object will be ref'd by this call.
 * @param font_height The font height to set as the new default.
 */
void
attributes_set_default_font(DiaFont *font, real font_height)
{
  if (attributes_font != NULL)
    dia_font_unref(attributes_font);  
  attributes_font = dia_font_ref(font);
  attributes_font_height = font_height;
}
