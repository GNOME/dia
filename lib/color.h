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
#ifndef COLOR_H
#define COLOR_H

#include <gdk/gdk.h>

typedef struct _Color Color;

struct _Color {
  float red;
  float green;
  float blue;
};

void color_init(void);
void color_convert(Color *color, GdkColor *gdkcolor);
gboolean color_equals(Color *color1, Color *color2);

#ifdef G_OS_WIN32
#  ifdef LIBDIA_COMPILATION
#    define DIAVAR __declspec(dllexport)
#  else  /* !LIBDIA_COMPILATION */
#    define DIAVAR extern __declspec(dllimport)
#  endif /* !LIBDIA_COMPILATION */
#else  /* !G_OS_WIN32 */
#  define DIAVAR extern
#endif

DIAVAR Color color_black, color_white;
DIAVAR GdkColor color_gdk_black, color_gdk_white;

#endif /* COLOR_H */
