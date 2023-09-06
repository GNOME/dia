/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support
 * Copyright(C) 2000 Cyrille Chepelov
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
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "color.h"
#include "properties.h"
#include "geometry.h"
#include "connpoint_line.h"

#include "grafcet.h"

#include "pixmaps/vergent.xpm"

#define VERGENT_LINE_WIDTH (GRAFCET_GENERAL_LINE_WIDTH * 1.5)


typedef enum { VERGENT_OR, VERGENT_AND } VergentType;

typedef struct _Vergent {
  Connection connection;

  ConnectionPoint northeast, northwest, southwest, southeast;
  ConnPointLine *north, *south;

  VergentType type;
} Vergent;


static DiaObjectChange *vergent_move_handle     (Vergent          *vergent,
                                                 Handle           *handle,
                                                 Point            *to,
                                                 ConnectionPoint  *cp,
                                                 HandleMoveReason  reason,
                                                 ModifierKeys      modifiers);
static DiaObjectChange *vergent_move            (Vergent          *vergent,
                                                 Point            *to);
static void             vergent_select          (Vergent          *vergent,
                                                 Point            *clicked_point,
                                                 DiaRenderer      *interactive_renderer);
static void             vergent_draw            (Vergent          *vergent,
                                                 DiaRenderer      *renderer);
static DiaObject       *vergent_create          (Point            *startpoint,
                                                 void             *user_data,
                                                 Handle          **handle1,
                                                 Handle          **handle2);
static real             vergent_distance_from   (Vergent          *vergent,
                                                 Point            *point);
static void             vergent_update_data     (Vergent          *vergent);
static void             vergent_destroy         (Vergent          *vergent);
static DiaObject       *vergent_load            (ObjectNode        obj_node,
                                                 int               version,
                                                 DiaContext       *ctx);
static PropDescription *vergent_describe_props  (Vergent          *vergent);
static void             vergent_get_props       (Vergent          *vergent,
                                                 GPtrArray        *props);
static void             vergent_set_props       (Vergent          *vergent,
                                                 GPtrArray        *props);
static DiaMenu         *vergent_get_object_menu (Vergent          *vergent,
                                                 Point            *clickedpoint);


static ObjectTypeOps vergent_type_ops = {
  (CreateFunc)vergent_create,   /* create */
  (LoadFunc)  vergent_load,/*using properties*/     /* load */
  (SaveFunc)  object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};


DiaObjectType vergent_type = {
  "GRAFCET - Vergent", /* name */
  0,                /* version */
  vergent_xpm,       /* pixmap */
  &vergent_type_ops,    /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};


static ObjectOps vergent_ops = {
  (DestroyFunc)         vergent_destroy,
  (DrawFunc)            vergent_draw,
  (DistanceFunc)        vergent_distance_from,
  (SelectFunc)          vergent_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            vergent_move,
  (MoveHandleFunc)      vergent_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      vergent_get_object_menu,
  (DescribePropsFunc)   vergent_describe_props,
  (GetPropsFunc)        vergent_get_props,
  (SetPropsFunc)        vergent_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};


static PropEnumData prop_vtype_data[] = {
  { N_("OR"), VERGENT_OR },
  { N_("AND"), VERGENT_AND },
  { NULL, 0 }
};


static PropDescription vergent_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "cpl_north", PROP_TYPE_CONNPOINT_LINE, 0,
    "cpl_north","cpl_north"},
  { "cpl_south", PROP_TYPE_CONNPOINT_LINE, 0,
    "cpl_south","cpl_south"},
  { "vtype", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Vergent type:"), NULL, prop_vtype_data},
  PROP_DESC_END
};


static PropDescription *
vergent_describe_props (Vergent *vergent)
{
  if (vergent_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (vergent_props);
  }

  return vergent_props;
}


static PropOffset vergent_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "cpl_north", PROP_TYPE_CONNPOINT_LINE, offsetof (Vergent, north)},
  { "cpl_south", PROP_TYPE_CONNPOINT_LINE, offsetof (Vergent, south)},
  { "vtype", PROP_TYPE_ENUM, offsetof (Vergent, type)},
  { NULL, 0, 0 }
};


static void
vergent_get_props(Vergent *vergent, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (vergent),
                                 vergent_offsets,props);
}


static void
vergent_set_props (Vergent *vergent, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (vergent),
                                 vergent_offsets,props);
  vergent_update_data (vergent);
}


static real
vergent_distance_from (Vergent *vergent, Point *point)
{
  Connection *conn = &vergent->connection;
  DiaRectangle rectangle;

  rectangle.left = conn->endpoints[0].x;
  rectangle.right = conn->endpoints[1].x;
  rectangle.top = conn->endpoints[0].y;

  switch (vergent->type) {
    case VERGENT_OR:
      rectangle.top -= .5 * VERGENT_LINE_WIDTH;
      rectangle.bottom = rectangle.top + VERGENT_LINE_WIDTH;
      break;
    case VERGENT_AND:
      rectangle.top -= 1.5 * VERGENT_LINE_WIDTH;
      rectangle.bottom = rectangle.top + (3.0 * VERGENT_LINE_WIDTH);
      break;
    default:
      g_return_val_if_reached (0.0);
  }

  return distance_rectangle_point (&rectangle,point);
}


static void
vergent_select (Vergent     *vergent,
                Point       *clicked_point,
                DiaRenderer *interactive_renderer)
{
  vergent_update_data (vergent);
}


static DiaObjectChange *
vergent_move_handle (Vergent          *vergent,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  g_return_val_if_fail (vergent != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_ENDPOINT) {
    Point to2;

    to2.x = to->x;
    to2.y = vergent->connection.endpoints[0].y;
    connection_move_handle (&vergent->connection,
                            HANDLE_MOVE_ENDPOINT,
                            &to2,
                            NULL,
                            reason,
                            0);
  }
  connection_move_handle (&vergent->connection,
                          handle->id,
                          to,
                          cp,
                          reason,
                          modifiers);
  vergent_update_data (vergent);

  return NULL;
}


static DiaObjectChange *
vergent_move (Vergent *vergent, Point *to)
{
  Point start_to_end;
  Point *endpoints = &vergent->connection.endpoints[0];

  start_to_end = endpoints[1];
  point_sub (&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add (&endpoints[1], &start_to_end);

  vergent_update_data (vergent);

  return NULL;
}


static void
vergent_draw (Vergent *vergent, DiaRenderer *renderer)
{
  Connection *conn = &vergent->connection;
  Point p1,p2;

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  switch(vergent->type) {
    case VERGENT_OR:
      dia_renderer_set_linewidth (renderer, VERGENT_LINE_WIDTH);
      dia_renderer_draw_line (renderer,
                              &conn->endpoints[0],
                              &conn->endpoints[1],
                              &color_black);
      break;
    case VERGENT_AND:
      dia_renderer_set_linewidth (renderer, 2.0 * VERGENT_LINE_WIDTH);
      dia_renderer_draw_line (renderer,
                              &conn->endpoints[0],
                              &conn->endpoints[1],
                              &color_white);
      dia_renderer_set_linewidth (renderer, VERGENT_LINE_WIDTH);
      p1.x = conn->endpoints[0].x;
      p2.x = conn->endpoints[1].x;
      p1.y = p2.y = conn->endpoints[0].y - VERGENT_LINE_WIDTH;
      dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
      p1.y = p2.y = conn->endpoints[0].y + VERGENT_LINE_WIDTH;
      dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
      break;
    default:
      g_return_if_reached ();
  }
}


static void
vergent_update_data (Vergent *vergent)
{
  Connection *conn = &vergent->connection;
  LineBBExtras *extra = &conn->extra_spacing;
  DiaObject *obj = &conn->object;
  Point p0,p1;

  conn->endpoints[1].y = conn->endpoints[0].y;
  if (ABS(conn->endpoints[1].x-conn->endpoints[0].x) < 3.0)
    conn->endpoints[1].x = conn->endpoints[0].x + 3.0;

  obj->position = conn->endpoints[0];

  p0.x = conn->endpoints[0].x + 1.0;
  p1.x = conn->endpoints[1].x - 1.0;
  p0.y = p1.y = conn->endpoints[0].y;

  switch (vergent->type) {
    case VERGENT_OR:
      extra->start_trans =
        extra->start_long =
        extra->end_trans =
        extra->end_long = VERGENT_LINE_WIDTH / 2.0;
      connection_update_boundingbox (conn);

      /* place the connection point lines */
      connpointline_update (vergent->north);
      connpointline_putonaline (vergent->north, &p0, &p1, DIR_NORTH);
      vergent->northwest.pos = p0;
      vergent->northwest.directions = DIR_NORTH;
      vergent->northeast.pos = p1;
      vergent->northeast.directions = DIR_NORTH;
      connpointline_update (vergent->south);
      connpointline_putonaline (vergent->south, &p0, &p1, DIR_SOUTH);
      vergent->southwest.pos = p0;
      vergent->southwest.directions = DIR_SOUTH;
      vergent->southeast.pos = p1;
      vergent->southeast.directions = DIR_SOUTH;
      break;
    case VERGENT_AND:
      extra->start_trans =
        extra->end_trans = (3 * VERGENT_LINE_WIDTH) / 2.0;
      extra->start_long =
        extra->end_long = VERGENT_LINE_WIDTH / 2.0;
      connection_update_boundingbox (conn);
      connection_update_boundingbox (conn);

      /* place the connection point lines */
      p0.y = p1.y = p0.y - VERGENT_LINE_WIDTH;
      connpointline_update (vergent->north);
      connpointline_putonaline (vergent->north, &p0, &p1, DIR_NORTH);
      vergent->northwest.pos = p0;
      vergent->northwest.directions = DIR_NORTH;
      vergent->northeast.pos = p1;
      vergent->northeast.directions = DIR_NORTH;
      p0.y = p1.y = p0.y + 2.0 *VERGENT_LINE_WIDTH;
      connpointline_update (vergent->south);
      connpointline_putonaline (vergent->south, &p0, &p1, DIR_SOUTH);
      vergent->southwest.pos = p0;
      vergent->southwest.directions = DIR_SOUTH;
      vergent->southeast.pos = p1;
      vergent->southeast.directions = DIR_SOUTH;
      break;
    default:
      g_return_if_reached ();
  }

  connection_update_handles (conn);
}

/* DiaObject menu handling */


#define DIA_GRAFCET_TYPE_VERGENT_OBJECT_CHANGE dia_grafcet_vergent_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaGRAFCETVergentObjectChange,
                      dia_grafcet_vergent_object_change,
                      DIA_GRAFCET, VERGENT_OBJECT_CHANGE,
                      DiaObjectChange)


struct _DiaGRAFCETVergentObjectChange {
  DiaObjectChange obj_change;

  DiaObjectChange *north, *south;
};


DIA_DEFINE_OBJECT_CHANGE (DiaGRAFCETVergentObjectChange,
                          dia_grafcet_vergent_object_change)


static void
dia_grafcet_vergent_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaGRAFCETVergentObjectChange *change = DIA_GRAFCET_VERGENT_OBJECT_CHANGE (self);

  dia_object_change_apply (change->north, obj);
  dia_object_change_apply (change->south, obj);
}


static void
dia_grafcet_vergent_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaGRAFCETVergentObjectChange *change = DIA_GRAFCET_VERGENT_OBJECT_CHANGE (self);

  dia_object_change_revert (change->north,obj);
  dia_object_change_revert (change->south,obj);
}


static void
dia_grafcet_vergent_object_change_free (DiaObjectChange *self)
{
  DiaGRAFCETVergentObjectChange *change = DIA_GRAFCET_VERGENT_OBJECT_CHANGE (self);

  g_clear_pointer (&change->north, dia_object_change_unref);
  g_clear_pointer (&change->south, dia_object_change_unref);
}


static DiaObjectChange *
vergent_create_change (Vergent *vergent, int add, Point *clicked)
{
  DiaGRAFCETVergentObjectChange *vc;

  vc = dia_object_change_new (DIA_GRAFCET_TYPE_VERGENT_OBJECT_CHANGE);

  if (add) {
    vc->north = connpointline_add_point (vergent->north,clicked);
    vc->south = connpointline_add_point (vergent->south,clicked);
  } else {
    vc->north = connpointline_remove_point (vergent->north,clicked);
    vc->south = connpointline_remove_point (vergent->south,clicked);
  }

  vergent_update_data (vergent);

  return DIA_OBJECT_CHANGE (vc);
}


static DiaObjectChange *
vergent_add_cp_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  return vergent_create_change ((Vergent *) obj, 1, clicked);
}


static DiaObjectChange *
vergent_delete_cp_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  return vergent_create_change ((Vergent *) obj, 0, clicked);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), vergent_add_cp_callback, NULL, 1 },
  { N_("Delete connection point"), vergent_delete_cp_callback, NULL, 1 },
};


static DiaMenu object_menu = {
  N_("GRAFCET OR/AND vergent"),
  sizeof (object_menu_items) / sizeof (DiaMenuItem),
  object_menu_items,
  NULL
};


static DiaMenu *
vergent_get_object_menu (Vergent *vergent, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  g_return_val_if_fail (vergent->north->num_connections == vergent->south->num_connections,
                        NULL);

  object_menu_items[0].active = 1;
  object_menu_items[1].active = (vergent->north->num_connections > 1);

  return &object_menu;
}


static DiaObject *
vergent_create (Point   *startpoint,
                void    *user_data,
                Handle **handle1,
                Handle **handle2)
{
  Vergent *vergent;
  Connection *conn;
  DiaObject *obj;
  int i;
  Point defaultlen  = { 6.0, 0.0 };

  vergent = g_new0 (Vergent, 1);
  conn = &vergent->connection;
  obj = DIA_OBJECT (conn);

  obj->type = &vergent_type;
  obj->ops = &vergent_ops;

  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add (&conn->endpoints[1], &defaultlen);

  connection_init (conn, 2, 4);

  obj->connections[0] = &vergent->northeast;
  obj->connections[1] = &vergent->northwest;
  obj->connections[2] = &vergent->southwest;
  obj->connections[3] = &vergent->southeast;
  for (i=0; i<4; i++) {
    obj->connections[i]->object = obj;
    obj->connections[i]->connected = NULL;
  }

  vergent->north = connpointline_create (obj,1);
  vergent->south = connpointline_create (obj,1);

  switch (GPOINTER_TO_INT (user_data)) {
    case VERGENT_OR:
    case VERGENT_AND:
      vergent->type = GPOINTER_TO_INT (user_data);
      break;
    default:
      g_warning ("in vergent_create(): incorrect user_data %p", user_data);
      vergent->type = VERGENT_OR;
  }

  vergent_update_data (vergent);

  *handle1 = &conn->endpoint_handles[0];
  *handle2 = &conn->endpoint_handles[1];

  return &vergent->connection.object;
}


static void
vergent_destroy (Vergent *vergent)
{
  connpointline_destroy (vergent->south);
  connpointline_destroy (vergent->north);
  connection_destroy (&vergent->connection);
}


static DiaObject *
vergent_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties (&vergent_type,
                                       obj_node,
                                       version,
                                       ctx);
}
