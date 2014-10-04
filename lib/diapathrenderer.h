/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diapathrenderer.c -- render _everything_ to a path
 * Copyright (C) 2012, Hans Breuer <Hans@Breuer.Org>
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
#ifndef DIA_PATH_RENDERER_H
#define DIA_PATH_RENDERER_H

#include "diarenderer.h"

typedef struct _DiaPathRenderer DiaPathRenderer;
typedef struct _DiaPathRendererClass DiaPathRendererClass;

/*! GObject boiler plate, create runtime information */
#define DIA_TYPE_PATH_RENDERER           (dia_path_renderer_get_type ())
/*! GObject boiler plate, a safe type cast */
#define DIA_PATH_RENDERER(obj)           (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_PATH_RENDERER, DiaPathRenderer))
/*! GObject boiler plate, in C++ this would be the vtable */
#define DIA_PATH_RENDERER_CLASS(klass)   (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_PATH_RENDERER, DiaPathRendererClass))
/*! GObject boiler plate, type check */
#define DIA_IS_PATH_RENDERER(obj)        (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_PATH_RENDERER))
/*! GObject boiler plate, get from object to class (vtable) */
#define DIA_PATH_RENDERER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_PATH_RENDERER, DiaPathRendererClass))

GType dia_path_renderer_get_type (void) G_GNUC_CONST;

void path_build_arc (GArray *path, Point *center,
		     real width, real height,
		     real angle1, real angle2,
		     gboolean closed);
void path_build_ellipse (GArray *path,
			 Point *center,
			 real width, real height);

/* in path-math.c */

#endif

