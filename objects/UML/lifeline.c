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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "properties.h"
#include "connpoint_line.h"
#include "attributes.h"

#include "pixmaps/lifeline.xpm"

typedef struct _Lifeline Lifeline;

#define LIFELINE_NUM_HANDLES 5
#define LIFELINE_NUM_STD_CPS 7

struct _Lifeline {
  Connection connection;

  ConnectionPoint connections[LIFELINE_NUM_STD_CPS]; /* 7°th cp position is at rbot handle location
                                     (this is useful to attach another lifeline
                                      just below this one in case of an object
                                      activated more than once in a sequence diagram.)*/
  Handle boxbot_handle;
  Handle boxtop_handle;
  Handle boxmid_handle;

  /* these are _relative_ to endpoints[0] */
  real rtop, rbot;
  real cp_distance;

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

#define LIFELINE_CP_DEFAULT_DISTANCE 0.5
#define LIFELINE_CP_DISTANCE_INCREASE_FACTOR 0.25
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
#define HANDLE_BOXMID (HANDLE_CUSTOM3)

static DiaObjectChange *lifeline_move_handle   (Lifeline         *lifeline,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange *lifeline_move          (Lifeline         *lifeline,
                                                Point            *to);
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
static DiaObject *lifeline_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *lifeline_describe_props(Lifeline *lifeline);

static void lifeline_get_props(Lifeline * lifeline, GPtrArray *props);
static void lifeline_set_props(Lifeline * lifeline, GPtrArray *props);
static DiaMenu *lifeline_get_object_menu(Lifeline *lifeline,
					Point *clickedpoint);


typedef enum {
  LIFELINE_CHANGE_ADD = 0x01,
  LIFELINE_CHANGE_DEL = 0x02,
  LIFELINE_CHANGE_INC = 0x03,
  LIFELINE_CHANGE_DEC = 0x04,
  LIFELINE_CHANGE_DEF = 0x05
} LifelineChangeType;

static DiaObjectChange *lifeline_create_change (Lifeline           *lifeline,
                                                LifelineChangeType  changetype,
                                                Point              *clicked);

static ObjectTypeOps lifeline_type_ops =
{
  (CreateFunc) lifeline_create,
  (LoadFunc)   lifeline_load,/* using properties */
  (SaveFunc)   object_save_using_properties
};

DiaObjectType lifeline_type =
{
  "UML - Lifeline", /* name */
  0,             /* version */
  lifeline_xpm,   /* pixmap */
  &lifeline_type_ops /* ops */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      lifeline_get_object_menu,
  (DescribePropsFunc)   lifeline_describe_props,
  (GetPropsFunc)        lifeline_get_props,
  (SetPropsFunc)        lifeline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription lifeline_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  { "rtop", PROP_TYPE_REAL, PROP_FLAG_NO_DEFAULTS, NULL,NULL,NULL},
  { "rbot", PROP_TYPE_REAL, PROP_FLAG_NO_DEFAULTS, NULL,NULL,NULL},
  { "draw_focus", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw focus of control:"), NULL, NULL },
  { "draw_cross", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Draw destruction mark:"), NULL, NULL },
  { "cpl_northwest",PROP_TYPE_CONNPOINT_LINE, PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS, NULL, NULL },
  { "cpl_southwest",PROP_TYPE_CONNPOINT_LINE, PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS, NULL, NULL },
  { "cpl_northeast",PROP_TYPE_CONNPOINT_LINE, PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS, NULL, NULL },
  { "cpl_southeast",PROP_TYPE_CONNPOINT_LINE, PROP_FLAG_OPTIONAL|PROP_FLAG_NO_DEFAULTS, NULL, NULL },
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

  /* This magic is necessary to correctly load data
     from old lifeline behavior. It calculates the
     cp_distance in function of saved rtop-rbot distance and
     num of cp.
     ( It's just the inverse of lifeline_rect_size :)
  */
  lifeline->cp_distance = (lifeline->rbot - lifeline->rtop)
                          / (lifeline->northwest->num_connections + 1 +
			     lifeline->southwest->num_connections + 1);
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


static inline real
lifeline_rect_size(Lifeline *lifeline)
{
   return (  (lifeline->northwest->num_connections+1) * lifeline->cp_distance
           + (lifeline->southwest->num_connections+1) * lifeline->cp_distance);
}

static void
lifeline_select(Lifeline *lifeline, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&lifeline->connection);
}

static gboolean
lifeline_point_above_mid (Lifeline *lifeline,
			  Point    *pt)
{
  return (pt->y < lifeline->boxmid_handle.pos.y);
}


/*!
 * Moving handles of a lifeline
 *
 * - the top handle is usually connected to an object, when moved connected
 * the whole lifeline is moved horizontally (but may not any longer vertically).
 * - the top box handle is used to move the whole box vertically.
 * - the bottom box handle should maybe resize the box without changing the
 *   connection point distance, i.e. adding/removing some (maybe limited
 *   by connected points)
 * - the bottom handle just move itself, not beyond the lower box handle
 */
static DiaObjectChange *
lifeline_move_handle (Lifeline         *lifeline,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  double s, dy;
  Connection *conn;

  g_return_val_if_fail (lifeline != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  conn = &lifeline->connection;
  if (handle->id == HANDLE_BOXBOT) {
    /* distance between upper handle and boxtop must not be smaller than zero */
    dy = to->y - conn->endpoints[0].y;
    if (dy > lifeline_rect_size(lifeline)) {
      real dist = dy - lifeline->rbot;
      real di;

      modf (dist, &di);
      /* the integer part gives the number of points to add or remove */
      if (fabs (di) > 0) {
        int ni = (int)di;

	if (lifeline_point_above_mid (lifeline, to) ?
	      lifeline->northeast->num_connections + ni > 0 :
	      lifeline->southeast->num_connections + ni > 0)
	  return lifeline_create_change (lifeline, ni > 0 ? LIFELINE_CHANGE_ADD : LIFELINE_CHANGE_DEL, to);
	else
	  return NULL;
      }
    }
  } else if (handle->id == HANDLE_BOXMID) {
    /* the box can not move over the top - same logic, but old movement behavior as above */
    real dist = (to->y - handle->pos.y); /* the potential movement */
    if (dist > 0 ||
        -dist < lifeline->rtop) {
      lifeline->rbot += dist;
      lifeline->rtop = lifeline->rbot - lifeline_rect_size(lifeline);
    }
  } else if (handle->id == HANDLE_BOXTOP) {
    /* Distance between upper handle and boxtop must not be smaller than zero,
     * Same for boxbot and lower handle */
    dy = to->y - conn->endpoints[0].y;
    if (dy > 0 &&
      dy + lifeline_rect_size(lifeline) < conn->endpoints[1].y) {
      lifeline->rtop = dy;
    }
  } else {
    /* move horizontally only if startpoint is moved */
    if (handle->id==HANDLE_MOVE_STARTPOINT) {
      conn->endpoints[0].x = conn->endpoints[1].x = to->x;
    } else {
      to->x = conn->endpoints[0].x;
    }
    /* If connected don't change size */
    dy = (reason==HANDLE_MOVE_CONNECTED) ?
	  conn->endpoints[1].y - conn->endpoints[0].y : lifeline->rbot;
    connection_move_handle(conn, handle->id, to, cp, reason, modifiers);
    s = conn->endpoints[1].y - conn->endpoints[0].y;
    if (handle->id==HANDLE_MOVE_ENDPOINT && s < dy &&
        s > lifeline->rtop + LIFELINE_BOXMINHEIGHT)
      lifeline->rbot = s;
    else if (reason==HANDLE_MOVE_CONNECTED || s < dy)
      conn->endpoints[1].y = conn->endpoints[0].y + dy;
  }

  lifeline_update_data(lifeline);

  return NULL;
}


static DiaObjectChange *
lifeline_move (Lifeline *lifeline, Point *to)
{
  Point start_to_end;
  Point delta;
  Point *endpoints = &lifeline->connection.endpoints[0];

  delta = *to;
  point_sub (&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub (&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add (&endpoints[1], &start_to_end);

  lifeline_update_data (lifeline);

  return NULL;
}


static void
lifeline_draw (Lifeline *lifeline, DiaRenderer *renderer)
{
  Point *endpoints, p1, p2;

  g_return_if_fail (lifeline != NULL);
  g_return_if_fail (renderer != NULL);

  endpoints = &lifeline->connection.endpoints[0];

  dia_renderer_set_linewidth (renderer, LIFELINE_LINEWIDTH);
  dia_renderer_set_linestyle (renderer,
                              DIA_LINE_STYLE_DASHED,
                              LIFELINE_DASHLEN);

  /* Ok, instead rendering one big line between two endpoints we just
     from endpoints to rtop and rbottom respectively.
     This trick lets dashed line displayed correctly if we attach another
     lifeline object to "rbot" connectionpoint
  */
  p1.x = p2.x = endpoints[0].x;
  p1.y = endpoints[0].y + lifeline->rtop;
  p2.y = endpoints[0].y + lifeline->rbot;
  dia_renderer_draw_line (renderer,
                          &endpoints[0],
                          &p1,
                          &lifeline->line_color);
  dia_renderer_draw_line (renderer,
                          &p2,
                          &endpoints[1],
                          &lifeline->line_color);


  dia_renderer_set_linewidth (renderer, LIFELINE_BOXWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  p1.x = endpoints[0].x - LIFELINE_WIDTH/2.0;
  p1.y = endpoints[0].y + lifeline->rtop;
  p2.x = endpoints[0].x + LIFELINE_WIDTH/2.0;
  p2.y = endpoints[0].y + lifeline->rbot;

  if (lifeline->draw_focuscontrol) {
    dia_renderer_draw_rect (renderer,
                            &p1,
                            &p2,
                            &lifeline->fill_color,
                            &lifeline->line_color);
  }

  if (lifeline->draw_cross) {
    dia_renderer_set_linewidth (renderer, LIFELINE_CROSSWIDTH);
    p1.x = endpoints[1].x + LIFELINE_CROSSLEN;
    p2.x = endpoints[1].x - LIFELINE_CROSSLEN;
    p1.y = endpoints[1].y + LIFELINE_CROSSLEN;
    p2.y = endpoints[1].y - LIFELINE_CROSSLEN;
    dia_renderer_draw_line (renderer,
                            &p1,
                            &p2,
                            &lifeline->line_color);
    p1.y = p2.y;
    p2.y = endpoints[1].y + LIFELINE_CROSSLEN;
    dia_renderer_draw_line (renderer,
                            &p1,
                            &p2,
                            &lifeline->line_color);
  }
}


#define DIA_UML_TYPE_LIFELINE_OBJECT_CHANGE dia_uml_lifeline_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaUMLLifelineObjectChange,
                      dia_uml_lifeline_object_change,
                      DIA_UML, LIFELINE_OBJECT_CHANGE,
                      DiaObjectChange)


/* DiaObject menu handling */
struct _DiaUMLLifelineObjectChange {
  DiaObjectChange obj_change;

  DiaObjectChange *east, *west;
  double cp_distance_change;
  LifelineChangeType type;
};


DIA_DEFINE_OBJECT_CHANGE (DiaUMLLifelineObjectChange,
                          dia_uml_lifeline_object_change)


static void
dia_uml_lifeline_object_change_apply (DiaObjectChange *self, DiaObject *obj)
{
  DiaUMLLifelineObjectChange *change = DIA_UML_LIFELINE_OBJECT_CHANGE (self);

  if (change->type == LIFELINE_CHANGE_ADD || change->type == LIFELINE_CHANGE_DEL) {
    dia_object_change_apply (change->west,obj);
    dia_object_change_apply (change->east,obj);
  } else {
    ((Lifeline*) obj)->cp_distance += change->cp_distance_change;
  }
}


static void
dia_uml_lifeline_object_change_revert (DiaObjectChange *self, DiaObject *obj)
{
  DiaUMLLifelineObjectChange *change = DIA_UML_LIFELINE_OBJECT_CHANGE (self);

  if (change->type == LIFELINE_CHANGE_ADD || change->type == LIFELINE_CHANGE_DEL) {
    dia_object_change_revert (change->west,obj);
    dia_object_change_revert (change->east,obj);
  } else {
    ((Lifeline*) obj)->cp_distance -= change->cp_distance_change;
  }
}


static void
dia_uml_lifeline_object_change_free (DiaObjectChange *self)
{
  DiaUMLLifelineObjectChange *change = DIA_UML_LIFELINE_OBJECT_CHANGE (self);

  if (change->type == LIFELINE_CHANGE_ADD ||
      change->type == LIFELINE_CHANGE_DEL) {
    g_clear_pointer (&change->east, dia_object_change_unref);
    g_clear_pointer (&change->west, dia_object_change_unref);
  }
}


static DiaObjectChange *
lifeline_create_change (Lifeline           *lifeline,
                        LifelineChangeType  changetype,
                        Point              *clicked)
{
  DiaUMLLifelineObjectChange *vc;

  vc = dia_object_change_new (DIA_UML_TYPE_LIFELINE_OBJECT_CHANGE);

  vc->type = changetype;

  switch (vc->type) {
    case LIFELINE_CHANGE_ADD:
      if (lifeline_point_above_mid (lifeline, clicked)) {
        vc->east = connpointline_add_point(lifeline->northeast,clicked);
        vc->west = connpointline_add_point(lifeline->northwest,clicked);
      } else {
        vc->east = connpointline_add_point(lifeline->southeast,clicked);
        vc->west = connpointline_add_point(lifeline->southwest,clicked);
      }
      break;
    case LIFELINE_CHANGE_DEL:
      if (lifeline_point_above_mid (lifeline, clicked)) {
        vc->east = connpointline_remove_point(lifeline->northeast,clicked);
        vc->west = connpointline_remove_point(lifeline->northwest,clicked);
      } else {
        vc->east = connpointline_remove_point(lifeline->southeast,clicked);
        vc->west = connpointline_remove_point(lifeline->southwest,clicked);
      }
      break;
    case LIFELINE_CHANGE_INC:
      vc->cp_distance_change = LIFELINE_CP_DISTANCE_INCREASE_FACTOR;
      lifeline->cp_distance += vc->cp_distance_change;
      break;
    case LIFELINE_CHANGE_DEC:
      vc->cp_distance_change = -1 * LIFELINE_CP_DISTANCE_INCREASE_FACTOR;
      lifeline->cp_distance += vc->cp_distance_change;
      break;
    case LIFELINE_CHANGE_DEF:
      vc->cp_distance_change = LIFELINE_CP_DEFAULT_DISTANCE*2 - lifeline->cp_distance;
      lifeline->cp_distance += vc->cp_distance_change;
      break;
    default:
      g_return_val_if_reached (NULL);
  }

  lifeline_update_data (lifeline);

  return DIA_OBJECT_CHANGE (vc);
}


static DiaObjectChange *
lifeline_cp_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  LifelineChangeType type = GPOINTER_TO_INT (data);
  return lifeline_create_change ((Lifeline *) obj, type, clicked);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection points"), lifeline_cp_callback, GINT_TO_POINTER(LIFELINE_CHANGE_ADD), 1 },
  { N_("Remove connection points"), lifeline_cp_callback, GINT_TO_POINTER(LIFELINE_CHANGE_DEL), 1 },
  { N_("Increase connection points distance"), lifeline_cp_callback, GINT_TO_POINTER(LIFELINE_CHANGE_INC), 1 },
  { N_("Decrease connection points distance"), lifeline_cp_callback, GINT_TO_POINTER(LIFELINE_CHANGE_DEC), 1 },
  { N_("Set default connection points distance"), lifeline_cp_callback, GINT_TO_POINTER(LIFELINE_CHANGE_DEF), 1 },
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
            (lifeline->southwest->num_connections ==
             lifeline->southeast->num_connections) );

  object_menu_items[0].active = 1;
  /* don't allow to remove the last connection point */
  if (lifeline_point_above_mid (lifeline, clickedpoint))
    object_menu_items[1].active = (lifeline->northeast->num_connections > 1);
  else
    object_menu_items[1].active = (lifeline->southeast->num_connections > 1);

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

  lifeline = g_new0 (Lifeline, 1);
  lifeline->cp_distance = LIFELINE_CP_DEFAULT_DISTANCE;

  conn = &lifeline->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[0].x += LIFELINE_WIDTH/2;
  conn->endpoints[1] = conn->endpoints[0];
  conn->endpoints[1].y = conn->endpoints[0].y + 6.0*lifeline->cp_distance;

  obj = &conn->object;

  obj->type = &lifeline_type;
  obj->ops = &lifeline_ops;

  connection_init(conn, LIFELINE_NUM_HANDLES, LIFELINE_NUM_STD_CPS);

  lifeline->line_color = attributes_get_foreground();
  lifeline->fill_color = attributes_get_background();

  /* _relative_ from conn->endpoints[0].y */
  lifeline->rtop = lifeline->cp_distance;
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

  lifeline->boxmid_handle.id = HANDLE_BOXMID;
  lifeline->boxmid_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxmid_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxmid_handle.connected_to = NULL;
  obj->handles[4] = &lifeline->boxmid_handle;

  /* Only the start point should be connectable */
  obj->handles[1]->connect_type = HANDLE_NONCONNECTABLE;

  /* Connection points */
  for (i=0;i<LIFELINE_NUM_STD_CPS;i++) {
    obj->connections[i] = &lifeline->connections[i];
    lifeline->connections[i].object = obj;
    lifeline->connections[i].connected = NULL;
  }

  /* **must** be the same init order as in the property descriptors */
  lifeline->northwest = connpointline_create(obj, 1);
  lifeline->southwest = connpointline_create(obj, 1);
  lifeline->northeast = connpointline_create(obj, 1);
  lifeline->southeast = connpointline_create(obj, 1);

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

  /* Update lifeline rbot using num. of cp and cp_distance */
  lifeline->rbot =  lifeline->rtop + lifeline_rect_size( lifeline ) ;
  /* Update conn->endpoints[0].y if rbot is greater */
  if( conn->endpoints[1].y < conn->endpoints[0].y + lifeline->rbot )
     conn->endpoints[1].y = conn->endpoints[0].y + lifeline->rbot + lifeline->cp_distance;


  /* box handles: */
  p1.x = conn->endpoints[0].x;
  p1.y = conn->endpoints[0].y + lifeline->rtop;
  lifeline->boxtop_handle.pos = p1;
  p2.x = p1.x;
  p2.y = conn->endpoints[0].y + lifeline->rbot;
  lifeline->boxbot_handle.pos = p2;
  /* middle handle - between the cpls */
  lifeline->boxmid_handle.pos.x = p1.x;
  lifeline->boxmid_handle.pos.y = p1.y + (lifeline->cp_distance * (lifeline->northwest->num_connections + 1));

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

  pmw.y = pme.y = (p1.y + lifeline->cp_distance * (lifeline->northwest->num_connections+1));

  lifeline->connections[6].pos.x = conn->endpoints[0].x;
  lifeline->connections[6].pos.y = conn->endpoints[0].y + lifeline->rbot;
  lifeline->connections[6].directions = DIR_SOUTH;

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
  connpointline_putonaline(lifeline->northwest,&pnw,&pmw, DIR_WEST);
  connpointline_update(lifeline->southwest);
  connpointline_putonaline(lifeline->southwest,&pmw,&psw, DIR_WEST);
  connpointline_update(lifeline->northeast);
  connpointline_putonaline(lifeline->northeast,&pne,&pme, DIR_EAST);
  connpointline_update(lifeline->southeast);
  connpointline_putonaline(lifeline->southeast,&pme,&pse, DIR_EAST);
}

static DiaObject *
lifeline_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&lifeline_type,
                                                obj_node,version,ctx);

  return obj;
}
