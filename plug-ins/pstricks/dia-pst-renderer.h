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

#pragma once

#include "diarenderer.h"

G_BEGIN_DECLS

#define DIA_PST_TYPE_RENDERER (dia_pst_renderer_get_type ())
G_DECLARE_FINAL_TYPE (DiaPstRenderer, dia_pst_renderer, DIA_PST, RENDERER, DiaRenderer)

gboolean      dia_pst_renderer_export              (DiaPstRenderer *self,
                                                    DiaContext     *ctx,
                                                    DiagramData    *data,
                                                    const char     *filename,
                                                    const char     *diafilename);

G_END_DECLS
