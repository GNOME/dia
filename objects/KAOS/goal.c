/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Objects for drawing KAOS goal diagrams.
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

#include "pixmaps/goal.xpm"

#define DEFAULT_WIDTH 3.0
#define DEFAULT_HEIGHT 1
#define DEFAULT_BORDER 0.2
#define DEFAULT_PADDING 0.4
#define DEFAULT_FONT 0.7
#define GOAL_LINE_SIMPLE_WIDTH 0.09
#define GOAL_LINE_DOUBLE_WIDTH 0.18
#define GOAL_FG_COLOR color_black
#define GOAL_BG_COLOR color_white
#define GOAL_OFFSET 0.5


typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;


typedef enum {
  SOFTGOAL,
  GOAL,
  REQUIREMENT,
  ASSUMPTION,
  OBSTACLE
} GoalType;


static PropEnumData prop_goal_type_data[] = {
  { N_("Softgoal"), SOFTGOAL },
  { N_("Goal"), GOAL },
  { N_("Requirement"), REQUIREMENT },
  { N_("Assumption"), ASSUMPTION },
  { N_("Obstacle"), OBSTACLE },
  { NULL, 0}
};


typedef struct _Goal {
  Element element;
  ConnPointLine *north,*south,*east,*west;

  Text *text;
  real padding;
  GoalType type;
} Goal;


static double           goal_distance_from             (Goal             *goal,
                                                        Point            *point);
static void             goal_select                    (Goal             *goal,
                                                        Point            *clicked_point,
                                                        DiaRenderer      *interactive_renderer);
static DiaObjectChange *goal_move_handle               (Goal             *goal,
                                                        Handle           *handle,
                                                        Point            *to,
                                                        ConnectionPoint  *cp,
                                                        HandleMoveReason  reason,
                                                        ModifierKeys      modifiers);
static DiaObjectChange *goal_move                      (Goal             *goal,
                                                        Point            *to);
static void goal_draw(Goal *goal, DiaRenderer *renderer);
static void goal_update_data(Goal *goal, AnchorShape horix, AnchorShape vert);
static DiaObject *goal_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void goal_destroy(Goal *goal);
static DiaObject *goal_load(ObjectNode obj_node, int version,
                            DiaContext *ctx);
static DiaMenu *goal_get_object_menu(Goal *goal, Point *clickedpoint);

static PropDescription *goal_describe_props(Goal *goal);
static void goal_get_props(Goal *goal, GPtrArray *props);
static void goal_set_props(Goal *goal, GPtrArray *props);


static ObjectTypeOps kaos_goal_type_ops =
{
  (CreateFunc) goal_create,
  (LoadFunc)   goal_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};


DiaObjectType kaos_goal_type =
{
  "KAOS - goal",     /* name */
  0,              /* version */
  kaos_goal_xpm,   /* pixmap */
  &kaos_goal_type_ops, /* ops */

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
  (ObjectMenuFunc)      goal_get_object_menu,
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

  { "cpl_north", PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_west", PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_south", PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_east", PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  PROP_DESC_END
};


static PropDescription *
goal_describe_props (Goal *goal)
{
  if (goal_props[0].quark == 0) {
    prop_desc_list_calculate_quarks (goal_props);
  }
  return goal_props;
}


static PropOffset goal_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof (Goal, type)},
  { "text", PROP_TYPE_TEXT, offsetof (Goal, text)},
  { "cpl_north", PROP_TYPE_CONNPOINT_LINE, offsetof (Goal, north)},
  { "cpl_west", PROP_TYPE_CONNPOINT_LINE, offsetof (Goal, west)},
  { "cpl_south", PROP_TYPE_CONNPOINT_LINE, offsetof (Goal, south)},
  { "cpl_east", PROP_TYPE_CONNPOINT_LINE, offsetof (Goal, east)},
  {NULL}
};


static void
goal_get_props (Goal *goal, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (goal),
                                 goal_offsets, props);
}


static void
goal_set_props (Goal *goal, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (goal),
                                 goal_offsets, props);
  goal_update_data (goal, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static real
goal_distance_from (Goal *goal, Point *point)
{
  Element *elem = &goal->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - GOAL_LINE_SIMPLE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + GOAL_LINE_SIMPLE_WIDTH/2;
  rect.top = elem->corner.y - GOAL_LINE_SIMPLE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + GOAL_LINE_SIMPLE_WIDTH/2;
  return distance_rectangle_point (&rect, point);
}


static void
goal_select (Goal        *goal,
             Point       *clicked_point,
             DiaRenderer *interactive_renderer)
{
  text_set_cursor (goal->text, clicked_point, interactive_renderer);
  text_grab_focus (goal->text, &goal->element.object);
  element_update_handles (&goal->element);
}


static DiaObjectChange *
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

  element_move_handle (&goal->element,
                       handle->id,
                       to,
                       cp,
                       reason,
                       modifiers);

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


/* auxialliary computations */
static void
compute_cloud (Goal *goal, BezPoint* bpl) {
  double wd,hd;
  Element *elem;

  elem=&goal->element;
  wd=elem->width/4.0;
  hd=elem->height/4.0;

  bpl[0].type=BEZ_MOVE_TO;
  bpl[0].p1.x=elem->corner.x+wd/2.0;
  bpl[0].p1.y=elem->corner.y+hd;

  bpl[1].type=BEZ_CURVE_TO;
  bpl[1].p3.x=bpl[0].p1.x+wd;
  bpl[1].p3.y=bpl[0].p1.y+2*hd/5.0;
  bpl[1].p1.x=bpl[0].p1.x;
  bpl[1].p1.y=bpl[0].p1.y-hd*1.6;
  bpl[1].p2.x=bpl[1].p3.x;
  bpl[1].p2.y=bpl[1].p3.y-hd*1.6;

  bpl[2].type=BEZ_CURVE_TO;
  bpl[2].p3.x=bpl[1].p3.x+wd;
  bpl[2].p3.y=bpl[0].p1.y-hd/5.0;
  bpl[2].p1.x=bpl[1].p3.x;
  bpl[2].p1.y=bpl[1].p3.y-hd*1.45;
  bpl[2].p2.x=bpl[2].p3.x;
  bpl[2].p2.y=bpl[2].p3.y-hd*1.45;

  bpl[3].type=BEZ_CURVE_TO;
  bpl[3].p3.x=bpl[2].p3.x+wd;
  bpl[3].p3.y=bpl[0].p1.y+2*hd/5.0;
  bpl[3].p1.x=bpl[2].p3.x;
  bpl[3].p1.y=bpl[2].p3.y-hd*1.45;
  bpl[3].p2.x=bpl[3].p3.x+wd/2.0;
  bpl[3].p2.y=bpl[3].p3.y-hd*1.45;

  /* bottom of cloud starts here */
  bpl[4].type=BEZ_CURVE_TO;
  bpl[4].p3.x=bpl[3].p3.x;
  bpl[4].p3.y=bpl[0].p1.y+2*hd;
  bpl[4].p1.x=bpl[3].p3.x+wd/1.5;
  bpl[4].p1.y=bpl[3].p3.y;
  bpl[4].p2.x=bpl[4].p3.x+wd/1.5;
  bpl[4].p2.y=bpl[4].p3.y;

  bpl[5].type=BEZ_CURVE_TO;
  bpl[5].p3.x=bpl[4].p3.x-wd-wd/5.0;
  bpl[5].p3.y=bpl[4].p3.y+wd/20.0;
  bpl[5].p1.x=bpl[4].p3.x+wd/2.0;
  bpl[5].p1.y=bpl[4].p3.y+hd*1.3;
  bpl[5].p2.x=bpl[5].p3.x-wd/20.0;
  bpl[5].p2.y=bpl[5].p3.y+hd*1.3;

  bpl[6].type=BEZ_CURVE_TO;
  bpl[6].p3.x=bpl[5].p3.x-wd;
  bpl[6].p3.y=bpl[4].p3.y+wd/10.0;
  bpl[6].p1.x=bpl[5].p3.x;
  bpl[6].p1.y=bpl[5].p3.y+hd*1.3;
  bpl[6].p2.x=bpl[6].p3.x;
  bpl[6].p2.y=bpl[6].p3.y+hd*1.3;

  bpl[7].type=BEZ_CURVE_TO;
  bpl[7].p3.x=bpl[6].p3.x-wd+wd/10.0;
  bpl[7].p3.y=bpl[4].p3.y-wd/5.0;
  bpl[7].p1.x=bpl[6].p3.x;
  bpl[7].p1.y=bpl[6].p3.y+hd*1.45;
  bpl[7].p2.x=bpl[7].p3.x;
  bpl[7].p2.y=bpl[7].p3.y+hd*1.45;

  bpl[8].type=BEZ_CURVE_TO;
  bpl[8].p3.x=bpl[0].p1.x;
  bpl[8].p3.y=bpl[0].p1.y;
  bpl[8].p1.x=bpl[7].p3.x-wd/1.6;
  bpl[8].p1.y=bpl[7].p3.y;
  bpl[8].p2.x=bpl[0].p1.x-wd/1.6;
  bpl[8].p2.y=bpl[0].p1.y;
}


/* drawing stuff */
static void
goal_draw (Goal *goal, DiaRenderer *renderer)
{
  double dx,ix;
  Point pl[4],p1,p2;
  BezPoint bpl[9];
  Element *elem;

  /* some asserts */
  g_return_if_fail (goal != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &goal->element;

  /* drawing polygon */
  switch (goal->type) {
    case REQUIREMENT:
    case GOAL:
    case ASSUMPTION:
      pl[0].x=elem->corner.x+ GOAL_OFFSET;
      pl[0].y= elem->corner.y;
      pl[1].x=elem->corner.x + elem->width;
      pl[1].y= elem->corner.y;
      pl[2].x=elem->corner.x + elem->width - GOAL_OFFSET;
      pl[2].y= elem->corner.y + elem->height;
      pl[3].x=elem->corner.x;
      pl[3].y= elem->corner.y + elem->height;
      break;
    case OBSTACLE:
      pl[0].x=elem->corner.x;
      pl[0].y= elem->corner.y;
      pl[1].x=elem->corner.x + elem->width - GOAL_OFFSET;
      pl[1].y= elem->corner.y;
      pl[2].x=elem->corner.x + elem->width;
      pl[2].y= elem->corner.y + elem->height;
      pl[3].x=elem->corner.x + GOAL_OFFSET;
      pl[3].y= elem->corner.y + elem->height;
      break;
    // TODO: Should this be handled?
    case SOFTGOAL:
    default:
      break;
  }

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  if (goal->type != SOFTGOAL) {
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

    if ((goal->type == REQUIREMENT) || (goal->type == ASSUMPTION)) {
      dia_renderer_set_linewidth (renderer, GOAL_LINE_DOUBLE_WIDTH);
    } else {
      dia_renderer_set_linewidth (renderer, GOAL_LINE_SIMPLE_WIDTH);
    }

    dia_renderer_draw_polygon (renderer, pl, 4, &GOAL_BG_COLOR, &GOAL_FG_COLOR);

    /* adding decoration for assumption */
    if (goal->type == ASSUMPTION) {
      dx=GOAL_OFFSET+elem->height/10;
      if (dx + GOAL_OFFSET > elem->height) {
        dx=elem->height-GOAL_OFFSET; /* min size */
      }
      p1.x=elem->corner.x+ GOAL_OFFSET + dx;
      p1.y=elem->corner.y;
      ix=GOAL_OFFSET*(GOAL_OFFSET+dx-elem->height)/(GOAL_OFFSET-elem->height);
      p2.x=elem->corner.x+ix;
      p2.y=elem->corner.y+GOAL_OFFSET+dx-ix;
      dia_renderer_draw_line (renderer, &p1, &p2, &GOAL_FG_COLOR);
    }
  } else { /* SOFTGOAL IS HERE */
    compute_cloud (goal,bpl);
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
    dia_renderer_draw_beziergon (renderer, bpl, 9, &GOAL_BG_COLOR, &GOAL_FG_COLOR);
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
  real width, height;
  Point nw,ne,se,sw;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox (goal->text, NULL);
  width = goal->text->max_width + goal->padding*2;
  height = goal->text->height * goal->text->numlines + goal->padding*2;

  if (width < 2 * GOAL_OFFSET) width = 2 * GOAL_OFFSET;
  if (width > elem->width) elem->width = width;
  if (height > elem->height) elem->height = height;

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
      elem->corner.y = center.y - elem->height/2;
      break;
    case ANCHOR_END:
      elem->corner.y = bottom_right.y - elem->height;
      break;
    case ANCHOR_START:
    default:
      break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - goal->text->height * goal->text->numlines / 2 +
    goal->text->ascent;
  text_set_position (goal->text, &p);

  extra->border_trans = GOAL_LINE_DOUBLE_WIDTH / 2.0;
  element_update_boundingbox (elem);

  obj->position = elem->corner;

  element_update_handles (elem);

  /* Update connections: */
  nw = elem->corner;
  se.x = nw.x+elem->width;
  se.y = nw.y+elem->height;
  ne.x = se.x;
  ne.y = nw.y;
  sw.y = se.y;
  sw.x = nw.x;

  connpointline_update (goal->north);
  connpointline_putonaline (goal->north, &ne, &nw, DIR_NORTH);
  connpointline_update (goal->west);
  connpointline_putonaline (goal->west, &nw, &sw, DIR_SOUTH);
  connpointline_update (goal->south);
  connpointline_putonaline (goal->south, &sw, &se, DIR_SOUTH);
  connpointline_update (goal->east);
  connpointline_putonaline (goal->east, &se, &ne, DIR_EAST);
}


static ConnPointLine *
goal_get_clicked_border (Goal *goal, Point *clicked)
{
  ConnPointLine *cpl;
  real dist,dist2;

  cpl = goal->north;
  dist = distance_line_point (&goal->north->start,
                              &goal->north->end,
                              0,
                              clicked);

  dist2 = distance_line_point (&goal->west->start,
                               &goal->west->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = goal->west;
    dist = dist2;
  }

  dist2 = distance_line_point (&goal->south->start,
                               &goal->south->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = goal->south;
    dist = dist2;
  }

  dist2 = distance_line_point (&goal->east->start,
                               &goal->east->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = goal->east;
    /*dist = dist2;*/
  }

  return cpl;
}


inline static DiaObjectChange *
goal_create_change (Goal *goal, DiaObjectChange *inner, ConnPointLine *cpl)
{
  return (DiaObjectChange *) inner;
}


static DiaObjectChange *
goal_add_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Goal *goal = (Goal *)obj;

  cpl = goal_get_clicked_border(goal,clicked);
  change = connpointline_add_point(cpl, clicked);
  goal_update_data((Goal *)obj,ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return goal_create_change(goal,change,cpl);
}


static DiaObjectChange *
goal_remove_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Goal *goal = (Goal *)obj;

  cpl = goal_get_clicked_border(goal,clicked);
  change = connpointline_remove_point(cpl, clicked);
  goal_update_data((Goal *)obj,ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return goal_create_change(goal,change,cpl);
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), goal_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), goal_remove_connpoint_callback,
    NULL, 1 },
};

static DiaMenu object_menu = {
  N_("KAOS goal"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
goal_get_object_menu(Goal *goal, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = goal_get_clicked_border(goal,clickedpoint);
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = connpointline_can_add_point(cpl, clickedpoint);
  object_menu_items[1].active = connpointline_can_remove_point(cpl, clickedpoint);
  return &object_menu;
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

  goal = g_new0 (Goal, 1);
  elem = &goal->element;
  obj = &elem->object;

  obj->type = &kaos_goal_type;

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

  element_init(elem, 8, 0);

  goal->north = connpointline_create(obj,3);
  goal->west = connpointline_create(obj,0);
  goal->south = connpointline_create(obj,3);
  goal->east = connpointline_create(obj,0);

  goal->element.extra_spacing.border_trans = GOAL_LINE_SIMPLE_WIDTH/2.0;
  goal_update_data(goal, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

  /* handling type info */
  switch (GPOINTER_TO_INT(user_data)) {
    case 1:  goal->type=GOAL; break;
    case 2:  goal->type=SOFTGOAL; break;
    case 3:  goal->type=REQUIREMENT; break;
    case 4:  goal->type=ASSUMPTION; break;
    case 5:  goal->type=OBSTACLE; break;
    default: goal->type=GOAL; break;
  }

  return &goal->element.object;
}

static void
goal_destroy(Goal *goal)
{
  text_destroy(goal->text);

  connpointline_destroy(goal->east);
  connpointline_destroy(goal->south);
  connpointline_destroy(goal->west);
  connpointline_destroy(goal->north);

  element_destroy(&goal->element);
}


static DiaObject *
goal_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&kaos_goal_type,
                                      obj_node,version,ctx);
}


