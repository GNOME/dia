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
#ifndef RENDER_PIXMAP_H
#define RENDER_PIXMAP_H

#include "diarenderer.h"

DiaRenderer *new_pixmap_renderer(GdkWindow *window, int width, int height);
void renderer_pixmap_set_pixmap(DiaRenderer *renderer,
                                GdkDrawable *drawable, 
                                int xoffset, int yoffset,
                                int width, int height);
GdkPixmap *renderer_pixmap_get_pixmap(DiaRenderer *renderer);

#endif /* RENDER_PIXMAP_H */
