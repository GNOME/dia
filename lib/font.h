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
#ifdef HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#endif

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} Alignment;

#ifdef HAVE_FREETYPE
typedef struct _FreetypeFamily FreetypeFamily;
typedef struct _FreetypeFace FreetypeFace;
typedef struct _FreetypeString FreetypeString;

struct _FreetypeFace {
  FT_Face face;
  FreetypeFamily *family;
  char *from_file;
};

struct _FreetypeFamily {
  gchar *family;
  GList *faces;
};
GHashTable *freetype_fonts;

struct _FreetypeString {
  FT_Face face;
  real height;
  gint num_glyphs;
  FT_Glyph *glyphs;
  gchar *text;
  /* The width of this string in pixels */
  real width;
  /* Something that stores a rendering */
};
#endif

typedef struct _DiaFont DiaFont;

#ifdef HAVE_FREETYPE
typedef struct _DiaFontFamily DiaFontFamily;

struct _DiaFontFamily {
  FreetypeFamily *freetype_family;
  GList *diafonts;
};
#endif

struct _DiaFont {
  char *name;
#ifdef HAVE_FREETYPE
  DiaFontFamily *family;
  char *style;
#endif
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

#ifdef HAVE_FREETYPE
typedef void (*BitmapCopyFunc)(FT_GlyphSlot glyph, int x, int y,
			       gpointer userdata);
#endif

void font_init(void);
void font_init_freetype(void);
DiaFont *font_getfont(const char *name);
GdkFont *font_get_gdkfont(DiaFont *font, int height);
SuckFont *font_get_suckfont(DiaFont *font, int height);
char *font_get_psfontname(DiaFont *font);
#ifdef HAVE_FREETYPE
FreetypeString *freetype_load_string(const char *string, FT_Face face, int len);
void freetype_free_string(FreetypeString *fts);
FT_Face font_get_freetypefont(DiaFont *font, real height);
gboolean freetype_file_is_fontfile(char *filename);
void freetype_add_font(char *dirname, char *filename);
void freetype_scan_directory(char *dirname);
void font_init_freetype();
char *font_get_freetypefontname(DiaFont *font);
DiaFont *font_getfont_with_style(const char *name, const char *style);
void freetype_render_string(FreetypeString *fts, int x, int y, 
			    BitmapCopyFunc func, gpointer userdata);
#endif
/* Get the width of the string with the given font in cm */
real font_string_width(const char *string, DiaFont *font, real height);
/* Get the max ascent of the font in cm (a positive number) */
real font_ascent(DiaFont *font, real height);
/* Get the max descent of the font in cm (a positive number) */
real font_descent(DiaFont *font, real height);

#endif /* FONT_H */
