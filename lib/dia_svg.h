/* dia -- an diagram creation/manipulation program
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

#include "dia_xml.h"
#include "font.h"

/* special colours */
enum DiaSvgColours
{
  DIA_SVG_COLOUR_NONE = -1,
  DIA_SVG_COLOUR_FOREGROUND = -2,
  DIA_SVG_COLOUR_BACKGROUND = -3,
  DIA_SVG_COLOUR_TEXT = -4
};

/* these should be changed if they ever cause a conflict */
enum DiaSvgLineDefaults
{
  DIA_SVG_LINECAPS_DEFAULT = 20,
  DIA_SVG_LINEJOIN_DEFAULT = 20,
  DIA_SVG_LINESTYLE_DEFAULT = 20
};

typedef struct _DiaSvgGraphicStyle DiaSvgGraphicStyle;
struct _DiaSvgGraphicStyle {
  real line_width;
  gint32 stroke;
  gint32 fill;

  LineCaps linecap;
  LineJoin linejoin;
  LineStyle linestyle;
  real dashlength;

  DiaFont *font;
  real font_height;
  Alignment alignment;
};

void dia_svg_parse_style(xmlNodePtr node, DiaSvgGraphicStyle *s);
