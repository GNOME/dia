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
#ifndef ELEMENT_H
#define ELEMENT_H

#include "object.h"
#include "handle.h"
#include "connectionpoint.h"

typedef struct _Element Element;

/* This is a subclass of Object used to help implementing objects
 * of a type with 8 handles around ..... more info here. */
struct _Element {
  /* Object must be first because this is a 'subclass' of it. */
  Object object;
  
  Handle resize_handles[8];

  Point corner;
  real width;
  real height;
};

extern void element_update_handles(Element *elem);
extern void element_update_boundingbox(Element *elem);
extern void element_init(Element *elem, int num_handles, int num_connections);
extern void element_destroy(Element *elem);
extern void element_copy(Element *from, Element *to);
extern void element_move_handle(Element *elem, HandleId id,
				Point *to, HandleMoveReason reason);
extern void element_move_handle_aspect(Element *elem, HandleId id,
				       Point *to, real aspect_ratio);

extern void element_save(Element *elem, ObjectNode obj_node);
extern void element_load(Element *elem, ObjectNode obj_node);

/* base property stuff ... */
#define ELEMENT_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "elem_corner", PROP_TYPE_POINT, 0, \
    "Element corner", "The corner of the element"}, \
  { "elem_width", PROP_TYPE_REAL, 0, \
    "Element width", "The width of the element"}, \
  { "elem_height", PROP_TYPE_REAL, 0, \
    "Element height", "The height of the element"}

#define ELEMENT_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "elem_corner", PROP_TYPE_POINT, offsetof(Element, corner) }, \
  { "elem_width", PROP_TYPE_REAL, offsetof(Element, width) }, \
  { "elem_height", PROP_TYPE_REAL, offsetof(Element, height) }

#endif /* ELEMENT_H */
