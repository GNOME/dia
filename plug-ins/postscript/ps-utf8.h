/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * ps-utf8.h: builds custom Postscript encodings to write arbitrary utf8
 * strings and have the device understand what's going on.
 * Copyright (C) 2001 Cyrille Chepelov <chepelov@calixo.net>
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

#ifndef PS_UTF8_H
#define PS_UTF8_H

#include <config.h>
#include <glib.h>

#include "diatypes.h"

typedef struct _PSFontDescriptor PSFontDescriptor;
typedef struct _PSEncodingPage PSEncodingPage;
typedef struct _PSUnicoder PSUnicoder;
typedef struct _PSUnicoderCallbacks PSUnicoderCallbacks;

#define PSEPAGE_BEGIN 32
#define PSEPAGE_SIZE (256-PSEPAGE_BEGIN)

struct _PSFontDescriptor {
  const char *face; /* LE */
  char *name; /* LE */
  const PSEncodingPage *encoding; /* DNLE */
  int encoding_serial_num; /* if encoding->serial_num > this, we have to
                              re-emit the font. If it's negative,
                              the font has never been emitted. */
};

/* "defined_fonts" cache is simply a dictionary (g_hash_table):
       ("face-e%d-%f" % (encoding_page_#,size)) --> font descriptor

*/

struct _PSEncodingPage {
  char *name; /* LE. */
  int page_num;
  int serial_num;
  int last_realized;
  int entries;
  GHashTable *backpage; /* LE(unichar -> char) */
  gunichar page[PSEPAGE_SIZE];
};

struct _PSUnicoder {
  gpointer usrdata;
  const PSUnicoderCallbacks *callbacks;

  const char *face;
  float size;
  float current_size; /* might lag a little... */
  PSFontDescriptor *current_font; /* DNLE */

  GHashTable *defined_fonts;   /* LE(DNLE name -> LE PSFontDescriptor *fd) */
  GHashTable *unicode_to_page; /* LE(unicode -> DNLE PSEncodingPage *ep) */
  GSList *encoding_pages;      /* LE(LE PSEncodingPage) */
  PSEncodingPage *last_page;   /* DNLE */
  const PSEncodingPage *current_encoding; /* DNLE */
};

typedef void (*DestroyPSFontFunc)(gpointer usrdata, const char *fontname);
typedef void (*BuildPSEncodingFunc)(gpointer usrdata,
                                    const char *name,
                                    const gunichar table[PSEPAGE_SIZE]);
/* note: the first $PSEPAGE_BEGIN will always be /.notdef, likewise
   for ( and ) */
typedef void (*BuildPSFontFunc)(gpointer usrdata,
                                const char *name,
                                const char *face,
                                const char *encoding_name);
/* must definefont a font under "fontname", with the "encname" encoding
   (already defined). */
typedef void (*SelectPSFontFunc)(gpointer usrdata,
                                 const char *fontname,
                                 float size);
/* must select (setfont) an already found and defined font. */
typedef void (*ShowStringFunc)(gpointer usrdata, const char *string);
/* typically, does "(string) show" (but can do other things).
   string is guaranteed to not have escaping issues. */
typedef void (*GetStringWidthFunc)(gpointer usrdata, const char *string,
                               gboolean first);
/* gets the string width on the PS stack. if !first, adds it to the previous
   element of the stack. */

struct _PSUnicoderCallbacks {
  DestroyPSFontFunc destroy_ps_font;
  BuildPSEncodingFunc build_ps_encoding;
  BuildPSFontFunc build_ps_font;
  SelectPSFontFunc select_ps_font;
  ShowStringFunc show_string;
  GetStringWidthFunc get_string_width;
};


extern PSUnicoder *ps_unicoder_new(const PSUnicoderCallbacks *psucbk,
                                   gpointer usrdata);
extern void ps_unicoder_destroy(PSUnicoder *psu);

extern void psu_set_font_face(PSUnicoder *psu, const char *face, float size);
/* just stores the font face and size we'll later use */

extern void psu_check_string_encodings(PSUnicoder *psu,
                                       const char *utf8_string);
/* appends what's going to be needed in the encoding tables */

extern void psu_show_string(PSUnicoder *psu,
                           const char *utf8_string);
/* shows the string, asking back for as many font and encoding manipulations
   as necessary. */
extern void psu_get_string_width(PSUnicoder *psu,
                                 const char *utf8_string);
/* puts the string width on the PS stack. */

extern const char *unicode_to_ps_name(gunichar val);
/* returns "uni1234", or a special Adobe entity name. */


#endif /* !PS_UTF8_H */


