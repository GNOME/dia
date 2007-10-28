/* -*- Mode: C; c-basic-offset: 4 -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * autocad_pal.h: AutoCAD definitions for DXF Import
 * Copyright (C) 2002 Angus Ainslie 
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
#ifndef AUTOCAD_PAL_H
#define AUTOCAD_PAL_H

typedef struct 
{
   unsigned char r, g, b;
} RGB_t;

RGB_t pal_get_rgb   (int index);
int   pal_get_index (const RGB_t rgb);

#endif /* AUTOCAD_PAL_H */
