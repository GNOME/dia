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
#include "charconv.h"

#ifdef HAVE_UNICODE
#include <string.h>
#include <unicode.h>

#ifndef EFAULT_8BIT_CHARSET /* this is a hack until dia talks utf8 internally. */
#define EFAULT_8BIT_CHARSET "ISO-8859-1" /* this whole file is, anyway. */
#endif

static unicode_iconv_t conv_u2l = (unicode_iconv_t)0;
static unicode_iconv_t conv_l2u = (unicode_iconv_t)0;

static void 
check_conv_l2u(void){
  if (conv_u2l==(unicode_iconv_t)0) {
    conv_u2l = unicode_iconv_open(EFAULT_8BIT_CHARSET,"UTF-8");
  } 
  if (conv_l2u==(unicode_iconv_t)0) {
    conv_l2u = unicode_iconv_open("UTF-8",EFAULT_8BIT_CHARSET);
  } 
}

static void 
check_conv_u2l(void){
  if (conv_u2l==(unicode_iconv_t)0) {
    conv_u2l = unicode_iconv_open(EFAULT_8BIT_CHARSET,"UTF-8");
  } 
}
  

extern gchar *
charconv_local8_to_utf8(const gchar *local)
{
  const gchar *l = local;
  int lleft = strlen(local);
  gchar *utf,*u,*ures;
  int uleft;
  int lost = 0;

  check_conv_l2u();
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
charconv_utf8_to_local8(const gchar *utf)
{
  const gchar *u = utf;
  int uleft = strlen(utf);
  gchar *local,*l,*lres;
  int lleft;
  int lost = 0;

  check_conv_u2l();
  lleft = uleft;
  l = local = g_malloc(lleft);
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


#else /* !HAVE_UNICODE */

extern gchar *
charconv_local8_to_utf8(const gchar *local)
{
  return g_strdup(local);
}

extern gchar *
charconv_utf8_to_local8(const gchar *utf)
{
  return g_strdup(utf);
}

#endif

