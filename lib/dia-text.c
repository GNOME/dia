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
#include "diarenderer.h"
#include "diainteractiverenderer.h"
#include "diagramdata.h"
#include "textline.h"
#include "attributes.h"
#include "object.h"
#include "dia-object-change-list.h"
#include "focus.h"

#include "dia-text.h"


/**
 * DiaText:
 *
 * The Text object provides high level service for on canvas text editing.
 *
 * It is used in various #DiaObject implementations, but also part of the
 * #DiaRenderer interface.
 */
struct _DiaText {
  DiaPart parent;

  /* Text data: */
  GPtrArray *lines;

  /* Attributes: */
  DiaFont *font;
  double height;
  Point position;
  DiaColour colour;
  DiaAlignment alignment;

  /* Cursor pos: */
  int cursor_pos;
  int cursor_row;
  Focus focus;

  /* Computed values:  */
  double ascent; /* **average** ascent */
  double descent; /* **average** descent */
  double max_width;
};


G_DEFINE_TYPE (DiaText, dia_text, DIA_TYPE_PART)


static void
dia_text_clear (DiaPart *part)
{
  DiaText *self = DIA_TEXT (part);

  g_clear_pointer (&self->lines, g_ptr_array_unref);
  g_clear_object (&self->font);

  DIA_PART_CLASS (dia_text_parent_class)->clear (part);
}


static void
dia_text_class_init (DiaTextClass *klass)
{
  DiaPartClass *part_class = DIA_PART_CLASS (klass);

  part_class->clear = dia_text_clear;
}


static void
dia_text_init (DiaText *self)
{

}


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

  DiaText *text;
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
 * dia_text_get_line:
 * @text: the #Text
 * @line: the line number in @text to fetch
 *
 * Encapsulation functions for transferring to text_line
 *
 * Since: 0.98
 */
const char *
dia_text_get_line (DiaText *self, int line)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), NULL);

  return text_line_get_string (g_ptr_array_index (self->lines, line));
}


/**
 * text_set_line:
 *
 * Raw sets one line to a given text, not copying, not freeing.
 *
 * Since: 0.98
 */
static void
text_set_line (DiaText *self, int line_no, char *line)
{
  text_line_set_string (g_ptr_array_index (self->lines, line_no), line);
}


/**
 * text_set_line_text:
 *
 * Set the text of a line, freeing, copying and mallocing as required.
 * Updates strlen and row_width entries, but not max_width.
 *
 * Since: 0.98
 */
static void
text_set_line_text (DiaText *self, int line_no, char *line)
{
  text_set_line (self, line_no, line);
}


/**
 * text_delete_line:
 *
 * Delete the line, freeing appropriately and moving stuff up.
 * This function circumvents the normal free/alloc cycle of
 * text_set_line_text.
 *
 * Since: 0.98
 */
static void
text_delete_line (DiaText *self, int line_no)
{
  g_ptr_array_remove_index (self->lines, line_no);
}


/**
 * text_insert_line:
 *
 * Insert a new (empty) line at line_no.
 * This function circumvents the normal free/alloc cycle of
 * text_set_line_text.
 *
 * Since: 0.98
 */
static void
text_insert_line (DiaText *self, int line_no)
{
  g_ptr_array_insert (self->lines,
                      line_no,
                      text_line_new ("", self->font, self->height));
}


/**
 * dia_text_get_line_width:
 * @text: The text object;
 * @line_no: The index of the line in the text object, starting at 0.
 *
 * Get the in-diagram width of the given line.
 *
 * Returns: The width in cm of the indicated line.
 *
 * Since: 0.98
 */
double
dia_text_get_line_width (DiaText *self, int line_no)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);

  return text_line_get_width (g_ptr_array_index (self->lines, line_no));
}


/**
 * dia_text_get_line_strlen:
 * @text: The text object;
 * @line_no: The index of the line in the text object, starting at 0.
 *
 * Get the number of characters of the given line.
 *
 * Returns: The number of UTF-8 characters of the indicated line.
 *
 * Since: 0.98
 */
size_t
dia_text_get_line_strlen (DiaText *self, int line_no)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0);

  return g_utf8_strlen (text_line_get_string (g_ptr_array_index (self->lines, line_no)), -1);
}


double
dia_text_get_max_width (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);

  return self->max_width;
}


/**
 * dia_text_get_ascent:
 * @text: Text object
 *
 * Get the *average* ascent of this #Text object.
 *
 * Returns: the average of the ascents of each line (height above baseline)
 */
double
dia_text_get_ascent (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);

  return self->ascent;
}


/**
 * dia_text_get_descent:
 * @text: a #Text object
 *
 * Get the *average* descent of this Text object.
 *
 * Returns: the average of the descents of each line (height below baseline)
 *
 * Since: 0.98
 */
double
dia_text_get_descent (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);

  return self->descent;
}


static DiaObjectChange *text_create_change (DiaText        *text,
                                            TextChangeType  type,
                                            gunichar        ch,
                                            int             pos,
                                            int             row,
                                            DiaObject      *obj);


static void
calc_width (DiaText *self)
{
  double width = 0.0;

  for (size_t i = 0; i < self->lines->len; i++) {
    width = MAX (width, dia_text_get_line_width (self, i));
  }

  self->max_width = width;
}


static void
calc_ascent_descent (DiaText *self)
{
  double sig_a = 0.0, sig_d = 0.0;

  for (size_t i = 0; i < self->lines->len; i++) {
    sig_a += text_line_get_ascent (g_ptr_array_index (self->lines, i));
    sig_d += text_line_get_descent (g_ptr_array_index (self->lines, i));
  }

  self->ascent = sig_a / (double) self->lines->len;
  self->descent = sig_d / (double) self->lines->len;
}


static void
set_string (DiaText *self, const char *string)
{
  int numlines;
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
  if (s != NULL) {
    while ((s = g_utf8_strchr (s, -1, '\n')) != NULL) {
      numlines++;
      if (*s) {
        s = g_utf8_next_char (s);
      }
    }
  }

  g_clear_pointer (&self->lines, g_ptr_array_unref);
  self->lines =
    g_ptr_array_new_null_terminated (numlines, (GDestroyNotify) text_line_destroy, TRUE);
  for (int i = 0; i < numlines; i++) {
    g_ptr_array_add (self->lines,
                     text_line_new ("", self->font, self->height));
  }

  s = fallback ? fallback : string;
  if (s == NULL) {
    text_set_line_text (self, 0, "");
    return;
  }

  for (int i = 0; i < numlines; i++) {
    char *string_line;
    s2 = g_utf8_strchr (s, -1, '\n');
    if (s2 == NULL) { /* No newline */
      s2 = s + strlen (s);
    }
    string_line = g_strndup (s, s2 - s);
    text_set_line_text (self, i, string_line);
    g_clear_pointer (&string_line, g_free);
    s = s2;
    if (*s) {
      s = g_utf8_next_char (s);
    }
  }

  if (self->cursor_row >= self->lines->len) {
    self->cursor_row = self->lines->len - 1;
  }

  if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
    self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
  }
  g_clear_pointer (&fallback, g_free);
}


void
dia_text_set_string (DiaText *self, const char *string)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  g_clear_pointer (&self->lines, g_ptr_array_unref);

  set_string (self, string);
}


DiaText *
dia_text_new (const char   *string,
              DiaFont      *font,
              double        height,
              Point        *pos,
              DiaColour    *colour,
              DiaAlignment  align)
{
  DiaText *self = dia_part_alloc (DIA_TYPE_TEXT);

  self->lines =
    g_ptr_array_new_null_terminated (4, (GDestroyNotify) text_line_destroy, TRUE);

  g_set_object (&self->font, font);
  self->height = height;

  self->position = *pos;
  self->colour = *colour;
  self->alignment = align;

  self->cursor_pos = 0;
  self->cursor_row = 0;

  self->focus.obj = NULL;
  self->focus.has_focus = FALSE;
  self->focus.key_event = text_key_event;
  self->focus.text = self;

  set_string (self, string);

  calc_ascent_descent (self);

  return self;
}


/**
 * dia_text_new_default:
 * @pos: the #Point the text is located
 * @colour: the #DiaColour of the text
 * @align: the #DiaAlignment of the text
 *
 * Fallback function returning a default initialized text object.
 */
DiaText *
dia_text_new_default (Point *pos, DiaColour *colour, DiaAlignment align)
{
  DiaText *text;
  DiaFont *font;
  double font_height;

  attributes_get_default_font (&font, &font_height);
  text = dia_text_new ("", font, font_height, pos, colour, align);
  g_clear_object (&font);

  return text;
}


static TextLine *
dup_textline (TextLine *source, gpointer user_data)
{
  return text_line_new (text_line_get_string (source),
                        text_line_get_font (source),
                        text_line_get_height (source));
}


DiaText *
dia_text_copy (DiaText *self)
{
  DiaText *copy;

  g_return_val_if_fail (DIA_IS_TEXT (self), NULL);

  copy = dia_part_alloc (DIA_TYPE_TEXT);
  copy->lines =
    g_ptr_array_copy (self->lines, (GCopyFunc) dup_textline, NULL);

  copy->font = dia_font_copy (self->font);
  copy->height = self->height;
  copy->position = self->position;
  copy->colour = self->colour;
  copy->alignment = self->alignment;

  copy->cursor_pos = 0;
  copy->cursor_row = 0;
  copy->focus.obj = NULL;
  copy->focus.has_focus = FALSE;
  copy->focus.key_event = text_key_event;
  copy->focus.text = copy;

  copy->ascent = self->ascent;
  copy->descent = self->descent;
  copy->max_width = self->max_width;

  return copy;
}


void
dia_text_set_height (DiaText *self, double height)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  self->height = height;
  for (size_t i = 0; i < self->lines->len; i++) {
    text_line_set_height (g_ptr_array_index (self->lines, i), height);
  }
  calc_width (self);
  calc_ascent_descent (self);
}


double
dia_text_get_height (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);

  return self->height;
}


void
dia_text_set_font (DiaText *self, DiaFont *font)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  g_set_object (&self->font, font);

  for (size_t i = 0; i < self->lines->len; i++) {
    text_line_set_font (g_ptr_array_index (self->lines, i), font);
  }

  calc_width (self);
  calc_ascent_descent (self);
}


DiaFont *
dia_text_get_font (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), NULL);

  return self->font;
}


void
dia_text_set_position (DiaText *self, Point *pos)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (pos != NULL);

  self->position = *pos;
}


/**
 * dia_text_get_position:
 * @self: the #Text
 * @pos: (out): text position
 *
 * Since: 0.98
 */
void
dia_text_get_position (DiaText *self,
                       Point   *pos)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (pos != NULL);

  *pos = self->position;
}


void
dia_text_set_colour (DiaText *self, DiaColour *col)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (col != NULL);

  self->colour = *col;
}


/**
 * dia_text_get_colour:
 * @self: the #Text
 * @colour: (out): active text colour
 *
 * Since: 0.98
 */
void
dia_text_get_colour (DiaText   *self,
                     DiaColour *colour)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (colour != NULL);

  *colour = self->colour;
}


void
dia_text_set_alignment (DiaText *self, DiaAlignment align)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  self->alignment = align;
}


DiaAlignment
dia_text_get_alignment (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), DIA_ALIGN_CENTRE);

  return self->alignment;
}


void
dia_text_calc_boundingbox (DiaText *self, DiaRectangle *box)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  calc_width (self);
  calc_ascent_descent (self);

  if (box == NULL) {
    return; /* For those who just want the text info
               updated */
  }

  box->left = self->position.x;

  switch (self->alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      box->left -= self->max_width / 2.0;
      break;
    case DIA_ALIGN_RIGHT:
      box->left -= self->max_width;
      break;
    default:
      g_return_if_reached ();
  }

  box->right = box->left + self->max_width;

  box->top = self->position.y - self->ascent;
#if 0
  box->bottom = box->top + self->height*self->lines->len + self->descent;
#else
  /* why should we add one descent? isn't ascent+descent~=height? */
  box->bottom = box->top + (self->ascent+self->descent+self->height*(self->lines->len-1));
#endif
  if (self->focus.has_focus) {
    double height = self->ascent + self->descent;
    if (self->cursor_pos == 0) {
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
dia_text_get_string_copy (DiaText *self)
{
  int num;
  char *str;

  g_return_val_if_fail (DIA_IS_TEXT (self), NULL);

  num = 0;
  for (size_t i = 0; i < self->lines->len; i++) {
    /* This is for allocation, so it should not use g_utf8_strlen() */
    num += strlen (dia_text_get_line (self, i)) + 1;
  }

  str = g_new0 (char, num);

  for (size_t i = 0; i < self->lines->len; i++) {
    strcat (str, dia_text_get_line (self, i));
    if (i != (self->lines->len - 1)) {
      strcat (str, "\n");
    }
  }

  return str;
}


double
dia_text_distance_from (DiaText *self, Point *point)
{
  double dx, dy;
  double topy, bottomy;
  double left, right;
  int line;

  g_return_val_if_fail (DIA_IS_TEXT (self), 0.0);
  g_return_val_if_fail (point != NULL, 0.0);

  topy = self->position.y - self->ascent;
  bottomy = self->position.y + self->descent + self->height *
                (self->lines->len - 1);
  if (point->y <= topy) {
    dy = topy - point->y;
    line = 0;
  } else if (point->y >= bottomy) {
    dy = point->y - bottomy;
    line = self->lines->len - 1;
  } else {
    dy = 0.0;
    line = (int) floor ((point->y - topy) / self->height);
    if (line >= self->lines->len) {
      line = self->lines->len - 1;
    }
  }

  left = self->position.x;
  switch (self->alignment) {
    case DIA_ALIGN_LEFT:
      break;
    case DIA_ALIGN_CENTRE:
      left -= dia_text_get_line_width (self, line) / 2.0;
      break;
    case DIA_ALIGN_RIGHT:
      left -= dia_text_get_line_width (self, line);
      break;
    default:
      g_return_val_if_reached (0.0);
  }
  right = left + dia_text_get_line_width (self, line);

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
dia_text_draw (DiaText *self, DiaRenderer *renderer)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (DIA_IS_RENDERER (renderer));

  dia_renderer_draw_text (renderer, self);

  if (DIA_IS_INTERACTIVE_RENDERER (renderer) && (self->focus.has_focus)) {
    double curs_x, curs_y;
    double str_width_first;
    double str_width_whole;
    Point p1, p2;
    double height = self->ascent+self->descent;
    curs_y = self->position.y - self->ascent + self->cursor_row*self->height;

    dia_renderer_set_font (renderer, self->font, self->height);

    str_width_first = dia_renderer_get_text_width (renderer,
                                                   dia_text_get_line (self, self->cursor_row),
                                                   self->cursor_pos);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   dia_text_get_line (self, self->cursor_row),
                                                   dia_text_get_line_strlen (self, self->cursor_row));
    curs_x = self->position.x + str_width_first;

    switch (self->alignment) {
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
    dia_renderer_draw_line (renderer, &p1, &p2, &DIA_COLOUR_BLACK);
  }
}


void
dia_text_grab_focus (DiaText *self, DiaObject *object)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  self->focus.obj = object;
  request_focus (&self->focus);
}


void
dia_text_set_cursor_at_end (DiaText *self)
{
  g_return_if_fail (DIA_IS_TEXT (self));

  self->cursor_row = self->lines->len - 1;
  self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
}


typedef enum {
  WORD_START = 1,
  WORD_END
} CursorMovement;


static void
text_move_cursor (DiaText *self, CursorMovement mv)
{
  const char *str =
    text_line_get_string (g_ptr_array_index (self->lines, self->cursor_row));
  const char *p = str;
  int curmax = dia_text_get_line_strlen (self, self->cursor_row);

  if (self->cursor_pos > 0 && self->cursor_pos <= curmax) {
    for (int i = 0; i < self->cursor_pos; i++) {
      p = g_utf8_next_char (p);
    }
  }

  if (WORD_START == mv && self->cursor_pos < 1) {
    if (self->cursor_row) {
      self->cursor_row--;
      self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
    }
    return;
  } else if (WORD_END == mv && self->cursor_pos == curmax) {
    if (self->cursor_row < self->lines->len - 1) {
      self->cursor_row++;
      self->cursor_pos = 0;
    }
    return;
  }

  while (!g_unichar_isalnum (g_utf8_get_char (p))) {
    p = (WORD_START == mv ? g_utf8_find_prev_char (str, p) : g_utf8_next_char (p));

    if (p) {
      self->cursor_pos += (WORD_START == mv ? -1 : 1);
    }

    if (!p || !*p) {
      return;
    }

    if (!self->cursor_pos || self->cursor_pos == curmax) {
      return;
    }
  }

  while (g_unichar_isalnum (g_utf8_get_char (p))) {
    p = (WORD_START == mv ? g_utf8_find_prev_char (str, p) : g_utf8_next_char (p));

    if (p) {
      self->cursor_pos += (WORD_START == mv ? -1 : 1);
    }

    if (!p || !*p) {
      return;
    }

    if (!self->cursor_pos || self->cursor_pos == curmax) {
      return;
    }
  }
}


/* The renderer is only used to determine where the click is, so is not
 * required when no point is given. */
void
dia_text_set_cursor (DiaText     *self,
                     Point       *clicked_point,
                     DiaRenderer *renderer)
{
  double str_width_whole;
  double str_width_first;
  double top;
  double start_x;
  int row;
  int i;

  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (DIA_IS_RENDERER (renderer));

  if (clicked_point != NULL) {
    top = self->position.y - self->ascent;

    row = (int) floor ((clicked_point->y - top) / self->height);

    if (row < 0)
      row = 0;

    if (row >= self->lines->len) {
      row = self->lines->len - 1;
    }

    self->cursor_row = row;
    self->cursor_pos = 0;

    if (!DIA_IS_INTERACTIVE_RENDERER (renderer)) {
      g_warning ("Internal error: Select gives non interactive renderer!\n"
                 "renderer: %s", g_type_name (G_TYPE_FROM_INSTANCE (renderer)));
      return;
    }


    dia_renderer_set_font (renderer, self->font, self->height);
    str_width_whole = dia_renderer_get_text_width (renderer,
                                                   dia_text_get_line (self, row),
                                                   dia_text_get_line_strlen (self, row));
    start_x = self->position.x;
    switch (self->alignment) {
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
      for (i = 0; i <= dia_text_get_line_strlen (self, row); i++) {
        double dist;
        str_width_first = dia_renderer_get_text_width (renderer, dia_text_get_line (self, row), i);
        dist = fabs (clicked_point->x - (start_x + str_width_first));
        if (dist < min_dist) {
          min_dist = dist;
          self->cursor_pos = i;
        } else {
          return;
        }
      }
    }
    self->cursor_pos = dia_text_get_line_strlen (self, row);
  } else {
    /* No clicked point, leave cursor where it is */
  }
}


static void
text_join_lines (DiaText *self, int first_line)
{
  char *combined_line;
  int len1;

  len1 = dia_text_get_line_strlen (self, first_line);

  combined_line = g_strconcat (dia_text_get_line (self, first_line),
                               dia_text_get_line (self, first_line + 1), NULL);
  text_delete_line (self, first_line);
  text_set_line_text (self, first_line, combined_line);
  g_clear_pointer (&combined_line, g_free);

  self->max_width = MAX (self->max_width,
                         dia_text_get_line_width (self, first_line));

  self->cursor_row = first_line;
  self->cursor_pos = len1;
}


static void
text_delete_forward (DiaText *self)
{
  int row;
  double width;
  const char *line;
  char *utf8_before, *utf8_after;
  char *str1, *str;

  row = self->cursor_row;

  if (self->cursor_pos >= dia_text_get_line_strlen (self, row)) {
    if (row + 1 < self->lines->len) {
      text_join_lines (self, row);
    }
    return;
  }

  line = dia_text_get_line (self, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong)(self->cursor_pos));
  utf8_after = g_utf8_offset_to_pointer (utf8_before, 1);
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, utf8_after, NULL);
  text_set_line_text (self, row, str);
  g_clear_pointer (&str1, g_free);
  g_clear_pointer (&str, g_free);

  if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
    self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
  }

  width = 0.0;
  for (int i = 0; i < self->lines->len; i++) {
    width = MAX (width, dia_text_get_line_width (self, i));
  }
  self->max_width = width;
}


static void
text_delete_backward (DiaText *self)
{
  int row;
  double width;
  const char *line;
  char *utf8_before, *utf8_after;
  char *str1, *str;

  row = self->cursor_row;

  if (self->cursor_pos <= 0) {
    if (row > 0) {
      text_join_lines (self, row - 1);
    }
    return;
  }

  line = dia_text_get_line (self, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (self->cursor_pos - 1));
  utf8_after = g_utf8_offset_to_pointer (utf8_before, 1);
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, utf8_after, NULL);
  text_set_line_text (self, row, str);
  g_clear_pointer (&str, g_free);
  g_clear_pointer (&str1, g_free);

  self->cursor_pos --;
  if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
    self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
  }

  width = 0.0;
  for (int i = 0; i < self->lines->len; i++) {
    width = MAX (width, dia_text_get_line_width (self, i));
  }
  self->max_width = width;
}


static void
text_split_line (DiaText *self)
{
  const char *line;
  double width;
  char *utf8_before;
  char *str1, *str2;

  /* Split the lines at cursor_pos */
  line = dia_text_get_line (self, self->cursor_row);
  text_insert_line (self, self->cursor_row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (self->cursor_pos));
  str1 = g_strndup (line, utf8_before - line);
  str2 = g_strdup (utf8_before); /* Must copy before dealloc */
  text_set_line_text (self, self->cursor_row, str1);
  text_set_line_text (self, self->cursor_row + 1, str2);
  g_clear_pointer (&str2, g_free);
  g_clear_pointer (&str1, g_free);

  self->cursor_row ++;
  self->cursor_pos = 0;

  width = 0.0;
  for (int i = 0; i < self->lines->len; i++) {
    width = MAX (width, dia_text_get_line_width (self, i));
  }
  self->max_width = width;
}


static void
text_insert_char (DiaText *self, gunichar c)
{
  char ch[7];
  int unilen;
  int row;
  const char *line;
  char *str;
  char *utf8_before;
  char *str1;

  /* Make a string of the the char */
  unilen = g_unichar_to_utf8 (c, ch);
  ch[unilen] = 0;

  row = self->cursor_row;

  /* Copy the before and after parts with the new char in between */
  line = dia_text_get_line (self, row);
  utf8_before = g_utf8_offset_to_pointer (line, (glong) (self->cursor_pos));
  str1 = g_strndup (line, utf8_before - line);
  str = g_strconcat (str1, ch, utf8_before, NULL);
  text_set_line_text (self, row, str);
  g_clear_pointer (&str, g_free);
  g_clear_pointer (&str1, g_free);

  self->cursor_pos++;
  self->max_width = MAX (self->max_width,
                         dia_text_get_line_width (self, row));
}


gboolean
text_delete_key_handler (Focus *focus, DiaObjectChange **change)
{
  DiaText *self;
  int row, i;
  const char *utf;
  gunichar c;

  self = focus->text;
  row = self->cursor_row;
  if (self->cursor_pos >= dia_text_get_line_strlen (self, row)) {
    if (row+1 < self->lines->len) {
      *change = text_create_change (self, TYPE_JOIN_ROW, 'Q',
                                    self->cursor_pos, row, focus->obj);
    } else {
      return FALSE;
    }
  } else {
    utf = dia_text_get_line (self, row);
    for (i = 0; i < self->cursor_pos; i++) {
      utf = g_utf8_next_char (utf);
    }
    c = g_utf8_get_char (utf);
    *change = text_create_change (self, TYPE_DELETE_FORWARD, c,
                                  self->cursor_pos, self->cursor_row, focus->obj);
  }
  text_delete_forward (self);
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
  DiaText *self;
  int return_val = FALSE;
  int row, i;
  const char *utf;
  gunichar c;

  *change = NULL;

  self = focus->text;

  switch (keyval) {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
      self->cursor_row--;
      if (self->cursor_row < 0) {
        self->cursor_row = 0;
      }

      if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
        self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
      }

      break;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
      self->cursor_row++;
      if (self->cursor_row >= self->lines->len) {
        self->cursor_row = self->lines->len - 1;
      }

      if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
        self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
      }

      break;
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
      if (keystate & GDK_CONTROL_MASK) {
        text_move_cursor (self, WORD_START);
      } else {
        self->cursor_pos--;
      }

      if (self->cursor_pos < 0) {
        self->cursor_pos = 0;
      }
      break;
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
      if (keystate & GDK_CONTROL_MASK) {
        text_move_cursor (self, WORD_END);
      } else {
        self->cursor_pos++;
      }

      if (self->cursor_pos > dia_text_get_line_strlen (self, self->cursor_row)) {
        self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
      }
      break;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:
      self->cursor_pos = 0;
      break;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:
      self->cursor_pos = dia_text_get_line_strlen (self, self->cursor_row);
      break;
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:
      return_val = text_delete_key_handler (focus, change);
      break;
    case GDK_KEY_BackSpace:
      return_val = TRUE;
      row = self->cursor_row;
      if (self->cursor_pos <= 0) {
        if (row > 0) {
          *change = text_create_change (self,
                                        TYPE_JOIN_ROW,
                                        'Q',
                                        dia_text_get_line_strlen (self, row - 1),
                                        row - 1,
                                        focus->obj);
        } else {
          return_val = FALSE;
          break;
        }
      } else {
        utf = dia_text_get_line (self, row);
        for (i = 0; i < (self->cursor_pos - 1); i++) {
          utf = g_utf8_next_char (utf);
        }
        c = g_utf8_get_char (utf);
        *change = text_create_change (self,
                                      TYPE_DELETE_BACKWARD,
                                      c,
                                      self->cursor_pos - 1,
                                      self->cursor_row,
                                      focus->obj);
      }
      text_delete_backward (self);
      break;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
      return_val = TRUE;
      *change = text_create_change (self,
                                    TYPE_SPLIT_ROW,
                                    'Q',
                                    self->cursor_pos,
                                    self->cursor_row,
                                    focus->obj);
      text_split_line (self);
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
      if (str || (strlen > 0)) {
        if (str && *str == '\r') {
          break; /* avoid putting junk into our string */
        }
        return_val = TRUE;
        *change = dia_object_change_list_new ();
        for (utf = str; utf && *utf && strlen > 0;
             utf = g_utf8_next_char (utf), strlen--) {
          DiaObjectChange *step;
          c = g_utf8_get_char (utf);

          step = text_create_change (self,
                                     TYPE_INSERT_CHAR,
                                     c,
                                     self->cursor_pos,
                                     self->cursor_row,
                                     focus->obj);
          dia_object_change_list_add (DIA_OBJECT_CHANGE_LIST (*change),
                                      step);
          text_insert_char (self, c);
        }
      }
      break;
  }

  return return_val;
}


gboolean
dia_text_is_empty (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), FALSE);

  for (int i = 0; i < self->lines->len; i++) {
    if (dia_text_get_line_strlen (self, i) != 0) {
      return FALSE;
    }
  }

  return TRUE;
}


gboolean
dia_text_delete_all (DiaText          *self,
                     DiaObjectChange **change,
                     DiaObject        *obj)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), FALSE);
  g_return_val_if_fail (change != NULL, FALSE);
  g_return_val_if_fail (obj != NULL, FALSE);

  if (!dia_text_is_empty (self)) {
    *change = text_create_change (self,
                                  TYPE_DELETE_ALL,
                                  0,
                                  self->cursor_pos,
                                  self->cursor_row,
                                  obj);

    dia_text_set_string (self, "");
    calc_ascent_descent (self);
    return TRUE;
  }

  return FALSE;
}


void
data_add_text (AttributeNode attr, DiaText *text, DiaContext *ctx)
{
  DataNode composite;
  char *str;

  g_return_if_fail (DIA_IS_TEXT (text));

  composite = data_add_composite(attr, "text", ctx);

  str = dia_text_get_string_copy (text);
  data_add_string (composite_add_attribute (composite, "string"),
                   str,
                   ctx);
  g_clear_pointer (&str, g_free);
  data_add_font (composite_add_attribute (composite, "font"),
                 text->font,
                 ctx);
  data_add_real (composite_add_attribute (composite, "height"),
                 text->height,
                 ctx);
  data_add_point (composite_add_attribute (composite, "pos"),
                  &text->position,
                  ctx);
  data_add_color (composite_add_attribute (composite, "color"),
                  &text->colour,
                  ctx);
  data_add_enum (composite_add_attribute (composite, "alignment"),
                 text->alignment,
                 ctx);
}


DiaText *
data_text (AttributeNode text_attr, DiaContext *ctx)
{
  char *string = NULL;
  DiaFont *font;
  double height;
  Point pos = { 0.0, 0.0 };
  DiaColour col;
  DiaAlignment align;
  AttributeNode attr;
  DiaText *text;

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

  col = DIA_COLOUR_BLACK;
  attr = composite_find_attribute (text_attr, "color");
  if (attr != NULL) {
    data_color (attribute_first_data (attr), &col, ctx);
  }

  align = DIA_ALIGN_LEFT;
  attr = composite_find_attribute (text_attr, "alignment");
  if (attr != NULL) {
    align = data_enum (attribute_first_data (attr), ctx);
  }

  text = dia_text_new (string ? string : "",
                       font,
                       height,
                       &pos,
                       &col,
                       align);

  g_clear_object (&font);
  g_clear_pointer (&string, g_free);

  return text;
}


void
dia_text_get_attributes (DiaText *self, TextAttributes *attr)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (attr != NULL);

  g_set_object (&attr->font, self->font);
  attr->height = self->height;
  attr->position = self->position;
  attr->color = self->colour;
  attr->alignment = self->alignment;
}


void
dia_text_set_attributes (DiaText *self, TextAttributes *attr)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (attr != NULL);

  dia_text_set_font (self, attr->font);
  dia_text_set_height (self, attr->height);
  dia_text_set_position (self, &attr->position);
  dia_text_set_colour (self, &attr->color);
  dia_text_set_alignment (self, attr->alignment);
}


static void
dia_text_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaTextObjectChange *change = DIA_TEXT_OBJECT_CHANGE (self);
  DiaText *text = change->text;

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
  DiaText *text = change->text;

  switch (change->type) {
    case TYPE_INSERT_CHAR:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_delete_forward (text);
      break;
    case TYPE_DELETE_BACKWARD:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_insert_char (text, change->ch);
      break;
    case TYPE_DELETE_FORWARD:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_insert_char (text, change->ch);
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      break;
    case TYPE_SPLIT_ROW:
      text_join_lines (text, change->row);
      break;
    case TYPE_JOIN_ROW:
      text->cursor_pos = change->pos;
      text->cursor_row = change->row;
      text_split_line (text);
      break;
    case TYPE_DELETE_ALL:
      set_string (text, change->str);
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
text_create_change (DiaText        *text,
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
    change->str = dia_text_get_string_copy (text);
  } else {
    change->str = NULL;
  }

  return DIA_OBJECT_CHANGE (change);
}


gboolean
apply_textattr_properties (GPtrArray      *props,
                           DiaText        *text,
                           const char     *textname,
                           TextAttributes *attrs)
{
  TextProperty *textprop =
    (TextProperty *) find_prop_by_name_and_type (props, textname, PROP_TYPE_TEXT);

  if ((!textprop) ||
      ((textprop->common.experience & (PXP_LOADED|PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    dia_text_set_attributes (text, attrs);
    return TRUE;
  }
  return FALSE;
}


gboolean
apply_textstr_properties (GPtrArray  *props,
                          DiaText    *text,
                          const char *textname,
                          const char *str)
{
  TextProperty *textprop =
    (TextProperty *) find_prop_by_name_and_type (props, textname, PROP_TYPE_TEXT);

  if ((!textprop) ||
      ((textprop->common.experience & (PXP_LOADED | PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    dia_text_set_string (text, str);
    return TRUE;
  }

  return FALSE;
}


TextLine **
dia_text_get_lines (DiaText *self, size_t *n_lines)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), NULL);

  if (n_lines) {
    *n_lines = self->lines->len;
  }

  return (TextLine **) self->lines->pdata;
}


size_t
dia_text_get_n_lines (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), 0);

  return self->lines->len;
}


gboolean
dia_text_has_focus (DiaText *self)
{
  g_return_val_if_fail (DIA_IS_TEXT (self), FALSE);

  return self->focus.has_focus;
}


void
dia_text_get_cursor (DiaText *self, int *cursor_row, int *cursor_pos)
{
  g_return_if_fail (DIA_IS_TEXT (self));
  g_return_if_fail (cursor_row != NULL);
  g_return_if_fail (cursor_pos != NULL);

  *cursor_row = self->cursor_row;
  *cursor_pos = self->cursor_pos;
}
