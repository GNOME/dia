/* xxxxxx -- an diagram creation/manipulation program
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
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"

#define OBJECT_CONNECT_DISTANCE 4.5

void
object_add_updates(Object *obj, Diagram *dia)
{
  int i;

  /* Bounding box */
  diagram_add_update(dia, &obj->bounding_box);

  /* Handles */
  for (i=0;i<obj->num_handles;i++) {
    handle_add_update(obj->handles[i], dia);
  }

  /* Connection points */
  for (i=0;i<obj->num_connections;i++) {
    connectionpoint_add_update(obj->connections[i], dia);
  }

}

void
object_add_updates_list(GList *list, Diagram *dia)
{
  Object *obj;
  
  while (list != NULL) {
    obj = (Object *)list->data;

    object_add_updates(obj, dia);
    
    list = g_list_next(list);
  }
}

ConnectionPoint *
object_find_connectpoint_display(DDisplay *ddisp, Point *pos)
{
  real distance;
  ConnectionPoint *connectionpoint;
  
  distance =
    diagram_find_closest_connectionpoint(ddisp->diagram, &connectionpoint, pos);

  distance = ddisplay_transform_length(ddisp, distance);
  if (distance < OBJECT_CONNECT_DISTANCE) {
    return connectionpoint;
  }

  return NULL;
}

void
object_connect_display(DDisplay *ddisp, Object *obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  if (handle == NULL)
    return;

  if (handle->connected_to == NULL) {
    connectionpoint = object_find_connectpoint_display(ddisp, &handle->pos);
  
    if (connectionpoint != NULL) {
      object_connect(obj, handle, connectionpoint);
      
      object_add_updates(obj, ddisp->diagram);
      obj->ops->move_handle(obj, handle , &connectionpoint->pos,
			    HANDLE_MOVE_CONNECTED);
      object_add_updates(obj, ddisp->diagram);
    }
  }
}

static guint
pointer_hash(gpointer some_pointer)
{
  return (guint) some_pointer;
}

GList *
object_copy_list(GList *list_orig)
{
  GList *list_copy;
  GList *list;
  Object *obj;
  Object *obj_copy;
  GHashTable *hash_table;
  int i;

  hash_table = g_hash_table_new((GHashFunc) pointer_hash, NULL);

  list = list_orig;
  list_copy = NULL;
  while (list != NULL) {
    obj = (Object *)list->data;
    obj_copy = obj->ops->copy(obj);

    g_hash_table_insert(hash_table, obj, obj_copy);
    
    list_copy = g_list_append(list_copy, obj_copy);
    
    list = g_list_next(list);
  }

  /* Rebuild the connections between the objects in the list: */
  list = list_orig;
  while (list != NULL) {
    obj = (Object *)list->data;
    obj_copy = g_hash_table_lookup(hash_table, obj);
    
    for (i=0;i<obj->num_handles;i++) {
      ConnectionPoint *con_point;
      con_point = obj->handles[i]->connected_to;
      
      if ( con_point != NULL ) {
	Object *other_obj;
	Object *other_obj_copy;
	int con_point_nr;
	
	other_obj = con_point->object;
	other_obj_copy = g_hash_table_lookup(hash_table, other_obj);

	if (other_obj_copy == NULL)
	  break; /* other object was not on list. */
	
	con_point_nr=0;
	while (other_obj->connections[con_point_nr] != con_point) {
	  con_point_nr++;
	}

	object_connect(obj_copy, obj_copy->handles[i],
		       other_obj_copy->connections[con_point_nr]);
      }
    }
    
    list = g_list_next(list);
  }
  
  g_hash_table_destroy(hash_table);
  
  return list_copy;
}

Point
object_list_corner(GList *list)
{
  Point p = {0.0,0.0};
  Object *obj;

  if (list == NULL)
    return p;

  obj = (Object *)list->data;
  p.x = obj->bounding_box.left;
  p.y = obj->bounding_box.top;

  list = g_list_next(list);
  
  while (list != NULL) {
    obj = (Object *)list->data;

    if (p.x > obj->bounding_box.left)
      p.x = obj->bounding_box.left;
    if (p.y > obj->bounding_box.top)
      p.y = obj->bounding_box.top;
    
    list = g_list_next(list);
  }

  return p;
}

extern void
object_list_move_delta(GList *objects, Point *delta)
{
  GList *list;
  Object *obj;
  Point pos;

  list = objects;
  while (list != NULL) {
    obj = (Object *) list->data;
    
    pos = obj->position;
    point_add(&pos, delta);

    obj->ops->move(obj, &pos);

    list = g_list_next(list);
  }
}


/* Align. Maybe this enum should be in a header file
 
enum {
    DIA_NO_HALIGN=0,
    DIA_ALIGN_LEFT,
    DIA_ALIGN_HCENTER,
    DIA_ALIGN_RIGHT,
    DIA_NO_VALIGN=0,
    DIA_ALIGN_TOP=4,
    DIA_ALIGN_VCENTER=8,
    DIA_ALIGN_BOTTOM=12
};

 Todo:
 * H eq. distance
 * v eq. distance
 */


void
object_list_align(GList *objects, int align)
{
  GList *list;
  Object *obj;
  Point pos;
  real dx;
  int halign, valign;
  real w=0, h=0, x, y, x1 = 1000, y1 = 1000, x2 = -1000, y2 = -1000;  

  list = objects;
  
  halign = align & 3;
  valign = (align >> 2) & 3;
    
  /* get block's corners */
  while (list != NULL) {
    obj = (Object *) list->data;
    
    pos = obj->position;
    w = obj->bounding_box.right - obj->bounding_box.left;
    h = obj->bounding_box.bottom - obj->bounding_box.top;
           
    if (x1 > pos.x) x1 = pos.x;
    if (y1 > pos.y) y1 = pos.y;
    if (x2 < pos.x+w) x2 = pos.x+w;
    if (y2 < pos.y+h) y2 = pos.y+h; 
      
    list = g_list_next(list);
  }
    
  list = objects;

  x = x1 + (x2 - x1)*(halign-1)/2;
  y = y1 + (y2 - y1)*(valign-1)/2;
    
  /* Move objects */
  while (list != NULL) {
    obj = (Object *) list->data;
    pos = obj->position;
      
    if (halign > 0) {  
	dx = -(halign-1)*(obj->bounding_box.right - obj->bounding_box.left)/2;
	pos.x = x + dx;
    }

    if (valign > 0) {  
	dx = -(valign-1)*(obj->bounding_box.bottom - obj->bounding_box.top)/2;
	pos.y = y + dx;
    }
      
    obj->ops->move(obj, &pos);
    list = g_list_next(list);
  }
}

