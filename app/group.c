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

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "object.h"
#include "object_ops.h"
#include "connectionpoint.h"
#include "render.h"
#include "group.h"
#include "object_ops.h"
#include "lib/properties.h"

typedef struct _Group Group;

struct _Group {
 /* Object must be first because this is a 'subclass' of it. */
  Object object;
  
  Handle resize_handles[8];

  GList *objects;

  PropDescription *pdesc;
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
static const PropDescription *group_describe_props(Group *group);
static void group_get_props(Group *group, GPtrArray *props);
static void group_set_props(Group *group, GPtrArray *props);

static ObjectOps group_ops = {
  (DestroyFunc)         group_destroy,
  (DrawFunc)            group_draw,
  (DistanceFunc)        group_distance_from,
  (SelectFunc)          group_select,
  (CopyFunc)            group_copy,
  (MoveFunc)            group_move,
  (MoveHandleFunc)      group_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   group_describe_props,
  (GetPropsFunc)        group_get_props,
  (SetPropsFunc)        group_set_props
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
    
    renderer->ops->draw_object(renderer, obj);

    list = g_list_next(list);
  }
}

void 
group_destroy_shallow(Object *obj)
{
  Group *group = (Group *)obj;
  if (obj->handles)
    g_free(obj->handles);

  if (obj->connections)
    g_free(obj->connections);

  g_list_free(group->objects);

  prop_desc_list_free_handler_chain(group->pdesc);
  g_free(group->pdesc);

  g_free(group);
}

static void 
group_destroy(Group *group)
{
  Object *obj = &group->object;
  
  destroy_object_list(group->objects);

  /* ConnectionPoints in the inner objects have already
     been unconnected and freed. */
  obj->num_connections = 0;
  
  prop_desc_list_free_handler_chain(group->pdesc);
  g_free(group->pdesc);

  object_destroy(obj);
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
  
  newgroup =  g_new0(Group,1);
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

  /* NULL out the property description field */
  newgroup->pdesc = NULL;

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

  obj = (Object *) group->objects->data;
  
  /* Move group by the point of the first object, otherwise a group
     with all objects on grid might be moved off grid. */
  group->object.position = obj->position;
  /* group->object.position.x = group->object.bounding_box.left;
    group->object.position.y = group->object.bounding_box.top; */
  
  group_update_handles(group);
}

/* Make sure there are no connections from objects to objects
 * outside of the created group.
 */
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

  group->pdesc = NULL;

  /* Make new connections as references to object connections: */
  num_conn = 0;
  list = objects;
  while (list != NULL) {
    part_obj = (Object *) list->data;

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
    obj->handles[i]->connect_type = HANDLE_NONCONNECTABLE;
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

static gboolean
group_prop_event_deliver(Group *group, Property *prop)
{
  GList *tmp;
  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    Object *obj = tmp->data;

    if (obj->ops->describe_props) {
      const PropDescription *pdesc,*plist;

      /* I'm sorry. I haven't found a working less ugly solution :( */
      plist = obj->ops->describe_props(obj);
      pdesc = prop_desc_list_find_prop(plist,prop->name);
      if (pdesc && pdesc->event_handler) {
        /* deliver */
        PropEventHandler hdl = prop_desc_find_real_handler(pdesc);
        if (hdl) {
          return hdl(obj,prop);
        } else {
          g_warning("dropped group event on prop %s, "
                    "final handler was NULL",prop->name);
          return FALSE;
        }
      }
    }
  }
  g_warning("undelivered group property event for prop %s",prop->name); 
  return FALSE;
}

static const PropDescription *
group_describe_props(Group *group)
{
  int i;
  if (group->pdesc == NULL) {
    GList *descs = NULL, *tmp;

    /* create list of property descriptions */
    for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
      const PropDescription *desc = NULL;
      Object *obj = tmp->data;

      desc = object_get_prop_descriptions(obj);

      if (desc) descs = g_list_append(descs, (gpointer)desc);
    }
    group->pdesc = prop_desc_lists_intersection(descs);
    g_list_free(descs); /* XXX: haven't we got a leak here ? */

    if (group->pdesc != NULL) {
      /* hijack event delivery */
      for (i=0; group->pdesc[i].name != NULL; i++) {
        if (group->pdesc[i].event_handler) 
          prop_desc_insert_handler(&group->pdesc[i],
                                   (PropEventHandler)group_prop_event_deliver);
      }
    }
  }

  return group->pdesc;
}

static void
group_get_props(Group *group, GPtrArray *props)
{
  GList *tmp;

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    Object *obj = tmp->data;

    if (obj->ops->get_props) {
      obj->ops->get_props(obj, props);
    }
  }
}

static void
group_set_props(Group *group, GPtrArray *props)
{
  GList *tmp;

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    Object *obj = tmp->data;

    if (obj->ops->set_props) {
      obj->ops->set_props(obj, props);
    }
  }
}
