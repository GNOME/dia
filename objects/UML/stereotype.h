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

#ifndef STEREOTYPE_H
#define STEREOTYPE_H

#include "uml.h"

char *string_to_bracketted (char *str, 
			       const char *start_bracket, const char *end_bracket);
char *bracketted_to_string (char *bracketted, const char *start_bracket, 
			       const char *end_bracket);
char *string_to_stereotype (char *str);
char *stereotype_to_string (char *stereotype);
char *remove_stereotype_from_string (char *stereotype);

#endif
