/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * render_gnomeprint.[ch] -- gnome-print renderer for dia.
 * Copyright (C) 1999 James Henstridge
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

#ifndef RENDER_GNOMEPRINT_H
#define RENDER_GNOMEPRINT_H

#include <stdio.h>

#include <libgnomeprint/gnome-print.h>

typedef struct _RendererGPrint RendererGPrint;

#include "geometry.h"
#include "render.h"
#include "display.h"

struct _RendererGPrint {
  Renderer renderer;

  GnomePrintContext *ctx;
  GnomeFont *font;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;
};

RendererGPrint *new_gnomeprint_renderer(Diagram *dia,
					GnomePrintContext *ctx);
#endif /* RENDER_GNOMEPRINT_H */
