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
#include <config.h>

#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h> /* memcpy() */

#include "element.h"
#include "message.h"

void
element_update_boundingbox(Element *elem) {
  Rectangle *bb;
  Point *corner;
  ElementBBExtras *extra = &elem->extra_spacing;

  assert(elem != NULL);

  bb = &elem->object.bounding_box;
  corner = &elem->corner;
  bb->left = corner->x - extra->border_trans;
  bb->right = corner->x + elem->width + extra->border_trans;
  bb->top = corner->y - extra->border_trans;
  bb->bottom = corner->y + elem->height + extra->border_trans;
}

void
element_update_handles(Element *elem)
{
  Point *corner = &elem->corner;
  
  elem->resize_handles[0].id = HANDLE_RESIZE_NW;
  elem->resize_handles[0].pos.x = corner->x;
  elem->resize_handles[0].pos.y = corner->y;

  elem->resize_handles[1].id = HANDLE_RESIZE_N;
  elem->resize_handles[1].pos.x = corner->x + elem->width/2.0;
  elem->resize_handles[1].pos.y = corner->y;

  elem->resize_handles[2].id = HANDLE_RESIZE_NE;
  elem->resize_handles[2].pos.x = corner->x + elem->width;
  elem->resize_handles[2].pos.y = corner->y;

  elem->resize_handles[3].id = HANDLE_RESIZE_W;
  elem->resize_handles[3].pos.x = corner->x;
  elem->resize_handles[3].pos.y = corner->y + elem->height/2.0;

  elem->resize_handles[4].id = HANDLE_RESIZE_E;
  elem->resize_handles[4].pos.x = corner->x + elem->width;
  elem->resize_handles[4].pos.y = corner->y + elem->height/2.0;

  elem->resize_handles[5].id = HANDLE_RESIZE_SW;
  elem->resize_handles[5].pos.x = corner->x;
  elem->resize_handles[5].pos.y = corner->y + elem->height;

  elem->resize_handles[6].id = HANDLE_RESIZE_S;
  elem->resize_handles[6].pos.x = corner->x + elem->width/2.0;
  elem->resize_handles[6].pos.y = corner->y + elem->height;

  elem->resize_handles[7].id = HANDLE_RESIZE_SE;
  elem->resize_handles[7].pos.x = corner->x + elem->width;
  elem->resize_handles[7].pos.y = corner->y + elem->height;
}

void
element_move_handle(Element *elem, HandleId id,
		    Point *to, HandleMoveReason reason)
{
  Point p;
  Point *corner;
  
  assert(id>=HANDLE_RESIZE_NW);
  assert(id<=HANDLE_RESIZE_SE);

  corner = &elem->corner;
  
  p = *to;
  point_sub(&p, &elem->corner);

  switch(id) {
  case HANDLE_RESIZE_NW:
    if ( to->x < (corner->x+elem->width)) {
      corner->x += p.x;
      elem->width -= p.x;
    }
    if ( to->y < (corner->y+elem->height)) {
      corner->y += p.y;
      elem->height -= p.y;
    }
    break;
  case HANDLE_RESIZE_N:
    if ( to->y < (corner->y+elem->height)) {
      corner->y += p.y;
      elem->height -= p.y;
    }
    break;
  case HANDLE_RESIZE_NE:
    if (p.x>0.0) 
      elem->width = p.x;
    if ( to->y < (corner->y+elem->height)) {
      corner->y += p.y;
      elem->height -= p.y;
    }
    break;
  case HANDLE_RESIZE_W:
    if ( to->x < (corner->x+elem->width)) {
      corner->x += p.x;
      elem->width -= p.x;
    }
    break;
  case HANDLE_RESIZE_E:
    if (p.x>0.0) 
      elem->width = p.x;
    break;
  case HANDLE_RESIZE_SW:
    if ( to->x < (corner->x+elem->width)) {
      corner->x += p.x;
      elem->width -= p.x;
    }
    if (p.y>0.0)
      elem->height = p.y;
    break;
  case HANDLE_RESIZE_S:
    if (p.y>0.0)
      elem->height = p.y;
    break;
  case HANDLE_RESIZE_SE:
    if (p.x>0.0) 
      elem->width = p.x;
    if (p.y>0.0)
      elem->height = p.y;
    break;
  default:
    message_error("Error, called element_move_handle() with wrong handle-id\n");
  }
}

void
element_move_handle_aspect(Element *elem, HandleId id,
			   Point *to, real aspect_ratio)
{
  Point p;
  Point *corner;
  real width, height;
  real new_width, new_height;
  real move_x, move_y;
  
  assert(id>=HANDLE_RESIZE_NW);
  assert(id<=HANDLE_RESIZE_SE);

  corner = &elem->corner;
  
  p = *to;
  point_sub(&p, &elem->corner);
  
  width = elem->width;
  height = elem->height;

  new_width = 0.0;
  new_height = 0.0;
  
  
  switch(id) {
  case HANDLE_RESIZE_NW:
    new_width = width - p.x;
    new_height = height - p.y;
    move_x = 1.0;
    move_y = 1.0;
    break;
  case HANDLE_RESIZE_N:
    new_height = height - p.y;
    move_y = 1.0;
    move_x = 0.5;
    break;
  case HANDLE_RESIZE_NE:
    new_width = p.x;
    new_height = height - p.y;
    move_x = 0.0;
    move_y = 1.0;
    break;
  case HANDLE_RESIZE_W:
    new_width = width - p.x;
    move_x = 1.0;
    move_y = 0.5;
    break;
  case HANDLE_RESIZE_E:
    new_width = p.x;
    move_x = 0.0;
    move_y = 0.5;
    break;
  case HANDLE_RESIZE_SW:
    new_width = width - p.x;
    new_height = p.y;
    move_x = 1.0;
    move_y = 0.0;
    break;
  case HANDLE_RESIZE_S:
    new_height = p.y;
    move_x = 0.5;
    move_y = 0.0;
    break;
  case HANDLE_RESIZE_SE:
    new_width = p.x;
    new_height = p.y;
    move_x = 0.0;
    move_y = 0.0;
    break;
  default:
    message_error("Error, called element_move_handle() with wrong handle-id\n");
  }

  /* Which of the two versions to use: */
  if (new_width > new_height*aspect_ratio) {
    new_height = new_width/aspect_ratio;
  } else {
    new_width = new_height*aspect_ratio;
  }

  if ( (new_width<0.0) || (new_height<0.0)) {
    new_width = 0.0;
    new_height = 0.0;
  }
  
  corner->x -= (new_width - width)*move_x;
  corner->y -= (new_height - height)*move_y;
  
  elem->width  = new_width;
  elem->height = new_height;
}

/* Needs to have the first 8 handles */
void
element_init(Element *elem, int num_handles, int num_connections)
{
  Object *obj;
  int i;

  obj = &elem->object;

  assert(num_handles>=8);

  object_init(obj, num_handles, num_connections);

  for (i=0;i<8;i++) {
    obj->handles[i] = &elem->resize_handles[i];
    obj->handles[i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[i]->connected_to = NULL;
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
  }
}

void
element_copy(Element *from, Element *to)
{
  Object *toobj, *fromobj;
  int i;

  fromobj = &from->object;
  toobj = &to->object;

  object_copy(fromobj, toobj);
  
  to->corner = from->corner;
  to->width = from->width;
  to->height = from->height;
  
  for (i=0;i<8;i++) {
    to->resize_handles[i] = from->resize_handles[i];
    to->resize_handles[i].connected_to = NULL;
    toobj->handles[i] = &to->resize_handles[i];
  }
  memcpy(&to->extra_spacing,&from->extra_spacing,sizeof(to->extra_spacing));
}

void
element_destroy(Element *elem)
{
  object_destroy(&elem->object);
}


void element_save(Element *elem, ObjectNode obj_node)
{
  object_save(&elem->object, obj_node);

  data_add_point(new_attribute(obj_node, "elem_corner"),
		 &elem->corner);
  data_add_real(new_attribute(obj_node, "elem_width"),
		 elem->width);
  data_add_real(new_attribute(obj_node, "elem_height"),
		 elem->height);
}

void element_load(Element *elem, ObjectNode obj_node)
{
  AttributeNode attr;

  object_load(&elem->object, obj_node);

  elem->corner.x = 0.0;
  elem->corner.y = 0.0;
  attr = object_find_attribute(obj_node, "elem_corner");
  if (attr != NULL)
    data_point( attribute_first_data(attr), &elem->corner );

  elem->width = 1.0;
  attr = object_find_attribute(obj_node, "elem_width");
  if (attr != NULL)
    elem->width = data_real( attribute_first_data(attr));

  elem->height = 1.0;
  attr = object_find_attribute(obj_node, "elem_height");
  if (attr != NULL)
    elem->height = data_real( attribute_first_data(attr));

}
