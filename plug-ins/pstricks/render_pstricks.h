/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_pstricks.h: Exporting module/plug-in to TeX Pstricks
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
 *  6.5.2000
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
#ifndef RENDER_PSTRICKS_H
#define RENDER_PSTRICKS_H

#include <stdio.h>

typedef struct _RendererPSTRICKS RendererPSTRICKS;

#include "geometry.h"
#include "render.h"
#include "diagramdata.h"
#include "filter.h"

struct _RendererPSTRICKS {
    Renderer renderer;

    FILE *file;
    int is_ps;
    int pagenum;

    LineStyle saved_line_style;
    real dash_length;
    real dot_length;
};

extern RendererPSTRICKS * new_pstricks_renderer(DiagramData *data,
						const char *filename,
						const char *diafilename);
extern void destroy_pstricks_renderer(RendererPSTRICKS *renderer);

extern DiaExportFilter pstricks_export_filter;

#endif /* RENDER_PSTRICKS_H */

