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

/* ** \file element.h -- Definition of a diagram - usual rectangular - object with eight handles  */
#pragma once

#include "diatypes.h"
#include "object.h"
#include "handle.h"
#include "connectionpoint.h"
#include "boundingbox.h"
#include "properties.h" /* win32: PropNumData */

G_BEGIN_DECLS

#define DIA_TYPE_ELEMENT_OBJECT_CHANGE dia_element_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaElementObjectChange, dia_element_object_change, DIA, ELEMENT_OBJECT_CHANGE, DiaObjectChange)


/**
 * Element:
 * @resize_handles: not only for resizing but may also be used for connections
 * @corner: upper-left corner of the #Element
 * @width: width of the object (with 0 line width)
 * @height: height of the object (with 0 line width)
 * @extra_spacing: extra data used for bounding box calculation, filled from
 *                 line width
 *
 * Beside #OrthConn one of the most use object classes
 *
 * This is a subclass of #DiaObject used to help implementing objects
 * of a type with 8 handles.
 */
struct _Element {
  /* < private > */
  DiaObject object;

  /* < public > */
  Handle resize_handles[8];

  Point corner;
  double width;
  double height;
  ElementBBExtras extra_spacing;
};


/*! \protected Update internal state after property change */
void element_update_handles(Element *elem);
void element_update_connections_rectangle(Element *elem,
					  ConnectionPoint *cps);
void element_update_connections_directions (Element *elem,
                                            ConnectionPoint *cps);
void element_update_boundingbox(Element *elem);
void element_init(Element *elem, int num_handles, int num_connections);
void element_destroy(Element *elem);
void element_copy(Element *from, Element *to);
DiaObjectChange *element_move_handle           (Element          *elem,
                                                HandleId          id,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
void element_move_handle_aspect(Element *elem, HandleId id,
				Point *to, real aspect_ratio);

void element_save(Element *elem, ObjectNode obj_node, DiaContext *ctx);
void element_load(Element *elem, ObjectNode obj_node, DiaContext *ctx);

DiaObjectChange *element_change_new            (const Point      *corner,
                                                double            width,
                                                double            height,
                                                Element          *elem);

void element_get_poly (const Element *elem, real angle, Point corners[4]);


/* base property stuff ... */
#ifdef G_OS_WIN32
/* see lib/properties.h for the reason */
static PropNumData width_range = { -G_MAXFLOAT, G_MAXFLOAT, 0.1};
#else
/* use extern on Linux/gcc to avoid
 * warning: 'width_range' defined but not used */
extern PropNumData width_range;
#endif

#define ELEMENT_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  { "elem_corner", PROP_TYPE_POINT, PROP_FLAG_NO_DEFAULTS, \
    "Element corner", "The corner of the element"}, \
  { "elem_width", PROP_TYPE_REAL, PROP_FLAG_DONT_MERGE | PROP_FLAG_NO_DEFAULTS | PROP_FLAG_OPTIONAL, \
    "Element width", "The width of the element", &width_range}, \
  { "elem_height", PROP_TYPE_REAL, PROP_FLAG_DONT_MERGE  | PROP_FLAG_NO_DEFAULTS | PROP_FLAG_OPTIONAL, \
    "Element height", "The height of the element", &width_range}

   /* Would like to have the frame, but need to figure out why
      custom_object ext_attributes lose their updates when they're on
      (see http://mail.gnome.org/archives/dia-list/2005-August/msg00053.html)
   */
      /*
#define ELEMENT_COMMON_PROPERTIES \
  OBJECT_COMMON_PROPERTIES, \
  PROP_FRAME_BEGIN("size",0,N_("Object dimensions")), \
  { "elem_corner", PROP_TYPE_POINT, 0, \
    "Element corner", "The corner of the element"}, \
  { "elem_width", PROP_TYPE_REAL, PROP_FLAG_VISIBLE, \
    "Element width", "The width of the element", &width_range}, \
  { "elem_height", PROP_TYPE_REAL, PROP_FLAG_VISIBLE, \
    "Element height", "The height of the element", &width_range}, \
  PROP_FRAME_END("size", 0)
      */
#define ELEMENT_COMMON_PROPERTIES_OFFSETS \
  OBJECT_COMMON_PROPERTIES_OFFSETS, \
  { "elem_corner", PROP_TYPE_POINT, offsetof(Element, corner) }, \
  { "elem_width", PROP_TYPE_REAL, offsetof(Element, width) }, \
  { "elem_height", PROP_TYPE_REAL, offsetof(Element, height) }

G_END_DECLS
