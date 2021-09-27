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

#pragma once

typedef enum {
  TEXT_EDIT_START,
  TEXT_EDIT_CHANGE,
  TEXT_EDIT_END
} TextEditState;

#include <glib.h>
#include "diatypes.h"
#include "textattr.h"
#include "focus.h"
#include "dia_xml.h" /* for AttributeNode */
#include "diarenderer.h"
#include "dia-object-change.h"

G_BEGIN_DECLS

#define DIA_TYPE_TEXT_OBJECT_CHANGE dia_text_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaTextObjectChange,
                      dia_text_object_change,
                      DIA, TEXT_OBJECT_CHANGE,
                      DiaObjectChange)


/*!
 * \brief Multiline text representation
 *
 * The Text object provides high level service for on canvas text editing.
 *
 * It is used in various _DiaObject implementations, but also part of the
 * _DiaRenderer interface
 *
 * \ingroup ObjectParts
 */
struct _Text {
  /* don't change these values directly, use the text_set* functions */

  /* Text data: */
  int numlines;
  TextLine **lines;

  /* Attributes: */
  DiaFont *font;
  double height;
  Point position;
  Color color;
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


/* makes an internal copy of the string */
/*! \brief Text object creation \memberof _Text */
Text   *new_text              (const char    *string,
                               DiaFont       *font,
                               double         height,
                               Point         *pos,
                               Color         *color,
                               DiaAlignment   align);
Text   *new_text_default      (Point         *pos,
                               Color         *color,
                               DiaAlignment   align);
void    text_destroy          (Text       *text);
Text   *text_copy             (Text       *text);
char   *text_get_line         (const Text *text,
                               int         line);
char   *text_get_string_copy  (const Text *text);
void    text_set_string       (Text       *text,
                               const char *string);
void    text_set_height       (Text       *text,
                               double      height);
double  text_get_height       (const Text *text);
void    text_set_font         (Text       *text,
                               DiaFont    *font);
void    text_set_position     (Text       *text,
                               Point      *pos);
void    text_set_color        (Text       *text,
                               Color      *col);
void    text_set_alignment    (Text          *text,
                               DiaAlignment   align);
double  text_distance_from    (Text       *text,
                               Point      *point);
void text_calc_boundingbox(Text *text, DiaRectangle *box);
void text_draw(Text *text, DiaRenderer *renderer);
void text_set_cursor(Text *text, Point *clicked_point,
		     DiaRenderer *interactive_renderer);
void text_set_cursor_at_end( Text* text );
void text_grab_focus(Text *text, DiaObject *object);
int text_is_empty(const Text *text);
int text_delete_all (Text *text, DiaObjectChange **change, DiaObject *obj);
void text_get_attributes(Text *text, TextAttributes *attr);
void text_set_attributes(Text *text, TextAttributes *attr);

double text_get_line_width(const Text *text, int line_no);
int text_get_line_strlen(const Text *text, int line_no);
double text_get_max_width(Text *text);
double text_get_ascent(Text *text);
double text_get_descent(Text *text);

/** Exposing this is a hack, but currently GTK still captures the key
 * events of insensitive clods^H^H^H^H^Hmenu items. LC 21/10 2007*/
gboolean text_delete_key_handler(Focus *focus, DiaObjectChange **change);
void data_add_text(AttributeNode attr, Text *text, DiaContext *ctx);
Text *data_text(AttributeNode attr, DiaContext *ctx);

gboolean apply_textattr_properties(GPtrArray *props,
                                   Text *text, const gchar *textname,
                                   TextAttributes *attrs);
gboolean apply_textstr_properties(GPtrArray *props,
                                  Text *text, const gchar *textname,
                                  const gchar *str);

G_END_DECLS
