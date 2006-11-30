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
text_line_copy(const TextLine *text_line)
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
  DIA_RENDERER_GET_CLASS(renderer)->draw_text_line(renderer, text_line,
						   pos, color);
}

gchar *
text_line_get_string(const TextLine *text_line)
{
  return text_line->chars;
}

DiaFont *
text_line_get_font(const TextLine *text_line)
{
  return text_line->font;
}

real 
text_line_get_height(const TextLine *text_line)
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

/** Set some cache data for the renderer object.  This data will get
 * freed if this textline is ever changed or viewed at a different size.
 * Any cache already set, regardless of identity, will be freed.
 * @param text_line
 * @param renderer
 * @param free_func A function for freeing the data.
 * @param scale The zooming scale factor (as defined by the renderer) used
 * to make these data.  If scale independent, just use 0.0.
 * @param data
 */
void
text_line_set_renderer_cache(TextLine *text_line, DiaRenderer *renderer,
			     RendererCacheFreeFunc free_func, real scale,
			     gpointer data) {
  RendererCache *cache;
  if (text_line->renderer_cache != NULL) {
    (*text_line->renderer_cache->free_func)(text_line->renderer_cache);
    text_line->renderer_cache = NULL;
  }
  cache = g_new(RendererCache, 1);
  cache->renderer = renderer;
  cache->free_func = free_func;
  cache->scale = scale;
  cache->data = data;
}

/** Get any renderer cache data that might be around.
 * @param text_line
 * @param renderer
 * @param scale The scale we want text rendered at, or 0.0 if this renderer
 * is scale independent.
 * @returns Previously cached data (which shouldn't be freed) for the
 * same text rendering.
 */
gpointer
text_line_get_renderer_cache(TextLine *text_line, DiaRenderer *renderer,
			     real scale) {
  if (text_line->clean && text_line->renderer_cache != NULL &&
      text_line->renderer_cache->renderer == renderer &&
      fabs(text_line->renderer_cache->scale - scale) < 0.0000001) {
    return text_line->renderer_cache->data;
  } else {
    return NULL;
  }
}

/** Return the amount this text line would need to be shifted in order to
 * implement the given alignment.
 * @param text_line a line of text
 * @param alignment how to align it.
 * @returns The amount (in diagram lengths) to shift the x positiion of
 * rendering this such that it looks aligned when printed with x at the left.
 * Always a positive number.
 */
real
text_line_get_alignment_adjustment(TextLine *text_line, Alignment alignment)
{
  text_line_cache_values(text_line);
  switch (alignment) {
      case ALIGN_CENTER:
	return text_line->width / 2;
      case ALIGN_RIGHT:
	return text_line->width;
      default:
	return 0.0;
   }  
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
      text_line->offsets = NULL;
    }
    if (text_line->renderer_cache != NULL) {
      (*text_line->renderer_cache->free_func)(text_line->renderer_cache);
      text_line->renderer_cache = NULL;
    }
    if (text_line->layout_offsets != NULL) {
/* Non-debugged code for when we have multiple runs in a line.
        GSList *runs = text_line->layout_offsets->runs;

	for (; runs != NULL; runs = g_slist_next(runs)) {
	  PangoGlyphItem *run = (PangoGlyphItem *) runs->data;
	  int i;
	  
	  for (i = 0; i < run->item->num_chars; i++) {
	    g_free(run->glyphs[i].glyphs);
	  }
	  g_free(run->glyphs);
	}
	g_slist_free(runs);
	g_free(text_line->layout_offsets);
*/
    }

    if (text_line->chars == NULL ||
	text_line->chars[0] == '\0') {
      text_line->offsets = g_new(real, 0);
      text_line->layout_offsets = NULL;
      text_line->ascent = text_line->height * .5;
      text_line->descent = text_line->height * .5;
      text_line->width = 0;
    } else {
      text_line->offsets = 
	dia_font_get_sizes(text_line->chars, text_line->font, text_line->height,
			   &text_line->width, &text_line->ascent, 
			   &text_line->descent, &n_offsets, NULL);
    }
    text_line->clean = TRUE;
    text_line->chars_cache = text_line->chars;
    text_line->font_cache = text_line->font;
    text_line->height_cache = text_line->height;
  }
}

/** Adjust a line of glyphs to match the sizes stored in the TextLine
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

/** Adjust a layout line to match the more fine-grained values stored in the
 * textline.  This circumvents the rounding errors in Pango and ensures a
 * linear scaling for zooming and export filters.
 * @param line The TextLine object that corresponds to the glyphs.
 * @param layoutline The one set of glyphs contained in the TextLine's layout.
 * @param scale The relative height of the font in glyphs.
 * @return An adjusted glyphstring, which should be freed by the caller.
 */
void
text_line_adjust_layout_line(TextLine *line, PangoLayoutLine *layoutline,
			     real scale)
{

  /* This is a one-run version that uses tried-and-true offset arrays. */
  if (line->offsets == NULL) {
    return;
  } else {
    PangoGlyphString *glyphs;
    PangoGlyphItem *glyphItem;

    if (g_slist_length(layoutline->runs) != 1) {
      message_warning("Unexpected %d runs in %s\n",
		      g_slist_length(layoutline->runs),
		      line->chars);
    }

    glyphItem = (PangoGlyphItem*)layoutline->runs->data;
    glyphs = glyphItem->glyphs;
    text_line_adjust_glyphs(line, glyphItem->glyphs, scale);
  }

/* More advanced version.
 * Commented out until we can have more than one run in a line.
  GSList *layoutruns = layoutline->runs;
  GSList *runs;

  if (line->layout_offsets == NULL) {
    return;
  }

  runs = line->layout_offsets->runs;

  for (; runs != NULL && layoutruns != NULL; runs = g_slist_next(runs),
	 layoutruns = g_slist_next(layoutruns)) {
    PangoGlyphString *glyphs = ((PangoLayoutRun *) runs->data)->glyphs;
    PangoGlyphString *layoutglyphs =
      ((PangoLayoutRun *) layoutruns->data)->glyphs;
    int i;

    for (i = 0; i < glyphs->num_glyphs && i < layoutglyphs->num_glyphs; i++) {
      layoutglyphs->glyphs[i].geometry.width =
	(int)(glyphs->glyphs[i].geometry.width * scale * 20.0 * PANGO_SCALE);
      layoutglyphs->glyphs[i].geometry.x_offset =
	(int)(glyphs->glyphs[i].geometry.x_offset * scale * 20.0 * PANGO_SCALE);
      layoutglyphs->glyphs[i].geometry.y_offset =
	(int)(glyphs->glyphs[i].geometry.y_offset * scale * 20.0 * PANGO_SCALE);
    }
    if (glyphs->num_glyphs != layoutglyphs->num_glyphs) {
      printf("Glyph length error: %d != %d\n", 
	     glyphs->num_glyphs, layoutglyphs->num_glyphs);
    }
  }
  if (runs != layoutruns) {
    printf("Runs length error: %d != %d\n",
	   g_slist_length(line->layout_offsets->runs),
	   g_slist_length(layoutline->runs));
  }
*/
}
