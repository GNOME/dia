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

#include <gdk/gdk.h>

G_BEGIN_DECLS

/**
 * DiaColour:
 *
 * Dia's internal colour representation
 */
typedef struct _Color DiaColour;
struct _Color {
  float red;   /*!< 0..1 */
  float green; /*!< 0..1 */
  float blue;  /*!< 0..1 */
  float alpha; /*!< 0..1 */
};

#define DIA_TYPE_COLOUR (dia_colour_get_type ())


DiaColour    *dia_colour_copy      (DiaColour   *self);
void          dia_colour_free      (DiaColour   *self);
GType         dia_colour_get_type  (void);
void          dia_colour_parse     (DiaColour   *self,
                                    const char  *str);
char         *dia_colour_to_string (DiaColour   *self);
DiaColour    *dia_colour_new_rgb   (float        r,
                                    float        g,
                                    float        b);
DiaColour    *dia_colour_new_rgba  (float        r,
                                    float        g,
                                    float        b,
                                    float        alpha);
void          dia_colour_as_gdk    (DiaColour   *self,
                                    GdkRGBA     *rgba);
void          dia_colour_from_gdk  (DiaColour   *self,
                                    GdkRGBA     *rgba);
gboolean      dia_colour_equals    (DiaColour   *colour_a,
                                    DiaColour   *colour_b);
gboolean      dia_colour_is_black  (DiaColour   *self);
gboolean      dia_colour_is_white  (DiaColour   *self);


#define DIA_COLOUR_BLACK ((DiaColour) { 0.0f, 0.0f, 0.0f, 1.0f })
#define DIA_COLOUR_WHITE ((DiaColour) { 1.0f, 1.0f, 1.0f, 1.0f })

G_END_DECLS
