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

/* This renderer is similar to RenderGdk, except that it renders onto
 * its own little GdkPixmap, doesn't deal with the DDisplay at all, and
 * uses a 1-1 mapping between points and pixels.  It is used for rendering
 * arrows and lines onto selectors.  This way, any change in the arrow 
 * definitions are automatically reflected in the selectors.
 */

/*
 * 2002-10-06 : Leaved the original comment but removed almost
 * anything else. The 'RendererPixmap' now uses the DiaGdkRenderer which 
 * is the ported to GObject 'RendererGdk'. No more huge code duplication 
 * but only some renderer parametrization tweaking.                 --hb
 */
#include <config.h>

#include <math.h>
#include <stdlib.h>
#include <gdk/gdk.h>

#include "render_pixmap.h"
#include "message.h"
#include "diagdkrenderer.h"

static Rectangle rect;
static real zoom = 1.0;

DiaRenderer *
new_pixmap_renderer(GdkWindow *window, int width, int height)
{
  DiaGdkRenderer *renderer;
  GdkColor color;

  rect.left = 0;
  rect.top = 0;
  rect.right = width;
  rect.bottom = height;

  renderer = g_object_new (DIA_TYPE_GDK_RENDERER, NULL);
  renderer->transform = dia_transform_new (&rect, &zoom);
  renderer->pixmap = gdk_pixmap_new(window, width, height, -1);
  renderer->gc = gdk_gc_new(window);

  gdk_color_white(gdk_colormap_get_system(), &color);
  gdk_gc_set_foreground(renderer->gc, &color);
  gdk_draw_rectangle(renderer->pixmap, renderer->gc, 1,
		 0, 0, width, height);

  return DIA_RENDERER(renderer);
}

void
renderer_pixmap_set_pixmap (DiaRenderer *ren, 
                            GdkDrawable *drawable,
                            int xoffset, int yoffset, 
                            int width, int height)
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (ren);

  if (renderer->pixmap != NULL)
    g_object_unref(renderer->pixmap);

  if (renderer->gc != NULL)
    g_object_unref(renderer->gc);

  g_object_ref(drawable);
  renderer->pixmap = drawable;
  renderer->gc = gdk_gc_new(drawable);

  rect.left = -xoffset;
  rect.top = -yoffset;
  rect.right = width;
  rect.bottom = height;
}

GdkPixmap *
renderer_pixmap_get_pixmap (DiaRenderer *ren) 
{
  DiaGdkRenderer *renderer = DIA_GDK_RENDERER (ren);

  return renderer->pixmap;
}
