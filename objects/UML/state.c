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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "text.h"

#include "pixmaps/state.xpm"

typedef struct _StatePropertiesDialog StatePropertiesDialog;
typedef struct _State State;


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
};


struct _StatePropertiesDialog {
  GtkWidget *dialog;

  GtkWidget *normal;
  GtkWidget *end;
  GtkWidget *begin;  
};


#define STATE_WIDTH  4
#define STATE_HEIGHT 3
#define STATE_RATIO 1
#define STATE_ENDRATIO 1.5
#define STATE_LINEWIDTH 0.1
#define STATE_MARGIN_X 0.5
#define STATE_MARGIN_Y 0.5

static StatePropertiesDialog *properties_dialog;

static real state_distance_from(State *state, Point *point);
static void state_select(State *state, Point *clicked_point,
			Renderer *interactive_renderer);
static void state_move_handle(State *state, Handle *handle,
			     Point *to, HandleMoveReason reason);
static void state_move(State *state, Point *to);
static void state_draw(State *state, Renderer *renderer);
static Object *state_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void state_destroy(State *state);
static Object *state_copy(State *state);

static void state_save(State *state, ObjectNode obj_node,
			 const char *filename);
static Object *state_load(ObjectNode obj_node, int version,
			    const char *filename);
static void state_update_data(State *state);
static void state_apply_properties(State *state);
static GtkWidget *state_get_properties(State *state);

void
draw_rounded_rectangle(Renderer *renderer, Point p1, Point p2, real radio);

static ObjectTypeOps state_type_ops =
{
  (CreateFunc) state_create,
  (LoadFunc)   state_load,
  (SaveFunc)   state_save
};

ObjectType state_type =
{
  "UML - State",   /* name */
  0,                      /* version */
  (char **) state_xpm,  /* pixmap */
  
  &state_type_ops       /* ops */
};

SheetObject state_sheetobj =
{
  "UML - State",             /* type */
  N_("Create a state machine"),    /* description */
  (char **) state_xpm,     /* pixmap */

  NULL                       /* user_data */
};

static ObjectOps state_ops = {
  (DestroyFunc)         state_destroy,
  (DrawFunc)            state_draw,
  (DistanceFunc)        state_distance_from,
  (SelectFunc)          state_select,
  (CopyFunc)            state_copy,
  (MoveFunc)            state_move,
  (MoveHandleFunc)      state_move_handle,
  (GetPropertiesFunc)   state_get_properties,
  (ApplyPropertiesFunc) state_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

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
  text_grab_focus(state->text, (Object *)state);
  element_update_handles(&state->element);
}

static void
state_move_handle(State *state, Handle *handle,
		 Point *to, HandleMoveReason reason)
{
  assert(state!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
state_move(State *state, Point *to)
{
  real h;
  Point p;
  
  state->element.corner = *to;
  h = state->text->height*state->text->numlines;
  
  p = *to;
  p.x += state->element.width/2.0;
  p.y += (state->element.height - h)/2.0 + state->text->ascent;
  text_set_position(state->text, &p);
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
  Object *obj = (Object *) state;
  
  if (state->state_type==STATE_NORMAL) { 
      w = state->text->max_width + 2*STATE_MARGIN_X;
      h = state->text->height*state->text->numlines +2*STATE_MARGIN_Y;
      if (w < STATE_WIDTH)
	  w = STATE_WIDTH;
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
  Font *font;
  int i;
  
  state = g_malloc(sizeof(State));
  elem = &state->element;
  obj = (Object *) state;
  
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
  state->state_type = STATE_NORMAL;
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &state->connections[i];
    state->connections[i].object = obj;
    state->connections[i].connected = NULL;
  }
  state_update_data(state);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)state;
}

static void
state_destroy(State *state)
{
  text_destroy(state->text);

  element_destroy(&state->element);
}

static Object *
state_copy(State *state)
{
  int i;
  State *newstate;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &state->element;
  
  newstate = g_malloc(sizeof(State));
  newelem = &newstate->element;
  newobj = (Object *) newstate;

  element_copy(elem, newelem);

  newstate->text = text_copy(state->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newstate->connections[i];
    newstate->connections[i].object = newobj;
    newstate->connections[i].connected = NULL;
    newstate->connections[i].pos = state->connections[i].pos;
    newstate->connections[i].last_pos = state->connections[i].last_pos;
  }

  state_update_data(newstate);
  
  return (Object *)newstate;
}


static void
state_save(State *state, ObjectNode obj_node, const char *filename)
{
  element_save(&state->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		state->text);

  data_add_int(new_attribute(obj_node, "type"),
	       state->state_type);

}

static Object *
state_load(ObjectNode obj_node, int version, const char *filename)
{
  State *state;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  state = g_malloc(sizeof(State));
  elem = &state->element;
  obj = (Object *) state;
  
  obj->type = &state_type;
  obj->ops = &state_ops;

  element_load(elem, obj_node);
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
      state->text = data_text(attribute_first_data(attr));
  
  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
      state->state_type = data_int(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &state->connections[i];
    state->connections[i].object = obj;
    state->connections[i].connected = NULL;
  }
  state_update_data(state);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)state;
}


static void
state_apply_properties(State *state)
{
  StatePropertiesDialog *prop_dialog;

  prop_dialog = properties_dialog;

  if (GTK_TOGGLE_BUTTON(prop_dialog->normal)->active) 
      state->state_type = STATE_NORMAL;
  else if (GTK_TOGGLE_BUTTON(prop_dialog->begin)->active) 
      state->state_type = STATE_BEGIN;
  else if (GTK_TOGGLE_BUTTON(prop_dialog->end)->active) 
      state->state_type = STATE_END;

  state_update_data(state);
}

static void
fill_in_dialog(State *state)
{
  StatePropertiesDialog *prop_dialog;
  GtkToggleButton *button=NULL;

  prop_dialog = properties_dialog;
  switch (state->state_type) {
  case STATE_NORMAL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->normal);
      break;
  case STATE_BEGIN:
      button = GTK_TOGGLE_BUTTON(prop_dialog->begin);
      break;
  case STATE_END:
      button = GTK_TOGGLE_BUTTON(prop_dialog->end);
      break;
  }
  if (button)
      gtk_toggle_button_set_active(button, TRUE);
}

static GtkWidget *
state_get_properties(State *state)
{
  StatePropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GSList *group;

  if (properties_dialog == NULL) {
    prop_dialog = g_new(StatePropertiesDialog, 1);
    properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    prop_dialog->normal = gtk_radio_button_new_with_label (NULL, _("Normal"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->normal, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->normal);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop_dialog->normal), TRUE);
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->normal));
    prop_dialog->begin = gtk_radio_button_new_with_label(group, _("Begin"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->begin, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->begin);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->begin));
    prop_dialog->end = gtk_radio_button_new_with_label(group, _("End"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->end, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->end);
  }
  
  fill_in_dialog(state);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
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
