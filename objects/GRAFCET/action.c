/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support
 * Copyright(C) 2000,2001 Cyrille Chepelov
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
#include "text.h"
#include "connpoint_line.h"

#include "grafcet.h"
#include "action_text_draw.h"

#include "pixmaps/action.xpm"

#define ACTION_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH
#define ACTION_FONT (DIA_FONT_SANS|DIA_FONT_BOLD)
#define ACTION_FONT_HEIGHT 0.8
#define ACTION_HEIGHT (2.0)

typedef struct _Action {
  Connection connection;

  Text *text;
  gboolean macro_call;

  /* computed values : */
  real space_width; /* width of a space in the current font
                     Fallacy! space is a very flexible thing in Pango...*/
  real label_width;
  DiaRectangle labelbb; /* The bounding box of the label itself */
  Point labelstart;

  ConnPointLine *cps; /* aaahrg ! again one ! */
} Action;

static DiaObjectChange* action_move_handle(Action *action, Handle *handle,
					Point *to, ConnectionPoint *cp,
					HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* action_move(Action *action, Point *to);
static void action_select(Action *action, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void action_draw(Action *action, DiaRenderer *renderer);
static DiaObject *action_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real action_distance_from(Action *action, Point *point);
static void action_update_data(Action *action);
static void action_destroy(Action *action);
static DiaObject *action_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *action_describe_props(Action *action);
static void action_get_props(Action *action,
                                 GPtrArray *props);
static void action_set_props(Action *action,
                                 GPtrArray *props);


static ObjectTypeOps action_type_ops =
{
  (CreateFunc)action_create,   /* create */
  (LoadFunc)  action_load,/*using_properties*/     /* load */
  (SaveFunc)  object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType action_type =
{
  "GRAFCET - Action", /* name */
  0,               /* version */
  action_xpm,       /* pixmap */
  &action_type_ops     /* ops */
};

static ObjectOps action_ops = {
  (DestroyFunc)         action_destroy,
  (DrawFunc)            action_draw,
  (DistanceFunc)        action_distance_from,
  (SelectFunc)          action_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            action_move,
  (MoveHandleFunc)      action_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   action_describe_props,
  (GetPropsFunc)        action_get_props,
  (SetPropsFunc)        action_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription action_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "text", PROP_TYPE_TEXT, 0,NULL,NULL},
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "macro_call", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Macro call"),N_("This action is a call to a macro-step")},
  PROP_DESC_END
};

static PropDescription *
action_describe_props(Action *action)
{
  if (action_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(action_props);
  }
  return action_props;
}

static PropOffset action_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Action,text)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Action,text),offsetof(Text,alignment)},
  {"text_font",PROP_TYPE_FONT,offsetof(Action,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Action,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Action,text),offsetof(Text,color)},
  {"macro_call",PROP_TYPE_BOOL,offsetof(Action,macro_call)},
  { NULL,0,0 }
};

static void
action_get_props(Action *action, GPtrArray *props)
{
  object_get_props_from_offsets(&action->connection.object,
                                action_offsets,props);
}

static void
action_set_props(Action *action, GPtrArray *props)
{
  object_set_props_from_offsets(&action->connection.object,
                                action_offsets,props);
  action_update_data(action);
}


static real
action_distance_from(Action *action, Point *point)
{
  Connection *conn = &action->connection;
  real dist; Point p1,p2;
  dist = distance_rectangle_point(&action->labelbb,point);
  p1.x = p2.x = (conn->endpoints[0].x+conn->endpoints[1].x)/2;
  p1.y = conn->endpoints[0].y; p2.y = conn->endpoints[0].y;
  dist = MIN(dist,distance_line_point(&conn->endpoints[0],&p1,
				      ACTION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&conn->endpoints[1],&p2,
				      ACTION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&p2,&p1,ACTION_LINE_WIDTH,point));
  return dist;
}

static void
action_select(Action *action, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  action_update_data(action);
  text_grab_focus(action->text, &action->connection.object);
}

static DiaObjectChange*
action_move_handle(Action *action, Handle *handle,
		   Point *to, ConnectionPoint *cp,
		   HandleMoveReason reason, ModifierKeys modifiers)
{
  g_assert(action!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

#if 0
  if (handle->id == HANDLE_MOVE_STARTPOINT) {
    Point to2;
    /* move also the second point */
    to2 = *to;
    point_sub(&to2,&action->connection.endpoints[0]);
    point_add(&to2,&action->connection.endpoints[1]);
    connection_move_handle(&action->connection, HANDLE_MOVE_ENDPOINT,
			   to, NULL, reason, 0);
  }
#endif
  connection_move_handle(&action->connection, handle->id, to, cp,
			 reason, modifiers);
  action_update_data(action);

  return NULL;
}


static DiaObjectChange*
action_move(Action *action, Point *to)
{
  Point start_to_end;
  Point *endpoints = &action->connection.endpoints[0];

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  action_update_data(action);

  return NULL;
}

static void
action_update_data(Action *action)
{
  Point p1,p2;
  real x,x1;
  real left,right;
  int i;
  real chunksize;
  Connection *conn = &action->connection;
  DiaObject *obj = &conn->object;

  obj->position = conn->endpoints[0];
  connection_update_boundingbox(conn);

  /* compute the label's width and bounding box */
  action->space_width = action_text_spacewidth(action->text);

  action->labelstart = conn->endpoints[1];
  action->labelbb.left = action->labelstart.x;
  action->labelstart.y += .3 * action->text->height;
  action->labelstart.x = action->labelbb.left + action->space_width;
  if (action->macro_call) {
    action->labelstart.x += 2.0 * action->space_width;
  }
  text_set_position(action->text,&action->labelstart);

  action_text_calc_boundingbox(action->text,&action->labelbb);

  if (action->macro_call) {
    action->labelbb.right += 2.0 * action->space_width;
  }
  action->labelbb.top = conn->endpoints[1].y - .5*ACTION_HEIGHT;
  action->labelbb.bottom = action->labelstart.y + .5*ACTION_HEIGHT;

  action->label_width = action->labelbb.right -
    action->labelbb.left;


  /* Adjust the count and positions of the condition connection points. */


  left = x = conn->endpoints[1].x;
  right = left + action->label_width;
  p1.x = conn->endpoints[1].x;
  p1.y = conn->endpoints[1].y - .5 * ACTION_HEIGHT;
  p2.y = p1.y + ACTION_HEIGHT;
  connpointline_adjust_count(action->cps,2+(2 * action->text->numlines), &p1);

  for (i=0; i<action->text->numlines; i++) {
    chunksize = text_get_line_width(action->text, i);
    x1 = x + 1.0;
    if (x1 >= right) {
      x1 = right - ACTION_LINE_WIDTH;
    }
    p1.x = p2.x = x1;

    obj->connections[(2*i) + 2]->pos = p1;
    obj->connections[(2*i) + 2]->directions = DIR_NORTH;
    obj->connections[(2*i) + 3]->pos = p2;
    obj->connections[(2*i) + 3]->directions = DIR_SOUTH;

    x = x + chunksize + 2 * action->space_width;
  }
  p1.y = p2.y = conn->endpoints[1].y;
  p1.x = left; p2.x = right;
  obj->connections[0]->pos = p1;
  obj->connections[0]->directions = DIR_WEST;
  obj->connections[1]->pos = p2;
  obj->connections[1]->directions = DIR_EAST;

  /* fix boundingbox for line_width: */
  action->labelbb.top -= ACTION_LINE_WIDTH/2;
  action->labelbb.left -= ACTION_LINE_WIDTH/2;
  action->labelbb.bottom += ACTION_LINE_WIDTH/2;
  action->labelbb.right += ACTION_LINE_WIDTH/2;

  rectangle_union(&obj->bounding_box,&action->labelbb);
  connection_update_handles(conn);
}


static void
action_draw (Action *action, DiaRenderer *renderer)
{
  Connection *conn = &action->connection;
  Point ul,br,p1,p2;
  int i;
  real chunksize;
  Color cl;

  dia_renderer_set_linewidth (renderer, ACTION_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  /* first, draw the line or polyline from the step to the action label */
  if (conn->endpoints[0].y == conn->endpoints[1].y) {
    /* simpler case */
    dia_renderer_draw_line (renderer,
                            &conn->endpoints[0],
                            &conn->endpoints[1],
                            &color_black);
  } else {
    Point pts[4];
    pts[0] = conn->endpoints[0];
    pts[3] = conn->endpoints[1];
    pts[1].y = pts[0].y;
    pts[2].y = pts[3].y;
    pts[1].x = pts[2].x = .5 * (pts[0].x + pts[3].x);

    dia_renderer_draw_polyline (renderer,
                                pts,
                                sizeof(pts)/sizeof(pts[0]),
                                &color_black);
  }

  /* Now, draw the action label. */
  ul.x = conn->endpoints[1].x;
  ul.y = conn->endpoints[1].y - .5 * ACTION_HEIGHT;
  br.x = ul.x + action->label_width;
  br.y = ul.y + ACTION_HEIGHT;

  dia_renderer_draw_rect (renderer, &ul, &br, &color_white, NULL);

  action_text_draw (action->text,renderer);

  p1.x = p2.x = ul.x;
  p1.y = ul.y; p2.y = br.y;

  for (i=0; i<action->text->numlines-1; i++) {
    chunksize = text_get_line_width (action->text, i);
    p1.x = p2.x = p1.x + chunksize + 2 * action->space_width;
    dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
  }

  if (action->macro_call) {
    p1.x = p2.x = ul.x + 2.0 * action->space_width;
    dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
    p1.x = p2.x = br.x - 2.0 * action->space_width;
    dia_renderer_draw_line (renderer, &p1, &p2, &color_black);
  }

  cl.red = 1.0; cl.blue = cl.green = .2; cl.alpha = 1.0;
  dia_renderer_draw_rect (renderer, &ul, &br, NULL, &color_black);
}

static DiaObject *
action_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Action *action;
  Connection *conn;
  DiaObject *obj;
  LineBBExtras *extra;
  Point defaultlen  = {1.0,0.0}, pos;

  DiaFont* action_font;

  action = g_new0 (Action, 1);
  conn = &action->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &action_type;
  obj->ops = &action_ops;

  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  connection_init(conn, 2,0);
  action->cps = connpointline_create(obj,0);

  pos = conn->endpoints[1];
  action_font = dia_font_new_from_style (ACTION_FONT, ACTION_FONT_HEIGHT);
  action->text = new_text ("",
                           action_font,
                           ACTION_FONT_HEIGHT,
                           &pos, /* never used */
                           &color_black,
                           DIA_ALIGN_LEFT);
  g_clear_object (&action_font);

  action->macro_call = FALSE;

  extra->start_long =
    extra->start_trans =
    extra->end_trans =
    extra->end_long = ACTION_LINE_WIDTH/2.0;

  action_update_data(action);

  conn->endpoint_handles[1].connect_type = HANDLE_NONCONNECTABLE;

  *handle1 = &conn->endpoint_handles[0];
  *handle2 = &conn->endpoint_handles[1];

  return &action->connection.object;
}

static void
action_destroy(Action *action)
{
  text_destroy(action->text);
  connpointline_destroy(action->cps);
  connection_destroy(&action->connection);
}

static DiaObject *
action_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&action_type,
                                      obj_node,version,ctx);
}












