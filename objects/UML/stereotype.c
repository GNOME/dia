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
string_to_stereotype(char *str) {
  char *stereotype;

  stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
  stereotype[0] = UML_STEREOTYPE_START;
  stereotype[1] = 0;
  strcat(stereotype, str);
  stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
  stereotype[strlen(str)+2] = 0;

  return stereotype;
}

char *
stereotype_to_string(char *stereotype) {
  char *str;

  str = strdup(stereotype);
  strcpy(str, stereotype+1);
  str[strlen(str)-1] = 0;

  return str;
}
