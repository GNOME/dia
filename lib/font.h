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

#include <gdk/gdk.h>
#include "geometry.h"
#include "diavar.h"

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} Alignment;


typedef struct _DiaFont DiaFont;

struct _DiaFont {
  char *name;
};

typedef struct _SuckFont SuckFont;
typedef struct _SuckChar SuckChar;

struct _SuckChar {
	int     left_sb;
	int     right_sb;
	int     width;
	int     ascent;
	int     descent;
	int     bitmap_offset; /* in pixels */
};

struct _SuckFont {
	guchar *bitmap;
	gint    bitmap_width;
	gint    bitmap_height;
	gint    ascent;
	SuckChar chars[256];
};


DIAVAR GList *font_names; /* GList with 'char *' data.*/

void font_init(void);
void font_init_freetype(void);
DiaFont *font_getfont(const char *name);
GdkFont *font_get_gdkfont(DiaFont *font, int height);
SuckFont *font_get_suckfont (GdkFont *font, gchar *text);
void suck_font_free (SuckFont *suckfont);
char *font_get_psfontname(DiaFont *font);
/* Get the width of the string with the given font in cm */
real font_string_width(const char *string, DiaFont *font, real height);
/* Get the max ascent of the font in cm (a positive number) */
real font_ascent(DiaFont *font, real height);
/* Get the max descent of the font in cm (a positive number) */
real font_descent(DiaFont *font, real height);

#endif /* FONT_H */
