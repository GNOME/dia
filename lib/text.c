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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <math.h>

#include <gdk/gdkkeysyms.h>

#include "propinternals.h"
#include "text.h"
#include "message.h"

static int text_key_event(Focus *focus, guint keysym,
			  const gchar *str, int strlen,
			  ObjectChange **change);

enum change_type {
  TYPE_DELETE_BACKWARD,
  TYPE_DELETE_FORWARD,
  TYPE_INSERT_CHAR,
  TYPE_JOIN_ROW,
  TYPE_SPLIT_ROW,
  TYPE_DELETE_ALL
};

struct TextObjectChange {
  ObjectChange obj_change;

  Text *text;
  enum change_type type;
  gunichar ch;
  int pos;
  int row;
  gchar *str;
};

static ObjectChange *text_create_change(Text *text, enum change_type type,
					gunichar ch, int pos, int row);

static void
calc_width(Text *text)
{
  real width;
  int i;

  width = 0.0;
  for (i=0;i<text->numlines;i++) {
    text->row_width[i] =
      dia_font_string_width(text->line[i], text->font, text->height);
    width = MAX(width, text->row_width[i]);
  }
  
  text->max_width = width;
}

static void
calc_ascent_descent(Text *text)
{
  real sig_a = 0.0,sig_d = 0.0;
  guint i;
    
  for ( i = 0; i < text->numlines; i++) {
      sig_a += dia_font_ascent(text->line[i], text->font, text->height);
      sig_d += dia_font_descent(text->line[i], text->font, text->height);
  }
  
  text->ascent = sig_a / (real)text->numlines;
  text->descent = sig_d / (real)text->numlines;
}

static void
free_string(Text *text)
{
  int i;

  for (i=0;i<text->numlines;i++) {
    g_free(text->line[i]);
  }

  g_free(text->line);
  text->line = NULL;
  
  g_free(text->strlen);
  text->strlen = NULL;

  g_free(text->alloclen);
  text->alloclen = NULL;

  g_free(text->row_width);
  text->row_width = NULL;
}

static void
set_string(Text *text, const char *string)
{
  int numlines, i;
  int alloclen;
  const char *s,*s2;
  
  s = string;
  
  numlines = 1;
  if (s != NULL) 
    while ( (s = g_utf8_strchr(s,-1,'\n')) != NULL ) {
      s++;
      if ((*s) != 0) {
        numlines++;
      }
    }
  text->numlines = numlines;
  text->line = g_malloc(sizeof(char *)*numlines);
  text->strlen = g_malloc(sizeof(int)*numlines);
  text->alloclen = g_malloc(sizeof(int)*numlines);
  text->row_width = g_malloc(sizeof(real)*numlines);

  s = string;

  if (string==NULL) {
    text->line[0] = (char *)g_malloc(1);
    text->line[0][0] = 0;
    text->strlen[0] = 0;
    text->alloclen[0] = 1;
    return;
  }

  for (i=0;i<numlines;i++) {
    s2 = g_utf8_strchr(s,-1,'\n');
    if (s2==NULL) {
      alloclen = strlen(s);
    } else {
      alloclen = s2 - s;
    }
    text->line[i] = (char *)g_malloc(alloclen+1);
    text->alloclen[i] = alloclen+1;
    strncpy (text->line[i], s, alloclen);
    text->line[i][alloclen] = 0; /* end line with \0 */
    text->strlen[i] = g_utf8_strlen(text->line[i], -1);;
    s = s2+1;
  }

  if (text->cursor_row >= text->numlines) {
    text->cursor_row = text->numlines - 1;
  }
  
  if (text->cursor_pos > text->strlen[text->cursor_row]) {
    text->cursor_pos = text->strlen[text->cursor_row];
  }
}

void
text_set_string(Text *text, const char *string)
{
  if (text->line != NULL)
    free_string(text);
  
  set_string(text, string);

  calc_width(text);
}

Text *
new_text(const char *string, DiaFont *font, real height,
	 Point *pos, Color *color, Alignment align)
{
  Text *text;

  text = g_new(Text, 1);

  text->font = dia_font_ref(font);
  text->height = height;

  text->position = *pos;
  text->color = *color;
  text->alignment = align;

  text->cursor_pos = 0;
  text->cursor_row = 0;
  
  text->focus.obj = NULL;
  text->focus.has_focus = FALSE;
  text->focus.user_data = (void *)text;
  text->focus.key_event = text_key_event;
  
  set_string(text, string);

  calc_width(text);
  calc_ascent_descent(text);

  return text;
}

Text *
text_copy(Text *text)
{
  Text *copy;
  int i;

  copy = g_new(Text, 1);
  copy->numlines = text->numlines;
  copy->line = g_malloc(sizeof(char *)*text->numlines);
  copy->strlen = g_malloc(sizeof(int)*copy->numlines);
  copy->alloclen = g_malloc(sizeof(int)*copy->numlines);
  copy->row_width = g_malloc(sizeof(real)*copy->numlines);
  
  for (i=0;i<text->numlines;i++) {
    copy->line[i] = (char *)g_malloc(text->alloclen[i]+1);
    strcpy(copy->line[i], text->line[i]);
    copy->strlen[i] = text->strlen[i];
    copy->alloclen[i] = text->alloclen[i];
  }
  
  copy->font = dia_font_ref(text->font);
  copy->height = text->height;
  copy->position = text->position;
  copy->color = text->color;
  copy->alignment = text->alignment;

  copy->cursor_pos = 0;
  copy->cursor_row = 0;
  copy->focus.obj = NULL;
  copy->focus.has_focus = FALSE;
  copy->focus.user_data = (void *)copy;
  copy->focus.key_event = text_key_event;
  
  copy->ascent = text->ascent;
  copy->descent = text->descent;
  copy->max_width = text->max_width;
  for (i=0;i<text->numlines;i++) {
    copy->row_width[i] = text->row_width[i];
  }
  
  return copy;
}

void
text_destroy(Text *text)
{
  free_string(text);
  dia_font_unref(text->font);
  g_free(text);
}

void
text_set_height(Text *text, real height)
{
  text->height = height;

  calc_width(text);
  calc_ascent_descent(text);
}

void
text_set_font(Text *text, DiaFont *font)
{
  dia_font_unref(text->font);
  text->font = dia_font_ref(text->font);
  
  calc_width(text);
  calc_ascent_descent(text);
}

void
text_set_position(Text *text, Point *pos)
{
  text->position = *pos;
}

void
text_set_color(Text *text, Color *col)
{
  text->color = *col;
}

void
text_set_alignment(Text *text, Alignment align)
{
  text->alignment = align;
}

void
text_calc_boundingbox(Text *text, Rectangle *box)
{

  calc_width(text);
  calc_ascent_descent(text);

  if (box == NULL) return; /* For those who just want the text info
			      updated */
  box->left = text->position.x;
  switch (text->alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    box->left -= text->max_width / 2.0;
    break;
  case ALIGN_RIGHT:
    box->left -= text->max_width;
    break;
  }

  box->right = box->left + text->max_width;
  
  box->top = text->position.y - text->ascent;

  box->bottom = box->top + text->height*text->numlines + text->descent;

  if (text->focus.has_focus) {
    if (text->cursor_pos == 0) {
      box->left -= 0.05; /* Half the cursor width */
    } else {
      box->right += 0.05; /* Half the cursor width. Assume that
			     if it isn't at position zero, it might be 
			     at the last position possible. */
    }
   
    /* Account for the size of the cursor top and bottom */
    box->top -= 0.05;
    box->bottom += 0.1;
  }
}



char *
text_get_string_copy(Text *text)
{
  int num,i;
  char *str;
  
  num = 0;
  for (i=0;i<text->numlines;i++) {
    num += strlen(text->line[i])+1;
  }

  str = g_malloc(num);

  *str = 0;
  
  for (i=0;i<text->numlines;i++) {
    strcat(str, text->line[i]);
    if (i != (text->numlines-1)) {
      strcat(str, "\n");
    }
  }
  
  return str;
}

real
text_distance_from(Text *text, Point *point)
{
  real dx, dy;
  real topy, bottomy;
  real left, right;
  int line;
  
  topy = text->position.y - text->ascent;
  bottomy = topy + text->height*text->numlines;
  if (point->y <= topy) {
    dy = topy - point->y;
    line = 0;
  } else if (point->y >= bottomy) {
    dy = point->y - bottomy;
    line = text->numlines - 1;
  } else {
    dy = 0.0;
    line = (int) floor( (point->y - topy) / text->height );
  }

  left = text->position.x;
  switch (text->alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    left -= text->row_width[line] / 2.0;
    break;
  case ALIGN_RIGHT:
    left -= text->row_width[line];
    break;
  }
  right = left + text->row_width[line];

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
text_draw(Text *text, Renderer *renderer)
{
  Point pos;
  int i;
  
  renderer->ops->set_font(renderer, text->font, text->height);
  
  pos = text->position;
  
  for (i=0;i<text->numlines;i++) {
    renderer->ops->draw_string(renderer,
			       text->line[i],
			       &pos, text->alignment,
			       &text->color);
    pos.y += text->height;
  }


  if ((renderer->is_interactive) && (text->focus.has_focus)) {
    real curs_x, curs_y;
    real str_width_first;
    real str_width_whole;
    Point p1, p2;
    curs_y = text->position.y - text->ascent + text->cursor_row*text->height; 

    str_width_first =
      renderer->interactive_ops->get_text_width(renderer,
						text->line[text->cursor_row],
						text->cursor_pos);
    str_width_whole =
      renderer->interactive_ops->get_text_width(renderer,
						text->line[text->cursor_row],
						text->strlen[text->cursor_row]);
    curs_x = text->position.x + str_width_first;

    switch (text->alignment) {
    case ALIGN_LEFT:
      break;
    case ALIGN_CENTER:
      curs_x -= str_width_whole / 2.0;
      break;
    case ALIGN_RIGHT:
      curs_x -= str_width_whole;
      break;
    }

    p1.x = curs_x;
    p1.y = curs_y;
    p2.x = curs_x;
    p2.y = curs_y + text->height;
    
    renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
    renderer->ops->set_linewidth(renderer, 0.1);
    renderer->ops->draw_line(renderer, &p1, &p2, &color_black);
  }
}

void
text_grab_focus(Text *text, Object *object)
{
  text->focus.obj = object;
  request_focus(&text->focus);
}

void
text_set_cursor_at_end( Text* text )
{
  text->cursor_row = text->numlines - 1 ;
  text->cursor_pos = text->strlen[text->cursor_row] ;
}

void
text_set_cursor(Text *text, Point *clicked_point,
		Renderer *renderer)
{
  real str_width_whole;
  real str_width_first;
  real top;
  real start_x;
  int row;
  int i;

  top = text->position.y - text->ascent;
  
  row = (int)floor((clicked_point->y - top) / text->height);

  if (row < 0)
    row = 0;

  if (row >= text->numlines)
    row = text->numlines - 1;
    
  text->cursor_row = row;
  text->cursor_pos = 0;

  if (!renderer->is_interactive) {
    message_error("Internal error: Select gives non interactive renderer!\n"
		  "val: %d\n", renderer->is_interactive);
    return;
  }

  renderer->ops->set_font(renderer, text->font, text->height);
  str_width_whole =
    renderer->interactive_ops->get_text_width(renderer,
					      text->line[row],
					      text->strlen[row]);
  start_x = text->position.x;
  switch (text->alignment) {
  case ALIGN_LEFT:
    break;
  case ALIGN_CENTER:
    start_x -= str_width_whole / 2.0;
    break;
  case ALIGN_RIGHT:
    start_x -= str_width_whole;
    break;
  }

  /* Do an ugly linear search for the cursor index:
     TODO: Change to binary search */
  
  for (i=0;i<=text->strlen[row];i++) {
    str_width_first =
      renderer->interactive_ops->get_text_width(renderer, text->line[row], i);
    if (clicked_point->x - start_x >= str_width_first) {
      text->cursor_pos = i;
    } else {
      return;
    }
  }
  text->cursor_pos = text->strlen[row];
}

static void
text_join_lines(Text *text, int first_line)
{
  int i;
  int len1, len2, alloclen1, alloclen2;
  real width;
  char *str1, *str2;
  int numlines;

  str1 = text->line[first_line];
  str2 = text->line[first_line+1];
  len1 = text->strlen[first_line];
  len2 = text->strlen[first_line + 1];
  alloclen1 = text->alloclen[first_line];
  alloclen2 = text->alloclen[first_line + 1];

  text->line[first_line] = NULL;
  text->line[first_line+1] = NULL;

  for (i=first_line+1;i<text->numlines-1;i++) {
    text->line[i] = text->line[i+1];
    text->strlen[i] = text->strlen[i+1];
    text->alloclen[i] = text->alloclen[i+1];
    text->row_width[i] = text->row_width[i+1];
  }
  text->strlen[first_line] = len1 + len2;
  text->alloclen[first_line] = alloclen1 + alloclen2;
  text->line[first_line] =
    g_malloc(sizeof(char)*(text->alloclen[first_line]));
  strcpy(text->line[first_line], str1);
  strcat(text->line[first_line], str2);
  g_free(str1);
  g_free(str2);
  
  text->numlines -= 1;
  numlines = text->numlines;
  text->line = g_realloc(text->line, sizeof(char *)*numlines);
  text->strlen = g_realloc(text->strlen, sizeof(int)*numlines);
  text->alloclen = g_realloc(text->alloclen, sizeof(int)*numlines);
  text->row_width = g_realloc(text->row_width, sizeof(real)*numlines);

  text->row_width[first_line] = 
    dia_font_string_width(text->line[first_line], text->font, text->height);

  width = 0.0;
  for (i=0;i<text->numlines;i++) {
    width = MAX(width, text->row_width[i]);
  }
  text->max_width = width;

  text->cursor_row = first_line;
  text->cursor_pos = len1;
  
}

static void
text_delete_forward(Text *text)
{
  int row;
  int i;
  real width;
  gchar *start, *end;
  int len;
  
  row = text->cursor_row;
  
  if (text->cursor_pos >= text->strlen[row]) {
    if (row+1 < text->numlines)
      text_join_lines(text, row);
    return;
  }
  start = text->line[row];
  for (i = 0; i < text->cursor_pos; i++) {
    start = g_utf8_next_char (start);
  }
  end = g_utf8_next_char (start);

  len = strlen (text->line[row]);
  memmove (start, end, text->line[row] + len - start);

  text->strlen[row] = g_utf8_strlen(text->line[row], -1);//text->strlen[row]--;
  
  if (text->cursor_pos > text->strlen[text->cursor_row])
    text->cursor_pos = text->strlen[text->cursor_row];

  text->row_width[row] = dia_font_string_width(text->line[row],
                                              text->font,
                                              text->height);
  width = 0.0;
  for (i=0;i<text->numlines;i++) {
    width = MAX(width, text->row_width[i]);
  }
  text->max_width = width;
}

static void
text_delete_backward(Text *text)
{
  int row;
  int i;
  real width;
  gchar *start, *end;
  int len;
  
  row = text->cursor_row;
  
  if (text->cursor_pos <= 0) {
    if (row > 0)
      text_join_lines(text, row-1);
    return;
  }
  start = g_utf8_offset_to_pointer(text->line[row], 
				   (glong)(text->cursor_pos-1));
  end = g_utf8_offset_to_pointer(start, 1);
  len = g_utf8_offset_to_pointer(text->line[row], text->strlen[row]) - end;
  memmove (start, end, len + 1);

  text->strlen[row] = g_utf8_strlen(text->line[row], -1);

  text->cursor_pos --;
  
  if (text->cursor_pos > text->strlen[text->cursor_row])
    text->cursor_pos = text->strlen[text->cursor_row];

  text->row_width[row] = dia_font_string_width(text->line[row],
                                              text->font,
                                              text->height);

  width = 0.0;
  for (i=0;i<text->numlines;i++) {
    width = MAX(width, text->row_width[i]);
  }
  text->max_width = width;
}

static void
text_split_line(Text *text)
{
  int i;
  int row;
  int numlines;
  char *line;
  int orig_len;
  real width;
  gchar *str;
  int orig_alloclen;
  
  text->numlines += 1;
  numlines = text->numlines;
  text->line = g_realloc(text->line, sizeof(char *)*numlines);
  text->strlen = g_realloc(text->strlen, sizeof(int)*numlines);
  text->alloclen = g_realloc(text->alloclen, sizeof(int)*numlines);
  text->row_width = g_realloc(text->row_width, sizeof(real)*numlines);

  row = text->cursor_row;
  for (i=text->numlines-1;i>row+1;i--) {
    text->line[i] = text->line[i-1];
    text->strlen[i] = text->strlen[i-1];
    text->alloclen[i] = text->alloclen[i-1];
    text->row_width[i] = text->row_width[i-1];
  }
  /* row and row+1 needs to be changed: */
  line = text->line[row];
  orig_len = text->strlen[row];
  orig_alloclen = text->alloclen[row];
  
  text->strlen[row] = text->cursor_pos;
  str = text->line[row];
  for (i = 0; i < text->cursor_pos; i++) {
    str = g_utf8_next_char (str);
  }
  text->alloclen[row] = str - text->line[row] + 1;
  text->line[row] = g_strndup (line, str - text->line[row]);
  
  text->strlen[row + 1] = orig_len - text->strlen[row];
  text->alloclen[row + 1] = orig_alloclen - strlen (text->line[row]) + 1;
  text->line[row + 1] = g_strndup (str, text->alloclen[row + 1] - 1);

  g_free(line);

  text->row_width[row] = 
    dia_font_string_width(text->line[row], text->font, text->height);
  text->row_width[row+1] = 
    dia_font_string_width(text->line[row+1], text->font, text->height);

  width = 0.0;
  for (i=0;i<text->numlines;i++) {
    width = MAX(width, text->row_width[i]);
  }
  text->max_width = width;

  text->cursor_row += 1;
  text->cursor_pos = 0;
}

static void
text_insert_char(Text *text, gunichar c)
{
  int row;
  int i;
  gchar *line, *str;
  gchar ch[7];
  int unilen, length;

  unilen = g_unichar_to_utf8 (c, ch);
  ch[unilen] = 0;
  
  row = text->cursor_row;

  length = strlen (text->line[row]);
  if ((length + unilen + 1) > text->alloclen[row]) {
    text->alloclen[row] = length * 2 + unilen;
    text->line[row] = g_realloc (text->line[row], sizeof (gchar) * text->alloclen[row]);
  }

  str = text->line[row];
  for (i = 0; i < text->cursor_pos; i++) {
    str = g_utf8_next_char (str);
  }

  line = text->line[row];
  /* copy the part to the right of the insertion */
  for (i = length; &line[i] >= str; i--) {
    line[i + unilen] = line[i];
  }
  strncpy (str, ch, unilen);
  line[length + unilen] = 0; /* null terminate */
  text->cursor_pos += 1;
  text->strlen[row] = g_utf8_strlen(text->line[row], -1);// length + unilen;

  text->row_width[row] =
      dia_font_string_width(text->line[row], text->font, text->height);
  text->max_width = MAX(text->max_width, text->row_width[row]);
}

static int
text_key_event(Focus *focus, guint keyval, const gchar *str, int strlen,
               ObjectChange **change)
{
  Text *text;
  int return_val = FALSE;
  int row, i;
  const char *utf;
  gunichar c;

  *change = NULL;
  
  text = (Text *)focus->user_data;

  switch(keyval) {
      case GDK_Up:
        text->cursor_row--;
        if (text->cursor_row<0)
          text->cursor_row = 0;

        if (text->cursor_pos > text->strlen[text->cursor_row])
          text->cursor_pos = text->strlen[text->cursor_row];

        break;
      case GDK_Down:
        text->cursor_row++;
        if (text->cursor_row >= text->numlines)
          text->cursor_row = text->numlines - 1;

        if (text->cursor_pos > text->strlen[text->cursor_row])
          text->cursor_pos = text->strlen[text->cursor_row];
    
        break;
      case GDK_Left:
        text->cursor_pos--;
        if (text->cursor_pos<0)
          text->cursor_pos = 0;
        break;
      case GDK_Right:
        text->cursor_pos++;
        if (text->cursor_pos > text->strlen[text->cursor_row])
          text->cursor_pos = text->strlen[text->cursor_row];
        break;
      case GDK_Home:
        text->cursor_pos = 0;
        break;
      case GDK_End:
        text->cursor_pos = text->strlen[text->cursor_row];
        break;
      case GDK_Delete:
        return_val = TRUE;
        row = text->cursor_row;
        if (text->cursor_pos >= text->strlen[row]) {
          if (row+1 < text->numlines) {
            *change = text_create_change(text, TYPE_JOIN_ROW, 'Q',
                                         text->cursor_pos, row);
          } else {
            return_val = FALSE;
            break;
          }
        } else {
          utf = text->line[row];
          for (i = 0; i < text->cursor_pos; i++)
            utf = g_utf8_next_char (utf);
          c = g_utf8_get_char (utf);
          *change = text_create_change (text, TYPE_DELETE_FORWARD, c,
                                        text->cursor_pos, text->cursor_row);
        }
        text_delete_forward(text);
        break;
      case GDK_BackSpace:
        return_val = TRUE;
        row = text->cursor_row;
        if (text->cursor_pos <= 0) {
          if (row > 0) {
            *change = text_create_change(text, TYPE_JOIN_ROW, 'Q',
                                         text->strlen[row-1], row-1);
          } else {
            return_val = FALSE;
            break;
          }
        } else {
          utf = text->line[row];
          for (i = 0; i < (text->cursor_pos - 1); i++)
            utf = g_utf8_next_char (utf);
          c = g_utf8_get_char (utf);
          *change = text_create_change (text, TYPE_DELETE_BACKWARD, c,
                                        text->cursor_pos - 1,
                                        text->cursor_row);
        }
        text_delete_backward(text);
        break;
      case GDK_Return:
        return_val = TRUE;
        *change = text_create_change(text, TYPE_SPLIT_ROW, 'Q',
                                     text->cursor_pos, text->cursor_row);
        text_split_line(text);
        break;
      default:
        if (str || (strlen>0)) {

          return_val = TRUE;
          utf = str;
          for (utf = str ; utf && *utf ; utf = g_utf8_next_char (utf)) {
            c = g_utf8_get_char (utf);
            
            *change = text_create_change (text, TYPE_INSERT_CHAR, c,
                                          text->cursor_pos, text->cursor_row);
            text_insert_char (text, c);
          }
        }
        break;
  }  
  
  return return_val;
}

int text_is_empty(Text *text)
{
  int i;
  for (i=0;i<text->numlines;i++) {
    if (text->strlen[i]!=0) {
      return FALSE;
    }
  }
  return TRUE;
}

int
text_delete_all(Text *text, ObjectChange **change)
{
  if (!text_is_empty(text)) {
    *change = text_create_change(text, TYPE_DELETE_ALL,
				 0, text->cursor_pos, text->cursor_row);
    
    text_set_string(text, "");
    calc_ascent_descent(text);
    return TRUE;
  }
  return FALSE;
}

void
data_add_text(AttributeNode attr, Text *text)
{
  DataNode composite;
  char *str;

  composite = data_add_composite(attr, "text");

  str = text_get_string_copy(text);
  data_add_string(composite_add_attribute(composite, "string"),
		  str);
  g_free(str);
  data_add_font(composite_add_attribute(composite, "font"),
		text->font);
  data_add_real(composite_add_attribute(composite, "height"),
		text->height);
  data_add_point(composite_add_attribute(composite, "pos"),
		    &text->position);
  data_add_color(composite_add_attribute(composite, "color"),
		 &text->color);
  data_add_enum(composite_add_attribute(composite, "alignment"),
		text->alignment);
}


Text *
data_text(AttributeNode text_attr)
{
  char *string = "";
  DiaFont *font;
  real height;
  Point pos = {0.0, 0.0};
  Color col;
  Alignment align;
  AttributeNode attr;
  DataNode composite_node;
  Text *text;

  composite_node = attribute_first_data(text_attr);

  attr = composite_find_attribute(text_attr, "string");
  if (attr != NULL)
    string = data_string(attribute_first_data(attr));

  height = 1.0;
  attr = composite_find_attribute(text_attr, "height");
  if (attr != NULL)
    height = data_real(attribute_first_data(attr));

  attr = composite_find_attribute(text_attr, "font");
  if (attr != NULL) {
    font = data_font(attribute_first_data(attr));
  } else {
    font = dia_font_new_from_style(DIA_FONT_SANS,1.0);
  }
  
  attr = composite_find_attribute(text_attr, "pos");
  if (attr != NULL)
    data_point(attribute_first_data(attr), &pos);

  col = color_black;
  attr = composite_find_attribute(text_attr, "color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &col);

  align = ALIGN_LEFT;
  attr = composite_find_attribute(text_attr, "alignment");
  if (attr != NULL)
    align = data_enum(attribute_first_data(attr));
  
  text = new_text(string, font, height, &pos, &col, align);
  if (font) dia_font_unref(font);
  if (string) g_free(string);
  return text;
}

void
text_get_attributes(Text *text, TextAttributes *attr)
{    
  attr->font = text->font;
  attr->height = text->height;
  attr->position = text->position;
  attr->color = text->color;
  attr->alignment = text->alignment;
}

void
text_set_attributes(Text *text, TextAttributes *attr)
{
  if (text->font != attr->font) {
      dia_font_unref(text->font);
      text->font = dia_font_ref(attr->font);
  }
  text->height = attr->height;
  text->position = attr->position;
  text->color = attr->color;
  text->alignment = attr->alignment;
}

static void
text_change_apply(struct TextObjectChange *change, Object *obj)
{
  Text *text = change->text;
  switch (change->type) {
  case TYPE_INSERT_CHAR:
    text->cursor_pos = change->pos;
    text->cursor_row = change->row;
    text_insert_char(text, change->ch);
    break;
  case TYPE_DELETE_BACKWARD:
    text->cursor_pos = change->pos+1;
    text->cursor_row = change->row;
    text_delete_backward(text);
    break;
  case TYPE_DELETE_FORWARD:
    text->cursor_pos = change->pos;
    text->cursor_row = change->row;
    text_delete_forward(text);
    break;
  case TYPE_SPLIT_ROW:
    text->cursor_pos = change->pos;
    text->cursor_row = change->row;
    text_split_line(text);
    break;
  case TYPE_JOIN_ROW:
    text_join_lines(text, change->row);
    break;
  case TYPE_DELETE_ALL:
    set_string(text, "");
    text->cursor_pos = 0;
    text->cursor_row = 0;
    break;
  }
}

static void
text_change_revert(struct TextObjectChange *change, Object *obj)
{
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
  }
}

static void
text_change_free(struct TextObjectChange *change) {
  g_free(change->str);
}

static ObjectChange *
text_create_change(Text *text, enum change_type type,
		   gunichar ch, int pos, int row)
{
  struct TextObjectChange *change;

  change = g_new(struct TextObjectChange, 1);

  change->obj_change.apply = (ObjectChangeApplyFunc) text_change_apply;
  change->obj_change.revert = (ObjectChangeRevertFunc) text_change_revert;
  change->obj_change.free = (ObjectChangeFreeFunc) text_change_free;

  change->text = text;
  change->type = type;
  change->ch = ch;
  change->pos = pos;
  change->row = row;
  if (type == TYPE_DELETE_ALL)
    change->str = text_get_string_copy(text);
  else
    change->str = NULL;
  return (ObjectChange *)change;
}

gboolean 
apply_textattr_properties(GPtrArray *props,
                          Text *text, const gchar *textname,
                          TextAttributes *attrs)
{
  TextProperty *textprop = 
    (TextProperty *)find_prop_by_name_and_type(props,textname,PROP_TYPE_TEXT);

  if ((!textprop) || 
      ((textprop->common.experience & (PXP_LOADED|PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    text_set_attributes(text,attrs);
    return TRUE; 
  }
  return FALSE;
}

gboolean 
apply_textstr_properties(GPtrArray *props,
                         Text *text, const gchar *textname,
                         const gchar *str)
{
  TextProperty *textprop = 
    (TextProperty *)find_prop_by_name_and_type(props,textname,PROP_TYPE_TEXT);

  if ((!textprop) || 
      ((textprop->common.experience & (PXP_LOADED|PXP_SFO))==0 )) {
    /* most likely we're called after the dialog box has been applied */
    text_set_string(text,str);
    return TRUE; 
  }
  return FALSE;
}

