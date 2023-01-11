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

#include "pixmaps/actor.xpm"

#define NUM_CONNECTIONS 9

typedef struct _Actor Actor;
struct _Actor {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;

  real line_width;
  Color line_color;
  Color fill_color;
};

#define ACTOR_WIDTH 2.2
#define ACTOR_HEIGHT 4.6
#define ACTOR_HEAD(h) (h*0.6/4.6)
#define ACTOR_BODY(h) (h*4.0/4.6)
#define ACTOR_LINEWIDTH 0.1
#define ACTOR_MARGIN_X 0.3
#define ACTOR_MARGIN_Y 0.3

static real actor_distance_from(Actor *actor, Point *point);
static void actor_select(Actor *actor, Point *clicked_point,
			DiaRenderer *interactive_renderer);
static DiaObjectChange *actor_move_handle (Actor            *actor,
                                           Handle           *handle,
                                           Point            *to,
                                           ConnectionPoint  *cp,
                                           HandleMoveReason  reason,
                                           ModifierKeys      modifiers);
static DiaObjectChange *actor_move        (Actor            *actor,
                                           Point            *to);
static void actor_draw(Actor *actor, DiaRenderer *renderer);
static DiaObject *actor_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void actor_destroy(Actor *actor);
static DiaObject *actor_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *actor_describe_props(Actor *actor);
static void actor_get_props(Actor *actor, GPtrArray *props);
static void actor_set_props(Actor *actor, GPtrArray *props);

static void actor_update_data(Actor *actor);

static ObjectTypeOps actor_type_ops =
{
  (CreateFunc) actor_create,
  (LoadFunc)   actor_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType actor_type =
{
  "UML - Actor", /* name */
  0,          /* version */
  actor_xpm,   /* pixmap */
  &actor_type_ops /* ops */
};

static ObjectOps actor_ops = {
  (DestroyFunc)         actor_destroy,
  (DrawFunc)            actor_draw,
  (DistanceFunc)        actor_distance_from,
  (SelectFunc)          actor_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            actor_move,
  (MoveHandleFunc)      actor_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   actor_describe_props,
  (GetPropsFunc)        actor_get_props,
  (SetPropsFunc)        actor_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription actor_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
actor_describe_props(Actor *actor)
{
  if (actor_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(actor_props);
  }
  return actor_props;
}

static PropOffset actor_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Actor,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Actor,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Actor,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Actor,text),offsetof(Text,color)},
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Actor, line_width) },
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Actor,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Actor,fill_color)},
  { NULL, 0, 0 },
};

static void
actor_get_props(Actor * actor, GPtrArray *props)
{
  object_get_props_from_offsets(&actor->element.object,
                                actor_offsets,props);
}

static void
actor_set_props(Actor *actor, GPtrArray *props)
{
  object_set_props_from_offsets(&actor->element.object,
                                actor_offsets,props);
  actor_update_data(actor);
}

static real
actor_distance_from(Actor *actor, Point *point)
{
  DiaObject *obj = &actor->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
actor_select(Actor *actor, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(actor->text, clicked_point, interactive_renderer);
  text_grab_focus(actor->text, &actor->element.object);
  element_update_handles(&actor->element);
}


static DiaObjectChange *
actor_move_handle (Actor            *actor,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  DiaObjectChange *oc;

  g_return_val_if_fail (actor != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  oc = element_move_handle (&actor->element,
                            handle->id,
                            to,
                            cp,
                            reason,
                            modifiers);
  actor_update_data (actor);

  return oc;
}


static DiaObjectChange *
actor_move (Actor *actor, Point *to)
{
  Element *elem = &actor->element;

  elem->corner = *to;
  elem->corner.x -= elem->width/2.0;
  elem->corner.y -= elem->height / 2.0;

  actor_update_data(actor);

  return NULL;
}


static void
actor_draw (Actor *actor, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w;
  double r, r1;
  Point ch, cb, p1, p2;
  double actor_height;

  g_return_if_fail (actor != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &actor->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  actor_height = elem->height - actor->text->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, actor->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  r = ACTOR_HEAD (actor_height);
  r1 = 2*r;
  ch.x = x + w*0.5;
  ch.y = y + r + ACTOR_MARGIN_Y;
  cb.x = ch.x;
  cb.y = ch.y + r1 + r;

  /* head */
  dia_renderer_draw_ellipse (renderer,
                             &ch,
                             r,
                             r,
                             &actor->fill_color,
                             &actor->line_color);

  /* Arms */
  p1.x = ch.x - r1;
  p2.x = ch.x + r1;
  p1.y = p2.y = ch.y + r;
  dia_renderer_draw_line (renderer,
                          &p1,
                          &p2,
                          &actor->line_color);

  p1.x = ch.x;
  p1.y = ch.y + r*0.5;
  /* body & legs  */
  dia_renderer_draw_line (renderer,
                          &p1,
                          &cb,
                          &actor->line_color);

  p2.x = ch.x - r1;
  p2.y = y + ACTOR_BODY (actor_height);
  dia_renderer_draw_line (renderer,
                          &cb,
                          &p2,
                          &actor->line_color);

  p2.x =  ch.x + r1;
  dia_renderer_draw_line (renderer,
                          &cb,
                          &p2,
                          &actor->line_color);

  text_draw (actor->text, renderer);
}

static void
actor_update_data(Actor *actor)
{
  Element *elem = &actor->element;
  DiaObject *obj = &elem->object;
  DiaRectangle text_box;
  Point p;
  real actor_height;

  text_calc_boundingbox(actor->text, &text_box);

  /* minimum size */
  if (elem->width < ACTOR_WIDTH + ACTOR_MARGIN_X)
    elem->width = ACTOR_WIDTH + ACTOR_MARGIN_X;
  if (elem->height < ACTOR_HEIGHT + actor->text->height)
    elem->height = ACTOR_HEIGHT + actor->text->height;
  actor_height = elem->height - actor->text->height;

  /* Update connections: */
  element_update_connections_rectangle(elem, actor->connections);

  element_update_boundingbox(elem);

  p = elem->corner;
  p.x += elem->width/2;
  p.y +=  actor_height + actor->text->ascent;
  text_set_position(actor->text, &p);
  /* may have moved */
  text_calc_boundingbox(actor->text, &text_box);

  /* Add bounding box for text: */
  rectangle_union(&obj->bounding_box, &text_box);

  obj->position = elem->corner;
  obj->position.x += elem->width/2.0;
  obj->position.y += elem->height / 2.0;

  element_update_handles(elem);
}

static DiaObject *
actor_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Actor *actor;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  actor = g_new0 (Actor, 1);
  elem = &actor->element;
  obj = &elem->object;

  obj->type = &actor_type;
  obj->ops = &actor_ops;
  elem->corner = *startpoint;
  elem->width = ACTOR_WIDTH;
  elem->height = ACTOR_HEIGHT;

  actor->line_width = attributes_get_default_linewidth();
  actor->line_color = attributes_get_foreground();
  actor->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, 0.8);
  p = *startpoint;
  p.x += ACTOR_MARGIN_X;
  p.y += ACTOR_HEIGHT - dia_font_descent(_("Actor"),font, 0.8);

  actor->text = new_text (_("Actor"),
                          font,
                          0.8,
                          &p,
                          &color_black,
                          DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &actor->connections[i];
    actor->connections[i].object = obj;
    actor->connections[i].connected = NULL;
  }
  actor->connections[8].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = actor->line_width/2.0;
  actor_update_data(actor);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &actor->element.object;
}

static void
actor_destroy(Actor *actor)
{
  text_destroy(actor->text);

  element_destroy(&actor->element);
}

static DiaObject *
actor_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&actor_type,
                                                obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * ACTOR_LINEWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((Actor*)obj)->line_width = ACTOR_LINEWIDTH;

  return obj;
}


