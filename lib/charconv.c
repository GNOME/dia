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
  *charset = NULL;
#endif
  if (local_is_utf8) {
      this_charset = *charset;
      return local_is_utf8;
  }
  if (*charset) {
      
      if ((0==strcmp(*charset,"US-ASCII")) ||
          (0==strcmp(*charset,"ANSI_X3.4-1968"))) {
              /* unicode_get_charset() is broken. Usually, when it says
                 US-ASCII, it's something else. */
          *charset = nl_langinfo(CODESET);

          if ((!charset) ||
              (0==strcmp(*charset,"US-ASCII")) ||
              (0==strcmp(*charset,"ANSI_X3.4-1968"))) {
                  /* we got basic stupid ASCII here. We use its sane
                     superset instead. Especially since libxml2 doesn't like
                     the pedantic name of ASCII. */
              *charset = "UTF-8";
          }
      }
  }
  
  this_charset = *charset;
  local_is_utf8 = (0==strcmp(*charset,"UTF-8"));
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
      g_warning("unicode_iconv(l2u,...) failed, because '%s'",
                strerror(errno));
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
      g_warning("unicode_iconv(u2l,...) failed, because '%s'",
                strerror(errno));
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


#else /* !HAVE_UNICODE */

extern utfchar *
charconv_local8_to_utf8(const gchar *local)
{
  return g_strdup(local);
}

extern gchar *
charconv_utf8_to_local8(const utfchar *utf)
{
  return g_strdup(utf);
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

#endif

