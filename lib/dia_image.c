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
#include <string.h> /* memmove */

#include "geometry.h"
#include "dia_image.h"
#include <gtk/gtkwidget.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "pixmaps/broken.xpm"

#define SCALING_CACHE

struct _DiaImage {
  GdkPixbuf *image;
  gchar *filename;
#ifdef SCALING_CACHE
  GdkPixbuf *scaled; /* a cache of the last scaled version */
  int scaled_width, scaled_height;
#endif
};

gboolean _dia_image_initialized = FALSE;

void 
dia_image_init(void)
{
  if (!_dia_image_initialized) {
    gtk_widget_set_default_colormap(gdk_rgb_get_cmap());
    _dia_image_initialized = TRUE;
  }
}

DiaImage
dia_image_get_broken(void)
{
  static DiaImage broken = NULL;

  if (broken == NULL) {
    broken = g_new(struct _DiaImage, 1);
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

  image = gdk_pixbuf_new_from_file(filename, NULL);
  if (image == NULL)
    return NULL;

  dia_img = g_new(struct _DiaImage, 1);
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
					      GDK_INTERP_TILES);
      image->scaled_width = width;
      image->scaled_height = height;
    }
    scaled = image->scaled;
#else
    scaled = gdk_pixbuf_scale_simple(image->image, width, height, 
				     GDK_INTERP_TILES);
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

int
dia_image_rowstride(DiaImage image)
{
  return gdk_pixbuf_get_rowstride(image->image);
}

guint8 *
dia_image_rgb_data(DiaImage image)
{
  int width = dia_image_width(image);
  int height = dia_image_height(image);
  int rowstride = dia_image_rowstride(image);
  int size = height*rowstride;
  guint8 *rgb_pixels = g_malloc(size);

  if (gdk_pixbuf_get_has_alpha(image->image)) {
    guint8 *pixels = gdk_pixbuf_get_pixels(image->image);
    int i, j;
    for (i = 0; i < height; i++) {
      for (j = 0; j < width; j++) {
	rgb_pixels[i*rowstride+j*3] = pixels[i*rowstride+j*4];
	rgb_pixels[i*rowstride+j*3+1] = pixels[i*rowstride+j*4+1];
	rgb_pixels[i*rowstride+j*3+2] = pixels[i*rowstride+j*4+2];
      }
    }
    return rgb_pixels;
  } else {
    guint8 *pixels = gdk_pixbuf_get_pixels(image->image);

    g_memmove(rgb_pixels, pixels, height*rowstride);
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

const guint8 *
dia_image_rgba_data(DiaImage image)
{
  if (gdk_pixbuf_get_has_alpha(image->image)) {
    const guint8 *pixels = gdk_pixbuf_get_pixels(image->image);
    
    return pixels;
  } else {
    return NULL;
  }
}

char *
dia_image_filename(DiaImage image)
{
  return image->filename;
}
