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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>

#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "handle_ops.h"
#include "message.h"

#define OBJECT_CONNECT_DISTANCE 4.5

void
object_add_updates(DiaObject *obj, Diagram *dia)
{
  int i;

  /* Bounding box */
  if (obj->highlight_color != NULL) {
    diagram_add_update_with_border(dia, &obj->bounding_box, 5);
  } else {
    diagram_add_update(dia, &obj->bounding_box);
  }

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
  DiaObject *obj;
  
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    object_add_updates(obj, dia);
    
    list = g_list_next(list);
  }
}

ConnectionPoint *
object_find_connectpoint_display(DDisplay *ddisp, Point *pos, DiaObject *notthis)
{
  real distance;
  ConnectionPoint *connectionpoint;
  GList *avoid = NULL;
  
  distance =
    diagram_find_closest_connectionpoint(ddisp->diagram, &connectionpoint, 
					 pos, notthis);

  distance = ddisplay_transform_length(ddisp, distance);
  if (distance < OBJECT_CONNECT_DISTANCE) {
    return connectionpoint;
  }
  /* Try to find an all-object CP. */
  avoid = g_list_prepend(avoid, notthis);
  DiaObject *obj_here = 
    diagram_find_clicked_object_except(ddisp->diagram,
				       pos, 0.00001, avoid);
  if (obj_here != NULL) {
    int i;
    for (i = 0; i < obj_here->num_connections; i++) {
      if (obj_here->connections[i]->flags & CP_FLAG_ANYPLACE) {
	g_list_free(avoid);
	return obj_here->connections[i];
      }
    }
  }

  return NULL;
}

/* pushes undo info */
void
object_connect_display(DDisplay *ddisp, DiaObject *obj, Handle *handle)
{
  ConnectionPoint *connectionpoint;

  if (handle == NULL)
    return;

  if (handle->connected_to == NULL) {
    connectionpoint = object_find_connectpoint_display(ddisp, &handle->pos,
						       obj);
  
    if (connectionpoint != NULL) {
      Change *change = undo_connect(ddisp->diagram, obj, handle,
				    connectionpoint);
      (change->apply)(change, ddisp->diagram);
    }
  }
}

Point
object_list_corner(GList *list)
{
  Point p = {0.0,0.0};
  DiaObject *obj;

  if (list == NULL)
    return p;

  obj = (DiaObject *)list->data;
  p.x = obj->bounding_box.left;
  p.y = obj->bounding_box.top;

  list = g_list_next(list);
  
  while (list != NULL) {
    obj = (DiaObject *)list->data;

    if (p.x > obj->bounding_box.left)
      p.x = obj->bounding_box.left;
    if (p.y > obj->bounding_box.top)
      p.y = obj->bounding_box.top;
    
    list = g_list_next(list);
  }

  return p;
}

static int
object_list_sort_vertical(const void *o1, const void *o2) {
    DiaObject *obj1 = *(DiaObject **)o1;
    DiaObject *obj2 = *(DiaObject **)o2;

    return (obj1->bounding_box.bottom+obj1->bounding_box.top)/2 -
	(obj2->bounding_box.bottom+obj2->bounding_box.top)/2;
}

/*
  Align objects by moving them vertically:
*/
void
object_list_align_v(GList *objects, Diagram *dia, int align)
{
  GList *list;
  Point *orig_pos;
  Point *dest_pos;
  real y_pos = 0;
  DiaObject *obj;
  Point pos;
  real top, bottom, freespc;
  int nobjs;
  int i;
  gboolean sort_alloc = FALSE;

  if (objects==NULL)
    return;

  obj = (DiaObject *) objects->data; /*  First object */

  top = obj->bounding_box.top;
  bottom = obj->bounding_box.bottom;
  freespc = bottom - top;

  nobjs = 1;
  list = objects->next;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (obj->bounding_box.top < top)
      top = obj->bounding_box.top;
    if (obj->bounding_box.bottom > bottom)
      bottom = obj->bounding_box.bottom;

    freespc += obj->bounding_box.bottom - obj->bounding_box.top;
    nobjs++;

    list = g_list_next(list);
  }

  /*
   * These alignments can alter the order of elements, so we need
   * to sort them out by position.
   */
  if (align == DIA_ALIGN_EQUAL || align == DIA_ALIGN_ADJACENT) {
      DiaObject **object_array = (DiaObject **)g_malloc(sizeof(DiaObject*)*nobjs);
      int i = 0;

      list = objects;
      while (list != NULL) {
	  obj = (DiaObject *) list->data;
	  object_array[i] = obj;
	  i++;
	  list = g_list_next(list);
      }
      qsort(object_array, nobjs, sizeof(DiaObject*), object_list_sort_vertical);
      list = NULL;
      for (i = 0; i < nobjs; i++) {
	  list = g_list_append(list, object_array[i]);
      }
      objects = list;
      sort_alloc = TRUE; /* Must remember to free the list */
      g_free(object_array);
  }

  switch (align) {
  case DIA_ALIGN_TOP: /* TOP */
    y_pos = top;
    break;
  case DIA_ALIGN_CENTER: /* CENTER */
    y_pos = (top + bottom)/2.0;
    break;
  case DIA_ALIGN_BOTTOM: /* BOTTOM */
    y_pos = bottom;
    break;
  case DIA_ALIGN_POSITION: /* OBJECT POSITION */
    y_pos = (top + bottom)/2.0;
    break;
  case DIA_ALIGN_EQUAL: /* EQUAL DISTANCE */
    freespc = (bottom - top - freespc)/(double)(nobjs - 1);
    y_pos = top;
    break;
  case DIA_ALIGN_ADJACENT: /* ADJACENT */
    y_pos = top;
    break;
  default:
    message_warning("Wrong argument to object_list_align_v()\n");
  }
  
  dest_pos = g_new(Point, nobjs);
  orig_pos = g_new(Point, nobjs);

  i = 0;
  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    pos.x = obj->position.x;
    
    switch (align) {
    case DIA_ALIGN_TOP: /* TOP */
      pos.y = y_pos + obj->position.y - obj->bounding_box.top;
      break;
    case DIA_ALIGN_CENTER: /* CENTER */
      pos.y = y_pos + obj->position.y - (obj->bounding_box.top + obj->bounding_box.bottom)/2.0;
      break;
    case DIA_ALIGN_BOTTOM: /* BOTTOM */
      pos.y = y_pos - (obj->bounding_box.bottom - obj->position.y);
      break;
    case DIA_ALIGN_POSITION: /* OBJECT POSITION */
      pos.y = y_pos;
      break;
    case DIA_ALIGN_EQUAL: /* EQUAL DISTANCE */
      pos.y = y_pos + obj->position.y - obj->bounding_box.top;
      y_pos += obj->bounding_box.bottom - obj->bounding_box.top + freespc;
      break;
    case DIA_ALIGN_ADJACENT: /* ADJACENT */
      pos.y = y_pos + obj->position.y - obj->bounding_box.top;
      y_pos += obj->bounding_box.bottom - obj->bounding_box.top;
      break;
    }

    orig_pos[i] = obj->position;
    dest_pos[i] = pos;
    
    obj->ops->move(obj, &pos);

    i++;
    list = g_list_next(list);
  }
  
  undo_move_objects(dia, orig_pos, dest_pos, g_list_copy(objects));
  if (sort_alloc)
    g_list_free(objects);
}


static int
object_list_sort_horizontal(const void *o1, const void *o2) {
  DiaObject *obj1 = *(DiaObject **)o1;
  DiaObject *obj2 = *(DiaObject **)o2;

  return (obj1->bounding_box.right+obj1->bounding_box.left)/2 -
    (obj2->bounding_box.right+obj2->bounding_box.left)/2;
}

/*
  Align objects by moving then horizontally
*/
void
object_list_align_h(GList *objects, Diagram *dia, int align)
{
  GList *list;
  Point *orig_pos;
  Point *dest_pos;
  real x_pos = 0;
  DiaObject *obj;
  Point pos;
  real left, right, freespc = 0;
  int nobjs;
  int i;
  gboolean sort_alloc = FALSE;

  if (objects==NULL)
    return;

  obj = (DiaObject *) objects->data; /*  First object */

  left = obj->bounding_box.left;
  right = obj->bounding_box.right;
  freespc = right - left;

  nobjs = 1;
  list = objects->next;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    if (obj->bounding_box.left < left)
      left = obj->bounding_box.left;
    if (obj->bounding_box.right > right)
      right = obj->bounding_box.right;

    freespc += obj->bounding_box.right - obj->bounding_box.left;
    nobjs++;

    list = g_list_next(list);
  }

  /*
   * These alignments can alter the order of elements, so we need
   * to sort them out by position.
   */
  if (align == DIA_ALIGN_EQUAL || align == DIA_ALIGN_ADJACENT) {
    DiaObject **object_array = (DiaObject **)g_malloc(sizeof(DiaObject*)*nobjs);
    int i = 0;

    list = objects;
    while (list != NULL) {
      obj = (DiaObject *) list->data;
      object_array[i] = obj;
      i++;
      list = g_list_next(list);
    }
    qsort(object_array, nobjs, sizeof(DiaObject*), object_list_sort_horizontal);
    list = NULL;
    for (i = 0; i < nobjs; i++) {
      list = g_list_append(list, object_array[i]);
    }
    objects = list;
    sort_alloc = TRUE;
    g_free(object_array);
  }

  switch (align) {
  case DIA_ALIGN_LEFT:
    x_pos = left;
    break;
  case DIA_ALIGN_CENTER:
    x_pos = (left + right)/2.0;
    break;
  case DIA_ALIGN_RIGHT:
    x_pos = right;
    break;
  case DIA_ALIGN_POSITION:
    x_pos = (left + right)/2.0;
    break;
  case DIA_ALIGN_EQUAL:
    freespc = (right - left - freespc)/(double)(nobjs - 1);
    x_pos = left;
    break;
  case DIA_ALIGN_ADJACENT:
    x_pos = left;
    break;
  default:
    message_warning("Wrong argument to object_list_align_h()\n");
  }

  dest_pos = g_new(Point, nobjs);
  orig_pos = g_new(Point, nobjs);

  i = 0;
  list = objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    switch (align) {
    case DIA_ALIGN_LEFT:
      pos.x = x_pos + obj->position.x - obj->bounding_box.left;
      break;
    case DIA_ALIGN_CENTER:
      pos.x = x_pos + obj->position.x - (obj->bounding_box.left + obj->bounding_box.right)/2.0;
      break;
    case DIA_ALIGN_RIGHT:
      pos.x = x_pos - (obj->bounding_box.right - obj->position.x);
      break;
    case DIA_ALIGN_POSITION:
      pos.x = x_pos;
      break;
    case DIA_ALIGN_EQUAL:
      pos.x = x_pos + obj->position.x - obj->bounding_box.left;
      x_pos += obj->bounding_box.right - obj->bounding_box.left + freespc;
      break;
    case DIA_ALIGN_ADJACENT:
      pos.x = x_pos + obj->position.x - obj->bounding_box.left;
      x_pos += obj->bounding_box.right - obj->bounding_box.left;
      break;
    }
    
    pos.y = obj->position.y;
    
    orig_pos[i] = obj->position;
    dest_pos[i] = pos;

    obj->ops->move(obj, &pos);

    i++;
    list = g_list_next(list);
  }
    
  undo_move_objects(dia, orig_pos, dest_pos, g_list_copy(objects)); 
  if (sort_alloc)
    g_list_free(objects);
}
