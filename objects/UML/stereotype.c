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

/* Separate functions to handle stereotypes in guillemots. */

#include <config.h>

#include <string.h>
#include "stereotype.h"
#include "charconv.h"

utfchar *
string_to_bracketted (utfchar *str, 
		      const char *start_bracket, 
		      const char *end_bracket)
{
	utfchar *utfstart, *utfend;
	utfchar *retval;

#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
	utfstart = charconv_local8_to_utf8 (start_bracket);
	utfend = charconv_local8_to_utf8 (end_bracket);
	retval = g_strconcat (utfstart, (str ? str : ""), utfend, NULL);
	g_free (utfstart);
	g_free (utfend);
#else
	retval = g_strconcat (start_bracket, (str ? str : ""), end_bracket, NULL);
#endif

	return retval;
}

utfchar *
bracketted_to_string (utfchar *bracketted,
		      const char *start_bracket,
		      const char *end_bracket)
{
	utfchar *utfstart, *utfend, *utfstr;
	utfchar *retval;
	int start_len, end_len, str_len;

	if (!bracketted) return NULL;
#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
	utfstart = charconv_local8_to_utf8 (start_bracket);
	utfend = charconv_local8_to_utf8 (end_bracket);
#else
	utfstart = start_bracket;
	utfend = end_bracket;
#endif
	start_len = strlen (utfstart);
	end_len = strlen (utfend);
	str_len = strlen (bracketted);
	utfstr = bracketted;

	if (!strncmp (utfstr, utfstart, start_len)) {
		utfstr += start_len;
		str_len -= start_len;
	}
	if (str_len >= end_len && end_len > 0) {
#if !GLIB_CHECK_VERSION(2,0,0)
		utfchar *utf = utfstr;
		utfchar *utfprev;
		int unilen = uni_strlen (utfend, end_len);
		int i;

		while (*utf) {
			utfprev = utf;
			utf = uni_next (utf);
		}
		utf = utfprev;
		for (i = 0; i < (unilen - 1); i++) {
			utf = uni_previous (utfstr, utf);
		}
		if (!strncmp (utf, utfend, end_len)) {
			str_len -= end_len;
		}
#else
		if (g_utf8_strrchr (utfstr, str_len, g_utf8_get_char (utfend)))
			str_len -= end_len;
#endif
	}
	retval = g_strndup (utfstr, str_len);

#ifdef GTK_DOESNT_TALK_UTF8_WE_DO
	g_free (utfstart);
	g_free (utfend);
#endif

	return retval;
}

utfchar *
string_to_stereotype (utfchar *str)
{
  if ((str) && str[0] != '\0')  
    return string_to_bracketted(str, UML_STEREOTYPE_START, UML_STEREOTYPE_END);
  return g_strdup(str);
}

utfchar *
remove_stereotype_from_string (utfchar *stereotype)
{
  if (stereotype) { 
    utfchar *tmp = bracketted_to_string (stereotype, 
					 UML_STEREOTYPE_START, UML_STEREOTYPE_END);
    g_free(stereotype);
    return tmp;
  } else {
    return NULL;
  }
}

utfchar *
stereotype_to_string (utfchar *stereotype)
{
  return bracketted_to_string(stereotype, 
                              UML_STEREOTYPE_START, UML_STEREOTYPE_END);
}

