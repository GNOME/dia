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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "connectionpoint.h"
#include "group.h"
#include "properties.h"
#include "diarenderer.h"


/*!
 * \brief Allow grouping other objects and hiding them from the diagram
 */
struct _Group {
  /*! \protected DiaObject must be first because this is a 'subclass' of it. */
  DiaObject object;
  /*! \protected To move handles use Group::move_handle() */
  Handle handles[8];
  /*! \protected To group objects use group_create() */
  GList *objects;
  /*! \protected Description of the contained properties */
  const PropDescription *pdesc;
  /*! Optional transformation matrix */
  DiaMatrix *matrix;
};


static DiaObjectChange       *group_apply_properties_list    (Group            *group,
                                                              GPtrArray        *props);
static double                 group_distance_from            (Group            *group,
                                                              Point            *point);
static void                   group_select                   (Group            *group);
static DiaObjectChange       *group_move_handle              (Group            *group,
                                                              Handle           *handle,
                                                              Point            *to,
                                                              ConnectionPoint  *cp,
                                                              HandleMoveReason  reason,
                                                              ModifierKeys      modifiers);
static DiaObjectChange       *group_move                     (Group            *group,
                                                              Point            *to);
static void                   group_draw                     (Group            *group,
                                                              DiaRenderer      *renderer);
static void                   group_update_data              (Group            *group);
static void                   group_update_handles           (Group            *group);
static void                   group_update_connectionpoints  (Group            *group);
static void                   group_destroy                  (Group            *group);
static DiaObject             *group_copy                     (Group            *group);
static const PropDescription *group_describe_props           (Group            *group);
static void                   group_get_props                (Group            *group,
                                                              GPtrArray        *props);
static void                   group_set_props                (Group            *group,
                                                              GPtrArray        *props);
static void                   group_transform                (Group            *group,
                                                              const DiaMatrix  *m);


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
  (TransformFunc)       group_transform
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

    dist = MIN (dist, dia_object_distance_from (obj, &tp));

    list = g_list_next (list);
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
  DiaRectangle *bb = &group->object.bounding_box;

  group->handles[0].id = HANDLE_RESIZE_NW;
  group->handles[0].pos.x = bb->left;
  group->handles[0].pos.y = bb->top;

  group->handles[1].id = HANDLE_RESIZE_N;
  group->handles[1].pos.x = (bb->left + bb->right) / 2.0;
  group->handles[1].pos.y = bb->top;

  group->handles[2].id = HANDLE_RESIZE_NE;
  group->handles[2].pos.x = bb->right;
  group->handles[2].pos.y = bb->top;

  group->handles[3].id = HANDLE_RESIZE_W;
  group->handles[3].pos.x = bb->left;
  group->handles[3].pos.y = (bb->top + bb->bottom) / 2.0;

  group->handles[4].id = HANDLE_RESIZE_E;
  group->handles[4].pos.x = bb->right;
  group->handles[4].pos.y = (bb->top + bb->bottom) / 2.0;

  group->handles[5].id = HANDLE_RESIZE_SW;
  group->handles[5].pos.x = bb->left;
  group->handles[5].pos.y = bb->bottom;

  group->handles[6].id = HANDLE_RESIZE_S;
  group->handles[6].pos.x = (bb->left + bb->right) / 2.0;
  group->handles[6].pos.y = bb->bottom;

  group->handles[7].id = HANDLE_RESIZE_SE;
  group->handles[7].pos.x = bb->right;
  group->handles[7].pos.y = bb->bottom;
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

static void
group_objects_move_delta (Group *group, const Point *delta)
{
  if (group->matrix) {
    DiaMatrix *m = group->matrix;

    m->x0 += delta->x;
    m->y0 += delta->y;
  } else {
    Point dt = *delta;
    object_list_move_delta(group->objects, &dt);
  }
}


static DiaObjectChange *
group_move_handle (Group            *group,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  DiaObject *obj = &group->object;
  DiaRectangle *bb = &obj->bounding_box;
  /* top and left handles are also changing the objects position */
  Point top_left = { bb->left, bb->top };
  /* before and after width and height */
  real w0, h0, w1, h1;
  Point fixed;

  g_return_val_if_fail (handle->id >= HANDLE_RESIZE_NW, NULL);
  g_return_val_if_fail (handle->id <= HANDLE_RESIZE_SE, NULL);

  w0 = w1 = bb->right - bb->left;
  h0 = h1 = bb->bottom - bb->top;

  /* Movement and scaling only work as intended with no active rotation.
   * For rotated group we should either translate the handle positions
   * or we might rotate the horizontal and vertical scale ...
   */
  switch (handle->id) {
    case HANDLE_RESIZE_NW:
      g_return_val_if_fail (group->handles[7].id == HANDLE_RESIZE_SE, NULL);
      fixed = group->handles[7].pos;
      w1 = w0 - (to->x - top_left.x);
      h1 = h0 - (to->y - top_left.y);
      break;
    case HANDLE_RESIZE_N:
      g_return_val_if_fail (group->handles[6].id == HANDLE_RESIZE_S, NULL);
      fixed = group->handles[6].pos;
      h1 = h0 - (to->y - top_left.y);
      break;
    case HANDLE_RESIZE_NE:
      g_return_val_if_fail (group->handles[5].id == HANDLE_RESIZE_SW, NULL);
      fixed = group->handles[5].pos;
      w1 = to->x - top_left.x;
      h1 = h0 - (to->y - top_left.y);
      break;
    case HANDLE_RESIZE_W:
      g_return_val_if_fail (group->handles[4].id == HANDLE_RESIZE_E, NULL);
      fixed = group->handles[4].pos;
      w1 = w0 - (to->x - top_left.x);
      break;
    case HANDLE_RESIZE_E:
      g_return_val_if_fail (group->handles[3].id == HANDLE_RESIZE_W, NULL);
      fixed = group->handles[3].pos;
      w1 = to->x - top_left.x;
      break;
    case HANDLE_RESIZE_SW:
      g_return_val_if_fail (group->handles[2].id == HANDLE_RESIZE_NE, NULL);
      fixed = group->handles[2].pos;
      w1 = w0 - (to->x - top_left.x);
      h1 = to->y - top_left.y;
      break;
    case HANDLE_RESIZE_S:
      g_return_val_if_fail (group->handles[1].id == HANDLE_RESIZE_N, NULL);
      fixed = group->handles[1].pos;
      h1 = to->y - top_left.y;
      break;
    case HANDLE_RESIZE_SE:
      g_return_val_if_fail (group->handles[0].id == HANDLE_RESIZE_NW, NULL);
      fixed = group->handles[0].pos;
      w1 = to->x - top_left.x;
      h1 = to->y - top_left.y;
      break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      g_warning ("group_move_handle() called with wrong handle-id %d",
                 handle->id);
  }

  if (!group->matrix) {
    group->matrix = g_new0 (DiaMatrix, 1);
    group->matrix->xx = 1.0;
    group->matrix->yy = 1.0;
  }

  /* The resizing is in the diagram coordinate system, translate
   * it to respective object offset. BEWARE: this is not completely
   * reversible, so we might need to deliver some extra undo information.
   */
  w1 = MAX(w1, 0.05); /* arbitrary minimum size */
  h1 = MAX(h1, 0.05);
  {
    DiaMatrix m = {w1/w0, 0, 0, h1/h0, 0, 0 };
    DiaMatrix t = { 1, 0, 0, 1, fixed.x, fixed.y };

    dia_matrix_multiply (&m, &m, &t);
    t.x0 = -fixed.x;
    t.y0 = -fixed.y;
    dia_matrix_multiply (&m, &t, &m);
    dia_matrix_multiply (group->matrix, group->matrix, &m);
  }
  group_update_data(group);

  return NULL;
}


static DiaObjectChange *
group_move (Group *group, Point *to)
{
  Point delta,pos;

  delta = *to;
  pos = group->object.position;
  point_sub(&delta, &pos);

  /* We don't need any transformation of delta, because
   * group_update_data () maintains the relative position.
   */
  group_objects_move_delta(group, &delta);

  group_update_data(group);

  return NULL;
}

static void
group_draw (Group *group, DiaRenderer *renderer)
{
  GList *list;
  DiaObject *obj;

  list = group->objects;
  while (list != NULL) {
    obj = (DiaObject *) list->data;

    dia_renderer_draw_object (renderer, obj, group->matrix);
    list = g_list_next(list);
  }
}

void
group_destroy_shallow(DiaObject *obj)
{
  Group *group = (Group *)obj;

  g_clear_pointer (&obj->handles, g_free);
  g_clear_pointer (&obj->connections, g_free);

  g_list_free(group->objects);

  prop_desc_list_free_handler_chain((PropDescription *)group->pdesc);

  g_free ((PropDescription *) group->pdesc);
  g_clear_pointer (&group->matrix, g_free);
  g_clear_pointer (&group, g_free);
}

static void
group_destroy(Group *group)
{
  DiaObject *obj = &group->object;

  destroy_object_list(group->objects);

  /* ConnectionPoints in the inner objects have already
     been unconnected and freed. */
  obj->num_connections = 0;

  prop_desc_list_free_handler_chain((PropDescription *) group->pdesc);

  g_free ((PropDescription *) group->pdesc);
  g_clear_pointer (&group->matrix, g_free);

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
    newobj->handles[i] = &newgroup->handles[i];
    newgroup->handles[i] = group->handles[i];
  }

  newgroup->matrix = g_memdup2 (group->matrix, sizeof (DiaMatrix));

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
      DiaRectangle box;
      DiaRectangle *bb = &group->object.bounding_box;
      DiaMatrix *m = group->matrix;

      /* maintain obj->position */
      transform_point (&group->object.position, m);

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

  if (dia_matrix_is_identity (matrix)) {
    /* just drop it as it has no effect */
    g_clear_pointer (&matrix, g_free);
    matrix = NULL;
  }
  group->matrix = matrix;
  group_update_data(group);
  return &group->object;
}
/*!
 * \brief Grouping of objects
 *
 * Make sure there are no connections from objects to objects
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

  /* it's a programmer's error to create a group of nothing */
  g_return_val_if_fail (objects != NULL, NULL);

  group = g_new0(Group,1);
  obj = DIA_OBJECT (group);

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
    obj->handles[i] = &group->handles[i];
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
group_prop_event_deliver (Group *group, Property *prop)
{
  GList *tmp;
  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;
    const PropDescription *pdesc,*plist;

    /* I'm sorry. I haven't found a working less ugly solution :( */
    plist = dia_object_describe_properties (obj);
    pdesc = prop_desc_list_find_prop (plist, prop->descr->name);
    if (pdesc && pdesc->event_handler) {
      /* deliver */
      PropEventHandler hdl = prop_desc_find_real_handler (pdesc);
      if (hdl) {
        return hdl (obj,prop);
      } else {
        g_warning ("dropped group event on prop %s, "
                   "final handler was NULL",prop->descr->name);
        return FALSE;
      }
    }
  }

  g_warning ("undelivered group property event for prop %s", prop->descr->name);

  return FALSE;
}

static PropDescription _group_props[] = {
  { "meta", PROP_TYPE_DICT, PROP_FLAG_SELF_ONLY|PROP_FLAG_OPTIONAL,
    "Object meta info", "Some key/value pairs"},
  { "matrix", PROP_TYPE_MATRIX, PROP_FLAG_SELF_ONLY|PROP_FLAG_DONT_MERGE|PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS,
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
      /* Ensure we have no duplicates with own props */
      int n_other = 0;
      for (i=0; group->pdesc[i].name != NULL; i++) {
	int j;
	gboolean is_own = FALSE;
	for (j=0; j < G_N_ELEMENTS(_group_props); ++j) {
	  if (   group->pdesc[i].quark == _group_props[j].quark
	      && group->pdesc[i].type_quark == _group_props[j].type_quark)
	    is_own = TRUE;
	}
	if (!is_own) {
	  if (n_other != i)
	    memcpy ((gpointer)&group->pdesc[n_other], &group->pdesc[i], sizeof(PropDescription));
	  ++n_other;
	}
      }
      /* Ensure NULL termination */
      ((PropDescription *)&group->pdesc[n_other])->name = NULL;

      /* hijack event delivery */
      for (i=0; i < n_other; i++) {
	/* Ensure we have no duplicates with our own properties */
        if (group->pdesc[i].event_handler)
          prop_desc_insert_handler((PropDescription *)&group->pdesc[i],
                                   (PropEventHandler)group_prop_event_deliver);
      }
      /* mix in our own properties */
      {
	int n_own = G_N_ELEMENTS(_group_props) - 1;
        PropDescription *arr = g_new (PropDescription, n_other+n_own+1);

	for (i = 0; i < n_own; ++i)
	  arr[i] = _group_props[i];
	memcpy (&arr[n_own], group->pdesc, (n_other+1) * sizeof(PropDescription));
	for (i=n_own; arr[i].name != NULL; ++i) {
	  /* these are not meant to be saved/loaded with the group */
	  arr[i].flags |= (PROP_FLAG_DONT_SAVE|PROP_FLAG_OPTIONAL);
	}
        g_free ((PropDescription *) group->pdesc);
        group->pdesc = arr;
      }
    }
  }

  return group->pdesc;
}

static PropOffset _group_offsets[] = {
  { "meta", PROP_TYPE_DICT, offsetof(DiaObject, meta) },
  { "matrix", PROP_TYPE_MATRIX, offsetof(Group, matrix) },
  { NULL }
};

static void
group_get_props(Group *group, GPtrArray *props)
{
  GList *tmp;
  GPtrArray *props_list, *props_self;
  guint i;

  /* Need to split the passed in properties to self props
   * and the ones really passed to the list of owned objects.
   */
  props_self = g_ptr_array_new();
  props_list = g_ptr_array_new();
  for (i=0; i < props->len; i++) {
    Property *p = g_ptr_array_index(props,i);

    if ((p->descr->flags & PROP_FLAG_SELF_ONLY) != 0)
      g_ptr_array_add(props_self, p);
    else
      g_ptr_array_add(props_list, p);
  }

  object_get_props_from_offsets (&group->object, _group_offsets, props_self);

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;

    dia_object_get_properties (obj, props_list);
  }

  g_ptr_array_free (props_list, TRUE);
  g_ptr_array_free (props_self, TRUE);
}

static void
group_set_props(Group *group, GPtrArray *props)
{
  GList *tmp;
  GPtrArray *props_list, *props_self;
  guint i;

  /* Need to split the passed in properties to self props
   * and the ones really passed to the list of owned objects.
   */
  props_self = g_ptr_array_new();
  props_list = g_ptr_array_new();
  for (i=0; i < props->len; i++) {
    Property *p = g_ptr_array_index(props,i);

    if ((p->descr->flags & PROP_FLAG_SELF_ONLY) != 0)
      g_ptr_array_add(props_self, p);
    else
      g_ptr_array_add(props_list, p);
  }

  object_set_props_from_offsets(&group->object, _group_offsets, props_self);

  for (tmp = group->objects; tmp != NULL; tmp = tmp->next) {
    DiaObject *obj = tmp->data;

    dia_object_set_properties (obj, props_list);
  }

  g_ptr_array_free(props_list, TRUE);
  g_ptr_array_free(props_self, TRUE);

  group_update_data (group);
}


struct _DiaGroupObjectChange {
  DiaObjectChange obj_change;
  Group *group;
  GList *changes_per_object;
};


DIA_DEFINE_OBJECT_CHANGE (DiaGroupObjectChange, dia_group_object_change)


static void
dia_group_object_change_free (DiaObjectChange *self)
{
  DiaGroupObjectChange *change = DIA_GROUP_OBJECT_CHANGE (self);

  g_list_free_full (change->changes_per_object, dia_object_change_unref);
}


static void
dia_group_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaGroupObjectChange *change = DIA_GROUP_OBJECT_CHANGE (self);
  GList *tmp;

  for (tmp = change->changes_per_object; tmp != NULL;
       tmp = g_list_next (tmp)) {
    DiaObjectChange *obj_change = DIA_OBJECT_CHANGE (tmp->data);

    /*
      This call to apply() depends on the fact that it is actually a
      call to object_prop_change_apply_revert(), which never uses its
      second argument. This particular behaviour used in
      ObjectPropChange is actually better than the behaviour implied
      in ObjectChange. Read comments near the ObjectChange struct in
      objchange.h
     */
    dia_object_change_apply (obj_change, NULL);
  }
}


static void
dia_group_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaGroupObjectChange *change = DIA_GROUP_OBJECT_CHANGE (self);
  GList *tmp;

  for (tmp = change->changes_per_object; tmp != NULL;
       tmp = g_list_next (tmp)) {
    DiaObjectChange *obj_change = DIA_OBJECT_CHANGE (tmp->data);

    /*
      This call to revert() depends on the fact that it is actually a
      call to object_prop_change_apply_revert(), which never uses its
      second argument. This particular behaviour used in
      ObjectPropChange is actually better than the behaviour implied
      in ObjectChange. Read comments near the ObjectChange struct in
      objchange.h
     */
    dia_object_change_revert (obj_change, NULL);
  }
}


static DiaObjectChange *
dia_group_object_change_new (Group *group, GList *changes_per_object)
{
  DiaGroupObjectChange *self = dia_object_change_new (DIA_TYPE_GROUP_OBJECT_CHANGE);

  self->group = group;
  self->changes_per_object = changes_per_object;

  return DIA_OBJECT_CHANGE (self);
}


DiaObjectChange *
group_apply_properties_list (Group *group, GPtrArray *props)
{
  GList *tmp = NULL;
  GList *clist = NULL;
  DiaObjectChange *objchange;
  GPtrArray *props_list, *props_self;
  guint i;

  /* Need to split the passed in properties to self props
   * and the ones really passed to the list of owned objects.
   */
  props_self = g_ptr_array_new ();
  props_list = g_ptr_array_new ();
  for (i=0; i < props->len; ++i) {
    Property *p = g_ptr_array_index (props,i);

    if (p->experience & PXP_NOTSET) {
      continue;
    } else if ((p->descr->flags & PROP_FLAG_SELF_ONLY) != 0) {
      g_ptr_array_add (props_self, p);
    } else {
      g_ptr_array_add (props_list, p);
    }
  }

  for (tmp = group->objects; tmp != NULL; tmp = g_list_next (tmp)) {
    DiaObject *obj = (DiaObject*) tmp->data;
    objchange = NULL;

    objchange = dia_object_apply_properties (obj, props_list);
    clist = g_list_append (clist, objchange);
  }

  /* finally ourself */
  objchange = object_apply_props (&group->object, props_self);
  clist = g_list_append (clist, objchange);

  g_ptr_array_free (props_list, TRUE);
  g_ptr_array_free (props_self, TRUE);

  group_update_data (group);

  return dia_group_object_change_new (group, clist);
}


static void
group_transform (Group *group, const DiaMatrix *m)
{
  g_return_if_fail (m != NULL);

  if (group->matrix) {
    dia_matrix_multiply (group->matrix, group->matrix, m);
  } else {
    group->matrix = g_memdup2 (m, sizeof(*m));
  }
  group_update_data (group);
}


const DiaMatrix *
group_get_transform (Group *group)
{
  return group->matrix;
}
