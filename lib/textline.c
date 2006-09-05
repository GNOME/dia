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
#include "diarenderer.h"
#include "diagramdata.h"
#include "objchange.h"
#include "textline.h"

static void text_line_dirty_cache(TextLine *text_line);
static void text_line_cache_values(TextLine *text_line);

/** Sets this object to display a particular string.
 * @param text_line The object to change.
 * @param string The string to display.  This string will be copied.
 */
void
text_line_set_string(TextLine *text_line, const gchar *string)
{
  if (text_line->chars == NULL ||
      strcmp(text_line->chars, string)) {
    if (text_line->chars != NULL) {
      g_free(text_line->chars);
    }
    
    text_line->chars = g_strdup(string);
    
    text_line_dirty_cache(text_line);
  }
}

/** Sets the font used by this object.
 * @param text_line The object to change.
 * @param font The font to use for displaying this object.
 */
void
text_line_set_font(TextLine *text_line, DiaFont *font)
{
  if (text_line->font != font) {
    DiaFont *old_font = text_line->font;
    dia_font_ref(font);
    text_line->font = font;
    if (old_font != NULL) {
      dia_font_unref(old_font);
    }
    text_line_dirty_cache(text_line);
  }
}

/** Sets the font height used by this object.
 * @param text_line The object to change.
 * @param height The font height to use for displaying this object 
 * (in cm, from baseline to baseline)
 */
void
text_line_set_height(TextLine *text_line, real height)
{
  if (fabs(text_line->height - height) > 0.00001) {
    text_line->height = height;
    text_line_dirty_cache(text_line);
  }
}

/** Creates a new TextLine object from its components.
 * @param string the string to display
 * @param font the font to display the string with.
 * @param height the height of the font, in cm from baseline to baseline.
 */
TextLine *
text_line_new(const gchar *string, DiaFont *font, real height)
{
  TextLine *text_line = g_new0(TextLine, 1);

  text_line_set_string(text_line, string);
  text_line_set_font(text_line, font);
  text_line_set_height(text_line, height);

  return text_line;
}

TextLine *
text_line_copy(TextLine *text_line)
{
  return text_line_new(text_line->chars, text_line->font, text_line->height);
}

/** Destroy a text_line object, deallocating all memory used and unreffing
 * reffed objects.
 * @param text_line the object to kill.
 */
void
text_line_destroy(TextLine *text_line)
{
  if (text_line->chars != NULL) {
    g_free(text_line->chars);
  }
  if (text_line->font != NULL) {
    dia_font_unref(text_line->font);
  }
  /* TODO: Handle renderer's cached content. */
  g_free(text_line);
}

/** Calculate the bounding box size of this object.  Since a text object has no
 * position or alignment, this collapses to just a size. 
 * @param text_line
 * @param size A place to store the width and height of the text.
 */
void
text_line_calc_boundingbox_size(TextLine *text_line, Point *size)
{
  text_line_cache_values(text_line);

  size->x = text_line->width;
  size->y = text_line->ascent + text_line->descent;
}

/** Draw a line of text at a given position and in a given color.
 * @param renderer
 * @param text_line
 * @param pos
 * @param color
 */
void
text_line_draw(DiaRenderer *renderer, TextLine *text_line,
	       Point *pos, Color *color)
{
#ifdef DIRECT_TEXT_LINE
  DIA_RENDERER_GET_CLASS(renderer)->draw_text_line(renderer, 
						   text_line,
						   pos, color);
#else
  DIA_RENDERER_GET_CLASS(renderer)->set_font(renderer, text_line->font,
					     text_line->height);
  DIA_RENDERER_GET_CLASS(renderer)->draw_string(renderer, 
						text_line->chars,
						pos, ALIGN_LEFT, color);
#endif
}

gchar *
text_line_get_string(TextLine *text_line)
{
  return text_line->chars;
}

DiaFont *
text_line_get_font(TextLine *text_line)
{
  return text_line->font;
}

real 
text_line_get_height(TextLine *text_line)
{
  return text_line->height;
}

real
text_line_get_width(TextLine *text_line)
{
  text_line_cache_values(text_line);
  return text_line->width;
}

real
text_line_get_ascent(TextLine *text_line)
{
  text_line_cache_values(text_line);
  return text_line->ascent;
}

real
text_line_get_descent(TextLine *text_line)
{
  text_line_cache_values(text_line);
  return text_line->descent;
}

/* **** Private functions **** */
/** Mark this object as needing update before usage. 
 * @param text_line the object that has changed.
 */
static void
text_line_dirty_cache(TextLine *text_line)
{
  text_line->clean = FALSE;
}

static void
text_line_cache_values(TextLine *text_line)
{
  if (!text_line->clean ||
      text_line->chars != text_line->chars_cache ||
      text_line->font != text_line->font_cache ||
      text_line->height != text_line->height_cache) {
    int n_offsets;

    if (text_line->offsets != NULL) {
      g_free(text_line->offsets);
    }
    if (text_line->chars == NULL ||
	text_line->chars[0] == '\0') {
      text_line->offsets = g_new(real, 0);
      text_line->ascent = text_line->height * .5;
      text_line->descent = text_line->height * .5;
      text_line->width = 0;
    } else {
      text_line->offsets = 
	dia_font_get_sizes(text_line->chars, text_line->font, text_line->height,
			   &text_line->width, &text_line->ascent, 
			   &text_line->descent, &n_offsets);
    }
    text_line->clean = TRUE;
    text_line->chars_cache = text_line->chars;
    text_line->font_cache = text_line->font;
    text_line->height_cache = text_line->height;
  }
}
