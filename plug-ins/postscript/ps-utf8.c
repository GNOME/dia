/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * ps-utf8.c: builds custom Postscript encodings to write arbitrary utf8
 * strings and have the device understand what's going on.
 * Copyright (C) 2001 Cyrille Chepelov <chepelov@calixo.net>
 *
 * Portions derived from Gnome-Print/gp-ps-unicode.c, written by
 * Lauris Kaplinski <lauris@helixcode.com>,
 * Copyright (C) 1999-2000 Helix Code, Inc.
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
#include "ps-utf8.h"

#include <config.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

/* forward prototypes */
static gchar *make_font_descriptor_name(const gchar *face,
                                        const gchar *encoding);

static PSFontDescriptor *font_descriptor_new(const gchar *face,
                                             const PSEncodingPage *encoding,
                                             const gchar *name);

static void font_descriptor_destroy(PSFontDescriptor *fd);
static void use_font(PSUnicoder *psu, PSFontDescriptor *fd);
static PSEncodingPage *encoding_page_new(int num);
static void encoding_page_destroy(PSEncodingPage *ep);
static int encoding_page_add_unichar(PSEncodingPage *ep,
                                     gunichar uchar);

static void use_encoding(PSUnicoder *psu, PSEncodingPage *ep);

static void psu_add_encoding(PSUnicoder *psu, gunichar uchar);
static void psu_make_new_encoding_page(PSUnicoder *psu);
/* Unicoder functions */

extern PSUnicoder *ps_unicoder_new(const PSUnicoderCallbacks *psucbk,
                                   gpointer usrdata)
{
  PSUnicoder *psu = g_new0(PSUnicoder,1);
  psu->usrdata = usrdata;
  psu->callbacks = psucbk;

  psu->defined_fonts = g_hash_table_new(g_str_hash,g_str_equal);
  psu->unicode_to_page = g_hash_table_new(NULL,NULL);

  psu_make_new_encoding_page(psu);
  return psu;
}

static gboolean
ghrfunc_remove_font(gpointer key, gpointer value, gpointer user_data)
{
  font_descriptor_destroy((PSFontDescriptor *)value);
  return TRUE;
}

static void
gfunc_free_encoding_page(gpointer item, gpointer user_data)
{
  encoding_page_destroy((PSEncodingPage *)item);
}


extern void ps_unicoder_destroy(PSUnicoder *psu)
{
  g_hash_table_destroy(psu->unicode_to_page);
  g_hash_table_foreach_remove(psu->defined_fonts,
                              ghrfunc_remove_font,
                              NULL);
  g_hash_table_destroy(psu->defined_fonts);
  g_slist_foreach(psu->encoding_pages,
                  gfunc_free_encoding_page,
                  NULL);
  g_clear_pointer (&psu, g_free);
}

extern void
psu_set_font_face(PSUnicoder *psu, const gchar *face, float size)
{
  psu->face = face;
  psu->size = size;
  psu->current_font = NULL;
}

static void psu_make_new_encoding_page(PSUnicoder *psu)
{
  int num = 0;
  PSEncodingPage *ep;

  if (psu->last_page) num = psu->last_page->page_num + 1;
  ep = encoding_page_new(num);
  psu->last_page = ep;
  psu->encoding_pages = g_slist_append(psu->encoding_pages,ep);

  if (num == 1) {
    g_warning("You are going to use more than %d different characters; dia will begin to \n"
              "heavily use encoding switching. This feature has never been tested; \n"
              "please report success or crash to chepelov@calixo.net. Thank you very much.\n",
              PSEPAGE_SIZE);
  }
}

static void psu_add_encoding(PSUnicoder *psu, gunichar uchar)
{
  if (g_hash_table_lookup(psu->unicode_to_page,
                          GINT_TO_POINTER(uchar))) {
    return;
  }
  if (!encoding_page_add_unichar(psu->last_page,uchar)) {
    psu_make_new_encoding_page(psu);
    if (!encoding_page_add_unichar(psu->last_page,uchar))
      {
	g_assert_not_reached();
      }
  }
  g_hash_table_insert(psu->unicode_to_page,
                      GINT_TO_POINTER(uchar),
                      psu->last_page);
  if (psu->last_page == psu->current_encoding) {
    psu->current_encoding = NULL; /* so that it's re-emitted. */
    psu->current_font = NULL; /* anyway it will have to be re-emitted too. */
  }
}

extern void
psu_check_string_encodings(PSUnicoder *psu,
                           const char *utf8_string)
{
  gunichar uchar;
  const char *p = utf8_string;

  while (p && (*p)) {
    uchar = g_utf8_get_char(p);
    p = g_utf8_next_char(p);

    psu_add_encoding(psu,uchar);
    if ((uchar > 32) && (uchar < 2048)) {
      psu_add_encoding(psu,uchar);
    }
  }
}

#define BUFSIZE 256

typedef void (*FlushFunc)(const PSUnicoder *psu,
                          const gchar *buf,
                          gboolean first);


static void psu_show_flush_buffer(const PSUnicoder *psu,
                                  gchar buf[], int *bufu,
                                  FlushFunc flush,gboolean first)
{
  buf[*bufu] = 0;
  (*flush)(psu,buf,first);
  *bufu = 0;
}


static void
encoded_psu_show_string (PSUnicoder *psu,
                         const char *utf8_string,
                         FlushFunc   flushfunc)
{
  gunichar uchar;
  gchar c;
  gchar buf[BUFSIZE];
  int bufu = 0;
  gboolean first = TRUE;
  size_t len = 0;
  const char *p = utf8_string;

  while (p && (*p)) {
    uchar = g_utf8_get_char(p);
    p = g_utf8_next_char(p);
    len++;

    if (psu->current_encoding) {
      c =
        (gchar)GPOINTER_TO_INT(g_hash_table_lookup(psu->current_encoding->backpage,
                                                   GINT_TO_POINTER(uchar)));
    } else {
      c = 0;
    }

    if (!c) {
      /* No page or not the right page... switch ! */
      PSEncodingPage *ep = g_hash_table_lookup(psu->unicode_to_page,
                                               GINT_TO_POINTER(uchar));
      if (ep) {
        use_encoding(psu,ep);
        /* now psu->current_encoding == ep */
        c = (gchar)GPOINTER_TO_INT(g_hash_table_lookup(ep->backpage,
                                                       GINT_TO_POINTER(uchar)));
      } else {
        c = 31;
      }

      if ((!c)||(c==31)) {
        g_message("uchar %.4X has not been found in the encoding pages !",uchar);
        g_assert_not_reached();
      }
    }
    /* We now are in the right page, and c is the image of uchar with that
       encoding. */

    if ( ((!psu->current_font) ||
          (psu->current_font->encoding != psu->current_encoding))) {
      PSFontDescriptor *fd;
      char *font_name;
      if (bufu) {
        psu_show_flush_buffer(psu,buf,&bufu,flushfunc,first);
        first = FALSE;
      }

      font_name = make_font_descriptor_name (psu->face,
                                             psu->current_encoding->name);
      fd = g_hash_table_lookup (psu->defined_fonts, font_name);
      if (fd) {
        g_clear_pointer (&font_name, g_free);
        font_name = fd->name;
      } else {
        fd = font_descriptor_new (psu->face,
                                  psu->current_encoding,
                                  font_name);
        g_clear_pointer (&font_name, g_free);
        font_name = fd->name;
        g_hash_table_insert(psu->defined_fonts,(gpointer)font_name,fd);
      }
      use_font(psu,fd);
    }
    if (bufu >= BUFSIZE - 2) {
      psu_show_flush_buffer(psu,buf,&bufu,flushfunc,first);
      first = FALSE;
    }
    buf[bufu++] = c;
  }
  if ((bufu)||(!len)) /* even if !len, to avoid crashing the PS stack. */
    psu_show_flush_buffer(psu,buf,&bufu,flushfunc,first);
}

static void
symbol_psu_show_string(PSUnicoder *psu,
                        const char *utf8_string,
                        FlushFunc flushfunc)
{
  /* Special case with Symbol fonts: */
  gunichar uchar;
  gchar c;
  gchar buf[BUFSIZE];
  int bufu = 0;
  gboolean first = TRUE;
  size_t len = 0;
  const char *p = utf8_string;
  PSFontDescriptor *fd;
  const gchar *font_name = "Symbol";

  /* Locate and use the Symbol font (bare): */

  fd = g_hash_table_lookup(psu->defined_fonts,font_name);
  if (fd) {
    font_name = fd->name;
  } else {
    fd = font_descriptor_new(psu->face,
                             NULL,
                             font_name);
    font_name = fd->name;
    g_hash_table_insert(psu->defined_fonts,(gpointer)font_name,fd);
  }
  use_font(psu,fd);

  /* We have the Symbol font loaded. */

  while (p && (*p)) {
    uchar = g_utf8_get_char(p);
    p = g_utf8_next_char(p);
    len++;

    if (uchar < 256) c = uchar;
    else c = '?';

    switch (c) {
    case ')':
    case '(':
    case '\\':
      buf[bufu++] = '\\';
      buf[bufu++] = c;
      break;
    default:
      buf[bufu++] = c;
    }

    if (bufu >= BUFSIZE - 3) {
      psu_show_flush_buffer(psu,buf,&bufu,flushfunc,first);
      first = FALSE;
    }
  }

  if ((bufu)||(!len)) /* even if !len, to avoid crashing the PS stack. */
    psu_show_flush_buffer(psu,buf,&bufu,flushfunc,first);
}

static void
flush_show_string(const PSUnicoder *psu,
                  const gchar *buf,
                  gboolean first)
{
  psu->callbacks->show_string(psu->usrdata,buf);
}

extern void
psu_show_string(PSUnicoder *psu,
                const char *utf8_string)
{
  if (0==strcmp(psu->face,"Symbol")) {
    symbol_psu_show_string(psu,utf8_string,&flush_show_string);
  } else {
    encoded_psu_show_string(psu,utf8_string,&flush_show_string);
  }
}


static void
flush_get_string_width(const PSUnicoder *psu,
                            const char *buf,
                            gboolean first)
{
  psu->callbacks->get_string_width(psu->usrdata,buf,first);
}

extern void
psu_get_string_width(PSUnicoder *psu,
                     const char *utf8_string)
{
  if (0==strcmp(psu->face,"Symbol")) {
    symbol_psu_show_string(psu,utf8_string,&flush_get_string_width);
  } else {
    encoded_psu_show_string(psu,utf8_string,&flush_get_string_width);
  }
}


/* Encoding Page functions */

static PSEncodingPage *
encoding_page_new(int num)
{
  PSEncodingPage *ep = g_new0(PSEncodingPage,1);

  ep->name = g_strdup_printf("e%d",num);
  ep->page_num = 0;
  ep->serial_num = 0;
  ep->last_realized = -1;
  ep->entries = 0;
  ep->backpage = g_hash_table_new(NULL,NULL);
  return ep;
}


static void
encoding_page_destroy (PSEncodingPage *ep)
{
  g_clear_pointer (&ep->name, g_free);
  g_hash_table_destroy (ep->backpage);
  g_clear_pointer (&ep, g_free);
}


static int
encoding_page_add_unichar(PSEncodingPage *ep, gunichar uchar)
{
  int res;
  if (ep->entries >= PSEPAGE_SIZE) return 0; /* page is full. */

  while ((ep->entries == (')'-PSEPAGE_BEGIN)) ||
         (ep->entries == ('\\' - PSEPAGE_BEGIN)) ||
         (ep->entries == ('('-PSEPAGE_BEGIN))) ep->entries++;

  res = ep->entries++;

  ep->page[res] = uchar;

  res += PSEPAGE_BEGIN;
  g_hash_table_insert(ep->backpage,
                      GINT_TO_POINTER(uchar),GINT_TO_POINTER(res));
  ep->serial_num++;
  return res;

}

static void
use_encoding(PSUnicoder *psu, PSEncodingPage *ep)
{
  if (ep->serial_num != ep->last_realized) {
    psu->callbacks->build_ps_encoding(psu->usrdata,
                                      ep->name,
                                      ep->page);
    ep->last_realized = ep->serial_num;
  }
  psu->current_encoding = ep;
}

/* DiaFont descriptor functions */

static gchar *
make_font_descriptor_name(const gchar *face,
                          const gchar *encoding)
{
  return g_strdup_printf("%s_%s",face,encoding);
}

static PSFontDescriptor *
font_descriptor_new(const gchar *face,
                    const PSEncodingPage *encoding,
                    const gchar *name)
{
  PSFontDescriptor *fd = g_new(PSFontDescriptor,1);

  fd->face = face;
  fd->encoding = encoding;
  fd->encoding_serial_num = -1;

  if (name)
    fd->name = g_strdup(name);
  else
    fd->name = make_font_descriptor_name(face,encoding->name);
  return fd;
}


static void
font_descriptor_destroy (PSFontDescriptor *fd)
{
  g_clear_pointer (&fd->name, g_free);
  g_clear_pointer (&fd, g_free);
}


static void
use_font(PSUnicoder *psu, PSFontDescriptor *fd)
{
  if (psu->current_font == fd) return; /* Already done. */

  if (!fd->encoding) { /* bare font. No encoding. */
    psu->callbacks->select_ps_font(psu->usrdata,fd->name,psu->size);
  } else {
    gboolean undef,define,select;

    define = (fd->encoding->serial_num != fd->encoding_serial_num);
    undef = (define && (fd->encoding_serial_num <= 0));
    select = ((psu->current_font != fd) || (psu->current_size != psu->size));

    if (undef) psu->callbacks->destroy_ps_font(psu->usrdata,fd->name);
    if (define) psu->callbacks->build_ps_font(psu->usrdata,fd->name,
                                              fd->face, fd->encoding->name);
    fd->encoding_serial_num = fd->encoding->serial_num;
    if (select) psu->callbacks->select_ps_font(psu->usrdata,
                                               fd->name,
                                               psu->size);
  }

  psu->current_size = psu->size;
  psu->current_font = fd;
  psu->current_encoding = fd->encoding;
}


/* ------------------------------------------------------------------- */
/* Conversion from unicode char numbers (16-bit) to Adobe entity names */
/* The big table is derived from Gnome-Print's gp-ps-unicode.c         */
/* ------------------------------------------------------------------- */

typedef struct {
  gint unicode;
  gchar * name;
} GPPSUniTab;

static GPPSUniTab unitab[] = {
  { 0x0041, "A" },
  { 0x00C6, "AE" },
  { 0x01FC, "AEacute" },
  { 0xF7E6, "AEsmall" },
  { 0x00C1, "Aacute" },
  { 0xF7E1, "Aacutesmall" },
  { 0x0102, "Abreve" },
  { 0x00C2, "Acircumflex" },
  { 0xF7E2, "Acircumflexsmall" },
  { 0xF6C9, "Acute" },
  { 0xF7B4, "Acutesmall" },
  { 0x00C4, "Adieresis" },
  { 0xF7E4, "Adieresissmall" },
  { 0x00C0, "Agrave" },
  { 0xF7E0, "Agravesmall" },
  { 0x0391, "Alpha" },
  { 0x0386, "Alphatonos" },
  { 0x0100, "Amacron" },
  { 0x0104, "Aogonek" },
  { 0x00C5, "Aring" },
  { 0x01FA, "Aringacute" },
  { 0xF7E5, "Aringsmall" },
  { 0xF761, "Asmall" },
  { 0x00C3, "Atilde" },
  { 0xF7E3, "Atildesmall" },
  { 0x0042, "B" },
  { 0x0392, "Beta" },
  { 0xF6F4, "Brevesmall" },
  { 0xF762, "Bsmall" },
  { 0x0043, "C" },
  { 0x0106, "Cacute" },
  { 0xF6CA, "Caron" },
  { 0xF6F5, "Caronsmall" },
  { 0x010C, "Ccaron" },
  { 0x00C7, "Ccedilla" },
  { 0xF7E7, "Ccedillasmall" },
  { 0x0108, "Ccircumflex" },
  { 0x010A, "Cdotaccent" },
  { 0xF7B8, "Cedillasmall" },
  { 0x03A7, "Chi" },
  { 0xF6F6, "Circumflexsmall" },
  { 0xF763, "Csmall" },
  { 0x0044, "D" },
  { 0x010E, "Dcaron" },
  { 0x0110, "Dcroat" },
  { 0x2206, "Delta" },
  { 0x0394, "Delta" },
  { 0xF6CB, "Dieresis" },
  { 0xF6CC, "DieresisAcute" },
  { 0xF6CD, "DieresisGrave" },
  { 0xF7A8, "Dieresissmall" },
  { 0xF6F7, "Dotaccentsmall" },
  { 0xF764, "Dsmall" },
  { 0x0045, "E" },
  { 0x00C9, "Eacute" },
  { 0xF7E9, "Eacutesmall" },
  { 0x0114, "Ebreve" },
  { 0x011A, "Ecaron" },
  { 0x00CA, "Ecircumflex" },
  { 0xF7EA, "Ecircumflexsmall" },
  { 0x00CB, "Edieresis" },
  { 0xF7EB, "Edieresissmall" },
  { 0x0116, "Edotaccent" },
  { 0x00C8, "Egrave" },
  { 0xF7E8, "Egravesmall" },
  { 0x0112, "Emacron" },
  { 0x014A, "Eng" },
  { 0x0118, "Eogonek" },
  { 0x0395, "Epsilon" },
  { 0x0388, "Epsilontonos" },
  { 0xF765, "Esmall" },
  { 0x0397, "Eta" },
  { 0x0389, "Etatonos" },
  { 0x00D0, "Eth" },
  { 0xF7F0, "Ethsmall" },
  { 0x20AC, "Euro" },
  { 0x0046, "F" },
  { 0xF766, "Fsmall" },
  { 0x0047, "G" },
  { 0x0393, "Gamma" },
  { 0x011E, "Gbreve" },
  { 0x01E6, "Gcaron" },
  { 0x011C, "Gcircumflex" },
  { 0x0122, "Gcommaaccent" },
  { 0x0120, "Gdotaccent" },
  { 0xF6CE, "Grave" },
  { 0xF760, "Gravesmall" },
  { 0xF767, "Gsmall" },
  { 0x0048, "H" },
  { 0x25CF, "H18533" },
  { 0x25AA, "H18543" },
  { 0x25AB, "H18551" },
  { 0x25A1, "H22073" },
  { 0x0126, "Hbar" },
  { 0x0124, "Hcircumflex" },
  { 0xF768, "Hsmall" },
  { 0xF6CF, "Hungarumlaut" },
  { 0xF6F8, "Hungarumlautsmall" },
  { 0x0049, "I" },
  { 0x0132, "IJ" },
  { 0x00CD, "Iacute" },
  { 0xF7ED, "Iacutesmall" },

  { 0x012C, "Ibreve" },
  { 0x00CE, "Icircumflex" },
  { 0xF7EE, "Icircumflexsmall" },
  { 0x00CF, "Idieresis" },
  { 0xF7EF, "Idieresissmall" },
  { 0x0130, "Idotaccent" },
  { 0x2111, "Ifraktur" },
  { 0x00CC, "Igrave" },
  { 0xF7EC, "Igravesmall" },
  { 0x012A, "Imacron" },
  { 0x012E, "Iogonek" },
  { 0x0399, "Iota" },
  { 0x03AA, "Iotadieresis" },
  { 0x038A, "Iotatonos" },
  { 0xF769, "Ismall" },
  { 0x0128, "Itilde" },
  { 0x004A, "J" },
  { 0x0134, "Jcircumflex" },
  { 0xF76A, "Jsmall" },
  { 0x004B, "K" },
  { 0x039A, "Kappa" },
  { 0x0136, "Kcommaaccent" },
  { 0xF76B, "Ksmall" },
  { 0x004C, "L" },
  { 0xF6BF, "LL" },
  { 0x0139, "Lacute" },
  { 0x039B, "Lambda" },
  { 0x013D, "Lcaron" },
  { 0x013B, "Lcommaaccent" },
  { 0x013F, "Ldot" },
  { 0x0141, "Lslash" },
  { 0xF6F9, "Lslashsmall" },
  { 0xF76C, "Lsmall" },
  { 0x004D, "M" },
  { 0xF6D0, "Macron" },
  { 0xF7AF, "Macronsmall" },
  { 0xF76D, "Msmall" },
  { 0x039C, "Mu" },
  { 0x004E, "N" },
  { 0x0143, "Nacute" },
  { 0x0147, "Ncaron" },
  { 0x0145, "Ncommaaccent" },
  { 0xF76E, "Nsmall" },
  { 0x00D1, "Ntilde" },
  { 0xF7F1, "Ntildesmall" },
  { 0x039D, "Nu" },
  { 0x004F, "O" },
  { 0x0152, "OE" },
  { 0xF6FA, "OEsmall" },
  { 0x00D3, "Oacute" },
  { 0xF7F3, "Oacutesmall" },
  { 0x014E, "Obreve" },
  { 0x00D4, "Ocircumflex" },
  { 0xF7F4, "Ocircumflexsmall" },
  { 0x00D6, "Odieresis" },
  { 0xF7F6, "Odieresissmall" },
  { 0xF6FB, "Ogoneksmall" },
  { 0x00D2, "Ograve" },
  { 0xF7F2, "Ogravesmall" },
  { 0x01A0, "Ohorn" },
  { 0x0150, "Ohungarumlaut" },
  { 0x014C, "Omacron" },
  { 0x2126, "Omega" },
  { 0x03A9, "Omega" },
  { 0x038F, "Omegatonos" },
  { 0x039F, "Omicron" },
  { 0x038C, "Omicrontonos" },
  { 0x00D8, "Oslash" },
  { 0x01FE, "Oslashacute" },
  { 0xF7F8, "Oslashsmall" },
  { 0xF76F, "Osmall" },
  { 0x00D5, "Otilde" },
  { 0xF7F5, "Otildesmall" },
  { 0x0050, "P" },
  { 0x03A6, "Phi" },
  { 0x03A0, "Pi" },
  { 0x03A8, "Psi" },
  { 0xF770, "Psmall" },
  { 0x0051, "Q" },
  { 0xF771, "Qsmall" },
  { 0x0052, "R" },
  { 0x0154, "Racute" },
  { 0x0158, "Rcaron" },
  { 0x0156, "Rcommaaccent" },
  { 0x211C, "Rfraktur" },
  { 0x03A1, "Rho" },
  { 0xF6FC, "Ringsmall" },
  { 0xF772, "Rsmall" },
  { 0x0053, "S" },
  { 0x250C, "SF010000" },
  { 0x2514, "SF020000" },
  { 0x2510, "SF030000" },
  { 0x2518, "SF040000" },
  { 0x253C, "SF050000" },
  { 0x252C, "SF060000" },
  { 0x2534, "SF070000" },
  { 0x251C, "SF080000" },
  { 0x2524, "SF090000" },
  { 0x2500, "SF100000" },
  { 0x2502, "SF110000" },
  { 0x2561, "SF190000" },
  { 0x2562, "SF200000" },
  { 0x2556, "SF210000" },
  { 0x2555, "SF220000" },
  { 0x2563, "SF230000" },
  { 0x2551, "SF240000" },
  { 0x2557, "SF250000" },
  { 0x255D, "SF260000" },
  { 0x255C, "SF270000" },
  { 0x255B, "SF280000" },
  { 0x255E, "SF360000" },
  { 0x255F, "SF370000" },
  { 0x255A, "SF380000" },
  { 0x2554, "SF390000" },
  { 0x2569, "SF400000" },
  { 0x2566, "SF410000" },
  { 0x2560, "SF420000" },
  { 0x2550, "SF430000" },
  { 0x256C, "SF440000" },
  { 0x2567, "SF450000" },
  { 0x2568, "SF460000" },
  { 0x2564, "SF470000" },
  { 0x2565, "SF480000" },
  { 0x2559, "SF490000" },
  { 0x2558, "SF500000" },
  { 0x2552, "SF510000" },
  { 0x2553, "SF520000" },
  { 0x256B, "SF530000" },
  { 0x256A, "SF540000" },
  { 0x015A, "Sacute" },
  { 0x0160, "Scaron" },
  { 0xF6FD, "Scaronsmall" },
  { 0x015E, "Scedilla" },
  { 0xF6C1, "Scedilla" },
  { 0x015C, "Scircumflex" },
  { 0x0218, "Scommaaccent" },
  { 0x03A3, "Sigma" },
  { 0xF773, "Ssmall" },
  { 0x0054, "T" },
  { 0x03A4, "Tau" },
  { 0x0166, "Tbar" },
  { 0x0164, "Tcaron" },
  { 0x0162, "Tcommaaccent" },
  { 0x021A, "Tcommaaccent" },
  { 0x0398, "Theta" },
  { 0x00DE, "Thorn" },
  { 0xF7FE, "Thornsmall" },
  { 0xF6FE, "Tildesmall" },
  { 0xF774, "Tsmall" },
  { 0x0055, "U" },
  { 0x00DA, "Uacute" },
  { 0xF7FA, "Uacutesmall" },
  { 0x016C, "Ubreve" },
  { 0x00DB, "Ucircumflex" },
  { 0xF7FB, "Ucircumflexsmall" },
  { 0x00DC, "Udieresis" },
  { 0xF7FC, "Udieresissmall" },
  { 0x00D9, "Ugrave" },
  { 0xF7F9, "Ugravesmall" },
  { 0x01AF, "Uhorn" },
  { 0x0170, "Uhungarumlaut" },
  { 0x016A, "Umacron" },
  { 0x0172, "Uogonek" },
  { 0x03A5, "Upsilon" },
  { 0x03D2, "Upsilon1" },
  { 0x03AB, "Upsilondieresis" },
  { 0x038E, "Upsilontonos" },
  { 0x016E, "Uring" },
  { 0xF775, "Usmall" },
  { 0x0168, "Utilde" },
  { 0x0056, "V" },
  { 0xF776, "Vsmall" },
  { 0x0057, "W" },
  { 0x1E82, "Wacute" },
  { 0x0174, "Wcircumflex" },
  { 0x1E84, "Wdieresis" },
  { 0x1E80, "Wgrave" },
  { 0xF777, "Wsmall" },
  { 0x0058, "X" },
  { 0x039E, "Xi" },
  { 0xF778, "Xsmall" },
  { 0x0059, "Y" },
  { 0x00DD, "Yacute" },
  { 0xF7FD, "Yacutesmall" },
  { 0x0176, "Ycircumflex" },
  { 0x0178, "Ydieresis" },
  { 0xF7FF, "Ydieresissmall" },
  { 0x1EF2, "Ygrave" },
  { 0xF779, "Ysmall" },
  { 0x005A, "Z" },
  { 0x0179, "Zacute" },
  { 0x017D, "Zcaron" },
  { 0xF6FF, "Zcaronsmall" },
  { 0x017B, "Zdotaccent" },
  { 0x0396, "Zeta" },
  { 0xF77A, "Zsmall" },
  { 0x0061, "a" },
  { 0x00E1, "aacute" },
  { 0x0103, "abreve" },
  { 0x00E2, "acircumflex" },
  { 0x00B4, "acute" },
  { 0x0301, "acutecomb" },
  { 0x00E4, "adieresis" },
  { 0x00E6, "ae" },
  { 0x01FD, "aeacute" },
  { 0x2015, "afii00208" },
  { 0x0410, "afii10017" },
  { 0x0411, "afii10018" },
  { 0x0412, "afii10019" },
  { 0x0413, "afii10020" },
  { 0x0414, "afii10021" },
  { 0x0415, "afii10022" },
  { 0x0401, "afii10023" },
  { 0x0416, "afii10024" },
  { 0x0417, "afii10025" },
  { 0x0418, "afii10026" },
  { 0x0419, "afii10027" },
  { 0x041A, "afii10028" },
  { 0x041B, "afii10029" },
  { 0x041C, "afii10030" },
  { 0x041D, "afii10031" },
  { 0x041E, "afii10032" },
  { 0x041F, "afii10033" },
  { 0x0420, "afii10034" },
  { 0x0421, "afii10035" },
  { 0x0422, "afii10036" },
  { 0x0423, "afii10037" },
  { 0x0424, "afii10038" },
  { 0x0425, "afii10039" },
  { 0x0426, "afii10040" },
  { 0x0427, "afii10041" },
  { 0x0428, "afii10042" },
  { 0x0429, "afii10043" },
  { 0x042A, "afii10044" },
  { 0x042B, "afii10045" },
  { 0x042C, "afii10046" },
  { 0x042D, "afii10047" },
  { 0x042E, "afii10048" },
  { 0x042F, "afii10049" },
  { 0x0490, "afii10050" },
  { 0x0402, "afii10051" },
  { 0x0403, "afii10052" },
  { 0x0404, "afii10053" },
  { 0x0405, "afii10054" },
  { 0x0406, "afii10055" },
  { 0x0407, "afii10056" },
  { 0x0408, "afii10057" },
  { 0x0409, "afii10058" },
  { 0x040A, "afii10059" },
  { 0x040B, "afii10060" },
  { 0x040C, "afii10061" },
  { 0x040E, "afii10062" },
  { 0xF6C4, "afii10063" },
  { 0xF6C5, "afii10064" },
  { 0x0430, "afii10065" },
  { 0x0431, "afii10066" },
  { 0x0432, "afii10067" },
  { 0x0433, "afii10068" },
  { 0x0434, "afii10069" },
  { 0x0435, "afii10070" },
  { 0x0451, "afii10071" },
  { 0x0436, "afii10072" },
  { 0x0437, "afii10073" },
  { 0x0438, "afii10074" },
  { 0x0439, "afii10075" },
  { 0x043A, "afii10076" },
  { 0x043B, "afii10077" },
  { 0x043C, "afii10078" },
  { 0x043D, "afii10079" },
  { 0x043E, "afii10080" },
  { 0x043F, "afii10081" },
  { 0x0440, "afii10082" },
  { 0x0441, "afii10083" },
  { 0x0442, "afii10084" },
  { 0x0443, "afii10085" },
  { 0x0444, "afii10086" },
  { 0x0445, "afii10087" },
  { 0x0446, "afii10088" },
  { 0x0447, "afii10089" },
  { 0x0448, "afii10090" },
  { 0x0449, "afii10091" },
  { 0x044A, "afii10092" },
  { 0x044B, "afii10093" },
  { 0x044C, "afii10094" },
  { 0x044D, "afii10095" },
  { 0x044E, "afii10096" },
  { 0x044F, "afii10097" },
  { 0x0491, "afii10098" },
  { 0x0452, "afii10099" },
  { 0x0453, "afii10100" },
  { 0x0454, "afii10101" },
  { 0x0455, "afii10102" },
  { 0x0456, "afii10103" },
  { 0x0457, "afii10104" },
  { 0x0458, "afii10105" },
  { 0x0459, "afii10106" },
  { 0x045A, "afii10107" },
  { 0x045B, "afii10108" },
  { 0x045C, "afii10109" },
  { 0x045E, "afii10110" },
  { 0x040F, "afii10145" },
  { 0x0462, "afii10146" },
  { 0x0472, "afii10147" },
  { 0x0474, "afii10148" },
  { 0xF6C6, "afii10192" },
  { 0x045F, "afii10193" },
  { 0x0463, "afii10194" },
  { 0x0473, "afii10195" },
  { 0x0475, "afii10196" },
  { 0xF6C7, "afii10831" },
  { 0xF6C8, "afii10832" },
  { 0x04D9, "afii10846" },
  { 0x200E, "afii299" },
  { 0x200F, "afii300" },
  { 0x200D, "afii301" },
  { 0x066A, "afii57381" },
  { 0x060C, "afii57388" },
  { 0x0660, "afii57392" },
  { 0x0661, "afii57393" },
  { 0x0662, "afii57394" },
  { 0x0663, "afii57395" },
  { 0x0664, "afii57396" },
  { 0x0665, "afii57397" },
  { 0x0666, "afii57398" },
  { 0x0667, "afii57399" },
  { 0x0668, "afii57400" },
  { 0x0669, "afii57401" },
  { 0x061B, "afii57403" },
  { 0x061F, "afii57407" },
  { 0x0621, "afii57409" },
  { 0x0622, "afii57410" },
  { 0x0623, "afii57411" },
  { 0x0624, "afii57412" },
  { 0x0625, "afii57413" },
  { 0x0626, "afii57414" },
  { 0x0627, "afii57415" },
  { 0x0628, "afii57416" },
  { 0x0629, "afii57417" },
  { 0x062A, "afii57418" },
  { 0x062B, "afii57419" },
  { 0x062C, "afii57420" },
  { 0x062D, "afii57421" },
  { 0x062E, "afii57422" },
  { 0x062F, "afii57423" },
  { 0x0630, "afii57424" },
  { 0x0631, "afii57425" },
  { 0x0632, "afii57426" },
  { 0x0633, "afii57427" },
  { 0x0634, "afii57428" },
  { 0x0635, "afii57429" },
  { 0x0636, "afii57430" },
  { 0x0637, "afii57431" },
  { 0x0638, "afii57432" },
  { 0x0639, "afii57433" },
  { 0x063A, "afii57434" },
  { 0x0640, "afii57440" },
  { 0x0641, "afii57441" },
  { 0x0642, "afii57442" },
  { 0x0643, "afii57443" },
  { 0x0644, "afii57444" },
  { 0x0645, "afii57445" },
  { 0x0646, "afii57446" },
  { 0x0648, "afii57448" },
  { 0x0649, "afii57449" },
  { 0x064A, "afii57450" },
  { 0x064B, "afii57451" },
  { 0x064C, "afii57452" },
  { 0x064D, "afii57453" },
  { 0x064E, "afii57454" },
  { 0x064F, "afii57455" },
  { 0x0650, "afii57456" },
  { 0x0651, "afii57457" },
  { 0x0652, "afii57458" },
  { 0x0647, "afii57470" },
  { 0x06A4, "afii57505" },
  { 0x067E, "afii57506" },
  { 0x0686, "afii57507" },
  { 0x0698, "afii57508" },
  { 0x06AF, "afii57509" },
  { 0x0679, "afii57511" },
  { 0x0688, "afii57512" },
  { 0x0691, "afii57513" },
  { 0x06BA, "afii57514" },
  { 0x06D2, "afii57519" },
  { 0x06D5, "afii57534" },
  { 0x20AA, "afii57636" },
  { 0x05BE, "afii57645" },
  { 0x05C3, "afii57658" },
  { 0x05D0, "afii57664" },
  { 0x05D1, "afii57665" },
  { 0x05D2, "afii57666" },
  { 0x05D3, "afii57667" },
  { 0x05D4, "afii57668" },
  { 0x05D5, "afii57669" },
  { 0x05D6, "afii57670" },
  { 0x05D7, "afii57671" },
  { 0x05D8, "afii57672" },
  { 0x05D9, "afii57673" },
  { 0x05DA, "afii57674" },
  { 0x05DB, "afii57675" },
  { 0x05DC, "afii57676" },
  { 0x05DD, "afii57677" },
  { 0x05DE, "afii57678" },
  { 0x05DF, "afii57679" },
  { 0x05E0, "afii57680" },
  { 0x05E1, "afii57681" },
  { 0x05E2, "afii57682" },
  { 0x05E3, "afii57683" },
  { 0x05E4, "afii57684" },
  { 0x05E5, "afii57685" },
  { 0x05E6, "afii57686" },
  { 0x05E7, "afii57687" },
  { 0x05E8, "afii57688" },
  { 0x05E9, "afii57689" },
  { 0x05EA, "afii57690" },
  { 0xFB2A, "afii57694" },
  { 0xFB2B, "afii57695" },
  { 0xFB4B, "afii57700" },
  { 0xFB1F, "afii57705" },
  { 0x05F0, "afii57716" },
  { 0x05F1, "afii57717" },
  { 0x05F2, "afii57718" },
  { 0xFB35, "afii57723" },
  { 0x05B4, "afii57793" },
  { 0x05B5, "afii57794" },
  { 0x05B6, "afii57795" },
  { 0x05BB, "afii57796" },
  { 0x05B8, "afii57797" },
  { 0x05B7, "afii57798" },
  { 0x05B0, "afii57799" },
  { 0x05B2, "afii57800" },
  { 0x05B1, "afii57801" },
  { 0x05B3, "afii57802" },
  { 0x05C2, "afii57803" },
  { 0x05C1, "afii57804" },
  { 0x05B9, "afii57806" },
  { 0x05BC, "afii57807" },
  { 0x05BD, "afii57839" },
  { 0x05BF, "afii57841" },
  { 0x05C0, "afii57842" },
  { 0x02BC, "afii57929" },
  { 0x2105, "afii61248" },
  { 0x2113, "afii61289" },
  { 0x2116, "afii61352" },
  { 0x202C, "afii61573" },
  { 0x202D, "afii61574" },
  { 0x202E, "afii61575" },
  { 0x200C, "afii61664" },
  { 0x066D, "afii63167" },
  { 0x02BD, "afii64937" },
  { 0x00E0, "agrave" },
  { 0x2135, "aleph" },
  { 0x03B1, "alpha" },
  { 0x03AC, "alphatonos" },
  { 0x0101, "amacron" },
  { 0x0026, "ampersand" },
  { 0xF726, "ampersandsmall" },
  { 0x2220, "angle" },
  { 0x2329, "angleleft" },
  { 0x232A, "angleright" },
  { 0x0387, "anoteleia" },
  { 0x0105, "aogonek" },
  { 0x2248, "approxequal" },
  { 0x00E5, "aring" },
  { 0x01FB, "aringacute" },
  { 0x2194, "arrowboth" },
  { 0x21D4, "arrowdblboth" },
  { 0x21D3, "arrowdbldown" },
  { 0x21D0, "arrowdblleft" },
  { 0x21D2, "arrowdblright" },
  { 0x21D1, "arrowdblup" },
  { 0x2193, "arrowdown" },
  { 0xF8E7, "arrowhorizex" },
  { 0x2190, "arrowleft" },
  { 0x2192, "arrowright" },
  { 0x2191, "arrowup" },
  { 0x2195, "arrowupdn" },
  { 0x21A8, "arrowupdnbse" },
  { 0xF8E6, "arrowvertex" },
  { 0x005E, "asciicircum" },
  { 0x007E, "asciitilde" },
  { 0x002A, "asterisk" },
  { 0x2217, "asteriskmath" },
  { 0xF6E9, "asuperior" },
  { 0x0040, "at" },
  { 0x00E3, "atilde" },
  { 0x0062, "b" },
  { 0x005C, "backslash" },
  { 0x007C, "bar" },
  { 0x03B2, "beta" },
  { 0x2588, "block" },
  { 0xF8F4, "braceex" },
  { 0x007B, "braceleft" },
  { 0xF8F3, "braceleftbt" },
  { 0xF8F2, "braceleftmid" },
  { 0xF8F1, "bracelefttp" },
  { 0x007D, "braceright" },
  { 0xF8FE, "bracerightbt" },
  { 0xF8FD, "bracerightmid" },
  { 0xF8FC, "bracerighttp" },
  { 0x005B, "bracketleft" },
  { 0xF8F0, "bracketleftbt" },
  { 0xF8EF, "bracketleftex" },
  { 0xF8EE, "bracketlefttp" },
  { 0x005D, "bracketright" },
  { 0xF8FB, "bracketrightbt" },
  { 0xF8FA, "bracketrightex" },
  { 0xF8F9, "bracketrighttp" },
  { 0x02D8, "breve" },
  { 0x00A6, "brokenbar" },
  { 0xF6EA, "bsuperior" },
  { 0x2022, "bullet" },
  { 0x0063, "c" },
  { 0x0107, "cacute" },
  { 0x02C7, "caron" },
  { 0x21B5, "carriagereturn" },
  { 0x010D, "ccaron" },
  { 0x00E7, "ccedilla" },
  { 0x0109, "ccircumflex" },
  { 0x010B, "cdotaccent" },
  { 0x00B8, "cedilla" },
  { 0x00A2, "cent" },
  { 0xF6DF, "centinferior" },
  { 0xF7A2, "centoldstyle" },
  { 0xF6E0, "centsuperior" },
  { 0x03C7, "chi" },
  { 0x25CB, "circle" },
  { 0x2297, "circlemultiply" },
  { 0x2295, "circleplus" },
  { 0x02C6, "circumflex" },
  { 0x2663, "club" },
  { 0x003A, "colon" },
  { 0x20A1, "colonmonetary" },
  { 0x002C, "comma" },
  { 0xF6C3, "commaaccent" },
  { 0xF6E1, "commainferior" },
  { 0xF6E2, "commasuperior" },
  { 0x2245, "congruent" },
  { 0x00A9, "copyright" },
  { 0xF8E9, "copyrightsans" },
  { 0xF6D9, "copyrightserif" },
  { 0x00A4, "currency" },
  { 0xF6D1, "cyrBreve" },
  { 0xF6D2, "cyrFlex" },
  { 0xF6D4, "cyrbreve" },
  { 0xF6D5, "cyrflex" },
  { 0x0064, "d" },
  { 0x2020, "dagger" },
  { 0x2021, "daggerdbl" },
  { 0xF6D3, "dblGrave" },
  { 0xF6D6, "dblgrave" },
  { 0x010F, "dcaron" },
  { 0x0111, "dcroat" },
  { 0x00B0, "degree" },
  { 0x03B4, "delta" },
  { 0x2666, "diamond" },
  { 0x00A8, "dieresis" },
  { 0xF6D7, "dieresisacute" },
  { 0xF6D8, "dieresisgrave" },
  { 0x0385, "dieresistonos" },
  { 0x00F7, "divide" },
  { 0x2593, "dkshade" },
  { 0x2584, "dnblock" },
  { 0x0024, "dollar" },
  { 0xF6E3, "dollarinferior" },
  { 0xF724, "dollaroldstyle" },
  { 0xF6E4, "dollarsuperior" },
  { 0x20AB, "dong" },
  { 0x02D9, "dotaccent" },
  { 0x0323, "dotbelowcomb" },
  { 0x0131, "dotlessi" },
  { 0xF6BE, "dotlessj" },
  { 0x22C5, "dotmath" },
  { 0xF6EB, "dsuperior" },
  { 0x0065, "e" },
  { 0x00E9, "eacute" },
  { 0x0115, "ebreve" },
  { 0x011B, "ecaron" },
  { 0x00EA, "ecircumflex" },
  { 0x00EB, "edieresis" },
  { 0x0117, "edotaccent" },
  { 0x00E8, "egrave" },
  { 0x0038, "eight" },
  { 0x2088, "eightinferior" },
  { 0xF738, "eightoldstyle" },
  { 0x2078, "eightsuperior" },
  { 0x2208, "element" },
  { 0x2026, "ellipsis" },
  { 0x0113, "emacron" },
  { 0x2014, "emdash" },
  { 0x2205, "emptyset" },
  { 0x2013, "endash" },
  { 0x014B, "eng" },
  { 0x0119, "eogonek" },
  { 0x03B5, "epsilon" },
  { 0x03AD, "epsilontonos" },
  { 0x003D, "equal" },
  { 0x2261, "equivalence" },
  { 0x212E, "estimated" },
  { 0xF6EC, "esuperior" },
  { 0x03B7, "eta" },
  { 0x03AE, "etatonos" },
  { 0x00F0, "eth" },
  { 0x0021, "exclam" },
  { 0x203C, "exclamdbl" },
  { 0x00A1, "exclamdown" },
  { 0xF7A1, "exclamdownsmall" },
  { 0xF721, "exclamsmall" },
  { 0x2203, "existential" },
  { 0x0066, "f" },
  { 0x2640, "female" },
  { 0xFB00, "ff" },
  { 0xFB03, "ffi" },
  { 0xFB04, "ffl" },
  { 0xFB01, "fi" },
  { 0x2012, "figuredash" },
  { 0x25A0, "filledbox" },
  { 0x25AC, "filledrect" },
  { 0x0035, "five" },
  { 0x215D, "fiveeighths" },
  { 0x2085, "fiveinferior" },
  { 0xF735, "fiveoldstyle" },
  { 0x2075, "fivesuperior" },
  { 0xFB02, "fl" },
  { 0x0192, "florin" },
  { 0x0034, "four" },
  { 0x2084, "fourinferior" },
  { 0xF734, "fouroldstyle" },
  { 0x2074, "foursuperior" },
  { 0x2044, "fraction" },
  { 0x2215, "fraction" },
  { 0x20A3, "franc" },
  { 0x0067, "g" },
  { 0x03B3, "gamma" },
  { 0x011F, "gbreve" },
  { 0x01E7, "gcaron" },
  { 0x011D, "gcircumflex" },
  { 0x0123, "gcommaaccent" },
  { 0x0121, "gdotaccent" },
  { 0x00DF, "germandbls" },
  { 0x2207, "gradient" },
  { 0x0060, "grave" },
  { 0x0300, "gravecomb" },
  { 0x003E, "greater" },
  { 0x2265, "greaterequal" },
  { 0x00AB, "guillemotleft" },
  { 0x00BB, "guillemotright" },
  { 0x2039, "guilsinglleft" },
  { 0x203A, "guilsinglright" },
  { 0x0068, "h" },
  { 0x0127, "hbar" },
  { 0x0125, "hcircumflex" },
  { 0x2665, "heart" },
  { 0x0309, "hookabovecomb" },
  { 0x2302, "house" },
  { 0x02DD, "hungarumlaut" },
  { 0x002D, "hyphen" },
  { 0x00AD, "hyphen" },
  { 0xF6E5, "hypheninferior" },
  { 0xF6E6, "hyphensuperior" },
  { 0x0069, "i" },
  { 0x00ED, "iacute" },
  { 0x012D, "ibreve" },
  { 0x00EE, "icircumflex" },
  { 0x00EF, "idieresis" },
  { 0x00EC, "igrave" },
  { 0x0133, "ij" },
  { 0x012B, "imacron" },
  { 0x221E, "infinity" },
  { 0x222B, "integral" },
  { 0x2321, "integralbt" },
  { 0xF8F5, "integralex" },
  { 0x2320, "integraltp" },
  { 0x2229, "intersection" },
  { 0x25D8, "invbullet" },
  { 0x25D9, "invcircle" },
  { 0x263B, "invsmileface" },
  { 0x012F, "iogonek" },
  { 0x03B9, "iota" },
  { 0x03CA, "iotadieresis" },
  { 0x0390, "iotadieresistonos" },
  { 0x03AF, "iotatonos" },
  { 0xF6ED, "isuperior" },
  { 0x0129, "itilde" },
  { 0x006A, "j" },
  { 0x0135, "jcircumflex" },
  { 0x006B, "k" },
  { 0x03BA, "kappa" },
  { 0x0137, "kcommaaccent" },
  { 0x0138, "kgreenlandic" },
  { 0x006C, "l" },
  { 0x013A, "lacute" },
  { 0x03BB, "lambda" },
  { 0x013E, "lcaron" },
  { 0x013C, "lcommaaccent" },
  { 0x0140, "ldot" },
  { 0x003C, "less" },
  { 0x2264, "lessequal" },
  { 0x258C, "lfblock" },
  { 0x20A4, "lira" },
  { 0xF6C0, "ll" },
  { 0x2227, "logicaland" },
  { 0x00AC, "logicalnot" },
  { 0x2228, "logicalor" },
  { 0x017F, "longs" },
  { 0x25CA, "lozenge" },
  { 0x0142, "lslash" },
  { 0xF6EE, "lsuperior" },
  { 0x2591, "ltshade" },
  { 0x006D, "m" },
  { 0x00AF, "macron" },
  { 0x02C9, "macron" },
  { 0x2642, "male" },
  { 0x2212, "minus" },
  { 0x2032, "minute" },
  { 0xF6EF, "msuperior" },
  { 0x00B5, "mu" },
  { 0x03BC, "mu" },
  { 0x00D7, "multiply" },
  { 0x266A, "musicalnote" },
  { 0x266B, "musicalnotedbl" },
  { 0x006E, "n" },
  { 0x0144, "nacute" },
  { 0x0149, "napostrophe" },
  { 0x0148, "ncaron" },
  { 0x0146, "ncommaaccent" },
  { 0x0039, "nine" },
  { 0x2089, "nineinferior" },
  { 0xF739, "nineoldstyle" },
  { 0x2079, "ninesuperior" },
  { 0x2209, "notelement" },
  { 0x2260, "notequal" },
  { 0x2284, "notsubset" },
  { 0x207F, "nsuperior" },
  { 0x00F1, "ntilde" },
  { 0x03BD, "nu" },
  { 0x0023, "numbersign" },
  { 0x006F, "o" },
  { 0x00F3, "oacute" },
  { 0x014F, "obreve" },
  { 0x00F4, "ocircumflex" },
  { 0x00F6, "odieresis" },
  { 0x0153, "oe" },
  { 0x02DB, "ogonek" },
  { 0x00F2, "ograve" },
  { 0x01A1, "ohorn" },
  { 0x0151, "ohungarumlaut" },
  { 0x014D, "omacron" },
  { 0x03C9, "omega" },
  { 0x03D6, "omega1" },
  { 0x03CE, "omegatonos" },
  { 0x03BF, "omicron" },
  { 0x03CC, "omicrontonos" },
  { 0x0031, "one" },
  { 0x2024, "onedotenleader" },
  { 0x215B, "oneeighth" },
  { 0xF6DC, "onefitted" },
  { 0x00BD, "onehalf" },
  { 0x2081, "oneinferior" },
  { 0xF731, "oneoldstyle" },
  { 0x00BC, "onequarter" },
  { 0x00B9, "onesuperior" },
  { 0x2153, "onethird" },
  { 0x25E6, "openbullet" },
  { 0x00AA, "ordfeminine" },
  { 0x00BA, "ordmasculine" },
  { 0x221F, "orthogonal" },
  { 0x00F8, "oslash" },
  { 0x01FF, "oslashacute" },
  { 0xF6F0, "osuperior" },
  { 0x00F5, "otilde" },
  { 0x0070, "p" },
  { 0x00B6, "paragraph" },
  { 0x0028, "parenleft" },
  { 0xF8ED, "parenleftbt" },
  { 0xF8EC, "parenleftex" },
  { 0x208D, "parenleftinferior" },
  { 0x207D, "parenleftsuperior" },
  { 0xF8EB, "parenlefttp" },
  { 0x0029, "parenright" },
  { 0xF8F8, "parenrightbt" },
  { 0xF8F7, "parenrightex" },
  { 0x208E, "parenrightinferior" },
  { 0x207E, "parenrightsuperior" },
  { 0xF8F6, "parenrighttp" },
  { 0x2202, "partialdiff" },
  { 0x0025, "percent" },
  { 0x002E, "period" },
  { 0x00B7, "periodcentered" },
  { 0x2219, "periodcentered" },
  { 0xF6E7, "periodinferior" },
  { 0xF6E8, "periodsuperior" },
  { 0x22A5, "perpendicular" },
  { 0x2030, "perthousand" },
  { 0x20A7, "peseta" },
  { 0x03C6, "phi" },
  { 0x03D5, "phi1" },
  { 0x03C0, "pi" },
  { 0x002B, "plus" },
  { 0x00B1, "plusminus" },
  { 0x211E, "prescription" },
  { 0x220F, "product" },
  { 0x2282, "propersubset" },
  { 0x2283, "propersuperset" },
  { 0x221D, "proportional" },
  { 0x03C8, "psi" },
  { 0x0071, "q" },
  { 0x003F, "question" },
  { 0x00BF, "questiondown" },
  { 0xF7BF, "questiondownsmall" },
  { 0xF73F, "questionsmall" },
  { 0x0022, "quotedbl" },
  { 0x201E, "quotedblbase" },
  { 0x201C, "quotedblleft" },
  { 0x201D, "quotedblright" },
  { 0x2018, "quoteleft" },
  { 0x201B, "quotereversed" },
  { 0x2019, "quoteright" },
  { 0x201A, "quotesinglbase" },
  { 0x0027, "quotesingle" },
  { 0x0072, "r" },
  { 0x0155, "racute" },
  { 0x221A, "radical" },
  { 0xF8E5, "radicalex" },
  { 0x0159, "rcaron" },
  { 0x0157, "rcommaaccent" },
  { 0x2286, "reflexsubset" },
  { 0x2287, "reflexsuperset" },
  { 0x00AE, "registered" },
  { 0xF8E8, "registersans" },
  { 0xF6DA, "registerserif" },
  { 0x2310, "revlogicalnot" },
  { 0x03C1, "rho" },
  { 0x02DA, "ring" },
  { 0xF6F1, "rsuperior" },
  { 0x2590, "rtblock" },
  { 0xF6DD, "rupiah" },
  { 0x0073, "s" },
  { 0x015B, "sacute" },
  { 0x0161, "scaron" },
  { 0x015F, "scedilla" },
  { 0xF6C2, "scedilla" },
  { 0x015D, "scircumflex" },
  { 0x0219, "scommaaccent" },
  { 0x2033, "second" },
  { 0x00A7, "section" },
  { 0x003B, "semicolon" },
  { 0x0037, "seven" },
  { 0x215E, "seveneighths" },
  { 0x2087, "seveninferior" },
  { 0xF737, "sevenoldstyle" },
  { 0x2077, "sevensuperior" },
  { 0x2592, "shade" },
  { 0x03C3, "sigma" },
  { 0x03C2, "sigma1" },
  { 0x223C, "similar" },
  { 0x0036, "six" },
  { 0x2086, "sixinferior" },
  { 0xF736, "sixoldstyle" },
  { 0x2076, "sixsuperior" },
  { 0x002F, "slash" },
  { 0x263A, "smileface" },
  { 0x0020, "space" },
  { 0x00A0, "space" },
  { 0x2660, "spade" },
  { 0xF6F2, "ssuperior" },
  { 0x00A3, "sterling" },
  { 0x220B, "suchthat" },
  { 0x2211, "summation" },
  { 0x263C, "sun" },
  { 0x0074, "t" },
  { 0x03C4, "tau" },
  { 0x0167, "tbar" },
  { 0x0165, "tcaron" },
  { 0x0163, "tcommaaccent" },
  { 0x021B, "tcommaaccent" },
  { 0x2234, "therefore" },
  { 0x03B8, "theta" },
  { 0x03D1, "theta1" },
  { 0x00FE, "thorn" },
  { 0x0033, "three" },
  { 0x215C, "threeeighths" },
  { 0x2083, "threeinferior" },
  { 0xF733, "threeoldstyle" },
  { 0x00BE, "threequarters" },
  { 0xF6DE, "threequartersemdash" },
  { 0x00B3, "threesuperior" },
  { 0x02DC, "tilde" },
  { 0x0303, "tildecomb" },
  { 0x0384, "tonos" },
  { 0x2122, "trademark" },
  { 0xF8EA, "trademarksans" },
  { 0xF6DB, "trademarkserif" },
  { 0x25BC, "triagdn" },
  { 0x25C4, "triaglf" },
  { 0x25BA, "triagrt" },
  { 0x25B2, "triagup" },
  { 0xF6F3, "tsuperior" },
  { 0x0032, "two" },
  { 0x2025, "twodotenleader" },
  { 0x2082, "twoinferior" },
  { 0xF732, "twooldstyle" },
  { 0x00B2, "twosuperior" },
  { 0x2154, "twothirds" },
  { 0x0075, "u" },
  { 0x00FA, "uacute" },
  { 0x016D, "ubreve" },
  { 0x00FB, "ucircumflex" },
  { 0x00FC, "udieresis" },
  { 0x00F9, "ugrave" },
  { 0x01B0, "uhorn" },
  { 0x0171, "uhungarumlaut" },
  { 0x016B, "umacron" },
  { 0x005F, "underscore" },
  { 0x2017, "underscoredbl" },
  { 0x222A, "union" },
  { 0x2200, "universal" },
  { 0x0173, "uogonek" },
  { 0x2580, "upblock" },
  { 0x03C5, "upsilon" },
  { 0x03CB, "upsilondieresis" },
  { 0x03B0, "upsilondieresistonos" },
  { 0x03CD, "upsilontonos" },
  { 0x016F, "uring" },
  { 0x0169, "utilde" },
  { 0x0076, "v" },
  { 0x0077, "w" },
  { 0x1E83, "wacute" },
  { 0x0175, "wcircumflex" },
  { 0x1E85, "wdieresis" },
  { 0x2118, "weierstrass" },
  { 0x1E81, "wgrave" },
  { 0x0078, "x" },
  { 0x03BE, "xi" },
  { 0x0079, "y" },
  { 0x00FD, "yacute" },
  { 0x0177, "ycircumflex" },
  { 0x00FF, "ydieresis" },
  { 0x00A5, "yen" },
  { 0x1EF3, "ygrave" },
  { 0x007A, "z" },
  { 0x017A, "zacute" },
  { 0x017E, "zcaron" },
  { 0x017C, "zdotaccent" },
  { 0x0030, "zero" },
  { 0x2080, "zeroinferior" },
  { 0xF730, "zerooldstyle" },
  { 0x2070, "zerosuperior" },
  { 0x03B6, "zeta" }
};
#define unitabs (sizeof (unitab) / sizeof (unitab[0]))


/*
 * fixme: We should really use integer table for dingbats
 * (???)
 */

static GPPSUniTab dingtab[] = {
  { 0x0020, "space" }, /* SPACE */
  { 0x2701, "a1" }, /* UPPER BLADE SCISSORS */
  { 0x2702, "a2" }, /* BLACK SCISSORS */
  { 0x2703, "a202" }, /* LOWER BLADE SCISSORS */
  { 0x2704, "a3" }, /* WHITE SCISSORS */
  { 0x260E, "a4" }, /* BLACK TELEPHONE */
  { 0x2706, "a5" }, /* TELEPHONE LOCATION SIGN */
  { 0x2707, "a119" }, /* TAPE DRIVE */
  { 0x2708, "a118" }, /* AIRPLANE */
  { 0x2709, "a117" }, /* ENVELOPE */
  { 0x261B, "a11" }, /* BLACK RIGHT POINTING INDEX */
  { 0x261E, "a12" }, /* WHITE RIGHT POINTING INDEX */
  { 0x270C, "a13" }, /* VICTORY HAND */
  { 0x270D, "a14" }, /* WRITING HAND */
  { 0x270E, "a15" }, /* LOWER RIGHT PENCIL */
  { 0x270F, "a16" }, /* PENCIL */
  { 0x2710, "a105" }, /* UPPER RIGHT PENCIL */
  { 0x2711, "a17" }, /* WHITE NIB */
  { 0x2712, "a18" }, /* BLACK NIB */
  { 0x2713, "a19" }, /* CHECK MARK */
  { 0x2714, "a20" }, /* HEAVY CHECK MARK */
  { 0x2715, "a21" }, /* MULTIPLICATION X */
  { 0x2716, "a22" }, /* HEAVY MULTIPLICATION X */
  { 0x2717, "a23" }, /* BALLOT X */
  { 0x2718, "a24" }, /* HEAVY BALLOT X */
  { 0x2719, "a25" }, /* OUTLINED GREEK CROSS */
  { 0x271A, "a26" }, /* HEAVY GREEK CROSS */
  { 0x271B, "a27" }, /* OPEN CENTRE CROSS */
  { 0x271C, "a28" }, /* HEAVY OPEN CENTRE CROSS */
  { 0x271D, "a6" }, /* LATIN CROSS */
  { 0x271E, "a7" }, /* SHADOWED WHITE LATIN CROSS */
  { 0x271F, "a8" }, /* OUTLINED LATIN CROSS */
  { 0x2720, "a9" }, /* MALTESE CROSS */
  { 0x2721, "a10" }, /* STAR OF DAVID */
  { 0x2722, "a29" }, /* FOUR TEARDROP-SPOKED ASTERISK */
  { 0x2723, "a30" }, /* FOUR BALLOON-SPOKED ASTERISK */
  { 0x2724, "a31" }, /* HEAVY FOUR BALLOON-SPOKED ASTERISK */
  { 0x2725, "a32" }, /* FOUR CLUB-SPOKED ASTERISK */
  { 0x2726, "a33" }, /* BLACK FOUR POINTED STAR */
  { 0x2727, "a34" }, /* WHITE FOUR POINTED STAR */
  { 0x2605, "a35" }, /* BLACK STAR */
  { 0x2729, "a36" }, /* STRESS OUTLINED WHITE STAR */
  { 0x272A, "a37" }, /* CIRCLED WHITE STAR */
  { 0x272B, "a38" }, /* OPEN CENTRE BLACK STAR */
  { 0x272C, "a39" }, /* BLACK CENTRE WHITE STAR */
  { 0x272D, "a40" }, /* OUTLINED BLACK STAR */
  { 0x272E, "a41" }, /* HEAVY OUTLINED BLACK STAR */
  { 0x272F, "a42" }, /* PINWHEEL STAR */
  { 0x2730, "a43" }, /* SHADOWED WHITE STAR */
  { 0x2731, "a44" }, /* HEAVY ASTERISK */
  { 0x2732, "a45" }, /* OPEN CENTRE ASTERISK */
  { 0x2733, "a46" }, /* EIGHT SPOKED ASTERISK */
  { 0x2734, "a47" }, /* EIGHT POINTED BLACK STAR */
  { 0x2735, "a48" }, /* EIGHT POINTED PINWHEEL STAR */
  { 0x2736, "a49" }, /* SIX POINTED BLACK STAR */
  { 0x2737, "a50" }, /* EIGHT POINTED RECTILINEAR BLACK STAR */
  { 0x2738, "a51" }, /* HEAVY EIGHT POINTED RECTILINEAR BLACK STAR */
  { 0x2739, "a52" }, /* TWELVE POINTED BLACK STAR */
  { 0x273A, "a53" }, /* SIXTEEN POINTED ASTERISK */
  { 0x273B, "a54" }, /* TEARDROP-SPOKED ASTERISK */
  { 0x273C, "a55" }, /* OPEN CENTRE TEARDROP-SPOKED ASTERISK */
  { 0x273D, "a56" }, /* HEAVY TEARDROP-SPOKED ASTERISK */
  { 0x273E, "a57" }, /* SIX PETALLED BLACK AND WHITE FLORETTE */
  { 0x273F, "a58" }, /* BLACK FLORETTE */
  { 0x2740, "a59" }, /* WHITE FLORETTE */
  { 0x2741, "a60" }, /* EIGHT PETALLED OUTLINED BLACK FLORETTE */
  { 0x2742, "a61" }, /* CIRCLED OPEN CENTRE EIGHT POINTED STAR */
  { 0x2743, "a62" }, /* HEAVY TEARDROP-SPOKED PINWHEEL ASTERISK */
  { 0x2744, "a63" }, /* SNOWFLAKE */
  { 0x2745, "a64" }, /* TIGHT TRIFOLIATE SNOWFLAKE */
  { 0x2746, "a65" }, /* HEAVY CHEVRON SNOWFLAKE */
  { 0x2747, "a66" }, /* SPARKLE */
  { 0x2748, "a67" }, /* HEAVY SPARKLE */
  { 0x2749, "a68" }, /* BALLOON-SPOKED ASTERISK */
  { 0x274A, "a69" }, /* EIGHT TEARDROP-SPOKED PROPELLER ASTERISK */
  { 0x274B, "a70" }, /* HEAVY EIGHT TEARDROP-SPOKED PROPELLER ASTERISK */
  { 0x25CF, "a71" }, /* BLACK CIRCLE */
  { 0x274D, "a72" }, /* SHADOWED WHITE CIRCLE */
  { 0x25A0, "a73" }, /* BLACK SQUARE */
  { 0x274F, "a74" }, /* LOWER RIGHT DROP-SHADOWED WHITE SQUARE */
  { 0x2750, "a203" }, /* UPPER RIGHT DROP-SHADOWED WHITE SQUARE */
  { 0x2751, "a75" }, /* LOWER RIGHT SHADOWED WHITE SQUARE */
  { 0x2752, "a204" }, /* UPPER RIGHT SHADOWED WHITE SQUARE */
  { 0x25B2, "a76" }, /* BLACK UP-POINTING TRIANGLE */
  { 0x25BC, "a77" }, /* BLACK DOWN-POINTING TRIANGLE */
  { 0x25C6, "a78" }, /* BLACK DIAMOND */
  { 0x2756, "a79" }, /* BLACK DIAMOND MINUS WHITE X */
  { 0x25D7, "a81" }, /* RIGHT HALF BLACK CIRCLE */
  { 0x2758, "a82" }, /* LIGHT VERTICAL BAR */
  { 0x2759, "a83" }, /* MEDIUM VERTICAL BAR */
  { 0x275A, "a84" }, /* HEAVY VERTICAL BAR */
  { 0x275B, "a97" }, /* HEAVY SINGLE TURNED COMMA QUOTATION MARK ORNAMENT */
  { 0x275C, "a98" }, /* HEAVY SINGLE COMMA QUOTATION MARK ORNAMENT */
  { 0x275D, "a99" }, /* HEAVY DOUBLE TURNED COMMA QUOTATION MARK ORNAMENT */
  { 0x275E, "a100" }, /* HEAVY DOUBLE COMMA QUOTATION MARK ORNAMENT */
  { 0xF8D7, "a89" }, /* MEDIUM LEFT PARENTHESIS ORNAMENT */
  { 0xF8D8, "a90" }, /* MEDIUM RIGHT PARENTHESIS ORNAMENT */
  { 0xF8D9, "a93" }, /* MEDIUM FLATTENED LEFT PARENTHESIS ORNAMENT */
  { 0xF8DA, "a94" }, /* MEDIUM FLATTENED RIGHT PARENTHESIS ORNAMENT */
  { 0xF8DB, "a91" }, /* MEDIUM LEFT-POINTING ANGLE BRACKET ORNAMENT */
  { 0xF8DC, "a92" }, /* MEDIUM RIGHT-POINTING ANGLE BRACKET ORNAMENT */
  { 0xF8DD, "a205" }, /* HEAVY LEFT-POINTING ANGLE QUOTATION MARK ORNAMENT */
  { 0xF8DE, "a85" }, /* HEAVY RIGHT-POINTING ANGLE QUOTATION MARK ORNAMENT */
  { 0xF8DF, "a206" }, /* HEAVY LEFT-POINTING ANGLE BRACKET ORNAMENT */
  { 0xF8E0, "a86" }, /* HEAVY RIGHT-POINTING ANGLE BRACKET ORNAMENT */
  { 0xF8E1, "a87" }, /* LIGHT LEFT TORTOISE SHELL BRACKET ORNAMENT */
  { 0xF8E2, "a88" }, /* LIGHT RIGHT TORTOISE SHELL BRACKET ORNAMENT */
  { 0xF8E3, "a95" }, /* MEDIUM LEFT CURLY BRACKET ORNAMENT */
  { 0xF8E4, "a96" }, /* MEDIUM RIGHT CURLY BRACKET ORNAMENT */
  { 0x2761, "a101" }, /* CURVED STEM PARAGRAPH SIGN ORNAMENT */
  { 0x2762, "a102" }, /* HEAVY EXCLAMATION MARK ORNAMENT */
  { 0x2763, "a103" }, /* HEAVY HEART EXCLAMATION MARK ORNAMENT */
  { 0x2764, "a104" }, /* HEAVY BLACK HEART */
  { 0x2765, "a106" }, /* ROTATED HEAVY BLACK HEART BULLET */
  { 0x2766, "a107" }, /* FLORAL HEART */
  { 0x2767, "a108" }, /* ROTATED FLORAL HEART BULLET */
  { 0x2663, "a112" }, /* BLACK CLUB SUIT */
  { 0x2666, "a111" }, /* BLACK DIAMOND SUIT */
  { 0x2665, "a110" }, /* BLACK HEART SUIT */
  { 0x2660, "a109" }, /* BLACK SPADE SUIT */
  { 0x2460, "a120" }, /* CIRCLED DIGIT ONE */
  { 0x2461, "a121" }, /* CIRCLED DIGIT TWO */
  { 0x2462, "a122" }, /* CIRCLED DIGIT THREE */
  { 0x2463, "a123" }, /* CIRCLED DIGIT FOUR */
  { 0x2464, "a124" }, /* CIRCLED DIGIT FIVE */
  { 0x2465, "a125" }, /* CIRCLED DIGIT SIX */
  { 0x2466, "a126" }, /* CIRCLED DIGIT SEVEN */
  { 0x2467, "a127" }, /* CIRCLED DIGIT EIGHT */
  { 0x2468, "a128" }, /* CIRCLED DIGIT NINE */
  { 0x2469, "a129" }, /* CIRCLED NUMBER TEN */
  { 0x2776, "a130" }, /* DINGBAT NEGATIVE CIRCLED DIGIT ONE */
  { 0x2777, "a131" }, /* DINGBAT NEGATIVE CIRCLED DIGIT TWO */
  { 0x2778, "a132" }, /* DINGBAT NEGATIVE CIRCLED DIGIT THREE */
  { 0x2779, "a133" }, /* DINGBAT NEGATIVE CIRCLED DIGIT FOUR */
  { 0x277A, "a134" }, /* DINGBAT NEGATIVE CIRCLED DIGIT FIVE */
  { 0x277B, "a135" }, /* DINGBAT NEGATIVE CIRCLED DIGIT SIX */
  { 0x277C, "a136" }, /* DINGBAT NEGATIVE CIRCLED DIGIT SEVEN */
  { 0x277D, "a137" }, /* DINGBAT NEGATIVE CIRCLED DIGIT EIGHT */
  { 0x277E, "a138" }, /* DINGBAT NEGATIVE CIRCLED DIGIT NINE */
  { 0x277F, "a139" }, /* DINGBAT NEGATIVE CIRCLED NUMBER TEN */
  { 0x2780, "a140" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT ONE */
  { 0x2781, "a141" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT TWO */
  { 0x2782, "a142" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT THREE */
  { 0x2783, "a143" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT FOUR */
  { 0x2784, "a144" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT FIVE */
  { 0x2785, "a145" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT SIX */
  { 0x2786, "a146" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT SEVEN */
  { 0x2787, "a147" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT EIGHT */
  { 0x2788, "a148" }, /* DINGBAT CIRCLED SANS-SERIF DIGIT NINE */
  { 0x2789, "a149" }, /* DINGBAT CIRCLED SANS-SERIF NUMBER TEN */
  { 0x278A, "a150" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT ONE */
  { 0x278B, "a151" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT TWO */
  { 0x278C, "a152" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT THREE */
  { 0x278D, "a153" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FOUR */
  { 0x278E, "a154" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT FIVE */
  { 0x278F, "a155" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SIX */
  { 0x2790, "a156" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT SEVEN */
  { 0x2791, "a157" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT EIGHT */
  { 0x2792, "a158" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF DIGIT NINE */
  { 0x2793, "a159" }, /* DINGBAT NEGATIVE CIRCLED SANS-SERIF NUMBER TEN */
  { 0x2794, "a160" }, /* HEAVY WIDE-HEADED RIGHTWARDS ARROW */
  { 0x2192, "a161" }, /* RIGHTWARDS ARROW */
  { 0x2194, "a163" }, /* LEFT RIGHT ARROW */
  { 0x2195, "a164" }, /* UP DOWN ARROW */
  { 0x2798, "a196" }, /* HEAVY SOUTH EAST ARROW */
  { 0x2799, "a165" }, /* HEAVY RIGHTWARDS ARROW */
  { 0x279A, "a192" }, /* HEAVY NORTH EAST ARROW */
  { 0x279B, "a166" }, /* DRAFTING POINT RIGHTWARDS ARROW */
  { 0x279C, "a167" }, /* HEAVY ROUND-TIPPED RIGHTWARDS ARROW */
  { 0x279D, "a168" }, /* TRIANGLE-HEADED RIGHTWARDS ARROW */
  { 0x279E, "a169" }, /* HEAVY TRIANGLE-HEADED RIGHTWARDS ARROW */
  { 0x279F, "a170" }, /* DASHED TRIANGLE-HEADED RIGHTWARDS ARROW */
  { 0x27A0, "a171" }, /* HEAVY DASHED TRIANGLE-HEADED RIGHTWARDS ARROW */
  { 0x27A1, "a172" }, /* BLACK RIGHTWARDS ARROW */
  { 0x27A2, "a173" }, /* THREE-D TOP-LIGHTED RIGHTWARDS ARROWHEAD */
  { 0x27A3, "a162" }, /* THREE-D BOTTOM-LIGHTED RIGHTWARDS ARROWHEAD */
  { 0x27A4, "a174" }, /* BLACK RIGHTWARDS ARROWHEAD */
  { 0x27A5, "a175" }, /* HEAVY BLACK CURVED DOWNWARDS AND RIGHTWARDS ARROW */
  { 0x27A6, "a176" }, /* HEAVY BLACK CURVED UPWARDS AND RIGHTWARDS ARROW */
  { 0x27A7, "a177" }, /* SQUAT BLACK RIGHTWARDS ARROW */
  { 0x27A8, "a178" }, /* HEAVY CONCAVE-POINTED BLACK RIGHTWARDS ARROW */
  { 0x27A9, "a179" }, /* RIGHT-SHADED WHITE RIGHTWARDS ARROW */
  { 0x27AA, "a193" }, /* LEFT-SHADED WHITE RIGHTWARDS ARROW */
  { 0x27AB, "a180" }, /* BACK-TILTED SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27AC, "a199" }, /* FRONT-TILTED SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27AD, "a181" }, /* HEAVY LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27AE, "a200" }, /* HEAVY UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27AF, "a182" }, /* NOTCHED LOWER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27B1, "a201" }, /* NOTCHED UPPER RIGHT-SHADOWED WHITE RIGHTWARDS ARROW */
  { 0x27B2, "a183" }, /* CIRCLED HEAVY WHITE RIGHTWARDS ARROW */
  { 0x27B3, "a184" }, /* WHITE-FEATHERED RIGHTWARDS ARROW */
  { 0x27B4, "a197" }, /* BLACK-FEATHERED SOUTH EAST ARROW */
  { 0x27B6, "a194" }, /* BLACK-FEATHERED NORTH EAST ARROW */
  { 0x27B7, "a198" }, /* HEAVY BLACK-FEATHERED SOUTH EAST ARROW */
  { 0x27B8, "a186" }, /* HEAVY BLACK-FEATHERED RIGHTWARDS ARROW */
  { 0x27B9, "a195" }, /* HEAVY BLACK-FEATHERED NORTH EAST ARROW */
  { 0x27BA, "a187" }, /* TEARDROP-BARBED RIGHTWARDS ARROW */
  { 0x27BB, "a188" }, /* HEAVY TEARDROP-SHANKED RIGHTWARDS ARROW */
  { 0x27BC, "a189" }, /* WEDGE-TAILED RIGHTWARDS ARROW */
  { 0x27BD, "a190" }, /* HEAVY WEDGE-TAILED RIGHTWARDS ARROW */
  { 0x27BE, "a191" } /* OPEN-OUTLINED RIGHTWARDS ARROW */
};

#define dingtabs (sizeof (dingtab) / sizeof (dingtab[0]))


static GHashTable *uni2ps = NULL;
static void
new_uni_to_adobe_hash(void)
{
  int i;
  if (uni2ps) return;
  uni2ps = g_hash_table_new (NULL,NULL);
  for (i=0; i< unitabs; i++) {
    g_hash_table_insert(uni2ps,
                        GINT_TO_POINTER(unitab[i].unicode),
                        unitab[i].name);
  }
  for (i=0; i< dingtabs; i++) {
    g_hash_table_insert(uni2ps,
                        GINT_TO_POINTER(dingtab[i].unicode),
                        dingtab[i].name);
  }
}

extern const char *
unicode_to_ps_name (gunichar val)
{
	static GHashTable *std2ps = NULL;
	char *ps;

	if (!val) return "xi"; /* of course, you shouldn't see this everywhere */

	if (!uni2ps) new_uni_to_adobe_hash ();
	ps = g_hash_table_lookup (uni2ps, GINT_TO_POINTER (val));
	if (!ps) {
		if (!std2ps) std2ps = g_hash_table_new (NULL, NULL);
		ps = g_hash_table_lookup (std2ps, GINT_TO_POINTER (val));
		if (!ps) {
			ps = g_strdup_printf ("uni%.4X", val);
			g_hash_table_insert (uni2ps, GINT_TO_POINTER (val), ps);
		}
	}
	return ps;
}


