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
#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include "config.h"

int nearest_pow (int num);
int format_string_length_upper_bound (const char* fmt,
				      va_list *args);

#ifndef HAVE_SNPRINTF
int snprintf ( char *str, size_t n, const char *format, ... );
#endif

#endif /* UTILS_H */





