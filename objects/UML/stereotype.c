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

char *
string_to_bracketted(char *str, char *start_bracket, char *end_bracket) {
#ifdef UNICODE_WORK_IN_PROGRESS
  char *bracketted;

  bracketted = g_strconcat(start_bracket,tmpstr,end_bracket,NULL);

  return bracketted;
#else
  char *tmpstr;
  char *bracketted,*tmp;

  tmpstr = charconv_local8_to_utf8((str?str:""));
  bracketted = g_strconcat(start_bracket,tmpstr,end_bracket,NULL);
  tmp = charconv_utf8_to_local8(bracketted);
  g_free(bracketted);
  return tmp;
#endif
}

static char *strend(char *p) {
  if (!p) return NULL;
  while (*p) p++;
  return p;
}

#ifdef UNICODE_WORK_IN_PROGRESS
char *
bracketted_to_string(utfchar *bracketted, 
                     utfchar *start_bracket, 
                     utfchar *end_bracket){
#else
static char *
_bracketted_to_string(utfchar *bracketted, 
                      utfchar *start_bracket, 
                      utfchar *end_bracket){
#endif
  char *str;
  int start_len = strlen(start_bracket);
  int end_len = strlen(end_bracket);
  int str_len = strlen(bracketted);

  if (!bracketted) return NULL;

  str = bracketted;
  if (0==strncmp(str,start_bracket,start_len)) {
    str += start_len;
    str_len -= start_len;
  }
  if (0 == strncmp(strend(str) - end_len,end_bracket,end_len)) {
    str_len -= end_len;
  }
  return g_strndup(str,str_len);
}

#ifndef UNICODE_WORK_IN_PROGRESS
char *
bracketted_to_string(char *bracketted, char *start_bracket, char *end_bracket){
  char *tmp = charconv_local8_to_utf8((bracketted?bracketted:""));  
  char *uref = _bracketted_to_string(tmp,start_bracket,end_bracket);
  char *ret = charconv_utf8_to_local8(uref);

  g_free(uref); g_free(tmp); 
  return ret;
}  
#endif

char uml_start_bracket[] = { UML_STEREOTYPE_START, '\0'};
char uml_end_bracket[] = { UML_STEREOTYPE_END, '\0'};

char *
string_to_stereotype(char *str) {
  return string_to_bracketted(str, uml_start_bracket, uml_end_bracket);
}

char *
remove_stereotype_from_string(char *stereotype) {
  if (stereotype) { 
    char *tmp = bracketted_to_string(stereotype, 
                                     uml_start_bracket, uml_end_bracket);
    g_free(stereotype);
    return tmp;
  } else {
    return NULL;
  }
}

char *
stereotype_to_string(char *stereotype) {
  return bracketted_to_string(stereotype, 
                              uml_start_bracket, uml_end_bracket);
}

