/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_svg.c - an SVG renderer for dia, based on render_eps.c
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
#ifndef RENDER_SVG_H
#define RENDER_SVG_H

#include <stdio.h>

typedef struct _RendererSVG RendererSVG;

#include <tree.h>

#include "geometry.h"
#include "render.h"
#include "display.h"
#include "filter.h"

struct _RendererSVG {
  Renderer renderer;

  char *filename;

  xmlDocPtr doc;
  xmlNodePtr root;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;

  real linewidth;
  const char *linecap;
  const char *linejoin;
  char *linestyle; /* not const -- must free */

  real fontsize;
};

extern RendererSVG *new_svg_renderer(DiagramData *data, const char *filename);
extern void destroy_svg_renderer(RendererSVG *renderer);

extern DiaExportFilter svg_export_filter;

#endif /* RENDER_SVG_H */
