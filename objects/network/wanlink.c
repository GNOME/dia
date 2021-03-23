/* Dia -- an diagram creation/manipulation program
 *
 * Wan Link netwrok object
 * Code based on wanlink.c and bus.c
 *
 * Copyright (C) 1998 Alexander Larsson
 * Copyright (C) 2001 Hubert Figuiere
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
#include <math.h>

#include "config.h"
#include "intl.h"
#include "connection.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "network.h"
#include "dia-graphene.h"

#include "pixmaps/wanlink.xpm"

#define WANLINK_POLY_LEN 6


typedef struct _WanLink {
    Connection connection;

    Color line_color;
    Color fill_color;

    float width;
    Point poly[WANLINK_POLY_LEN];
} WanLink;

static DiaObject *wanlink_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void wanlink_destroy(WanLink *wanlink);
static void wanlink_draw (WanLink *wanlink, DiaRenderer *renderer);
static float wanlink_distance_from(WanLink *wanlink, Point *point);
static void wanlink_select(WanLink *wanlink, Point *clicked_point,
			 DiaRenderer *interactive_renderer);
static DiaObject *wanlink_copy(WanLink *wanlink);
static DiaObjectChange* wanlink_move(WanLink *wanlink, Point *to);
static DiaObjectChange* wanlink_move_handle(WanLink *wanlink, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason,
			      ModifierKeys modifiers);

static PropDescription *wanlink_describe_props(WanLink *wanlink);
static void wanlink_get_props(WanLink *wanlink, GPtrArray *props);
static void wanlink_set_props(WanLink *wanlink, GPtrArray *props);
static void wanlink_save(WanLink *wanlink, ObjectNode obj_node,
			 DiaContext *ctx);
static DiaObject *wanlink_load(ObjectNode obj_node, int version, DiaContext *ctx);
static void wanlink_update_data(WanLink *wanlink);

static ObjectTypeOps wanlink_type_ops =
{
  (CreateFunc) wanlink_create,
  (LoadFunc)   wanlink_load,
  (SaveFunc)   wanlink_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static ObjectOps wanlink_ops =
{
  (DestroyFunc)         wanlink_destroy,
  (DrawFunc)            wanlink_draw,
  (DistanceFunc)        wanlink_distance_from,
  (SelectFunc)          wanlink_select,
  (CopyFunc)            wanlink_copy,
  (MoveFunc)            wanlink_move,
  (MoveHandleFunc)      wanlink_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   wanlink_describe_props,
  (GetPropsFunc)        wanlink_get_props,
  (SetPropsFunc)        wanlink_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,

};


DiaObjectType wanlink_type = {
  "Network - WAN Link",        /* name */
  1,                           /* version */
  (const char **) wanlink_xpm, /* pixmap */
  &wanlink_type_ops            /* ops */
};


static PropDescription wanlink_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "width", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Width"), NULL, NULL },
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_DESC_END
};

static PropDescription *
wanlink_describe_props(WanLink *wanlink)
{
  if (wanlink_props[0].quark == 0)
    prop_desc_list_calculate_quarks(wanlink_props);
  return wanlink_props;
}

static PropOffset wanlink_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "width", PROP_TYPE_REAL, offsetof(WanLink, width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(WanLink, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(WanLink, fill_color) },
  { NULL, 0, 0 }
};

static void
wanlink_get_props(WanLink *wanlink, GPtrArray *props)
{
  object_get_props_from_offsets(&wanlink->connection.object, wanlink_offsets,
				props);
}

static void
wanlink_set_props(WanLink *wanlink, GPtrArray *props)
{
  object_set_props_from_offsets(&wanlink->connection.object, wanlink_offsets,
				props);
  wanlink_update_data(wanlink);
}

#define FLASH_LINE NETWORK_GENERAL_LINEWIDTH
#define FLASH_WIDTH 1.0
#define FLASH_HEIGHT 11.0
#define FLASH_BORDER 0.325

#define FLASH_BOTTOM (FLASH_HEIGHT)


static DiaObject *
wanlink_create(Point *startpoint,
	       void *user_data,
	       Handle **handle1,
	       Handle **handle2)
{
  WanLink *wanlink;
  Connection *conn;
  DiaObject *obj;
  int i;
  Point defaultpoly = {0.0, 0.0};
  Point defaultlen = { 5.0, 0.0 };

  wanlink = g_malloc0(sizeof(WanLink));

  conn = &wanlink->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = (DiaObject *) wanlink;

  obj->type = &wanlink_type;
  obj->ops = &wanlink_ops;

  connection_init(conn, 2, 0);

  for (i = 0; i < WANLINK_POLY_LEN ; i++)
      wanlink->poly[i] = defaultpoly;

  wanlink->width = FLASH_WIDTH;
  /* both colors where black at the time this was hardcoded ... */
  wanlink->line_color = color_black;
  wanlink->fill_color = color_black;

  wanlink->line_color = attributes_get_foreground();
  wanlink->fill_color = attributes_get_foreground();

  wanlink_update_data(wanlink);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (DiaObject *)wanlink;
}



static void
wanlink_destroy(WanLink *wanlink)
{
  connection_destroy(&wanlink->connection);
}


static void
wanlink_draw (WanLink *wanlink, DiaRenderer *renderer)
{
  assert (wanlink != NULL);
  assert (renderer != NULL);

  dia_renderer_set_linewidth (renderer, FLASH_LINE);
  dia_renderer_set_linejoin (renderer, LINEJOIN_MITER);
  dia_renderer_set_linestyle (renderer, LINESTYLE_SOLID, 0.0);

  dia_renderer_draw_polygon (renderer,
                             wanlink->poly,
                             WANLINK_POLY_LEN,
                             &wanlink->fill_color,
                             &wanlink->line_color);
}

static float
wanlink_distance_from(WanLink *wanlink, Point *point)
{
  return distance_polygon_point (wanlink->poly, WANLINK_POLY_LEN, wanlink->width, point);
}

static void
wanlink_select(WanLink *wanlink, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&wanlink->connection);
}

static DiaObject *
wanlink_copy(WanLink *wanlink)
{
  WanLink *newwanlink;
  Connection *conn, *newconn;

  conn = &wanlink->connection;

  newwanlink = g_malloc0(sizeof(WanLink));
  newconn = &newwanlink->connection;

  connection_copy(conn, newconn);

  newwanlink->width = wanlink->width;
  newwanlink->line_color = wanlink->line_color;
  newwanlink->fill_color = wanlink->fill_color;

  return (DiaObject *)newwanlink;
}

static DiaObjectChange*
wanlink_move(WanLink *wanlink, Point *to)
{
  Point delta;
  Point *endpoints = &wanlink->connection.endpoints[0];
  DiaObject *obj = (DiaObject *) wanlink;
  int i;

  delta = *to;
  point_sub(&delta, &obj->position);

  for (i=0;i<2;i++) {
    point_add(&endpoints[i], &delta);
  }

  wanlink_update_data(wanlink);

  return NULL;
}

static DiaObjectChange*
wanlink_move_handle(WanLink *wanlink, Handle *handle,
		    Point *to, ConnectionPoint *cp,
		    HandleMoveReason reason, ModifierKeys modifiers)
{
  connection_move_handle(&wanlink->connection, handle->id, to, cp,
			 reason, modifiers);
  connection_adjust_for_autogap(&wanlink->connection);

  wanlink_update_data(wanlink);

  return NULL;
}

static void
wanlink_save(WanLink *wanlink, ObjectNode obj_node,
	     DiaContext *ctx)
{
    AttributeNode attr;

    connection_save((Connection *)wanlink, obj_node, ctx);

    attr = new_attribute(obj_node, "width");
    data_add_real(attr, wanlink->width, ctx);

    data_add_color( new_attribute(obj_node, "line_color"), &wanlink->line_color, ctx);
    data_add_color( new_attribute(obj_node, "fill_color"), &wanlink->fill_color, ctx);
}

static DiaObject *
wanlink_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
    WanLink *wanlink;
    Connection *conn;
    DiaObject *obj;
    AttributeNode attr;
    DataNode data;

    wanlink = g_malloc0(sizeof(WanLink));

    conn = &wanlink->connection;
    obj = (DiaObject *) wanlink;

    obj->type = &wanlink_type;
    obj->ops = &wanlink_ops;

    connection_load(conn, obj_node, ctx);
    connection_init(conn, 2, 0);

    attr = object_find_attribute(obj_node, "width");
    if (attr != NULL) {
	data = attribute_first_data(attr);
	wanlink->width = data_real(data, ctx);
    }

    wanlink->line_color = color_black;
    attr = object_find_attribute(obj_node, "line_color");
    if (attr != NULL)
      data_color(attribute_first_data(attr), &wanlink->line_color, ctx);
    /* both colors where black at the time this was hardcoded ... */
    wanlink->fill_color = color_black;
    attr = object_find_attribute(obj_node, "fill_color");
    if (attr != NULL)
      data_color(attribute_first_data(attr), &wanlink->fill_color, ctx);

    wanlink_update_data (wanlink);

    return obj;
}


static void
wanlink_update_data (WanLink *wanlink)
{
  Connection *conn = &wanlink->connection;
  DiaObject *obj = (DiaObject *) wanlink;
  //Point v, vhat;
  graphene_vec2_t v, vhat;
  graphene_vec2_t endpoints[2];
  float width, width_2;
  float len, angle;
  Point origin;
  int i;
  graphene_matrix_t m;
  graphene_rect_t bbox;
  graphene_point_t pt;
  float tmp[GRAPHENE_VEC2_LEN];

  width = wanlink->width;
  width_2 = width / 2.0;

  if (connpoint_is_autogap (conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap (conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap (conn);
  }

  for (int j = 0; j < 2; j++) {
    dia_point_to_vec2 (&conn->endpoints[j], &endpoints[j]);
  }

  dia_vec2_to_point (&endpoints[0], &obj->position);

  graphene_vec2_init_from_vec2 (&v, &endpoints[1]);
  graphene_vec2_subtract (&v, &endpoints[0], &v);

  graphene_vec2_to_float (&v, tmp);

  if ((tmp[0] == 0.0) && (tmp[1] == 0.0)) {
    graphene_vec2_init (&v, tmp[0] + 0.01, tmp[1]);
  }

  graphene_vec2_init_from_vec2 (&vhat, &v);
  graphene_vec2_normalize (&vhat, &vhat);

  connection_update_boundingbox (conn);

  /** compute the polygon **/
  origin = wanlink->connection.endpoints [0];
  len = graphene_vec2_length (&v);

  graphene_vec2_to_float (&vhat, tmp);

  angle = atan2 (tmp[1], tmp[0]) - G_PI_2;

    /* The case of the wanlink */
  wanlink->poly[0].x = (width * 0.50) - width_2;
  wanlink->poly[0].y = (len * 0.00);
  wanlink->poly[1].x = (width * 0.50) - width_2;
  wanlink->poly[1].y = (len * 0.45);
  wanlink->poly[2].x = (width * 0.94) - width_2;
  wanlink->poly[2].y = (len * 0.45);
  wanlink->poly[3].x = (width * 0.50) - width_2;
  wanlink->poly[3].y = (len * 1.00);
  wanlink->poly[4].x = (width * 0.50) - width_2;
  wanlink->poly[4].y = (len * 0.55);
  wanlink->poly[5].x = (width * 0.06) - width_2;
  wanlink->poly[5].y = (len * 0.55);

  /* rotate */
  graphene_matrix_init_identity (&m);
  graphene_matrix_rotate_z (&m, DIA_DEGREES (angle));

  graphene_rect_init (&bbox, origin.x, origin.y, 0, 0);
  dia_point_to_graphene (&conn->endpoints[1], &pt);
  graphene_rect_expand (&bbox, &pt, &bbox);

  for (i = 0; i < WANLINK_POLY_LEN; i++) {
    graphene_point_t tpt;
    Point dpt;
    graphene_point_t new_pt;

    dia_point_to_graphene (&wanlink->poly[i], &tpt);
    graphene_matrix_transform_point (&m, &tpt, &new_pt);
    dia_graphene_to_point (&new_pt, &dpt);
    point_add (&dpt, &origin);
    wanlink->poly[i] = dpt;
  }

  /* calculate bounding box */
  {
    PolyBBExtras bbex = { 0, 0, wanlink->width / 2, 0, 0 };

    polyline_bbox (&wanlink->poly[0], WANLINK_POLY_LEN, &bbex, TRUE, &bbox);
  }

  dia_object_set_bounding_box (obj, &bbox);

  connection_update_handles (conn);
}
