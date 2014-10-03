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

#ifndef DIA_SVG_H
#define DIA_SVG_H

#include "dia_xml.h"
#include "font.h"

/* special colours */
enum DiaSvgColours
{
  DIA_SVG_COLOUR_NONE = -1,
  DIA_SVG_COLOUR_FOREGROUND = -2,
  DIA_SVG_COLOUR_BACKGROUND = -3,
  DIA_SVG_COLOUR_TEXT = -4,
  /* used for initlaization only */
  DIA_SVG_COLOUR_DEFAULT = -5
};

typedef struct _DiaSvgStyle DiaSvgStyle;
struct _DiaSvgStyle {
  real line_width;
  gint32 stroke;
  real   stroke_opacity;
  gint32 fill;
  real   fill_opacity;

  LineCaps linecap;
  LineJoin linejoin;
  LineStyle linestyle;
  real dashlength;

  DiaFont *font;
  real font_height;
  Alignment alignment;

  gint32 stop_color;
  real   stop_opacity;
};

void dia_svg_style_init (DiaSvgStyle *gs, DiaSvgStyle *parent_style);
void dia_svg_style_copy (DiaSvgStyle *dest, DiaSvgStyle *src);
gboolean dia_svg_parse_color(const gchar *str, Color *color);
void dia_svg_parse_style(xmlNodePtr node, DiaSvgStyle *s, real user_scale);
void dia_svg_parse_style_string (DiaSvgStyle *s, real user_scale, const gchar *str);
/* parse the svg sub format for pathes int an array of BezPoint */
gboolean dia_svg_parse_path(GArray *points, const gchar *path_str, gchar **unparsed,
			    gboolean *closed, Point *current_point);
DiaMatrix *dia_svg_parse_transform(const gchar *trans, real scale);
gchar *dia_svg_from_matrix(const DiaMatrix *matrix, real scale);

#endif /* DIA_SVG_H */
