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

/* This file contains some code from glib.
  Here is the copyright notice:
  
 * GLIB - Library of useful routines for C programming
 * Copyright (C) 1995-1997  Peter Mattis, Spencer Kimball and Josh MacDonald
 */


#include <config.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib.h>

#include "utils.h"

int
nearest_pow (int num)
{
  int n = 1;

  while (n < num)
    n <<= 1;

  return n;
}

int
format_string_length_upper_bound (const char* fmt, va_list *args)
{
  int len = 0;
  int short_int;
  int long_int;
  int done;
  char *tmp;

  while (*fmt)
    {
      char c = *fmt++;

      short_int = FALSE;
      long_int = FALSE;

      if (c == '%')
	{
	  done = FALSE;
	  while (*fmt && !done)
	    {
	      switch (*fmt++)
		{
		case '*':
		  len += va_arg(*args, int);
		  break;
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		  fmt -= 1;
		  len += strtol (fmt, (char **)&fmt, 10);
		  break;
		case 'h':
		  short_int = TRUE;
		  break;
		case 'l':
		  long_int = TRUE;
		  break;

		  /* I ignore 'q' and 'L', they're not portable anyway. */

		case 's':
		  tmp = va_arg(*args, char *);
		  if(tmp)
		    len += strlen (tmp);
		  else
		    len += strlen ("(null)");
		  done = TRUE;
		  break;
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		  if (long_int)
		    (void)va_arg (*args, long);
		  else if (short_int)
		    (void)va_arg (*args, int);
		  else
		    (void)va_arg (*args, int);
		  len += 32;
		  done = TRUE;
		  break;
		case 'D':
		case 'O':
		case 'U':
		  (void)va_arg (*args, long);
		  len += 32;
		  done = TRUE;
		  break;
		case 'e':
		case 'E':
		case 'f':
		case 'g':
		  (void)va_arg (*args, double);
		  len += 32;
		  done = TRUE;
		  break;
		case 'c':
		  (void)va_arg (*args, int);
		  len += 1;
		  done = TRUE;
		  break;
		case 'p':
		case 'n':
		  (void)va_arg (*args, void*);
		  len += 32;
		  done = TRUE;
		  break;
		case '%':
		  len += 1;
		  done = TRUE;
		  break;
		default:
		  break;
		}
	    }
	}
      else
	len += 1;
    }

  return len;
}

#ifndef HAVE_SNPRINTF
int
snprintf ( char *str, size_t n, const char *format, ... )
{
  static char *buf = NULL;
  static int alloc = 0;
  int len;
  va_list args, args2;

  va_start (args, format);
  len = format_string_length_upper_bound (format, &args);
  va_end (args);
  
  if (len >= alloc) {
      if (buf)
	g_free (buf);

      alloc = nearest_pow (MAX(len + 1, 1024));
      
      buf = g_new (char, alloc);
  }

  va_start (args2, format);
  vsprintf (buf, format, args2);
  va_end (args2);

  strncpy(str, buf, n);
  str[n-1] = 0;

  return (len >= n)?-1:strlen(str);
}
#endif







