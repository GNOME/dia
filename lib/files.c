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
#include <sys/types.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <math.h>

#include "files.h"
#include "message.h"

void
write_int32(int fd, gint32 x)
{
  int to_write;
  guint32 y;
  char *ptr;
  int count;
  
  y = htonl((guint32) x);

  ptr = (char *) &y;
  to_write = 4;
  while (to_write > 0) {
    count = write(fd, ptr, to_write);
    if (count == -1) {
      message_error("Error saving file");
      return;
    }
    to_write -= count;
    ptr += count;
  }
}

void
write_real(int fd, real x)
{
  int exp_int;
  gint32 exp, mant;
  double fraction;

  fraction = frexp(x, &exp_int);
  exp = (gint32) exp_int;
  
  mant = (gint32) ceil((fraction * 2147483648.0));

  write_int32(fd, mant);
  write_int32(fd, exp);
}

void
write_string(int fd, char *string)
{
  int len, count;

  if (string==NULL) {
    len = -1;
    write_int32(fd, len);
    return;
  }
  
  len = strlen(string);
  write_int32(fd, len);

  while (len > 0) {
    count = write(fd, string, len);
    len -= count;
    string += count;
  }
}

void write_point(int fd, Point *point)
{
  write_real(fd, point->x);
  write_real(fd, point->y);
}

void write_rectangle(int fd, Rectangle *rect)
{
  write_real(fd, rect->top);
  write_real(fd, rect->left);
  write_real(fd, rect->bottom);
  write_real(fd, rect->right);
}

void write_color(int fd, Color *col)
{
  write_real(fd, col->red);
  write_real(fd, col->green);
  write_real(fd, col->blue);
}

void write_text(int fd, Text *text)
{
  char *str;

  str = text_get_string_copy(text);
  write_string(fd, str);
  g_free(str);
  write_string(fd, text->font->name);
  write_real(fd, text->height);
  write_point(fd, &text->position);
  write_color(fd, &text->color);
  write_int32(fd, text->alignment);
}



gint32
read_int32(int fd)
{
  int to_read;
  guint32 y;
  char *ptr;
  int count;
  
  to_read = 4;
  ptr = (char *)&y;
  while (to_read > 0) {
    count = read(fd, ptr, to_read);
    if (count == -1) {
      perror("Error reading file");
      return 0;
    }
    to_read -= count;
    ptr += count;
  }
  y = ntohl(y);
  return (gint32)y;
}

real
read_real(int fd)
{
  double fraction;
  gint32 exp, mant;
  
  mant = read_int32(fd);
  exp = read_int32(fd);
  fraction = ((double)mant)/2147483648.0;

  return ldexp(fraction, (int)exp);
}

char *
read_string(int fd)
{
  gint32 len;
  char *str;
  char *ptr;
  int count;

  len = read_int32(fd);
  if (len == -1) {
    return NULL;
  }
  
  str = g_malloc(len+1);

  ptr = str;
  while (len > 0) {
    count = read(fd, ptr, len);
    if (count == -1) {
      message_error("Error reading file");
      return 0;
    }
    len -= count;
    ptr += count;
  }
  *ptr = 0;
  
  return str;
}

void
read_point(int fd, Point *point)
{
  point->x = read_real(fd);
  point->y = read_real(fd);
}

void
read_rectangle(int fd, Rectangle *rect)
{
  rect->top = read_real(fd);
  rect->left = read_real(fd);
  rect->bottom = read_real(fd);
  rect->right = read_real(fd);
}

void
read_color(int fd, Color *col)
{
  col->red = read_real(fd);
  col->green = read_real(fd);
  col->blue = read_real(fd);
}

Text *read_text(int fd)
{
  char *str;
  char *fontname;
  real height;
  Text *text;
  Point pos;
  Color col;
  Alignment align;

  str = read_string(fd);
  fontname = read_string(fd);
  height = read_real(fd);
  read_point(fd, &pos);
  read_color(fd, &col);
  align = read_int32(fd);
  
  text = new_text(str, font_getfont(fontname), height, &pos, &col, align);
  g_free(str);
  g_free(fontname);
  
  return text;
}







