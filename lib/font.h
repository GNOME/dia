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
#include <glib-object.h>
#include <pango/pango.h>

#include "diatypes.h"
#include "dia-enums.h"
#include "geometry.h"

G_BEGIN_DECLS

/* Do NOT put these strings through the .po mechanism. If you need to add
   non-Roman alternatives, please insert them in the list */
#define BASIC_SANS_FONT "arial, helvetica, helv, sans"
#define BASIC_SERIF_FONT "times new roman, times, serif"
#define BASIC_MONOSPACE_FONT "courier new, courier, monospace, fixed"


/*
 * In a goodly selection of fonts, 500 is very common, yet Pango doesn't name it.
 * I am calling 500 'medium' and 600 'demibold'.
 * We should really have a more flexible system where 300 or 400 is normal,
 * the next one up bold, or something.  But this will do for now.
 * We should probably store the actual weight...
 */

/* We are having DIA_FONT_WEIGHT_NORMAL be 0 to avoid having to do
 * DIA_FONT_MONOSPACE | DIA_FONT_WEIGHT_NORMAL all over the place.
 * This introduces a few hacks in font.c and widgets.c, but not too
 * many.
 */

/* storing different style info like
 * (DIA_FONT_SANS | DIA_FONT_ITALIC | DIA_FONT_BOLD)
 */
typedef guint DiaFontStyle;

typedef enum /*< flags >*/ {
  DIA_FONT_FAMILY_ANY = 0,
  DIA_FONT_SANS       = 1,
  DIA_FONT_SERIF      = 2,
  DIA_FONT_MONOSPACE  = 3
} DiaFontFamily;

typedef enum /*< flags >*/ {
  DIA_FONT_NORMAL  = (0<<2),
  DIA_FONT_OBLIQUE = (1<<2),
  DIA_FONT_ITALIC  = (2<<2)
} DiaFontSlant;

typedef enum /*< flags >*/ {
  DIA_FONT_ULTRALIGHT    = (1<<4),
  DIA_FONT_LIGHT         = (2<<4),
  DIA_FONT_WEIGHT_NORMAL = (0<<4), /* Deliberately 0 for DIA_FONT_NORMAL */
  DIA_FONT_MEDIUM        = (3<<4),
  DIA_FONT_DEMIBOLD      = (4<<4),
  DIA_FONT_BOLD          = (5<<4),
  DIA_FONT_ULTRABOLD     = (6<<4),
  DIA_FONT_HEAVY         = (7<<4)
} DiaFontWeight;

/* macros to get a specific style info */
#define DIA_FONT_STYLE_GET_FAMILY(st)    ((st) & (0x3))
#define DIA_FONT_STYLE_GET_SLANT(st)     ((st) & (0x3<<2))
#define DIA_FONT_STYLE_GET_WEIGHT(st)    ((st) & (0x7<<4))


#define DIA_TYPE_FONT dia_font_get_type ()
G_DECLARE_FINAL_TYPE (DiaFont, dia_font, DIA, FONT, GObject)


PangoContext               *dia_font_get_context            (void);
DiaFont                    *dia_font_new                    (const char       *family,
                                                             DiaFontStyle      style,
                                                             double            height);
DiaFont                    *dia_font_new_from_style         (DiaFontStyle      style,
                                                             double            height);
DiaFont                    *dia_font_new_from_legacy_name   (const char       *name);
const char                 *dia_font_get_legacy_name        (DiaFont          *font);
DiaFont                    *dia_font_copy                   (DiaFont          *font);
DiaFontStyle                dia_font_get_style              (DiaFont          *font);
const char                 *dia_font_get_family             (DiaFont          *font);
const PangoFontDescription *dia_font_get_description        (DiaFont          *font);
double                      dia_font_get_height             (DiaFont          *font);
void                        dia_font_set_height             (DiaFont          *font,
                                                             double            height);
double                      dia_font_get_size               (DiaFont          *font);
void                        dia_font_set_slant              (DiaFont          *font,
                                                             DiaFontSlant      slant);
void                        dia_font_set_weight             (DiaFont          *font,
                                                             DiaFontWeight     weight);
void                        dia_font_set_family             (DiaFont          *font,
                                                             DiaFontFamily     family);
void                        dia_font_set_any_family         (DiaFont          *font,
                                                             const char       *family);
const char                 *dia_font_get_psfontname         (DiaFont          *font);
const char                 *dia_font_get_weight_string      (DiaFont          *font);
const char                 *dia_font_get_slant_string       (DiaFont          *font);
void                        dia_font_set_weight_from_string (DiaFont          *font,
                                                             const char       *weight);
void                        dia_font_set_slant_from_string  (DiaFont          *font,
                                                             const char       *slant);

/* -------- Font and string functions - unscaled versions.
   Use these version in Objects, primarily. */

double                      dia_font_string_width           (const char       *string,
                                                             DiaFont          *font,
                                                             double            height);
double                      dia_font_ascent                 (const char       *string,
                                                             DiaFont          *font,
                                                             double            height);
double                      dia_font_descent                (const char       *string,
                                                             DiaFont          *font,
                                                             double            height);
PangoLayout                *dia_font_build_layout           (const char       *string,
                                                             DiaFont          *font,
                                                             double            height);
double                     *dia_font_get_sizes              (const char       *string,
                                                             DiaFont          *font,
                                                             double            height,
                                                             double           *width,
                                                             double           *ascent,
                                                             double           *descent,
                                                             int              *n_offsets,
                                                             PangoLayoutLine **layout_offsets);

/* -------- Font and string functions - scaled versions.
   Use these version in Renderers, exclusively. */

G_END_DECLS
