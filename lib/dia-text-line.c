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

#include <pango/pango.h>

#include "dia-part.h"

#include "dia-text-line.h"


/**
 * DiaTextLine:
 *
 * Helper class to cache text drawing and related calculations
 *
 * The DiaTextLine object is a single line of text with related information,
 * such as font and font size.  It can (not) be edited directly in the diagram.
 *
 * TODO: Actually make it editable:)
 */
struct _DiaTextLine {
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

  /* Whether nothing has changed in this object since values were computed. */
  gboolean clean;

  /* Copies of the real fields to keep track of changes caused by
   * properties setting.  These may go away if we create DiaTextLine properties.
   */
  char *chars_cache;
  DiaFont *font_cache;
  double height_cache;

  /** Offsets of the individual glyphs in the string.  */
  double *offsets;
  PangoLayoutLine *layout_offsets;
};


G_DEFINE_TYPE (DiaTextLine, dia_text_line, DIA_TYPE_PART)


static void
clear_layout_offset (DiaTextLine *self)
{
  if (self->layout_offsets != NULL) {
    GSList *runs = self->layout_offsets->runs;

    for (; runs != NULL; runs = g_slist_next(runs)) {
      PangoGlyphItem *run = (PangoGlyphItem *) runs->data;

      g_clear_pointer (&run->glyphs->glyphs, g_free);
      g_clear_pointer (&run->glyphs, g_free);
    }
    g_slist_free(runs);
    g_clear_pointer (&self->layout_offsets, g_free);
  }
}


static void
dia_text_line_clear (DiaPart *part)
{
  DiaTextLine *self = DIA_TEXT_LINE (part);

  g_clear_pointer (&self->chars, g_free);
  g_clear_object (&self->font);
  clear_layout_offset (self);
  g_clear_pointer (&self->offsets, g_free);

  DIA_PART_CLASS (dia_text_line_parent_class)->clear (part);
}


static void
dia_text_line_class_init (DiaTextLineClass *klass)
{
  DiaPartClass *part_class = DIA_PART_CLASS (klass);

  part_class->clear = dia_text_line_clear;
}


static void
dia_text_line_init (DiaTextLine *self)
{

}


/**
 * text_line_dirty_cache:
 * @text_line: the object that has changed.
 *
 * Mark this object as needing update before usage.
 *
 * Since: 0.98
 */
static void
text_line_dirty_cache (DiaTextLine *self)
{
  self->clean = FALSE;
}


static void text_line_cache_values (DiaTextLine *self);


/**
 * dia_text_line_set_string:
 * @self: The object to change.
 * @string: The string to display.  This string will be copied.
 *
 * Sets this object to display a particular string.
 */
void
dia_text_line_set_string (DiaTextLine *self, const char *string)
{
  g_return_if_fail (DIA_IS_TEXT_LINE (self));

  if (g_set_str (&self->chars, string)) {
    text_line_dirty_cache (self);
  }
}


/**
 * dia_text_line_set_font:
 * @text_line: The object to change.
 * @font: The font to use for displaying this object.
 *
 * Sets the font used by this object.
 */
void
dia_text_line_set_font (DiaTextLine *self, DiaFont *font)
{
  g_return_if_fail (DIA_IS_TEXT_LINE (self));

  if (g_set_object (&self->font, font)) {
    text_line_dirty_cache (self);
  }
}


/**
 * dia_text_line_set_height:
 * @text_line: The object to change.
 * @height: The font height to use for displaying this object (in cm, from
 *          baseline to baseline)
 *
 * Sets the font height used by this object.
 */
void
dia_text_line_set_height (DiaTextLine *self, double height)
{
  g_return_if_fail (DIA_IS_TEXT_LINE (self));

  if (!G_APPROX_VALUE (self->height, height, 0.00001)) {
    self->height = height;
    text_line_dirty_cache (self);
  }
}


/**
 * dia_text_line_new:
 * @string: the string to display
 * @font: the font to display the string with.
 * @height: the height of the font, in cm from baseline to baseline.
 *
 * Creates a new DiaTextLine object from its components.
 */
DiaTextLine *
dia_text_line_new (const char *string, DiaFont *font, double height)
{
  DiaTextLine *self = dia_part_alloc (DIA_TYPE_TEXT_LINE);

  dia_text_line_set_string (self, string);
  dia_text_line_set_font (self, font);
  dia_text_line_set_height (self, height);

  return self;
}


/**
 * dia_text_line_copy:
 * @self: The object to copy.
 *
 * Make a deep copy of the given DiaTextLine
 */
DiaTextLine *
dia_text_line_copy (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), NULL);

  return dia_text_line_new (self->chars,
                            self->font,
                            self->height);
}


const char *
dia_text_line_get_string (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), NULL);

  return self->chars;
}


DiaFont *
dia_text_line_get_font (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), NULL);

  return self->font;
}


double
dia_text_line_get_height (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), 0.0);

  return self->height;
}


double
dia_text_line_get_width (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), 0.0);

  text_line_cache_values (self);

  return self->width;
}


double
dia_text_line_get_ascent (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), 0.0);

  text_line_cache_values (self);

  return self->ascent;
}


double
dia_text_line_get_descent (DiaTextLine *self)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), 0.0);

  text_line_cache_values (self);

  return self->descent;
}


/**
 * dia_text_line_get_alignment_adjustment:
 * @text_line: a line of text
 * @alignment: how to align it.
 *
 * Calculate #DiaTextLine adjustment for #DiaAlignment
 *
 * Return the amount this text line would need to be shifted in order to
 * implement the given alignment.
 *
 * Returns: The amount (in diagram lengths) to shift the x positiion of
 * rendering this such that it looks aligned when printed with x at the left.
 * Always a positive number.
 *
 * Since: 0.98
 */
double
dia_text_line_get_alignment_adjustment (DiaTextLine *self, DiaAlignment alignment)
{
  g_return_val_if_fail (DIA_IS_TEXT_LINE (self), 0.0);

  text_line_cache_values (self);

  switch (alignment) {
    case DIA_ALIGN_CENTRE:
     return self->width / 2;
    case DIA_ALIGN_RIGHT:
       return self->width;
    case DIA_ALIGN_LEFT:
    default:
     return 0.0;
  }
}


static void
text_line_cache_values (DiaTextLine *text_line)
{
  if (!text_line->clean ||
      text_line->chars != text_line->chars_cache ||
      text_line->font != text_line->font_cache ||
      text_line->height != text_line->height_cache) {
    int n_offsets;

    g_clear_pointer (&text_line->offsets, g_free);
    clear_layout_offset (text_line);

    if (text_line->chars == NULL || text_line->chars[0] == '\0') {
      /* caclculate reasonable ascent/decent even for empty string */
      text_line->offsets = dia_font_get_sizes ("XjgM149",
                                               text_line->font,
                                               text_line->height,
                                               &text_line->width,
                                               &text_line->ascent,
                                               &text_line->descent,
                                               &n_offsets,
                                               &text_line->layout_offsets);
      clear_layout_offset (text_line);
      g_clear_pointer (&text_line->offsets, g_free);
      text_line->offsets = g_new (real, 0); /* another way to assign NULL;) */
      text_line->width = 0;
    } else {
      text_line->offsets = dia_font_get_sizes (text_line->chars,
                                               text_line->font,
                                               text_line->height,
                                               &text_line->width,
                                               &text_line->ascent,
                                               &text_line->descent,
                                               &n_offsets,
                                               &text_line->layout_offsets);
    }
    text_line->clean = TRUE;
    text_line->chars_cache = text_line->chars;
    text_line->font_cache = text_line->font;
    text_line->height_cache = text_line->height;
  }
}
