/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * render_metapost.h: Exporting module/plug-in to TeX Metapost
 * Copyright (C) 2001 Chris Sperandio
 * Originally derived from render_pstricks.h (pstricks plug-in)
 * Copyright (C) 2000 Jacek Pliszka <pliszka@fuw.edu.pl>
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
#ifndef RENDER_METAPOST_H
#define RENDER_METAPOST_H

#include <stdio.h>

typedef struct _RendererMETAPOST RendererMETAPOST;

#include "geometry.h"
#include "render.h"
#include "diagramdata.h"
#include "filter.h"

struct _RendererMETAPOST {
    Renderer renderer;

    FILE *file;
    
    LineStyle saved_line_style;
    LineCaps  saved_line_cap;
    LineJoin  saved_line_join;

    real line_width;
    real dash_length;
    real dot_length;
};

extern RendererMETAPOST * new_metapost_renderer(DiagramData *data,
						const char *filename,
						const char *diafilename);
extern void destroy_metapost_renderer(RendererMETAPOST *renderer);

extern DiaExportFilter metapost_export_filter;

#endif /* RENDER_METAPOST_H */

