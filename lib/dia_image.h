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

#include "geometry.h"
#include "render.h"


typedef struct _DiaImage *DiaImage;


void dia_image_init(void);

DiaImage dia_image_get_broken(void);

DiaImage dia_image_load(gchar *filename);
void dia_image_add_ref(DiaImage image);
void dia_image_release(DiaImage image);
void dia_image_draw(DiaImage image, GdkWindow *window,
		    int x, int y, int width, int height);

int dia_image_width(DiaImage image);
int dia_image_height(DiaImage image);
guint8 *dia_image_rgb_data(DiaImage image);
guint8 *dia_image_mask_data(DiaImage image);
char *dia_image_filename(DiaImage image);

#endif /* DIA_IMAGE_H */


