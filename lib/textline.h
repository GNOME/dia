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

#include <glib.h>

#include "diatypes.h"
#include "properties.h"

G_BEGIN_DECLS

/*!
 * \brief Helper class to cache text drawing and related calculations
 *
 * The TextLine object is a single line of text with related information,
 * such as font and font size.  It can (not) be edited directly in the diagram.
 *
 * TODO: Actually make it editable:)
 */
struct _TextLine {
  /* don't change these values directly, use the text_line_set* functions */

  /* Text data: */
  char *chars;

  /* Attributes: */
  DiaFont *font;
  double height;

  /* Computed values, only access these through text_line_get* functions  */
  double ascent;
  double descent;
  double width;

  /*< private >*/

  /* Whether nothing has changed in this object since values were computed. */
  gboolean clean;

  /* Copies of the real fields to keep track of changes caused by
   * properties setting.  These may go away if we create TextLine properties.
   */
  char *chars_cache;
  DiaFont *font_cache;
  double height_cache;

  /** Offsets of the individual glyphs in the string.  */
  double *offsets;
  PangoLayoutLine *layout_offsets;
};


TextLine     *text_line_new                          (const char       *string,
                                                      DiaFont          *font,
                                                      double            height);
void          text_line_destroy                      (TextLine         *text);
TextLine     *text_line_copy                         (TextLine         *text);
void          text_line_set_string                   (TextLine         *text,
                                                      const char       *string);
void          text_line_set_height                   (TextLine         *text,
                                                      double            height);
void          text_line_set_font                     (TextLine         *text,
                                                      DiaFont          *font);
const char   *text_line_get_string                   (TextLine         *text);
DiaFont      *text_line_get_font                     (TextLine         *text);
double        text_line_get_height                   (TextLine         *text);
void          text_line_calc_boundingbox_size        (TextLine         *text,
                                                      Point            *size);
double        text_line_get_width                    (TextLine         *text);
double        text_line_get_ascent                   (TextLine         *text);
double        text_line_get_descent                  (TextLine         *text);
void          text_line_adjust_glyphs                (TextLine         *line,
                                                      PangoGlyphString *glyphs,
                                                      double            scale);
void          text_line_adjust_layout_line           (TextLine         *line,
                                                      PangoLayoutLine  *layoutline,
                                                      double            scale);
double        text_line_get_alignment_adjustment     (TextLine         *text_line,
                                                      DiaAlignment      alignment);

G_END_DECLS
