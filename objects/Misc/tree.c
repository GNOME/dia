/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *              Hacked (c) 2007 Thomas Harding for tree object
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
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "diamenu.h"

#include "pixmaps/tree.xpm"

#define LINE_WIDTH 0.1

#define DEFAULT_NUMHANDLES 6
#define HANDLE_BUS (HANDLE_CUSTOM1)


typedef struct _Tree {
  Connection connection;

  int num_handles;
  Handle **handles;
  Point *parallel_points;
  Point real_ends[2];
  Color line_color;
} Tree;


#define DIA_MISC_TYPE_TREE_OBJECT_CHANGE dia_misc_tree_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaMiscTreeObjectChange,
                      dia_misc_tree_object_change,
                      DIA_MISC, TREE_OBJECT_CHANGE,
                      DiaObjectChange)


enum change_type {
  TYPE_ADD_POINT,
  TYPE_REMOVE_POINT
};


struct _DiaMiscTreeObjectChange {
  DiaObjectChange obj_change;

  enum change_type type;
  int applied;

  Point point;
  Handle *handle; /* owning ref when not applied for ADD_POINT
		     owning ref when applied for REMOVE_POINT */
  ConnectionPoint *connected_to; /* NULL if not connected */
};


DIA_DEFINE_OBJECT_CHANGE (DiaMiscTreeObjectChange, dia_misc_tree_object_change)


static DiaObjectChange *tree_move_handle         (Tree             *tree,
                                                  Handle           *handle,
                                                  Point            *to,
                                                  ConnectionPoint  *cp,
                                                  HandleMoveReason  reason,
                                                  ModifierKeys      modifiers);
static DiaObjectChange *tree_move                (Tree             *tree,
                                                  Point            *to);
static void tree_select(Tree *tree, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static void tree_draw(Tree *tree, DiaRenderer *renderer);
static DiaObject *tree_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static real tree_distance_from(Tree *tree, Point *point);
static void tree_update_data(Tree *tree);
static void tree_destroy(Tree *tree);
static DiaObject *tree_copy(Tree *tree);

static PropDescription *tree_describe_props(Tree *tree);
static void tree_get_props(Tree *tree, GPtrArray *props);
static void tree_set_props(Tree *tree, GPtrArray *props);
static void tree_save(Tree *tree, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *tree_load(ObjectNode obj_node, int version,DiaContext *ctx);
static DiaMenu *tree_get_object_menu(Tree *tree, Point *clickedpoint);

static DiaObjectChange *tree_create_change (Tree             *tree,
                                            enum change_type  type,
                                            Point            *point,
                                            Handle           *handle,
                                            ConnectionPoint  *connected_to);


static ObjectTypeOps tree_type_ops = {
  (CreateFunc) tree_create,
  (LoadFunc)   tree_load,
  (SaveFunc)   tree_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType tree_type =
{
  "Misc - Tree", /* name */
  0,          /* version */
  tree_xpm,    /* pixmap */
  &tree_type_ops  /* ops */
};

static ObjectOps tree_ops = {
  (DestroyFunc)         tree_destroy,
  (DrawFunc)            tree_draw,
  (DistanceFunc)        tree_distance_from,
  (SelectFunc)          tree_select,
  (CopyFunc)            tree_copy,
  (MoveFunc)            tree_move,
  (MoveHandleFunc)      tree_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      tree_get_object_menu,
  (DescribePropsFunc)   tree_describe_props,
  (GetPropsFunc)        tree_get_props,
  (SetPropsFunc)        tree_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription tree_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR,
  PROP_DESC_END
};

static PropDescription *
tree_describe_props(Tree *tree)
{
  if (tree_props[0].quark == 0)
    prop_desc_list_calculate_quarks(tree_props);
  return tree_props;
}

static PropOffset tree_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Tree, line_color) },
  { NULL, 0, 0 }
};

static void
tree_get_props(Tree *tree, GPtrArray *props)
{
  object_get_props_from_offsets(&tree->connection.object, tree_offsets,
				props);
}

static void
tree_set_props(Tree *tree, GPtrArray *props)
{
  object_set_props_from_offsets(&tree->connection.object, tree_offsets,
				props);
  tree_update_data(tree);
}

static real
tree_distance_from(Tree *tree, Point *point)
{
  Point *endpoints;
  real min_dist;
  int i;

  endpoints = &tree->real_ends[0];
  min_dist = distance_line_point( &endpoints[0], &endpoints[1],
				  LINE_WIDTH, point);
  for (i=0;i<tree->num_handles;i++) {
    min_dist = MIN(min_dist,
		   distance_line_point( &tree->handles[i]->pos,
					&tree->parallel_points[i],
					LINE_WIDTH, point));
  }
  return min_dist;
}

static void
tree_select(Tree *tree, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&tree->connection);
}


static DiaObjectChange *
tree_move_handle (Tree             *tree,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  Connection *conn = &tree->connection;
  Point *endpoints;
  real *parallel;
  real *perp;
  Point vhat, vhatperp;
  Point u;
  real vlen, vlen2;
  real len_scale;
  int i;
  const int num_handles = tree->num_handles; /* const to help scan-build */

  /* code copied from network/bus.c */
  parallel = (real *)g_alloca (num_handles * sizeof(real));
  perp = (real *)g_alloca (num_handles * sizeof(real));

  if (handle->id == HANDLE_BUS) {
    handle->pos = *to;
  } else {
    endpoints = &conn->endpoints[0];
    vhat = endpoints[1];
    point_sub(&vhat, &endpoints[0]);
    if ((fabs(vhat.x) == 0.0) && (fabs(vhat.y)==0.0)) {
      vhat.y += 0.01;
    }
    vlen = sqrt(point_dot(&vhat, &vhat));
    point_scale(&vhat, 1.0/vlen);

    vhatperp.y = -vhat.x;
    vhatperp.x =  vhat.y;
    for (i=0;i<num_handles;i++) {
      u = tree->handles[i]->pos;
      point_sub(&u, &endpoints[0]);
      parallel[i] = point_dot(&vhat, &u);
      perp[i] = point_dot(&vhatperp, &u);
    }

    connection_move_handle(&tree->connection, handle->id, to, cp,
			   reason, modifiers);

    vhat = endpoints[1];
    point_sub(&vhat, &endpoints[0]);
    if ((fabs(vhat.x) == 0.0) && (fabs(vhat.y)==0.0)) {
      vhat.y += 0.01;
    }
    vlen2 = sqrt(point_dot(&vhat, &vhat));
    len_scale = vlen2 / vlen;
    point_normalize(&vhat);
    vhatperp.y = -vhat.x;
    vhatperp.x =  vhat.y;
    for (i=0;i<num_handles;i++) {
      if (tree->handles[i]->connected_to == NULL) {
	u = vhat;
	point_scale(&u, parallel[i]*len_scale);
	point_add(&u, &endpoints[0]);
	tree->parallel_points[i] = u;
	u = vhatperp;
	point_scale(&u, perp[i]);
	point_add(&u, &tree->parallel_points[i]);
	tree->handles[i]->pos = u;
	}
    }
  }

  tree_update_data(tree);

  return NULL;
}


static DiaObjectChange *
tree_move (Tree *tree, Point *to)
{
  Point delta;
  Point *endpoints = &tree->connection.endpoints[0];
  DiaObject *obj = &tree->connection.object;
  int i;

  delta = *to;
  point_sub(&delta, &obj->position);

  for (i=0;i<2;i++) {
    point_add(&endpoints[i], &delta);
    point_add(&tree->real_ends[i], &delta);
  }

  for (i=0;i<tree->num_handles;i++) {
    if (tree->handles[i]->connected_to == NULL) {
      point_add(&tree->handles[i]->pos, &delta);
    }
  }

  tree_update_data(tree);

  return NULL;
}


static void
tree_draw (Tree *tree, DiaRenderer *renderer)
{
  Point *endpoints;
  int i;

  g_return_if_fail (tree != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &tree->real_ends[0];

  dia_renderer_set_linewidth (renderer, LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  dia_renderer_draw_line (renderer,
                          &endpoints[0],
                          &endpoints[1],
                          &tree->line_color);

  for (i=0;i<tree->num_handles;i++) {
    dia_renderer_draw_line (renderer,
                            &tree->parallel_points[i],
                            &tree->handles[i]->pos,
                            &tree->line_color);
  }
}

static DiaObject *
tree_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Tree *tree;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  Point defaultlen = { 0.0, 20.0 };

  tree = g_new0 (Tree, 1);

  conn = &tree->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &tree_type;
  obj->ops = &tree_ops;

  tree->num_handles = DEFAULT_NUMHANDLES;

  connection_init(conn, 2+tree->num_handles, 0);
  tree->line_color = attributes_get_foreground();
  tree->handles = g_new0 (Handle *, tree->num_handles);
  tree->parallel_points = g_new0 (Point, tree->num_handles);
  for (int i = 0; i < tree->num_handles; i++) {
    tree->handles[i] = g_new0 (Handle,1);
    tree->handles[i]->id = HANDLE_BUS;
    tree->handles[i]->type = HANDLE_MINOR_CONTROL;
    tree->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    tree->handles[i]->connected_to = NULL;
    tree->handles[i]->pos = *startpoint;
    tree->handles[i]->pos.y += 20*((real)i+1)/(tree->num_handles+1);
    tree->handles[i]->pos.x += 1.0; /* (i%2==0)?1.0:-1.0; */
    obj->handles[2+i] = tree->handles[i];
  }

  extra->start_trans =
    extra->end_trans =
    extra->start_long =
    extra->end_long = LINE_WIDTH/2.0;
  tree_update_data(tree);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &tree->connection.object;
}


static void
tree_destroy (Tree *tree)
{
  connection_destroy (&tree->connection);
  for (int i = 0; i < tree->num_handles; i++) {
    g_clear_pointer (&tree->handles[i], g_free);
  }
  g_clear_pointer (&tree->handles, g_free);
  g_clear_pointer (&tree->parallel_points, g_free);
}


static DiaObject *
tree_copy(Tree *tree)
{
  Tree *newtree;
  Connection *conn, *newconn;
  DiaObject *newobj;

  conn = &tree->connection;

  newtree = g_new0 (Tree, 1);
  newconn = &newtree->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newtree->num_handles = tree->num_handles;
  newtree->line_color = tree->line_color;

  newtree->handles = g_new0 (Handle *, newtree->num_handles);
  newtree->parallel_points = g_new0 (Point, newtree->num_handles);

  for (int i = 0; i < newtree->num_handles; i++) {
    newtree->handles[i] = g_new0 (Handle, 1);
    *newtree->handles[i] = *tree->handles[i];
    newtree->handles[i]->connected_to = NULL;
    newobj->handles[2+i] = newtree->handles[i];
    newtree->parallel_points[i] = tree->parallel_points[i];
  }

  newtree->real_ends[0] = tree->real_ends[0];
  newtree->real_ends[1] = tree->real_ends[1];

  return &newtree->connection.object;
}


static void
tree_update_data(Tree *tree)
{
  Connection *conn = &tree->connection;
  DiaObject *obj = &conn->object;
  int i;
  Point u, v, vhat;
  Point *endpoints;
  real ulen;
  real min_par, max_par;

/*
 * This seems to break stuff wildly.
 */
/*
  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
*/
  endpoints = &conn->endpoints[0];
  obj->position = endpoints[0];

  v = endpoints[1];
  point_sub(&v, &endpoints[0]);
  if ((fabs(v.x) == 0.0) && (fabs(v.y)==0.0)) {
    v.y += 0.01;
  }
  vhat = v;
  point_normalize(&vhat);
  min_par = 0.0;
  max_par = point_dot(&vhat, &v);
  for (i=0;i<tree->num_handles;i++) {
    u = tree->handles[i]->pos;
    point_sub(&u, &endpoints[0]);
    ulen = point_dot(&u, &vhat);
    min_par = MIN(min_par, ulen);
    max_par = MAX(max_par, ulen);
    tree->parallel_points[i] = vhat;
    point_scale(&tree->parallel_points[i], ulen);
    point_add(&tree->parallel_points[i], &endpoints[0]);
  }

  min_par -= LINE_WIDTH/2.0;
  max_par += LINE_WIDTH/2.0;

  tree->real_ends[0] = vhat;
  point_scale(&tree->real_ends[0], min_par);
  point_add(&tree->real_ends[0], &endpoints[0]);
  tree->real_ends[1] = vhat;
  point_scale(&tree->real_ends[1], max_par);
  point_add(&tree->real_ends[1], &endpoints[0]);

  connection_update_boundingbox(conn);
  rectangle_add_point(&obj->bounding_box, &tree->real_ends[0]);
  rectangle_add_point(&obj->bounding_box, &tree->real_ends[1]);
  for (i=0;i<tree->num_handles;i++) {
    rectangle_add_point(&obj->bounding_box, &tree->handles[i]->pos);
  }

  connection_update_handles(conn);
}

static void
tree_add_handle(Tree *tree, Point *p, Handle *handle)
{
  int i;

  tree->num_handles++;

  /* Allocate more handles */
  tree->handles = g_renew (Handle *, tree->handles, tree->num_handles);
  tree->parallel_points = g_renew (Point, tree->parallel_points, tree->num_handles);

  i = tree->num_handles - 1;

  tree->handles[i] = handle;
  tree->handles[i]->id = HANDLE_BUS;
  tree->handles[i]->type = HANDLE_MINOR_CONTROL;
  tree->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
  tree->handles[i]->connected_to = NULL;
  tree->handles[i]->pos = *p;
  object_add_handle(&tree->connection.object, tree->handles[i]);
}


static void
tree_remove_handle (Tree *tree, Handle *handle)
{
  for (int i = 0; i < tree->num_handles; i++) {
    if (tree->handles[i] == handle) {
      object_remove_handle (&tree->connection.object, handle);

      for (int j = i; j < tree->num_handles - 1; j++) {
        tree->handles[j] = tree->handles[j+1];
        tree->parallel_points[j] = tree->parallel_points[j+1];
      }

      tree->num_handles--;
      tree->handles = g_renew (Handle *, tree->handles, tree->num_handles);
      tree->parallel_points = g_renew (Point,
                                       tree->parallel_points,
                                       tree->num_handles);

      break;
    }
  }
}


static DiaObjectChange *
tree_add_handle_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Tree *tree = (Tree *) obj;
  Handle *handle;

  handle = g_new0 (Handle, 1);
  tree_add_handle (tree, clicked, handle);
  tree_update_data (tree);

  return tree_create_change (tree, TYPE_ADD_POINT, clicked, handle, NULL);
}


static int
tree_point_near_handle(Tree *tree, Point *p)
{
  int i, min;
  real dist = 1000.0;
  real d;

  min = -1;
  for (i=0;i<tree->num_handles;i++) {
    d = distance_line_point(&tree->parallel_points[i],
			    &tree->handles[i]->pos, 0.0, p);

    if (d < dist) {
      dist = d;
      min = i;
    }
  }

  if (dist < 0.5)
    return min;
  else
    return -1;
}


static DiaObjectChange *
tree_delete_handle_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  Tree *tree = (Tree *) obj;
  Handle *handle;
  int handle_num;
  ConnectionPoint *connectionpoint;
  Point p;

  handle_num = tree_point_near_handle(tree, clicked);

  handle = tree->handles[handle_num];
  p = handle->pos;
  connectionpoint = handle->connected_to;

  object_unconnect(obj, handle);
  tree_remove_handle(tree, handle );
  tree_update_data(tree);

  return tree_create_change(tree, TYPE_REMOVE_POINT, &p, handle,
			   connectionpoint);
}

static DiaMenuItem tree_menu_items[] = {
  { N_("Add Handle"), tree_add_handle_callback, NULL, 1 },
  { N_("Delete Handle"), tree_delete_handle_callback, NULL, 1 },
};

static DiaMenu tree_menu = {
  "Tree",
  sizeof(tree_menu_items)/sizeof(DiaMenuItem),
  tree_menu_items,
  NULL
};

static DiaMenu *
tree_get_object_menu(Tree *tree, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  tree_menu_items[0].active = 1;
  tree_menu_items[1].active = (tree_point_near_handle(tree, clickedpoint) >= 0);
  return &tree_menu;
}

static void
tree_save(Tree *tree, ObjectNode obj_node, DiaContext *ctx)
{
  int i;
  AttributeNode attr;

  connection_save(&tree->connection, obj_node, ctx);

  data_add_color( new_attribute(obj_node, "line_color"), &tree->line_color, ctx);

  attr = new_attribute(obj_node, "tree_handles");

  for (i=0;i<tree->num_handles;i++) {
    data_add_point(attr, &tree->handles[i]->pos, ctx);
  }
}

static DiaObject *
tree_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Tree *tree;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  AttributeNode attr;
  DataNode data;

  tree = g_new0 (Tree, 1);

  conn = &tree->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &tree_type;
  obj->ops = &tree_ops;

  connection_load(conn, obj_node, ctx);

  attr = object_find_attribute(obj_node, "tree_handles");

  tree->num_handles = 0;
  if (attr != NULL)
    tree->num_handles = attribute_num_data(attr);

  connection_init(conn, 2 + tree->num_handles, 0);

  data = attribute_first_data(attr);
  tree->handles = g_new0 (Handle *, tree->num_handles);
  tree->parallel_points = g_new0 (Point, tree->num_handles);
  for (int i = 0; i < tree->num_handles; i++) {
    tree->handles[i] = g_new0 (Handle, 1);
    tree->handles[i]->id = HANDLE_BUS;
    tree->handles[i]->type = HANDLE_MINOR_CONTROL;
    tree->handles[i]->connect_type = HANDLE_CONNECTABLE_NOBREAK;
    tree->handles[i]->connected_to = NULL;
    data_point(data, &tree->handles[i]->pos, ctx);
    obj->handles[2+i] = tree->handles[i];

    data = data_next(data);
  }

  tree->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &tree->line_color, ctx);

  extra->start_trans =
    extra->end_trans =
    extra->start_long =
    extra->end_long = LINE_WIDTH/2.0;
  tree_update_data(tree);

  return &tree->connection.object;
}


static void
dia_misc_tree_object_change_free (DiaObjectChange *self)
{
  DiaMiscTreeObjectChange *change = DIA_MISC_TREE_OBJECT_CHANGE (self);

  if ((change->type == TYPE_ADD_POINT && !change->applied) ||
      (change->type == TYPE_REMOVE_POINT && change->applied) ){
    g_clear_pointer (&change->handle, g_free);
  }
}


static void
dia_misc_tree_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaMiscTreeObjectChange *change = DIA_MISC_TREE_OBJECT_CHANGE (self);

  change->applied = 1;

  switch (change->type) {
    case TYPE_ADD_POINT:
      tree_add_handle ((Tree *) obj, &change->point, change->handle);
      break;
    case TYPE_REMOVE_POINT:
      object_unconnect (obj, change->handle);
      tree_remove_handle ((Tree *) obj, change->handle);
      break;
    default:
      g_return_if_reached ();
  }
  tree_update_data ((Tree *) obj);
}


static void
dia_misc_tree_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaMiscTreeObjectChange *change = DIA_MISC_TREE_OBJECT_CHANGE (self);

  switch (change->type) {
    case TYPE_ADD_POINT:
      tree_remove_handle ((Tree *) obj, change->handle);
      break;
    case TYPE_REMOVE_POINT:
      tree_add_handle ((Tree *) obj, &change->point, change->handle);
      if (change->connected_to) {
        object_connect (obj, change->handle, change->connected_to);
      }
      break;
    default:
      g_return_if_reached ();
  }

  tree_update_data ((Tree *) obj);

  change->applied = 0;
}


static DiaObjectChange *
tree_create_change (Tree             *tree,
                    enum change_type  type,
                    Point            *point,
                    Handle           *handle,
                    ConnectionPoint  *connected_to)
{
  DiaMiscTreeObjectChange *change;

  change = dia_object_change_new (DIA_MISC_TYPE_TREE_OBJECT_CHANGE);

  change->type = type;
  change->applied = 1;
  change->point = *point;
  change->handle = handle;
  change->connected_to = connected_to;

  return DIA_OBJECT_CHANGE (change);
}
