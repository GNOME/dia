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
#include "message.h"

#include "pixmaps/state.xpm"

typedef struct _State State;
typedef struct _StateState StateState;


enum {
  STATE_NORMAL,
  STATE_BEGIN,
  STATE_END
};

typedef enum {
  ENTRY_ACTION,
  DO_ACTION,
  EXIT_ACTION
} StateAction;

#define NUM_CONNECTIONS 9

struct _State {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;
  int state_type;

  Color line_color;
  Color fill_color;

  double line_width;

  char *entry_action;
  char *do_action;
  char *exit_action;
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
static char* state_get_action_text (State* state, StateAction action);
static void state_calc_action_text_pos(State* state, StateAction action, Point* pos);

static ObjectTypeOps state_type_ops =
{
  (CreateFunc) state_create,
  (LoadFunc)   state_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType state_type =
{
  "UML - State", /* name */
  0,          /* version */
  state_xpm,   /* pixmap */
  &state_type_ops /* ops */
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
      /* see below for the next field.

      Warning: break this and you'll get angry UML users after you. */
  { "type", PROP_TYPE_INT, PROP_FLAG_NO_DEFAULTS|PROP_FLAG_LOAD_ONLY|PROP_FLAG_OPTIONAL,
    "hack", NULL, NULL },

  { "entry_action", PROP_TYPE_STRING, PROP_FLAG_OPTIONAL | PROP_FLAG_VISIBLE, N_("Entry action"), NULL, NULL },
  { "do_action", PROP_TYPE_STRING, PROP_FLAG_OPTIONAL | PROP_FLAG_VISIBLE, N_("Do action"), NULL, NULL },
  { "exit_action", PROP_TYPE_STRING, PROP_FLAG_OPTIONAL | PROP_FLAG_VISIBLE, N_("Exit action"),  NULL, NULL },

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
state_describe_props(State *state)
{
  if (state_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(state_props);
  }
  return state_props;
}

static PropOffset state_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,

  {"entry_action",PROP_TYPE_STRING,offsetof(State,entry_action)},
  {"do_action",PROP_TYPE_STRING,offsetof(State,do_action)},
  {"exit_action",PROP_TYPE_STRING,offsetof(State,exit_action)},
      /* HACK: this is to recover files from 0.88.1 and older.
      if sizeof(enum) != sizeof(int), we're toast.             -- CC
      */
  { "type", PROP_TYPE_INT, offsetof(State, state_type) },

  {"text",PROP_TYPE_TEXT,offsetof(State,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(State,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(State,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(State,text),offsetof(Text,color)},

  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(State, line_width) },
  {"line_colour",PROP_TYPE_COLOUR,offsetof(State,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(State,fill_color)},

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
  text_set_cursor(state->text, clicked_point, interactive_renderer);
  text_grab_focus(state->text, &state->element.object);
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
state_draw_action_string (State       *state,
                          DiaRenderer *renderer,
                          StateAction  action)
{
  Point pos;
  char *action_text = state_get_action_text (state, action);

  state_calc_action_text_pos (state, action, &pos);
  dia_renderer_set_font (renderer, state->text->font, state->text->height);
  dia_renderer_draw_string (renderer,
                            action_text,
                            &pos,
                            DIA_ALIGN_LEFT,
                            &state->text->color);
  g_clear_pointer (&action_text, g_free);
}


static void
state_draw (State *state, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h, r;
  Point p1, p2, split_line_left, split_line_right;
  gboolean has_actions;

  g_return_if_fail (state != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &state->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, state->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  if (state->state_type!=STATE_NORMAL) {
    p1.x = x + w/2;
    p1.y = y + h/2;
    if (state->state_type==STATE_END) {
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
                               NULL,
                               &state->line_color);
  } else {
    p1.x = x;
    p1.y = y;
    p2.x = x + w;
    p2.y = y + h;
    dia_renderer_draw_rounded_rect (renderer,
                                    &p1,
                                    &p2,
                                    &state->fill_color,
                                    &state->line_color,
                                    0.5);

    text_draw (state->text, renderer);
    has_actions = FALSE;
    if (state->entry_action && strlen (state->entry_action) != 0) {
      state_draw_action_string (state, renderer, ENTRY_ACTION);
      has_actions = TRUE;
    }
    if (state->do_action && strlen (state->do_action) != 0) {
      state_draw_action_string (state, renderer, DO_ACTION);
      has_actions = TRUE;
    }
    if (state->exit_action && strlen (state->exit_action) != 0) {
      state_draw_action_string (state, renderer, EXIT_ACTION);
      has_actions = TRUE;
    }

    if (has_actions) {
      split_line_left.x = x;
      split_line_right.x = x+w;
      split_line_left.y = split_line_right.y
                        = state->element.corner.y + STATE_MARGIN_Y +
                          state->text->numlines*state->text->height;
      dia_renderer_draw_line (renderer,
                              &split_line_left,
                              &split_line_right,
                              &state->line_color);
    }
  }
}


static void
state_update_width_and_height_with_action_text (State       *state,
                                                StateAction  action,
                                                double      *width,
                                                double      *height)
{
  char *action_text = state_get_action_text (state, action);
  *width = MAX (*width,
                dia_font_string_width (action_text,
                                       state->text->font,
                                       state->text->height) + 2 * STATE_MARGIN_X);
  g_clear_pointer (&action_text, g_free);
  *height += state->text->height;
}


static void
state_update_data (State *state)
{
  double w, h;

  Element *elem = &state->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  Point p;

  text_calc_boundingbox(state->text, NULL);
  if (state->state_type==STATE_NORMAL) {
      /* First consider the state description text */
      w = state->text->max_width + 2*STATE_MARGIN_X;
      h = state->text->height*state->text->numlines +2*STATE_MARGIN_Y;
      if (w < STATE_WIDTH)
	  w = STATE_WIDTH;
      /* Then consider the actions texts */
      if (state->entry_action && strlen(state->entry_action) != 0) {
        state_update_width_and_height_with_action_text(state, ENTRY_ACTION, &w, &h);
      }
      if (state->do_action && strlen(state->do_action) != 0) {
        state_update_width_and_height_with_action_text(state, DO_ACTION, &w, &h);
      }
      if (state->exit_action && strlen(state->exit_action) != 0) {
        state_update_width_and_height_with_action_text(state, EXIT_ACTION, &w, &h);
      }

      p.x = elem->corner.x + w/2.0;
      p.y = elem->corner.y + STATE_MARGIN_Y + state->text->ascent;
      text_set_position(state->text, &p);
  } else {
      w = h = (state->state_type==STATE_END) ? STATE_ENDRATIO: STATE_RATIO;
  }

  elem->width = w;
  elem->height = h;
  extra->border_trans = state->line_width / 2.0;

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
  DiaFont *font;
  int i;

  state = g_new0 (State, 1);

  /* old default */
  state->line_width = 0.1;

  elem = &state->element;
  obj = &elem->object;

  obj->type = &state_type;
  obj->ops = &state_ops;
  elem->corner = *startpoint;
  elem->width = STATE_WIDTH;
  elem->height = STATE_HEIGHT;

  state->line_color = attributes_get_foreground();
  state->fill_color = attributes_get_background();

  font = dia_font_new_from_style(DIA_FONT_SANS, 0.8);
  p = *startpoint;
  p.x += STATE_WIDTH/2.0;
  p.y += STATE_HEIGHT/2.0;

  state->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_CENTRE);

  g_clear_object (&font);

  state->state_type = STATE_NORMAL;
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
state_destroy (State *state)
{
  g_clear_pointer (&state->entry_action, g_free);
  g_clear_pointer (&state->do_action, g_free);
  g_clear_pointer (&state->exit_action, g_free);

  text_destroy (state->text);

  element_destroy (&state->element);
}


static DiaObject *
state_load (ObjectNode obj_node, int version, DiaContext *ctx)
{
  State *obj = (State*) object_load_using_properties (&state_type,
                                                      obj_node,
                                                      version,
                                                      ctx);
  if (obj->state_type != STATE_NORMAL) {
    /* Would like to create a state_term instead, but making the connections
     * is a pain */
    message_warning (_("This diagram uses the State object for initial/final "
                       "states.\nThis option will go away in future versions."
                       "\nPlease use the Initial/Final State object "
                       "instead.\n"));
  }

  return (DiaObject *) obj;
}


static void
state_calc_action_text_pos (State* state, StateAction action, Point* pos)
{
  int entry_action_valid = state->entry_action &&
                           strlen (state->entry_action) != 0;
  int do_action_valid = state->do_action &&
                        strlen (state->do_action) != 0;

  real first_action_y = state->text->numlines*state->text->height +
                        state->text->position.y;

  pos->x = state->element.corner.x + STATE_MARGIN_X;

  switch (action) {
    case ENTRY_ACTION:
      pos->y = first_action_y;
      break;

    case DO_ACTION:
      pos->y = first_action_y;
      if (entry_action_valid) pos->y += state->text->height;
      break;

    case EXIT_ACTION:
      pos->y = first_action_y;
      if (entry_action_valid) pos->y += state->text->height;
      if (do_action_valid) pos->y += state->text->height;
      break;

    default:
      g_return_if_reached ();
  }
}


static char *
state_get_action_text (State* state, StateAction action)
{
  switch (action) {
    case ENTRY_ACTION:
      return g_strdup_printf ("entry/ %s", state->entry_action);
      break;

    case DO_ACTION:
      return g_strdup_printf ("do/ %s", state->do_action);
      break;

    case EXIT_ACTION:
      return g_strdup_printf ("exit/ %s", state->exit_action);
      break;

    default:
      g_return_val_if_reached (NULL);
  }

  return NULL;
}
