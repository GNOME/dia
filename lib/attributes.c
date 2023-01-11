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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "attributes.h"
#include "persistence.h"


static Color attributes_foreground = { 0.0f, 0.0f, 0.0f, 1.0f };
static Color attributes_background = { 1.0f, 1.0f, 1.0f, 1.0f };

static double attributes_default_linewidth = 0.1;

static Arrow attributes_start_arrow = {
  ARROW_NONE,
  DEFAULT_ARROW_SIZE,
  DEFAULT_ARROW_SIZE
};

static Arrow attributes_end_arrow = {
  ARROW_NONE,
  DEFAULT_ARROW_SIZE,
  DEFAULT_ARROW_SIZE
};

static DiaLineStyle attributes_linestyle = DIA_LINE_STYLE_SOLID;
static double attributes_dash_length = 1.0;

static DiaFont *attributes_font = NULL;
static double attributes_font_height = 0.8;


/**
 * attributes_get_foreground:
 *
 * Get the foreground color attribute (lines and text)
 *
 * Returns: The current foreground color as set in the toolbox.
 *
 * Since: dawn-of-time
 */
Color
attributes_get_foreground (void)
{
  return attributes_foreground;
}


/**
 * attributes_get_background:
 *
 * Get the background color attribute (for box background and such)
 *
 * Returns: The current background color as set in the toolbox.
 *
 * Since: dawn-of-time
 */
Color
attributes_get_background (void)
{
  return attributes_background;
}


/**
 * attributes_set_foreground:
 * @color: A color object to use for foreground color. This object is
 *         not stored by ths function and can be freed afterwards.
 *
 * Set the default foreground color for new objects.
 *
 * Since: dawn-of-time
 */
void
attributes_set_foreground (Color *color)
{
  attributes_foreground = *color;
  persistence_set_color ("fg_color", color);
}


/**
 * attributes_set_background:
 * @color: A color object to use for background color. This object is
 *         not stored by ths function and can be freed afterwards.
 *
 * Set the default background color for new objects.
 *
 * Since: dawn-of-time
 */
void
attributes_set_background (Color *color)
{
  attributes_background = *color;
  persistence_set_color ("bg_color", color);
}


/**
 * attributes_swap_fgbg:
 *
 * Swap the current foreground and background colors
 *
 * Since: dawn-of-time
 */
void
attributes_swap_fgbg (void)
{
  Color temp;
  temp = attributes_foreground;
  attributes_set_foreground (&attributes_background);
  attributes_set_background (&temp);
}


/**
 * attributes_default_fgbg:
 *
 * Set the default foreground and background colors to black and white.
 *
 * Since: dawn-of-time
 */
void
attributes_default_fgbg (void)
{
  attributes_set_foreground (&color_black);
  attributes_set_background (&color_white);
}


/**
 * attributes_get_default_linewidth:
 *
 * Get the default line width as defined by the toolbox.
 *
 * Returns: A linewidth (0.0 < linewidth < 10.0) that should be used as default
 *          for new objects.
 *
 * Since: dawn-of-time
 */
double
attributes_get_default_linewidth (void)
{
  return attributes_default_linewidth;
}


/**
 * attributes_set_default_linewidth:
 * @width: The line width (0.0 < linewidth < 10.0) to use for new objects.
 *
 * Set the default line width.
 *
 * Since: dawn-of-time
 */
void
attributes_set_default_linewidth (double width)
{
  attributes_default_linewidth = width;
  persistence_set_real ("linewidth", width);
}


/**
 * attributes_get_default_start_arrow:
 *
 * Get the default arrow type to put at the start (origin) of a connector.
 *
 * Returns: An arrow object for the arrow type defined in the toolbox.
 *
 * Since: dawn-of-time
 */
Arrow
attributes_get_default_start_arrow (void)
{
  return attributes_start_arrow;
}


/**
 * attributes_set_default_start_arrow:
 * @arrow: An arrow object to be used for the start of new connectors.
 *
 * Set the default arrow type that the toolbox will supply for new objects.
 *
 * Since: dawn-of-time
 */
void
attributes_set_default_start_arrow (Arrow arrow)
{
  attributes_start_arrow = arrow;
  persistence_set_string ("start-arrow-type",
                          arrow_get_name_from_type (arrow.type));
  persistence_set_real ("start-arrow-width", arrow.width);
  persistence_set_real ("start-arrow-length", arrow.length);
}


/**
 * attributes_get_default_end_arrow:
 *
 * Get the default arrow type to put at the end (target) of a connector.
 *
 * Returns: An arrow object for the arrow type defined in the toolbox.
 *
 * Since: dawn-of-time
 */
Arrow
attributes_get_default_end_arrow (void)
{
  return attributes_end_arrow;
}


/**
 * attributes_set_default_end_arrow:
 * @arrow: An arrow object to be used for the end of new connectors.
 *
 * Set the default arrow type that the toolbox will supply for new objects.
 *
 * Since: dawn-of-time
 */
void
attributes_set_default_end_arrow (Arrow arrow)
{
  attributes_end_arrow = arrow;
  persistence_set_string ("end-arrow-type",
                          arrow_get_name_from_type (arrow.type));
  persistence_set_real ("end-arrow-width", arrow.width);
  persistence_set_real ("end-arrow-length", arrow.length);
}


/**
 * attributes_get_default_line_style:
 * @style: (out): A place to return the style (number of dots and dashes)
 * @dash_length: (out): A place to return how long a dash is
 *                      (0.0 < dash_length < ???)
 *
 * Get the default line style (dashes, dots etc)
 *
 * Since: dawn-of-time
 */
void
attributes_get_default_line_style (DiaLineStyle *style, double *dash_length)
{
  if (style) {
    *style = attributes_linestyle;
  }
  if (dash_length) {
    *dash_length = attributes_dash_length;
  }
}


/**
 * attributes_set_default_line_style:
 * @style: The style (number of dots and dashes)
 * @dash_length: The length of a dash (0.0 < dash_length < ???)
 *
 * Set the default line style (dashes, dots etc)
 *
 * Since: dawn-of-time
 */
void
attributes_set_default_line_style (DiaLineStyle style, double dash_length)
{
  attributes_linestyle = style;
  attributes_dash_length = dash_length;
  persistence_set_integer ("line-style", style);
  persistence_set_real ("dash-length", dash_length);
}


/**
 * attributes_get_default_font:
 * @font: (out): A place to return the default font description set by the user
 * @font_height: (out): A place to return the default font height set by the user
 *
 * Get the default font.
 *
 * Note that there is currently no way for the user to set these.
 *
 * Since: dawn-of-time
 */
void
attributes_get_default_font (DiaFont **font, double *font_height)
{
  if (!attributes_font) {
    attributes_font = dia_font_new_from_style (DIA_FONT_SANS,
                                               attributes_font_height);
  }

  if (font) {
    *font = g_object_ref (attributes_font);
  }

  if (font_height) {
    *font_height = attributes_font_height;
  }
}


/**
 * attributes_set_default_font:
 * @font: The font to set as the new default. This object will be ref'd by
 *        this call.
 * @font_height: The font height to set as the new default.
 *
 * Set the default font.
 *
 * Note that this is not currently stored persistently.
 *
 * Since: dawn-of-time
 */
void
attributes_set_default_font (DiaFont *font, real font_height)
{
  g_set_object (&attributes_font, font);

  attributes_font_height = font_height;
}
