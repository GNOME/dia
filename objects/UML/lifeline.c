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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "properties.h"
#include "connpoint_line.h"
#include "attributes.h"

#include "pixmaps/lifeline.xpm"

typedef struct _Lifeline Lifeline;

    
struct _Lifeline {
  Connection connection;  

  ConnectionPoint connections[6]; /* the static ones. 6 is meant to 
                                     be hardcoded. */
  Handle boxbot_handle;
  Handle boxtop_handle;

  real rtop, rbot;
    
  int draw_focuscontrol;
  int draw_cross;
  
  Color line_color;
  Color fill_color;

  ConnPointLine *northwest,*southwest,*northeast,*southeast;

  /* we're (almost) obliged to do this stupid gymnastic with twin 
     connpoint_lines, because we really do want to reload older objects 
     (those created before we had the CPLs) without funny side effects. And 
     we don't want to have two connection points (one static, one dynamic) in 
     the same place either.
  */
};

#define LIFELINE_LINEWIDTH 0.05
#define LIFELINE_BOXWIDTH 0.1
#define LIFELINE_WIDTH 0.7
#define LIFELINE_HEIGHT 3.0
#define LIFELINE_BOXMINHEIGHT 0.5
#define LIFELINE_DASHLEN 0.4
#define LIFELINE_CROSSWIDTH 0.12
#define LIFELINE_CROSSLEN 0.8

#define HANDLE_BOXTOP (HANDLE_CUSTOM1)
#define HANDLE_BOXBOT (HANDLE_CUSTOM2)

static ObjectChange* lifeline_move_handle(Lifeline *lifeline, Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, 
                                 ModifierKeys modifiers);
static ObjectChange* lifeline_move(Lifeline *lifeline, Point *to);
static void lifeline_select(Lifeline *lifeline, Point *clicked_point,
                            DiaRenderer *interactive_renderer);
static void lifeline_draw(Lifeline *lifeline, DiaRenderer *renderer);
static DiaObject *lifeline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real lifeline_distance_from(Lifeline *lifeline, Point *point);
static void lifeline_update_data(Lifeline *lifeline);
static void lifeline_destroy(Lifeline *lifeline);
static DiaObject *lifeline_load(ObjectNode obj_node, int version,
			     const char *filename);
static PropDescription *lifeline_describe_props(Lifeline *lifeline);

static void lifeline_get_props(Lifeline * lifeline, GPtrArray *props);
static void lifeline_set_props(Lifeline * lifeline, GPtrArray *props);
static DiaMenu *lifeline_get_object_menu(Lifeline *lifeline,
					Point *clickedpoint);



static ObjectTypeOps lifeline_type_ops =
{
  (CreateFunc) lifeline_create,
  (LoadFunc)   lifeline_load,/* using properties */
  (SaveFunc)   object_save_using_properties
};

DiaObjectType lifeline_type =
{
  "UML - Lifeline",   /* name */
  0,                   /* version */
  (char **) lifeline_xpm,  /* pixmap */
  &lifeline_type_ops       /* ops */
};

static ObjectOps lifeline_ops = {
  (DestroyFunc)         lifeline_destroy,
  (DrawFunc)            lifeline_draw,
  (DistanceFunc)        lifeline_distance_from,
  (SelectFunc)          lifeline_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            lifeline_move,
  (MoveHandleFunc)      lifeline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      lifeline_get_object_menu,
  (DescribePropsFunc)   lifeline_describe_props,
  (GetPropsFunc)        lifeline_get_props,
  (SetPropsFunc)        lifeline_set_props
};

static PropDescription lifeline_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR_OPTIONAL, 
  PROP_STD_FILL_COLOUR_OPTIONAL, 
  { "rtop", PROP_TYPE_REAL, 0, NULL,NULL,NULL},
  { "rbot", PROP_TYPE_REAL, 0, NULL,NULL,NULL},
  { "draw_focus", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw focus of control:"), NULL, NULL },
  { "draw_cross", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw destruction mark:"), NULL, NULL },
  { "cpl_northwest",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL },
  { "cpl_southwest",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL },
  { "cpl_northeast",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL },
  { "cpl_southeast",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
lifeline_describe_props(Lifeline *lifeline)
{
  if (lifeline_props[0].quark == 0)
    prop_desc_list_calculate_quarks(lifeline_props);
  return lifeline_props;
}

static PropOffset lifeline_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Lifeline,line_color)},
  { "fill_colour",PROP_TYPE_COLOUR,offsetof(Lifeline,fill_color)},
  { "draw_focus", PROP_TYPE_BOOL, offsetof(Lifeline, draw_focuscontrol) },
  { "draw_cross", PROP_TYPE_BOOL, offsetof(Lifeline, draw_cross) },
  { "rtop", PROP_TYPE_REAL, offsetof(Lifeline, rtop) },
  { "rbot", PROP_TYPE_REAL, offsetof(Lifeline, rbot) },
  { "cpl_northwest",PROP_TYPE_CONNPOINT_LINE,offsetof(Lifeline,northwest)},
  { "cpl_southwest",PROP_TYPE_CONNPOINT_LINE,offsetof(Lifeline,southwest)},
  { "cpl_northeast",PROP_TYPE_CONNPOINT_LINE,offsetof(Lifeline,northeast)},
  { "cpl_southeast",PROP_TYPE_CONNPOINT_LINE,offsetof(Lifeline,southeast)},
  { NULL, 0, 0 },
};

static void
lifeline_get_props(Lifeline * lifeline, GPtrArray *props)
{
  object_get_props_from_offsets(&lifeline->connection.object, 
                                lifeline_offsets, props);
}

static void
lifeline_set_props(Lifeline *lifeline, GPtrArray *props)
{
  object_set_props_from_offsets(&lifeline->connection.object, 
                                lifeline_offsets, props);
  lifeline_update_data(lifeline);
}

static real
lifeline_distance_from(Lifeline *lifeline, Point *point)
{
  Point *endpoints;
  real dist1, dist2;
  
  endpoints = &lifeline->connection.endpoints[0];
  dist1 = distance_line_point( &endpoints[0], &endpoints[1],
			      LIFELINE_WIDTH, point);
  dist2 = dist1;
  
  return MIN(dist1, dist2);
}

static void
lifeline_select(Lifeline *lifeline, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&lifeline->connection);
}

static ObjectChange*
lifeline_move_handle(Lifeline *lifeline, Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason, ModifierKeys modifiers)
{
  real s, t;
  Connection *conn;

  assert(lifeline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  conn = &lifeline->connection;
  if (handle->id == HANDLE_BOXBOT) {
      t = to->y - conn->endpoints[0].y;
      if (t > LIFELINE_BOXMINHEIGHT && 
	  t < conn->endpoints[1].y - conn->endpoints[0].y) {
	  lifeline->rbot = t;
	  if (t < lifeline->rtop + LIFELINE_BOXMINHEIGHT)
	      lifeline->rtop = t - LIFELINE_BOXMINHEIGHT;
      }
  } else if (handle->id == HANDLE_BOXTOP) {
      t = to->y - conn->endpoints[0].y;
      if (t > 0 && 
	  t < conn->endpoints[1].y-conn->endpoints[0].y-LIFELINE_BOXMINHEIGHT) {
	  lifeline->rtop = t;	
	  if (t > lifeline->rbot - LIFELINE_BOXMINHEIGHT)
	      lifeline->rbot = t + LIFELINE_BOXMINHEIGHT;
      }
  } else {
    /* move horizontally only if startpoint is moved */
    if (handle->id==HANDLE_MOVE_STARTPOINT) {
	conn->endpoints[0].x = conn->endpoints[1].x = to->x;
    } else {
	to->x = conn->endpoints[0].x;
    }
    /* If connected don't change size */  
    t = (reason==HANDLE_MOVE_CONNECTED) ? 
	conn->endpoints[1].y - conn->endpoints[0].y:
	lifeline->rbot;
    connection_move_handle(conn, handle->id, to, cp, reason, modifiers);
    s = conn->endpoints[1].y - conn->endpoints[0].y;
    if (handle->id==HANDLE_MOVE_ENDPOINT && s < t && s > lifeline->rtop + LIFELINE_BOXMINHEIGHT)
	lifeline->rbot = s;
    else if (reason==HANDLE_MOVE_CONNECTED || s < t)
	conn->endpoints[1].y = conn->endpoints[0].y + t;
  }

  lifeline_update_data(lifeline);

  return NULL;
}

static ObjectChange*
lifeline_move(Lifeline *lifeline, Point *to)
{
  Point start_to_end;
  Point delta;
  Point *endpoints = &lifeline->connection.endpoints[0]; 

  delta = *to;
  point_sub(&delta, &endpoints[0]);
  
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);
  
  lifeline_update_data(lifeline);

  return NULL;
}

static void
lifeline_draw(Lifeline *lifeline, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point *endpoints, p1, p2;
  
  assert(lifeline != NULL);
  assert(renderer != NULL);

  endpoints = &lifeline->connection.endpoints[0];
  
  renderer_ops->set_linewidth(renderer, LIFELINE_LINEWIDTH);    
  renderer_ops->set_dashlength(renderer, LIFELINE_DASHLEN);
  renderer_ops->set_linestyle(renderer, LINESTYLE_DASHED);

  renderer_ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &lifeline->line_color);


  renderer_ops->set_linewidth(renderer, LIFELINE_BOXWIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1.x = endpoints[0].x - LIFELINE_WIDTH/2.0;
  p1.y = endpoints[0].y + lifeline->rtop;
  p2.x = endpoints[0].x + LIFELINE_WIDTH/2.0;
  p2.y = endpoints[0].y + lifeline->rbot;

  if (lifeline->draw_focuscontrol) {  
      renderer_ops->fill_rect(renderer, 
			       &p1, &p2,
			       &lifeline->fill_color);
  
      renderer_ops->draw_rect(renderer, 
			       &p1, &p2,
			       &lifeline->line_color);
  }
    
  if (lifeline->draw_cross) {      
      renderer_ops->set_linewidth(renderer, LIFELINE_CROSSWIDTH);
      p1.x = endpoints[1].x + LIFELINE_CROSSLEN;
      p2.x = endpoints[1].x - LIFELINE_CROSSLEN;
      p1.y = endpoints[1].y + LIFELINE_CROSSLEN;
      p2.y = endpoints[1].y - LIFELINE_CROSSLEN;
      renderer_ops->draw_line(renderer,
			       &p1, &p2,
			       &lifeline->line_color);
      p1.y = p2.y;
      p2.y = endpoints[1].y + LIFELINE_CROSSLEN;
      renderer_ops->draw_line(renderer,
			       &p1, &p2,
			       &lifeline->line_color);
      
  }
}

/* DiaObject menu handling */

typedef struct {
  ObjectChange obj_change;
  
  ObjectChange *northeast,*southeast,*northwest,*southwest;
} LifelineChange;

static void lifeline_change_apply(LifelineChange *change, DiaObject *obj)
{
  change->northwest->apply(change->northwest,obj);
  change->southwest->apply(change->southwest,obj);
  change->northeast->apply(change->northeast,obj);
  change->southeast->apply(change->southeast,obj);
}

static void lifeline_change_revert(LifelineChange *change, DiaObject *obj)
{
  change->northwest->revert(change->northwest,obj);
  change->southwest->revert(change->southwest,obj);
  change->northeast->revert(change->northeast,obj);
  change->southeast->revert(change->southeast,obj);
}

static void lifeline_change_free(LifelineChange *change) 
{
  if (change->northeast->free) change->northeast->free(change->northeast);
  g_free(change->northeast);
  if (change->northwest->free) change->northwest->free(change->northwest);
  g_free(change->northwest);
  if (change->southeast->free) change->southeast->free(change->southeast);
  g_free(change->southeast);
  if (change->southwest->free) change->southwest->free(change->southwest);
  g_free(change->southwest);
}

static ObjectChange *
lifeline_create_change(Lifeline *lifeline, int add, Point *clicked) 
{
  LifelineChange *vc;
 
  vc = g_new0(LifelineChange,1);
  vc->obj_change.apply = (ObjectChangeApplyFunc)lifeline_change_apply;
  vc->obj_change.revert = (ObjectChangeRevertFunc)lifeline_change_revert;
  vc->obj_change.free = (ObjectChangeFreeFunc)lifeline_change_free;
  
  if (add) {
    vc->northeast = connpointline_add_point(lifeline->northeast,clicked);
    vc->northwest = connpointline_add_point(lifeline->northwest,clicked);
    vc->southeast = connpointline_add_point(lifeline->southeast,clicked);
    vc->southwest = connpointline_add_point(lifeline->southwest,clicked);
  } else {
    vc->northeast = connpointline_remove_point(lifeline->northeast,clicked);
    vc->southwest = connpointline_remove_point(lifeline->southwest,clicked);
    vc->southeast = connpointline_remove_point(lifeline->southeast,clicked);
    vc->northwest = connpointline_remove_point(lifeline->northwest,clicked);
  }
  lifeline_update_data(lifeline);
  return (ObjectChange *)vc;
}

static ObjectChange *
lifeline_add_cp_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  return lifeline_create_change((Lifeline *)obj,1,clicked);
}

static ObjectChange *
lifeline_delete_cp_callback(DiaObject *obj, Point *clicked, gpointer data)
{ 
  return lifeline_create_change((Lifeline *)obj,0,clicked); 
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add connection points"), lifeline_add_cp_callback, NULL, 1 },
  { N_("Remove connection points"), lifeline_delete_cp_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  N_("UML Lifeline"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
lifeline_get_object_menu(Lifeline *lifeline, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  g_assert( (lifeline->northwest->num_connections == 
             lifeline->northeast->num_connections) || 
            (lifeline->northwest->num_connections == 
             lifeline->southwest->num_connections) ||
            (lifeline->southwest->num_connections == 
             lifeline->southeast->num_connections) );

  object_menu_items[0].active = 1;
  object_menu_items[1].active = (lifeline->northeast->num_connections > 1);

  return &object_menu;
}

static DiaObject *
lifeline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Lifeline *lifeline;
  Connection *conn;
  DiaObject *obj;
  int i;

  lifeline = g_malloc0(sizeof(Lifeline));

  conn = &lifeline->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[0].x += LIFELINE_WIDTH/2;
  conn->endpoints[1] = conn->endpoints[0];
  conn->endpoints[1].y += LIFELINE_HEIGHT; 
 
  obj = &conn->object;
  
  obj->type = &lifeline_type;
  obj->ops = &lifeline_ops;

  connection_init(conn, 4, 6);

  lifeline->line_color = attributes_get_foreground();
  lifeline->fill_color = attributes_get_background();

  lifeline->rtop = LIFELINE_HEIGHT/3;
  lifeline->rbot = lifeline->rtop+0.7;
  lifeline->draw_focuscontrol = 1;
  lifeline->draw_cross = 0;
    
  lifeline->boxbot_handle.id = HANDLE_BOXBOT;
  lifeline->boxbot_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxbot_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxbot_handle.connected_to = NULL;
  obj->handles[2] = &lifeline->boxbot_handle;
  
  lifeline->boxtop_handle.id = HANDLE_BOXTOP;
  lifeline->boxtop_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxtop_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxtop_handle.connected_to = NULL;
  obj->handles[3] = &lifeline->boxtop_handle;

  /* Only the start point should be connectable */
  obj->handles[1]->connect_type = HANDLE_NONCONNECTABLE;

  /* Connection points */
  for (i=0;i<6;i++) {
    obj->connections[i] = &lifeline->connections[i];
    lifeline->connections[i].object = obj;
    lifeline->connections[i].connected = NULL;
  }

  /* **must** be the same init order as in the property descriptors */
  lifeline->northwest = connpointline_create(obj,1);
  lifeline->southwest = connpointline_create(obj,1);
  lifeline->northeast = connpointline_create(obj,1);
  lifeline->southeast = connpointline_create(obj,1);

  lifeline_update_data(lifeline);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return &lifeline->connection.object;
}

static void
lifeline_destroy(Lifeline *lifeline)
{
  connpointline_destroy(lifeline->southeast);
  connpointline_destroy(lifeline->northwest);
  connpointline_destroy(lifeline->northeast);
  connpointline_destroy(lifeline->southwest);
  connection_destroy(&lifeline->connection);
}

static void
lifeline_update_data(Lifeline *lifeline)
{
  Connection *conn = &lifeline->connection;
  DiaObject *obj = &conn->object;
  LineBBExtras *extra = &conn->extra_spacing;
  Point p1, p2, pnw, psw, pne, pse, pmw,pme;

  obj->position = conn->endpoints[0];

  /* box handles: */
  p1.x = conn->endpoints[0].x;
  p1.y = conn->endpoints[0].y + lifeline->rtop;
  lifeline->boxtop_handle.pos = p1;
  p2.x = p1.x;
  p2.y = conn->endpoints[0].y + lifeline->rbot;
  lifeline->boxbot_handle.pos = p2;

  connection_update_handles(conn);

  /* Boundingbox: */
  extra->start_trans =
    extra->start_long = 
    extra->end_long =
    extra->end_trans = LIFELINE_LINEWIDTH/2.0;
  if (lifeline->draw_focuscontrol) {
    extra->start_trans =
      extra->end_trans = MAX(LIFELINE_LINEWIDTH/2,(LIFELINE_WIDTH/2+LIFELINE_BOXWIDTH/2));
  }
  if (lifeline->draw_cross) {
    extra->end_trans += LIFELINE_CROSSLEN;
    extra->end_long += LIFELINE_CROSSLEN;
  }
  connection_update_boundingbox(conn);

  if (lifeline->draw_focuscontrol) {  
      p1.x -= LIFELINE_WIDTH/2.0;
      p2.x += LIFELINE_WIDTH/2.0; 
  }
  /* Update connections: */      

  pnw.x = p1.x; pnw.y = p1.y;
  psw.x = p1.x; psw.y = p2.y;
  pne.x = p2.x; pne.y = p1.y;
  pse.x = p2.x; pse.y = p2.y;
  pmw.x = pnw.x;
  pme.x = pne.x;
  pmw.y = pme.y = (p1.y + p2.y)/2;

  lifeline->connections[0].pos = pnw;
  lifeline->connections[1].pos = pne;
  lifeline->connections[2].pos = pmw;
  lifeline->connections[3].pos = pme;
  lifeline->connections[4].pos = psw;
  lifeline->connections[5].pos = pse;
  lifeline->connections[0].directions = DIR_NORTH|DIR_WEST;
  lifeline->connections[1].directions = DIR_NORTH|DIR_EAST;
  lifeline->connections[2].directions = DIR_WEST;
  lifeline->connections[3].directions = DIR_EAST;
  lifeline->connections[4].directions = DIR_SOUTH|DIR_WEST;
  lifeline->connections[5].directions = DIR_SOUTH|DIR_EAST;

  connpointline_update(lifeline->northwest);
  connpointline_putonaline(lifeline->northwest,&pnw,&pmw);
  connpointline_update(lifeline->southwest);
  connpointline_putonaline(lifeline->southwest,&pmw,&psw);
  connpointline_update(lifeline->northeast);
  connpointline_putonaline(lifeline->northeast,&pne,&pme);
  connpointline_update(lifeline->southeast);
  connpointline_putonaline(lifeline->southeast,&pme,&pse);
}

static DiaObject *
lifeline_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&lifeline_type,
                                      obj_node,version,filename);
}








 
