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
#ifndef FONT_H
#define FONT_H

#include <glib.h>
#include <glib-object.h>
#include <pango/pango.h>
#include "geometry.h"
#include "diavar.h"

/* Do NOT put these strings through the .po mechanism. If you need to add
   non-Roman alternatives, please insert them in the list */
#define BASIC_SANS_FONT "arial, helvetica, helv, sans"
#define BASIC_SERIF_FONT "times new roman, times, serif"
#define BASIC_MONOSPACE_FONT "courier new, courier, monospace, fixed"

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} Alignment;


/** The Pango people don't seem to encounter the real world much.
 * In a goodly selection of fonts, 500 is very common, yet they don't name it.
 * I am calling 500 'medium' and 600 'demibold'.
 * We should really have a more flexible system where 300 or 400 is normal, 
 * the next one up bold, or something.  But this will do for now.
 * We should probably store the actual weight...
 */
typedef enum {
  STYLE_ULTRALIGHT = 1,
  STYLE_ULTRALIGHT_OBLIQUE,
  STYLE_ULTRALIGHT_ITALIC,
  STYLE_LIGHT,
  STYLE_LIGHT_OBLIQUE,
  STYLE_LIGHT_ITALIC,
  STYLE_NORMAL,
  STYLE_OBLIQUE,
  STYLE_ITALIC,
  STYLE_MEDIUM,
  STYLE_MEDIUM_OBLIQUE,
  STYLE_MEDIUM_ITALIC,
  STYLE_DEMIBOLD,
  STYLE_DEMIBOLD_OBLIQUE,
  STYLE_DEMIBOLD_ITALIC,
  STYLE_BOLD,
  STYLE_BOLD_OBLIQUE,
  STYLE_BOLD_ITALIC,
  STYLE_ULTRABOLD,
  STYLE_ULTRABOLD_OBLIQUE,
  STYLE_ULTRABOLD_ITALIC,
  STYLE_HEAVY,
  STYLE_HEAVY_OBLIQUE,
  STYLE_HEAVY_ITALIC,
} Style;

typedef struct _DiaFont DiaFont;
typedef struct _DiaFontClass DiaFontClass;

GType dia_font_get_type(void) G_GNUC_CONST;

#define DIA_TYPE_FONT (dia_font_get_type())

#define DIA_FONT(object)    (G_TYPE_CHECK_INSTANCE_CAST ((object), \
                                                         DIA_TYPE_FONT, \
                                                         DiaFont))
#define DIA_FONT_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), \
                                                        DIA_TYPE_FONT, \
                                                        DiaFontClass))

struct _DiaFontClass {
    GObjectClass parent_class;
};
struct _DiaFont {
    GObject parent_instance;

        /* Do not access directly !*/
    PangoFontDescription* pfd;    
    /* mutable */ char* legacy_name;    
};


void dia_font_init(PangoContext* pcontext);
                             
                             
    /* Get a font matching family,style,height. MUST be freed with
       dia_font_free(). */
DiaFont* dia_font_new(const char *family, Style style,
                     real height);

    /* Get a font from a legacy font name */ 
DiaFont* dia_font_new_from_legacy_name(const char *name);
    /* Get a font matching family,style,height. MUST be freed with
       dia_font_free(). "family" is guaranteed to be a static string. */
DiaFont *dia_font_new_from_static(const char *family, Style style,
                                  real height);

    /* Get a simple font name from a font.
       Name will be valid for the duration of the DiaFont* lifetime. */ 
G_CONST_RETURN char* dia_font_get_legacy_name(const DiaFont* font);

    /* Same attributes */
DiaFont *dia_font_copy(const DiaFont* font);

DiaFont* dia_font_ref(DiaFont* font);
void dia_font_unref(DiaFont* font);

    /* Retrieves the style of the font */
Style dia_font_get_style(const DiaFont* font);

/* These are only for those who mess with Pango fonts directly (widgets.c) */
int dia_font_pango_style_weight_to_dia(int style, int weight);
void dia_font_dia_style_to_pango(int style, PangoStyle *pango_style, PangoWeight *pango_weight);

    /* Retrieves the family of the font. Caller must NOT free. */
G_CONST_RETURN char* dia_font_get_family(const DiaFont* font);

    /* Retrieves the height of the font */
real dia_font_get_height(const DiaFont* font);
    /* Change the height inside a font record. */
void dia_font_set_height(DiaFont* font, real height);

    /* FIXME: what do we do with this, actually ?
       Name lives for as long as the DiaFont lives. */
G_CONST_RETURN char *dia_font_get_psfontname(const DiaFont *font);


/* -------- Font and string functions - unscaled versions.
   Use these version in Objects, primarily. */

    /* Get the width of the string with the given font in cm */
real dia_font_string_width(const char* string, DiaFont* font,
                          real height);
    /* Get the ascent of this string in cm (a positive number). */
real dia_font_ascent(const char* string, DiaFont* font, real height);
    /* Get the descent of the font in cm (a positive number) */
real dia_font_descent(const char* string, DiaFont* font, real height);

    /* prepares a layout of the text, in font 'font'. */
PangoLayout* dia_font_build_layout(const char* string, DiaFont* font,
                                   real height);

/* -------- Font and string functions - scaled versions.
   Use these version in Renderers, exclusively. */

    /* Call once at the beginning of a rendering pass, to let dia know
     what is 1:1 scale. zoom_factor will then be divided by size_one. */
void dia_font_set_nominal_zoom_factor(real size_one);

    /* Get the width of the string with the given font in cm */
real dia_font_scaled_string_width(const char* string, DiaFont* font,
                                  real height, real zoom_factor);
    /* Get the max ascent of the font in cm (a positive number).

    FIXME: may turn out that we need to pass a string here as well. */
real dia_font_scaled_ascent(const char* string, DiaFont* font,
                            real height, real zoom_factor);
    /* Get the max descent of the font in cm (a positive number) 
       FIXME: may turn out that we need to pass a string here as well. */
real dia_font_scaled_descent(const char* string, DiaFont* font,
                             real height, real zoom_factor);

    /* prepares a layout of the text, in font 'font'.

    When zoom_factor != 1.0, may tweak the font's size or stretch so that its
    bounding box is actually linear with respect to the zoom factor (kerning,
    ligaturing and other wild beasts usually get in the way of linear
    scaling). */
PangoLayout* dia_font_scaled_build_layout(const char *string, DiaFont* font,
                                          real height, real zoom_factor);


#endif /* FONT_H */
