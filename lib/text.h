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
#ifndef TEXT_H
#define TEXT_H

typedef struct _Text Text;

#include "font.h"
#include "focus.h"

struct _Text {
  /* don't change these values directly, use the text_set* functions */
  
  /* Text data: */
  char **line;
  int numlines;
  int *strlen;
  int *alloclen;

  /* Attributes: */
  Font *font;
  real height;
  Point position;
  Color color;
  Alignment alignment;

  /* Cursor pos: */
  int cursor_pos;
  int cursor_row;
  Focus focus;
  
  /* Computed values:  */
  real ascent;
  real descent;
  real max_width;
  real *row_width;
 
};

/* makes an internal copy of the string */
extern Text *new_text(const char *string, Font *font, real height,
		      Point *pos, Color *color, Alignment align);
extern void text_destroy(Text *text);
extern Text *text_copy(Text *text);
extern char *text_get_string_copy(Text *text);
extern void text_set_string(Text *text, const char *string);
extern void text_set_height(Text *text, real height);
extern void text_set_font(Text *text, Font *font);
extern void text_set_position(Text *text, Point *pos);
extern void text_set_color(Text *text, Color *col);
extern void text_set_alignment(Text *text, Alignment align);
extern real text_distance_from(Text *text, Point *point);
extern void text_calc_boundingbox(Text *text, Rectangle *box);
extern void text_draw(Text *text, Renderer *renderer);
extern void text_set_cursor(Text *text, Point *clicked_point,
			    Renderer *interactive_renderer);
extern void text_grab_focus(Text *text, Object *object);
extern int text_is_empty(Text *text);

extern void data_add_text(AttributeNode attr, Text *text);
extern Text *data_text(AttributeNode attr);
#endif /* TEXT_H */


