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

#include <config.h>

#include "geometry.h"
#include "render.h"
#include "dia_image.h"
#include <gtk/gtkwidget.h>
#ifdef HAVE_GDK_PIXBUF
#include <gdk-pixbuf/gdk-pixbuf.h>
#else
#include <gdk_imlib.h>
#endif

#include "pixmaps/broken.xpm"

#define SCALING_CACHE

#ifdef HAVE_GDK_PIXBUF
struct DiaImage {
  GdkPixbuf *image; /* GdkPixbuf does its own refcount */
  gchar *filename; /* ...buf doesn't remember the filename */
#ifdef SCALING_CACHE
  GdkPixbuf *scaled; /* ...and doesn't seem to cache the scaled versions */
  int scaled_width, scaled_height;
#endif
};

void 
dia_image_init(void)
{
}

DiaImage
dia_image_get_broken(void)
{
  static DiaImage broken = NULL;

  if (broken == NULL) {
    broken = g_new(struct DiaImage, 1);
    broken->image = gdk_pixbuf_new_from_xpm_data((const char **)broken_xpm);
  } else {
    gdk_pixbuf_ref(broken->image);
  }
  /* Kinda hard to export :) */
  broken->filename = g_strdup("broken");
#ifdef SCALING_CACHE
  broken->scaled = NULL;
#endif
  return broken;
}

DiaImage 
dia_image_load(gchar *filename) 
{
  DiaImage dia_img;
  GdkPixbuf *image;

  image = gdk_pixbuf_new_from_file(filename);
  if (image == NULL)
    return NULL;

  dia_img = g_new(struct DiaImage, 1);
  dia_img->image = image;
  /* We have a leak here, unless we add our own refcount */
  dia_img->filename = g_strdup(filename);
#ifdef SCALING_CACHE
  dia_img->scaled = NULL;
#endif
  return dia_img;
}

void
dia_image_add_ref(DiaImage image)
{
  gdk_pixbuf_ref(image->image);
}

void
dia_image_release(DiaImage image)
{
  gdk_pixbuf_unref(image->image);
}

void 
dia_image_draw(DiaImage image, GdkWindow *window,
	       int x, int y, int width, int height)
{
  GdkPixbuf *scaled;

  if (gdk_pixbuf_get_width(image->image) != width ||
      gdk_pixbuf_get_height(image->image) != height) {
    /* Using TILES to make it look more like PostScript */
#ifdef SCALING_CACHE
    if (image->scaled == NULL ||
	image->scaled_width != width || image->scaled_height != height) {
      if (image->scaled)
	gdk_pixbuf_unref(image->scaled);
      image->scaled = gdk_pixbuf_scale_simple(image->image, width, height, 
					      ART_FILTER_TILES);
      image->scaled_width = width;
      image->scaled_height = height;
    }
    scaled = image->scaled;
#else
    scaled = gdk_pixbuf_scale_simple(image->image, width, height, 
				     ART_FILTER_TILES);
#endif
  } else {
    scaled = image->image;
  }

  /* Once we can render Alpha, we'll do it! */
  gdk_pixbuf_render_to_drawable_alpha(scaled, window,
				      0, 0, x, y, width, height, 
				      GDK_PIXBUF_ALPHA_BILEVEL, 128,
				      GDK_RGB_DITHER_NORMAL, 0, 0);
#ifndef SCALING_CACHE
  gdk_pixbuf_unref(scaled);
#endif
}

int 
dia_image_width(DiaImage image)
{
  return gdk_pixbuf_get_width(image->image);
}

int 
dia_image_height(DiaImage image)
{
  return gdk_pixbuf_get_height(image->image);
}

guint8 *
dia_image_rgb_data(DiaImage image)
{
  int size = gdk_pixbuf_get_width(image->image)*
    gdk_pixbuf_get_height(image->image);
  guint8 *rgb_pixels = g_malloc(size*3);

  if (gdk_pixbuf_get_has_alpha(image->image)) {
    guint8 *pixels = gdk_pixbuf_get_pixels(image->image);
    int i;

    for (i = 0; i < size; i++) {
      rgb_pixels[i*3] = pixels[i*4];
      rgb_pixels[i*3+1] = pixels[i*4+1];
      rgb_pixels[i*3+2] = pixels[i*4+2];
    }
    return rgb_pixels;
  } else {
    g_memmove(rgb_pixels, gdk_pixbuf_get_pixels(image->image), size*3);
    return rgb_pixels;
  }
}

guint8 *
dia_image_mask_data(DiaImage image)
{
  guint8 *pixels;
  guint8 *mask;
  int i, size;

  if (!gdk_pixbuf_get_has_alpha(image->image)) {
    return NULL;
  }
  
  pixels = gdk_pixbuf_get_pixels(image->image);

  size = gdk_pixbuf_get_width(image->image)*
    gdk_pixbuf_get_height(image->image);

  mask = g_malloc(size);

  /* Pick every fourth byte (the alpha channel) into mask */
  for (i = 0; i < size; i++)
    mask[i] = pixels[i*4+3];

  return mask;
}

char *
dia_image_filename(DiaImage image)
{
  return image->filename;
}

#else HAVE_GDK_PIXBUF
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
  guint8 *rgb_data;
  int size = 3*image->image->rgb_width*image->image->rgb_height;

  rgb_data = g_malloc(size);
  g_memmove(rgb_data, image->image->rgb_data, size);
  return rgb_data;
}

char *
dia_image_filename(DiaImage image)
{
  return (image->image)->filename;
}

guint8 *
dia_image_mask_data(DiaImage image)
{
  GdkImlibColor transparent;
  guint8 *pixels;
  guint8 *mask;
  int i, size, trans;

  gdk_imlib_get_image_shape(image->image, &transparent);

  trans = (transparent.r<<16)+(transparent.g<<8)+transparent.b;

  pixels = image->image->rgb_data;

  size = image->image->rgb_width*image->image->rgb_height;

  mask = g_malloc(size);

  for (i = 0; i < size; i++) {
    int col = (pixels[i*3]<<16)+(pixels[i*3+1]<<8)+pixels[i*3+2];
    mask[i] = (col==trans?0:255);
  }

  return mask;
}
#endif HAVE_GDK_PIXBUF
