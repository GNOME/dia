/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#include "geometry.h"
#include "render.h"
#include "dia_image.h"
#include <gdk_imlib.h>

void
dia_image_init(void)
{
  gdk_imlib_init();
  /* FIXME:  Is this a good idea? */
  gtk_widget_push_visual(gdk_imlib_get_visual());
  gtk_widget_push_colormap(gdk_imlib_get_colormap());
}


DiaImage 
dia_image_load(gchar *filename) 
{
  return (DiaImage)gdk_imlib_load_image(filename);
}

void
dia_image_release(DiaImage image)
{
  gdk_imlib_destroy_image((GdkImlibImage *)image);
}

void
dia_image_draw(DiaImage image, GdkWindow *window,
	       int x, int y, int width, int height)
{
  gdk_imlib_paste_image((GdkImlibImage *)image, window,
			x, y, width, height);
} 

int 
dia_image_width(DiaImage image) 
{
  return ((GdkImlibImage *)image)->rgb_width;
}

int 
dia_image_height(DiaImage image)
{
  return ((GdkImlibImage *)image)->rgb_height;
}

guint8 *
dia_image_rgb_data(DiaImage image)
{
  return ((GdkImlibImage *)image)->rgb_data;
}

char *
dia_image_filename(DiaImage image)
{
  return ((GdkImlibImage *)image)->filename;
}

