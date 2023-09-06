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

#include "pixmaps/agent.xpm"

#define DEFAULT_WIDTH 3.0
#define DEFAULT_HEIGHT 1
#define DEFAULT_BORDER 0.2
#define DEFAULT_PADDING 0.4
#define DEFAULT_FONT 0.7
#define OTHER_LINE_SIMPLE_WIDTH 0.09
#define OTHER_LINE_DOUBLE_WIDTH 0.18
#define OTHER_FG_COLOR color_black
#define OTHER_BG_COLOR color_white
#define AGENT_LEFT 0.5

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef enum {
  AGENT
} OtherType;

static PropEnumData prop_other_type_data[] = {
  { N_("Agent"), AGENT },
  { NULL, 0}
};

typedef struct _Other {
  Element element;
  ConnPointLine *north,*south,*east,*west;

  Text *text;
  double padding;
  OtherType type;

  ConnectionPoint center_cp;
} Other;


static double           other_distance_from            (Other            *other,
                                                        Point            *point);
static void             other_select                   (Other            *other,
                                                        Point            *clicked_point,
                                                        DiaRenderer      *interactive_renderer);
static DiaObjectChange* other_move_handle              (Other            *other,
                                                        Handle           *handle,
                                                        Point            *to,
                                                        ConnectionPoint  *cp,
                                                        HandleMoveReason  reason,
                                                        ModifierKeys      modifiers);
static DiaObjectChange* other_move(Other *other, Point *to);
static void other_draw(Other *other, DiaRenderer *renderer);
static void other_update_data(Other *other, AnchorShape horix, AnchorShape vert);
static DiaObject *other_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void other_destroy(Other *other);
static DiaObject *other_load(ObjectNode obj_node, int version,
                             DiaContext *cts);
static DiaMenu *other_get_object_menu(Other *other, Point *clickedpoint);

static PropDescription *other_describe_props(Other *other);
static void other_get_props(Other *other, GPtrArray *props);
static void other_set_props(Other *other, GPtrArray *props);

static ObjectTypeOps kaos_other_type_ops =
{
  (CreateFunc) other_create,
  (LoadFunc)   other_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType kaos_other_type =
{
  "KAOS - other",     /* name */
  0,               /* version */
  kaos_agent_xpm,   /* pixmap */
  &kaos_other_type_ops /* ops */
};

static ObjectOps other_ops = {
  (DestroyFunc)         other_destroy,
  (DrawFunc)            other_draw,
  (DistanceFunc)        other_distance_from,
  (SelectFunc)          other_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            other_move,
  (MoveHandleFunc)      other_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      other_get_object_menu,
  (DescribePropsFunc)   other_describe_props,
  (GetPropsFunc)        other_get_props,
  (SetPropsFunc)        other_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription other_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Type"),
    N_("Type"),
    prop_other_type_data},

  { "text", PROP_TYPE_TEXT, 0,NULL,NULL},

  { "cpl_north",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_west",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_south",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_east",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  PROP_DESC_END
};

static PropDescription *
other_describe_props(Other *other)
{
  if (other_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(other_props);
  }
  return other_props;
}

static PropOffset other_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Other,type)},
  { "text", PROP_TYPE_TEXT, offsetof(Other,text)},
  { "cpl_north",PROP_TYPE_CONNPOINT_LINE, offsetof(Other,north)},
  { "cpl_west",PROP_TYPE_CONNPOINT_LINE, offsetof(Other,west)},
  { "cpl_south",PROP_TYPE_CONNPOINT_LINE, offsetof(Other,south)},
  { "cpl_east",PROP_TYPE_CONNPOINT_LINE, offsetof(Other,east)},
  {NULL}
};

static void
other_get_props(Other *other, GPtrArray *props)
{
  object_get_props_from_offsets(&other->element.object,
                                other_offsets,props);
}

static void
other_set_props(Other *other, GPtrArray *props)
{
  object_set_props_from_offsets(&other->element.object,
                                other_offsets,props);
  other_update_data(other, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static real
other_distance_from (Other *other, Point *point)
{
  Element *elem = &other->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - OTHER_LINE_SIMPLE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + OTHER_LINE_SIMPLE_WIDTH/2;
  rect.top = elem->corner.y - OTHER_LINE_SIMPLE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + OTHER_LINE_SIMPLE_WIDTH/2;
  return distance_rectangle_point (&rect, point);
}


static void
other_select (Other       *other,
              Point       *clicked_point,
              DiaRenderer *interactive_renderer)
{
  text_set_cursor (other->text, clicked_point, interactive_renderer);
  text_grab_focus (other->text, &other->element.object);
  element_update_handles (&other->element);
}


static DiaObjectChange *
other_move_handle (Other            *other,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  g_return_val_if_fail (other != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  element_move_handle (&other->element,
                       handle->id, to, cp, reason, modifiers);

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

  other_update_data (other, horiz, vert);

  return NULL;
}


static DiaObjectChange *
other_move (Other *other, Point *to)
{
  other->element.corner = *to;

  other_update_data (other, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
compute_agent (Other *other, Point *pl)
{
  double rx,ry,w,h,h2;
  Element *elem;

  elem=&other->element;
  w=elem->width;
  h=elem->height;
  rx=elem->corner.x;
  ry=elem->corner.y;
  h2=h/2;

  pl[0].x=rx;
  pl[0].y=ry+h2;
  pl[1].x=rx+h2;
  pl[1].y=ry;
  pl[2].x=rx+w-h2;
  pl[2].y=ry;
  pl[3].x=rx+w;
  pl[3].y=ry+h2;
  pl[4].x=rx+w-h2;
  pl[4].y=ry+h;
  pl[5].x=rx+h2;
  pl[5].y=ry+h;
}


static void
draw_agent_icon (Other *other, DiaRenderer *renderer)
{
  double rx,ry,h;
  Point c,p1,p2;
  Element *elem;

  elem=&other->element;
  h=elem->height;
  rx=elem->corner.x+h/2;
  ry=elem->corner.y+3*h/10;

  /* head */
  c.x=rx;
  c.y=ry;
  dia_renderer_draw_ellipse (renderer, &c, h/5, h/5, &OTHER_FG_COLOR, NULL);

  /* body */
  p1.x=rx;
  p1.y=ry;
  p2.x=p1.x;
  p2.y=p1.y+3.5*h/10;
  dia_renderer_draw_line (renderer, &p1, &p2, &OTHER_FG_COLOR);

  /* arms */
  p1.x=rx-1.5*h/10;
  p1.y=ry+2.2*h/10;
  p2.x=rx+1.5*h/10;
  p2.y=p1.y;
  dia_renderer_draw_line (renderer, &p1, &p2, &OTHER_FG_COLOR);

  /* left leg */
  p1.x=rx;
  p1.y=ry+3.5*h/10;
  p2.x=p1.x-h/10;
  p2.y=p1.y+2*h/10;
  dia_renderer_draw_line (renderer, &p1, &p2, &OTHER_FG_COLOR);

  /* right leg */
  p1.x=rx;
  p1.y=ry+3.5*h/10;
  p2.x=p1.x+h/10;
  p2.y=p1.y+2*h/10;
  dia_renderer_draw_line (renderer, &p1, &p2, &OTHER_FG_COLOR);
}

/* drawing stuff */
static void
other_draw (Other *other, DiaRenderer *renderer)
{
  Point pl[6];

  /* some asserts */
  g_return_if_fail (other != NULL);
  g_return_if_fail (renderer != NULL);

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  if (other->type==AGENT) {
    compute_agent (other,pl);
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
    dia_renderer_set_linewidth (renderer, OTHER_LINE_SIMPLE_WIDTH);
    dia_renderer_draw_polygon (renderer,
                               pl,
                               6,
                               &OTHER_BG_COLOR,
                               &OTHER_FG_COLOR);

    draw_agent_icon (other,renderer);
  }

  /* drawing text */
  text_draw (other->text, renderer);
}


static void
other_update_data (Other *other, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &other->element;
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

  text_calc_boundingbox (other->text, NULL);
  width = other->text->max_width + other->padding*2;
  /* reserve some place for agent icon */
  if (other->type==AGENT) {
    width += AGENT_LEFT;
  }
  height = other->text->height * other->text->numlines + other->padding*2;

  /* autoscale here */
  if (width > elem->width) {
    elem->width = width;
  }

  if (height > elem->height) {
    elem->height = height;
  }

  /* some personal touch for agent */
  if ((other->type==AGENT) && (elem->width<elem->height)) {
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
  if (other->type == AGENT) {
    p.x += (AGENT_LEFT+elem->width) / 2.0;
  } else {
    p.x += elem->width / 2.0;
  }

  p.y += elem->height / 2.0 - other->text->height * other->text->numlines / 2 +
    other->text->ascent;
  text_set_position (other->text, &p);

  extra->border_trans = OTHER_LINE_DOUBLE_WIDTH / 2.0;
  element_update_boundingbox (elem);

  obj->position = elem->corner;

  element_update_handles (elem);

  /* Update connections: */
  nw = elem->corner;
  se = bottom_right;
  ne.x = se.x;
  ne.y = nw.y;
  sw.y = se.y;
  sw.x = nw.x;

  connpointline_update (other->north);
  connpointline_putonaline(other->north, &ne, &nw, DIR_NORTH);
  connpointline_update (other->west);
  connpointline_putonaline (other->west, &nw, &sw, DIR_WEST);
  connpointline_update (other->south);
  connpointline_putonaline (other->south, &sw, &se, DIR_SOUTH);
  connpointline_update (other->east);
  connpointline_putonaline (other->east, &se, &ne, DIR_EAST);

  other->center_cp.pos.x = (nw.x + se.x) / 2;
  other->center_cp.pos.y = (nw.y + se.y) / 2;
}


static ConnPointLine *
other_get_clicked_border (Other *other, Point *clicked)
{
  ConnPointLine *cpl;
  real dist,dist2;

  cpl = other->north;
  dist = distance_line_point (&other->north->start,
                              &other->north->end,
                              0,
                              clicked);

  dist2 = distance_line_point (&other->west->start,
                               &other->west->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = other->west;
    dist = dist2;
  }

  dist2 = distance_line_point (&other->south->start,
                               &other->south->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = other->south;
    dist = dist2;
  }

  dist2 = distance_line_point (&other->east->start,
                               &other->east->end,
                               0,
                               clicked);

  if (dist2 < dist) {
    cpl = other->east;
    /*dist = dist2;*/
  }

  return cpl;
}


inline static DiaObjectChange *
other_create_change (Other           *other,
                     DiaObjectChange *inner,
                     ConnPointLine   *cpl)
{
  return (DiaObjectChange *) inner;
}


static DiaObjectChange *
other_add_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Other *other = (Other *) obj;

  cpl = other_get_clicked_border (other,clicked);
  change = connpointline_add_point (cpl, clicked);
  other_update_data ((Other *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return other_create_change (other, change, cpl);
}


static DiaObjectChange *
other_remove_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Other *other = (Other *) obj;

  cpl = other_get_clicked_border (other, clicked);
  change = connpointline_remove_point (cpl, clicked);
  other_update_data((Other *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
  return other_create_change (other, change, cpl);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), other_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), other_remove_connpoint_callback,
    NULL, 1 },
};

static DiaMenu object_menu = {
  N_("KAOS other"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
other_get_object_menu(Other *other, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = other_get_clicked_border(other,clickedpoint);
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = connpointline_can_add_point(cpl, clickedpoint);
  object_menu_items[1].active = connpointline_can_remove_point(cpl, clickedpoint);
  return &object_menu;
}

/* creating stuff */
static DiaObject *
other_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Other *other;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont* font;

  other = g_new0 (Other, 1);
  elem = &other->element;
  obj = &elem->object;

  obj->type = &kaos_other_type;

  obj->ops = &other_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  other->padding = DEFAULT_PADDING;

  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + DEFAULT_FONT / 2;

  font = dia_font_new_from_style( DIA_FONT_SANS , DEFAULT_FONT);

  other->text = new_text ("",
                          font,
                          DEFAULT_FONT,
                          &p,
                          &color_black,
                          DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  element_init(elem, 8, 1);

  other->north = connpointline_create(obj,3);
  other->west = connpointline_create(obj,1);
  other->south = connpointline_create(obj,3);
  other->east = connpointline_create(obj,1);

  obj->connections[0] = &other->center_cp;
  other->center_cp.object = obj;
  other->center_cp.connected = NULL;
  other->center_cp.flags = CP_FLAGS_MAIN;

  other->element.extra_spacing.border_trans = OTHER_LINE_SIMPLE_WIDTH/2.0;
  other_update_data(other, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

  /* handling type info */
  switch (GPOINTER_TO_INT(user_data)) {
    case 1:  other->type=AGENT; break;
    default: other->type=AGENT; break;
  }

  return &other->element.object;
}

static void
other_destroy(Other *other)
{
  text_destroy(other->text);

  connpointline_destroy(other->east);
  connpointline_destroy(other->south);
  connpointline_destroy(other->west);
  connpointline_destroy(other->north);

  element_destroy(&other->element);
}


static DiaObject *
other_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&kaos_other_type,
                                      obj_node,version,ctx);
}


