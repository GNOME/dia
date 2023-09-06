/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Jackson diagram -  adapted by Christophe Ponsard
 * This class captures all kind of domains (given, designed, machine)
 * both for generic problems and for problem frames (ie. with domain kinds)
 *
 * based on SADT diagrams copyright (C) 2000, 2001 Cyrille Chepelov
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

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/requirement.xpm"

typedef struct _Requirement Requirement;
typedef struct _RequirementState RequirementState;

#define NUM_CONNECTIONS 9

struct _Requirement {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;
};


#define REQ_FONT 0.7
#define REQ_WIDTH 3.25
#define REQ_HEIGHT 2
#define REQ_MIN_RATIO 1.5
#define REQ_MAX_RATIO 3
#define REQ_LINEWIDTH 0.09
#define REQ_MARGIN_Y 0.3
#define REQ_DASHLEN 0.5

static real req_distance_from(Requirement *req, Point *point);
static void req_select(Requirement *req, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static DiaObjectChange* req_move_handle(Requirement *req, Handle *handle,
				Point *to, ConnectionPoint *cp,
				HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* req_move(Requirement *req, Point *to);
static void req_draw(Requirement *req, DiaRenderer *renderer);
static DiaObject *req_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void req_destroy(Requirement *req);
static DiaObject *req_load(ObjectNode obj_node, int version,DiaContext *ctx);
static void req_update_data(Requirement *req);
static PropDescription *req_describe_props(Requirement *req);
static void req_get_props(Requirement *req, GPtrArray *props);
static void req_set_props(Requirement *req, GPtrArray *props);

static ObjectTypeOps req_type_ops =
{
  (CreateFunc) req_create,
  (LoadFunc)   req_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType jackson_requirement_type =
{
  "Jackson - requirement",   /* name */
  0,                      /* version */
  jackson_requirement_xpm, /* pixmap */
  &req_type_ops               /* ops */
};

static ObjectOps req_ops = {
  (DestroyFunc)         req_destroy,
  (DrawFunc)            req_draw,
  (DistanceFunc)        req_distance_from,
  (SelectFunc)          req_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            req_move,
  (MoveHandleFunc)      req_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   req_describe_props,
  (GetPropsFunc)        req_get_props,
  (SetPropsFunc)        req_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription req_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },

  PROP_DESC_END
};

static PropDescription *
req_describe_props(Requirement *req)
{
  return req_props;
}

static PropOffset req_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Requirement,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Requirement,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Requirement,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Requirement,text),offsetof(Text,color)},
  { NULL, 0, 0 },
};

static void
req_get_props(Requirement * req, GPtrArray *props)
{
  object_get_props_from_offsets(&req->element.object,
                                req_offsets,props);
}

static void
req_set_props(Requirement *req, GPtrArray *props)
{
  object_set_props_from_offsets(&req->element.object,
                                req_offsets,props);
  req_update_data(req);
}

static real
req_distance_from(Requirement *req, Point *point)
{
  Element *elem = &req->element;
  Point center;

  center.x = elem->corner.x+elem->width/2;
  center.y = elem->corner.y+elem->height/2;

  return distance_ellipse_point(&center, elem->width, elem->height,
				REQ_LINEWIDTH, point);
}

static void
req_select(Requirement *req, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(req->text, clicked_point, interactive_renderer);
  text_grab_focus(req->text, &req->element.object);
  element_update_handles(&req->element);
}


static DiaObjectChange *
req_move_handle (Requirement      *req,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  g_return_val_if_fail (req != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
req_move(Requirement *req, Point *to)
{
  real h;
  Point p;

  req->element.corner = *to;
  h = req->text->height*req->text->numlines;

  p = *to;
  p.x += req->element.width/2.0;
  p.y += (req->element.height - h)/2.0 + req->text->ascent;

  text_set_position(req->text, &p);
  req_update_data(req);
  return NULL;
}

/** draw is here */
static void
req_draw (Requirement *req, DiaRenderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point c;

  g_return_if_fail (req != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &req->element;

  x = elem->corner.x;
  y = elem->corner.y;

  w = elem->width;
  h = elem->height;
  c.x = x + w/2.0;
  c.y = y + h/2.0;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, REQ_LINEWIDTH);

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, REQ_DASHLEN);

  dia_renderer_draw_ellipse (renderer, &c, w, h, &color_white, &color_black);

  text_draw (req->text, renderer);
}


static void
req_update_data(Requirement *req)
{
  real w, h, ratio;
  Point c, half, r,p;

  Element *elem = &req->element;
  DiaObject *obj = &elem->object;

  text_calc_boundingbox(req->text, NULL);
  w = req->text->max_width;
  h = req->text->height*req->text->numlines;

  ratio = w/h;

  if (ratio > REQ_MAX_RATIO)
    ratio = REQ_MAX_RATIO;

  if (ratio < REQ_MIN_RATIO) {
    ratio = REQ_MIN_RATIO;
    r.y = w / ratio + h;
    r.x = r.y * ratio;
  } else {
    r.x = ratio*h + w;
    r.y = r.x / ratio;
  }
  if (r.x < REQ_WIDTH)
    r.x = REQ_WIDTH;
  if (r.y < REQ_HEIGHT)
    r.y = REQ_HEIGHT;

  elem->width = r.x;
  elem->height = r.y;

  r.x /= 2.0;
  r.y /= 2.0;
  c.x = elem->corner.x + elem->width / 2.0;
  c.y = elem->corner.y + r.y;
  half.x = r.x * M_SQRT1_2;
  half.y = r.y * M_SQRT1_2;

  /* Update connections: */
  req->connections[0].pos.x = c.x - half.x;
  req->connections[0].pos.y = c.y - half.y;
  req->connections[1].pos.x = c.x;
  req->connections[1].pos.y = elem->corner.y;
  req->connections[2].pos.x = c.x + half.x;
  req->connections[2].pos.y = c.y - half.y;
  req->connections[3].pos.x = c.x - r.x;
  req->connections[3].pos.y = c.y;
  req->connections[4].pos.x = c.x + r.x;
  req->connections[4].pos.y = c.y;

  req->connections[5].pos.x = c.x - half.x;
  req->connections[5].pos.y = c.y + half.y;
  req->connections[6].pos.x = c.x;
  req->connections[6].pos.y = elem->corner.y + elem->height;
  req->connections[7].pos.x = c.x + half.x;
  req->connections[7].pos.y = c.y + half.y;

  req->connections[8].pos.x = elem->corner.x + elem->width/2;
  req->connections[8].pos.y = elem->corner.y + elem->height/2;

  h = req->text->height*req->text->numlines;
  p = req->element.corner;
  p.x += req->element.width/2.0;
  p.y += (req->element.height - h)/2.0 + req->text->ascent;
  text_set_position(req->text, &p);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

  /* Boundingbox calculation including the line width */
  {
    DiaRectangle bbox;

    ellipse_bbox (&c, elem->width, elem->height, &elem->extra_spacing, &bbox);
    rectangle_union(&obj->bounding_box, &bbox);
  }
}

/** creation here */
static DiaObject *
req_create(Point *startpoint,
        void *user_data,
        Handle **handle1,
        Handle **handle2)
{
  Requirement *req;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  req = g_new0 (Requirement, 1);
  elem = &req->element;
  obj = &elem->object;

  obj->type = &jackson_requirement_type;
  obj->ops = &req_ops;

  elem->corner = *startpoint;
  elem->width = REQ_WIDTH;
  elem->height = REQ_HEIGHT;

  font = dia_font_new_from_style(DIA_FONT_SANS, REQ_FONT);
  p = *startpoint;
  p.x += REQ_WIDTH/2.0;
  p.y += REQ_HEIGHT/2.0;

  req->text = new_text ("", font, REQ_FONT, &p, &color_black, DIA_ALIGN_CENTRE);
  g_clear_object (&font);
  element_init (elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &req->connections[i];
    req->connections[i].object = obj;
    req->connections[i].connected = NULL;
    req->connections[i].flags = 0;
    req->connections[i].directions = 0;
    if (i < 3)
      req->connections[i].directions |= DIR_NORTH;
    else if (i > 4)
      req->connections[i].directions |= DIR_SOUTH;
    if (i == 0 || i == 3 || i == 5)
      req->connections[i].directions |= DIR_WEST;
    else if (i == 2 || i == 4 || i == 7)
      req->connections[i].directions |= DIR_EAST;
  }
  req->connections[8].flags = CP_FLAGS_MAIN;
  req->connections[8].directions |= DIR_ALL;
  elem->extra_spacing.border_trans = REQ_LINEWIDTH / 2.0;
  req_update_data(req);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  return &req->element.object;
}

static void
req_destroy(Requirement *req)
{
  text_destroy(req->text);

  element_destroy(&req->element);
}

static DiaObject *
req_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&jackson_requirement_type,
                                      obj_node,version,ctx);
}



