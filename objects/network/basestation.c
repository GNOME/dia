/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

/* Copyright 2003, W. Borgert <debacle@debian.org>
   copied a lot from UML/actor.c */

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

#include "pixmaps/basestation.xpm"

#define NUM_CONNECTIONS 9

typedef struct _Basestation Basestation;
struct _Basestation {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Color line_colour;
  Color fill_colour;

  Text *text;

  int sectors;			/* oftenly 3 or 4, always >= 1, but
				   check is missing */
};

#define BASESTATION_WIDTH 0.8
#define BASESTATION_HEIGHT 4.0
#define BASESTATION_LINEWIDTH 0.1

static real basestation_distance_from(Basestation *basestation,
                                      Point *point);
static void basestation_select(Basestation *basestation,
                               Point *clicked_point,
                               DiaRenderer *interactive_renderer);
static DiaObjectChange *basestation_move_handle    (Basestation      *basestation,
                                                    Handle           *handle,
                                                    Point            *to,
                                                    ConnectionPoint  *cp,
                                                    HandleMoveReason  reason,
                                                    ModifierKeys      modifiers);
static DiaObjectChange *basestation_move           (Basestation      *basestation,
                                                    Point            *to);
static void basestation_draw(Basestation *basestation,
                             DiaRenderer *renderer);
static DiaObject *basestation_create(Point *startpoint,
                                  void *user_data,
                                  Handle **handle1,
                                  Handle **handle2);
static void basestation_destroy(Basestation *basestation);
static DiaObject *basestation_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription
    *basestation_describe_props(Basestation *basestation);
static void basestation_get_props(Basestation *basestation,
                                  GPtrArray *props);
static void basestation_set_props(Basestation *basestation,
                                  GPtrArray *props);
static void basestation_update_data(Basestation *basestation);

static ObjectTypeOps basestation_type_ops =
  {
    (CreateFunc) basestation_create,
    (LoadFunc)   basestation_load,
    (SaveFunc)   object_save_using_properties,
    (GetDefaultsFunc)   NULL,
    (ApplyDefaultsFunc) NULL
  };

DiaObjectType basestation_type =
  {
    "Network - Base Station",   /* name */
    0,                          /* version */
    (const char **) basestation_xpm,  /* pixmap */

    &basestation_type_ops       /* ops */
  };

static ObjectOps basestation_ops = {
  (DestroyFunc)         basestation_destroy,
  (DrawFunc)            basestation_draw,
  (DistanceFunc)        basestation_distance_from,
  (SelectFunc)          basestation_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            basestation_move,
  (MoveHandleFunc)      basestation_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   basestation_describe_props,
  (GetPropsFunc)        basestation_get_props,
  (SetPropsFunc)        basestation_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription basestation_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT_ALIGNMENT,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  { "sectors", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
    N_("Sectors"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
basestation_describe_props(Basestation *basestation)
{
  if (basestation_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(basestation_props);
  }
  return basestation_props;
}

static PropOffset basestation_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"line_colour", PROP_TYPE_COLOUR, offsetof(Basestation, line_colour)},
  {"fill_colour", PROP_TYPE_COLOUR, offsetof(Basestation, fill_colour)},
  {"text", PROP_TYPE_TEXT, offsetof(Basestation, text)},
  {"text_font", PROP_TYPE_FONT, offsetof(Basestation,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Basestation,text),offsetof(Text,height)},
  {"text_colour", PROP_TYPE_COLOUR, offsetof(Basestation,text),offsetof(Text,color)},
  {"text_alignment", PROP_TYPE_ENUM,
   offsetof(Basestation,text),offsetof(Text,alignment)},
  {"sectors", PROP_TYPE_INT, offsetof(Basestation, sectors)},
  { NULL, 0, 0 },
};

static void
basestation_get_props(Basestation * basestation, GPtrArray *props)
{
  object_get_props_from_offsets(&basestation->element.object,
                                basestation_offsets,props);
}

static void
basestation_set_props(Basestation *basestation, GPtrArray *props)
{
  object_set_props_from_offsets(&basestation->element.object,
                                basestation_offsets, props);
  basestation_update_data(basestation);
}

static real
basestation_distance_from(Basestation *basestation, Point *point)
{
  DiaObject *obj = &basestation->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
basestation_select(Basestation *basestation, Point *clicked_point,
                   DiaRenderer *interactive_renderer)
{
  text_set_cursor(basestation->text, clicked_point, interactive_renderer);
  text_grab_focus(basestation->text, &basestation->element.object);
  element_update_handles(&basestation->element);
}


static DiaObjectChange *
basestation_move_handle (Basestation      *basestation,
                         Handle           *handle,
                         Point            *to,
                         ConnectionPoint  *cp,
                         HandleMoveReason  reason,
                         ModifierKeys      modifiers)
{
  DiaObjectChange *oc;

  g_return_val_if_fail (basestation != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);
  g_return_val_if_fail (handle->id < 8, NULL);

  if (handle->type == HANDLE_NON_MOVABLE)
    return NULL;

  oc = element_move_handle (&(basestation->element), handle->id, to, cp, reason, modifiers);

  return oc;
}


static DiaObjectChange*
basestation_move (Basestation *basestation, Point *to)
{
  Element *elem = &basestation->element;

  elem->corner = *to;
  elem->corner.x -= elem->width/2.0;
  elem->corner.y -= elem->height / 2.0;

  basestation_update_data(basestation);

  return NULL;
}

static void
basestation_draw (Basestation *basestation, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  double r = BASESTATION_WIDTH / 2.0;
  Point ct, cb, p1, p2;
  Point points[6];

  g_return_if_fail (basestation != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &basestation->element;

  x = elem->corner.x;
  y = elem->corner.y + r;
  w = elem->width;
  h = elem->height - r;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_ROUND);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_set_linewidth (renderer, BASESTATION_LINEWIDTH);

  ct.x = x + w/2.0;
  ct.y = y + r/2.0;
  cb.x = ct.x;
  cb.y = ct.y + h - r;

  /* antenna 1 */
  points[0] = ct; points[0].x -= r/4.0; points[0].y -= 3.0*r/4.0;
  points[1] = ct; points[1].x += r/4.0; points[1].y -= 3.0*r/4.0;
  points[2] = ct; points[2].x += r/4.0; points[2].y += 1.0;
  points[3] = ct; points[3].x -= r/4.0; points[3].y += 1.0;
  dia_renderer_draw_polygon (renderer,
                             points,
                             4,
                             &basestation->fill_colour,
                             &basestation->line_colour);
  /* bottom */
  dia_renderer_draw_ellipse (renderer,
                             &cb,
                             r,
                             r/2.0,
                             &basestation->fill_colour,
                             NULL);
  dia_renderer_draw_arc (renderer,
                         &cb,
                         r,
                         r/2.0,
                         180,
                         0,
                         &basestation->line_colour);
  /* bar */
  p1 = ct;
  p1.x -= r/2.0;
  p2 = cb;
  p2.x += r/2.0;
  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &basestation->fill_colour,
                          NULL);
  p2.x -= r;
  dia_renderer_draw_line (renderer,
                          &p1,
                          &p2,
                          &basestation->line_colour);
  p1.x += r;
  p2.x += r;
  dia_renderer_draw_line (renderer,
                          &p1,
                          &p2,
                          &basestation->line_colour);
  /* top */
  dia_renderer_draw_ellipse (renderer,
                             &ct,
                             r,
                             r/2.0,
                             &basestation->fill_colour,
                             &basestation->line_colour);
  /* antenna 2 */
  points[0] = ct; points[0].x += r/4.0;   points[0].y -= 0;
  points[1] = ct; points[1].x += 3.0*r/4.0; points[1].y -= r/2.0;
  points[2] = ct; points[2].x += 3.0*r/4.0; points[2].y += 1.0 - r/2.0;
  points[3] = ct; points[3].x += r/4.0;   points[3].y += 1.0;
  dia_renderer_draw_polygon (renderer,
                             points,
                             4,
                             &basestation->fill_colour,
                             &basestation->line_colour);
  /* antenna 3 */
  points[0] = ct; points[0].x -= r/4.0;   points[0].y -= 0;
  points[1] = ct; points[1].x -= 3.0*r/4.0; points[1].y -= r/2.0;
  points[2] = ct; points[2].x -= 3.0*r/4.0; points[2].y += 1.0 - r/2.0;
  points[3] = ct; points[3].x -= r/4.0;   points[3].y += 1.0;
  dia_renderer_draw_polygon (renderer,
                             points,
                             4,
                             &basestation->fill_colour,
                             &basestation->line_colour);
  text_draw (basestation->text, renderer);
}

static void
basestation_update_data(Basestation *basestation)
{
  Element *elem = &basestation->element;
  DiaObject *obj = &elem->object;
  DiaRectangle text_box;
  Point p;

  elem->width = BASESTATION_WIDTH;
  elem->height = BASESTATION_HEIGHT+basestation->text->height;

  p = elem->corner;
  p.x += elem->width/2;
  p.y += elem->height + basestation->text->ascent;
  text_set_position(basestation->text, &p);

  text_calc_boundingbox(basestation->text, &text_box);

  /* Update connections: */
  element_update_connections_rectangle (elem, basestation->connections);

  element_update_boundingbox(elem);

  /* Add bounding box for text: */
  rectangle_union(&obj->bounding_box, &text_box);

  obj->position = elem->corner;
  obj->position.x += elem->width/2.0;
  obj->position.y += elem->height/2.0;

  element_update_handles(elem);
}

static DiaObject *
basestation_create(Point *startpoint,
                   void *user_data,
                   Handle **handle1,
                   Handle **handle2)
{
  Basestation *basestation;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  basestation = g_new0(Basestation, 1);
  elem = &basestation->element;
  obj = &elem->object;

  obj->type = &basestation_type;
  obj->ops = &basestation_ops;
  elem->corner = *startpoint;
  elem->width = BASESTATION_WIDTH;
  elem->height = BASESTATION_HEIGHT;

  font = dia_font_new_from_style (DIA_FONT_MONOSPACE, 0.8);
  p = *startpoint;
  p.y += BASESTATION_HEIGHT - dia_font_descent (_("Base Station"), font, 0.8);

  basestation->text = new_text (_("Base Station"),
                                font,
                                0.8,
                                &p,
                                &color_black,
                                DIA_ALIGN_CENTRE);
  g_clear_object (&font);
  basestation->line_colour = color_black;
  basestation->fill_colour = color_white;
  basestation->sectors = 3;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0; i<NUM_CONNECTIONS; i++) {
    obj->connections[i] = &basestation->connections[i];
    basestation->connections[i].object = obj;
    basestation->connections[i].connected = NULL;
    basestation->connections[i].flags = 0;
  }
  basestation->connections[8].flags = CP_FLAGS_MAIN;

  elem->extra_spacing.border_trans = BASESTATION_LINEWIDTH/2.0;
  basestation_update_data(basestation);

  for (i=0; i<8; i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &basestation->element.object;
}

static void
basestation_destroy(Basestation *basestation)
{
  text_destroy(basestation->text);

  element_destroy(&basestation->element);
}

static DiaObject *
basestation_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&basestation_type,
                                      obj_node, version,ctx);
}
