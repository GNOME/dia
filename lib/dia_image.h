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
#ifndef DIA_IMAGE_H
#define DIA_IMAGE_H

#include "diatypes.h"
#include <gdk/gdk.h>
#include <gdk-pixbuf/gdk-pixbuf.h>

#include "geometry.h"

G_BEGIN_DECLS

#define DIA_TYPE_IMAGE dia_image_get_type ()

G_DECLARE_FINAL_TYPE (DiaImage, dia_image, DIA, IMAGE, GObject)

DiaImage        *dia_image_get_broken        (void);

DiaImage        *dia_image_load              (const gchar    *filename);
DiaImage        *dia_image_new_from_pixbuf   (GdkPixbuf      *pixbuf);
void             dia_image_add_ref           (DiaImage       *image);
void             dia_image_unref             (DiaImage       *image);

gboolean         dia_image_save              (DiaImage       *image,
                                              const gchar    *filename);

int              dia_image_width             (const DiaImage *image);
int              dia_image_rowstride         (const DiaImage *image);
int              dia_image_height            (const DiaImage *image);
/**
 * dia_image_rgb_data:
 * @image: the #DiaImage
 *
 * Returns: a copy of the RGB data in this image with any alpha stripped
 * The returned buffer must be freed after use.
 * The buffer is laid out as dia_image_width*dia_image_rowstride*3 bytes.
 */
guint8          *dia_image_rgb_data          (const DiaImage *image);
/**
 * dia_image_mask_data:
 * @image: the #DiaImage
 *
 * Returns: a copy of the alpha data in this image, or %NULL if none
 * The returned buffer must be freed after use.
 * The buffer is laid out as dia_image_width*dia_image_height bytes.
 */
guint8          *dia_image_mask_data         (const DiaImage *image);
/**
 * dia_image_rgba_data:
 * @image: the #DiaImage
 *
 * Returns: the RGBA data in this image, or NULL if there's no alpha.
 * Note that this is the raw data, not a copy.
 */
const guint8    *dia_image_rgba_data         (const DiaImage *image);
const char      *dia_image_filename          (const DiaImage *image);
const GdkPixbuf *dia_image_pixbuf            (const DiaImage *image);
const gchar     *dia_image_get_mime_type     (const DiaImage *image);
void             dia_image_set_mime_type     (DiaImage       *image,
                                              const gchar    *mime_type);

GdkPixbuf       *dia_image_get_scaled_pixbuf (DiaImage       *image,
                                              int             width,
                                              int             height);
cairo_surface_t *dia_image_get_surface       (DiaImage       *self);

G_END_DECLS

#endif /* DIA_IMAGE_H */

