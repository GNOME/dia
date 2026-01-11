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

#include "propinternals.h"
#include "text.h"
#include "message.h"
#include "textline.h"

static void text_line_dirty_cache(TextLine *text_line);
static void text_line_cache_values(TextLine *text_line);
static void clear_layout_offset (TextLine *text_line);


/**
 * text_line_set_string:
 * @text_line: The object to change.
 * @string: The string to display.  This string will be copied.
 *
 * Sets this object to display a particular string.
 */
void
text_line_set_string (TextLine *text_line, const char *string)
{
  if (g_set_str (&text_line->chars, string)) {
    text_line_dirty_cache (text_line);
  }
}


/**
 * text_line_set_font:
 * @text_line: The object to change.
 * @font: The font to use for displaying this object.
 *
 * Sets the font used by this object.
 */
void
text_line_set_font (TextLine *text_line, DiaFont *font)
{
  if (g_set_object (&text_line->font, font)) {
    text_line_dirty_cache (text_line);
  }
}


/**
 * text_line_set_height:
 * @text_line: The object to change.
 * @height: The font height to use for displaying this object (in cm, from
 *          baseline to baseline)
 *
 * Sets the font height used by this object.
 */
void
text_line_set_height (TextLine *text_line, double height)
{
  if (!G_APPROX_VALUE (text_line->height, height, 0.00001)) {
    text_line->height = height;
    text_line_dirty_cache (text_line);
  }
}


/**
 * text_line_new:
 * @string: the string to display
 * @font: the font to display the string with.
 * @height: the height of the font, in cm from baseline to baseline.
 *
 * Creates a new TextLine object from its components.
 */
TextLine *
text_line_new (const char *string, DiaFont *font, double height)
{
  TextLine *text_line = g_new0 (TextLine, 1);

  text_line_set_string (text_line, string);
  text_line_set_font (text_line, font);
  text_line_set_height (text_line, height);

  return text_line;
}


/**
 * text_line_copy:
 * @text_line: The object to copy.
 *
 * Make a deep copy of the given TextLine
 */
TextLine *
text_line_copy (TextLine *text_line)
{
  return text_line_new (text_line->chars,
                        text_line->font,
                        text_line->height);
}


/**
 * text_line_destroy:
 * @text_line: the object to kill.
 *
 * Destroy a text_line object, this is deallocating all memory used and
 * unreffing reffed objects.
 */
void
text_line_destroy (TextLine *text_line)
{
  g_clear_pointer (&text_line->chars, g_free);
  g_clear_object (&text_line->font);
  clear_layout_offset (text_line);
  g_clear_pointer (&text_line->offsets, g_free);
  g_free (text_line);
}


/**
 * text_line_calc_boundingbox_size:
 * @text_line:
 * @size: A place to store the width and height of the text.
 *
 * TextLine bounding box caclulation
 *
 * Calculate the bounding box size of this object. Since a text object has no
 * position or alignment, this collapses to just a size.
 */
void
text_line_calc_boundingbox_size (TextLine *text_line, Point *size)
{
  text_line_cache_values (text_line);

  size->x = text_line->width;
  size->y = text_line->ascent + text_line->descent;
}


const char *
text_line_get_string (TextLine *text_line)
{
  return text_line->chars;
}


DiaFont *
text_line_get_font (TextLine *text_line)
{
  return text_line->font;
}


double
text_line_get_height (TextLine *text_line)
{
  return text_line->height;
}


double
text_line_get_width (TextLine *text_line)
{
  text_line_cache_values (text_line);
  return text_line->width;
}


double
text_line_get_ascent (TextLine *text_line)
{
  text_line_cache_values (text_line);
  return text_line->ascent;
}


double
text_line_get_descent (TextLine *text_line)
{
  text_line_cache_values (text_line);
  return text_line->descent;
}


/**
 * text_line_get_alignment_adjustment:
 * @text_line: a line of text
 * @alignment: how to align it.
 *
 * Calculate #TextLine adjustment for #DiaAlignment
 *
 * Return the amount this text line would need to be shifted in order to
 * implement the given alignment.
 *
 * Returns: The amount (in diagram lengths) to shift the x positiion of
 * rendering this such that it looks aligned when printed with x at the left.
 * Always a positive number.
 *
 * Since: dawn-of-time
 */
double
text_line_get_alignment_adjustment (TextLine *text_line, DiaAlignment alignment)
{
  text_line_cache_values (text_line);

  switch (alignment) {
    case DIA_ALIGN_CENTRE:
     return text_line->width / 2;
    case DIA_ALIGN_RIGHT:
       return text_line->width;
    case DIA_ALIGN_LEFT:
    default:
     return 0.0;
  }
}

/* **** Private functions **** */

/**
 * text_line_dirty_cache:
 * @text_line: the object that has changed.
 *
 * Mark this object as needing update before usage.
 *
 * Since: dawn-of-time
 */
static void
text_line_dirty_cache(TextLine *text_line)
{
  text_line->clean = FALSE;
}


static void
clear_layout_offset (TextLine *text_line)
{
  if (text_line->layout_offsets != NULL) {
    GSList *runs = text_line->layout_offsets->runs;

    for (; runs != NULL; runs = g_slist_next(runs)) {
      PangoGlyphItem *run = (PangoGlyphItem *) runs->data;

      g_clear_pointer (&run->glyphs->glyphs, g_free);
      g_clear_pointer (&run->glyphs, g_free);
    }
    g_slist_free(runs);
    g_clear_pointer (&text_line->layout_offsets, g_free);
  }
}

static void
text_line_cache_values(TextLine *text_line)
{
  if (!text_line->clean ||
      text_line->chars != text_line->chars_cache ||
      text_line->font != text_line->font_cache ||
      text_line->height != text_line->height_cache) {
    int n_offsets;

    g_clear_pointer (&text_line->offsets, g_free);
    clear_layout_offset (text_line);

    if (text_line->chars == NULL ||
	text_line->chars[0] == '\0') {
      /* caclculate reasonable ascent/decent even for empty string */
      text_line->offsets =
        dia_font_get_sizes("XjgM149", text_line->font, text_line->height,
			   &text_line->width, &text_line->ascent,
			   &text_line->descent, &n_offsets,
			   &text_line->layout_offsets);
      clear_layout_offset (text_line);
      g_clear_pointer (&text_line->offsets, g_free);
      text_line->offsets = g_new (real,0); /* another way to assign NULL;) */
      text_line->width = 0;
    } else {
      text_line->offsets =
	dia_font_get_sizes(text_line->chars, text_line->font, text_line->height,
			   &text_line->width, &text_line->ascent,
			   &text_line->descent, &n_offsets,
			   &text_line->layout_offsets);
    }
    text_line->clean = TRUE;
    text_line->chars_cache = text_line->chars;
    text_line->font_cache = text_line->font;
    text_line->height_cache = text_line->height;
  }
}

/*!
 * \brief Move glyphs to approximate a desired total width
 *
 * Adjust a line of glyphs to match the sizes stored in the TextLine
 * @param line The TextLine object that corresponds to the glyphs.
 * @param glyphs The one set of glyphs contained in layout created for
 * this TextLine during rendering.  The values in here will be changed.
 * @param scale The relative height of the font in glyphs.
 */
void
text_line_adjust_glyphs(TextLine *line, PangoGlyphString *glyphs, real scale)
{
  int i;

  for (i = 0; i < glyphs->num_glyphs; i++) {
/*
    printf("Glyph %d: width %d, offset %f, textwidth %f\n",
	   i, new_glyphs->glyphs[i].geometry.width, line->offsets[i],
	   line->offsets[i] * scale * 20.0 * PANGO_SCALE);
*/
    glyphs->glyphs[i].geometry.width =
      (int)(line->offsets[i] * scale * 20.0 * PANGO_SCALE);
  }
}


/**
 * text_line_adjust_layout_line:
 * @line: The #TextLine object that corresponds to the glyphs.
 * @layoutline: The one set of glyphs contained in the #TextLine's layout.
 * @scale: The relative height of the font in glyphs.
 *
 * Adjust a layout line to match the more fine-grained values stored in the
 * textline. This circumvents the rounding errors in Pango and ensures a
 * linear scaling for zooming and export filters.
 */
void
text_line_adjust_layout_line (TextLine        *line,
                              PangoLayoutLine *layoutline,
                              double           scale)
{
  GSList *layoutruns = layoutline->runs;
  GSList *runs;

  if (line->layout_offsets == NULL) {
    return;
  }

  runs = line->layout_offsets->runs;

  if (g_slist_length(runs) != g_slist_length(layoutruns)) {
    g_printerr ("Runs length error: %d != %d\n",
                g_slist_length (line->layout_offsets->runs),
                g_slist_length (layoutline->runs));
  }
  for (; runs != NULL && layoutruns != NULL; runs = g_slist_next(runs),
	 layoutruns = g_slist_next(layoutruns)) {
    PangoGlyphString *glyphs = ((PangoLayoutRun *) runs->data)->glyphs;
    PangoGlyphString *layoutglyphs =
      ((PangoLayoutRun *) layoutruns->data)->glyphs;
    int i;

    for (i = 0; i < glyphs->num_glyphs && i < layoutglyphs->num_glyphs; i++) {
      layoutglyphs->glyphs[i].geometry.width =
	(int)(glyphs->glyphs[i].geometry.width * scale / 20.0);
      layoutglyphs->glyphs[i].geometry.x_offset =
	(int)(glyphs->glyphs[i].geometry.x_offset * scale / 20.0);
      layoutglyphs->glyphs[i].geometry.y_offset =
	(int)(glyphs->glyphs[i].geometry.y_offset * scale / 20.0);
    }
    if (glyphs->num_glyphs != layoutglyphs->num_glyphs) {
      g_printerr ("Glyph length error: %d != %d\n",
                  glyphs->num_glyphs,
                  layoutglyphs->num_glyphs);
    }
  }
}
