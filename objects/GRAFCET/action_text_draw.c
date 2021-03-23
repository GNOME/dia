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
#include <config.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include "geometry.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "text.h"
#include "action_text_draw.h"

/* This used to be really horrible code. Really.
   Now it's just a code fork. */

void
action_text_draw (Text *text, DiaRenderer *renderer)
{
  Point pos;
  double space_width;

  dia_renderer_set_font (renderer, text->font, text->height);

  dia_text_get_position (text, &pos);

  space_width = action_text_spacewidth (text);

  /* TODO: Use the TextLine object when available for faster rendering. */
  for (int i = 0; i < text->numlines; i++) {
    dia_renderer_draw_string (renderer,
                              text_get_line (text, i),
                              &pos,
                              text->alignment,
                              &text->color);
    pos.x += text_get_line_width (text, i) +
      2 * space_width;
  }

  if (DIA_IS_INTERACTIVE_RENDERER (renderer) && (text->focus.has_focus)) {
    double curs_x, curs_y;
    double str_width_first;
    double str_width_whole;
    Point p1, p2;


    str_width_first = dia_renderer_get_text_width (renderer,
                                                   text_get_line (text, text->cursor_row),
                                                   text->cursor_pos);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   text_get_line (text, text->cursor_row),
                                                   text_get_line_strlen (text, text->cursor_row));

    curs_x = pos.x + str_width_first;
    for (int i = 0; i < text->cursor_row; i++) {
      curs_x += text_get_line_width (text, i) + 2 * space_width;
    }
    curs_y = pos.y - text->ascent;

    switch (text->alignment) {
      case ALIGN_LEFT:
        break;
      case ALIGN_CENTER:
        curs_x -= str_width_whole / 2.0; /* undefined behaviour ! */
        break;
      case ALIGN_RIGHT:
        curs_x -= str_width_whole; /* undefined behaviour ! */
        break;
      default:
        g_return_if_reached ();
    }

    p1.x = curs_x;
    p1.y = curs_y;
    p2.x = curs_x;
    p2.y = curs_y + text->height;

    dia_renderer_set_linestyle (renderer, LINESTYLE_SOLID, 0.0);
    dia_renderer_set_linewidth (renderer, 0.1);
    dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
  }
}


void
action_text_calc_boundingbox (Text *text, DiaRectangle *box)
{
  double width;
  Point pos;

  dia_text_get_position (text, &pos);

  box->left = pos.x;
  switch (text->alignment) {
    case ALIGN_LEFT:
      break;
    case ALIGN_CENTER:
      box->left -= text->max_width / 2.0;
      break;
    case ALIGN_RIGHT:
      box->left -= text->max_width;
      break;
    default:
      g_return_if_reached ();
  }

  width = 0;
  for (int i = 0; i < text->numlines; i++) {
    width += text_get_line_width (text, i);
  }

  width += text->numlines * 2.0 * action_text_spacewidth (text);

  box->right = box->left + width;

  box->top = pos.y - text->ascent;

  box->bottom = box->top + text->height;
}


real
action_text_spacewidth (Text *text)
{
  return .2 * text->height;
}
