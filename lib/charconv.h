/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * charconv.h: simple wrapper around unicode_iconv until we get dia to use
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

#ifndef CHARCONV_H
#include <glib.h>

int get_local_charset(char **charset);

/* The following code is for the case we don't HAVE_UNICODE. We'll have to 
   define the same-looking macros around libunicode, but only when that's 
   actually expected to work :-) */
#ifndef UNICODE_WORK_IN_PROGRESS
#include <string.h>

typedef gchar unichar; /* this would have been a wide char */
typedef gchar utfchar; /* to be used as in "const utfchar *" */

#define uni_isalnum isalnum
#define uni_isalpha isalpha
#define uni_iscntrl iscntrl
#define uni_isdigit isdigit
#define uni_isgraph isgraph
#define uni_islower islower
#define uni_isprint isprint
#define uni_ispunct ispunct
#define uni_isspace isspace
#define uni_isupper isupper
#define uni_isxdigit isxdigit
#define uni_toupper toupper
#define uni_tolower tolower

G_INLINE_FUNC utfchar *uni_previous(const utfchar *start, 
                                    const utfchar *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC utfchar *uni_previous(const utfchar *start, 
                                    const utfchar *p)
{
  utfchar *pp = (utfchar *)(p-1);
  if (pp < start) return NULL;
  return (utfchar *)p;
}
#endif

G_INLINE_FUNC utfchar *uni_next(const utfchar *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC utfchar *uni_next(const utfchar *p)
{
  return (utfchar *)(p+1);
}
#endif

G_INLINE_FUNC int uni_strlen(const utfchar *p, int max);
#ifdef G_CAN_INLINE
G_INLINE_FUNC int uni_strlen(const utfchar *p, int max)
{
  int sz = strlen(p);
  if (max < 0) return sz;
  if (max < sz) return max;
  return sz;
}
#endif

G_INLINE_FUNC int uni_string_width(const utfchar *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC int uni_string_width(const utfchar *p)
{
  return strlen(p);
}
#endif

G_INLINE_FUNC utfchar *uni_get_utf8(const utfchar *p, unichar *result);
#ifdef G_CAN_INLINE
G_INLINE_FUNC utfchar *uni_get_utf8(const utfchar *p, unichar *result)
{
  *result = *p;
  return (utfchar *)(p+1);
}
#endif

G_INLINE_FUNC size_t uni_offset_to_index(const utfchar *p, int offset);
#ifdef G_CAN_INLINE
G_INLINE_FUNC size_t uni_offset_to_index(const utfchar *p, int offset)
{
  return (size_t) offset;
}
#endif

G_INLINE_FUNC size_t uni_index_to_offset(const utfchar *p, int offset);
#ifdef G_CAN_INLINE
G_INLINE_FUNC size_t uni_index_to_offset(const utfchar *p, int offset)
{
  return (size_t) offset;
}
#endif

G_INLINE_FUNC char *uni_last_utf8(const char *p);
#ifdef G_CAN_INLINE
G_INLINE_FUNC char *uni_last_utf8(const char *p) {
  char *pp = (char *)p;
  while (*pp) pp++;
  return --pp;
}
#endif

#define uni_strchr strchr
#define uni_strrchr strrchr
#define uni_strncpy strncpy

#else

#include <unicode.h>
typedef unicode_char_t unichar;
typedef gchar utfchar;

#define uni_isalnum uni_isalnum
#define uni_isalpha uni_isalpha
#define uni_iscntrl uni_iscntrl
#define uni_isdigit uni_isdigit
#define uni_isgraph uni_isgraph
#define uni_islower uni_islower
#define uni_isprint uni_isprint
#define uni_ispunct uni_ispunct
#define uni_isspace uni_isspace
#define uni_isupper uni_isupper
#define uni_isxdigit uni_isxdigit
#define uni_toupper uni_toupper
#define uni_tolower uni_tolower

#define uni_previous unicode_previous
#define uni_next unicode_next
#define uni_strlen unicode_strlen
#define uni_string_width unicode_string_width
#define uni_get_utf8 unicode_get_utf8
#define uni_offset_to_index unicode_offset_to_index
#define uni_index_to_offset unicode_index_to_offset
#define uni_last_utf8 unicode_last_utf8
#define uni_strchr unicode_strchr
#define uni_strrchr unicode_strrchr
#define uni_strncpy unicode_strncpy

#endif /* !HAVE_UNICODE */

/* The strings returned will have to be g_free()'d */
extern utfchar *charconv_local8_to_utf8(const gchar *local);
extern gchar *charconv_utf8_to_local8(const utfchar *utf);

/* The string here is statically allocated and must NOT be g_free()'d.*/
extern utfchar *charconv_unichar_to_utf8(unichar uc);

#define CHARCONV_H
#endif  /* CHARCONV_H */


