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

#include "utils.h"
#include "color.h"
#include "message.h"
#include "intl.h"
#include "text.h"
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
};

struct _FontPrivate {
  DiaFont public;
  char *fontname_x11;
  char **fontname_x11_vec;
  char *fontname_ps;
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
#ifdef G_OS_WIN32
  "system" /* Must be last. This is guaranteed to exist on a MS-Windows 
              system. */
#else
  "-urw-courier-medium-r-normal-*-%d-*-*-*-*-*-*-*",
  "-*-courier-medium-r-normal-*-%d-*-*-*-*-*-*-*",
  "fixed" /* Must be last. This is guaranteed to exist on an X11 system. */
#endif
};
#define NUM_LAST_RESORT_FONTS (sizeof(last_resort_fonts)/sizeof(char *))

char *xfs_configs[]={
  "/etc/X11/fs/config",		/* RedHat */
  "/etc/X11/xfs/config",	/* Debian */
};
#define NUM_XFS_CONFIGS (sizeof(xfs_configs)/sizeof(char *))

/* Maximum number of fonts per line in xfs configuration file */
#define MAX_NUM_FONTS_PER_LINE	(20)

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

  if (gdk_font == NULL) {
      message_warning(_("Unable to load any GDK font for %s font; even "
                        "last resorts alternatives failed.\n"
                        "This is fatal, sorry."),
                      x11_font);
      g_assert_not_reached();
  }
  
  height = (real)gdk_font->ascent + gdk_font->descent;
  font->ascent_ratio = gdk_font->ascent/height;
  font->descent_ratio = gdk_font->descent/height;

  gdk_font_unref(gdk_font);
}


void
font_init(void)
{
  int i,j;
  FontPrivate *font;

  fonts_hash = g_hash_table_new((GHashFunc)g_str_hash,
			       (GCompareFunc)g_str_equal);

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
}


DiaFont *
font_getfont(const char *name)
{
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
font_get_suckfont (GdkFont *font, gchar *text)
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

	str = g_strdup (text);
	length = strlen (text);
	wcstr = g_new0 (GdkWChar, length + 1);
	wclength = gdk_mbstowcs (wcstr, str, length);

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
  if ((wcstr != NULL) && (length != 0) && (*wcstr != 0)) {
      gdk_text_extents_wc (font, wcstr, length,
                           &lbearing, &rbearing, &ch_width,
                           &ascent, &descent);
  } else {
      lbearing = 0;
      rbearing = 0;
      ch_width = 0;
      ascent = 0;
      descent = 0;
  }
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

	gdk_draw_string (pixmap, font, gc,
			 suckfont->chars[0].bitmap_offset - suckfont->chars[0].left_sb,
			 font->ascent + 1,
			 str);
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
/* Note:  This function cannot be perfect without knowing the
   renderers scaling.  This is because fonts don't scale linearly,
   but in jumps.  The height isn't enough to tell the scaling.
   This means, unfortunately, that setting any box width based on text
   width is almost guaranteed to give imperfect results.  We're trying
   to err on the side of getting everything inside.
*/

real
font_string_width(const char *string, DiaFont *font, real height)
{
  GdkFont *gdk_font;
  GdkWChar *wcstr;
  double scaled_width;
  char *mbstr, *str;
  int length;
  int wclength, i;
  gint g_lbear, g_rbear, g_width, g_asc, g_desc, c_wide, c_tall;

  g_return_val_if_fail (string != NULL, 0.0);

  if (string[0] == 0) return 0.0;
  str = g_strdup (string);
  length = strlen (str);
  wcstr = g_new0 (GdkWChar, length);
  wclength = gdk_mbstowcs (wcstr, str, length);

  if (wclength > 0) {
	  length = wclength;
  } else {
	  for (i = 0; i < length; i++) {
		  wcstr[i] = (unsigned char) str[i];
	  }
  }

  /* this gets the extents of the text written.
     it divides width by height to get a wide-to-high ratio, then
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
  c_wide = g_rbear - g_lbear;
  if (c_wide == 0) return (real)0.0;
  /* Have to put in a fudge factor to account for the non-linearity */
  scaled_width = 1.1*((double)c_wide) / ((double)100) * height;

  g_free (wcstr);
  g_free (str);

  return (real)scaled_width;
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

