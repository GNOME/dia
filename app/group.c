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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "object.h"
#include "object_ops.h"
#include "connectionpoint.h"
#include "render.h"
#include "group.h"
#include "object_ops.h"

typedef struct _Group Group;

struct _Group {
 /* Object must be first because this is a 'subclass' of it. */
  Object object;
  
  Handle resize_handles[8];

  GList *objects;
};

static real group_distance_from(Group *group, Point *point);
static void group_select(Group *group);
static void group_move_handle(Group *group, Handle *handle, Point *to);
static void group_move(Group *group, Point *to);
static void group_draw(Group *group, Renderer *renderer);
static void group_update_data(Group *group);
static void group_update_handles(Group *group);
static void group_destroy(Group *group);
static Object *group_copy(Group *group);

static ObjectOps group_ops = {
  (DestroyFunc)         group_destroy,
  (DrawFunc)            group_draw,
  (DistanceFunc)        group_distance_from,
  (SelectFunc)          group_select,
  (CopyFunc)            group_copy,
  (MoveFunc)            group_move,
  (MoveHandleFunc)      group_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

ObjectType group_type = {
  "Group",
  0,
  NULL
};

static real
group_distance_from(Group *group, Point *point)
{
  real dist;
  GList *list;
  Object *obj;

  dist = 100000.0;
  
  list = group->objects;
  while (list != NULL) {
    obj = (Object *) list->data;
    
    dist = MIN(dist, obj->ops->distance_from(obj, point));

    list = g_list_next(list);
  }
  
  return dist;
}

static void
group_select(Group *group)
{
  group_update_handles(group);
}

static void
group_update_handles(Group *group)
{
  Rectangle *bb = &group->object.bounding_box;
  
  group->resize_handles[0].id = HANDLE_RESIZE_NW;
  group->resize_handles[0].pos.x = bb->left;
  group->resize_handles[0].pos.y = bb->top;

  group->resize_handles[1].id = HANDLE_RESIZE_N;
  group->resize_handles[1].pos.x = (bb->left + bb->right) / 2.0;
  group->resize_handles[1].pos.y = bb->top;

  group->resize_handles[2].id = HANDLE_RESIZE_NE;
  group->resize_handles[2].pos.x = bb->right;
  group->resize_handles[2].pos.y = bb->top;

  group->resize_handles[3].id = HANDLE_RESIZE_W;
  group->resize_handles[3].pos.x = bb->left;
  group->resize_handles[3].pos.y = (bb->top + bb->bottom) / 2.0;

  group->resize_handles[4].id = HANDLE_RESIZE_E;
  group->resize_handles[4].pos.x = bb->right;
  group->resize_handles[4].pos.y = (bb->top + bb->bottom) / 2.0;

  group->resize_handles[5].id = HANDLE_RESIZE_SW;
  group->resize_handles[5].pos.x = bb->left;
  group->resize_handles[5].pos.y = bb->bottom;

  group->resize_handles[6].id = HANDLE_RESIZE_S;
  group->resize_handles[6].pos.x = (bb->left + bb->right) / 2.0;
  group->resize_handles[6].pos.y = bb->bottom;

  group->resize_handles[7].id = HANDLE_RESIZE_SE;
  group->resize_handles[7].pos.x = bb->right;
  group->resize_handles[7].pos.y = bb->bottom;
}

static void
group_move_handle(Group *group, Handle *handle, Point *to)
{
}

static void
group_move(Group *group, Point *to)
{
  Point delta,pos;

  delta = *to;
  pos = group->object.position;
  point_sub(&delta, &pos);
  
  object_list_move_delta(group->objects, &delta);
  
  group_update_data(group);
}

static void
group_draw(Group *group, Renderer *renderer)
{
  GList *list;
  Object *obj;

  list = group->objects;
  while (list != NULL) {
    obj = (Object *) list->data;
    
    obj->ops->draw(obj, renderer);

    list = g_list_next(list);
  }
}

void 
group_destroy_shallow(Object *group)
{
  if (group->handles)
    g_free(group->handles);

  if (group->connections)
    g_free(group->connections);

  g_free(group);
}

static void 
group_destroy(Group *group)
{
  object_destroy_list(group->objects);
  
  object_destroy(&group->object);
}

static Object *
group_copy(Group *group)
{
  Group *newgroup;
  Object *newobj, *obj;
  Object *listobj;
  GList *list;
  int num_conn, i;
    
  obj = &group->object;
  
  newgroup =  g_new(Group,1);
  newobj = &newgroup->object;

  object_copy(obj, newobj);
  
  for (i=0;i<8;i++) {
    newobj->handles[i] = &newgroup->resize_handles[i];
    newgroup->resize_handles[i] = group->resize_handles[i];
  }

  newgroup->objects = object_copy_list(group->objects);

  /* Build connectionpoints: */
  num_conn = 0;
  list = newgroup->objects;
  while (list != NULL) {
    listobj = (Object *) list->data;

    for (i=0;i<listobj->num_connections;i++) {
      newobj->connections[num_conn++] = listobj->connections[i];
    }
    
    list = g_list_next(list);
  }

  return (Object *)newgroup;
}


static void
group_update_data(Group *group)
{
  GList *list;
  Object *obj;

  if (group->objects != NULL) {
    list = group->objects;
    obj = (Object *) list->data;
    group->object.bounding_box = obj->bounding_box;

    list = g_list_next(list);
    while (list != NULL) {
      obj = (Object *) list->data;
      
      rectangle_union(&group->object.bounding_box, &obj->bounding_box);
      
      list = g_list_next(list);
    }
  }

  group->object.position.x = group->object.bounding_box.left;
  group->object.position.y = group->object.bounding_box.top;
  
  group_update_handles(group);
}

Object *
group_create(GList *objects)
{
  Group *group;
  Object *obj, *part_obj;
  int i;
  GList *list;
  int num_conn;
  
  group = g_new(Group,1);
  obj = &group->object;
  
  obj->type = &group_type;

  obj->ops = &group_ops;

  group->objects = objects;

  /* Make new connections as references to object connections: */
  num_conn = 0;
  list = objects;
  while (list != NULL) {
    part_obj = (Object *) list->data;

    /* break connections in to objects outside grouping */
    for (i=0;i<part_obj->num_handles;i++) {
      if ( (part_obj->handles[i]->connected_to != NULL) &&
	   (g_list_find(objects, part_obj->handles[i]->connected_to->object)==NULL) )
	object_unconnect(part_obj, part_obj->handles[i]);
    }

    num_conn += part_obj->num_connections;
    
    list = g_list_next(list);
  }
  
  object_init(obj, 8, num_conn);
  
  /* Make connectionpoints be that of the 'inner' objects: */
  num_conn = 0;
  list = objects;
  while (list != NULL) {
    part_obj = (Object *) list->data;

    for (i=0;i<part_obj->num_connections;i++) {
      obj->connections[num_conn++] = part_obj->connections[i];
    }
    
    list = g_list_next(list);
  }

  for (i=0;i<8;i++) {
    obj->handles[i] = &group->resize_handles[i];
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
    obj->handles[i]->connectable = FALSE;
    obj->handles[i]->connected_to = NULL;
  }
  
  group_update_data(group);
  return (Object *)group;
}

GList *
group_objects(Object *group)
{
  Group *g;
  g = (Group *)group;

  return g->objects;
}

