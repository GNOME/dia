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

extern gchar *charconv_local8_to_utf8(const gchar *local);
extern gchar *charconv_utf8_to_local8(const gchar *utf);

#define CHARCONV_H
#endif  /* CHARCONV_H */
