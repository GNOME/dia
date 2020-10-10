/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * standard-path.c -- path based object drawing
 * Copyright (C) 2012 Hans Breuer <hans@breuer.org>
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

#pragma once

#include "diarenderer.h"

G_BEGIN_DECLS

#define DIA_TYPE_PATH_OBJECT_CHANGE dia_path_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaPathObjectChange,
                      dia_path_object_change,
                      DIA, PATH_OBJECT_CHANGE,
                      DiaObjectChange)


#define DIA_TYPE_PATH_TRANSFORM_OBJECT_CHANGE dia_path_transform_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaPathTransformObjectChange,
                      dia_path_transform_object_change,
                      DIA, PATH_TRANSFORM_OBJECT_CHANGE,
                      DiaObjectChange)


/* there should be no need to use DIAVAR */
extern DiaObjectType stdpath_type;

gboolean text_to_path (const Text *text, GArray *points);

G_END_DECLS
