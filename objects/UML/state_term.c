/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * State terminal type for UML diagrams
 * Copyright (C) 2002 Alejandro Sierra <asierra@servidor.unam.mx>
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

#include "pixmaps/state_term.xpm"

#define NUM_CONNECTIONS 9

typedef struct _State State;
struct _State {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  int is_final;

  Color line_color;
  Color fill_color;
};


#define STATE_WIDTH  4
#define STATE_HEIGHT 3
#define STATE_RATIO 1
#define STATE_ENDRATIO 1.5
#define STATE_LINEWIDTH 0.1
#define STATE_MARGIN_X 0.5
#define STATE_MARGIN_Y 0.5

static real state_distance_from(State *state, Point *point);
static void state_select(State *state, Point *clicked_point,
			DiaRenderer *interactive_renderer);
static DiaObjectChange* state_move_handle(State *state, Handle *handle,
				       Point *to, ConnectionPoint *cp,
				       HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* state_move(State *state, Point *to);
static void state_draw(State *state, DiaRenderer *renderer);
static DiaObject *state_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void state_destroy(State *state);
static DiaObject *state_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *state_describe_props(State *state);
static void state_get_props(State *state, GPtrArray *props);
static void state_set_props(State *state, GPtrArray *props);
static void state_update_data(State *state);

static ObjectTypeOps state_type_ops =
{
  (CreateFunc) state_create,
  (LoadFunc)   state_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType state_term_type =
{
  "UML - State Term", /* name */
  0,               /* version */
  state_term_xpm,   /* pixmap */
  &state_type_ops      /* ops */
};

static ObjectOps state_ops = {
  (DestroyFunc)         state_destroy,
  (DrawFunc)            state_draw,
  (DistanceFunc)        state_distance_from,
  (SelectFunc)          state_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            state_move,
  (MoveHandleFunc)      state_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   state_describe_props,
  (GetPropsFunc)        state_get_props,
  (SetPropsFunc)        state_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription state_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  { "is_final", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Is final"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
state_describe_props(State *state)
{
  if (state_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(state_props);
  }
  return state_props;
}

static PropOffset state_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_colour",PROP_TYPE_COLOUR,offsetof(State, line_color) },
  { "fill_colour",PROP_TYPE_COLOUR,offsetof(State, fill_color) },
  { "is_final", PROP_TYPE_BOOL, offsetof(State, is_final) },

  { NULL, 0, 0 },
};

static void
state_get_props(State * state, GPtrArray *props)
{
  object_get_props_from_offsets(&state->element.object,
                                state_offsets,props);
}

static void
state_set_props(State *state, GPtrArray *props)
{
  object_set_props_from_offsets(&state->element.object,
                                state_offsets,props);
  state_update_data(state);
}

static real
state_distance_from(State *state, Point *point)
{
  DiaObject *obj = &state->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
state_select(State *state, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  element_update_handles(&state->element);
}


static DiaObjectChange *
state_move_handle (State            *state,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  g_return_val_if_fail (state != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
state_move(State *state, Point *to)
{
  state->element.corner = *to;
  state_update_data(state);

  return NULL;
}

static void
state_draw (State *state, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h, r;
  Point p1;

  g_return_if_fail (state != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &state->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, STATE_LINEWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);

  p1.x = x + w/2;
  p1.y = y + h/2;
  if (state->is_final==1) {
    r = STATE_ENDRATIO;
    dia_renderer_draw_ellipse (renderer,
                                &p1,
                                r,
                                r,
                                &state->fill_color,
                                &state->line_color);
  }
  r = STATE_RATIO;
  dia_renderer_draw_ellipse (renderer,
                            &p1,
                            r,
                            r,
                            &state->line_color,
                            NULL); /* line_color not a typo! */
}


static void
state_update_data(State *state)
{
  real w, h;

  Element *elem = &state->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;

  w = h = (state->is_final) ? STATE_ENDRATIO: STATE_RATIO;

  elem->width = w;
  elem->height = h;
  extra->border_trans = STATE_LINEWIDTH / 2.0;

 /* Update connections: */
  element_update_connections_rectangle(elem, state->connections);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
state_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  State *state;
  Element *elem;
  DiaObject *obj;
  Point p;
  int i;

  state = g_new0 (State, 1);
  elem = &state->element;
  obj = &elem->object;

  obj->type = &state_term_type;
  obj->ops = &state_ops;
  elem->corner = *startpoint;
  elem->width = STATE_WIDTH;
  elem->height = STATE_HEIGHT;

  p = *startpoint;
  p.x += STATE_WIDTH/2.0;
  p.y += STATE_HEIGHT/2.0;

  state->line_color = attributes_get_foreground();
  state->fill_color = attributes_get_background();
  state->is_final = 0;
  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &state->connections[i];
    state->connections[i].object = obj;
    state->connections[i].connected = NULL;
  }
  state->connections[8].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = 0.0;
  state_update_data(state);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &state->element.object;
}

static void
state_destroy(State *state)
{
  element_destroy(&state->element);
}

static DiaObject *
state_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&state_term_type,
                                      obj_node,version,ctx);
}


