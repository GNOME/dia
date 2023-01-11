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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <string.h>
#include "stereotype.h"


char *
string_to_bracketted (char       *str,
                      const char *start_bracket,
                      const char *end_bracket)
{
  return g_strconcat (start_bracket, (str ? str : ""), end_bracket, NULL);
}


char *
bracketted_to_string (char       *bracketted,
                      const char *start_bracket,
                      const char *end_bracket)
{
  const char *utfstart, *utfend, *utfstr;
  int start_len, end_len, str_len;

  if (!bracketted) return NULL;

  utfstart = start_bracket;
  utfend = end_bracket;

  start_len = strlen (utfstart);
  end_len = strlen (utfend);
  str_len = strlen (bracketted);
  utfstr = bracketted;

  if (!strncmp (utfstr, utfstart, start_len)) {
    utfstr += start_len;
    str_len -= start_len;
  }
  if (str_len >= end_len && end_len > 0) {
    if (g_utf8_strrchr (utfstr, str_len, g_utf8_get_char (utfend)))
      str_len -= end_len;
  }
  return g_strndup (utfstr, str_len);
}


char *
string_to_stereotype (char *str)
{
  if ((str) && str[0] != '\0') {
    return string_to_bracketted (str,
                                 UML_STEREOTYPE_START,
                                 UML_STEREOTYPE_END);
  }
  return g_strdup (str);
}


char *
remove_stereotype_from_string (char *stereotype)
{
  if (stereotype) {
    char *tmp = bracketted_to_string (stereotype,
                                      UML_STEREOTYPE_START,
                                      UML_STEREOTYPE_END);
    g_free (stereotype);
    return tmp;
  } else {
    return NULL;
  }
}


char *
stereotype_to_string (char *stereotype)
{
  return bracketted_to_string (stereotype,
                               UML_STEREOTYPE_START,
                               UML_STEREOTYPE_END);
}
