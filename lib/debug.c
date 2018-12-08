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

/* This file contains various functions to aid in debugging in general. */

#include <config.h>

#include <stdio.h>
#include <glib/gprintf.h>
#include <stdarg.h>

#include "debug.h"

/** Output a message if the given value is not true. 
 * @param val A value to test. 
 * @param format A printf-style message to output if the value is false.
 * @return val
 */
gboolean
dia_assert_true(gboolean val, const gchar *format, ...)
{
  va_list args;
  if (!val) {
    va_start(args, format);
    g_vfprintf(stderr, format, args);
    va_end(args);
  }
  return val;
}
