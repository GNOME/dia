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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/state.xpm"

typedef struct _State State;
typedef struct _StateState StateState;


enum {
  STATE_NORMAL,
  STATE_BEGIN,
  STATE_END
};

struct _State {
  Element element;

  ConnectionPoint connections[8];

  Text *text;
  int state_type;

  TextAttributes attrs;
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
			Renderer *interactive_renderer);
static void state_move_handle(State *state, Handle *handle,
			     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void state_move(State *state, Point *to);
static void state_draw(State *state, Renderer *renderer);
static Object *state_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void state_destroy(State *state);
static Object *state_load(ObjectNode obj_node, int version,
			    const char *filename);
static PropDescription *state_describe_props(State *state);
static void state_get_props(State *state, GPtrArray *props);
static void state_set_props(State *state, GPtrArray *props);
static void state_update_data(State *state);

void
draw_rounded_rectangle(Renderer *renderer, Point p1, Point p2, real radio);

static ObjectTypeOps state_type_ops =
{
  (CreateFunc) state_create,
  (LoadFunc)   state_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType state_type =
{
  "UML - State",   /* name */
  0,                      /* version */
  (char **) state_xpm,  /* pixmap */
  
  &state_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   state_describe_props,
  (GetPropsFunc)        state_get_props,
  (SetPropsFunc)        state_set_props
};

static PropEnumData prop_state_type_data[] = {
  { N_("Normal"), STATE_NORMAL },
  { N_("Begin"), STATE_BEGIN },
  { N_("End"), STATE_END },
  { NULL, 0 }
};

static PropDescription state_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "state_type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
  N_("State Type"), NULL, prop_state_type_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL }, 
  
  PROP_DESC_END
};

static PropDescription *
state_describe_props(State *state)
{
  return state_props;
}

static PropOffset state_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(State,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(State,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(State,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(State,attrs.color)},
  { "state_type", PROP_TYPE_ENUM, offsetof(State, state_type) },
  { NULL, 0, 0 },
};

static void
state_get_props(State * state, GPtrArray *props)
{
  text_get_attributes(state->text,&state->attrs);
  object_get_props_from_offsets(&state->element.object,
                                state_offsets,props);
}

static void
state_set_props(State *state, GPtrArray *props)
{
  object_set_props_from_offsets(&state->element.object,
                                state_offsets,props);
  apply_textattr_properties(props,state->text,"text",&state->attrs);
  state_update_data(state);
}

static real
state_distance_from(State *state, Point *point)
{
  Object *obj = &state->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
state_select(State *state, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(state->text, clicked_point, interactive_renderer);
  text_grab_focus(state->text, &state->element.object);
  element_update_handles(&state->element);
}

static void
state_move_handle(State *state, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(state!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
state_move(State *state, Point *to)
{
  state->element.corner = *to;
  state_update_data(state);
}

static void
state_draw(State *state, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h, r;
  Point p1, p2;

  assert(state != NULL);
  assert(renderer != NULL);

  elem = &state->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, STATE_LINEWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  if (state->state_type!=STATE_NORMAL) {
      p1.x = x + w/2;
      p1.y = y + h/2;
      if (state->state_type==STATE_END) {
	  r = STATE_ENDRATIO;
	  renderer->ops->fill_ellipse(renderer, 
				      &p1,
				      r, r,
				      &color_white);
	  
	  renderer->ops->draw_ellipse(renderer, 
				      &p1,
				      r, r,
				      &color_black);
      }  
      r = STATE_RATIO;
      renderer->ops->fill_ellipse(renderer, 
				  &p1,
				  r, r,
				  &color_black);
  } else {
      p1.x = x;
      p1.y = y;
      p2.x = x + w;
      p2.y = y + h;
      renderer->ops->fill_rect(renderer, 
			       &p1, &p2,
			       &color_white);

      draw_rounded_rectangle(renderer, p1, p2, 0.5);

      text_draw(state->text, renderer);
  }
}


static void
state_update_data(State *state)
{
  real w, h;

  Element *elem = &state->element;
  Object *obj = &elem->object;
  Point p;
  
  if (state->state_type==STATE_NORMAL) { 
      w = state->text->max_width + 2*STATE_MARGIN_X;
      h = state->text->height*state->text->numlines +2*STATE_MARGIN_Y;
      if (w < STATE_WIDTH)
	  w = STATE_WIDTH;
      p.x = elem->corner.x + w/2.0;
      p.y = elem->corner.y + STATE_MARGIN_Y + state->text->ascent;
      text_set_position(state->text, &p);
  } else {
      w = h = (state->state_type==STATE_END) ? STATE_ENDRATIO: STATE_RATIO;
  }

  elem->width = w;
  elem->height = h;

 /* Update connections: */
  state->connections[0].pos = elem->corner;
  state->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  state->connections[1].pos.y = elem->corner.y;
  state->connections[2].pos.x = elem->corner.x + elem->width;
  state->connections[2].pos.y = elem->corner.y;
  state->connections[3].pos.x = elem->corner.x;
  state->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  state->connections[4].pos.x = elem->corner.x + elem->width;
  state->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  state->connections[5].pos.x = elem->corner.x;
  state->connections[5].pos.y = elem->corner.y + elem->height;
  state->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  state->connections[6].pos.y = elem->corner.y + elem->height;
  state->connections[7].pos.x = elem->corner.x + elem->width;
  state->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
state_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  State *state;
  Element *elem;
  Object *obj;
  Point p;
  DiaFont *font;
  int i;
  
  state = g_malloc0(sizeof(State));
  elem = &state->element;
  obj = &elem->object;
  
  obj->type = &state_type;
  obj->ops = &state_ops;
  elem->corner = *startpoint;
  elem->width = STATE_WIDTH;
  elem->height = STATE_HEIGHT;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += STATE_WIDTH/2.0;
  p.y += STATE_HEIGHT/2.0;
  
  state->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  text_get_attributes(state->text,&state->attrs);
  state->state_type = STATE_NORMAL;
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &state->connections[i];
    state->connections[i].object = obj;
    state->connections[i].connected = NULL;
  }
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
  text_destroy(state->text);

  element_destroy(&state->element);
}

static Object *
state_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&state_type,
                                      obj_node,version,filename);
}


void
draw_rounded_rectangle(Renderer *renderer, Point p1, Point p2, real r)
{
  real x, y, w, h, r2;
  Point c;

  r2 = 2*r;
  x = MIN(p1.x, p2.x);
  y = MIN(p1.y, p2.y);
  
  w = fabs(p2.x - p1.x);
  h = fabs(p2.y - p1.y);

  if (r<=0 || w<r2 || h<r2) {
    renderer->ops->draw_rect(renderer, 
			     &p1, &p2,
			     &color_black);
  }
  c.x = x + r;
  c.y = y + r;
  renderer->ops->draw_arc(renderer,
                          &c,
                          r2, r2,
                          90.0, 180.0,
                          &color_black);
  c.x = x + w - r;
  renderer->ops->draw_arc(renderer,
                          &c,
                          r2, r2,
                          0.0, 90.0, 
                          &color_black);

  c.y = y + h - r;
  renderer->ops->draw_arc(renderer,
                          &c,
                          r2, r2,
                          270.0, 360.0,
                          &color_black);

  c.x = x + r;
  renderer->ops->draw_arc(renderer,
                          &c,
                          r2, r2,
                          180.0, 270.0,
                          &color_black);

  p1.x = p2.x = x;
  p1.y = y + r;
  p2.y = y + h - r;
  renderer->ops->draw_line(renderer, &p1, &p2, &color_black);
  p1.x = p2.x = x + w;
  renderer->ops->draw_line(renderer, &p1, &p2, &color_black);
  p1.x = x + r;
  p2.x = x + w -r;
  p1.y = p2.y = y;
  renderer->ops->draw_line(renderer, &p1, &p2, &color_black);
  p1.y = p2.y = y + h;
  renderer->ops->draw_line(renderer, &p1, &p2, &color_black);
}
