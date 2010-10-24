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

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#include "pixmaps/wanlink.xpm"

#define WANLINK_POLY_LEN 6


typedef struct _WanLink {
    Connection connection;

    Color line_color;
    Color fill_color;
    
    real width;
    Point poly[WANLINK_POLY_LEN];
} WanLink;

static DiaObject *wanlink_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void wanlink_destroy(WanLink *wanlink);
static void wanlink_draw (WanLink *wanlink, DiaRenderer *renderer);
static real wanlink_distance_from(WanLink *wanlink, Point *point);
static void wanlink_select(WanLink *wanlink, Point *clicked_point,
			 DiaRenderer *interactive_renderer);
static DiaObject *wanlink_copy(WanLink *wanlink);
static ObjectChange* wanlink_move(WanLink *wanlink, Point *to);
static ObjectChange* wanlink_move_handle(WanLink *wanlink, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, 
			      ModifierKeys modifiers);

static PropDescription *wanlink_describe_props(WanLink *wanlink);
static void wanlink_get_props(WanLink *wanlink, GPtrArray *props);
static void wanlink_set_props(WanLink *wanlink, GPtrArray *props);
static void wanlink_save(WanLink *wanlink, ObjectNode obj_node,
			 const char *filename);
static DiaObject *wanlink_load(ObjectNode obj_node, int version,
			    const char *filename);
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

DiaObjectType wanlink_type =
{
  "Network - WAN Link",   /* name */
  1,                     /* version */
  (char **) wanlink_xpm, /* pixmap */

  &wanlink_type_ops      /* ops */
};

static PropDescription wanlink_props[] = {
  OBJECT_COMMON_PROPERTIES,
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
  OBJECT_COMMON_PROPERTIES_OFFSETS,
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
    DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
        
    assert (wanlink != NULL);
    assert (renderer != NULL);
    
    renderer_ops->set_linewidth(renderer, FLASH_LINE);
    renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
    renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
    
    
    renderer_ops->fill_polygon(renderer, wanlink->poly,  WANLINK_POLY_LEN, &wanlink->fill_color);
    renderer_ops->draw_polygon(renderer, wanlink->poly,  WANLINK_POLY_LEN, &wanlink->line_color);
}

static real
wanlink_distance_from(WanLink *wanlink, Point *point)
{
  Point *endpoints;
  real min_dist;

  /* TODO: handle the fact that this is not a line */
  endpoints = &wanlink->connection.endpoints[0];
  min_dist = distance_line_point( &endpoints[0], &endpoints[1],
                                  wanlink->width, point);
  return min_dist;
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

static ObjectChange*
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

static ObjectChange*
wanlink_move_handle(WanLink *wanlink, Handle *handle,
		    Point *to, ConnectionPoint *cp,
		    HandleMoveReason reason, ModifierKeys modifiers)
{
  connection_move_handle(&wanlink->connection, handle->id, to, cp,
			 reason, modifiers);
  
  wanlink_update_data(wanlink);

  return NULL;
}

static void
wanlink_save(WanLink *wanlink, ObjectNode obj_node,
	     const char *filename)
{
    AttributeNode attr;
  
    connection_save((Connection *)wanlink, obj_node);
    
    attr = new_attribute(obj_node, "width");
    data_add_real(attr, wanlink->width);
    
    data_add_color( new_attribute(obj_node, "line_color"), &wanlink->line_color);
    data_add_color( new_attribute(obj_node, "fill_color"), &wanlink->fill_color);
}

static DiaObject *
wanlink_load(ObjectNode obj_node, int version, const char *filename)
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

    connection_load(conn, obj_node);
    connection_init(conn, 2, 0);
    
    attr = object_find_attribute(obj_node, "width");
    if (attr != NULL) {
	data = attribute_first_data(attr);
	wanlink->width = data_real( data);
    }

    wanlink->line_color = color_black;
    attr = object_find_attribute(obj_node, "line_color");
    if (attr != NULL)
        data_color(attribute_first_data(attr), &wanlink->line_color);
    /* both colors where black at the time this was hardcoded ... */
    wanlink->fill_color = color_black;
    attr = object_find_attribute(obj_node, "fill_color");
    if (attr != NULL)
        data_color(attribute_first_data(attr), &wanlink->fill_color);
    
    wanlink_update_data (wanlink);
    
    return obj;
}

static void
wanlink_update_data(WanLink *wanlink)
{
  Connection *conn = &wanlink->connection;
  DiaObject *obj = (DiaObject *) wanlink;
  Point v, vhat;
  Point *endpoints;
  real width, width_2;
  real len, angle;
  Point origin;
  int i;
  Matrix m;
  

  width = wanlink->width;
  width_2 = width / 2.0;
  
  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  endpoints = &conn->endpoints[0]; 
  obj->position = endpoints[0];

  v = endpoints[1];
  point_sub(&v, &endpoints[0]);
  if ((fabs(v.x) == 0.0) && (fabs(v.y)==0.0)) {
    v.x += 0.01;
  }
  vhat = v;
  point_normalize(&vhat);

  connection_update_boundingbox(conn);

  /** compute the polygon **/
  origin = wanlink->connection.endpoints [0];
  len = point_len (&v);

  angle = atan2 (vhat.y, vhat.x) - M_PI_2;

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
  identity_matrix (m);
  rotate_matrix (m, angle);

  obj->bounding_box.top = origin.y;
  obj->bounding_box.left = origin.x;
  obj->bounding_box.bottom = conn->endpoints[1].y;
  obj->bounding_box.right = conn->endpoints[1].x;
  for (i = 0; i <  WANLINK_POLY_LEN; i++) 
  {
      Point new_pt;
      
      transform_point (m, &wanlink->poly[i], 
		       &new_pt);
      point_add (&new_pt, &origin);
      wanlink->poly[i] = new_pt;
      if (wanlink->poly [i].y < obj->bounding_box.top)
	  obj->bounding_box.top = wanlink->poly [i].y;
      if (wanlink->poly [i].x < obj->bounding_box.left)
	  obj->bounding_box.left = wanlink->poly [i].x;
      if (wanlink->poly [i].y > obj->bounding_box.bottom)
	  obj->bounding_box.bottom = wanlink->poly [i].y;
      if (wanlink->poly [i].x > obj->bounding_box.right)
	  obj->bounding_box.right = wanlink->poly [i].x;
  }


  connection_update_handles(conn);
}
