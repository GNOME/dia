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

/* Some of the font-definitions were borrowed from xfig.
   Here is the original copyright text from that code:

 * FIG : Facility for Interactive Generation of figures
 * Copyright (c) 1991 by Brian V. Smith
 *
 * The X Consortium, and any party obtaining a copy of these files from
 * the X Consortium, directly or indirectly, is granted, free of charge, a
 * full and unrestricted irrevocable, world-wide, paid up, royalty-free,
 * nonexclusive right and license to deal in this software and
 * documentation files (the "Software"), including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software subject to the restriction stated
 * below, and to permit persons who receive copies from any such party to
 * do so, with the only requirement being that this copyright notice remain
 * intact.
 * This license includes without limitation a license to do the foregoing
 * actions under any patents of the party supplying this software to the 
 * X Consortium.
 */

/*
 * The font suck code was taken from the gnome canvas text object
 * bearing the following copyright header:
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h> /* strlen */

#ifdef HAVE_FREETYPE
#include "X11/Xlib.h"
#include "gdk/gdkx.h"
#include <sys/types.h>
#include <dirent.h>
#endif

#include "intl.h"
#include "utils.h"
#include "font.h"
#include "color.h"
#include "message.h"

#define FONTCACHE_SIZE 17

#define NUM_X11_FONTS 2

typedef struct _FontPrivate FontPrivate;
typedef struct _FontCacheItem FontCacheItem;

struct _FontCacheItem {
  int height;
  GdkFont *gdk_font;
  SuckFont *suck_font;
#ifdef HAVE_FREETYPE
  FT_Face freetype_font;
#endif
};

struct _FontPrivate {
  DiaFont public;
  char *fontname_x11;
  char **fontname_x11_vec;
  char *fontname_ps;
#ifdef HAVE_FREETYPE
  FT_Face fontface_freetype;
#endif
  FontCacheItem *cache[FONTCACHE_SIZE];
  real ascent_ratio, descent_ratio;
};

typedef struct _DiaFontFamily DiaFontFamily;

struct _DiaFontFamily {
  FreetypeFamily *freetype_family;
  GList *diafonts;
};

typedef struct _FontData {
  char *fontname;
  char *fontname_ps;
  char *fontname_x11[NUM_X11_FONTS]; /* First choice */
#ifdef HAVE_FREETYPE
  char *fontname_freetype;
  char *fontstyle_freetype;
#endif
} FontData;

FontData font_data[] = {
  { "Times-Roman",
    "Times-Roman",
    { "-adobe-times-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  }, 
  { "Times-Italic",
    "Times-Italic",
    { "-adobe-times-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  }, 
  { "Times-Bold",
    "Times-Bold",
    { "-adobe-times-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  }, 
  { "Times-BoldItalic",
    "Times-BoldItalic",
    { "-adobe-times-bold-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  }, 
  { "AvantGarde-Book",
    "AvantGarde-Book",
    { "-adobe-avantgarde-book-r-normal-*-%d-*-*-*-*-*-*-*",
      "-schumacher-clean-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "AvantGarde-BookOblique",
    "AvantGarde-BookOblique",
    { "-adobe-avantgarde-book-o-normal-*-%d-*-*-*-*-*-*-*",
      "-schumacher-clean-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "AvantGarde-Demi",
    "AvantGarde-Demi",
    { "-adobe-avantgarde-demibold-r-normal-*-%d-*-*-*-*-*-*-*",
      "-schumacher-clean-bold-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "AvantGarde-DemiOblique",
    "AvantGarde-DemiOblique",
    { "-adobe-avantgarde-demibold-o-normal-*-%d-*-*-*-*-*-*-*",
      "-schumacher-clean-bold-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Bookman-Light",
    "Bookman-Light",
    { "-adobe-bookman-light-r-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Bookman-LightItalic",
    "Bookman-LightItalic",
    { "-adobe-bookman-light-i-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Bookman-Demi",
    "Bookman-Demi",
    { "-adobe-bookman-demibold-r-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-bold-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Bookman-DemiItalic",
    "Bookman-DemiItalic",
    { "-adobe-bookman-demibold-i-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-bold-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
#ifndef G_OS_WIN32
  { "Courier",
    "Courier",
    { "-adobe-courier-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-Oblique",
    "Courier-Oblique",
    { "-adobe-courier-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-Bold",
    "Courier-Bold",
    { "-adobe-courier-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-BoldOblique",
    "Courier-BoldOblique",
    { "-adobe-courier-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
#else /* G_OS_WIN32 */
  /* HB: force usage of true type font "Courier New", using the bitmap
   * version causes scaling problems mainly with uml. FIXME: there
   * must be a better way to do this ?
   */
  { "Courier",
    "Courier",
    { "-adobe-courier new-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-Oblique",
    "Courier-Oblique",
    { "-adobe-courier new-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-Bold",
    "Courier-Bold",
    { "-adobe-courier new-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Courier-BoldOblique",
    "Courier-BoldOblique",
    { "-adobe-courier new-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
#endif
  { "Helvetica",
    "Helvetica",
    { "-adobe-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Oblique",
    "Helvetica-Oblique",
    { "-adobe-helvetica-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Bold",
    "Helvetica-Bold",
    { "-adobe-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-BoldOblique",
    "Helvetica-BoldOblique",
    { "-adobe-helvetica-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Narrow",
    "Helvetica-Narrow",
    { "-adobe-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Narrow-Oblique",
    "Helvetica-Narrow-Oblique",
    { "-adobe-helvetica-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Narrow-Bold",
    "Helvetica-Narrow-Bold",
    { "-adobe-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Helvetica-Narrow-BoldOblique",
    "Helvetica-Narrow-BoldOblique",
    { "-adobe-helvetica-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "NewCenturySchoolbook-Roman",
    "NewCenturySchlbk-Roman",
    { "-adobe-new century schoolbook-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "NewCenturySchoolbook-Italic",
    "NewCenturySchlbk-Italic",
    { "-adobe-new century schoolbook-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "NewCenturySchoolbook-Bold",
    "NewCenturySchlbk-Bold",
    { "-adobe-new century schoolbook-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "NewCenturySchoolbook-BoldItalic",
    "NewCenturySchlbk-BoldItalic",
    { "-adobe-new century schoolbook-bold-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    }
  },
  { "Palatino-Roman",
    "Palatino-Roman",
    { "-adobe-palatino-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-lucidabright-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Palatino-Italic",
    "Palatino-Italic",
    { "-adobe-palatino-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      "-*-lucidabright-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Palatino-Bold",
    "Palatino-Bold",
    { "-adobe-palatino-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-lucidabright-demibold-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Palatino-BoldItalic",
    "Palatino-BoldItalic",
    { "-adobe-palatino-bold-i-normal-*-%d-*-*-*-*-*-*-*",
      "-*-lucidabright-demibold-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "Symbol",
    "Symbol",
    {
      "-adobe-symbol-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-symbol-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "ZapfChancery-MediumItalic",
    "ZapfChancery-MediumItalic",
    { "-adobe-zapf chancery-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      "-*-itc zapf chancery-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    }
  },
  { "ZapfDingbats",
    "ZapfDingbats",
    { "-adobe-zapf dingbats-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-itc zapf dingbats-*-*-*-*-%d-*-*-*-*-*-*-*"
    }
  },
};

#define NUM_FONTS (sizeof(font_data)/sizeof(FontData))

GList *fonts = NULL;
GList *font_names;
GHashTable *fonts_hash = NULL;

char *last_resort_fonts[] = {
  "-adobe-courier-medium-r-normal-*-%d-*-*-*-*-*-*-*",
#ifndef G_OS_WIN32
  "system" /* Must be last. This is guaranteed to exist on a MS-Windows 
              system. */
#else
  "fixed" /* Must be last. This is guaranteed to exist on an X11 system. */
#endif
};
#define NUM_LAST_RESORT_FONTS (sizeof(last_resort_fonts)/sizeof(char *))

static void suck_font_free (SuckFont *suckfont);
static SuckFont *suck_font (GdkFont *font);

static void
init_x11_font(FontPrivate *font)
{
  int i;
  GdkFont *gdk_font = NULL;
  int bufsize;
  char *buffer;
  char *x11_font;
  real height;
  
  fprintf(stderr, "init_x11_font\n");
  for (i=0;i<NUM_X11_FONTS;i++) {
    x11_font = font->fontname_x11_vec[i];
    if (x11_font == NULL)
      break;
    bufsize = strlen(x11_font)+6;  /* Should be enought*/
    buffer = (char *)g_malloc(bufsize);
    g_snprintf(buffer, bufsize, x11_font, 100);
    
    gdk_font = gdk_font_load(buffer);
    if (gdk_font!=NULL) {
      font->fontname_x11 = x11_font;
    }
    g_free(buffer);
    
    if (font->fontname_x11!=NULL)
      break;
  }

  if (font->fontname_x11 == NULL) {
    for (i=0;i<NUM_LAST_RESORT_FONTS;i++) {
      x11_font = last_resort_fonts[i];
      bufsize = strlen(x11_font)+6;  /* Should be enought*/
      buffer = (char *)g_malloc(bufsize);
      g_snprintf(buffer, bufsize, x11_font, 100);
      
      gdk_font = gdk_font_load(buffer);
      g_free(buffer);
      if (gdk_font!=NULL) {
	message_warning(_("Warning no X Font for %s found, \nusing %s instead.\n"), font->public.name, x11_font);
	font->fontname_x11 = x11_font;
	break;
      }
    }
  }

  height = (real)gdk_font->ascent + gdk_font->descent;
  font->ascent_ratio = gdk_font->ascent/height;
  font->descent_ratio = gdk_font->descent/height;

  gdk_font_unref(gdk_font);
}

#ifdef HAVE_FREETYPE
FT_Library  ft_library;

gboolean
freetype_file_is_fontfile(char *filename) {
  int len = strlen(filename);
  if (len < 4) return FALSE;
  if (strcmp(&filename[len-4], ".ttf") == 0) return TRUE;
  /*  if (strcmp(&filename[len-4], ".spd") == 0) return TRUE;*/
  if (strcmp(&filename[len-4], ".pfb") == 0) return TRUE;
  if (strcmp(&filename[len-4], ".pfa") == 0) return TRUE;
  return FALSE;
}

void 
freetype_add_font(char *dirname, char *filename) {
  FT_Face face = NULL, first_face = NULL;
  char *fullname;
  FT_Error error;
  int facenum;

  fullname = g_malloc(strlen(dirname)+strlen(filename)+1+
		      sizeof(G_DIR_SEPARATOR_S));
  sprintf(fullname, "%s%s%s", dirname, G_DIR_SEPARATOR_S, filename);

  facenum = 0;
  do {
    FreetypeFamily *new_font;
    FreetypeFace *new_face;

    error = FT_New_Face(ft_library, fullname, facenum, &face);
    if (error) {
      message_warning("Error reading face from %s:%d", filename, error);
      g_free(fullname);
      return;
    }

    new_font = (FreetypeFamily*)g_hash_table_lookup(freetype_fonts, face->family_name);
    if (new_font == NULL) {
      new_font = (FreetypeFamily*)g_malloc(sizeof(FreetypeFamily));
      new_font->family = strdup(face->family_name);
      new_font->faces = NULL;
      g_hash_table_insert(freetype_fonts, new_font->family, new_font);
    }
    

    new_face = (FreetypeFace*)g_malloc(sizeof(FreetypeFace));
    new_face->face = face;
    new_face->from_file = strdup(fullname);
    new_font->faces = g_list_append(new_font->faces, new_face);
    
    if (facenum == 0) first_face = face;
    facenum++;
  } while (facenum < first_face->num_faces);
  g_free(fullname);
}

void
freetype_scan_directory(char *dirname) {
  DIR *fontdir;
  struct dirent *dirent;

  fprintf(stderr, "freetype_scan_directory %s\n", dirname);
  /* If doing this at other than startup, first remove all old files
     from this dir */

  /* Scan the dir */
  fontdir = opendir(dirname);
  if (fontdir == NULL) {
    message_warning("Couldn't open font dir %s", dirname);
    return;
  }

  while ((dirent = readdir(fontdir)) != NULL) {
    if (freetype_file_is_fontfile(dirent->d_name)) {
      freetype_add_font(dirname, dirent->d_name);
    }
  }

  closedir(fontdir);
}

void
font_init_freetype()
{
  char **fontlist;
  int fontcount;
  int i;
  FT_Error error = FT_Init_FreeType( &ft_library );
  if ( error )
  {
    message_warning(_("Warning: FreeType selected, but library wouldn't load: %d\n"), error);
    return;
  }

  fprintf(stderr, "font_init_freetype\n");
  freetype_fonts = g_hash_table_new(g_str_hash, g_str_equal);

  fontlist = XGetFontPath(GDK_DISPLAY(), &fontcount);
  if (fontlist == NULL || fontcount == 0) {
    message_warning(_("Warning: No X fonts found.  The world is ending."));
    return;
  }

  for (i = 0; i < fontcount; i++) {
    freetype_scan_directory(fontlist[i]);
  }
}

static void
dia_add_freetype_font(char *key, gpointer value, gpointer user_data) {
  FontPrivate *font;
  FreetypeFamily *ft_font = (FreetypeFamily *)value;
  DiaFontFamily *diafonts;
  int j;
  GList *facelist;

  if (g_list_length(ft_font->faces) < 1) {
    fprintf(stderr, "Warning!  Font with no faces: %s\n", key);
    return;
  }
  fprintf(stderr, "Adding font %s with %d faces\n", key, g_list_length(ft_font->faces));

  diafonts = g_new(DiaFontFamily, 1);
  diafonts->freetype_family = ft_font;
  diafonts->diafonts = NULL;

  for (facelist = ft_font->faces; facelist != NULL; facelist = g_list_next(facelist)) {
    FT_Face face = ((FreetypeFace *)facelist->data)->face;

    font = g_new(FontPrivate, 1);
    font->fontface_freetype = face;
    font->public.name = face->family_name;
    font->public.style = face->style_name;

    if (face->ascender > 0 || face->descender > 0) {
      font->ascent_ratio = face->ascender/(face->ascender+face->descender);
      font->descent_ratio = face->descender/(face->ascender+face->descender);
    }

    for (j=0;j<FONTCACHE_SIZE;j++) {
      font->cache[j] = NULL;
    }

    diafonts->diafonts = g_list_append(diafonts->diafonts, font);
  }

  fonts = g_list_append(fonts, diafonts);
  font_names = g_list_append(font_names, key);
  g_hash_table_insert(fonts_hash, key, diafonts);
}

FreetypeFace *
get_freetype_font(const char *fontname, const char *fontstyle)
{
  FreetypeFamily *ft_font;
  GList *face;
  FreetypeFace *ft_face;

  ft_font = (FreetypeFamily*)g_hash_table_lookup(freetype_fonts, fontname);
  if (fontstyle == NULL) fontstyle = "Regular";
  for (face = ft_font->faces; face != NULL; face = g_list_next(face)) {
    ft_face = (FreetypeFace*)face->data;
    if (!strcmp(ft_face->face->style_name, fontstyle))
      break;
  }
  if (face == NULL) {
    if (strcmp(fontstyle, "Regular")) {
      message_warning("%s does not come in %s style, trying Regular",
		      fontname, fontstyle);
      return get_freetype_font(fontname, "Regular");
    }
    message_warning("Can't find Regular style for %s",
		    fontname);
    return NULL;
  }
  return ft_face;
}

FT_Face
font_get_freetypefont(DiaFont *font, real height)
{
  gint error;
  FT_Face face = ((FontPrivate *)font)->fontface_freetype;

  fprintf(stderr, "font_get_freetypefont: %s, %f\n", font->name, height);
  error = FT_Set_Char_Size(face,
			   0,      
			   (int)(height*72*64/2.54),
			   0,
			   0 );
  if (error) {
    message_warning("Can't set %s to size %f\n", face->family_name, height);
    return NULL;
  }

  return face;
}

/* Create a glyphed string from scratch */
FreetypeString *
freetype_load_string(const char *string, FT_Face face, int len)
{
  int i;
  real width = 0.0;
  gboolean use_kerning = FALSE;
  gint glyph_index, previous_index = 0, num_glyphs = 0;
  gint error;
  FreetypeString *fts;

  fprintf(stderr, "freetype_load_string %s len %d\n", string, len);

  fts = (FreetypeString*)g_malloc(sizeof(FreetypeString));
  fts->num_glyphs = len;
  fts->text = strdup(string);
  //  fts->glyphs = (FT_Glyph*)g_malloc(sizeof(FT_Glyph*)*len);
  fts->face = face;
  fts->width = 0.0;

  for (i = 0; i < len; i++) {
    fprintf(stderr, "Glyph #%d: %c\n", i, string[i]);
    // convert character code to glyph index
    glyph_index = FT_Get_Char_Index( face, string[i] );
                              
    // retrieve kerning distance and move pen position
    if ( use_kerning && previous_index && glyph_index )
    {
      FT_Vector  delta;
                                
      FT_Get_Kerning( face, previous_index, glyph_index,
		      ft_kerning_default, &delta );
                                                
      width += delta.x >> 6;
    }
                            
    // store current pen position
    // Why?
    /*
      pos[ num_glyphs ].x = pen_x;
      pos[ num_glyphs ].y = pen_y;
    */                      
    
    // load glyph image into the slot. DO NOT RENDER IT !!
    error = FT_Load_Glyph( face, glyph_index, FT_LOAD_DEFAULT );
    if (error) {
      fprintf(stderr, "Couldn't load glyph #%d: %d\n", i, error);
      continue;
    }
                              
    // extract glyph image and store it in our table
    //    error = FT_Get_Glyph( face->glyph, & fts->glyphs[num_glyphs] );
    //    if (error) continue;  // ignore errors, jump to next glyph
                              
    // increment pen position
    width += face->glyph->advance.x >> 6;
                              
    // record current glyph index
    previous_index = glyph_index;
                              
    // increment number of glyphs
    num_glyphs++;
  }
  fts->width = width*2.54/72.0;
  fprintf(stderr, "Width of %s is %f\n", string, fts->width);
  return fts;
}

void
freetype_free_string(FreetypeString *fts) 
{
  if (!fts) return;
  if (fts->text) g_free(fts->text);
  g_free(fts);
}

void
freetype_copy_glyph_bitmap(GdkPixmap *pixmap, GdkGC *gc,
			   FT_GlyphSlot glyph, int pen_x, int pen_y)
{
  FT_Bitmap *bitmap = &glyph->bitmap;
  guchar *buffer = bitmap->buffer;
  int rowstride = bitmap->pitch;

  fprintf(stderr, "freetype_copy_glyph_bitmap: pen_x %d, wxh = %dx%d, rowstride = %d, pixelmode = %d\n", pen_x, bitmap->width, bitmap->rows, rowstride, bitmap->pixel_mode);
  
  if (rowstride < 0) { // Cartesian bitmap
    buffer = buffer+rowstride*(bitmap->rows-1);
  }

  gdk_draw_gray_image(pixmap, gc, pen_x, pen_y,
		      bitmap->width, bitmap->rows,
		      GDK_RGB_DITHER_NONE,
		      bitmap->buffer, rowstride);
}

void
freetype_render_string(GdkPixmap *pixmap, FreetypeString *fts,
		       GdkGC *gc, int x, int y)
{
  gchar *string;
  int i, len;
  int previous_index = 0;
  int use_kerning = FALSE;
  int pen_x = 0, pen_y = 0;
  FT_Face face = fts->face;

  fprintf(stderr, "freetype_render_string\n");
  string = fts->text;
  len = strlen(string);
  for (i = 0; i < len; i++) {
    // convert character code to glyph index
    int glyph_index = FT_Get_Char_Index( face, string[i] );
    int error;

    fprintf(stderr, "Rendering for %c\n", string[i]);

    if (glyph_index == -1) {
      fprintf(stderr, "FT_Get_Char_Index: Couldn't find glyph\n");
      continue;
    }
                              
    // retrieve kerning distance and move pen position
    if ( use_kerning && previous_index && glyph_index )
    {
      FT_Vector  delta;
                                
      error = FT_Get_Kerning( face, previous_index, glyph_index,
			      ft_kerning_default, &delta );
      if (error) {
	fprintf(stderr, "FT_Get_Kerning error %d\n", error);
      }                                                
    }
                            
    // store current pen position
    // Why?
    /*
      pos[ num_glyphs ].x = pen_x;
      pos[ num_glyphs ].y = pen_y;
    */                      
    
    // load glyph image into the slot. DO NOT RENDER IT !!
    error = FT_Load_Glyph( face, glyph_index, FT_LOAD_NO_BITMAP );
    if (error) continue;  // ignore errors, jump to next glyph

    error = FT_Render_Glyph( face, ft_render_mode_normal );
    if (error) continue;  // ignore errors, jump to next glyph

    fprintf(stderr, "Copy bitmap\n");
    // now, draw to our target surface
    freetype_copy_glyph_bitmap( pixmap, gc, &face->glyph,
				pen_x + face->glyph->bitmap_left,
				pen_y - face->glyph->bitmap_top );
                         
    // increment pen position 
    pen_x += face->glyph->advance.x >> 6;
    pen_y += face->glyph->advance.y >> 6;   // unuseful for now..
  }
}
#endif

void
font_init(void)
{
  int i,j;
  FontPrivate *font;

  fonts_hash = g_hash_table_new((GHashFunc)g_str_hash,
			       (GCompareFunc)g_str_equal);

#ifdef HAVE_FREETYPE
  font_init_freetype();
  g_hash_table_foreach(freetype_fonts, dia_add_freetype_font, NULL);
#else
  for (i=0;i<NUM_FONTS;i++) {
    font = g_new(FontPrivate, 1);
    font->public.name = font_data[i].fontname;
    font->fontname_ps = font_data[i].fontname_ps;

    font->fontname_x11 = NULL;
    font->fontname_x11_vec = font_data[i].fontname_x11;
    
    for (j=0;j<FONTCACHE_SIZE;j++) {
      font->cache[j] = NULL;
    }

    fonts = g_list_append(fonts, font);
    font_names = g_list_append(font_names, font->public.name);
    g_hash_table_insert(fonts_hash, font->public.name, font);
  }
#endif
}

#ifdef HAVE_FREETYPE
DiaFont *
font_getfont_with_style(const char *name, const char *style)
{
  DiaFontFamily *fonts;
  GList *faces;

  fprintf(stderr, "font_getfont_with_style %s %s\n", name, style);
  g_assert(name!=NULL);
  fonts = (DiaFontFamily *)g_hash_table_lookup(fonts_hash, name);

  if (fonts == NULL) {
    fonts = g_hash_table_lookup(fonts_hash, "Courier");
    if (fonts == NULL) {
      fprintf(stderr, "Error, couldn't locate font. Shouldn't happen.\n");
      message_error(_("Error, couldn't locate font. Shouldn't happen.\n"));
    } else {
      fprintf(stderr, _("Font %s not found, using Courier instead.\n"), name);
      message_notice(_("Font %s not found, using Courier instead.\n"), name);
    }
  }
 
  for (faces = fonts->diafonts; faces != NULL; faces = g_list_next(faces)) {
    if (!strcmp(((DiaFont *)faces->data)->style, style)) {
      return (DiaFont *)faces->data;
    }
  }

  message_notice(_("Font %s has no style %s, using %s\n"),
		 ((DiaFont *)fonts->diafonts->data)->style);
  return (DiaFont *)fonts->diafonts->data;
}
#endif

DiaFont *
font_getfont(const char *name)
{
#ifdef HAVE_FREETYPE
  return font_getfont_with_style(name, "Regular");
#else
  FontPrivate *font;

  fprintf(stderr, "font_getfont %s\n", name);
  g_assert(name!=NULL);
  font = (FontPrivate *)g_hash_table_lookup(fonts_hash, (char *)name);

  if (font == NULL) {
    font = g_hash_table_lookup(fonts_hash, "Courier");
    if (font == NULL) {
      message_error("Error, couldn't locate font. Shouldn't happend.\n");
    } else {
      message_notice(_("Font %s not found, using Courier instead.\n"), name);
    }
  }


  if (font->fontname_x11 == NULL)
    init_x11_font(font);

  return (DiaFont *)font;
#endif  
}

static FontCacheItem *
font_get_cache(FontPrivate *font, int height)
{
  int index;

  fprintf(stderr, "font_get_cache\n");
  g_assert(font!=NULL);

  if (height<=0)
    height = 1;
  
  index = height % FONTCACHE_SIZE;
  
  if (font->cache[index]==NULL) {
    font->cache[index] = g_new(FontCacheItem, 1);
    font->cache[index]->height = height;
    font->cache[index]->gdk_font = NULL;
    font->cache[index]->suck_font = NULL;
  } else if (font->cache[index]->height != height) {
    gdk_font_unref(font->cache[index]->gdk_font);
    if (font->cache[index]->suck_font)
      suck_font_free(font->cache[index]->suck_font);
    font->cache[index]->height = height;
    font->cache[index]->gdk_font = NULL;
    font->cache[index]->suck_font = NULL;
  }
  return font->cache[index];
}

static GdkFont *
font_get_gdkfont_helper(FontPrivate *font, int height)
{
  int bufsize;
  char *buffer;
  GdkFont *gdk_font;
  
  fprintf(stderr, "font_get_gdkfont_helper\n");
  bufsize = strlen(font->fontname_x11)+6;  /* Should be enought*/
  buffer = (char *)malloc(bufsize);
  g_snprintf(buffer, bufsize, font->fontname_x11, height);
  gdk_font = gdk_font_load(buffer);
  free(buffer);

  return gdk_font;
}

GdkFont *
font_get_gdkfont(DiaFont *font, int height)
{
  FontCacheItem *cache_item;
  FontPrivate *fontprivate;

  fprintf(stderr, "font_get_gdkfont\n");
  g_assert(font!=NULL);

  fontprivate = (FontPrivate *)font;

  cache_item = font_get_cache(fontprivate, height);

  if (cache_item->gdk_font)
    return cache_item->gdk_font;
  
  /* Not in cache: */
  
  cache_item->gdk_font = font_get_gdkfont_helper(fontprivate, cache_item->height);

  return cache_item->gdk_font;
}

SuckFont *
font_get_suckfont(DiaFont *font, int height)
{
  FontCacheItem *cache_item;
  FontPrivate *fontprivate;

  fprintf(stderr, "font_get_suckfont\n");
  g_assert(font!=NULL);

  fontprivate = (FontPrivate *)font;

  cache_item = font_get_cache(fontprivate, height);

  if (!cache_item->gdk_font) {
    /* gdk_font not in cache: */
    cache_item->gdk_font = font_get_gdkfont_helper(fontprivate, cache_item->height);
  }

  if (!cache_item->suck_font) {
    /* Not in cache: */
    cache_item->suck_font = suck_font(cache_item->gdk_font);
  }

  return cache_item->suck_font;
}

char *
font_get_psfontname(DiaFont *font)
{
  FontPrivate *fontprivate;

  fprintf(stderr, "font_get_psfontname\n");
  g_assert(font!=NULL);

  fontprivate = (FontPrivate *)font;
  
  return fontprivate->fontname_ps;
}

real
font_string_width(const char *string, DiaFont *font, real height)
{
#ifdef HAVE_FREETYPE
  FT_Face *face;
  FreetypeString *ft_string;

  /* This is currently broken */
  fprintf(stderr, "font_string_width: %s %s\n", font->name, font->style);
  face = font_get_freetypefont(font, height);
  ft_string = freetype_load_string(string, face, strlen(string));

  return ft_string->width;
#else
  GdkFont *gdk_font;
  int iwidth, iheight;
  double width_height;
  /* Note: This is an ugly hack. It tries to overestimate the width with
     some magic stuff. No guarantees. */
  gdk_font = font_get_gdkfont(font, 100);
  iwidth = gdk_string_width(gdk_font, string);
  iheight = gdk_string_height(gdk_font, string);

  if ((iwidth==0) || (iheight==0))
    return 0.0;
  
  width_height = ((real)iwidth)/((real)iheight);
  width_height *= 1.01;
  return width_height*height*(iheight/100.0) + 0.2;
#endif
}

real
font_ascent(DiaFont *font, real height)
{
  FontPrivate *fontprivate;

  fprintf(stderr, "font_ascent\n");
  fontprivate = (FontPrivate *)font;
  return height*fontprivate->ascent_ratio;
}

real
font_descent(DiaFont *font, real height)
{
  FontPrivate *fontprivate;

  fprintf(stderr, "font_descent\n");
  fontprivate = (FontPrivate *)font;
  return height*fontprivate->descent_ratio;
}

/* Routines for sucking fonts from the X server */

static SuckFont *
suck_font (GdkFont *font)
{
	SuckFont *suckfont;
	int i;
	int x, y;
	char text[1];
	int lbearing, rbearing, ch_width, ascent, descent;
	GdkPixmap *pixmap;
	GdkColor black, white;
	GdkImage *image;
	GdkGC *gc;
	guchar *line;
	int width, height;
	int black_pixel, pixel;

	fprintf(stderr, "suck_font \n");
	if (!font)
		return NULL;

	suckfont = g_new (SuckFont, 1);

	height = font->ascent + font->descent;
	x = 0;
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_text_extents (font, text, 1,
				  &lbearing, &rbearing, &ch_width, &ascent, &descent);
		suckfont->chars[i].left_sb = lbearing;
		suckfont->chars[i].right_sb = ch_width - rbearing;
		suckfont->chars[i].width = rbearing - lbearing;
		suckfont->chars[i].ascent = ascent;
		suckfont->chars[i].descent = descent;
		suckfont->chars[i].bitmap_offset = x;
		x += (ch_width + 31) & -32;
	}

	width = x;

	suckfont->bitmap_width = width;
	suckfont->bitmap_height = height+1;
	suckfont->ascent = font->ascent+1;

	pixmap = gdk_pixmap_new (NULL, suckfont->bitmap_width,
				 suckfont->bitmap_height, 1);
	gc = gdk_gc_new (pixmap);
	gdk_gc_set_font (gc, font);

	/* this is a black and white pixmap: */
	black.pixel = 0;
	white.pixel = 1;
	black_pixel = black.pixel;
	gdk_gc_set_foreground (gc, &white);
	gdk_draw_rectangle (pixmap, gc, 1, 0, 0, suckfont->bitmap_width, suckfont->bitmap_height);

	gdk_gc_set_foreground (gc, &black);
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_draw_text (pixmap, font, gc,
			       suckfont->chars[i].bitmap_offset - suckfont->chars[i].left_sb,
			       font->ascent+1,
			       text, 1);
	}

	/* The handling of the image leaves me with distinct unease.  But this
	 * is more or less copied out of gimp/app/text_tool.c, so it _ought_ to
	 * work. -RLL
	 */

	image = gdk_image_get (pixmap, 0, 0, suckfont->bitmap_width, suckfont->bitmap_height);
	suckfont->bitmap = g_malloc0 ((width >> 3) * suckfont->bitmap_height);

	line = suckfont->bitmap;
	for (y = 0; y < suckfont->bitmap_height; y++) {
		for (x = 0; x < suckfont->bitmap_width; x++) {
			pixel = gdk_image_get_pixel (image, x, y);
			if (pixel == black_pixel)
				line[x >> 3] |= 128 >> (x & 7);
		}
		line += width >> 3;
	}

	gdk_image_destroy (image);

	/* free the pixmap */
	gdk_pixmap_unref (pixmap);

	/* free the gc */
	gdk_gc_destroy (gc);

	return suckfont;
}

static void
suck_font_free (SuckFont *suckfont)
{
	g_free (suckfont->bitmap);
	g_free (suckfont);
}

