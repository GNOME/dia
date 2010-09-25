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
#include "connectionpoint.h"
#include "group.h"
#include "properties.h"
#include "diarenderer.h"
#include "message.h"

struct _Group {
 /* DiaObject must be first because this is a 'subclass' of it. */
  DiaObject object;
  
  Handle resize_handles[8];

  GList *objects;

  const PropDescription *pdesc;
  
  DiaMatrix *matrix;

  gboolean self_only;
};

typedef struct _GroupPropChange GroupPropChange;
struct _GroupPropChange {
  ObjectChange obj_change;
  Group *group;
  GList *changes_per_object;
};

static GroupPropChange* group_apply_properties_list(Group *group, GPtrArray *props);
static void group_prop_change_apply(GroupPropChange *change, DiaObject *obj);
static void group_prop_change_revert(GroupPropChange *change, DiaObject *obj);
static void group_prop_change_free(GroupPropChange *change);

static real group_distance_from(Group *group, Point *point);
static void group_select(Group *group);
static ObjectChange* group_move_handle(Group *group, Handle *handle, Point *to, ConnectionPoint *cp,
					         HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* group_move(Group *group, Point *to);
static void group_draw(Group *group, DiaRenderer *renderer);
static void group_update_data(Group *group);
static void group_update_handles(Group *group);
static void group_update_connectionpoints(Group *group);
static void group_destroy(Group *group);
static DiaObject *group_copy(Group *group);
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   group_describe_props,
  (GetPropsFunc)        group_get_props,
  (SetPropsFunc)        group_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) group_apply_properties_list,
};

DiaObjectType group_type = {
  "Group",
  0,
  NULL
};

static real
group_distance_from(Group *group, Point *point)
{
  real dist;
  GList *list;
  DiaObject *obj;
  Point tp = *point;

  dist = 100000.0;
  
  if (group->matrix) {
    DiaMatrix mi = *group->matrix;

    if (cairo_matrix_invert ((cairo_matrix_t *)&mi) != CAIRO_STATUS_SUCCESS)
      g_warning ("Group::distance_from() matrix invert");
    tp.x = point->x * mi.xx + point->y * mi.xy + mi.x0;
    tp.y = point->x * mi.yx + point->y * mi.yy + mi.y0;
  }

  list = group->objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;
    
    dist = MIN(dist, obj->ops->distance_from(obj, &tp));

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

/*! \brief Update connection points positions of contained objects
 *  BEWARE: must only be called _once_ per update run!
 */
static void 
group_update_connectionpoints(Group *group)
{
#if 0
  /* FIXME: can't do it this way - lines are transformed twice than */
  /* Also can't do it by just copying the original connection points
   * into this object, e.g. connect() must work on the original cp.
   * So time for some serious interface update ...
   */
  if (group->matrix) {
    DiaObject *obj = &group->object;
    int i;

    for (i = 0; i < obj->num_connections; ++i) {
      ConnectionPoint *cp = obj->connections[i];
      DiaMatrix *m = group->matrix;
      Point p = cp->pos;

      cp->pos.x = p.x * m->xx + p.y * m->xy + m->x0;
      cp->pos.y = p.x * m->yx + p.y * m->yy + m->y0;
    }
  }
#endif
}

static ObjectChange*
group_move_handle(Group *group, Handle *handle, Point *to, ConnectionPoint *cp,
		      HandleMoveReason reason, ModifierKeys modifiers)
{
  DiaObject *obj = &group->object;
  Rectangle *bb = &obj->bounding_box;
  /* top and left handles are also changing the objects position */
  Point pos = obj->position;
  /* only flies, when the top left handle is the position */
  gboolean also_move = (   group->resize_handles[0].pos.x == pos.x 
                        && group->resize_handles[0].pos.y == pos.y);
  Point top_left = group->resize_handles[0].pos;
  Point delta;
  /* before and after width and height */
  real w0, h0, w1, h1;
  
  assert(handle->id>=HANDLE_RESIZE_NW);
  assert(handle->id<=HANDLE_RESIZE_SE);

  w0 = w1 = bb->right - bb->left;
  h0 = h1 = bb->bottom - bb->top;

  switch(handle->id) {
  case HANDLE_RESIZE_NW:
    delta.x = to->x - top_left.x;
    delta.y = to->y - top_left.y;
    w1 = w0 - delta.x;
    h1 = h0 - delta.y;
    break;
  case HANDLE_RESIZE_NE:
    w1 = to->x - pos.x;
    /* fall through */
  case HANDLE_RESIZE_N:
    delta.y = to->y - top_left.y;
    h1 = h0 - delta.y;
    break;
  case HANDLE_RESIZE_SW:
    h1 = to->y - pos.y;
    /* fall through */
  case HANDLE_RESIZE_W:
    delta.x = to->x - top_left.x;
    w1 = w0 - delta.x;
    break;
  case HANDLE_RESIZE_SE:
    w1 = to->x - top_left.x;
    h1 = to->y - top_left.y;
    break;
  case HANDLE_RESIZE_S:
    h1 = to->y - top_left.y;
    break;
  case HANDLE_RESIZE_E:
    w1 = to->x - top_left.x;
    break;
  default:
    message_error("Error, called group_move_handle() with wrong handle-id\n");
  }

  if (also_move)
    object_list_move_delta(group->objects, &delta);
  if (!group->matrix) {
    group->matrix = g_new0 (DiaMatrix, 1);
    group->matrix->xx = 1.0;
    group->matrix->yy = 1.0;
  }
  group->matrix->xx *= (w1 / w0);
  group->matrix->yy *= (h1 / h0);

  group_update_data(group);

  return NULL;
}

static ObjectChange*
group_move(Group *group, Point *to)
{
  Point delta,pos;

  delta = *to;
  pos = group->object.position;
  point_sub(&delta, &pos);
  
  /* We don't need any transformation of delta, because 
   * group_update_data () maintains the relative position.
   */
  object_list_move_delta(group->objects, &delta);
  
  group_update_data(group);

  return NULL;
}

static void
group_draw(Group *group, DiaRenderer *renderer)
{
  GList *list;
  DiaObject *obj;

  list = group->objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    DIA_RENDERER_GET_CLASS(renderer)->draw_object(renderer, obj, group->matrix);
    list = g_list_next(list);
  }
}

void 
group_destroy_shallow(DiaObject *obj)
{
  Group *group = (Group *)obj;
  if (obj->handles)
    g_free(obj->handles);

  if (obj->connections)
    g_free(obj->connections);

  g_list_free(group->objects);

  prop_desc_list_free_handler_chain((PropDescription *)group->pdesc);
  g_free((PropDescription *)group->pdesc);

  g_free(group);
}

static void 
group_destroy(Group *group)
{
  DiaObject *obj = &group->object;

  destroy_object_list(group->objects);

  /* ConnectionPoints in the inner objects have already
     been unconnected and freed. */
  obj->num_connections = 0;
  
  prop_desc_list_free_handler_chain((PropDescription *)group->pdesc);
  g_free((PropDescription *)group->pdesc);

  g_free (group->matrix);

  object_destroy(obj);
}
/*! Accessor for visible/usable number of connections for app/ 
 *
 * Should probably become a DiaObject member when more objects need
 * such special handling.
 */
int
dia_object_get_num_connections (DiaObject *obj)
{
  if (IS_GROUP(obj)) {
    if (((Group*)obj)->matrix)
      return 0;
  }

  return obj->num_connections;
}

static DiaObject *
group_copy(Group *group)
{
  Group *newgroup;
  DiaObject *newobj, *obj;
  DiaObject *listobj;
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
  
  newgroup->matrix = g_memdup(group->matrix, sizeof(DiaMatrix));

  newgroup->objects = object_copy_list(group->objects);

  /* Build connectionpoints: */
  num_conn = 0;
  list = newgroup->objects;
  while (list != NULL) {
    listobj = (DiaObject *) list->data;

    for (i=0;i<dia_object_get_num_connections(listobj);i++) {
      /* Make connectionpoints be that of the 'inner' objects: */
      newobj->connections[num_conn++] = listobj->connections[i];
    }
    
    list = g_list_next(list);
  }

  /* NULL out the property description field */
  newgroup->pdesc = NULL;

  return (DiaObject *)newgroup;
}


static void
group_update_data(Group *group)
{
  GList *list;
  DiaObject *obj;

  if (group->objects != NULL) {
    list = group->objects;
    obj = (DiaObject *) list->data;
    group->object.bounding_box = obj->bounding_box;

    list = g_list_next(list);
    while (list != NULL) {
      obj = (DiaObject *) list->data;
      
      rectangle_union(&group->object.bounding_box, &obj->bounding_box);
      
      list = g_list_next(list);
    }

    obj = (DiaObject *) group->objects->data;
    
    /* Move group by the point of the first object, otherwise a group
       with all objects on grid might be moved off grid. */
    group->object.position = obj->position;

    if (group->matrix) {
      Point p;
      Rectangle box;
      Rectangle *bb = &group->object.bounding_box;
      DiaMatrix *m = group->matrix;

      /* maintain obj->position */
      m->x0 = obj->position.x - (m->xx * obj->position.x + m->xy  * obj->position.y);
      m->y0 = obj->position.y - (m->yx * obj->position.x + m->yy  * obj->position.y);

      /* recalculate bounding box */
      /* need to consider all corners */
      p.x = m->xx * bb->left + m->xy * bb->top + m->x0;
      p.y = m->yx * bb->left + m->yy * bb->top + m->y0;
      box.left = box.right = p.x;
      box.top = box.bottom = p.y;

      p.x = m->xx * bb->right + m->xy * bb->top + m->x0;
      p.y = m->yx * bb->right + m->yy * bb->top + m->y0;
      rectangle_add_point (&box, &p);

      p.x = m->xx * bb->right + m->xy * bb->bottom + m->x0;
      p.y = m->yx * bb->right + m->yy * bb->bottom + m->y0;
      rectangle_add_point (&box, &p);

      p.x  = m->xx * bb->left + m->xy * bb->bottom + m->x0;
      p.y = m->yx * bb->left + m->yy * bb->bottom + m->y0;
      rectangle_add_point (&box, &p);
      /* the new one does not necessarily include the old one */
      group->object.bounding_box = box;
    }

    group_update_connectionpoints(group);
    group_update_handles(group);
  }
}

DiaObject *
group_create_with_matrix(GList *objects, DiaMatrix *matrix)
{
  Group *group = (Group *)group_create(objects);
  if (matrix->x0 != 0 || matrix->y0 != 0) {
    Point delta = {matrix->x0, matrix->y0};
    matrix->x0 = matrix->y0 = 0; /* offset used internally */
    object_list_move_delta(group->objects, &delta);
  }
  group->matrix = matrix;
  group_update_data(group);
  return &group->object;
}
/* Make sure there are no connections from objects to objects
 * outside of the created group.
 */
DiaObject *
group_create(GList *objects)
{
  Group *group;
  DiaObject *obj, *part_obj;
  int i;
  GList *list;
  int num_conn;

  group = g_new0(Group,1);
  obj = &group->object;

  obj->type = &group_type;
  
  obj->ops = &group_ops;

  group->objects = objects;

  group->matrix = NULL;

  group->pdesc = NULL;

  /* Make new connections as references to object connections: */
  num_conn = 0;
  list = objects;
  while (list != NULL) {
    part_obj = (DiaObject *) list->data;

    num_conn += dia_object_get_num_connections(part_obj);
    
    list = g_list_next(list);
  }
  
  object_init(obj, 8, num_conn);

  /* Make connectionpoints be that of the 'inner' objects: */
  num_conn = 0;
  list = objects;
  while (list != NULL) {
    part_obj = (DiaObject *) list->data;

    for (i=0;i<dia_object_get_num_connections(part_obj);i++) {
      obj->connections[num_conn++] = part_obj->connections[i];
    }
    
    list = g_list_next(list);
  }

  for (i=0;i<8;i++) {
    obj->handles[i] = &group->resize_handles[i];
    obj->handles[i]->type = HANDLE_MAJOR_CONTROL;
    obj->handles[i]->connect_type = HANDLE_NONCONNECTABLE;
    obj->handles[i]->connected_to = NULL;
  }

  group_update_data(group);
  return (DiaObject *)group;
}

GList *
group_objects(DiaObject *group)
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
    DiaObject *obj = tmp->data;

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

static PropDescription _group_props[] = {
  { "matrix", PROP_TYPE_MATRIX, PROP_FLAG_DONT_MERGE|PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS,
  N_("Transformation"), NULL, NULL },

  PROP_DESC_END
};

static const PropDescription *
group_describe_props(Group *group)
{
  int i;

  if (_group_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(_group_props);
  }
  if (group->pdesc == NULL) {

    /* create list of property descriptions. this must be freed by the
       DestroyFunc. */
    group->pdesc = object_list_get_prop_descriptions(group->objects, PROP_UNION);

    if (group->pdesc != NULL) {
      /* hijack event delivery */
      for (i=0; group->pdesc[i].name != NULL; i++) {
        if (group->pdesc[i].event_handler) 
          prop_desc_insert_handler((PropDescription *)&group->pdesc[i],
                                   (PropEventHandler)group_prop_event_deliver);
      }
      /* mix in our own single matrix property */
      {
        PropDescription *arr = g_new (PropDescription, i+2);
	
	arr[0] = _group_props[0];
	memcpy (&arr[1], group->pdesc, (i+1) * sizeof(PropDescription));
	for (i=1; arr[i].name != NULL; ++i) {
	  /* these are not meant to be saved/loaded with the group */
	  arr[i].flags |= (PROP_FLAG_DONT_SAVE|PROP_FLAG_OPTIONAL);
	}
	g_free ((PropDescription *)group->pdesc);
	group->pdesc = arr;
      }
    }
  }

  return group->pdesc;
}

static PropOffset _group_offsets[] = {
  { "matrix", PROP_TYPE_MATRIX, offsetof(Group, matrix) },
  { NULL }
};

static void
group_get_props(Group *group, GPtrArray *props)
{
  GList *tmp;

  /* FIXME: need to somehow avoid to ask included groups for matrix, too. */
  object_get_props_from_offsets(&group->object, _group_offsets, props);

  if (group->self_only)
    return;

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;

    if (obj->ops->get_props) {
      obj->ops->get_props(obj, props);
    }
  }
}

static void
group_set_props(Group *group, GPtrArray *props)
{
  GList *tmp;

  /* FIXME: need to somehow avoid to set included groups for matrix, too. */
  object_set_props_from_offsets(&group->object, _group_offsets, props);

  if (group->self_only)
    return;

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;

    if (obj->ops->set_props) {
      obj->ops->set_props(obj, props);
    }
  }
  group_update_data (group);
}

GroupPropChange *
group_apply_properties_list(Group *group, GPtrArray *props)
{
  GList *tmp = NULL;
  GList *clist = NULL;
  ObjectChange *objchange;
  GroupPropChange *change = NULL;
  change = g_new0(GroupPropChange, 1);


  change->obj_change.apply =
    (ObjectChangeApplyFunc) group_prop_change_apply;
  change->obj_change.revert =
    (ObjectChangeRevertFunc) group_prop_change_revert;
  change->obj_change.free =
    (ObjectChangeFreeFunc) group_prop_change_free;

  change->group = group;

  for (tmp = group->objects; tmp != NULL; tmp = g_list_next(tmp)) {
    DiaObject *obj = (DiaObject*)tmp->data;
    objchange = NULL;
    
    objchange = obj->ops->apply_properties_list(obj, props);
    clist = g_list_append(clist, objchange);
  }
  /* finally ourself */
  group->self_only = TRUE;
  objchange = object_apply_props (&group->object, props);
  group->self_only = FALSE;
  clist = g_list_append(clist, objchange);

  group_update_data (group);

  change->changes_per_object = clist;

  return change;
}

static void
group_prop_change_apply(GroupPropChange *change, DiaObject *obj)
{
  GList *tmp;

  for (tmp = change->changes_per_object; tmp != NULL;
       tmp = g_list_next(tmp)) {
    ObjectChange *obj_change = (ObjectChange*)tmp->data;

    /*
      This call to apply() depends on the fact that it is actually a
      call to object_prop_change_apply_revert(), which never uses its
      second argument. This particular behaviour used in
      ObjectPropChange is actually better than the behaviour implied
      in ObjectChange. Read comments near the ObjectChange struct in
      objchange.h
     */
    obj_change->apply(obj_change, NULL);
  }
}

static void
group_prop_change_revert(GroupPropChange *change, DiaObject *obj)
{
  GList *tmp;

  for (tmp = change->changes_per_object; tmp != NULL;
       tmp = g_list_next(tmp)) {
    ObjectChange *obj_change = (ObjectChange*)tmp->data;

    /*
      This call to revert() depends on the fact that it is actually a
      call to object_prop_change_apply_revert(), which never uses its
      second argument. This particular behaviour used in
      ObjectPropChange is actually better than the behaviour implied
      in ObjectChange. Read comments near the ObjectChange struct in
      objchange.h
     */
    obj_change->revert(obj_change, NULL);
  }
}

static void 
group_prop_change_free(GroupPropChange *change)
{
  GList *tmp;
  for (tmp = change->changes_per_object; tmp != NULL;
       tmp = g_list_next(tmp)) {
    ObjectChange *obj_change = (ObjectChange*)tmp->data;
    obj_change->free(obj_change);
    g_free(obj_change);
  }
  g_list_free(change->changes_per_object);
}
