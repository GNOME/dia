/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "diamenu.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/flow.xpm"

Color flow_color_energy   = { 1.0f, 0.0f, 0.0f, 1.0f };
Color flow_color_material = { 0.8f, 0.0f, 0.8f, 1.0f };
Color flow_color_signal   = { 0.0f, 0.0f, 1.0f, 1.0f };

typedef struct _Flow Flow;
typedef enum {
  FLOW_ENERGY,
  FLOW_MATERIAL,
  FLOW_SIGNAL
} FlowType;

struct _Flow {
  Connection connection;

  Handle text_handle;

  Text* text;
  FlowType type;
  Point textpos; /* This is the master position, but overridden in load */
};

#define FLOW_WIDTH 0.1
#define FLOW_MATERIAL_WIDTH 0.2
#define FLOW_DASHLEN 0.4
#define FLOW_FONTHEIGHT 0.8
#define FLOW_ARROWLEN 0.8
#define FLOW_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)

static DiaObjectChange *flow_move_handle       (Flow             *flow,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange *flow_move              (Flow             *flow,
                                                Point            *to);
static void flow_select(Flow *flow, Point *clicked_point,
			DiaRenderer *interactive_renderer);
static void flow_draw(Flow *flow, DiaRenderer *renderer);
static DiaObject *flow_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static real flow_distance_from(Flow *flow, Point *point);
static void flow_update_data(Flow *flow);
static void flow_destroy(Flow *flow);
static DiaObject *flow_copy(Flow *flow);
static void flow_save(Flow *flow, ObjectNode obj_node,
		      DiaContext *ctx);
static DiaObject *flow_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *flow_describe_props(Flow *mes);
static void
flow_get_props(Flow * flow, GPtrArray *props);
static void
flow_set_props(Flow * flow, GPtrArray *props);
static DiaMenu *flow_get_object_menu(Flow *flow, Point *clickedpoint) ;


static ObjectTypeOps flow_type_ops =
{
  (CreateFunc)		flow_create,
  (LoadFunc)		flow_load,
  (SaveFunc)		flow_save,
  (GetDefaultsFunc)	NULL,
  (ApplyDefaultsFunc)	NULL,
} ;

DiaObjectType flow_type =
{
  "FS - Flow",  /* name */
  0,         /* version */
  flow_xpm,   /* pixmap */
  &flow_type_ops /* ops */
};

static ObjectOps flow_ops = {
  (DestroyFunc)         flow_destroy,
  (DrawFunc)            flow_draw,
  (DistanceFunc)        flow_distance_from,
  (SelectFunc)          flow_select,
  (CopyFunc)            flow_copy,
  (MoveFunc)            flow_move,
  (MoveHandleFunc)      flow_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      flow_get_object_menu,
  (DescribePropsFunc)   flow_describe_props,
  (GetPropsFunc)        flow_get_props,
  (SetPropsFunc)        flow_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_flow_type_data[] = {
  { N_("Energy"),FLOW_ENERGY },
  { N_("Material"),FLOW_MATERIAL },
  { N_("Signal"),FLOW_SIGNAL },
  { NULL, 0 }
};

static PropDescription flow_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Type:"), NULL, prop_flow_type_data },
  { "text", PROP_TYPE_TEXT, 0, NULL, NULL },
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  /* Colour determined from type, don't show */
  { "text_colour", PROP_TYPE_COLOUR, PROP_FLAG_DONT_SAVE, },
  PROP_DESC_END
};

static PropDescription *
flow_describe_props(Flow *mes)
{
  if (flow_props[0].quark == 0)
    prop_desc_list_calculate_quarks(flow_props);
  return flow_props;

}

static PropOffset flow_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Flow, type) },
  { "text", PROP_TYPE_TEXT, offsetof (Flow, text) },
  { "text_alignment", PROP_TYPE_ENUM, offsetof(Flow,text),offsetof(Text,alignment) },
  { "text_font", PROP_TYPE_FONT, offsetof(Flow,text),offsetof(Text,font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Flow,text),offsetof(Text,height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(Flow,text),offsetof(Text,color) },
  { NULL, 0, 0 }
};

static void
flow_get_props(Flow * flow, GPtrArray *props)
{
  object_get_props_from_offsets(&flow->connection.object,
                                flow_offsets, props);
}

static void
flow_set_props(Flow *flow, GPtrArray *props)
{
  object_set_props_from_offsets(&flow->connection.object,
                                flow_offsets, props);
  flow_update_data(flow);
}


static real
flow_distance_from(Flow *flow, Point *point)
{
  Point *endpoints;
  real linedist;
  real textdist;

  endpoints = &flow->connection.endpoints[0];

  linedist = distance_line_point(&endpoints[0], &endpoints[1],
				 flow->type == FLOW_MATERIAL ? FLOW_MATERIAL_WIDTH : FLOW_WIDTH,
				 point);
  textdist = text_distance_from( flow->text, point ) ;

  return linedist > textdist ? textdist : linedist ;
}

static void
flow_select(Flow *flow, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  text_set_cursor(flow->text, clicked_point, interactive_renderer);
  text_grab_focus(flow->text, &flow->connection.object);

  connection_update_handles(&flow->connection);
}


static DiaObjectChange *
flow_move_handle (Flow             *flow,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  Point p1, p2;
  Point *endpoints;

  g_return_val_if_fail (flow != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    flow->textpos = *to;
  } else  {
    real dest_length ;
    real orig_length2 ;
    real along_mag, norm_mag ;
    Point along ;

    endpoints = &flow->connection.endpoints[0];
    p1 = flow->textpos ;
    point_sub( &p1, &endpoints[0] ) ;

    p2 = endpoints[1] ;
    point_sub( &p2, &endpoints[0] ) ;
    orig_length2= point_dot( &p2, &p2 ) ;
    along = p2 ;
    if ( orig_length2 > 1e-5 ) {
      along_mag = point_dot( &p2, &p1 )/sqrt(orig_length2) ;
      along_mag *= along_mag ;
      norm_mag = point_dot( &p1, &p1 ) ;
      norm_mag = (real)sqrt( (double)( norm_mag - along_mag ) ) ;
      along_mag = (real)sqrt( along_mag/orig_length2 ) ;
      if ( p1.x*p2.y - p1.y*p2.x > 0.0 )
	norm_mag = -norm_mag ;
    } else {
      along_mag = 0.5 ;
      norm_mag = (real)sqrt( (double) point_dot( &p1, &p1 ) ) ;
    }

    connection_move_handle(&flow->connection, handle->id, to, cp,
			   reason, modifiers);
    connection_adjust_for_autogap(&flow->connection);

    p2 = endpoints[1] ;
    point_sub( &p2, &endpoints[0] ) ;
    flow->textpos = endpoints[0] ;
    along = p2 ;
    p2.x = -along.y ;
    p2.y = along.x ;
    dest_length = point_dot( &p2, &p2 ) ;
    if ( dest_length > 1e-5 ) {
      point_normalize( &p2 ) ;
    } else {
      p2.x = 0.0 ; p2.y = -1.0 ;
    }
    point_scale( &p2, norm_mag ) ;
    point_scale( &along, along_mag ) ;
    point_add( &flow->textpos, &p2 ) ;
    point_add( &flow->textpos, &along ) ;
  }

  flow_update_data(flow);

  return NULL;
}


static DiaObjectChange *
flow_move (Flow *flow, Point *to)
{
  Point start_to_end;
  Point *endpoints = &flow->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&flow->textpos, &delta);

  flow_update_data(flow);

  return NULL;
}


static void
flow_draw (Flow *flow, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2;
  Arrow arrow;
  int n1 = 1, n2 = 0;
  Color* render_color = NULL;

  g_return_if_fail (flow != NULL);
  g_return_if_fail (renderer != NULL);

  arrow.type = ARROW_FILLED_TRIANGLE;
  arrow.width = FLOW_ARROWWIDTH;
  arrow.length = FLOW_ARROWLEN;

  endpoints = &flow->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, FLOW_WIDTH);

  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  switch (flow->type) {
    case FLOW_SIGNAL:
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, FLOW_DASHLEN);
      render_color = &flow_color_signal ;
      break ;
    case FLOW_MATERIAL:
      dia_renderer_set_linewidth (renderer, FLOW_MATERIAL_WIDTH);
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
      render_color = &flow_color_material;
      break ;
    case FLOW_ENERGY:
      render_color = &flow_color_energy;
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
      break;
    default:
      g_return_if_reached ();
  }

  p1 = endpoints[n1];
  p2 = endpoints[n2];

  dia_renderer_draw_line_with_arrows (renderer,
                                      &p1,
                                      &p2,
                                      FLOW_WIDTH,
                                      render_color,
                                      &arrow,
                                      NULL);

  text_draw (flow->text, renderer);
}

static DiaObject *
flow_create(Point *startpoint,
	    void *user_data,
	    Handle **handle1,
	    Handle **handle2)
{
  Flow *flow;
  Connection *conn;
  DiaObject *obj;
  LineBBExtras *extra;
  Point p ;
  Point n ;
  DiaFont *font;

  flow = g_new0 (Flow, 1);

  conn = &flow->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &flow_type;
  obj->ops = &flow_ops;

  connection_init(conn, 3, 0);

  p = conn->endpoints[1] ;
  point_sub( &p, &conn->endpoints[0] ) ;
  point_scale( &p, 0.5 ) ;
  n.x = p.y ;
  n.y = -p.x ;
  if ( fabs(n.x) < 1.e-5 && fabs(n.y) < 1.e-5 ) {
    n.x = 0. ;
    n.y = -1. ;
  } else {
    point_normalize( &n ) ;
  }
  point_scale( &n, 0.5*FLOW_FONTHEIGHT ) ;

  point_add( &p, &n ) ;
  point_add( &p, &conn->endpoints[0] ) ;
  flow->textpos = p;

  font = dia_font_new_from_style (DIA_FONT_SANS, FLOW_FONTHEIGHT);

  flow->text = new_text ("",
                         font,
                         FLOW_FONTHEIGHT,
                         &p,
                         &color_black,
                         DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  flow->text_handle.id = HANDLE_MOVE_TEXT;
  flow->text_handle.type = HANDLE_MINOR_CONTROL;
  flow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  flow->text_handle.connected_to = NULL;
  flow->text_handle.pos = p;
  obj->handles[2] = &flow->text_handle;

  extra->start_long =
    extra->end_long =
    extra->start_trans = FLOW_WIDTH/2.0;
  extra->end_trans = MAX(FLOW_WIDTH, FLOW_ARROWLEN) / 2.0;
  flow_update_data(flow);
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return &flow->connection.object;
}


static void
flow_destroy(Flow *flow)
{
  connection_destroy(&flow->connection);
  text_destroy(flow->text) ;
}

static DiaObject *
flow_copy(Flow *flow)
{
  Flow *newflow;
  Connection *conn, *newconn;
  DiaObject *newobj;

  conn = &flow->connection;

  newflow = g_new0 (Flow, 1);
  newconn = &newflow->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newflow->text_handle = flow->text_handle;
  newflow->text_handle.connected_to = NULL;
  newobj->handles[2] = &newflow->text_handle;
  newflow->textpos = flow->textpos;
  newflow->text = text_copy(flow->text);
  newflow->type = flow->type;

  flow_update_data(newflow);
  return &newflow->connection.object;
}


static void
flow_update_data(Flow *flow)
{
  Connection *conn = &flow->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle rect;
  Color* color = NULL;

  if (connpoint_is_autogap(flow->connection.endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(flow->connection.endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  obj->position = conn->endpoints[0];

  switch (flow->type) {
    case FLOW_ENERGY:
      color = &flow_color_energy ;
      break;
    case FLOW_MATERIAL:
      color = &flow_color_material ;
      break;
    case FLOW_SIGNAL:
      color = &flow_color_signal ;
      break;
    default:
      g_return_if_reached ();
  }
  text_set_color (flow->text, color);

  flow->text->position = flow->textpos;
  flow->text_handle.pos = flow->textpos;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  text_calc_boundingbox(flow->text, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);
}


static void
flow_save(Flow *flow, ObjectNode obj_node, DiaContext *ctx)
{
  connection_save(&flow->connection, obj_node, ctx);

  data_add_text(new_attribute(obj_node, "text"),
		flow->text, ctx) ;
  data_add_int(new_attribute(obj_node, "type"),
	       flow->type, ctx);
}

static DiaObject *
flow_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Flow *flow;
  AttributeNode attr;
  Connection *conn;
  DiaObject *obj;
  LineBBExtras *extra;

  flow = g_new0 (Flow, 1);

  conn = &flow->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &flow_type;
  obj->ops = &flow_ops;

  connection_load(conn, obj_node, ctx);

  connection_init(conn, 3, 0);

  flow->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    flow->text = data_text(attribute_first_data(attr), ctx);
  else { /* pathologic */
    DiaFont *font = dia_font_new_from_style (DIA_FONT_SANS, FLOW_FONTHEIGHT);

    flow->text = new_text ("",
                           font,
                           FLOW_FONTHEIGHT,
                           &obj->position,
                           &color_black,
                           DIA_ALIGN_CENTRE);
    g_clear_object (&font);
  }

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    flow->type = (FlowType)data_int(attribute_first_data(attr), ctx);

  flow->text_handle.id = HANDLE_MOVE_TEXT;
  flow->text_handle.type = HANDLE_MINOR_CONTROL;
  flow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  flow->text_handle.connected_to = NULL;
  flow->text_handle.pos = flow->text->position;
  obj->handles[2] = &flow->text_handle;

  extra->start_long =
    extra->end_long =
    extra->start_trans = FLOW_WIDTH/2.0;
  extra->end_trans = MAX(FLOW_WIDTH, FLOW_ARROWLEN) / 2.0;

  flow->textpos = flow->text->position;
  flow_update_data(flow);

  return &flow->connection.object;
}


#define DIA_FS_TYPE_FLOW_OBJECT_CHANGE dia_fs_flow_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaFSFlowObjectChange,
                      dia_fs_flow_object_change,
                      DIA_FS, FLOW_OBJECT_CHANGE,
                      DiaObjectChange)


struct _DiaFSFlowObjectChange {
  DiaObjectChange obj_change;
  int old_type;
  int new_type;
};


DIA_DEFINE_OBJECT_CHANGE (DiaFSFlowObjectChange, dia_fs_flow_object_change)


static void
dia_fs_flow_object_change_free (DiaObjectChange *change)
{
}


static void
dia_fs_flow_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaFSFlowObjectChange *change = DIA_FS_FLOW_OBJECT_CHANGE (self);
  Flow *flow = (Flow *) obj;

  flow->type = change->new_type;
  flow_update_data (flow);
}


static void
dia_fs_flow_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaFSFlowObjectChange *change = DIA_FS_FLOW_OBJECT_CHANGE (self);
  Flow *flow = (Flow*) obj;

  flow->type = change->old_type;
  flow_update_data (flow);
}


static DiaObjectChange *
type_create_change (Flow *flow, int type)
{
  DiaFSFlowObjectChange *change;

  change = dia_object_change_new (DIA_FS_TYPE_FLOW_OBJECT_CHANGE);

  change->old_type = flow->type;
  change->new_type = type;

  return DIA_OBJECT_CHANGE (change);
}


static DiaObjectChange *
flow_set_type_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  DiaObjectChange *change;

  change = type_create_change ((Flow *) obj, GPOINTER_TO_INT (data));
  dia_object_change_apply (change, obj);

  return change;
}


static DiaMenuItem flow_menu_items[] = {
  { N_("Energy"), flow_set_type_callback, (void*)FLOW_ENERGY, 1 },
  { N_("Material"), flow_set_type_callback, (void*)FLOW_MATERIAL, 1 },
  { N_("Signal"), flow_set_type_callback, (void*)FLOW_SIGNAL, 1 },
};

static DiaMenu flow_menu = {
  "Flow",
  sizeof(flow_menu_items)/sizeof(DiaMenuItem),
  flow_menu_items,
  NULL
};

static DiaMenu *
flow_get_object_menu(Flow *flow, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  flow_menu_items[0].active = 1;
  return &flow_menu;
}
