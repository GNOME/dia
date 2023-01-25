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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include <gdk/gdkkeysyms.h>

#include "propinternals.h"
#include "text.h"
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "diagramdata.h"
#include "textline.h"
#include "attributes.h"
#include "object.h"
#include "dia-object-change-list.h"


static int text_key_event (Focus            *focus,
                           guint             keystate,
                           guint             keysym,
                           const char       *str,
                           int               strlen,
                           DiaObjectChange **change);


typedef enum {
  TYPE_DELETE_BACKWARD,
  TYPE_DELETE_FORWARD,
  TYPE_INSERT_CHAR,
  TYPE_JOIN_ROW,
  TYPE_SPLIT_ROW,
  TYPE_DELETE_ALL
} TextChangeType;


struct _DiaTextObjectChange {
  DiaObjectChange obj_change;

  Text *text;
  TextChangeType type;
  gunichar ch;
  int pos;
  int row;
  char *str;

  /* the owning object ... */
  DiaObject *obj;
  /* ... and it's size position properties, which might change
   * as a side-effect of changing the text
   */
  GPtrArray *props;
};


DIA_DEFINE_OBJECT_CHANGE (DiaTextObjectChange, dia_text_object_change)


#define CURSOR_HEIGHT_RATIO 20


/**
 * text_get_line:
 * @text: the #Text
 * @line: the line number in @text to fetch
 *
 * Encapsulation functions for transferring to text_line
 *
 * Since: dawn-of-time
 */
char *
text_get_line (const Text *text, int line)
{
  return text_line_get_string (text->lines[line]);
}


/**
 * text_set_line:
 *
 * Raw sets one line to a given text, not copying, not freeing.
 *
 * Since: dawn-of-time
 */
static void
text_set_line (Text *text, int line_no, char *line)
{
  text_line_set_string (text->lines[line_no], line);
}


/**
 * text_set_line_text:
 *
 * Set the text of a line, freeing, copying and mallocing as required.
 * Updates strlen and row_width entries, but not max_width.
 *
 * Since: dawn-of-time
 */
static void
text_set_line_text (Text *text, int line_no, char *line)
{
  text_set_line (text, line_no, line);
}


/**
 * text_delete_line:
 *
 * Delete the line, freeing appropriately and moving stuff up.
 * This function circumvents the normal free/alloc cycle of
 * text_set_line_text.
 *
 * Since: dawn-of-time
 */
static void
text_delete_line (Text *text, int line_no)
{
  g_clear_pointer (&text->lines[line_no], g_free);

  for (int i = line_no; i < text->numlines - 1; i++) {
    text->lines[i] = text->lines[i+1];
  }

  text->numlines -= 1;
  text->lines = g_renew (TextLine *, text->lines, text->numlines);
}


/**
 * text_insert_line:
 *
 * Insert a new (empty) line at line_no.
 * This function circumvents the normal free/alloc cycle of
 * text_set_line_text.
 *
 * Since: dawn-of-time
 */
static void
text_insert_line (Text *text, int line_no)
{
  text->numlines += 1;
  text->lines = g_renew (TextLine *, text->lines, text->numlines);

  for (int i = text->numlines - 1; i > line_no; i--) {
    text->lines[i] = text->lines[i - 1];
  }
  text->lines[line_no] = text_line_new ("", text->font, text->height);
}


/**
 * text_get_line_width:
 * @text: The text object;
 * @line_no: The index of the line in the text object, starting at 0.
 *
 * Get the in-diagram width of the given line.
 *
 * Returns: The width in cm of the indicated line.
 *
 * Since: dawn-of-time
 */
double
text_get_line_width (const Text *text, int line_no)
{
  return text_line_get_width (text->lines[line_no]);
}


/**
 * text_get_line_strlen:
 * @text: The text object;
 * @line_no: The index of the line in the text object, starting at 0.
 *
 * Get the number of characters of the given line.
 *
 * Returns: The number of UTF-8 characters of the indicated line.
 *
 * Since: dawn-of-time
 */
int
text_get_line_strlen (const Text *text, int line_no)
{
  return g_utf8_strlen (text_line_get_string (text->lines[line_no]), -1);
}


double
text_get_max_width (Text *text)
{
  return text->max_width;
}


/**
 * text_get_ascent:
 * @text: Text object
 *
 * Get the *average* ascent of this #Text object.
 *
 * Returns: the average of the ascents of each line (height above baseline)
 */
double
text_get_ascent (Text *text)
{
  return text->ascent;
}


/**
 * text_get_descent:
 * @text: a #Text object
 *
 * Get the *average* descent of this Text object.
 *
 * Returns: the average of the descents of each line (height below baseline)
 *
 * Since: dawn-of-time
 */
double
text_get_descent (Text *text)
{
  return text->descent;
}


static DiaObjectChange *text_create_change (Text           *text,
                                            TextChangeType  type,
                                            gunichar        ch,
                                            int             pos,
                                            int             row,
                                            DiaObject      *obj);


static void
calc_width (Text *text)
{
  double width;

  width = 0.0;
  for (int i = 0; i < text->numlines; i++) {
    width = MAX (width, text_get_line_width (text, i));
  }

  text->max_width = width;
}


static void
calc_ascent_descent(Text *text)
{
  double sig_a = 0.0,sig_d = 0.0;

  for (int i = 0; i < text->numlines; i++) {
    sig_a += text_line_get_ascent (text->lines[i]);
    sig_d += text_line_get_descent (text->lines[i]);
  }

  text->ascent = sig_a / (double) text->numlines;
  text->descent = sig_d / (double) text->numlines;
}


static void
free_string (Text *text)
{
  for (int i = 0; i < text->numlines; i++) {
    text_line_destroy(text->lines[i]);
  }

  g_clear_pointer (&text->lines, g_free);
}


static void
set_string (Text *text, const char *string)
{
  int numlines, i;
  const char *s,*s2;
  char *fallback = NULL;

  if (string && !g_utf8_validate (string, -1, NULL)) {
    GError *error = NULL;
    s = fallback = g_locale_to_utf8 (string, -1, NULL, NULL, &error);
    if (!fallback) {
      g_warning ("Invalid string data, neither UTF-8 nor locale: %s", error->message);
      string = NULL;
    }
  } else {
    s = string;
  }
  numlines = 1;
  if (s != NULL)
    while ( (s = g_utf8_strchr (s, -1, '\n')) != NULL ) {
      numlines++;
      if (*s) {
        s = g_utf8_next_char (s);
      }
    }
  text->numlines = numlines;
  text->lines = g_new0 (TextLine *, numlines);
  for (i = 0; i < numlines; i++) {
    text->lines[i] = text_line_new ("", text->font, text->height);
  }

  s = fallback ? fallback : string;
  if (s == NULL) {
    text_set_line_text (text, 0, "");
    return;
  }

  for (i = 0; i < numlines; i++) {
    char *string_line;
    s2 = g_utf8_strchr (s, -1, '\n');
    if (s2 == NULL) { /* No newline */
      s2 = s + strlen (s);
    }
    string_line = g_strndup (s, s2 - s);
    text_set_line_text (text, i, string_line);
    g_clear_pointer (&string_line, g_free);
    s = s2;
    if (*s) {
      s = g_utf8_next_char (s);
    }
  }

  if (text->cursor_row >= text->numlines) {
    text->cursor_row = text->numlines - 1;
  }

  if (text->cursor_pos > text_get_line_strlen (text, text->cursor_row)) {
    text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
  }
  g_clear_pointer (&fallback, g_free);
}


void
text_set_string (Text *text, const char *string)
{
  if (text->lines != NULL) {
    free_string (text);
  }

  set_string (text, string);
}


Text *
new_text (const char   *string,
          DiaFont      *font,
          double        height,
          Point        *pos,
          Color        *color,
          DiaAlignment  align)
{
  Text *text;

  text = g_new0 (Text, 1);

  text->font = g_object_ref (font);
  text->height = height;

  text->position = *pos;
  text->color = *color;
  text->alignment = align;

  text->cursor_pos = 0;
  text->cursor_row = 0;

  text->focus.obj = NULL;
  text->focus.has_focus = FALSE;
  text->focus.key_event = text_key_event;
  text->focus.text = text;

  set_string (text, string);

  calc_ascent_descent (text);

  return text;
}


/**
 * new_text_default:
 * @pos: the #Point the text is located
 * @color: the #Color of the text
 * @align: the #DiaAlignment of the text
 *
 * Fallback function returning a default initialized text object.
 */
Text *
new_text_default (Point *pos, Color *color, DiaAlignment align)
{
  Text *text;
  DiaFont *font;
  double font_height;

  attributes_get_default_font (&font, &font_height);
  text = new_text ("", font, font_height, pos, color, align);
  g_clear_object (&font);

  return text;
}


Text *
text_copy (Text *text)
{
  Text *copy;
  int i;

  copy = g_new0 (Text, 1);
  copy->numlines = text->numlines;
  copy->lines = g_new0 (TextLine *, text->numlines);

  copy->font = dia_font_copy (text->font);
  copy->height = text->height;
  copy->position = text->position;
  copy->color = text->color;
  copy->alignment = text->alignment;

  for (i=0;i<text->numlines;i++) {
    TextLine *text_line = text->lines[i];
    copy->lines[i] = text_line_new(text_line_get_string(text_line),
				   text_line_get_font(text_line),
				   text_line_get_height(text_line));
  }

  copy->cursor_pos = 0;
  copy->cursor_row = 0;
  copy->focus.obj = NULL;
  copy->focus.has_focus = FALSE;
  copy->focus.key_event = text_key_event;
  copy->focus.text = copy;

  copy->ascent = text->ascent;
  copy->descent = text->descent;
  copy->max_width = text->max_width;

  return copy;
}


void
text_destroy (Text *text)
{
  free_string (text);
  g_clear_object (&text->font);
  g_clear_pointer (&text, g_free);
}


void
text_set_height (Text *text, double height)
{
  text->height = height;
  for (int i = 0; i < text->numlines; i++) {
    text_line_set_height (text->lines[i], height);
  }
  calc_width (text);
  calc_ascent_descent (text);
}


double
text_get_height (const Text *text)
{
  return text->height;
}


void
text_set_font (Text *text, DiaFont *font)
{
  g_set_object (&text->font, font);

  for (int i = 0; i < text->numlines; i++) {
    text_line_set_font (text->lines[i], font);
  }

  calc_width (text);
  calc_ascent_descent (text);
}


void
text_set_position (Text *text, Point *pos)
{
  text->position = *pos;
}


void
text_set_color (Text *text, Color *col)
{
  text->color = *col;
}


void
text_set_alignment (Text *text, DiaAlignment align)
{
  text->alignment = align;
}


void
text_calc_boundingbox (Text *text, DiaRectangle *box)
{
  calc_width (text);
  calc_ascent_descent (text);

  if (box == NULL) {
    return; /* For those who just want the text info
               updated */
  }

  box->left = text->position.x;

  switch (text->alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      box->left -= text->max_width / 2.0;
      break;
    case DIA_ALIGN_RIGHT:
      box->left -= text->max_width;
      break;
    default:
      g_return_if_reached ();
  }

  box->right = box->left + text->max_width;

  box->top = text->position.y - text->ascent;
#if 0
  box->bottom = box->top + text->height*text->numlines + text->descent;
#else
  /* why should we add one descent? isn't ascent+descent~=height? */
  box->bottom = box->top + (text->ascent+text->descent+text->height*(text->numlines-1));
#endif
  if (text->focus.has_focus) {
    double height = text->ascent + text->descent;
    if (text->cursor_pos == 0) {
      /* Half the cursor width */
      box->left -= height/(CURSOR_HEIGHT_RATIO*2);
    } else {
      /* Half the cursor width. Assume that
         if it isn't at position zero, it might be
         at the last position possible. */
      box->right += height/(CURSOR_HEIGHT_RATIO*2);
    }

    /* Account for the size of the cursor top and bottom */
    box->top -= height/(CURSOR_HEIGHT_RATIO*2);
    box->bottom += height/CURSOR_HEIGHT_RATIO;
  }
}


char *
text_get_string_copy (const Text *text)
{
  int num;
  char *str;

  num = 0;
  for (int i = 0; i < text->numlines; i++) {
    /* This is for allocation, so it should not use g_utf8_strlen() */
    num += strlen (text_get_line (text, i)) + 1;
  }

  str = g_new0 (char, num);

  for (int i = 0; i < text->numlines; i++) {
    strcat (str, text_get_line (text, i));
    if (i != (text->numlines - 1)) {
      strcat (str, "\n");
    }
  }

  return str;
}


double
text_distance_from (Text *text, Point *point)
{
  double dx, dy;
  double topy, bottomy;
  double left, right;
  int line;

  topy = text->position.y - text->ascent;
  bottomy = text->position.y + text->descent + text->height *
                (text->numlines - 1);
  if (point->y <= topy) {
    dy = topy - point->y;
    line = 0;
  } else if (point->y >= bottomy) {
    dy = point->y - bottomy;
    line = text->numlines - 1;
  } else {
    dy = 0.0;
    line = (int) floor ((point->y - topy) / text->height);
    if (line >= text->numlines) {
      line = text->numlines - 1;
    }
  }

  left = text->position.x;
  switch (text->alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      left -= text_get_line_width (text, line) / 2.0;
      break;
    case DIA_ALIGN_RIGHT:
      left -= text_get_line_width (text, line);
      break;
    default:
      g_return_val_if_reached (0.0);
  }
  right = left + text_get_line_width (text, line);

  if (point->x <= left) {
    dx = left - point->x;
  } else if (point->x >= right) {
    dx = point->x - right;
  } else {
    dx = 0.0;
  }

  return dx + dy;
}


void
text_draw (Text *text, DiaRenderer *renderer)
{
  dia_renderer_draw_text (renderer, text);

  if (DIA_IS_INTERACTIVE_RENDERER (renderer) && (text->focus.has_focus)) {
    double curs_x, curs_y;
    double str_width_first;
    double str_width_whole;
    Point p1, p2;
    double height = text->ascent+text->descent;
    curs_y = text->position.y - text->ascent + text->cursor_row*text->height;

    dia_renderer_set_font (renderer, text->font, text->height);

    str_width_first = dia_renderer_get_text_width (renderer,
                                                   text_get_line (text, text->cursor_row),
                                                   text->cursor_pos);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   text_get_line (text, text->cursor_row),
                                                   text_get_line_strlen (text, text->cursor_row));
    curs_x = text->position.x + str_width_first;

    switch (text->alignment) {
      case DIA_ALIGN_LEFT:
        break;
      case DIA_ALIGN_CENTRE:
        curs_x -= str_width_whole / 2.0;
        break;
      case DIA_ALIGN_RIGHT:
        curs_x -= str_width_whole;
        break;
      default:
        g_return_if_reached ();
    }

    p1.x = curs_x;
    p1.y = curs_y;
    p2.x = curs_x;
    p2.y = curs_y + height;

    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
    dia_renderer_set_linewidth (renderer, height / CURSOR_HEIGHT_RATIO);
    dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
  }
}


void
text_grab_focus (Text *text, DiaObject *object)
{
  text->focus.obj = object;
  request_focus (&text->focus);
}


void
text_set_cursor_at_end (Text* text )
{
  text->cursor_row = text->numlines - 1;
  text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
}


typedef enum {
  WORD_START = 1,
  WORD_END
} CursorMovement;


static void
text_move_cursor (Text *text, CursorMovement mv)
{
  char *str = text_line_get_string (text->lines[text->cursor_row]);
  char *p = str;
  int curmax = text_get_line_strlen (text, text->cursor_row);
  if (text->cursor_pos > 0 && text->cursor_pos <= curmax) {
    int i;
    for (i = 0; i < text->cursor_pos; ++i)
      p = g_utf8_next_char (p);
  }
  if (WORD_START == mv && text->cursor_pos < 1) {
    if (text->cursor_row) {
      text->cursor_row--;
      text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
    }
    return;
  } else if (WORD_END == mv && text->cursor_pos == curmax) {
    if (text->cursor_row < text->numlines - 1) {
      text->cursor_row++;
      text->cursor_pos = 0;
    }
    return;
  }
  while (!g_unichar_isalnum (g_utf8_get_char (p))) {
    p = (WORD_START == mv ? g_utf8_find_prev_char (str, p) : g_utf8_next_char (p));
    if (p) text->cursor_pos += (WORD_START == mv ? -1 : 1);
    if (!p || !*p)
      return;
    if (!text->cursor_pos || text->cursor_pos == curmax)
      return;
  }
  while (g_unichar_isalnum (g_utf8_get_char (p))) {
    p = (WORD_START == mv ? g_utf8_find_prev_char (str, p) : g_utf8_next_char (p));
    if (p) text->cursor_pos += (WORD_START == mv ? -1 : 1);
    if (!p || !*p)
      return;
    if (!text->cursor_pos || text->cursor_pos == curmax)
      return;
  }
}


/* The renderer is only used to determine where the click is, so is not
 * required when no point is given. */
void
text_set_cursor (Text        *text,
                 Point       *clicked_point,
                 DiaRenderer *renderer)
{
  double str_width_whole;
  double str_width_first;
  double top;
  double start_x;
  int row;
  int i;

  if (clicked_point != NULL) {
    top = text->position.y - text->ascent;

    row = (int)floor((clicked_point->y - top) / text->height);

    if (row < 0)
      row = 0;

    if (row >= text->numlines)
      row = text->numlines - 1;

    text->cursor_row = row;
    text->cursor_pos = 0;

    if (!DIA_IS_INTERACTIVE_RENDERER (renderer)) {
      g_warning ("Internal error: Select gives non interactive renderer!\n"
                 "renderer: %s", g_type_name (G_TYPE_FROM_INSTANCE (renderer)));
      return;
    }


    dia_renderer_set_font (renderer, text->font, text->height);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   text_get_line (text, row),
                                                   text_get_line_strlen (text, row));
    start_x = text->position.x;
    switch (text->alignment) {
      case DIA_ALIGN_LEFT:
        break;
      case DIA_ALIGN_CENTRE:
        start_x -= str_width_whole / 2.0;
        break;
      case DIA_ALIGN_RIGHT:
        start_x -= str_width_whole;
        break;
      default:
        g_return_if_reached ();
    }

    /* Do an ugly linear search for the cursor index:
       TODO: Change to binary search */
    {
      double min_dist = G_MAXDOUBLE;
      for (i = 0; i <= text_get_line_strlen (text, row); i++) {
        double dist;
        str_width_first = dia_renderer_get_text_width (renderer, text_get_line (text, row), i);
        dist = fabs (clicked_point->x - (start_x + str_width_first));
        if (dist < min_dist) {
          min_dist = dist;
          text->cursor_pos = i;
        } else {
          return;
        }
      }
    }
    text->cursor_pos = text_get_line_strlen (text, row);
  } else {
    /* No clicked point, leave cursor where it is */
  }
}


static void
text_join_lines (Text *text, int first_line)
{
  char *combined_line;
  int len1;

  len1 = text_get_line_strlen (text, first_line);

  combined_line = g_strconcat (text_get_line (text, first_line),
                               text_get_line (text, first_line + 1), NULL);
  text_delete_line (text, first_line);
  text_set_line_text (text, first_line, combined_line);
  g_clear_pointer (&combined_line, g_free);

  text->max_width = MAX (text->max_width,
                         text_get_line_width (text, first_line));

  text->cursor_row = first_line;
  text->cursor_pos = len1;
}


static void
text_delete_forward (Text *text)
{
  int row;
  int i;
  double width;
  char *line;
  char *utf8_before, *utf8_after;
  char *str1, *str;

  row = text->cursor_row;

  if (text->cursor_pos >= text_get_line_strlen (text, row)) {
    if (row + 1 < text->numlines) {
      text_join_lines (text, row);
    }
    return;
  }

  line = text_get_line (text, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong)(text->cursor_pos));
  utf8_after = g_utf8_offset_to_pointer (utf8_before, 1);
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, utf8_after, NULL);
  text_set_line_text (text, row, str);
  g_clear_pointer (&str1, g_free);
  g_clear_pointer (&str, g_free);

  if (text->cursor_pos > text_get_line_strlen (text, text->cursor_row)) {
    text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
  }

  width = 0.0;
  for (i = 0; i < text->numlines; i++) {
    width = MAX (width, text_get_line_width (text, i));
  }
  text->max_width = width;
}


static void
text_delete_backward (Text *text)
{
  int row;
  int i;
  double width;
  char *line;
  char *utf8_before, *utf8_after;
  char *str1, *str;

  row = text->cursor_row;

  if (text->cursor_pos <= 0) {
    if (row > 0) {
      text_join_lines (text, row - 1);
    }
    return;
  }

  line = text_get_line (text, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (text->cursor_pos - 1));
  utf8_after = g_utf8_offset_to_pointer (utf8_before, 1);
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, utf8_after, NULL);
  text_set_line_text (text, row, str);
  g_clear_pointer (&str, g_free);
  g_clear_pointer (&str1, g_free);

  text->cursor_pos --;
  if (text->cursor_pos > text_get_line_strlen(text, text->cursor_row)) {
    text->cursor_pos = text_get_line_strlen(text, text->cursor_row);
  }

  width = 0.0;
  for (i = 0; i < text->numlines; i++) {
    width = MAX (width, text_get_line_width (text, i));
  }
  text->max_width = width;
}


static void
text_split_line (Text *text)
{
  int i;
  char *line;
  double width;
  char *utf8_before;
  char *str1, *str2;

  /* Split the lines at cursor_pos */
  line = text_get_line (text, text->cursor_row);
  text_insert_line (text, text->cursor_row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (text->cursor_pos));
  str1 = g_strndup (line, utf8_before - line);
  str2 = g_strdup (utf8_before); /* Must copy before dealloc */
  text_set_line_text (text, text->cursor_row, str1);
  text_set_line_text (text, text->cursor_row + 1, str2);
  g_clear_pointer (&str2, g_free);
  g_clear_pointer (&str1, g_free);

  text->cursor_row ++;
  text->cursor_pos = 0;

  width = 0.0;
  for (i = 0; i < text->numlines; i++) {
    width = MAX (width, text_get_line_width (text, i));
  }
  text->max_width = width;
}


static void
text_insert_char (Text *text, gunichar c)
{
  char ch[7];
  int unilen;
  int row;
  char *line, *str;
  char *utf8_before;
  char *str1;

  /* Make a string of the the char */
  unilen = g_unichar_to_utf8 (c, ch);
  ch[unilen] = 0;

  row = text->cursor_row;

  /* Copy the before and after parts with the new char in between */
  line = text_get_line (text, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (text->cursor_pos));
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, ch, utf8_before, NULL);
  text_set_line_text (text, row, str);
  g_clear_pointer (&str, g_free);
  g_clear_pointer (&str1, g_free);

  text->cursor_pos++;
  text->max_width = MAX (text->max_width, text_get_line_width (text, row));
}


gboolean
text_delete_key_handler (Focus *focus, DiaObjectChange **change)
{
  Text *text;
  int row, i;
  const char *utf;
  gunichar c;

  text = focus->text;
  row = text->cursor_row;
  if (text->cursor_pos >= text_get_line_strlen(text, row)) {
    if (row+1 < text->numlines) {
      *change = text_create_change (text, TYPE_JOIN_ROW, 'Q',
                                    text->cursor_pos, row, focus->obj);
    } else {
      return FALSE;
    }
  } else {
    utf = text_get_line(text, row);
    for (i = 0; i < text->cursor_pos; i++) {
      utf = g_utf8_next_char (utf);
    }
    c = g_utf8_get_char (utf);
    *change = text_create_change (text, TYPE_DELETE_FORWARD, c,
                                  text->cursor_pos, text->cursor_row, focus->obj);
  }
  text_delete_forward(text);
  return TRUE;
}


static int
text_key_event (Focus            *focus,
                guint             keystate,
                guint             keyval,
                const char       *str,
                int               strlen,
                DiaObjectChange **change)
{
  Text *text;
  int return_val = FALSE;
  int row, i;
  const char *utf;
  gunichar c;

  *change = NULL;

  text = focus->text;

  switch(keyval) {
      case GDK_KEY_Up:
      case GDK_KEY_KP_Up:
        text->cursor_row--;
        if (text->cursor_row<0)
          text->cursor_row = 0;

        if (text->cursor_pos > text_get_line_strlen (text, text->cursor_row)) {
          text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
        }

        break;
      case GDK_KEY_Down:
      case GDK_KEY_KP_Down:
        text->cursor_row++;
        if (text->cursor_row >= text->numlines)
          text->cursor_row = text->numlines - 1;

        if (text->cursor_pos > text_get_line_strlen (text, text->cursor_row)) {
          text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
        }

        break;
      case GDK_KEY_Left:
      case GDK_KEY_KP_Left:
        if (keystate & GDK_CONTROL_MASK) {
          text_move_cursor (text, WORD_START);
        } else {
          text->cursor_pos--;
        }
        if (text->cursor_pos < 0) {
          text->cursor_pos = 0;
        }
        break;
      case GDK_KEY_Right:
      case GDK_KEY_KP_Right:
        if (keystate & GDK_CONTROL_MASK) {
          text_move_cursor(text, WORD_END);
        } else {
          text->cursor_pos++;
        }

        if (text->cursor_pos > text_get_line_strlen (text, text->cursor_row)) {
          text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
        }
        break;
      case GDK_KEY_Home:
      case GDK_KEY_KP_Home:
        text->cursor_pos = 0;
        break;
      case GDK_KEY_End:
      case GDK_KEY_KP_End:
        text->cursor_pos = text_get_line_strlen (text, text->cursor_row);
        break;
      case GDK_KEY_Delete:
      case GDK_KEY_KP_Delete:
        return_val = text_delete_key_handler (focus, change);
        break;
      case GDK_KEY_BackSpace:
        return_val = TRUE;
        row = text->cursor_row;
        if (text->cursor_pos <= 0) {
          if (row > 0) {
            *change = text_create_change (text, TYPE_JOIN_ROW, 'Q',
                                          text_get_line_strlen (text, row-1),
                                          row-1,
                                          focus->obj);
          } else {
            return_val = FALSE;
            break;
          }
        } else {
          utf = text_get_line (text, row);
          for (i = 0; i < (text->cursor_pos - 1); i++) {
            utf = g_utf8_next_char (utf);
          }
          c = g_utf8_get_char (utf);
          *change = text_create_change (text, TYPE_DELETE_BACKWARD, c,
                                        text->cursor_pos - 1,
                                        text->cursor_row,
                                        focus->obj);
        }
        text_delete_backward (text);
        break;
      case GDK_KEY_Return:
      case GDK_KEY_KP_Enter:
        return_val = TRUE;
        *change = text_create_change (text, TYPE_SPLIT_ROW, 'Q',
                                      text->cursor_pos, text->cursor_row,
                                      focus->obj);
        text_split_line (text);
        break;
      case GDK_KEY_Shift_L:
      case GDK_KEY_Shift_R:
      case GDK_KEY_Control_L:
      case GDK_KEY_Control_R:
      case GDK_KEY_Alt_L:
      case GDK_KEY_Alt_R:
      case GDK_KEY_Meta_L:
      case GDK_KEY_Meta_R:
        return_val = FALSE; /* no text change for modifiers */
        break;
      default:
        if (str || (strlen>0)) {
          if (str && *str == '\r') {
            break; /* avoid putting junk into our string */
          }
          return_val = TRUE;
          *change = dia_object_change_list_new ();
          for (utf = str; utf && *utf && strlen > 0 ;
               utf = g_utf8_next_char (utf), strlen--) {
            DiaObjectChange *step;
            c = g_utf8_get_char (utf);

            step = text_create_change (text, TYPE_INSERT_CHAR, c,
                                       text->cursor_pos, text->cursor_row,
                                       focus->obj);
            dia_object_change_list_add (DIA_OBJECT_CHANGE_LIST (*change),
                                        step);
            text_insert_char (text, c);
          }
        }
        break;
  }

  return return_val;
}


int
text_is_empty (const Text *text)
{
  for (int i = 0; i < text->numlines; i++) {
    if (text_get_line_strlen (text, i) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}


int
text_delete_all (Text *text, DiaObjectChange **change, DiaObject *obj)
{
  if (!text_is_empty (text)) {
    *change = text_create_change (text, TYPE_DELETE_ALL,
                                  0, text->cursor_pos, text->cursor_row,
                                  obj);

    text_set_string (text, "");
    calc_ascent_descent (text);
    return TRUE;
  }
  return FALSE;
}


void
data_add_text (AttributeNode attr, Text *text, DiaContext *ctx)
{
  DataNode composite;
  char *str;

  composite = data_add_composite(attr, "text", ctx);

  str = text_get_string_copy(text);
  data_add_string(composite_add_attribute(composite, "string"),
		  str, ctx);
  g_clear_pointer (&str, g_free);
  data_add_font(composite_add_attribute(composite, "font"),
		text->font, ctx);
  data_add_real(composite_add_attribute(composite, "height"),
		text->height, ctx);
  data_add_point(composite_add_attribute(composite, "pos"),
		 &text->position, ctx);
  data_add_color(composite_add_attribute(composite, "color"),
		 &text->color, ctx);
  data_add_enum(composite_add_attribute(composite, "alignment"),
		text->alignment, ctx);
}


Text *
data_text (AttributeNode text_attr, DiaContext *ctx)
{
  char *string = NULL;
  DiaFont *font;
  double height;
  Point pos = { 0.0, 0.0 };
  Color col;
  DiaAlignment align;
  AttributeNode attr;
  Text *text;

  attr = composite_find_attribute (text_attr, "string");
  if (attr != NULL) {
    string = data_string (attribute_first_data (attr), ctx);
  }

  height = 1.0;
  attr = composite_find_attribute (text_attr, "height");
  if (attr != NULL) {
    height = data_real (attribute_first_data (attr), ctx);
  }

  attr = composite_find_attribute (text_attr, "font");
  if (attr != NULL) {
    font = data_font (attribute_first_data (attr), ctx);
  } else {
    font = dia_font_new_from_style (DIA_FONT_SANS,1.0);
  }

  attr = composite_find_attribute (text_attr, "pos");
  if (attr != NULL) {
    data_point (attribute_first_data (attr), &pos, ctx);
  }

  col = color_black;
  attr = composite_find_attribute (text_attr, "color");
  if (attr != NULL) {
    data_color (attribute_first_data (attr), &col, ctx);
  }

  align = DIA_ALIGN_LEFT;
  attr = composite_find_attribute (text_attr, "alignment");
  if (attr != NULL) {
    align = data_enum (attribute_first_data (attr), ctx);
  }

  text = new_text (string ? string : "", font, height, &pos, &col, align);

  g_clear_object (&font);
  g_clear_pointer (&string, g_free);

  return text;
}


void
text_get_attributes (Text *text, TextAttributes *attr)
{
  DiaFont *old_font;
  old_font = attr->font;
  attr->font = g_object_ref (text->font);
  g_clear_object (&old_font);
  attr->height = text->height;
  attr->position = text->position;
  attr->color = text->color;
  attr->alignment = text->alignment;
}


void
text_set_attributes (Text *text, TextAttributes *attr)
{
  if (text->font != attr->font) {
    text_set_font (text, attr->font);
  }
  text_set_height (text, attr->height);
  text->position = attr->position;
  text->color = attr->color;
  text->alignment = attr->alignment;
}


static void
dia_text_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaTextObjectChange *change = DIA_TEXT_OBJECT_CHANGE (self);
  Text *text = change->text;

  dia_object_get_properties (change->obj, change->props);

  switch (change->type) {
    case TYPE_INSERT_CHAR:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_insert_char (text, change->ch);
      break;
    case TYPE_DELETE_BACKWARD:
      text->cursor_pos = change->pos+1;
      text->cursor_row = change->row;
      text_delete_backward (text);
      break;
    case TYPE_DELETE_FORWARD:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_delete_forward (text);
      break;
    case TYPE_SPLIT_ROW:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_split_line (text);
      break;
    case TYPE_JOIN_ROW:
      text_join_lines (text, change->row);
      break;
    case TYPE_DELETE_ALL:
      set_string (text, "");
      text->cursor_pos = 0;
      text->cursor_row = 0;
      break;
    default:
      g_return_if_reached ();
  }
}


static void
dia_text_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaTextObjectChange *change = DIA_TEXT_OBJECT_CHANGE (self);
  Text *text = change->text;

  switch (change->type) {
    case TYPE_INSERT_CHAR:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_delete_forward(text);
      break;
    case TYPE_DELETE_BACKWARD:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_insert_char(text, change->ch);
      break;
    case TYPE_DELETE_FORWARD:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_insert_char(text, change->ch);
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      break;
    case TYPE_SPLIT_ROW:
      text_join_lines(text, change->row);
      break;
    case TYPE_JOIN_ROW:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_split_line(text);
      break;
    case TYPE_DELETE_ALL:
      set_string(text, change->str);
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      break;
    default:
      g_return_if_reached ();
  }
  /* restore previous position/size */
  dia_object_set_properties (change->obj, change->props);
}


static void
dia_text_object_change_free (DiaObjectChange *self)
{
  DiaTextObjectChange *change = DIA_TEXT_OBJECT_CHANGE (self);

  g_clear_pointer (&change->str, g_free);
  prop_list_free (change->props);
}


/* If some object does not properly resize when undoing
 * text changes consider adding some additional properties.
 *
 * The list can contain more properties than supported by
 * the specific object, the'll get ignored than.
 */
static PropDescription _prop_descs[] = {
  { "elem_corner", PROP_TYPE_POINT },
  { "elem_width", PROP_TYPE_REAL },
  { "elem_height", PROP_TYPE_REAL },
    PROP_DESC_END
};


static GPtrArray *
make_posision_and_size_prop_list (void)
{
  GPtrArray *props;

  props = prop_list_from_descs (_prop_descs, pdtpp_true);

  return props;
}


static DiaObjectChange *
text_create_change (Text           *text,
                    TextChangeType  type,
                    gunichar        ch,
                    int             pos,
                    int             row,
                    DiaObject      *obj)
{
  DiaTextObjectChange *change;

  change = dia_object_change_new (DIA_TYPE_TEXT_OBJECT_CHANGE);

  change->obj = obj;
  change->props = make_posision_and_size_prop_list ();
  /* remember previous position/size */
  dia_object_get_properties (change->obj, change->props);

  change->text = text;
  change->type = type;
  change->ch = ch;
  change->pos = pos;
  change->row = row;

  if (type == TYPE_DELETE_ALL) {
    change->str = text_get_string_copy (text);
  } else {
    change->str = NULL;
  }

  return DIA_OBJECT_CHANGE (change);
}


gboolean
apply_textattr_properties (GPtrArray      *props,
                           Text           *text,
                           const char     *textname,
                           TextAttributes *attrs)
{
  TextProperty *textprop =
    (TextProperty *) find_prop_by_name_and_type (props, textname, PROP_TYPE_TEXT);

  if ((!textprop) ||
      ((textprop->common.experience & (PXP_LOADED|PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    text_set_attributes (text, attrs);
    return TRUE;
  }
  return FALSE;
}


gboolean
apply_textstr_properties (GPtrArray  *props,
                          Text       *text,
                          const char *textname,
                          const char *str)
{
  TextProperty *textprop =
    (TextProperty *) find_prop_by_name_and_type (props, textname, PROP_TYPE_TEXT);

  if ((!textprop) ||
      ((textprop->common.experience & (PXP_LOADED | PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    text_set_string (text, str);
    return TRUE;
  }

  return FALSE;
}

