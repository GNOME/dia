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
#ifndef RENDER_OBJECT_H
#define RENDER_OBJECT_H

#include "element.h"
#include "render_store.h"
#include "text.h"

typedef struct _RenderObjectDescriptor RenderObjectDescriptor;
typedef struct _RenderObject RenderObject;

struct _RenderObjectDescriptor {
  RenderStore *store;
  Point move_position; /* Relative 0.0 */
  real width;
  real height;
  real extra_border;

  /* text in object: */
  int use_text; 
  Point text_pos; /* position of text
		     (note: TOP of text, not ascent down) */
  Alignment initial_alignment;
  Font *initial_font;
  real initial_font_height;
  
  Point *connection_points;
  int num_connection_points;
  
  ObjectType *obj_type;
};

struct _RenderObject {
  /* Object must be first because this is a 'subclass' of it. */
  Element element;

  const RenderObjectDescriptor *desc;

  ConnectionPoint *connections;

  Text *text;
  
  real magnify;
};

extern Object *new_render_object(Point *startpoint,
				 Handle **handle1,
				 Handle **handle2,
				 const RenderObjectDescriptor *desc);
extern void render_object_save(RenderObject *rend_obj, ObjectNode obj_node);
extern Object *render_object_load(ObjectNode obj_node, 
				  const RenderObjectDescriptor *desc);
#endif /* RENDER_OBJECT_H */

