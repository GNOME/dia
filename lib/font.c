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

/* TODO:
   BoundingBox calculcations.
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

#include "utils.h"
#include "color.h"
#include "message.h"
#include "intl.h"
#include "text.h"
#include "charconv.h" 
#include "font.h"

#ifndef LARS_TRACE_MESSAGES
#define LC_DEBUG(op)
#else
#define LC_DEBUG(op) op
#endif

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

typedef struct _FontData {
  char *fontname;
  char *fontname_ps;
  char *fontname_x11[NUM_X11_FONTS]; /* First choice */
  char *fontname_freetype; /* Including these in non-freetype doesn't harm */
  char *fontstyle_freetype;
} FontData;

FontData font_data[] = {
  { "Times-Roman",
    "Times-Roman",
    { "-adobe-times-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Times New Roman", "Regular"
  }, 
  { "Times-Italic",
    "Times-Italic",
    { "-adobe-times-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Times New Roman", "Italic"
  }, 
  { "Times-Bold",
    "Times-Bold",
    { "-adobe-times-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Times New Roman", "Bold"
  }, 
  { "Times-BoldItalic",
    "Times-BoldItalic",
    { "-adobe-times-bold-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Times New Roman", "Bold Italic"
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
  { "Batang",
	"Batang",
	{ "-*-batang-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "BousungEG-Light-GB",
	"BousungEG-Light-GB",
	{ "-*-ar pl sungtil gb-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Bookman-Light",
    "Bookman-Light",
    { "-adobe-bookman-light-r-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    },
    "URW Bookman L", "Light"
  },
  { "Bookman-LightItalic",
    "Bookman-LightItalic",
    { "-adobe-bookman-light-i-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    },
    "URW Bookman L", "Light Italic"
  },
  { "Bookman-Demi",
    "Bookman-Demi",
    { "-adobe-bookman-demibold-r-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-bold-r-normal-*-%d-*-*-*-*-*-*-*"
    },
    "URW Bookman L", "Demi Bold"
  },
  { "Bookman-DemiItalic",
    "Bookman-DemiItalic",
    { "-adobe-bookman-demibold-i-normal-*-%d-*-*-*-*-*-*-*",
      "-adobe-times-bold-i-normal-*-%d-*-*-*-*-*-*-*"
    },
    "URW Bookman L", "Demi Bold Italic"
  },
#ifndef G_OS_WIN32
  { "Courier",
    "Courier",
    { "-adobe-courier-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Courier New", "Regular"
  },
  { "Courier-Oblique",
    "Courier-Oblique",
    { "-adobe-courier-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Courier New", "Italic"
  },
  { "Courier-Bold",
    "Courier-Bold",
    { "-adobe-courier-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Courier New", "Bold "
  },
  { "Courier-BoldOblique",
    "Courier-BoldOblique",
    { "-adobe-courier-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Courier New", "Bold Italic"
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
  { "Dotum",
	"Dotum",
	{ "-*-dotum-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "GBZenKai-Medium",
	"GBZenKai-Medium",
	{ "-*-ar pl kaitim gb-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "GothicBBB-Medium",
	"GothicBBB-Medium",
	{ "-*-gothic-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Gulim",
	"Gulim",
	{ "-*-gulim-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Headline",
	"Headline",
	{ "-*-headline-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Helvetica",
    "Helvetica",
    { "-adobe-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Regular"
  },
  { "Helvetica-Oblique",
    "Helvetica-Oblique",
    { "-adobe-helvetica-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Italic"
  },
  { "Helvetica-Bold",
    "Helvetica-Bold",
    { "-adobe-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Bold"
  },
  { "Helvetica-BoldOblique",
    "Helvetica-BoldOblique",
    { "-adobe-helvetica-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Bold Italic"
  },
  { "Helvetica-Narrow",
    "Helvetica-Narrow",
    { "-adobe-helvetica-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Regular Condensed"
  },
  { "Helvetica-Narrow-Oblique",
    "Helvetica-Narrow-Oblique",
    { "-adobe-helvetica-medium-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Italic Condensed"
  },
  { "Helvetica-Narrow-Bold",
    "Helvetica-Narrow-Bold",
    { "-adobe-helvetica-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Bold Condensed"
  },
  { "Helvetica-Narrow-BoldOblique",
    "Helvetica-Narrow-BoldOblique",
    { "-adobe-helvetica-bold-o-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Nimbus Sans L", "Bold Italic Condensed"
  },
  { "MOESung-Medium",
	"MOESung-Medium",
	{ "-*-ar pl kaitim big5-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "NewCenturySchoolbook-Roman",
    "NewCenturySchlbk-Roman",
    { "-adobe-new century schoolbook-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Century Schoolbook L", "Roman"
  },
  { "NewCenturySchoolbook-Italic",
    "NewCenturySchlbk-Italic",
    { "-adobe-new century schoolbook-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Century Schoolbook L", "Italic"
  },
  { "NewCenturySchoolbook-Bold",
    "NewCenturySchlbk-Bold",
    { "-adobe-new century schoolbook-bold-r-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Century Schoolbook L", "Bold"
  },
  { "NewCenturySchoolbook-BoldItalic",
    "NewCenturySchlbk-BoldItalic",
    { "-adobe-new century schoolbook-bold-i-normal-*-%d-*-*-*-*-*-*-*",
      NULL
    },
    "Century Schoolbook L", "Bold Italic"
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
  { "Ryumin-Light",
	"Ryumin-Light",
	{ "-*-mincho-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "ShanHeiSun-Light",
	"ShanHeiSun-Light",
	{ "-*-ar pl mingti2l big5-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Song-Medium",
	"Song-Medium",
	{ "-*-ar pl kaitim gb-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
	}
  },
  { "Symbol",
    "Symbol",
    {
      "-adobe-symbol-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-symbol-medium-r-normal-*-%d-*-*-*-*-*-*-*"
    },
    "Standard Symbols L", "Regular"
  },
  { "ZapfChancery-MediumItalic",
    "ZapfChancery-MediumItalic",
    { "-adobe-zapf chancery-medium-i-normal-*-%d-*-*-*-*-*-*-*",
      "-*-itc zapf chancery-medium-i-normal-*-%d-*-*-*-*-*-*-*"
    },
    "URW Chancery L", "Medium Italic"
  },
  { "ZapfDingbats",
    "ZapfDingbats",
    { "-adobe-zapf dingbats-medium-r-normal-*-%d-*-*-*-*-*-*-*",
      "-*-itc zapf dingbats-*-*-*-*-%d-*-*-*-*-*-*-*"
    },
    "Dingbats", ""
  },
  { "ZenKai-Medium",
	"ZenKai-Medium",
	{ "-*-ar pl kaitim big5-medium-r-normal-*-%d-*-*-*-*-*-*-*",
	  NULL
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

static void
init_x11_font(FontPrivate *font)
{
  int i;
  GdkFont *gdk_font = NULL;
  int bufsize;
  char *buffer;
  char *x11_font;
  real height;
  
  LC_DEBUG (fprintf(stderr, "init_x11_font\n"));
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

#ifdef HAVE_UNICODE
    FT_Select_Charmap(face, ft_encoding_unicode);
#endif

    if (facenum == 0) first_face = face;
    facenum++;
  } while (facenum < first_face->num_faces);
  g_free(fullname);
}

void
freetype_scan_directory(char *dirname) {
  DIR *fontdir;
  struct dirent *dirent;

  /* If doing this at other than startup, first remove all old files
     from this dir */
  if(dirname[0] != '/') return;

  /* Scan the dir */
  fontdir = opendir(dirname);
  if (fontdir == NULL) {
    /* Some X font dirs are bogus: The :unscaled ones */
    /*    message_warning("Couldn't open font dir %s", dirname); */
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

  LC_DEBUG (fprintf(stderr, "font_init_freetype\n"));
  freetype_fonts = g_hash_table_new(g_str_hash, g_str_equal);

  fontlist = XGetFontPath(GDK_DISPLAY(), &fontcount);
  if (fontlist == NULL || fontcount == 0) {
    message_warning(_("Warning: No X fonts found.  The world is ending."));
    return;
  }

  for (i = 0; i < fontcount; i++) {
    freetype_scan_directory(fontlist[i]);
  }

  if (g_hash_table_size(freetype_fonts) == 0) {
    message_warning(_("Warning: No fonts loaded.  Are there any font files in the X font path?"));
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
    LC_DEBUG (fprintf(stderr, "Warning!  Font with no faces: %s\n", key));
    return;
  }
  LC_DEBUG (fprintf(stderr, "Adding font %s with %d faces\n", key, g_list_length(ft_font->faces)));

  diafonts = g_new(DiaFontFamily, 1);
  diafonts->freetype_family = ft_font;
  diafonts->diafonts = NULL;

  for (facelist = ft_font->faces; facelist != NULL; facelist = g_list_next(facelist)) {
    FT_Face face = ((FreetypeFace *)facelist->data)->face;

    font = g_new(FontPrivate, 1);
    font->fontface_freetype = face;
    font->public.name = face->family_name;
    font->public.style = face->style_name;
    font->public.family = diafonts;

    if (face->ascender != 0 || face->descender != 0) {
      int descent = abs(face->descender);
      // The -1 is a piece of magic.  Probably rounding errors.
      font->ascent_ratio = ((real)face->ascender-1)/(face->ascender+descent);
      font->descent_ratio = ((real)descent)/(face->ascender+descent);
    }

    for (j=0;j<FONTCACHE_SIZE;j++) {
      font->cache[j] = NULL;
    }

    /* Need to deal with spaces in names here */
    font->fontname_ps = g_strdup(FT_Get_Postscript_Name(face));
    /*
    if (!strcmp(font->public.style, "Regular"))
      font->fontname_ps = g_strdup(font->public.name);
    else
      font->fontname_ps = g_strdup_printf("%s-%s", font->public.name, font->public.style);
    */
    diafonts->diafonts = g_list_append(diafonts->diafonts, font);
  }

  fonts = g_list_append(fonts, diafonts);
  font_names = g_list_append(font_names, key);
  g_hash_table_insert(fonts_hash, key, diafonts);
}

FT_Face
font_get_freetypefont(DiaFont *font, real height)
{
  gint error;
  FT_Face face = ((FontPrivate *)font)->fontface_freetype;

  LC_DEBUG (fprintf(stderr, "font_get_freetypefont: %s, %f\n", font->name, height));
  error = FT_Set_Char_Size(face,
			   0,      
			   (int)(height*64),
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
  gboolean use_kerning = TRUE;
  gint glyph_index, previous_index = 0, num_glyphs = 0;
  gint error;
  FreetypeString *fts;
  utfchar* p;

  LC_DEBUG (fprintf(stderr, "freetype_load_string %s len %d\n", string, len));

  fts = (FreetypeString*)g_malloc(sizeof(FreetypeString));
  fts->text = g_strndup(string, len);
  //  fts->glyphs = (FT_Glyph*)g_malloc(sizeof(FT_Glyph*)*len);
  fts->face = face;
  fts->width = 0.0;

  num_glyphs = 0;
  for (p = string; (*p); p = uni_next(p)) {
    unichar c;
    uni_get_utf8(p,&c);

    // If len is less that full string, stop here.
    if (num_glyphs == len) break;

    // store current pen position
    // Why?
    /*
      pos[ num_glyphs ].x = pen_x;
      pos[ num_glyphs ].y = pen_y;
    */                      
    
    // load glyph image into the slot.
    error = FT_Load_Char( face, c, FT_LOAD_NO_HINTING );
    if (error) {
      LC_DEBUG (fprintf(stderr, "Couldn't load glyph #%d: %d\n", i, error));
      continue;
    }

    // retrieve kerning distance and move pen position
    if ( use_kerning && previous_index && glyph_index )
    {
      FT_Vector  delta;
                                
      FT_Get_Kerning( face, previous_index, glyph_index,
		      ft_kerning_default, &delta );
      if (error) {
	LC_DEBUG (fprintf(stderr, "FT_Get_Kerning error %d\n", error));
      } else {
	width += delta.x >> 6;
      }
    }
                              
    // increment pen position
    width += face->glyph->advance.x >> 6;

    // record current glyph index
    previous_index = glyph_index;
    
    // increment number of glyphs
    num_glyphs++;
  }
  fts->width = width;
  LC_DEBUG (fprintf(stderr, "Width of %s is %f\n", string, fts->width));
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
freetype_render_string(FreetypeString *fts, int x, int y, 
		       BitmapCopyFunc func, gpointer userdata)
{
  gchar *string;
  int i, len;
  int glyph_index, previous_index = 0;
  int use_kerning = TRUE;
  int pen_x = x, pen_y = y;
  FT_Face face = fts->face;
  int error;
  utfchar *p;

  LC_DEBUG (fprintf(stderr, "freetype_render_string\n"));

  string = fts->text;

  for (p = string; (*p); p = uni_next(p)) {
    unichar c;
    uni_get_utf8(p,&c);

    // store current pen position
    // Why?
    /*
      pos[ num_glyphs ].x = pen_x;
      pos[ num_glyphs ].y = pen_y;
    */                      
    
    // load glyph image into the slot. DO NOT RENDER IT !!
    error = FT_Load_Char( face, c, FT_LOAD_NO_HINTING | FT_LOAD_RENDER );
    if (error) continue;  // ignore errors, jump to next glyph

    // retrieve kerning distance and move pen position
    if ( use_kerning && previous_index && glyph_index )
    {
      FT_Vector  delta;
                                
      error = FT_Get_Kerning( face, previous_index, glyph_index,
			      ft_kerning_default, &delta );
      if (error) {
	LC_DEBUG (fprintf(stderr, "FT_Get_Kerning error %d\n", error));
      } else {
	pen_x += delta.x >> 6;
      }
    }
                            
    // now, draw to our target surface
    (*func)(face->glyph, 
	    pen_x+face->glyph->bitmap_left,
	    pen_y-face->glyph->bitmap_top,
	    userdata);
    
    // increment pen position 
    pen_x += face->glyph->advance.x >> 6;
    pen_y += face->glyph->advance.y >> 6;   // unuseful for now..


    // record current glyph index for kerning
    previous_index = glyph_index;
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

#ifdef HAVE_FREETYPE
    font->public.family = fonts;
#endif
    
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

  LC_DEBUG (fprintf(stderr, "font_getfont_with_style %s %s\n", name, style));
  g_assert(name!=NULL);
  fonts = (DiaFontFamily *)g_hash_table_lookup(fonts_hash, name);

  /* If one of the old (hardcoded) fonts, try the list */
  if (fonts == NULL) {
    int i;
    for (i = 0; i < sizeof(font_data)/sizeof(*font_data); i++) {
      if (!strcmp(font_data[i].fontname, name)) {
	// Found old-style font
	if (font_data[i].fontname_freetype == NULL) continue;
	fonts = (DiaFontFamily *)g_hash_table_lookup(fonts_hash, font_data[i].fontname_freetype);
	style = font_data[i].fontstyle_freetype;
	break;
      }
    }
  }

  if (fonts == NULL) {
    fonts = g_hash_table_lookup(fonts_hash, "Courier");
    if (fonts == NULL) {
      message_error(_("Error, couldn't locate font. Shouldn't happen.\n"));
    } else {
      message_notice(_("Font %s not found, using Courier instead.\n"), name);
    }
  }

  for (faces = fonts->diafonts; faces != NULL; faces = g_list_next(faces)) {
    if (!strcmp(((DiaFont *)faces->data)->style, style)) {
      return (DiaFont *)faces->data;
    }
  }

  message_notice(_("Font %s has no style %s, using %s\n"),
		 name, style,
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

  LC_DEBUG (fprintf(stderr, "font_getfont %s\n", name));
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

  LC_DEBUG (fprintf(stderr, "font_get_cache\n"));
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
  
  LC_DEBUG (fprintf(stderr, "font_get_gdkfont_helper\n"));
  bufsize = strlen(font->fontname_x11)+6;  /* Should be enought*/
  buffer = (char *)malloc(bufsize);
  g_snprintf(buffer, bufsize, font->fontname_x11, height);
  gdk_font = gdk_fontset_load (buffer);
  if (!gdk_font) gdk_font = gdk_font_load(buffer);
  free(buffer);

  return gdk_font;
}

GdkFont *
font_get_gdkfont(DiaFont *font, int height)
{
  FontCacheItem *cache_item;
  FontPrivate *fontprivate;

  LC_DEBUG (fprintf(stderr, "font_get_gdkfont\n"));
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
font_get_suckfont (GdkFont *font, utfchar *text)
{
	SuckFont *suckfont;
	int width, height, x, y;
	int lbearing, rbearing, ch_width, ascent, descent;
	GdkWChar *wcstr;
	gchar *mbstr, *str;
	int length, wclength, i;
	GdkPixmap *pixmap;
	GdkGC *gc;
	GdkColor black, white;
	GdkImage *image;
	int black_pixel, pixel;
	guchar *line;

	if (!font) return NULL;

#ifndef GTK_TALKS_UTF8
	str = charconv_utf8_to_local8 (text);
	length = strlen (str);
	mbstr = g_strdup (str);
	wcstr = g_new0 (GdkWChar, length + 1);
	wclength = mbstowcs (wcstr, mbstr, length);
	g_free (mbstr);
#else
	str = charconv_local8_to_utf8 (text);
	length = strlen (text);
	wcstr = g_new0 (GdkWChar, length + 1);
	wclength = gdk_mbstowcs (wcstr, str, length);
#endif

	if (wclength > 0) {
		length = wclength;
	} else {
		for (i = 0; i < length; i++) {
			wcstr[i] = (unsigned char) str[i];
		}
	}

	suckfont = g_new (SuckFont, 1);

	height = font->ascent + font->descent;
	x = 0;
	gdk_text_extents_wc (font, wcstr, length, &lbearing, &rbearing, &ch_width, &ascent, &descent);
	suckfont->chars[0].left_sb = lbearing;
	suckfont->chars[0].right_sb = ch_width - rbearing;
	suckfont->chars[0].width = rbearing - lbearing;
	suckfont->chars[0].ascent = ascent;
	suckfont->chars[0].descent = descent;
	suckfont->chars[0].bitmap_offset = x;

	if (str == NULL || str[0] == 0) {
		/* it's fake for avoiding many warnings */
		ch_width = 1;
	}
	x += (ch_width + 31) & -32;
	width = x;

	suckfont->bitmap_width = width;
	suckfont->bitmap_height = height + 1;
	suckfont->ascent = font->ascent + 1;

	pixmap = gdk_pixmap_new (NULL, suckfont->bitmap_width,
				 suckfont->bitmap_height, 1);
	gc = gdk_gc_new (pixmap);
	gdk_gc_set_font (gc, font);

	/* the black and white pixel values need to be taken from a 1 bit 
	 * colormap rather than the default colormap
	 */
	black_pixel = 0;
	black.pixel = black_pixel;
	white.pixel = 1;
	gdk_gc_set_foreground (gc, &white);
	gdk_draw_rectangle (pixmap, gc, 1, 0, 0, suckfont->bitmap_width, suckfont->bitmap_height);

	gdk_gc_set_foreground (gc, &black);

#ifndef GTK_TALKS_UTF8
	gdk_draw_string (pixmap, font, gc,
			 suckfont->chars[0].bitmap_offset - suckfont->chars[0].left_sb,
			 font->ascent + 1,
			 str);
#else
	gdk_draw_string (pixmap, font, gc,
			 suckfont->chars[0].bitmap_offset - suckfont->chars[0].left_sb,
			 font->ascent + 1,
			 str);
#endif
	g_free (str);

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

#ifdef HAVE_FREETYPE
FT_Face
font_get_freetype_face(DiaFont *font)
{
  FontPrivate *fontprivate;

  LC_DEBUG (fprintf(stderr, "font_get_psfontname\n"));
  g_assert(font!=NULL);

  fontprivate = (FontPrivate *)font;
  
  return fontprivate->fontface_freetype;
}
#endif

char *
font_get_psfontname(DiaFont *font)
{
  FontPrivate *fontprivate;

  LC_DEBUG (fprintf(stderr, "font_get_psfontname\n"));
  g_assert(font!=NULL);

  fontprivate = (FontPrivate *)font;
  
  return fontprivate->fontname_ps;
}

/* Returns the width in cm */
real
font_string_width(const char *string, DiaFont *font, real height)
{
#ifdef HAVE_FREETYPE
  FT_Face face;
  FreetypeString *ft_string;
  real height_ratio;

  /* This is currently broken */
  LC_DEBUG (fprintf(stderr, "font_string_width: %s %s %f\n", font->name, font->style, height));
  face = font_get_freetypefont(font, height*72/2.54);
  ft_string = freetype_load_string(string, face, strlen(string));
  //  height_ratio = (face->height>>6)/height;
  height_ratio = 72/2.54;
  LC_DEBUG(fprintf(stderr, "result width is %f, height %f, face height %d, ratio %f, return %fcm\n", ft_string->width, height, (face->height>>6), height_ratio, ft_string->width/height_ratio));
  return ft_string->width/height_ratio;
#else
  GdkFont *gdk_font;
  GdkWChar *wcstr;
  double scaled_width;
  char *mbstr, *str;
  int length;
  int wclength, i;
  gint g_lbear, g_rbear, g_width, g_asc, g_desc, c_wide, c_tall;

  g_return_val_if_fail (string != NULL, 0.0);

  if (string[0] == 0) return 0.0;
#ifdef UNICODE_WORK_IN_PROGRESS
  str = charconv_utf8_to_local8 (string);
#elif !defined(GTK_TALKS_UTF8)
  str = g_strdup (string);
#else
  str = charconv_local8_to_utf8 (string);
#endif
  length = strlen (str);
  wcstr = g_new0 (GdkWChar, length);
#ifndef GTK_TALKS_UTF8
  mbstr = g_strdup (str);
  wclength = mbstowcs (wcstr, mbstr, length);
  g_free (mbstr);
#else
  wclength = gdk_mbstowcs (wcstr, str, length);
#endif

  if (wclength > 0) {
	  length = wclength;
  } else {
	  for (i = 0; i < length; i++) {
		  wcstr[i] = (unsigned char) str[i];
	  }
  }

  /* this gets the extents of the text written, and the ascent height of the 
     written font. it divides width by height to get a wide-to-high ratio, then
     multiplies by the scaled height.
   */
  gdk_font = font_get_gdkfont(font, 100);
  gdk_text_extents_wc(gdk_font, 
                      wcstr, 
                      length,
                      &g_lbear,
                      &g_rbear,
                      &g_width,
                      &g_asc,
                      &g_desc);
  c_wide = g_lbear + g_rbear;
  if (c_wide == 0) return (real)0.0;
  scaled_width = (double)c_wide / (double)g_asc * height;

  g_free (wcstr);
  g_free (str);

  return (real)scaled_width;
#endif
}

real
font_ascent(DiaFont *font, real height)
{
  FontPrivate *fontprivate;
  LC_DEBUG (fprintf(stderr, "font_ascent %f\n", height));

  fontprivate = (FontPrivate *)font;
  return height*fontprivate->ascent_ratio;
}

real
font_descent(DiaFont *font, real height)
{
  FontPrivate *fontprivate;

  LC_DEBUG (fprintf(stderr, "font_descent\n"));
  fontprivate = (FontPrivate *)font;
  return height*fontprivate->descent_ratio;
}

/* Routines for sucking fonts from the X server */

void
suck_font_free (SuckFont *suckfont)
{
	g_free (suckfont->bitmap);
	g_free (suckfont);
}

