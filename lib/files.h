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
#ifndef FILES_H
#define FILES_H

#include <glib.h>
#include "geometry.h"
#include "color.h"
#include "text.h"

extern void write_int32(int fd, gint32 x);
extern void write_real(int fd, real x);
extern void write_string(int fd, char *string); /* Handles NULL ptr's */
extern void write_point(int fd, Point *point);
extern void write_rectangle(int fd, Rectangle *rect);
extern void write_color(int fd, Color *col);
extern void write_text(int fd, Text *text);

extern gint32 read_int32(int fd);
extern real read_real(int fd);
extern char *read_string(int fd); /* can return a NULL ptr */
extern void read_point(int fd, Point *point);
extern void read_rectangle(int fd, Rectangle *rect);
extern void read_color(int fd, Color *col);
extern Text *read_text(int fd);
#endif /* FILES_H */

