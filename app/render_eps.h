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
#ifndef RENDER_EPS_H
#define RENDER_EPS_H

#include <stdio.h>

typedef struct _RendererEPS RendererEPS;

#include "geometry.h"
#include "render.h"
#include "display.h"
#include "filter.h"
#ifdef HAVE_UNICODE
#include "ps-utf8.h"
#endif


struct _RendererEPS {
  Renderer renderer;

  FILE *file;
  int is_ps;
  int pagenum;

  LineStyle saved_line_style;
  real dash_length;
  real dot_length;
#ifdef HAVE_UNICODE
  PSUnicoder *psu;
#endif
  Color lcolor;
};

RendererEPS *new_eps_renderer(Diagram *dia, char *filename);
RendererEPS *new_psprint_renderer(Diagram *dia, FILE *file);
void eps_renderer_prolog_done(RendererEPS *renderer);
void destroy_eps_renderer(RendererEPS *renderer);

extern DiaExportFilter eps_export_filter;

#endif /* RENDER_EPS_H */

