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
#include <gtk/gtkwidget.h>
#include <gdk_imlib.h>

#include "pixmaps/broken.xpm"

struct DiaImage {
  GdkImlibImage *image;
  int refcount;
};

void
dia_image_init(void)
{
  gdk_imlib_init();
  /* FIXME:  Is this a good idea? */
  gtk_widget_set_default_visual(gdk_imlib_get_visual());
  gtk_widget_set_default_colormap(gdk_imlib_get_colormap());
}


DiaImage
dia_image_get_broken(void)
{
  static DiaImage broken = NULL;

  if (broken == NULL) {
    broken = g_new(struct DiaImage, 1);
    broken->image = gdk_imlib_create_image_from_xpm_data(broken_xpm);
    broken->refcount = 0;
  }

  dia_image_add_ref(broken);
  return broken;
}

DiaImage 
dia_image_load(gchar *filename) 
{
  DiaImage dia_img;
  GdkImlibImage *image;

  image = gdk_imlib_load_image(filename);
  if (image == NULL)
    return NULL;

  dia_img = g_new(struct DiaImage, 1);
  dia_img->image = image;
  dia_img->refcount = 1;
  return dia_img;
}

void
dia_image_add_ref(DiaImage image)
{
  image->refcount++;
}

void
dia_image_release(DiaImage image)
{
  image->refcount--;
  if (image->refcount<=0)
    gdk_imlib_destroy_image(image->image);
}

void
dia_image_draw(DiaImage image, GdkWindow *window,
	       int x, int y, int width, int height)
{
  gdk_imlib_paste_image(image->image, window,
			x, y, width, height);
} 

int 
dia_image_width(DiaImage image) 
{
  return (image->image)->rgb_width;
}

int 
dia_image_height(DiaImage image)
{
  return (image->image)->rgb_height;
}

guint8 *
dia_image_rgb_data(DiaImage image)
{
  return (image->image)->rgb_data;
}

char *
dia_image_filename(DiaImage image)
{
  return (image->image)->filename;
}

