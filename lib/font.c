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
 *
 */


#include <stdio.h>
#include <stdlib.h>

#include "config.h"
#include "intl.h"
#include "utils.h"
#include "font.h"
#include "message.h"

#define FONTCACHE_SIZE 17

#define NUM_X11_FONTS 2

typedef struct _FontPrivate FontPrivate;
typedef struct  _FontCacheItem FontCacheItem;

struct _FontCacheItem {
  int height;
  GdkFont *gdk_font;
};

struct _FontPrivate {
  Font public;
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
  "fixed" /* Must be last. This is guaranteed to exist on an X11 system. */
};
#define NUM_LAST_RESORT_FONTS (sizeof(last_resort_fonts)/sizeof(char *))


static void
init_x11_font(FontPrivate *font)
{
  int i;
  GdkFont *gdk_font;
  int bufsize;
  char *buffer;
  char *x11_font;
  real height;
  
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


Font *
font_getfont(const char *name)
{
  FontPrivate *font;

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
  
  return (Font *)font;
}

GdkFont *
font_get_gdkfont(Font *font, int height)
{
  FontPrivate *fontprivate;
  GdkFont *gdk_font;
  int index;
  int bufsize;
  char *buffer;

  fontprivate = (FontPrivate *)font;

  if (height<=0)
    height = 1;
  
  index = height % FONTCACHE_SIZE;
  
  if (fontprivate->cache[index]==NULL) {
    fontprivate->cache[index] = g_new(FontCacheItem, 1);
  } else if (fontprivate->cache[index]->height == height) {
    return fontprivate->cache[index]->gdk_font;
  } else {
    gdk_font_unref(fontprivate->cache[index]->gdk_font);
  }
  
  /* Not in cache: */
  
  bufsize = strlen(fontprivate->fontname_x11)+6;  /* Should be enought*/
  buffer = (char *)malloc(bufsize);
  g_snprintf(buffer, bufsize, fontprivate->fontname_x11, height);
  gdk_font = gdk_font_load(buffer);
  free(buffer);
  
  fontprivate->cache[index]->height = height;
  fontprivate->cache[index]->gdk_font = gdk_font;

  return gdk_font;
}

char *
font_get_psfontname(Font *font)
{
  FontPrivate *fontprivate;

  fontprivate = (FontPrivate *)font;
  
  return fontprivate->fontname_ps;
}

real
font_string_width(const char *string, Font *font, real height)
{
  int iwidth, iheight;
  double width_height;
  GdkFont *gdk_font;

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
}

real
font_ascent(Font *font, real height)
{
  FontPrivate *fontprivate;
  fontprivate = (FontPrivate *)font;
  return height*fontprivate->ascent_ratio;
}

real
font_descent(Font *font, real height)
{
  FontPrivate *fontprivate;
  fontprivate = (FontPrivate *)font;
  return height*fontprivate->descent_ratio;
}

