/* xxxxxx -- an diagram creation/manipulation program
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
#ifndef FONT_H
#define FONT_H

#include <gdk/gdk.h>
#include "geometry.h"

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} Alignment;

typedef struct _Font Font;

struct _Font {
  char *name;
};

extern GList *font_names; /* GList with 'char *' data.*/

extern void font_init(void);
extern Font *font_getfont(const char *name);
extern GdkFont *font_get_gdkfont(Font *font, int height);
extern char *font_get_psfontname(Font *font);
extern real font_string_width(const char *string, Font *font, real height);
extern real font_ascent(Font *font, real height);
extern real font_descent(Font *font, real height);

#endif /* FONT_H */
