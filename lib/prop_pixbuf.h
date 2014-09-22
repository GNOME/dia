/* Dia -- a diagram creation/manipulation program -*- c -*-
 * Copyright (C) 1998 Alexander Larsson
 *
 * Property system for dia objects/shapes.
 * Copyright (C) 2000 James Henstridge
 * Copyright (C) 2001 Cyrille Chepelov
 * Major restructuration done in August 2001 by C. Chepelov
 *
 * Copyright (C) 2010 Hans Breuer
 * Property types for pixbuf.
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
#ifndef PROP_PIXBUF_H
#define PROP_PIXBUF_H

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "properties.h"
#include "dia_xml.h"

/*!
 * \brief Property for Pixbuf
 * \extends _Property
 */
typedef struct {
  Property common;
  GdkPixbuf *pixbuf; /* just the reference */
} PixbufProperty;

void prop_pixbuftypes_register(void);

gchar *pixbuf_encode_base64 (const GdkPixbuf *, const char *prefix);
GdkPixbuf *pixbuf_decode_base64 (const gchar *b64);

#endif /* PROP_PIXBUF_H */
