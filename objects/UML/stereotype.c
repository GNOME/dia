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

#include <string.h>
#include "stereotype.h"

char *
string_to_bracketted(char *str, char *start_bracket, char *end_bracket) {
  char *bracketted;

  bracketted = g_malloc(sizeof(char)*strlen(str)+
			2*strlen(start_bracket)+1);
  strcpy(bracketted, start_bracket);
  strcat(bracketted, str);
  strcat(bracketted, end_bracket);

  return bracketted;
}

char *
bracketted_to_string(char *bracketted, int bracket_len) {
  char *str;

  str = strdup(bracketted);
  strcpy(str, bracketted+bracket_len);
  str[strlen(str)-bracket_len] = 0;

  return str;
}

char uml_start_bracket[] = {(char)UML_STEREOTYPE_START, '\0'};
char uml_end_bracket[] = {(char)UML_STEREOTYPE_END, '\0'};

char *
string_to_stereotype(char *str) {
  return string_to_bracketted(str, uml_start_bracket, uml_end_bracket);
}

char *
stereotype_to_string(char *stereotype) {
  return bracketted_to_string(stereotype, 1);
}

