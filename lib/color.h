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

#include "diatypes.h"
#include <gdk/gdk.h>
#include "diavar.h"

/*!
 * \brief Dia's internal color representation
 * \ingroup ObjectParts
 */
struct _Color {
  float red;   /*!< 0..1 */
  float green; /*!< 0..1 */
  float blue;  /*!< 0..1 */
  float alpha; /*!< 0..1 */
};

#define DIA_TYPE_COLOUR (dia_colour_get_type ())


Color        *dia_colour_copy      (Color       *self);
void          dia_colour_free      (Color       *self);
GType         dia_colour_get_type  (void);
void          dia_colour_parse     (Color       *self,
                                    const char  *str);
char         *dia_colour_to_string (Color       *self);
Color        *color_new_rgb        (float        r,
                                    float        g,
                                    float        b);
Color        *color_new_rgba       (float        r,
                                    float        g,
                                    float        b,
                                    float        alpha);
void          color_convert        (const Color *color,
                                    GdkRGBA     *gdkcolor);
gboolean      color_equals         (const Color *color1,
                                    const Color *color2);

static G_GNUC_UNUSED Color color_black = { 0.0f, 0.0f, 0.0f, 1.0f };
static G_GNUC_UNUSED Color color_white = { 1.0f, 1.0f, 1.0f, 1.0f };

#define DIA_COLOR_TO_GDK(from, to)      \
  (to).red = (from).red;        \
  (to).green = (from).green;    \
  (to).blue = (from).blue;      \
  (to).alpha = (from).alpha;

#define GDK_COLOR_TO_DIA(from, to)      \
  (to).red = (from).red;      \
  (to).green = (from).green;            \
  (to).blue = (from).blue;    \
  (to).alpha = (from).alpha;
