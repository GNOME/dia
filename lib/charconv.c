/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * charconv.c: simple wrapper around unicode_iconv until we get dia to use
 * utf8 internally.
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

#include <config.h>

#include <glib.h>
#include <gdk/gdkkeysyms.h>

#if GLIB_CHECK_VERSION (2,0,0) && defined (UNICODE_WORK_IN_PROGRESS)
#error "This file isn't needed/supported anymore"
#endif

#ifdef HAVE_UNICODE
#define UNICODE_WORK_IN_PROGRESS /* here, it's mandatory or we break. */
#else
/* we won't actually break, because the code which is here will dumb itself
down to nops and strdups */
#endif

#include "charconv.h" 

#include <string.h>
#ifdef HAVE_UNICODE
#include <unicode.h>
#include <errno.h>
#endif

/* unfortunately, unicode_get_charset() is broken :'-( 
   As a hopefully temporary measure, we'll continue using the older charset 
   detection code if unicode_get_charset() dares to tell us the local charset 
   is US-ASCII.
*/
#ifdef HAVE_LANGINFO_CODESET
#include <langinfo.h>
#else /* HAVE_LANGINFO_CODESET */
#ifndef EFAULT_8BIT_CHARSET /* This is a last resort hack */
#define EFAULT_8BIT_CHARSET "ISO-8859-1"
#endif /* !EFAULT_8BIT_CHARSET */
#define CODESET
static const char *nl_langinfo(void) {
  g_warning("Could not determine the local charset. Using %s by default.\n"
            "This failure may be caused by an incorrect compilation.",
            EFAULT_8BIT_CHARSET);
  return EFAULT_8BIT_CHARSET;
}
#endif /* HAVE_LANGINFO_CODESET */


int get_local_charset(char **charset) 
{
  static char *this_charset = NULL;
  static int local_is_utf8 = 0;

  if (this_charset) {
    *charset = this_charset;
    return local_is_utf8;
  }

#ifdef HAVE_UNICODE 
  local_is_utf8 = unicode_get_charset(charset);
#else
# if GLIB_CHECK_VERSION(1,3,0)
  local_is_utf8 = g_get_charset (charset);
# else
  *charset = NULL;
# endif
#endif
  if (local_is_utf8) {
      this_charset = *charset;
      return local_is_utf8;
  }
  if ((*charset == NULL ) || (0==strcmp(*charset,"US-ASCII")) ||
      (0==strcmp(*charset,"ANSI_X3.4-1968"))) {
          /* unicode_get_charset() is broken. Usually, when it says
             US-ASCII, it's something else.

          if it gave us no charset, we try again. */
      *charset = nl_langinfo(CODESET);

      if ((*charset == NULL) ||
          (0==strcmp(*charset,"US-ASCII")) ||
          (0==strcmp(*charset,"ANSI_X3.4-1968"))) {
              /* we got basic stupid ASCII here. We use its sane
                 superset instead. Especially since libxml2 doesn't like
                 the pedantic name of ASCII. */
          *charset = "UTF-8";
      }

  }

  this_charset = *charset;
  local_is_utf8 = (*charset) && (0==strcmp(*charset,"UTF-8"));
  
  return local_is_utf8;
}


#ifdef HAVE_UNICODE
static unicode_iconv_t conv_u2l = (unicode_iconv_t)0;
static unicode_iconv_t conv_l2u = (unicode_iconv_t)0;
static int local_is_utf8 = 0;

static void
check_conv_l2u(void){
  char *charset = NULL;
  
  if (local_is_utf8 || (conv_l2u!=(unicode_iconv_t)0)) return;
  local_is_utf8 = get_local_charset(&charset);
  if (local_is_utf8) return;
  
  conv_l2u = unicode_iconv_open("UTF-8",charset);
}

static void 
check_conv_u2l(void){
  char *charset = NULL;
  
  if (local_is_utf8 || (conv_u2l!=(unicode_iconv_t)0)) return;
  local_is_utf8 = get_local_charset(&charset);
  if (local_is_utf8) return;

  conv_u2l = unicode_iconv_open(charset,"UTF-8");
}
  

extern utfchar *
charconv_local8_to_utf8(const gchar *local)
{
  const gchar *l = local;
  int lleft = strlen(local);
  utfchar *utf,*u,*ures;
  int uleft;
  int lost = 0;

  g_assert(local);
  if (!local) return NULL; /* GIGO */

  check_conv_l2u();
  if (local_is_utf8) return g_strdup(local);

  uleft = 6*lleft + 1; 
  u = utf = g_malloc(uleft);
  *u = 0;
  unicode_iconv(conv_l2u,NULL,NULL,NULL,NULL); /* reset the state machines */
  while ((uleft) && (lleft)) {
    ssize_t res = unicode_iconv(conv_l2u,
                                &l,&lleft,
                                &u,&uleft);
    *u = 0;
    if (res==(ssize_t)-1) {
      g_warning("unicode_iconv(l2u, loc=%s, ...) failed, because '%s'",
                local,strerror(errno));
      break;
    } else {
      lost += (int)res; /* lost chars in the process. */
    }
  }
  ures = g_strdup(utf); /* get the actual size. */
  g_free(utf); 
  return ures;
}

extern gchar *
charconv_utf8_to_local8(const utfchar *utf)
{
  const utfchar *u = utf;
  int uleft = strlen(utf);
  gchar *local,*l,*lres;
  int lleft;
  int lost = 0;

  g_assert(utf);
  if (!utf) return NULL; /* GIGO */

  check_conv_u2l();
  if (local_is_utf8) return g_strdup(utf);

  lleft = uleft +2;
  l = local = g_malloc(lleft+2);
  *l = 0;
  unicode_iconv(conv_u2l,NULL,NULL,NULL,NULL); /* reset the state machines */
  while ((uleft) && (lleft)) {
    ssize_t res = unicode_iconv(conv_u2l,
                                &u,&uleft,
                                &l,&lleft);
    *l = 0;
    if (res==(ssize_t)-1) {
      g_warning("unicode_iconv(u2l, utf=%s,....) failed, because '%s'",
                utf,strerror(errno));
      break;
    } else {
      lost += (int)res; /* lost chars in the process. */
    }
  }
  lres = g_strdup(local); /* get the actual size. */
  g_free(local); 
  return lres;
}

/* The string here is statically allocated and must NOT be g_free()'d.*/
extern utfchar *
charconv_unichar_to_utf8(guint uc)
{
  /* algorithm taken from Tom Tromey's libunicode utf8_write() routine
     Copyright (C) 1999 Tom Tromey. 
     License is LGPL v2 (or later) */
  int i,first;
  size_t len = 0;
  static char outbuf[7];
  unicode_char_t c = uc;
  
  if ((c < 0x80) && (c > 0)) {
      first = 0;
      len = 1;
  } else if (c < 0x800) {
      first = 0xc0;
      len = 2;
  } else if (c < 0x10000) {
    first = 0xe0;
    len = 3;
  } else if (c < 0x200000) {
    first = 0xf0;
    len = 4;
  } else if (c < 0x4000000) {
    first = 0xf8;
    len = 5;
  } else {
    first = 0xfc;
    len = 6;
  }
  
  for (i = len - 1; i > 0; --i) {
    outbuf[i] = (c & 0x3f) | 0x80;
    c >>= 6;
  }
  outbuf[0] = c | first;
  outbuf[len] = 0;
  
  return outbuf;
}

#define UTF8_COMPUTE(Char, Mask, Len)					      \
  if (Char < 128)							      \
    {									      \
      Len = 1;								      \
      Mask = 0x7f;							      \
    }									      \
  else if ((Char & 0xe0) == 0xc0)					      \
    {									      \
      Len = 2;								      \
      Mask = 0x1f;							      \
    }									      \
  else if ((Char & 0xf0) == 0xe0)					      \
    {									      \
      Len = 3;								      \
      Mask = 0x0f;							      \
    }									      \
  else if ((Char & 0xf8) == 0xf0)					      \
    {									      \
      Len = 4;								      \
      Mask = 0x07;							      \
    }									      \
  else if ((Char & 0xfc) == 0xf8)					      \
    {									      \
      Len = 5;								      \
      Mask = 0x03;							      \
    }									      \
  else if ((Char & 0xfe) == 0xfc)					      \
    {									      \
      Len = 6;								      \
      Mask = 0x01;							      \
    }									      \
  else									      \
    Len = -1;

#define UTF8_GET(Result, Chars, Count, Mask, Len)			      \
  (Result) = (Chars)[0] & (Mask);					      \
  for ((Count) = 1; (Count) < (Len); ++(Count))				      \
    {									      \
      if (((Chars)[(Count)] & 0xc0) != 0x80)				      \
	{								      \
	  (Result) = -1;						      \
	  break;							      \
	}								      \
      (Result) <<= 6;							      \
      (Result) |= ((Chars)[(Count)] & 0x3f);				      \
    }

/*
 * ported from GLib1.3
 *
 * g_utf8_get_char:
 * @p: a pointer to Unicode character encoded as UTF-8
 *
 * Converts a sequence of bytes encoded as UTF-8 to a Unicode character.
 * If @p does not point to a valid UTF-8 encoded character, results are
 * undefined. If you are not sure that the bytes are complete
 * valid Unicode characters, you should use g_utf_get_char_validated()
 * instead.
 *
 * Return value: the resulting character
 **/
unichar
charconv_utf8_get_char (const utfchar *p)
{
	int i, mask = 0, len;
	unichar result;
	unsigned char c = (unsigned char) *p;

	UTF8_COMPUTE (c, mask, len);
	if (len == -1)
		return (unichar)-1;
	UTF8_GET (result, p, i, mask, len);

	return result;
}

#else /* !HAVE_UNICODE */

extern utfchar *
charconv_local8_to_utf8(const gchar *local)
{
#if GLIB_CHECK_VERSION(1,3,1)
  char* charset;

  if (!local) /* g_strdup() handles NULL, g_locale_to_utf8() does not (yet) */
    return g_strdup("");

  get_local_charset(&charset);
  return g_convert_with_fallback (local, -1, "UTF-8", charset, 
                                  NULL, NULL, NULL, NULL);
# ifdef GLIB_1_3_x_API_was_stable
  return g_locale_to_utf8 (local, /* -1, NULL, NULL, */ NULL);
# endif
#else
  return g_strdup(local);
#endif
}

extern gchar *
charconv_utf8_to_local8(const utfchar *utf)
{
#if GLIB_CHECK_VERSION(1,3,1)
  char* charset;

  if (!utf) /* g_strdup() handles NULL, g_locale_to_utf8() does not (yet) */
    return g_strdup("");

  get_local_charset(&charset);
  return g_convert_with_fallback (utf, -1, charset, "UTF-8",
                                  NULL, NULL, NULL, NULL);
# ifdef GLIB_1_3_x_API_was_stable
  return g_locale_from_utf8 (utf, /* -1, NULL, NULL, */ NULL);
# endif
#else
  return g_strdup(utf);
#endif
}


/* The string here is statically allocated and must NOT be g_free()'d.*/
extern utfchar *
charconv_unichar_to_utf8(guint uc)
{
  static char outbuf[2];

  outbuf[0] = (uc & 0xFF);
  outbuf[1] = 0;
  
  return outbuf;
}

unichar
charconv_utf8_get_char (const utfchar *p)
{
	unichar result = *p;

	return result;
}

#endif

/* These tables could be compressed by contiguous ranges, but the benefit of doing so
 * is smallish. It would save about ~1000 bytes total.
 */

static struct {
  unsigned short keysym;
  unsigned short ucs;
} gdk_keysym_to_unicode_tab[] = {
  { 0x01a1, 0x0104 }, /*                     Aogonek ‚¡ LATIN CAPITAL LETTER A WITH OGONEK */
  { 0x01a2, 0x02d8 }, /*                       breve ‚¢ BREVE */
  { 0x01a3, 0x0141 }, /*                     Lstroke ‚£ LATIN CAPITAL LETTER L WITH STROKE */
  { 0x01a5, 0x013d }, /*                      Lcaron ‚¥ LATIN CAPITAL LETTER L WITH CARON */
  { 0x01a6, 0x015a }, /*                      Sacute ‚¦ LATIN CAPITAL LETTER S WITH ACUTE */
  { 0x01a9, 0x0160 }, /*                      Scaron ‚© LATIN CAPITAL LETTER S WITH CARON */
  { 0x01aa, 0x015e }, /*                    Scedilla ‚ª LATIN CAPITAL LETTER S WITH CEDILLA */
  { 0x01ab, 0x0164 }, /*                      Tcaron ‚« LATIN CAPITAL LETTER T WITH CARON */
  { 0x01ac, 0x0179 }, /*                      Zacute ‚¬ LATIN CAPITAL LETTER Z WITH ACUTE */
  { 0x01ae, 0x017d }, /*                      Zcaron ‚® LATIN CAPITAL LETTER Z WITH CARON */
  { 0x01af, 0x017b }, /*                   Zabovedot ‚¯ LATIN CAPITAL LETTER Z WITH DOT ABOVE */
  { 0x01b1, 0x0105 }, /*                     aogonek ‚± LATIN SMALL LETTER A WITH OGONEK */
  { 0x01b2, 0x02db }, /*                      ogonek ‚² OGONEK */
  { 0x01b3, 0x0142 }, /*                     lstroke ‚³ LATIN SMALL LETTER L WITH STROKE */
  { 0x01b5, 0x013e }, /*                      lcaron ‚µ LATIN SMALL LETTER L WITH CARON */
  { 0x01b6, 0x015b }, /*                      sacute ‚¶ LATIN SMALL LETTER S WITH ACUTE */
  { 0x01b7, 0x02c7 }, /*                       caron ‚· CARON */
  { 0x01b9, 0x0161 }, /*                      scaron ‚¹ LATIN SMALL LETTER S WITH CARON */
  { 0x01ba, 0x015f }, /*                    scedilla ‚º LATIN SMALL LETTER S WITH CEDILLA */
  { 0x01bb, 0x0165 }, /*                      tcaron ‚» LATIN SMALL LETTER T WITH CARON */
  { 0x01bc, 0x017a }, /*                      zacute ‚¼ LATIN SMALL LETTER Z WITH ACUTE */
  { 0x01bd, 0x02dd }, /*                 doubleacute ‚½ DOUBLE ACUTE ACCENT */
  { 0x01be, 0x017e }, /*                      zcaron ‚¾ LATIN SMALL LETTER Z WITH CARON */
  { 0x01bf, 0x017c }, /*                   zabovedot ‚¿ LATIN SMALL LETTER Z WITH DOT ABOVE */
  { 0x01c0, 0x0154 }, /*                      Racute ‚À LATIN CAPITAL LETTER R WITH ACUTE */
  { 0x01c3, 0x0102 }, /*                      Abreve ‚Ã LATIN CAPITAL LETTER A WITH BREVE */
  { 0x01c5, 0x0139 }, /*                      Lacute ‚Å LATIN CAPITAL LETTER L WITH ACUTE */
  { 0x01c6, 0x0106 }, /*                      Cacute ‚Æ LATIN CAPITAL LETTER C WITH ACUTE */
  { 0x01c8, 0x010c }, /*                      Ccaron ‚È LATIN CAPITAL LETTER C WITH CARON */
  { 0x01ca, 0x0118 }, /*                     Eogonek ‚Ê LATIN CAPITAL LETTER E WITH OGONEK */
  { 0x01cc, 0x011a }, /*                      Ecaron ‚Ì LATIN CAPITAL LETTER E WITH CARON */
  { 0x01cf, 0x010e }, /*                      Dcaron ‚Ï LATIN CAPITAL LETTER D WITH CARON */
  { 0x01d0, 0x0110 }, /*                     Dstroke ‚Ð LATIN CAPITAL LETTER D WITH STROKE */
  { 0x01d1, 0x0143 }, /*                      Nacute ‚Ñ LATIN CAPITAL LETTER N WITH ACUTE */
  { 0x01d2, 0x0147 }, /*                      Ncaron ‚Ò LATIN CAPITAL LETTER N WITH CARON */
  { 0x01d5, 0x0150 }, /*                Odoubleacute ‚Õ LATIN CAPITAL LETTER O WITH DOUBLE ACUTE */
  { 0x01d8, 0x0158 }, /*                      Rcaron ‚Ø LATIN CAPITAL LETTER R WITH CARON */
  { 0x01d9, 0x016e }, /*                       Uring ‚Ù LATIN CAPITAL LETTER U WITH RING ABOVE */
  { 0x01db, 0x0170 }, /*                Udoubleacute ‚Û LATIN CAPITAL LETTER U WITH DOUBLE ACUTE */
  { 0x01de, 0x0162 }, /*                    Tcedilla ‚Þ LATIN CAPITAL LETTER T WITH CEDILLA */
  { 0x01e0, 0x0155 }, /*                      racute ‚à LATIN SMALL LETTER R WITH ACUTE */
  { 0x01e3, 0x0103 }, /*                      abreve ‚ã LATIN SMALL LETTER A WITH BREVE */
  { 0x01e5, 0x013a }, /*                      lacute ‚å LATIN SMALL LETTER L WITH ACUTE */
  { 0x01e6, 0x0107 }, /*                      cacute ‚æ LATIN SMALL LETTER C WITH ACUTE */
  { 0x01e8, 0x010d }, /*                      ccaron ‚è LATIN SMALL LETTER C WITH CARON */
  { 0x01ea, 0x0119 }, /*                     eogonek ‚ê LATIN SMALL LETTER E WITH OGONEK */
  { 0x01ec, 0x011b }, /*                      ecaron ‚ì LATIN SMALL LETTER E WITH CARON */
  { 0x01ef, 0x010f }, /*                      dcaron ‚ï LATIN SMALL LETTER D WITH CARON */
  { 0x01f0, 0x0111 }, /*                     dstroke ‚ð LATIN SMALL LETTER D WITH STROKE */
  { 0x01f1, 0x0144 }, /*                      nacute ‚ñ LATIN SMALL LETTER N WITH ACUTE */
  { 0x01f2, 0x0148 }, /*                      ncaron ‚ò LATIN SMALL LETTER N WITH CARON */
  { 0x01f5, 0x0151 }, /*                odoubleacute ‚õ LATIN SMALL LETTER O WITH DOUBLE ACUTE */
  { 0x01f8, 0x0159 }, /*                      rcaron ‚ø LATIN SMALL LETTER R WITH CARON */
  { 0x01f9, 0x016f }, /*                       uring ‚ù LATIN SMALL LETTER U WITH RING ABOVE */
  { 0x01fb, 0x0171 }, /*                udoubleacute ‚û LATIN SMALL LETTER U WITH DOUBLE ACUTE */
  { 0x01fe, 0x0163 }, /*                    tcedilla ‚þ LATIN SMALL LETTER T WITH CEDILLA */
  { 0x01ff, 0x02d9 }, /*                    abovedot ‚ÿ DOT ABOVE */
  { 0x02a1, 0x0126 }, /*                     Hstroke ƒ¡ LATIN CAPITAL LETTER H WITH STROKE */
  { 0x02a6, 0x0124 }, /*                 Hcircumflex ƒ¦ LATIN CAPITAL LETTER H WITH CIRCUMFLEX */
  { 0x02a9, 0x0130 }, /*                   Iabovedot ƒ© LATIN CAPITAL LETTER I WITH DOT ABOVE */
  { 0x02ab, 0x011e }, /*                      Gbreve ƒ« LATIN CAPITAL LETTER G WITH BREVE */
  { 0x02ac, 0x0134 }, /*                 Jcircumflex ƒ¬ LATIN CAPITAL LETTER J WITH CIRCUMFLEX */
  { 0x02b1, 0x0127 }, /*                     hstroke ƒ± LATIN SMALL LETTER H WITH STROKE */
  { 0x02b6, 0x0125 }, /*                 hcircumflex ƒ¶ LATIN SMALL LETTER H WITH CIRCUMFLEX */
  { 0x02b9, 0x0131 }, /*                    idotless ƒ¹ LATIN SMALL LETTER DOTLESS I */
  { 0x02bb, 0x011f }, /*                      gbreve ƒ» LATIN SMALL LETTER G WITH BREVE */
  { 0x02bc, 0x0135 }, /*                 jcircumflex ƒ¼ LATIN SMALL LETTER J WITH CIRCUMFLEX */
  { 0x02c5, 0x010a }, /*                   Cabovedot ƒÅ LATIN CAPITAL LETTER C WITH DOT ABOVE */
  { 0x02c6, 0x0108 }, /*                 Ccircumflex ƒÆ LATIN CAPITAL LETTER C WITH CIRCUMFLEX */
  { 0x02d5, 0x0120 }, /*                   Gabovedot ƒÕ LATIN CAPITAL LETTER G WITH DOT ABOVE */
  { 0x02d8, 0x011c }, /*                 Gcircumflex ƒØ LATIN CAPITAL LETTER G WITH CIRCUMFLEX */
  { 0x02dd, 0x016c }, /*                      Ubreve ƒÝ LATIN CAPITAL LETTER U WITH BREVE */
  { 0x02de, 0x015c }, /*                 Scircumflex ƒÞ LATIN CAPITAL LETTER S WITH CIRCUMFLEX */
  { 0x02e5, 0x010b }, /*                   cabovedot ƒå LATIN SMALL LETTER C WITH DOT ABOVE */
  { 0x02e6, 0x0109 }, /*                 ccircumflex ƒæ LATIN SMALL LETTER C WITH CIRCUMFLEX */
  { 0x02f5, 0x0121 }, /*                   gabovedot ƒõ LATIN SMALL LETTER G WITH DOT ABOVE */
  { 0x02f8, 0x011d }, /*                 gcircumflex ƒø LATIN SMALL LETTER G WITH CIRCUMFLEX */
  { 0x02fd, 0x016d }, /*                      ubreve ƒý LATIN SMALL LETTER U WITH BREVE */
  { 0x02fe, 0x015d }, /*                 scircumflex ƒþ LATIN SMALL LETTER S WITH CIRCUMFLEX */
  { 0x03a2, 0x0138 }, /*                         kra „¢ LATIN SMALL LETTER KRA */
  { 0x03a3, 0x0156 }, /*                    Rcedilla „£ LATIN CAPITAL LETTER R WITH CEDILLA */
  { 0x03a5, 0x0128 }, /*                      Itilde „¥ LATIN CAPITAL LETTER I WITH TILDE */
  { 0x03a6, 0x013b }, /*                    Lcedilla „¦ LATIN CAPITAL LETTER L WITH CEDILLA */
  { 0x03aa, 0x0112 }, /*                     Emacron „ª LATIN CAPITAL LETTER E WITH MACRON */
  { 0x03ab, 0x0122 }, /*                    Gcedilla „« LATIN CAPITAL LETTER G WITH CEDILLA */
  { 0x03ac, 0x0166 }, /*                      Tslash „¬ LATIN CAPITAL LETTER T WITH STROKE */
  { 0x03b3, 0x0157 }, /*                    rcedilla „³ LATIN SMALL LETTER R WITH CEDILLA */
  { 0x03b5, 0x0129 }, /*                      itilde „µ LATIN SMALL LETTER I WITH TILDE */
  { 0x03b6, 0x013c }, /*                    lcedilla „¶ LATIN SMALL LETTER L WITH CEDILLA */
  { 0x03ba, 0x0113 }, /*                     emacron „º LATIN SMALL LETTER E WITH MACRON */
  { 0x03bb, 0x0123 }, /*                    gcedilla „» LATIN SMALL LETTER G WITH CEDILLA */
  { 0x03bc, 0x0167 }, /*                      tslash „¼ LATIN SMALL LETTER T WITH STROKE */
  { 0x03bd, 0x014a }, /*                         ENG „½ LATIN CAPITAL LETTER ENG */
  { 0x03bf, 0x014b }, /*                         eng „¿ LATIN SMALL LETTER ENG */
  { 0x03c0, 0x0100 }, /*                     Amacron „À LATIN CAPITAL LETTER A WITH MACRON */
  { 0x03c7, 0x012e }, /*                     Iogonek „Ç LATIN CAPITAL LETTER I WITH OGONEK */
  { 0x03cc, 0x0116 }, /*                   Eabovedot „Ì LATIN CAPITAL LETTER E WITH DOT ABOVE */
  { 0x03cf, 0x012a }, /*                     Imacron „Ï LATIN CAPITAL LETTER I WITH MACRON */
  { 0x03d1, 0x0145 }, /*                    Ncedilla „Ñ LATIN CAPITAL LETTER N WITH CEDILLA */
  { 0x03d2, 0x014c }, /*                     Omacron „Ò LATIN CAPITAL LETTER O WITH MACRON */
  { 0x03d3, 0x0136 }, /*                    Kcedilla „Ó LATIN CAPITAL LETTER K WITH CEDILLA */
  { 0x03d9, 0x0172 }, /*                     Uogonek „Ù LATIN CAPITAL LETTER U WITH OGONEK */
  { 0x03dd, 0x0168 }, /*                      Utilde „Ý LATIN CAPITAL LETTER U WITH TILDE */
  { 0x03de, 0x016a }, /*                     Umacron „Þ LATIN CAPITAL LETTER U WITH MACRON */
  { 0x03e0, 0x0101 }, /*                     amacron „à LATIN SMALL LETTER A WITH MACRON */
  { 0x03e7, 0x012f }, /*                     iogonek „ç LATIN SMALL LETTER I WITH OGONEK */
  { 0x03ec, 0x0117 }, /*                   eabovedot „ì LATIN SMALL LETTER E WITH DOT ABOVE */
  { 0x03ef, 0x012b }, /*                     imacron „ï LATIN SMALL LETTER I WITH MACRON */
  { 0x03f1, 0x0146 }, /*                    ncedilla „ñ LATIN SMALL LETTER N WITH CEDILLA */
  { 0x03f2, 0x014d }, /*                     omacron „ò LATIN SMALL LETTER O WITH MACRON */
  { 0x03f3, 0x0137 }, /*                    kcedilla „ó LATIN SMALL LETTER K WITH CEDILLA */
  { 0x03f9, 0x0173 }, /*                     uogonek „ù LATIN SMALL LETTER U WITH OGONEK */
  { 0x03fd, 0x0169 }, /*                      utilde „ý LATIN SMALL LETTER U WITH TILDE */
  { 0x03fe, 0x016b }, /*                     umacron „þ LATIN SMALL LETTER U WITH MACRON */
  { 0x047e, 0x203e }, /*                    overline ˆ¯ OVERLINE */
  { 0x04a1, 0x3002 }, /*               kana_fullstop ’¡£ IDEOGRAPHIC FULL STOP */
  { 0x04a2, 0x300c }, /*         kana_openingbracket ’¡Ö LEFT CORNER BRACKET */
  { 0x04a3, 0x300d }, /*         kana_closingbracket ’¡× RIGHT CORNER BRACKET */
  { 0x04a4, 0x3001 }, /*                  kana_comma ’¡¢ IDEOGRAPHIC COMMA */
  { 0x04a5, 0x30fb }, /*            kana_conjunctive ’¡¦ KATAKANA MIDDLE DOT */
  { 0x04a6, 0x30f2 }, /*                     kana_WO ’¥ò KATAKANA LETTER WO */
  { 0x04a7, 0x30a1 }, /*                      kana_a ’¥¡ KATAKANA LETTER SMALL A */
  { 0x04a8, 0x30a3 }, /*                      kana_i ’¥£ KATAKANA LETTER SMALL I */
  { 0x04a9, 0x30a5 }, /*                      kana_u ’¥¥ KATAKANA LETTER SMALL U */
  { 0x04aa, 0x30a7 }, /*                      kana_e ’¥§ KATAKANA LETTER SMALL E */
  { 0x04ab, 0x30a9 }, /*                      kana_o ’¥© KATAKANA LETTER SMALL O */
  { 0x04ac, 0x30e3 }, /*                     kana_ya ’¥ã KATAKANA LETTER SMALL YA */
  { 0x04ad, 0x30e5 }, /*                     kana_yu ’¥å KATAKANA LETTER SMALL YU */
  { 0x04ae, 0x30e7 }, /*                     kana_yo ’¥ç KATAKANA LETTER SMALL YO */
  { 0x04af, 0x30c3 }, /*                    kana_tsu ’¥Ã KATAKANA LETTER SMALL TU */
  { 0x04b0, 0x30fc }, /*              prolongedsound ’¡¼ KATAKANA-HIRAGANA PROLONGED SOUND MARK */
  { 0x04b1, 0x30a2 }, /*                      kana_A ’¥¢ KATAKANA LETTER A */
  { 0x04b2, 0x30a4 }, /*                      kana_I ’¥¤ KATAKANA LETTER I */
  { 0x04b3, 0x30a6 }, /*                      kana_U ’¥¦ KATAKANA LETTER U */
  { 0x04b4, 0x30a8 }, /*                      kana_E ’¥¨ KATAKANA LETTER E */
  { 0x04b5, 0x30aa }, /*                      kana_O ’¥ª KATAKANA LETTER O */
  { 0x04b6, 0x30ab }, /*                     kana_KA ’¥« KATAKANA LETTER KA */
  { 0x04b7, 0x30ad }, /*                     kana_KI ’¥­ KATAKANA LETTER KI */
  { 0x04b8, 0x30af }, /*                     kana_KU ’¥¯ KATAKANA LETTER KU */
  { 0x04b9, 0x30b1 }, /*                     kana_KE ’¥± KATAKANA LETTER KE */
  { 0x04ba, 0x30b3 }, /*                     kana_KO ’¥³ KATAKANA LETTER KO */
  { 0x04bb, 0x30b5 }, /*                     kana_SA ’¥µ KATAKANA LETTER SA */
  { 0x04bc, 0x30b7 }, /*                    kana_SHI ’¥· KATAKANA LETTER SI */
  { 0x04bd, 0x30b9 }, /*                     kana_SU ’¥¹ KATAKANA LETTER SU */
  { 0x04be, 0x30bb }, /*                     kana_SE ’¥» KATAKANA LETTER SE */
  { 0x04bf, 0x30bd }, /*                     kana_SO ’¥½ KATAKANA LETTER SO */
  { 0x04c0, 0x30bf }, /*                     kana_TA ’¥¿ KATAKANA LETTER TA */
  { 0x04c1, 0x30c1 }, /*                    kana_CHI ’¥Á KATAKANA LETTER TI */
  { 0x04c2, 0x30c4 }, /*                    kana_TSU ’¥Ä KATAKANA LETTER TU */
  { 0x04c3, 0x30c6 }, /*                     kana_TE ’¥Æ KATAKANA LETTER TE */
  { 0x04c4, 0x30c8 }, /*                     kana_TO ’¥È KATAKANA LETTER TO */
  { 0x04c5, 0x30ca }, /*                     kana_NA ’¥Ê KATAKANA LETTER NA */
  { 0x04c6, 0x30cb }, /*                     kana_NI ’¥Ë KATAKANA LETTER NI */
  { 0x04c7, 0x30cc }, /*                     kana_NU ’¥Ì KATAKANA LETTER NU */
  { 0x04c8, 0x30cd }, /*                     kana_NE ’¥Í KATAKANA LETTER NE */
  { 0x04c9, 0x30ce }, /*                     kana_NO ’¥Î KATAKANA LETTER NO */
  { 0x04ca, 0x30cf }, /*                     kana_HA ’¥Ï KATAKANA LETTER HA */
  { 0x04cb, 0x30d2 }, /*                     kana_HI ’¥Ò KATAKANA LETTER HI */
  { 0x04cc, 0x30d5 }, /*                     kana_FU ’¥Õ KATAKANA LETTER HU */
  { 0x04cd, 0x30d8 }, /*                     kana_HE ’¥Ø KATAKANA LETTER HE */
  { 0x04ce, 0x30db }, /*                     kana_HO ’¥Û KATAKANA LETTER HO */
  { 0x04cf, 0x30de }, /*                     kana_MA ’¥Þ KATAKANA LETTER MA */
  { 0x04d0, 0x30df }, /*                     kana_MI ’¥ß KATAKANA LETTER MI */
  { 0x04d1, 0x30e0 }, /*                     kana_MU ’¥à KATAKANA LETTER MU */
  { 0x04d2, 0x30e1 }, /*                     kana_ME ’¥á KATAKANA LETTER ME */
  { 0x04d3, 0x30e2 }, /*                     kana_MO ’¥â KATAKANA LETTER MO */
  { 0x04d4, 0x30e4 }, /*                     kana_YA ’¥ä KATAKANA LETTER YA */
  { 0x04d5, 0x30e6 }, /*                     kana_YU ’¥æ KATAKANA LETTER YU */
  { 0x04d6, 0x30e8 }, /*                     kana_YO ’¥è KATAKANA LETTER YO */
  { 0x04d7, 0x30e9 }, /*                     kana_RA ’¥é KATAKANA LETTER RA */
  { 0x04d8, 0x30ea }, /*                     kana_RI ’¥ê KATAKANA LETTER RI */
  { 0x04d9, 0x30eb }, /*                     kana_RU ’¥ë KATAKANA LETTER RU */
  { 0x04da, 0x30ec }, /*                     kana_RE ’¥ì KATAKANA LETTER RE */
  { 0x04db, 0x30ed }, /*                     kana_RO ’¥í KATAKANA LETTER RO */
  { 0x04dc, 0x30ef }, /*                     kana_WA ’¥ï KATAKANA LETTER WA */
  { 0x04dd, 0x30f3 }, /*                      kana_N ’¥ó KATAKANA LETTER N */
  { 0x04de, 0x309b }, /*                 voicedsound ’¡« KATAKANA-HIRAGANA VOICED SOUND MARK */
  { 0x04df, 0x309c }, /*             semivoicedsound ’¡¬ KATAKANA-HIRAGANA SEMI-VOICED SOUND MARK */
  { 0x05ac, 0x060c }, /*                Arabic_comma œô­Ì ARABIC COMMA */
  { 0x05bb, 0x061b }, /*            Arabic_semicolon œô­Û ARABIC SEMICOLON */
  { 0x05bf, 0x061f }, /*        Arabic_question_mark œô­ß ARABIC QUESTION MARK */
  { 0x05c1, 0x0621 }, /*                Arabic_hamza œô­á ARABIC LETTER HAMZA */
  { 0x05c2, 0x0622 }, /*          Arabic_maddaonalef œô­â ARABIC LETTER ALEF WITH MADDA ABOVE */
  { 0x05c3, 0x0623 }, /*          Arabic_hamzaonalef œô­ã ARABIC LETTER ALEF WITH HAMZA ABOVE */
  { 0x05c4, 0x0624 }, /*           Arabic_hamzaonwaw œô­ä ARABIC LETTER WAW WITH HAMZA ABOVE */
  { 0x05c5, 0x0625 }, /*       Arabic_hamzaunderalef œô­å ARABIC LETTER ALEF WITH HAMZA BELOW */
  { 0x05c6, 0x0626 }, /*           Arabic_hamzaonyeh œô­æ ARABIC LETTER YEH WITH HAMZA ABOVE */
  { 0x05c7, 0x0627 }, /*                 Arabic_alef œô­ç ARABIC LETTER ALEF */
  { 0x05c8, 0x0628 }, /*                  Arabic_beh œô­è ARABIC LETTER BEH */
  { 0x05c9, 0x0629 }, /*           Arabic_tehmarbuta œô­é ARABIC LETTER TEH MARBUTA */
  { 0x05ca, 0x062a }, /*                  Arabic_teh œô­ê ARABIC LETTER TEH */
  { 0x05cb, 0x062b }, /*                 Arabic_theh œô­ë ARABIC LETTER THEH */
  { 0x05cc, 0x062c }, /*                 Arabic_jeem œô­ì ARABIC LETTER JEEM */
  { 0x05cd, 0x062d }, /*                  Arabic_hah œô­í ARABIC LETTER HAH */
  { 0x05ce, 0x062e }, /*                 Arabic_khah œô­î ARABIC LETTER KHAH */
  { 0x05cf, 0x062f }, /*                  Arabic_dal œô­ï ARABIC LETTER DAL */
  { 0x05d0, 0x0630 }, /*                 Arabic_thal œô­ð ARABIC LETTER THAL */
  { 0x05d1, 0x0631 }, /*                   Arabic_ra œô­ñ ARABIC LETTER REH */
  { 0x05d2, 0x0632 }, /*                 Arabic_zain œô­ò ARABIC LETTER ZAIN */
  { 0x05d3, 0x0633 }, /*                 Arabic_seen œô­ó ARABIC LETTER SEEN */
  { 0x05d4, 0x0634 }, /*                Arabic_sheen œô­ô ARABIC LETTER SHEEN */
  { 0x05d5, 0x0635 }, /*                  Arabic_sad œô­õ ARABIC LETTER SAD */
  { 0x05d6, 0x0636 }, /*                  Arabic_dad œô­ö ARABIC LETTER DAD */
  { 0x05d7, 0x0637 }, /*                  Arabic_tah œô­÷ ARABIC LETTER TAH */
  { 0x05d8, 0x0638 }, /*                  Arabic_zah œô­ø ARABIC LETTER ZAH */
  { 0x05d9, 0x0639 }, /*                  Arabic_ain œô­ù ARABIC LETTER AIN */
  { 0x05da, 0x063a }, /*                Arabic_ghain œô­ú ARABIC LETTER GHAIN */
  { 0x05e0, 0x0640 }, /*              Arabic_tatweel œô®  ARABIC TATWEEL */
  { 0x05e1, 0x0641 }, /*                  Arabic_feh œô®¡ ARABIC LETTER FEH */
  { 0x05e2, 0x0642 }, /*                  Arabic_qaf œô®¢ ARABIC LETTER QAF */
  { 0x05e3, 0x0643 }, /*                  Arabic_kaf œô®£ ARABIC LETTER KAF */
  { 0x05e4, 0x0644 }, /*                  Arabic_lam œô®¤ ARABIC LETTER LAM */
  { 0x05e5, 0x0645 }, /*                 Arabic_meem œô®¥ ARABIC LETTER MEEM */
  { 0x05e6, 0x0646 }, /*                 Arabic_noon œô®¦ ARABIC LETTER NOON */
  { 0x05e7, 0x0647 }, /*                   Arabic_ha œô®§ ARABIC LETTER HEH */
  { 0x05e8, 0x0648 }, /*                  Arabic_waw œô®¨ ARABIC LETTER WAW */
  { 0x05e9, 0x0649 }, /*          Arabic_alefmaksura œô®© ARABIC LETTER ALEF MAKSURA */
  { 0x05ea, 0x064a }, /*                  Arabic_yeh œô®ª ARABIC LETTER YEH */
  { 0x05eb, 0x064b }, /*             Arabic_fathatan œô®« ARABIC FATHATAN */
  { 0x05ec, 0x064c }, /*             Arabic_dammatan œô®¬ ARABIC DAMMATAN */
  { 0x05ed, 0x064d }, /*             Arabic_kasratan œô®­ ARABIC KASRATAN */
  { 0x05ee, 0x064e }, /*                Arabic_fatha œô®® ARABIC FATHA */
  { 0x05ef, 0x064f }, /*                Arabic_damma œô®¯ ARABIC DAMMA */
  { 0x05f0, 0x0650 }, /*                Arabic_kasra œô®° ARABIC KASRA */
  { 0x05f1, 0x0651 }, /*               Arabic_shadda œô®± ARABIC SHADDA */
  { 0x05f2, 0x0652 }, /*                Arabic_sukun œô®² ARABIC SUKUN */
  { 0x06a1, 0x0452 }, /*                 Serbian_dje Œò CYRILLIC SMALL LETTER DJE */
  { 0x06a2, 0x0453 }, /*               Macedonia_gje Œó CYRILLIC SMALL LETTER GJE */
  { 0x06a3, 0x0451 }, /*                 Cyrillic_io Œñ CYRILLIC SMALL LETTER IO */
  { 0x06a4, 0x0454 }, /*                Ukrainian_ie Œô CYRILLIC SMALL LETTER UKRAINIAN IE */
  { 0x06a5, 0x0455 }, /*               Macedonia_dse Œõ CYRILLIC SMALL LETTER DZE */
  { 0x06a6, 0x0456 }, /*                 Ukrainian_i Œö CYRILLIC SMALL LETTER BYELORUSSIAN-UKRAINIAN I */
  { 0x06a7, 0x0457 }, /*                Ukrainian_yi Œ÷ CYRILLIC SMALL LETTER YI */
  { 0x06a8, 0x0458 }, /*                 Cyrillic_je Œø CYRILLIC SMALL LETTER JE */
  { 0x06a9, 0x0459 }, /*                Cyrillic_lje Œù CYRILLIC SMALL LETTER LJE */
  { 0x06aa, 0x045a }, /*                Cyrillic_nje Œú CYRILLIC SMALL LETTER NJE */
  { 0x06ab, 0x045b }, /*                Serbian_tshe Œû CYRILLIC SMALL LETTER TSHE */
  { 0x06ac, 0x045c }, /*               Macedonia_kje Œü CYRILLIC SMALL LETTER KJE */
  { 0x06ae, 0x045e }, /*         Byelorussian_shortu Œþ CYRILLIC SMALL LETTER SHORT U */
  { 0x06af, 0x045f }, /*               Cyrillic_dzhe Œÿ CYRILLIC SMALL LETTER DZHE */
  { 0x06b0, 0x2116 }, /*                  numerosign Œð NUMERO SIGN */
  { 0x06b1, 0x0402 }, /*                 Serbian_DJE Œ¢ CYRILLIC CAPITAL LETTER DJE */
  { 0x06b2, 0x0403 }, /*               Macedonia_GJE Œ£ CYRILLIC CAPITAL LETTER GJE */
  { 0x06b3, 0x0401 }, /*                 Cyrillic_IO Œ¡ CYRILLIC CAPITAL LETTER IO */
  { 0x06b4, 0x0404 }, /*                Ukrainian_IE Œ¤ CYRILLIC CAPITAL LETTER UKRAINIAN IE */
  { 0x06b5, 0x0405 }, /*               Macedonia_DSE Œ¥ CYRILLIC CAPITAL LETTER DZE */
  { 0x06b6, 0x0406 }, /*                 Ukrainian_I Œ¦ CYRILLIC CAPITAL LETTER BYELORUSSIAN-UKRAINIAN I */
  { 0x06b7, 0x0407 }, /*                Ukrainian_YI Œ§ CYRILLIC CAPITAL LETTER YI */
  { 0x06b8, 0x0408 }, /*                 Cyrillic_JE Œ¨ CYRILLIC CAPITAL LETTER JE */
  { 0x06b9, 0x0409 }, /*                Cyrillic_LJE Œ© CYRILLIC CAPITAL LETTER LJE */
  { 0x06ba, 0x040a }, /*                Cyrillic_NJE Œª CYRILLIC CAPITAL LETTER NJE */
  { 0x06bb, 0x040b }, /*                Serbian_TSHE Œ« CYRILLIC CAPITAL LETTER TSHE */
  { 0x06bc, 0x040c }, /*               Macedonia_KJE Œ¬ CYRILLIC CAPITAL LETTER KJE */
  { 0x06be, 0x040e }, /*         Byelorussian_SHORTU Œ® CYRILLIC CAPITAL LETTER SHORT U */
  { 0x06bf, 0x040f }, /*               Cyrillic_DZHE Œ¯ CYRILLIC CAPITAL LETTER DZHE */
  { 0x06c0, 0x044e }, /*                 Cyrillic_yu Œî CYRILLIC SMALL LETTER YU */
  { 0x06c1, 0x0430 }, /*                  Cyrillic_a ŒÐ CYRILLIC SMALL LETTER A */
  { 0x06c2, 0x0431 }, /*                 Cyrillic_be ŒÑ CYRILLIC SMALL LETTER BE */
  { 0x06c3, 0x0446 }, /*                Cyrillic_tse Œæ CYRILLIC SMALL LETTER TSE */
  { 0x06c4, 0x0434 }, /*                 Cyrillic_de ŒÔ CYRILLIC SMALL LETTER DE */
  { 0x06c5, 0x0435 }, /*                 Cyrillic_ie ŒÕ CYRILLIC SMALL LETTER IE */
  { 0x06c6, 0x0444 }, /*                 Cyrillic_ef Œä CYRILLIC SMALL LETTER EF */
  { 0x06c7, 0x0433 }, /*                Cyrillic_ghe ŒÓ CYRILLIC SMALL LETTER GHE */
  { 0x06c8, 0x0445 }, /*                 Cyrillic_ha Œå CYRILLIC SMALL LETTER HA */
  { 0x06c9, 0x0438 }, /*                  Cyrillic_i ŒØ CYRILLIC SMALL LETTER I */
  { 0x06ca, 0x0439 }, /*             Cyrillic_shorti ŒÙ CYRILLIC SMALL LETTER SHORT I */
  { 0x06cb, 0x043a }, /*                 Cyrillic_ka ŒÚ CYRILLIC SMALL LETTER KA */
  { 0x06cc, 0x043b }, /*                 Cyrillic_el ŒÛ CYRILLIC SMALL LETTER EL */
  { 0x06cd, 0x043c }, /*                 Cyrillic_em ŒÜ CYRILLIC SMALL LETTER EM */
  { 0x06ce, 0x043d }, /*                 Cyrillic_en ŒÝ CYRILLIC SMALL LETTER EN */
  { 0x06cf, 0x043e }, /*                  Cyrillic_o ŒÞ CYRILLIC SMALL LETTER O */
  { 0x06d0, 0x043f }, /*                 Cyrillic_pe Œß CYRILLIC SMALL LETTER PE */
  { 0x06d1, 0x044f }, /*                 Cyrillic_ya Œï CYRILLIC SMALL LETTER YA */
  { 0x06d2, 0x0440 }, /*                 Cyrillic_er Œà CYRILLIC SMALL LETTER ER */
  { 0x06d3, 0x0441 }, /*                 Cyrillic_es Œá CYRILLIC SMALL LETTER ES */
  { 0x06d4, 0x0442 }, /*                 Cyrillic_te Œâ CYRILLIC SMALL LETTER TE */
  { 0x06d5, 0x0443 }, /*                  Cyrillic_u Œã CYRILLIC SMALL LETTER U */
  { 0x06d6, 0x0436 }, /*                Cyrillic_zhe ŒÖ CYRILLIC SMALL LETTER ZHE */
  { 0x06d7, 0x0432 }, /*                 Cyrillic_ve ŒÒ CYRILLIC SMALL LETTER VE */
  { 0x06d8, 0x044c }, /*           Cyrillic_softsign Œì CYRILLIC SMALL LETTER SOFT SIGN */
  { 0x06d9, 0x044b }, /*               Cyrillic_yeru Œë CYRILLIC SMALL LETTER YERU */
  { 0x06da, 0x0437 }, /*                 Cyrillic_ze Œ× CYRILLIC SMALL LETTER ZE */
  { 0x06db, 0x0448 }, /*                Cyrillic_sha Œè CYRILLIC SMALL LETTER SHA */
  { 0x06dc, 0x044d }, /*                  Cyrillic_e Œí CYRILLIC SMALL LETTER E */
  { 0x06dd, 0x0449 }, /*              Cyrillic_shcha Œé CYRILLIC SMALL LETTER SHCHA */
  { 0x06de, 0x0447 }, /*                Cyrillic_che Œç CYRILLIC SMALL LETTER CHE */
  { 0x06df, 0x044a }, /*           Cyrillic_hardsign Œê CYRILLIC SMALL LETTER HARD SIGN */
  { 0x06e0, 0x042e }, /*                 Cyrillic_YU ŒÎ CYRILLIC CAPITAL LETTER YU */
  { 0x06e1, 0x0410 }, /*                  Cyrillic_A Œ° CYRILLIC CAPITAL LETTER A */
  { 0x06e2, 0x0411 }, /*                 Cyrillic_BE Œ± CYRILLIC CAPITAL LETTER BE */
  { 0x06e3, 0x0426 }, /*                Cyrillic_TSE ŒÆ CYRILLIC CAPITAL LETTER TSE */
  { 0x06e4, 0x0414 }, /*                 Cyrillic_DE Œ´ CYRILLIC CAPITAL LETTER DE */
  { 0x06e5, 0x0415 }, /*                 Cyrillic_IE Œµ CYRILLIC CAPITAL LETTER IE */
  { 0x06e6, 0x0424 }, /*                 Cyrillic_EF ŒÄ CYRILLIC CAPITAL LETTER EF */
  { 0x06e7, 0x0413 }, /*                Cyrillic_GHE Œ³ CYRILLIC CAPITAL LETTER GHE */
  { 0x06e8, 0x0425 }, /*                 Cyrillic_HA ŒÅ CYRILLIC CAPITAL LETTER HA */
  { 0x06e9, 0x0418 }, /*                  Cyrillic_I Œ¸ CYRILLIC CAPITAL LETTER I */
  { 0x06ea, 0x0419 }, /*             Cyrillic_SHORTI Œ¹ CYRILLIC CAPITAL LETTER SHORT I */
  { 0x06eb, 0x041a }, /*                 Cyrillic_KA Œº CYRILLIC CAPITAL LETTER KA */
  { 0x06ec, 0x041b }, /*                 Cyrillic_EL Œ» CYRILLIC CAPITAL LETTER EL */
  { 0x06ed, 0x041c }, /*                 Cyrillic_EM Œ¼ CYRILLIC CAPITAL LETTER EM */
  { 0x06ee, 0x041d }, /*                 Cyrillic_EN Œ½ CYRILLIC CAPITAL LETTER EN */
  { 0x06ef, 0x041e }, /*                  Cyrillic_O Œ¾ CYRILLIC CAPITAL LETTER O */
  { 0x06f0, 0x041f }, /*                 Cyrillic_PE Œ¿ CYRILLIC CAPITAL LETTER PE */
  { 0x06f1, 0x042f }, /*                 Cyrillic_YA ŒÏ CYRILLIC CAPITAL LETTER YA */
  { 0x06f2, 0x0420 }, /*                 Cyrillic_ER ŒÀ CYRILLIC CAPITAL LETTER ER */
  { 0x06f3, 0x0421 }, /*                 Cyrillic_ES ŒÁ CYRILLIC CAPITAL LETTER ES */
  { 0x06f4, 0x0422 }, /*                 Cyrillic_TE ŒÂ CYRILLIC CAPITAL LETTER TE */
  { 0x06f5, 0x0423 }, /*                  Cyrillic_U ŒÃ CYRILLIC CAPITAL LETTER U */
  { 0x06f6, 0x0416 }, /*                Cyrillic_ZHE Œ¶ CYRILLIC CAPITAL LETTER ZHE */
  { 0x06f7, 0x0412 }, /*                 Cyrillic_VE Œ² CYRILLIC CAPITAL LETTER VE */
  { 0x06f8, 0x042c }, /*           Cyrillic_SOFTSIGN ŒÌ CYRILLIC CAPITAL LETTER SOFT SIGN */
  { 0x06f9, 0x042b }, /*               Cyrillic_YERU ŒË CYRILLIC CAPITAL LETTER YERU */
  { 0x06fa, 0x0417 }, /*                 Cyrillic_ZE Œ· CYRILLIC CAPITAL LETTER ZE */
  { 0x06fb, 0x0428 }, /*                Cyrillic_SHA ŒÈ CYRILLIC CAPITAL LETTER SHA */
  { 0x06fc, 0x042d }, /*                  Cyrillic_E ŒÍ CYRILLIC CAPITAL LETTER E */
  { 0x06fd, 0x0429 }, /*              Cyrillic_SHCHA ŒÉ CYRILLIC CAPITAL LETTER SHCHA */
  { 0x06fe, 0x0427 }, /*                Cyrillic_CHE ŒÇ CYRILLIC CAPITAL LETTER CHE */
  { 0x06ff, 0x042a }, /*           Cyrillic_HARDSIGN ŒÊ CYRILLIC CAPITAL LETTER HARD SIGN */
  { 0x07a1, 0x0386 }, /*           Greek_ALPHAaccent †¶ GREEK CAPITAL LETTER ALPHA WITH TONOS */
  { 0x07a2, 0x0388 }, /*         Greek_EPSILONaccent †¸ GREEK CAPITAL LETTER EPSILON WITH TONOS */
  { 0x07a3, 0x0389 }, /*             Greek_ETAaccent †¹ GREEK CAPITAL LETTER ETA WITH TONOS */
  { 0x07a4, 0x038a }, /*            Greek_IOTAaccent †º GREEK CAPITAL LETTER IOTA WITH TONOS */
  { 0x07a5, 0x03aa }, /*         Greek_IOTAdiaeresis †Ú GREEK CAPITAL LETTER IOTA WITH DIALYTIKA */
  { 0x07a7, 0x038c }, /*         Greek_OMICRONaccent †¼ GREEK CAPITAL LETTER OMICRON WITH TONOS */
  { 0x07a8, 0x038e }, /*         Greek_UPSILONaccent †¾ GREEK CAPITAL LETTER UPSILON WITH TONOS */
  { 0x07a9, 0x03ab }, /*       Greek_UPSILONdieresis †Û GREEK CAPITAL LETTER UPSILON WITH DIALYTIKA */
  { 0x07ab, 0x038f }, /*           Greek_OMEGAaccent †¿ GREEK CAPITAL LETTER OMEGA WITH TONOS */
  { 0x07ae, 0x0385 }, /*        Greek_accentdieresis †µ GREEK DIALYTIKA TONOS */
  { 0x07af, 0x2015 }, /*              Greek_horizbar †¯ HORIZONTAL BAR */
  { 0x07b1, 0x03ac }, /*           Greek_alphaaccent †Ü GREEK SMALL LETTER ALPHA WITH TONOS */
  { 0x07b2, 0x03ad }, /*         Greek_epsilonaccent †Ý GREEK SMALL LETTER EPSILON WITH TONOS */
  { 0x07b3, 0x03ae }, /*             Greek_etaaccent †Þ GREEK SMALL LETTER ETA WITH TONOS */
  { 0x07b4, 0x03af }, /*            Greek_iotaaccent †ß GREEK SMALL LETTER IOTA WITH TONOS */
  { 0x07b5, 0x03ca }, /*          Greek_iotadieresis †ú GREEK SMALL LETTER IOTA WITH DIALYTIKA */
  { 0x07b6, 0x0390 }, /*    Greek_iotaaccentdieresis †À GREEK SMALL LETTER IOTA WITH DIALYTIKA AND TONOS */
  { 0x07b7, 0x03cc }, /*         Greek_omicronaccent †ü GREEK SMALL LETTER OMICRON WITH TONOS */
  { 0x07b8, 0x03cd }, /*         Greek_upsilonaccent †ý GREEK SMALL LETTER UPSILON WITH TONOS */
  { 0x07b9, 0x03cb }, /*       Greek_upsilondieresis †û GREEK SMALL LETTER UPSILON WITH DIALYTIKA */
  { 0x07ba, 0x03b0 }, /* Greek_upsilonaccentdieresis †à GREEK SMALL LETTER UPSILON WITH DIALYTIKA AND TONOS */
  { 0x07bb, 0x03ce }, /*           Greek_omegaaccent †þ GREEK SMALL LETTER OMEGA WITH TONOS */
  { 0x07c1, 0x0391 }, /*                 Greek_ALPHA †Á GREEK CAPITAL LETTER ALPHA */
  { 0x07c2, 0x0392 }, /*                  Greek_BETA †Â GREEK CAPITAL LETTER BETA */
  { 0x07c3, 0x0393 }, /*                 Greek_GAMMA †Ã GREEK CAPITAL LETTER GAMMA */
  { 0x07c4, 0x0394 }, /*                 Greek_DELTA †Ä GREEK CAPITAL LETTER DELTA */
  { 0x07c5, 0x0395 }, /*               Greek_EPSILON †Å GREEK CAPITAL LETTER EPSILON */
  { 0x07c6, 0x0396 }, /*                  Greek_ZETA †Æ GREEK CAPITAL LETTER ZETA */
  { 0x07c7, 0x0397 }, /*                   Greek_ETA †Ç GREEK CAPITAL LETTER ETA */
  { 0x07c8, 0x0398 }, /*                 Greek_THETA †È GREEK CAPITAL LETTER THETA */
  { 0x07c9, 0x0399 }, /*                  Greek_IOTA †É GREEK CAPITAL LETTER IOTA */
  { 0x07ca, 0x039a }, /*                 Greek_KAPPA †Ê GREEK CAPITAL LETTER KAPPA */
  { 0x07cb, 0x039b }, /*                Greek_LAMBDA †Ë GREEK CAPITAL LETTER LAMDA */
  { 0x07cc, 0x039c }, /*                    Greek_MU †Ì GREEK CAPITAL LETTER MU */
  { 0x07cd, 0x039d }, /*                    Greek_NU †Í GREEK CAPITAL LETTER NU */
  { 0x07ce, 0x039e }, /*                    Greek_XI †Î GREEK CAPITAL LETTER XI */
  { 0x07cf, 0x039f }, /*               Greek_OMICRON †Ï GREEK CAPITAL LETTER OMICRON */
  { 0x07d0, 0x03a0 }, /*                    Greek_PI †Ð GREEK CAPITAL LETTER PI */
  { 0x07d1, 0x03a1 }, /*                   Greek_RHO †Ñ GREEK CAPITAL LETTER RHO */
  { 0x07d2, 0x03a3 }, /*                 Greek_SIGMA †Ó GREEK CAPITAL LETTER SIGMA */
  { 0x07d4, 0x03a4 }, /*                   Greek_TAU †Ô GREEK CAPITAL LETTER TAU */
  { 0x07d5, 0x03a5 }, /*               Greek_UPSILON †Õ GREEK CAPITAL LETTER UPSILON */
  { 0x07d6, 0x03a6 }, /*                   Greek_PHI †Ö GREEK CAPITAL LETTER PHI */
  { 0x07d7, 0x03a7 }, /*                   Greek_CHI †× GREEK CAPITAL LETTER CHI */
  { 0x07d8, 0x03a8 }, /*                   Greek_PSI †Ø GREEK CAPITAL LETTER PSI */
  { 0x07d9, 0x03a9 }, /*                 Greek_OMEGA †Ù GREEK CAPITAL LETTER OMEGA */
  { 0x07e1, 0x03b1 }, /*                 Greek_alpha †á GREEK SMALL LETTER ALPHA */
  { 0x07e2, 0x03b2 }, /*                  Greek_beta †â GREEK SMALL LETTER BETA */
  { 0x07e3, 0x03b3 }, /*                 Greek_gamma †ã GREEK SMALL LETTER GAMMA */
  { 0x07e4, 0x03b4 }, /*                 Greek_delta †ä GREEK SMALL LETTER DELTA */
  { 0x07e5, 0x03b5 }, /*               Greek_epsilon †å GREEK SMALL LETTER EPSILON */
  { 0x07e6, 0x03b6 }, /*                  Greek_zeta †æ GREEK SMALL LETTER ZETA */
  { 0x07e7, 0x03b7 }, /*                   Greek_eta †ç GREEK SMALL LETTER ETA */
  { 0x07e8, 0x03b8 }, /*                 Greek_theta †è GREEK SMALL LETTER THETA */
  { 0x07e9, 0x03b9 }, /*                  Greek_iota †é GREEK SMALL LETTER IOTA */
  { 0x07ea, 0x03ba }, /*                 Greek_kappa †ê GREEK SMALL LETTER KAPPA */
  { 0x07eb, 0x03bb }, /*                Greek_lambda †ë GREEK SMALL LETTER LAMDA */
  { 0x07ec, 0x03bc }, /*                    Greek_mu †ì GREEK SMALL LETTER MU */
  { 0x07ed, 0x03bd }, /*                    Greek_nu †í GREEK SMALL LETTER NU */
  { 0x07ee, 0x03be }, /*                    Greek_xi †î GREEK SMALL LETTER XI */
  { 0x07ef, 0x03bf }, /*               Greek_omicron †ï GREEK SMALL LETTER OMICRON */
  { 0x07f0, 0x03c0 }, /*                    Greek_pi †ð GREEK SMALL LETTER PI */
  { 0x07f1, 0x03c1 }, /*                   Greek_rho †ñ GREEK SMALL LETTER RHO */
  { 0x07f2, 0x03c3 }, /*                 Greek_sigma †ó GREEK SMALL LETTER SIGMA */
  { 0x07f3, 0x03c2 }, /*       Greek_finalsmallsigma †ò GREEK SMALL LETTER FINAL SIGMA */
  { 0x07f4, 0x03c4 }, /*                   Greek_tau †ô GREEK SMALL LETTER TAU */
  { 0x07f5, 0x03c5 }, /*               Greek_upsilon †õ GREEK SMALL LETTER UPSILON */
  { 0x07f6, 0x03c6 }, /*                   Greek_phi †ö GREEK SMALL LETTER PHI */
  { 0x07f7, 0x03c7 }, /*                   Greek_chi †÷ GREEK SMALL LETTER CHI */
  { 0x07f8, 0x03c8 }, /*                   Greek_psi †ø GREEK SMALL LETTER PSI */
  { 0x07f9, 0x03c9 }, /*                 Greek_omega †ù GREEK SMALL LETTER OMEGA */
/*  0x08a1                               leftradical ? ??? */
/*  0x08a2                            topleftradical ? ??? */
/*  0x08a3                            horizconnector ? ??? */
  { 0x08a4, 0x2320 }, /*                 topintegral œôû  TOP HALF INTEGRAL */
  { 0x08a5, 0x2321 }, /*                 botintegral œôû¡ BOTTOM HALF INTEGRAL */
  { 0x08a6, 0x2502 }, /*               vertconnector ’¨¢ BOX DRAWINGS LIGHT VERTICAL */
/*  0x08a7                          topleftsqbracket ? ??? */
/*  0x08a8                          botleftsqbracket ? ??? */
/*  0x08a9                         toprightsqbracket ? ??? */
/*  0x08aa                         botrightsqbracket ? ??? */
/*  0x08ab                             topleftparens ? ??? */
/*  0x08ac                             botleftparens ? ??? */
/*  0x08ad                            toprightparens ? ??? */
/*  0x08ae                            botrightparens ? ??? */
/*  0x08af                      leftmiddlecurlybrace ? ??? */
/*  0x08b0                     rightmiddlecurlybrace ? ??? */
/*  0x08b1                          topleftsummation ? ??? */
/*  0x08b2                          botleftsummation ? ??? */
/*  0x08b3                 topvertsummationconnector ? ??? */
/*  0x08b4                 botvertsummationconnector ? ??? */
/*  0x08b5                         toprightsummation ? ??? */
/*  0x08b6                         botrightsummation ? ??? */
/*  0x08b7                      rightmiddlesummation ? ??? */
  { 0x08bc, 0x2264 }, /*               lessthanequal ‘¡Ü LESS-THAN OR EQUAL TO */
  { 0x08bd, 0x2260 }, /*                    notequal ’¡â NOT EQUAL TO */
  { 0x08be, 0x2265 }, /*            greaterthanequal ‘¡Ý GREATER-THAN OR EQUAL TO */
  { 0x08bf, 0x222b }, /*                    integral ’¢é INTEGRAL */
  { 0x08c0, 0x2234 }, /*                   therefore ’¡è THEREFORE */
  { 0x08c1, 0x221d }, /*                   variation ’¢ç PROPORTIONAL TO */
  { 0x08c2, 0x221e }, /*                    infinity ’¡ç INFINITY */
  { 0x08c5, 0x2207 }, /*                       nabla ’¢à NABLA */
  { 0x08c8, 0x2245 }, /*                 approximate —¢í APPROXIMATELY EQUAL TO */
/*  0x08c9                              similarequal ? ??? */
  { 0x08cd, 0x21d4 }, /*                    ifonlyif ’¢Î LEFT RIGHT DOUBLE ARROW */
  { 0x08ce, 0x21d2 }, /*                     implies ’¢Í RIGHTWARDS DOUBLE ARROW */
  { 0x08cf, 0x2261 }, /*                   identical ’¢á IDENTICAL TO */
  { 0x08d6, 0x221a }, /*                     radical ’¢å SQUARE ROOT */
  { 0x08da, 0x2282 }, /*                  includedin ’¢¾ SUBSET OF */
  { 0x08db, 0x2283 }, /*                    includes ’¢¿ SUPERSET OF */
  { 0x08dc, 0x2229 }, /*                intersection ’¢Á INTERSECTION */
  { 0x08dd, 0x222a }, /*                       union ’¢À UNION */
  { 0x08de, 0x2227 }, /*                  logicaland ’¢Ê LOGICAL AND */
  { 0x08df, 0x2228 }, /*                   logicalor ’¢Ë LOGICAL OR */
  { 0x08ef, 0x2202 }, /*           partialderivative ’¢ß PARTIAL DIFFERENTIAL */
  { 0x08f6, 0x0192 }, /*                    function œô¡Ò LATIN SMALL LETTER F WITH HOOK */
  { 0x08fb, 0x2190 }, /*                   leftarrow ’¢« LEFTWARDS ARROW */
  { 0x08fc, 0x2191 }, /*                     uparrow ’¢¬ UPWARDS ARROW */
  { 0x08fd, 0x2192 }, /*                  rightarrow ’¢ª RIGHTWARDS ARROW */
  { 0x08fe, 0x2193 }, /*                   downarrow ’¢­ DOWNWARDS ARROW */
  { 0x09df, 0x2422 }, /*                       blank œôýâ BLANK SYMBOL */
  { 0x09e0, 0x25c6 }, /*                soliddiamond ’¢¡ BLACK DIAMOND */
  { 0x09e1, 0x2592 }, /*                checkerboard “¢Æ MEDIUM SHADE */
  { 0x09e2, 0x2409 }, /*                          ht •Âª SYMBOL FOR HORIZONTAL TABULATION */
  { 0x09e3, 0x240c }, /*                          ff •Â­ SYMBOL FOR FORM FEED */
  { 0x09e4, 0x240d }, /*                          cr •Â® SYMBOL FOR CARRIAGE RETURN */
  { 0x09e5, 0x240a }, /*                          lf •Â« SYMBOL FOR LINE FEED */
  { 0x09e8, 0x2424 }, /*                          nl œôýä SYMBOL FOR NEWLINE */
  { 0x09e9, 0x240b }, /*                          vt •Â¬ SYMBOL FOR VERTICAL TABULATION */
  { 0x09ea, 0x2518 }, /*              lowrightcorner ’¨¥ BOX DRAWINGS LIGHT UP AND LEFT */
  { 0x09eb, 0x2510 }, /*               uprightcorner ’¨¤ BOX DRAWINGS LIGHT DOWN AND LEFT */
  { 0x09ec, 0x250c }, /*                upleftcorner ’¨£ BOX DRAWINGS LIGHT DOWN AND RIGHT */
  { 0x09ed, 0x2514 }, /*               lowleftcorner ’¨¦ BOX DRAWINGS LIGHT UP AND RIGHT */
  { 0x09ee, 0x253c }, /*               crossinglines ’¨« BOX DRAWINGS LIGHT VERTICAL AND HORIZONTAL */
/*  0x09ef                            horizlinescan1 ? ??? */
/*  0x09f0                            horizlinescan3 ? ??? */
  { 0x09f1, 0x2500 }, /*              horizlinescan5 ’¨¡ BOX DRAWINGS LIGHT HORIZONTAL */
/*  0x09f2                            horizlinescan7 ? ??? */
/*  0x09f3                            horizlinescan9 ? ??? */
  { 0x09f4, 0x251c }, /*                       leftt ’¨§ BOX DRAWINGS LIGHT VERTICAL AND RIGHT */
  { 0x09f5, 0x2524 }, /*                      rightt ’¨© BOX DRAWINGS LIGHT VERTICAL AND LEFT */
  { 0x09f6, 0x2534 }, /*                        bott ’¨ª BOX DRAWINGS LIGHT UP AND HORIZONTAL */
  { 0x09f7, 0x252c }, /*                        topt ’¨¨ BOX DRAWINGS LIGHT DOWN AND HORIZONTAL */
  { 0x09f8, 0x2502 }, /*                     vertbar ’¨¢ BOX DRAWINGS LIGHT VERTICAL */
  { 0x0aa1, 0x2003 }, /*                     emspace •¥í EM SPACE */
  { 0x0aa2, 0x2002 }, /*                     enspace œôòâ EN SPACE */
  { 0x0aa3, 0x2004 }, /*                    em3space œôòä THREE-PER-EM SPACE */
  { 0x0aa4, 0x2005 }, /*                    em4space œôòå FOUR-PER-EM SPACE */
  { 0x0aa5, 0x2007 }, /*                  digitspace œôòç FIGURE SPACE */
  { 0x0aa6, 0x2008 }, /*                  punctspace œôòè PUNCTUATION SPACE */
  { 0x0aa7, 0x2009 }, /*                   thinspace œôòé THIN SPACE */
  { 0x0aa8, 0x200a }, /*                   hairspace œôòê HAIR SPACE */
  { 0x0aa9, 0x2014 }, /*                      emdash •¡· EM DASH */
  { 0x0aaa, 0x2013 }, /*                      endash —£ü EN DASH */
/*  0x0aac                               signifblank ? ??? */
  { 0x0aae, 0x2026 }, /*                    ellipsis ’¡Ä HORIZONTAL ELLIPSIS */
/*  0x0aaf                           doubbaselinedot ? ??? */
  { 0x0ab0, 0x2153 }, /*                    onethird —§ø VULGAR FRACTION ONE THIRD */
  { 0x0ab1, 0x2154 }, /*                   twothirds —§ù VULGAR FRACTION TWO THIRDS */
  { 0x0ab2, 0x2155 }, /*                    onefifth —§ú VULGAR FRACTION ONE FIFTH */
  { 0x0ab3, 0x2156 }, /*                   twofifths œôö¶ VULGAR FRACTION TWO FIFTHS */
  { 0x0ab4, 0x2157 }, /*                 threefifths œôö· VULGAR FRACTION THREE FIFTHS */
  { 0x0ab5, 0x2158 }, /*                  fourfifths œôö¸ VULGAR FRACTION FOUR FIFTHS */
  { 0x0ab6, 0x2159 }, /*                    onesixth œôö¹ VULGAR FRACTION ONE SIXTH */
  { 0x0ab7, 0x215a }, /*                  fivesixths œôöº VULGAR FRACTION FIVE SIXTHS */
  { 0x0ab8, 0x2105 }, /*                      careof •¢¢ CARE OF */
  { 0x0abb, 0x2012 }, /*                     figdash œôòò FIGURE DASH */
  { 0x0abc, 0x2329 }, /*            leftanglebracket œôû© LEFT-POINTING ANGLE BRACKET */
  { 0x0abd, 0x002e }, /*                decimalpoint . FULL STOP */
  { 0x0abe, 0x232a }, /*           rightanglebracket œôûª RIGHT-POINTING ANGLE BRACKET */
/*  0x0abf                                    marker ? ??? */
  { 0x0ac3, 0x215b }, /*                   oneeighth “¨û VULGAR FRACTION ONE EIGHTH */
  { 0x0ac4, 0x215c }, /*                threeeighths “¨ü VULGAR FRACTION THREE EIGHTHS */
  { 0x0ac5, 0x215d }, /*                 fiveeighths “¨ý VULGAR FRACTION FIVE EIGHTHS */
  { 0x0ac6, 0x215e }, /*                seveneighths “¨þ VULGAR FRACTION SEVEN EIGHTHS */
  { 0x0ac9, 0x2122 }, /*                   trademark ”¢ï TRADE MARK SIGN */
  { 0x0aca, 0x2613 }, /*               signaturemark œò¢ó SALTIRE */
/*  0x0acb                         trademarkincircle ? ??? */
  { 0x0acc, 0x25c1 }, /*            leftopentriangle —££ WHITE LEFT-POINTING TRIANGLE */
  { 0x0acd, 0x25b7 }, /*           rightopentriangle —£¡ WHITE RIGHT-POINTING TRIANGLE */
  { 0x0ace, 0x25cb }, /*                emopencircle ’¡û WHITE CIRCLE */
  { 0x0acf, 0x25a1 }, /*             emopenrectangle ’¢¢ WHITE SQUARE */
  { 0x0ad0, 0x2018 }, /*         leftsinglequotemark †¡ LEFT SINGLE QUOTATION MARK */
  { 0x0ad1, 0x2019 }, /*        rightsinglequotemark †¢ RIGHT SINGLE QUOTATION MARK */
  { 0x0ad2, 0x201c }, /*         leftdoublequotemark ’¡È LEFT DOUBLE QUOTATION MARK */
  { 0x0ad3, 0x201d }, /*        rightdoublequotemark ’¡É RIGHT DOUBLE QUOTATION MARK */
  { 0x0ad4, 0x211e }, /*                prescription œôõÞ PRESCRIPTION TAKE */
  { 0x0ad6, 0x2032 }, /*                     minutes ’¡ì PRIME */
  { 0x0ad7, 0x2033 }, /*                     seconds ’¡í DOUBLE PRIME */
  { 0x0ad9, 0x271d }, /*                  latincross œò¥Ý LATIN CROSS */
/*  0x0ada                                  hexagram ? ??? */
  { 0x0adb, 0x25ac }, /*            filledrectbullet œò¡ì BLACK RECTANGLE */
  { 0x0adc, 0x25c0 }, /*         filledlefttribullet —£¤ BLACK LEFT-POINTING TRIANGLE */
  { 0x0add, 0x25b6 }, /*        filledrighttribullet —£¢ BLACK RIGHT-POINTING TRIANGLE */
  { 0x0ade, 0x25cf }, /*              emfilledcircle ’¡ü BLACK CIRCLE */
  { 0x0adf, 0x25a0 }, /*                emfilledrect ’¢£ BLACK SQUARE */
  { 0x0ae0, 0x25e6 }, /*            enopencircbullet —£¿ WHITE BULLET */
  { 0x0ae1, 0x25ab }, /*          enopensquarebullet œò¡ë WHITE SMALL SQUARE */
  { 0x0ae2, 0x25ad }, /*              openrectbullet œò¡í WHITE RECTANGLE */
  { 0x0ae3, 0x25b3 }, /*             opentribulletup ’¢¤ WHITE UP-POINTING TRIANGLE */
  { 0x0ae4, 0x25bd }, /*           opentribulletdown ’¢¦ WHITE DOWN-POINTING TRIANGLE */
  { 0x0ae5, 0x2606 }, /*                    openstar ’¡ù WHITE STAR */
  { 0x0ae6, 0x2022 }, /*          enfilledcircbullet —£À BULLET */
  { 0x0ae7, 0x25aa }, /*            enfilledsqbullet œò¡ê BLACK SMALL SQUARE */
  { 0x0ae8, 0x25b2 }, /*           filledtribulletup ’¢¥ BLACK UP-POINTING TRIANGLE */
  { 0x0ae9, 0x25bc }, /*         filledtribulletdown ’¢§ BLACK DOWN-POINTING TRIANGLE */
  { 0x0aea, 0x261c }, /*                 leftpointer “¢Ð WHITE LEFT POINTING INDEX */
  { 0x0aeb, 0x261e }, /*                rightpointer —­þ WHITE RIGHT POINTING INDEX */
  { 0x0aec, 0x2663 }, /*                        club —¦À BLACK CLUB SUIT */
  { 0x0aed, 0x2666 }, /*                     diamond —¦¼ BLACK DIAMOND SUIT */
  { 0x0aee, 0x2665 }, /*                       heart —¦¾ BLACK HEART SUIT */
  { 0x0af0, 0x2720 }, /*                maltesecross œò¥à MALTESE CROSS */
  { 0x0af1, 0x2020 }, /*                      dagger ’¢÷ DAGGER */
  { 0x0af2, 0x2021 }, /*                doubledagger ’¢ø DOUBLE DAGGER */
  { 0x0af3, 0x2713 }, /*                   checkmark —§û CHECK MARK */
  { 0x0af4, 0x2717 }, /*                 ballotcross œò¥× BALLOT X */
  { 0x0af5, 0x266f }, /*                musicalsharp ’¢ô MUSIC SHARP SIGN */
  { 0x0af6, 0x266d }, /*                 musicalflat ’¢õ MUSIC FLAT SIGN */
  { 0x0af7, 0x2642 }, /*                  malesymbol ’¡é MALE SIGN */
  { 0x0af8, 0x2640 }, /*                femalesymbol ’¡ê FEMALE SIGN */
  { 0x0af9, 0x260e }, /*                   telephone —¦ç BLACK TELEPHONE */
  { 0x0afa, 0x2315 }, /*           telephonerecorder œôúõ TELEPHONE RECORDER */
  { 0x0afb, 0x2117 }, /*         phonographcopyright œôõ× SOUND RECORDING COPYRIGHT */
  { 0x0afc, 0x2038 }, /*                       caret œôó¸ CARET */
  { 0x0afd, 0x201a }, /*          singlelowquotemark œôòú SINGLE LOW-9 QUOTATION MARK */
  { 0x0afe, 0x201e }, /*          doublelowquotemark œôòþ DOUBLE LOW-9 QUOTATION MARK */
/*  0x0aff                                    cursor ? ??? */
  { 0x0ba3, 0x003c }, /*                   leftcaret < LESS-THAN SIGN */
  { 0x0ba6, 0x003e }, /*                  rightcaret > GREATER-THAN SIGN */
  { 0x0ba8, 0x2228 }, /*                   downcaret ’¢Ë LOGICAL OR */
  { 0x0ba9, 0x2227 }, /*                     upcaret ’¢Ê LOGICAL AND */
  { 0x0bc0, 0x00af }, /*                     overbar ¯ MACRON */
  { 0x0bc2, 0x22a4 }, /*                    downtack œôùä DOWN TACK */
  { 0x0bc3, 0x2229 }, /*                      upshoe ’¢Á INTERSECTION */
  { 0x0bc4, 0x230a }, /*                   downstile œôúê LEFT FLOOR */
  { 0x0bc6, 0x005f }, /*                    underbar _ LOW LINE */
  { 0x0bca, 0x2218 }, /*                         jot œôø¸ RING OPERATOR */
  { 0x0bcc, 0x2395 }, /*                        quad œôüµ APL FUNCTIONAL SYMBOL QUAD (Unicode 3.0) */
  { 0x0bce, 0x22a5 }, /*                      uptack ’¢Ý UP TACK */
  { 0x0bcf, 0x25cb }, /*                      circle ’¡û WHITE CIRCLE */
  { 0x0bd3, 0x2308 }, /*                     upstile œôúè LEFT CEILING */
  { 0x0bd6, 0x222a }, /*                    downshoe ’¢À UNION */
  { 0x0bd8, 0x2283 }, /*                   rightshoe ’¢¿ SUPERSET OF */
  { 0x0bda, 0x2282 }, /*                    leftshoe ’¢¾ SUBSET OF */
  { 0x0bdc, 0x22a3 }, /*                    lefttack œôùã LEFT TACK */
  { 0x0bfc, 0x22a2 }, /*                   righttack œôùâ RIGHT TACK */
  { 0x0cdf, 0x2017 }, /*        hebrew_doublelowline ˆß DOUBLE LOW LINE */
  { 0x0ce0, 0x05d0 }, /*                hebrew_aleph ˆà HEBREW LETTER ALEF */
  { 0x0ce1, 0x05d1 }, /*                  hebrew_bet ˆá HEBREW LETTER BET */
  { 0x0ce2, 0x05d2 }, /*                hebrew_gimel ˆâ HEBREW LETTER GIMEL */
  { 0x0ce3, 0x05d3 }, /*                hebrew_dalet ˆã HEBREW LETTER DALET */
  { 0x0ce4, 0x05d4 }, /*                   hebrew_he ˆä HEBREW LETTER HE */
  { 0x0ce5, 0x05d5 }, /*                  hebrew_waw ˆå HEBREW LETTER VAV */
  { 0x0ce6, 0x05d6 }, /*                 hebrew_zain ˆæ HEBREW LETTER ZAYIN */
  { 0x0ce7, 0x05d7 }, /*                 hebrew_chet ˆç HEBREW LETTER HET */
  { 0x0ce8, 0x05d8 }, /*                  hebrew_tet ˆè HEBREW LETTER TET */
  { 0x0ce9, 0x05d9 }, /*                  hebrew_yod ˆé HEBREW LETTER YOD */
  { 0x0cea, 0x05da }, /*            hebrew_finalkaph ˆê HEBREW LETTER FINAL KAF */
  { 0x0ceb, 0x05db }, /*                 hebrew_kaph ˆë HEBREW LETTER KAF */
  { 0x0cec, 0x05dc }, /*                hebrew_lamed ˆì HEBREW LETTER LAMED */
  { 0x0ced, 0x05dd }, /*             hebrew_finalmem ˆí HEBREW LETTER FINAL MEM */
  { 0x0cee, 0x05de }, /*                  hebrew_mem ˆî HEBREW LETTER MEM */
  { 0x0cef, 0x05df }, /*             hebrew_finalnun ˆï HEBREW LETTER FINAL NUN */
  { 0x0cf0, 0x05e0 }, /*                  hebrew_nun ˆð HEBREW LETTER NUN */
  { 0x0cf1, 0x05e1 }, /*               hebrew_samech ˆñ HEBREW LETTER SAMEKH */
  { 0x0cf2, 0x05e2 }, /*                 hebrew_ayin ˆò HEBREW LETTER AYIN */
  { 0x0cf3, 0x05e3 }, /*              hebrew_finalpe ˆó HEBREW LETTER FINAL PE */
  { 0x0cf4, 0x05e4 }, /*                   hebrew_pe ˆô HEBREW LETTER PE */
  { 0x0cf5, 0x05e5 }, /*            hebrew_finalzade ˆõ HEBREW LETTER FINAL TSADI */
  { 0x0cf6, 0x05e6 }, /*                 hebrew_zade ˆö HEBREW LETTER TSADI */
  { 0x0cf7, 0x05e7 }, /*                 hebrew_qoph ˆ÷ HEBREW LETTER QOF */
  { 0x0cf8, 0x05e8 }, /*                 hebrew_resh ˆø HEBREW LETTER RESH */
  { 0x0cf9, 0x05e9 }, /*                 hebrew_shin ˆù HEBREW LETTER SHIN */
  { 0x0cfa, 0x05ea }, /*                  hebrew_taw ˆú HEBREW LETTER TAV */
  { 0x0da1, 0x0e01 }, /*                  Thai_kokai …¡ THAI CHARACTER KO KAI */
  { 0x0da2, 0x0e02 }, /*                Thai_khokhai …¢ THAI CHARACTER KHO KHAI */
  { 0x0da3, 0x0e03 }, /*               Thai_khokhuat …£ THAI CHARACTER KHO KHUAT */
  { 0x0da4, 0x0e04 }, /*               Thai_khokhwai …¤ THAI CHARACTER KHO KHWAI */
  { 0x0da5, 0x0e05 }, /*                Thai_khokhon …¥ THAI CHARACTER KHO KHON */
  { 0x0da6, 0x0e06 }, /*             Thai_khorakhang …¦ THAI CHARACTER KHO RAKHANG */
  { 0x0da7, 0x0e07 }, /*                 Thai_ngongu …§ THAI CHARACTER NGO NGU */
  { 0x0da8, 0x0e08 }, /*                Thai_chochan …¨ THAI CHARACTER CHO CHAN */
  { 0x0da9, 0x0e09 }, /*               Thai_choching …© THAI CHARACTER CHO CHING */
  { 0x0daa, 0x0e0a }, /*               Thai_chochang …ª THAI CHARACTER CHO CHANG */
  { 0x0dab, 0x0e0b }, /*                   Thai_soso …« THAI CHARACTER SO SO */
  { 0x0dac, 0x0e0c }, /*                Thai_chochoe …¬ THAI CHARACTER CHO CHOE */
  { 0x0dad, 0x0e0d }, /*                 Thai_yoying …­ THAI CHARACTER YO YING */
  { 0x0dae, 0x0e0e }, /*                Thai_dochada …® THAI CHARACTER DO CHADA */
  { 0x0daf, 0x0e0f }, /*                Thai_topatak …¯ THAI CHARACTER TO PATAK */
  { 0x0db0, 0x0e10 }, /*                Thai_thothan …° THAI CHARACTER THO THAN */
  { 0x0db1, 0x0e11 }, /*          Thai_thonangmontho …± THAI CHARACTER THO NANGMONTHO */
  { 0x0db2, 0x0e12 }, /*             Thai_thophuthao …² THAI CHARACTER THO PHUTHAO */
  { 0x0db3, 0x0e13 }, /*                  Thai_nonen …³ THAI CHARACTER NO NEN */
  { 0x0db4, 0x0e14 }, /*                  Thai_dodek …´ THAI CHARACTER DO DEK */
  { 0x0db5, 0x0e15 }, /*                  Thai_totao …µ THAI CHARACTER TO TAO */
  { 0x0db6, 0x0e16 }, /*               Thai_thothung …¶ THAI CHARACTER THO THUNG */
  { 0x0db7, 0x0e17 }, /*              Thai_thothahan …· THAI CHARACTER THO THAHAN */
  { 0x0db8, 0x0e18 }, /*               Thai_thothong …¸ THAI CHARACTER THO THONG */
  { 0x0db9, 0x0e19 }, /*                   Thai_nonu …¹ THAI CHARACTER NO NU */
  { 0x0dba, 0x0e1a }, /*               Thai_bobaimai …º THAI CHARACTER BO BAIMAI */
  { 0x0dbb, 0x0e1b }, /*                  Thai_popla …» THAI CHARACTER PO PLA */
  { 0x0dbc, 0x0e1c }, /*               Thai_phophung …¼ THAI CHARACTER PHO PHUNG */
  { 0x0dbd, 0x0e1d }, /*                   Thai_fofa …½ THAI CHARACTER FO FA */
  { 0x0dbe, 0x0e1e }, /*                Thai_phophan …¾ THAI CHARACTER PHO PHAN */
  { 0x0dbf, 0x0e1f }, /*                  Thai_fofan …¿ THAI CHARACTER FO FAN */
  { 0x0dc0, 0x0e20 }, /*             Thai_phosamphao …À THAI CHARACTER PHO SAMPHAO */
  { 0x0dc1, 0x0e21 }, /*                   Thai_moma …Á THAI CHARACTER MO MA */
  { 0x0dc2, 0x0e22 }, /*                  Thai_yoyak …Â THAI CHARACTER YO YAK */
  { 0x0dc3, 0x0e23 }, /*                  Thai_rorua …Ã THAI CHARACTER RO RUA */
  { 0x0dc4, 0x0e24 }, /*                     Thai_ru …Ä THAI CHARACTER RU */
  { 0x0dc5, 0x0e25 }, /*                 Thai_loling …Å THAI CHARACTER LO LING */
  { 0x0dc6, 0x0e26 }, /*                     Thai_lu …Æ THAI CHARACTER LU */
  { 0x0dc7, 0x0e27 }, /*                 Thai_wowaen …Ç THAI CHARACTER WO WAEN */
  { 0x0dc8, 0x0e28 }, /*                 Thai_sosala …È THAI CHARACTER SO SALA */
  { 0x0dc9, 0x0e29 }, /*                 Thai_sorusi …É THAI CHARACTER SO RUSI */
  { 0x0dca, 0x0e2a }, /*                  Thai_sosua …Ê THAI CHARACTER SO SUA */
  { 0x0dcb, 0x0e2b }, /*                  Thai_hohip …Ë THAI CHARACTER HO HIP */
  { 0x0dcc, 0x0e2c }, /*                Thai_lochula …Ì THAI CHARACTER LO CHULA */
  { 0x0dcd, 0x0e2d }, /*                   Thai_oang …Í THAI CHARACTER O ANG */
  { 0x0dce, 0x0e2e }, /*               Thai_honokhuk …Î THAI CHARACTER HO NOKHUK */
  { 0x0dcf, 0x0e2f }, /*              Thai_paiyannoi …Ï THAI CHARACTER PAIYANNOI */
  { 0x0dd0, 0x0e30 }, /*                  Thai_saraa …Ð THAI CHARACTER SARA A */
  { 0x0dd1, 0x0e31 }, /*             Thai_maihanakat …Ñ THAI CHARACTER MAI HAN-AKAT */
  { 0x0dd2, 0x0e32 }, /*                 Thai_saraaa …Ò THAI CHARACTER SARA AA */
  { 0x0dd3, 0x0e33 }, /*                 Thai_saraam …Ó THAI CHARACTER SARA AM */
  { 0x0dd4, 0x0e34 }, /*                  Thai_sarai …Ô THAI CHARACTER SARA I */
  { 0x0dd5, 0x0e35 }, /*                 Thai_saraii …Õ THAI CHARACTER SARA II */
  { 0x0dd6, 0x0e36 }, /*                 Thai_saraue …Ö THAI CHARACTER SARA UE */
  { 0x0dd7, 0x0e37 }, /*                Thai_sarauee …× THAI CHARACTER SARA UEE */
  { 0x0dd8, 0x0e38 }, /*                  Thai_sarau …Ø THAI CHARACTER SARA U */
  { 0x0dd9, 0x0e39 }, /*                 Thai_sarauu …Ù THAI CHARACTER SARA UU */
  { 0x0dda, 0x0e3a }, /*                Thai_phinthu …Ú THAI CHARACTER PHINTHU */
  { 0x0dde, 0x0e3e }, /*      Thai_maihanakat_maitho œôÃ¾ ??? */
  { 0x0ddf, 0x0e3f }, /*                   Thai_baht …ß THAI CURRENCY SYMBOL BAHT */
  { 0x0de0, 0x0e40 }, /*                  Thai_sarae …à THAI CHARACTER SARA E */
  { 0x0de1, 0x0e41 }, /*                 Thai_saraae …á THAI CHARACTER SARA AE */
  { 0x0de2, 0x0e42 }, /*                  Thai_sarao …â THAI CHARACTER SARA O */
  { 0x0de3, 0x0e43 }, /*          Thai_saraaimaimuan …ã THAI CHARACTER SARA AI MAIMUAN */
  { 0x0de4, 0x0e44 }, /*         Thai_saraaimaimalai …ä THAI CHARACTER SARA AI MAIMALAI */
  { 0x0de5, 0x0e45 }, /*            Thai_lakkhangyao …å THAI CHARACTER LAKKHANGYAO */
  { 0x0de6, 0x0e46 }, /*               Thai_maiyamok …æ THAI CHARACTER MAIYAMOK */
  { 0x0de7, 0x0e47 }, /*              Thai_maitaikhu …ç THAI CHARACTER MAITAIKHU */
  { 0x0de8, 0x0e48 }, /*                  Thai_maiek …è THAI CHARACTER MAI EK */
  { 0x0de9, 0x0e49 }, /*                 Thai_maitho …é THAI CHARACTER MAI THO */
  { 0x0dea, 0x0e4a }, /*                 Thai_maitri …ê THAI CHARACTER MAI TRI */
  { 0x0deb, 0x0e4b }, /*            Thai_maichattawa …ë THAI CHARACTER MAI CHATTAWA */
  { 0x0dec, 0x0e4c }, /*            Thai_thanthakhat …ì THAI CHARACTER THANTHAKHAT */
  { 0x0ded, 0x0e4d }, /*               Thai_nikhahit …í THAI CHARACTER NIKHAHIT */
  { 0x0df0, 0x0e50 }, /*                 Thai_leksun …ð THAI DIGIT ZERO */
  { 0x0df1, 0x0e51 }, /*                Thai_leknung …ñ THAI DIGIT ONE */
  { 0x0df2, 0x0e52 }, /*                Thai_leksong …ò THAI DIGIT TWO */
  { 0x0df3, 0x0e53 }, /*                 Thai_leksam …ó THAI DIGIT THREE */
  { 0x0df4, 0x0e54 }, /*                  Thai_leksi …ô THAI DIGIT FOUR */
  { 0x0df5, 0x0e55 }, /*                  Thai_lekha …õ THAI DIGIT FIVE */
  { 0x0df6, 0x0e56 }, /*                 Thai_lekhok …ö THAI DIGIT SIX */
  { 0x0df7, 0x0e57 }, /*                Thai_lekchet …÷ THAI DIGIT SEVEN */
  { 0x0df8, 0x0e58 }, /*                Thai_lekpaet …ø THAI DIGIT EIGHT */
  { 0x0df9, 0x0e59 }, /*                 Thai_lekkao …ù THAI DIGIT NINE */
  { 0x0ea1, 0x3131 }, /*               Hangul_Kiyeog “¤¡ HANGUL LETTER KIYEOK */
  { 0x0ea2, 0x3132 }, /*          Hangul_SsangKiyeog “¤¢ HANGUL LETTER SSANGKIYEOK */
  { 0x0ea3, 0x3133 }, /*           Hangul_KiyeogSios “¤£ HANGUL LETTER KIYEOK-SIOS */
  { 0x0ea4, 0x3134 }, /*                Hangul_Nieun “¤¤ HANGUL LETTER NIEUN */
  { 0x0ea5, 0x3135 }, /*           Hangul_NieunJieuj “¤¥ HANGUL LETTER NIEUN-CIEUC */
  { 0x0ea6, 0x3136 }, /*           Hangul_NieunHieuh “¤¦ HANGUL LETTER NIEUN-HIEUH */
  { 0x0ea7, 0x3137 }, /*               Hangul_Dikeud “¤§ HANGUL LETTER TIKEUT */
  { 0x0ea8, 0x3138 }, /*          Hangul_SsangDikeud “¤¨ HANGUL LETTER SSANGTIKEUT */
  { 0x0ea9, 0x3139 }, /*                Hangul_Rieul “¤© HANGUL LETTER RIEUL */
  { 0x0eaa, 0x313a }, /*          Hangul_RieulKiyeog “¤ª HANGUL LETTER RIEUL-KIYEOK */
  { 0x0eab, 0x313b }, /*           Hangul_RieulMieum “¤« HANGUL LETTER RIEUL-MIEUM */
  { 0x0eac, 0x313c }, /*           Hangul_RieulPieub “¤¬ HANGUL LETTER RIEUL-PIEUP */
  { 0x0ead, 0x313d }, /*            Hangul_RieulSios “¤­ HANGUL LETTER RIEUL-SIOS */
  { 0x0eae, 0x313e }, /*           Hangul_RieulTieut “¤® HANGUL LETTER RIEUL-THIEUTH */
  { 0x0eaf, 0x313f }, /*          Hangul_RieulPhieuf “¤¯ HANGUL LETTER RIEUL-PHIEUPH */
  { 0x0eb0, 0x3140 }, /*           Hangul_RieulHieuh “¤° HANGUL LETTER RIEUL-HIEUH */
  { 0x0eb1, 0x3141 }, /*                Hangul_Mieum “¤± HANGUL LETTER MIEUM */
  { 0x0eb2, 0x3142 }, /*                Hangul_Pieub “¤² HANGUL LETTER PIEUP */
  { 0x0eb3, 0x3143 }, /*           Hangul_SsangPieub “¤³ HANGUL LETTER SSANGPIEUP */
  { 0x0eb4, 0x3144 }, /*            Hangul_PieubSios “¤´ HANGUL LETTER PIEUP-SIOS */
  { 0x0eb5, 0x3145 }, /*                 Hangul_Sios “¤µ HANGUL LETTER SIOS */
  { 0x0eb6, 0x3146 }, /*            Hangul_SsangSios “¤¶ HANGUL LETTER SSANGSIOS */
  { 0x0eb7, 0x3147 }, /*                Hangul_Ieung “¤· HANGUL LETTER IEUNG */
  { 0x0eb8, 0x3148 }, /*                Hangul_Jieuj “¤¸ HANGUL LETTER CIEUC */
  { 0x0eb9, 0x3149 }, /*           Hangul_SsangJieuj “¤¹ HANGUL LETTER SSANGCIEUC */
  { 0x0eba, 0x314a }, /*                Hangul_Cieuc “¤º HANGUL LETTER CHIEUCH */
  { 0x0ebb, 0x314b }, /*               Hangul_Khieuq “¤» HANGUL LETTER KHIEUKH */
  { 0x0ebc, 0x314c }, /*                Hangul_Tieut “¤¼ HANGUL LETTER THIEUTH */
  { 0x0ebd, 0x314d }, /*               Hangul_Phieuf “¤½ HANGUL LETTER PHIEUPH */
  { 0x0ebe, 0x314e }, /*                Hangul_Hieuh “¤¾ HANGUL LETTER HIEUH */
  { 0x0ebf, 0x314f }, /*                    Hangul_A “¤¿ HANGUL LETTER A */
  { 0x0ec0, 0x3150 }, /*                   Hangul_AE “¤À HANGUL LETTER AE */
  { 0x0ec1, 0x3151 }, /*                   Hangul_YA “¤Á HANGUL LETTER YA */
  { 0x0ec2, 0x3152 }, /*                  Hangul_YAE “¤Â HANGUL LETTER YAE */
  { 0x0ec3, 0x3153 }, /*                   Hangul_EO “¤Ã HANGUL LETTER EO */
  { 0x0ec4, 0x3154 }, /*                    Hangul_E “¤Ä HANGUL LETTER E */
  { 0x0ec5, 0x3155 }, /*                  Hangul_YEO “¤Å HANGUL LETTER YEO */
  { 0x0ec6, 0x3156 }, /*                   Hangul_YE “¤Æ HANGUL LETTER YE */
  { 0x0ec7, 0x3157 }, /*                    Hangul_O “¤Ç HANGUL LETTER O */
  { 0x0ec8, 0x3158 }, /*                   Hangul_WA “¤È HANGUL LETTER WA */
  { 0x0ec9, 0x3159 }, /*                  Hangul_WAE “¤É HANGUL LETTER WAE */
  { 0x0eca, 0x315a }, /*                   Hangul_OE “¤Ê HANGUL LETTER OE */
  { 0x0ecb, 0x315b }, /*                   Hangul_YO “¤Ë HANGUL LETTER YO */
  { 0x0ecc, 0x315c }, /*                    Hangul_U “¤Ì HANGUL LETTER U */
  { 0x0ecd, 0x315d }, /*                  Hangul_WEO “¤Í HANGUL LETTER WEO */
  { 0x0ece, 0x315e }, /*                   Hangul_WE “¤Î HANGUL LETTER WE */
  { 0x0ecf, 0x315f }, /*                   Hangul_WI “¤Ï HANGUL LETTER WI */
  { 0x0ed0, 0x3160 }, /*                   Hangul_YU “¤Ð HANGUL LETTER YU */
  { 0x0ed1, 0x3161 }, /*                   Hangul_EU “¤Ñ HANGUL LETTER EU */
  { 0x0ed2, 0x3162 }, /*                   Hangul_YI “¤Ò HANGUL LETTER YI */
  { 0x0ed3, 0x3163 }, /*                    Hangul_I “¤Ó HANGUL LETTER I */
  { 0x0ed4, 0x11a8 }, /*             Hangul_J_Kiyeog œôÌÈ HANGUL JONGSEONG KIYEOK */
  { 0x0ed5, 0x11a9 }, /*        Hangul_J_SsangKiyeog œôÌÉ HANGUL JONGSEONG SSANGKIYEOK */
  { 0x0ed6, 0x11aa }, /*         Hangul_J_KiyeogSios œôÌÊ HANGUL JONGSEONG KIYEOK-SIOS */
  { 0x0ed7, 0x11ab }, /*              Hangul_J_Nieun œôÌË HANGUL JONGSEONG NIEUN */
  { 0x0ed8, 0x11ac }, /*         Hangul_J_NieunJieuj œôÌÌ HANGUL JONGSEONG NIEUN-CIEUC */
  { 0x0ed9, 0x11ad }, /*         Hangul_J_NieunHieuh œôÌÍ HANGUL JONGSEONG NIEUN-HIEUH */
  { 0x0eda, 0x11ae }, /*             Hangul_J_Dikeud œôÌÎ HANGUL JONGSEONG TIKEUT */
  { 0x0edb, 0x11af }, /*              Hangul_J_Rieul œôÌÏ HANGUL JONGSEONG RIEUL */
  { 0x0edc, 0x11b0 }, /*        Hangul_J_RieulKiyeog œôÌÐ HANGUL JONGSEONG RIEUL-KIYEOK */
  { 0x0edd, 0x11b1 }, /*         Hangul_J_RieulMieum œôÌÑ HANGUL JONGSEONG RIEUL-MIEUM */
  { 0x0ede, 0x11b2 }, /*         Hangul_J_RieulPieub œôÌÒ HANGUL JONGSEONG RIEUL-PIEUP */
  { 0x0edf, 0x11b3 }, /*          Hangul_J_RieulSios œôÌÓ HANGUL JONGSEONG RIEUL-SIOS */
  { 0x0ee0, 0x11b4 }, /*         Hangul_J_RieulTieut œôÌÔ HANGUL JONGSEONG RIEUL-THIEUTH */
  { 0x0ee1, 0x11b5 }, /*        Hangul_J_RieulPhieuf œôÌÕ HANGUL JONGSEONG RIEUL-PHIEUPH */
  { 0x0ee2, 0x11b6 }, /*         Hangul_J_RieulHieuh œôÌÖ HANGUL JONGSEONG RIEUL-HIEUH */
  { 0x0ee3, 0x11b7 }, /*              Hangul_J_Mieum œôÌ× HANGUL JONGSEONG MIEUM */
  { 0x0ee4, 0x11b8 }, /*              Hangul_J_Pieub œôÌØ HANGUL JONGSEONG PIEUP */
  { 0x0ee5, 0x11b9 }, /*          Hangul_J_PieubSios œôÌÙ HANGUL JONGSEONG PIEUP-SIOS */
  { 0x0ee6, 0x11ba }, /*               Hangul_J_Sios œôÌÚ HANGUL JONGSEONG SIOS */
  { 0x0ee7, 0x11bb }, /*          Hangul_J_SsangSios œôÌÛ HANGUL JONGSEONG SSANGSIOS */
  { 0x0ee8, 0x11bc }, /*              Hangul_J_Ieung œôÌÜ HANGUL JONGSEONG IEUNG */
  { 0x0ee9, 0x11bd }, /*              Hangul_J_Jieuj œôÌÝ HANGUL JONGSEONG CIEUC */
  { 0x0eea, 0x11be }, /*              Hangul_J_Cieuc œôÌÞ HANGUL JONGSEONG CHIEUCH */
  { 0x0eeb, 0x11bf }, /*             Hangul_J_Khieuq œôÌß HANGUL JONGSEONG KHIEUKH */
  { 0x0eec, 0x11c0 }, /*              Hangul_J_Tieut œôÌà HANGUL JONGSEONG THIEUTH */
  { 0x0eed, 0x11c1 }, /*             Hangul_J_Phieuf œôÌá HANGUL JONGSEONG PHIEUPH */
  { 0x0eee, 0x11c2 }, /*              Hangul_J_Hieuh œôÌâ HANGUL JONGSEONG HIEUH */
  { 0x0eef, 0x316d }, /*     Hangul_RieulYeorinHieuh “¤Ý HANGUL LETTER RIEUL-YEORINHIEUH */
  { 0x0ef0, 0x3171 }, /*    Hangul_SunkyeongeumMieum “¤á HANGUL LETTER KAPYEOUNMIEUM */
  { 0x0ef1, 0x3178 }, /*    Hangul_SunkyeongeumPieub “¤è HANGUL LETTER KAPYEOUNPIEUP */
  { 0x0ef2, 0x317f }, /*              Hangul_PanSios “¤ï HANGUL LETTER PANSIOS */
/*  0x0ef3                  Hangul_KkogjiDalrinIeung ? ??? */
  { 0x0ef4, 0x3184 }, /*   Hangul_SunkyeongeumPhieuf “¤ô HANGUL LETTER KAPYEOUNPHIEUPH */
  { 0x0ef5, 0x3186 }, /*          Hangul_YeorinHieuh “¤ö HANGUL LETTER YEORINHIEUH */
  { 0x0ef6, 0x318d }, /*                Hangul_AraeA “¤ý HANGUL LETTER ARAEA */
  { 0x0ef7, 0x318e }, /*               Hangul_AraeAE “¤þ HANGUL LETTER ARAEAE */
  { 0x0ef8, 0x11eb }, /*            Hangul_J_PanSios œôÍ« HANGUL JONGSEONG PANSIOS */
/*  0x0ef9                Hangul_J_KkogjiDalrinIeung ? ??? */
  { 0x0efa, 0x11f9 }, /*        Hangul_J_YeorinHieuh œôÍ¹ HANGUL JONGSEONG YEORINHIEUH */
  { 0x0eff, 0x20a9 }, /*                  Korean_Won œôôÉ WON SIGN */
  { 0x13bc, 0x0152 }, /*                          OE Ž¼ LATIN CAPITAL LIGATURE OE */
  { 0x13bd, 0x0153 }, /*                          oe Ž½ LATIN SMALL LIGATURE OE */
  { 0x13be, 0x0178 }, /*                  Ydiaeresis ¯ LATIN CAPITAL LETTER Y WITH DIAERESIS */
  { 0x20a0, 0x20a0 }, /*                     EcuSign œôôÀ EURO-CURRENCY SIGN */
  { 0x20a1, 0x20a1 }, /*                   ColonSign œôôÁ COLON SIGN */
  { 0x20a2, 0x20a2 }, /*                CruzeiroSign œôôÂ CRUZEIRO SIGN */
  { 0x20a3, 0x20a3 }, /*                  FFrancSign œôôÃ FRENCH FRANC SIGN */
  { 0x20a4, 0x20a4 }, /*                    LiraSign œôôÄ LIRA SIGN */
  { 0x20a5, 0x20a5 }, /*                    MillSign œôôÅ MILL SIGN */
  { 0x20a6, 0x20a6 }, /*                   NairaSign œôôÆ NAIRA SIGN */
  { 0x20a7, 0x20a7 }, /*                  PesetaSign œôôÇ PESETA SIGN */
  { 0x20a8, 0x20a8 }, /*                   RupeeSign œôôÈ RUPEE SIGN */
  { 0x20a9, 0x20a9 }, /*                     WonSign œôôÉ WON SIGN */
  { 0x20aa, 0x20aa }, /*               NewSheqelSign œôôÊ NEW SHEQEL SIGN */
  { 0x20ab, 0x20ab }, /*                    DongSign œôôË DONG SIGN */
  { 0x20ac, 0x20ac }, /*                    EuroSign Ž¤ EURO SIGN */


  /* Following items added to GTK, not in the xterm table */

  /* Numeric keypad */
  
  { 0xFF80 /* Space */, ' ' },
  { 0xFF89 /* Tab */, '\t' },
  { 0xFF8D /* Enter */, '\n' },
  { 0xFFAA /* Multiply */, '*' },
  { 0xFFAB /* Add */, '+' },
  { 0xFFAD /* Subtract */, '-' },
  { 0xFFAE /* Decimal */, '.' },
  { 0xFFAF /* Divide */, '/' },
  { 0xFFB0 /* 0 */, '0' },
  { 0xFFB1 /* 1 */, '1' },
  { 0xFFB2 /* 2 */, '2' },
  { 0xFFB3 /* 3 */, '3' },
  { 0xFFB4 /* 4 */, '4' },
  { 0xFFB5 /* 5 */, '5' },
  { 0xFFB6 /* 6 */, '6' },
  { 0xFFB7 /* 7 */, '7' },
  { 0xFFB8 /* 8 */, '8' },
  { 0xFFB9 /* 9 */, '9' },
  { 0xFFBD /* Equal */, '=' },  

  /* End numeric keypad */
};

/* ported from GTK+1.3:
 * gdk_keyval_to_unicode:
 * @keyval: a GDK key symbol
 *
 * Convert from a GDK key symbol to the corresponding ISO10646 (Unicode)
 * character.
 *
 * Return value: the corresponding unicode character, or 0 if there
 *               is no corresponding character.
 */
unichar
charconv_keyval_to_unicode (guint keysym)
{
	int min = 0;
	int max = sizeof (gdk_keysym_to_unicode_tab) / sizeof (gdk_keysym_to_unicode_tab[0]) - 1;
	int mid;

	/* First check for Latin-1 characters (1:1 mapping) */
	if ((keysym >= 0x0020 && keysym <= 0x007e) ||
		(keysym >= 0x00a0 && keysym <= 0x00ff))
		return keysym;

	/* Also check for directly encoded 24-bit UCS characters */
	if ((keysym & 0xff000000) == 0x0100000000)
		return keysym & 0x00ffffff;

	/* binary search in table */
	while (max >= min) {
		mid = (min + max) / 2;
		if (gdk_keysym_to_unicode_tab[mid].keysym < keysym)
			min = mid + 1;
		else if (gdk_keysym_to_unicode_tab[mid].keysym > keysym)
			max = mid - 1;
		else {
			/* found it */
			return gdk_keysym_to_unicode_tab[mid].ucs;
		}
	}

	/* No matching Unicode value found */
	return 0;
}

utfchar *
charconv_utf8_from_gtk_event_key (guint keyval, gchar *string)
{
	utfchar *utf;
	unichar unival;

	if (keyval == GDK_VoidSymbol) {
		utf = charconv_local8_to_utf8 (string);
	} else {
		unival = charconv_keyval_to_unicode (keyval);

		if (unival < ' ') return NULL;
		utf = g_strdup (charconv_unichar_to_utf8 (unival));
	}

	return utf;
}

/* Two helper functions for manipulating gtk_entry */
void
charconv_gtk_entry_set_text(GtkEntry *entry, utfchar *text) {
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
  utfchar *utftext;
  utftext = charconv_utf8_to_local8(text);
  gtk_entry_set_text(entry, utftext);
  g_free(utftext);
#else
  gtk_entry_set_text(entry, text);
#endif
}

utfchar *
charconv_gtk_editable_get_chars(GtkEditable *entry, int start, int end) {
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
  utfchar *utftext;
  gchar *text;

  text = gtk_editable_get_chars(entry, start, end);
  utftext = charconv_local8_to_utf8(text);
  g_free(text);
  return utftext;
#else
  return gtk_editable_get_chars(entry, start, end);
#endif
}
