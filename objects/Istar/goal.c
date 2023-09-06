/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Objects for drawing i-star goals
 * This class supports the whole goal specialization hierarchy
 * Copyright (C) 2002 Christophe Ponsard
 *
 * Based on SADT box object
 * Copyright (C) 2000, 2001 Cyrille Chepelov
 *
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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
#include <glib.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "connpoint_line.h"
#include "color.h"
#include "properties.h"

#include "pixmaps/softgoal.xpm"

#define DEFAULT_WIDTH 3.0
#define DEFAULT_HEIGHT 1
#define DEFAULT_PADDING 0.4
#define DEFAULT_FONT 0.7
#define GOAL_LINE_WIDTH 0.12
#define GOAL_FG_COLOR color_black
#define GOAL_BG_COLOR color_white
#define GOAL_OFFSET 0.5

#define NUM_CONNECTIONS 9

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef enum {
  SOFTGOAL,
  GOAL
} GoalType;

static PropEnumData prop_goal_type_data[] = {
  { N_("Softgoal"), SOFTGOAL },
  { N_("Goal"),     GOAL },
  { NULL, 0}
};

typedef struct _Goal {
  Element element;
  ConnectionPoint connector[NUM_CONNECTIONS];

  Text *text;
  real padding;
  GoalType type;

} Goal;

static real goal_distance_from(Goal *goal, Point *point);
static void goal_select(Goal *goal, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange* goal_move_handle(Goal *goal, Handle *handle,
			    Point *to, ConnectionPoint *cp,
			    HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* goal_move(Goal *goal, Point *to);
static void goal_draw(Goal *goal, DiaRenderer *renderer);
static void goal_update_data(Goal *goal, AnchorShape horix, AnchorShape vert);
static DiaObject *goal_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void goal_destroy(Goal *goal);
static DiaObject *goal_load(ObjectNode obj_node, int version,
                            DiaContext *ctx);

static PropDescription *goal_describe_props(Goal *goal);
static void goal_get_props(Goal *goal, GPtrArray *props);
static void goal_set_props(Goal *goal, GPtrArray *props);

static void update_softgoal_connectors(ConnectionPoint *c, Point p, double w, double h);
static void update_goal_connectors(ConnectionPoint *c, Point p, double w, double h);
static void compute_cloud(Goal *goal, BezPoint* bpl);

static ObjectTypeOps istar_goal_type_ops =
{
  (CreateFunc) goal_create,
  (LoadFunc)   goal_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType istar_goal_type =
{
  "Istar - goal",       /* name */
  0,                 /* version */
  istar_softgoal_xpm, /* pixmap */
  &istar_goal_type_ops,  /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

static ObjectOps goal_ops = {
  (DestroyFunc)         goal_destroy,
  (DrawFunc)            goal_draw,
  (DistanceFunc)        goal_distance_from,
  (SelectFunc)          goal_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            goal_move,
  (MoveHandleFunc)      goal_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   goal_describe_props,
  (GetPropsFunc)        goal_get_props,
  (SetPropsFunc)        goal_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription goal_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Goal Type"),
    N_("Goal Type"),
    prop_goal_type_data},

  { "text", PROP_TYPE_TEXT, 0,NULL,NULL},
  PROP_DESC_END
};

static PropDescription *
goal_describe_props(Goal *goal)
{
  if (goal_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(goal_props);
  }
  return goal_props;
}

static PropOffset goal_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Goal,type)},
  { "text", PROP_TYPE_TEXT, offsetof(Goal,text)},
  { NULL, 0, 0 }
};

static void
goal_get_props(Goal *goal, GPtrArray *props)
{
  object_get_props_from_offsets(&goal->element.object,
                                goal_offsets,props);
}

static void
goal_set_props(Goal *goal, GPtrArray *props)
{
  object_set_props_from_offsets(&goal->element.object,
                                goal_offsets,props);
  goal_update_data(goal, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static real
goal_distance_from(Goal *goal, Point *point)
{
  Element *elem = &goal->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - GOAL_LINE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + GOAL_LINE_WIDTH/2;
  rect.top = elem->corner.y - GOAL_LINE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + GOAL_LINE_WIDTH/2;
  return distance_rectangle_point(&rect, point);
}

static void
goal_select(Goal *goal, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  text_set_cursor(goal->text, clicked_point, interactive_renderer);
  text_grab_focus(goal->text, &goal->element.object);
  element_update_handles(&goal->element);
}


static DiaObjectChange*
goal_move_handle (Goal             *goal,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  g_return_val_if_fail (goal != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  element_move_handle (&goal->element, handle->id, to, cp, reason, modifiers);

  switch (handle->id) {
    case HANDLE_RESIZE_NW:
      horiz = ANCHOR_END;
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_N:
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_NE:
      horiz = ANCHOR_START;
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_E:
      horiz = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SE:
      horiz = ANCHOR_START;
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_S:
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SW:
      horiz = ANCHOR_END;
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_W:
      horiz = ANCHOR_END;
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
      break;
  }

  goal_update_data (goal, horiz, vert);

  return NULL;
}


static DiaObjectChange*
goal_move (Goal *goal, Point *to)
{
  goal->element.corner = *to;

  goal_update_data (goal, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
compute_cloud(Goal *goal, BezPoint* bpl)
{
  Point p;
  double w,h;
  Element *elem;

  elem=&goal->element;
  p=elem->corner;
  w=elem->width;
  h=elem->height;

  bpl[0].type=BEZ_MOVE_TO;
  bpl[0].p1.x=p.x+w*0.19;
  bpl[0].p1.y=p.y;

  bpl[1].type=BEZ_CURVE_TO;
  bpl[1].p3.x=p.x+w*0.81;
  bpl[1].p3.y=p.y;
  bpl[1].p1.x=bpl[0].p1.x+w/4;
  bpl[1].p1.y=bpl[0].p1.y+h/10;
  bpl[1].p2.x=bpl[1].p3.x-w/4;
  bpl[1].p2.y=bpl[1].p3.y+h/10;

  bpl[2].type=BEZ_CURVE_TO;
  bpl[2].p3.x=p.x+w*0.81;
  bpl[2].p3.y=p.y+h;
  bpl[2].p1.x=bpl[1].p3.x+w/4;
  bpl[2].p1.y=bpl[1].p3.y-h/10;
  bpl[2].p2.x=bpl[2].p3.x+w/4;
  bpl[2].p2.y=bpl[2].p3.y+h/10;

  bpl[3].type=BEZ_CURVE_TO;
  bpl[3].p3.x=p.x+w*0.19;
  bpl[3].p3.y=p.y+h;
  bpl[3].p1.x=bpl[2].p3.x-w/4;
  bpl[3].p1.y=bpl[2].p3.y-h/10;
  bpl[3].p2.x=bpl[3].p3.x+w/4;
  bpl[3].p2.y=bpl[3].p3.y-h/10;

  bpl[4].type=BEZ_CURVE_TO;
  bpl[4].p3.x=p.x+w*0.19;
  bpl[4].p3.y=p.y;
  bpl[4].p1.x=bpl[3].p3.x-w/4;
  bpl[4].p1.y=bpl[3].p3.y+h/10;
  bpl[4].p2.x=bpl[4].p3.x-w/4;
  bpl[4].p2.y=bpl[4].p3.y-h/10;
}


/* drawing stuff */
static void
goal_draw (Goal *goal, DiaRenderer *renderer)
{
  Point p1,p2;
  BezPoint bpl[5];
  Element *elem;

  /* some asserts */
  g_return_if_fail (goal != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &goal->element;

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linewidth (renderer, GOAL_LINE_WIDTH);

  if (goal->type==GOAL) {  /* goal */
    p1.x=elem->corner.x;
    p1.y= elem->corner.y;
    p2.x=p1.x+elem->width;
    p2.y=p1.y+elem->height;
    dia_renderer_draw_rounded_rect (renderer,
                                    &p1,
                                    &p2,
                                    &GOAL_BG_COLOR,
                                    &GOAL_FG_COLOR,
                                    elem->height/2.0);
  } else {                 /* softgoal */
    compute_cloud (goal,bpl);
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
    dia_renderer_draw_beziergon (renderer, bpl, 5, &GOAL_BG_COLOR, &GOAL_FG_COLOR);
  }

  /* drawing text */
  text_draw (goal->text, renderer);
}


static void
goal_update_data (Goal *goal, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &goal->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  real w, h;
  ConnectionPoint *c;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox (goal->text, NULL);
  w = goal->text->max_width + goal->padding*2;
  h = goal->text->height * goal->text->numlines + goal->padding*2;

  /* autoscale here */
  if (w > elem->width) {
    elem->width = w;
  }

  if (h > elem->height) {
    elem->height = h;
  }

  if (elem->width < elem->height) {
    elem->width = elem->height;
  }

  /* move shape if necessary ... */
  switch (horiz) {
    case ANCHOR_MIDDLE:
      elem->corner.x = center.x - elem->width/2;
      break;
    case ANCHOR_END:
      elem->corner.x = bottom_right.x - elem->width;
      break;
    case ANCHOR_START:
    default:
      break;
  }

  switch (vert) {
    case ANCHOR_MIDDLE:
      elem->corner.y = center.y - elem->height/2; break;
    case ANCHOR_END:
      elem->corner.y = bottom_right.y - elem->height; break;
    case ANCHOR_START:
    default:
      break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - goal->text->height * goal->text->numlines / 2 +
    goal->text->ascent;
  text_set_position (goal->text, &p);

  extra->border_trans = GOAL_LINE_WIDTH;
  element_update_boundingbox (elem);

  obj->position = elem->corner;

  element_update_handles (elem);

  /* Update connections: */
  p = elem->corner;
  w = elem->width;
  h = elem->height;
  c = goal->connector;
  switch (goal->type) {
    case SOFTGOAL: update_softgoal_connectors(c,p,w,h); break;
    case GOAL:     update_goal_connectors(c,p,w,h); break;
    default:
      g_return_if_reached ();
  }
}

static void update_softgoal_connectors(ConnectionPoint *c, Point p, double w, double h) {
  /* lateral */
  c[0].pos.x=p.x;
  c[0].pos.y=p.y+h/2;
  c[0].directions = DIR_WEST;
  c[1].pos.x=p.x+w;
  c[1].pos.y=p.y+h/2;
  c[1].directions = DIR_EAST;
  /* top */
  c[2].pos.x=p.x+w/6;
  c[2].pos.y=p.y;
  c[2].directions = DIR_NORTH;
  c[3].pos.x=p.x+w/2;
  c[3].pos.y=p.y+w/20;
  c[3].directions = DIR_NORTH;
  c[4].pos.x=p.x+5*w/6;
  c[4].pos.y=p.y;
  c[4].directions = DIR_NORTH;
  /* bottom */
  c[5].pos.x=p.x+w/6;
  c[5].pos.y=p.y+h;
  c[5].directions = DIR_SOUTH;
  c[6].pos.x=p.x+w/2;
  c[6].pos.y=p.y+h-w/20;
  c[6].directions = DIR_SOUTH;
  c[7].pos.x=p.x+5*w/6;
  c[7].pos.y=p.y+h;
  c[7].directions = DIR_SOUTH;
  c[8].pos.x=p.x+w/2;
  c[8].pos.y=p.y+h/2;
  c[8].directions = DIR_ALL;
}

static void update_goal_connectors(ConnectionPoint *c, Point p, double w, double h) {
  /* lateral */
  c[0].pos.x=p.x;
  c[0].pos.y=p.y+h/2;
  c[0].directions = DIR_WEST;
  c[1].pos.x=p.x+w;
  c[1].pos.y=p.y+h/2;
  c[1].directions = DIR_EAST;
  /* top */
  c[2].pos.x=p.x+w/6;
  c[2].pos.y=p.y;
  c[2].directions = DIR_NORTH;
  c[3].pos.x=p.x+w/2;
  c[3].pos.y=p.y;
  c[3].directions = DIR_NORTH;
  c[4].pos.x=p.x+5*w/6;
  c[4].pos.y=p.y;
  c[4].directions = DIR_NORTH;
  /* bottom */
  c[5].pos.x=p.x+w/6;
  c[5].pos.y=p.y+h;
  c[5].directions = DIR_SOUTH;
  c[6].pos.x=p.x+w/2;
  c[6].pos.y=p.y+h;
  c[6].directions = DIR_SOUTH;
  c[7].pos.x=p.x+5*w/6;
  c[7].pos.y=p.y+h;
  c[7].directions = DIR_SOUTH;
  c[8].pos.x=p.x+w/2;
  c[8].pos.y=p.y+h/2;
  c[8].directions = DIR_ALL;
}

/* creating stuff */
static DiaObject *
goal_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Goal *goal;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont* font;
  int i;

  goal = g_new0 (Goal, 1);
  elem = &goal->element;
  obj = &elem->object;

  obj->type = &istar_goal_type;

  obj->ops = &goal_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  goal->padding = DEFAULT_PADDING;

  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + DEFAULT_FONT / 2;

  font = dia_font_new_from_style( DIA_FONT_SANS , DEFAULT_FONT);

  goal->text = new_text ("", font,
                         DEFAULT_FONT, &p,
                         &color_black,
                         DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  element_init(elem, 8, NUM_CONNECTIONS);

  for(i=0; i<NUM_CONNECTIONS; i++) {
    obj->connections[i] = &goal->connector[i];
    goal->connector[i].object = obj;
    goal->connector[i].connected = NULL;
  }
  goal->connector[8].flags = CP_FLAGS_MAIN;

  goal->element.extra_spacing.border_trans = GOAL_LINE_WIDTH/2.0;
  goal_update_data(goal, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

  /* handling type info */
  switch (GPOINTER_TO_INT(user_data)) {
    case 1:  goal->type=SOFTGOAL; break;
    case 2:  goal->type=GOAL; break;
    default: goal->type=SOFTGOAL; break;
  }

  return &goal->element.object;
}

static void
goal_destroy(Goal *goal)
{
  text_destroy(goal->text);
  element_destroy(&goal->element);
}


static DiaObject *
goal_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&istar_goal_type,
                                      obj_node,version,ctx);
}


