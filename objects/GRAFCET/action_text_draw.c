/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This is code is a ripoff from lib/text.c's text_draw() routine, modified
 * for the GRAFCET action text's strange behaviour.
 * The variations from the original code are Copyright(C) 2000 Cyrille Chepelov
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

#include <glib.h>

#include "action_text_draw.h"
#include "dia-text.h"
#include "diainteractiverenderer.h"
#include "diarenderer.h"
#include "geometry.h"

/* This used to be really horrible code. Really.
   Now it's just a code fork. */

/* Okay, so really what even is this object, why is any of this like this */

void
action_text_draw (DiaText *text, DiaRenderer *renderer)
{
  Point pos;
  double space_width;

  dia_renderer_set_font (renderer,
                         dia_text_get_font (text),
                         dia_text_get_height (text));

  dia_text_get_position (text, &pos);

  space_width = action_text_spacewidth (text);

  /* TODO: Use the TextLine object when available for faster rendering. */
  for (size_t i = 0; i < dia_text_get_n_lines (text); i++) {
    DiaColour text_colour;

    dia_text_get_colour (text, &text_colour);
    dia_renderer_draw_string (renderer,
                              dia_text_get_line (text, i),
                              &pos,
                              dia_text_get_alignment (text),
                              &text_colour);
    pos.x += dia_text_get_line_width (text, i) + (2 * space_width);
  }

  if (DIA_IS_INTERACTIVE_RENDERER (renderer) && dia_text_has_focus (text)) {
    double curs_x, curs_y;
    double str_width_first;
    double str_width_whole;
    Point p1, p2;
    Point text_position;
    int cursor_row, cursor_pos;

    dia_text_get_position (text, &text_position);

    dia_text_get_cursor (text, &cursor_row, &cursor_pos);

    str_width_first = dia_renderer_get_text_width (renderer,
                                                   dia_text_get_line (text, cursor_row),
                                                   cursor_pos);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   dia_text_get_line (text, cursor_row),
                                                   dia_text_get_line_strlen (text, cursor_row));

    curs_x = text_position.x + str_width_first;
    for (size_t i = 0; i < cursor_row; i++) {
      curs_x += dia_text_get_line_width (text, i) + 2 * space_width;
    }
    curs_y = text_position.y - dia_text_get_ascent (text);

    switch (dia_text_get_alignment (text)) {
      case DIA_ALIGN_LEFT:
        break;
      case DIA_ALIGN_CENTRE:
        curs_x -= str_width_whole / 2.0; /* undefined behaviour ! */
        break;
      case DIA_ALIGN_RIGHT:
        curs_x -= str_width_whole; /* undefined behaviour ! */
        break;
      default:
        g_return_if_reached ();
    }

    p1.x = curs_x;
    p1.y = curs_y;
    p2.x = curs_x;
    p2.y = curs_y + dia_text_get_height (text);

    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    dia_renderer_set_linewidth (renderer, 0.1);
    dia_renderer_draw_line (renderer, &p1, &p2, &DIA_COLOUR_BLACK);
  }
}


void
action_text_calc_boundingbox (DiaText *text, DiaRectangle *box)
{
  double width;
  Point text_position;

  dia_text_get_position (text, &text_position);

  box->left = text_position.x;
  switch (dia_text_get_alignment (text)) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      box->left -= dia_text_get_max_width (text) / 2.0;
      break;
    case DIA_ALIGN_RIGHT:
      box->left -= dia_text_get_max_width (text);
      break;
    default:
      g_return_if_reached ();
  }

  width = 0;
  for (size_t i = 0; i < dia_text_get_n_lines (text); i++) {
    width += dia_text_get_line_width (text, i);
  }

  width += dia_text_get_n_lines (text) * 2.0 * action_text_spacewidth (text);

  box->right = box->left + width;

  box->top = text_position.y - dia_text_get_ascent (text);

  box->bottom = box->top + dia_text_get_height (text);
}


double
action_text_spacewidth (DiaText *text)
{
  return .2 * dia_text_get_height (text);
}
