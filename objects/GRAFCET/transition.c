/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support 
 * Copyright(C) 2000 Cyrille Chepelov
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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "color.h"
#include "lazyprops.h"
#include "geometry.h"
#include "text.h"

#include "grafcet.h"
#include "boolequation.h"

#include "pixmaps/transition.xpm"

#define HANDLE_NORTH HANDLE_CUSTOM1
#define HANDLE_SOUTH HANDLE_CUSTOM2


#define TRANSITION_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH
#define TRANSITION_FONT "Helvetica-Bold"
#define TRANSITION_FONT_HEIGHT 0.8
#define TRANSITION_DECLAREDHEIGHT 2.0
#define TRANSITION_DECLAREDWIDTH 2.0
#define TRANSITION_HEIGHT .5
#define TRANSITION_WIDTH 1.5

typedef struct _TransitionPropertiesDialog TransitionPropertiesDialog;
typedef struct _TransitionDefaultsDialog TransitionDefaultsDialog;
typedef struct _TransitionState TransitionState;

struct _TransitionState {
  ObjectState obj_state;
  
  char *rcep_value;
  Font *rcep_font;
  real rcep_fontheight;
  Color rcep_color;
};

typedef struct _Transition {
  Element element;
  
  Boolequation *receptivity;
  Font *rcep_font;
  real rcep_fontheight;
  Color rcep_color;
  char *rcep_value;

  ConnectionPoint connections[2];
  Handle north,south;
  Point SD1,SD2,NU1,NU2;
  
  /* computed values : */
  Rectangle rceptbb; /* The bounding box of the receptivity */
  Point A,B,C,D,Z;
} Transition;

struct _TransitionPropertiesDialog {
  AttributeDialog dialog;
  Transition *parent;

  StringAttribute rcep_value;
  FontAttribute rcep_font;
  FontHeightAttribute rcep_fontheight;
  ColorAttribute rcep_color;
};

typedef struct _TransitionDefaults {
  Font *font;
  real font_size;
  Color font_color;
} TransitionDefaults;

struct _TransitionDefaultsDialog {
  AttributeDialog dialog;
  TransitionDefaults *parent;

  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

static TransitionPropertiesDialog *transition_properties_dialog;
static TransitionDefaultsDialog *transition_defaults_dialog;
static TransitionDefaults defaults;

static void transition_move_handle(Transition *transition, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void transition_move(Transition *transition, Point *to);
static void transition_select(Transition *transition, Point *clicked_point,
			      Renderer *interactive_renderer);
static void transition_draw(Transition *transition, Renderer *renderer);
static Object *transition_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real transition_distance_from(Transition *transition, Point *point);
static void transition_update_data(Transition *transition);
static void transition_destroy(Transition *transition);
static Object *transition_copy(Transition *transition);
static GtkWidget *transition_get_properties(Transition *transition);
static ObjectChange *transition_apply_properties(Transition *transition);

static TransitionState *transition_get_state(Transition *transition);
static void transition_set_state(Transition *transition, TransitionState *state);

static void transition_save(Transition *transition, ObjectNode obj_node,
			    const char *filename);
static Object *transition_load(ObjectNode obj_node, int version,
			       const char *filename);

static GtkWidget *transition_get_defaults();
static void transition_apply_defaults();

static ObjectTypeOps transition_type_ops =
{
  (CreateFunc)transition_create,   /* create */
  (LoadFunc)  transition_load,     /* load */
  (SaveFunc)  transition_save,      /* save */
  (GetDefaultsFunc)   transition_get_defaults, 
  (ApplyDefaultsFunc) transition_apply_defaults
};

ObjectType transition_type =
{
  "GRAFCET - Transition",   /* name */
  0,                         /* version */
  (char **) transition_xpm,      /* pixmap */
  
  &transition_type_ops       /* ops */
};


static ObjectOps transition_ops = {
  (DestroyFunc)         transition_destroy,
  (DrawFunc)            transition_draw,
  (DistanceFunc)        transition_distance_from,
  (SelectFunc)          transition_select,
  (CopyFunc)            transition_copy,
  (MoveFunc)            transition_move,
  (MoveHandleFunc)      transition_move_handle,
  (GetPropertiesFunc)   transition_get_properties,
  (ApplyPropertiesFunc) transition_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
transition_apply_properties(Transition *transition)
{
  ObjectState *old_state;
  TransitionPropertiesDialog *dlg = transition_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,transition);
  
  old_state = (ObjectState *)transition_get_state(transition);

  PROPDLG_APPLY_STRING(dlg,rcep_value);
  PROPDLG_APPLY_FONT(dlg,rcep_font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,rcep_fontheight);
  PROPDLG_APPLY_COLOR(dlg,rcep_color);
  
  boolequation_set_value(transition->receptivity,transition->rcep_value);
  transition->receptivity->font = transition->rcep_font;
  transition->receptivity->fontheight = transition->rcep_fontheight;
  transition->receptivity->color = transition->rcep_color;

  transition_update_data(transition);
  return new_object_state_change((Object *)transition, old_state, 
				 (GetStateFunc)transition_get_state,
				 (SetStateFunc)transition_set_state);
}

static PROPDLG_TYPE
transition_get_properties(Transition *transition)
{
  TransitionPropertiesDialog *dlg = transition_properties_dialog;
  
  PROPDLG_CREATE(dlg,transition);
  PROPDLG_SHOW_STRING(dlg,rcep_value,_("Receptivity:"));
  PROPDLG_SHOW_FONT(dlg,rcep_font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,rcep_fontheight,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,rcep_color,_("Text color:"));
  PROPDLG_READY(dlg);
  
  transition_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void 
transition_apply_defaults()
{
  TransitionDefaultsDialog *dlg = transition_defaults_dialog;  

  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);
}

static void
init_default_values() {
  static int defaults_initialized = 0;
  
  if (!defaults_initialized) {
    defaults.font = font_getfont(TRANSITION_FONT);
    defaults.font_size = TRANSITION_FONT_HEIGHT;
    defaults.font_color = color_black;

    defaults_initialized = 1;
  }
}

static PROPDLG_TYPE
transition_get_defaults()
{
  TransitionDefaultsDialog *dlg = transition_defaults_dialog;
  init_default_values();
  PROPDLG_CREATE(dlg, &defaults);

  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Text color:"));
  PROPDLG_READY(dlg);

  transition_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void 
transition_free(ObjectState *objstate)
{
  TransitionState *state = (TransitionState *)objstate;

  OBJECT_FREE_STRING(state,rcep_value);
  OBJECT_FREE_FONT(state,rcep_font);
  OBJECT_FREE_COLOR(state,rcep_color);
  OBJECT_FREE_FONTHEIGHT(state,rcep_fontheight);
}

static TransitionState *
transition_get_state(Transition *transition)
{
  TransitionState *state = g_new0(TransitionState, 1);

  state->obj_state.free = transition_free;

  OBJECT_GET_STRING(transition,state,rcep_value);
  OBJECT_GET_FONT(transition,state,rcep_font);
  OBJECT_GET_COLOR(transition,state,rcep_color);
  OBJECT_GET_FONTHEIGHT(transition,state,rcep_fontheight);

  return state;
}

static void
transition_set_state(Transition *transition, TransitionState *state)
{
  OBJECT_SET_STRING(transition,state,rcep_value);
  OBJECT_SET_FONT(transition,state,rcep_font);
  OBJECT_SET_COLOR(transition,state,rcep_color);
  OBJECT_SET_FONTHEIGHT(transition,state,rcep_fontheight);
  boolequation_set_value(transition->receptivity,transition->rcep_value);
  transition->receptivity->font = transition->rcep_font;
  transition->receptivity->fontheight = transition->rcep_fontheight;
  transition->receptivity->color = transition->rcep_color;

  g_free(state);
  
  transition_update_data(transition);
}

static void
transition_update_data(Transition *transition)
{
  Element *elem = &transition->element;
  Object *obj = (Object *) transition;
  Point *p;

  obj->position = elem->corner;
  elem->width = TRANSITION_DECLAREDWIDTH;
  elem->height = TRANSITION_DECLAREDWIDTH;

  /* compute the useful points' positions : */
  transition->A.x = transition->B.x = (TRANSITION_DECLAREDWIDTH / 2.0);
  transition->A.y = (TRANSITION_DECLAREDHEIGHT / 2.0) 
    - (TRANSITION_HEIGHT / 2.0);
  transition->B.y = transition->A.y + TRANSITION_HEIGHT;
  transition->C.y = transition->D.y = (TRANSITION_DECLAREDHEIGHT / 2.0);
  transition->C.x = 
    (TRANSITION_DECLAREDWIDTH / 2.0) - (TRANSITION_WIDTH / 2.0);
  transition->D.x = transition->C.x + TRANSITION_WIDTH;
  
  transition->Z.y = (TRANSITION_DECLAREDHEIGHT / 2.0) 
    + (.3 * transition->receptivity->fontheight);

  transition->Z.x = transition->D.x + 
    font_string_width(" ",transition->receptivity->font,
		      transition->receptivity->fontheight);

  for (p = &transition->A; p <= &transition->Z; p++) 
    point_add(p,&elem->corner);

  transition->receptivity->pos = transition->Z;


  /* Update handles: */
  if (transition->north.pos.x == -65536.0) {
    transition->north.pos = transition->A;
    transition->south.pos = transition->B;
  }
  transition->NU1.x = transition->north.pos.x;
  transition->NU2.x = transition->A.x;
  transition->NU1.y = transition->NU2.y = 
    (transition->north.pos.y + transition->A.y) / 2.0;
  transition->SD1.x = transition->B.x;
  transition->SD2.x = transition->south.pos.x;
  transition->SD1.y = transition->SD2.y = 
    (transition->south.pos.y + transition->B.y) / 2.0;

#if 0
  /* Not really a good idea */
  obj->connections[0]->pos = transition->north.pos;
  obj->connections[1]->pos = transition->south.pos;
#else
  obj->connections[0]->pos = transition->A;
  obj->connections[1]->pos = transition->B;
#endif

  element_update_boundingbox(elem);

  rectangle_add_point(&obj->bounding_box,&transition->north.pos);
  rectangle_add_point(&obj->bounding_box,&transition->south.pos);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= TRANSITION_LINE_WIDTH/2;
  obj->bounding_box.left -= TRANSITION_LINE_WIDTH/2 ;
  obj->bounding_box.bottom += TRANSITION_LINE_WIDTH/2;
  obj->bounding_box.right += TRANSITION_LINE_WIDTH/2;

  /* compute the rcept's width and bounding box, then merge. */
  boolequation_calc_boundingbox(transition->receptivity,&transition->rceptbb);

  rectangle_union(&obj->bounding_box,&transition->rceptbb);
  element_update_handles(elem);
}

static real
transition_distance_from(Transition *transition, Point *point)
{
  real dist;
  dist = distance_rectangle_point(&transition->rceptbb,point);
  dist = MIN(dist,distance_line_point(&transition->C,&transition->D,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->north.pos,&transition->NU1,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->NU1,&transition->NU2,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->NU2,&transition->SD1,
				      TRANSITION_LINE_WIDTH,point));
  /* A and B are on the [NU2; SD1] segment. */
  dist = MIN(dist,distance_line_point(&transition->SD1,&transition->SD2,
				      TRANSITION_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&transition->SD2,&transition->south.pos,
				      TRANSITION_LINE_WIDTH,point));

  return dist;
}

static void
transition_select(Transition *transition, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  transition_update_data(transition);
}

static void
transition_move_handle(Transition *transition, Handle *handle,
		       Point *to, HandleMoveReason reason, 
		       ModifierKeys modifiers)
{
  g_assert(transition!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  switch(handle->id) {
  case HANDLE_NORTH:
    transition->north.pos = *to;
    if (transition->north.pos.y > transition->A.y) 
      transition->north.pos.y = transition->A.y;
    break;
  case HANDLE_SOUTH:
    transition->south.pos = *to;
    if (transition->south.pos.y < transition->B.y) 
      transition->south.pos.y = transition->B.y;
    break;
  default:
    element_move_handle(&transition->element, handle->id, to, reason);
  }

  transition_update_data(transition);
}


static void
transition_move(Transition *transition, Point *to)
{
  Point delta = *to;
  Element *elem = &transition->element;
  point_sub(&delta,&transition->element.corner);
  transition->element.corner = *to;
  point_add(&transition->north.pos,&delta);
  point_add(&transition->south.pos,&delta);

  element_update_handles(elem);
  transition_update_data(transition);
}


static void 
transition_draw(Transition *transition, Renderer *renderer)
{
  Point pts[6];
  renderer->ops->set_linewidth(renderer, TRANSITION_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  pts[0] = transition->north.pos;
  pts[1] = transition->NU1;
  pts[2] = transition->NU2;
  pts[3] = transition->SD1;
  pts[4] = transition->SD2;
  pts[5] = transition->south.pos;
  renderer->ops->draw_polyline(renderer,pts,sizeof(pts)/sizeof(pts[0]),
			       &color_black);

  renderer->ops->draw_line(renderer,
			     &transition->C,&transition->D,
			     &color_black);
  
  boolequation_draw(transition->receptivity,renderer);
}

static Object *
transition_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Transition *transition;
  Object *obj;
  int i;
  Element *elem;

  init_default_values();
  transition = g_malloc0(sizeof(Transition));
  elem = &transition->element;
  obj = (Object *) transition;
  
  obj->type = &transition_type;
  obj->ops = &transition_ops;
  
  elem->corner = *startpoint;
  elem->width = TRANSITION_DECLAREDWIDTH;
  elem->height = TRANSITION_DECLAREDHEIGHT;

  element_init(elem, 10,2);

  
  transition->receptivity = boolequation_create("",
					       defaults.font,
					       defaults.font_size,
					       &defaults.font_color);
  transition->rcep_value = g_strdup("");
  transition->rcep_font = defaults.font;
  transition->rcep_fontheight = defaults.font_size;
  transition->rcep_color = defaults.font_color;


  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &transition->north;
  obj->handles[9] = &transition->south;
  transition->north.connect_type = HANDLE_CONNECTABLE;
  transition->north.type = HANDLE_MAJOR_CONTROL;
  transition->north.id = HANDLE_NORTH;
  transition->south.connect_type = HANDLE_CONNECTABLE;
  transition->south.type = HANDLE_MAJOR_CONTROL;
  transition->south.id = HANDLE_SOUTH;
  transition->north.pos.x = -65536.0; /* magic */

  for (i=0;i<2;i++) {
    obj->connections[i] = &transition->connections[i];
    obj->connections[i]->object = obj;
    obj->connections[i]->connected = NULL;
  }

  transition_update_data(transition);

  *handle1 = NULL;
  *handle2 = obj->handles[0];  

  return (Object *)transition;
}

static void
transition_destroy(Transition *transition)
{
  boolequation_destroy(transition->receptivity);
  element_destroy(&transition->element);
}

static Object *
transition_copy(Transition *transition)
{
  Transition *newtransition;
  Element *elem, *newelem;
  int i;
  Object *newobj;
  
  elem = &transition->element;
 
  newtransition = g_malloc0(sizeof(Transition));
  newelem = &newtransition->element;
  newobj = (Object *) newtransition;

  element_copy(elem, newelem);

  newtransition->receptivity = boolequation_create(transition->rcep_value,
						  transition->rcep_font,
						  transition->rcep_fontheight,
						  &transition->rcep_color);
  newtransition->rcep_value = g_strdup(transition->rcep_value);
  newtransition->rcep_fontheight = transition->rcep_fontheight;
  newtransition->rcep_font = transition->rcep_font;
  newtransition->rcep_color = transition->rcep_color;

  for (i=0;i<8;i++) {
    newobj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  newobj->handles[8] = &newtransition->north;
  newobj->handles[9] = &newtransition->south;
  newtransition->north.connect_type = HANDLE_CONNECTABLE;
  newtransition->north.type = HANDLE_MAJOR_CONTROL;
  newtransition->north.id = HANDLE_NORTH;
  newtransition->south.connect_type = HANDLE_CONNECTABLE;
  newtransition->south.type = HANDLE_MAJOR_CONTROL;
  newtransition->south.id = HANDLE_SOUTH;
  newtransition->north.pos = transition->A;
  newtransition->south.pos = transition->B;

  for (i=0;i<2;i++) {
    newobj->connections[i] = &newtransition->connections[i];
    newobj->connections[i]->object = newobj;
    newobj->connections[i]->connected = NULL;
  }

  transition_update_data(newtransition);

  return (Object *)newtransition;
}


static void
transition_save(Transition *transition, ObjectNode obj_node,
		const char *filename)
{
  element_save(&transition->element, obj_node);

  save_boolequation(obj_node,"receptivity",transition->receptivity);
  save_font(obj_node,"rcep_font",transition->rcep_font);
  save_real(obj_node,"rcep_fontheight",transition->rcep_fontheight);
  save_color(obj_node,"rcep_color",&transition->rcep_color);

  save_point(obj_node,"north_pos",&transition->north.pos);
  save_point(obj_node,"south_pos",&transition->south.pos);

}

static Object *
transition_load(ObjectNode obj_node, int version, const char *filename)
{
  Transition *transition;
  Element *elem;
  Object *obj;
  int i;
  Point pos = { -65536.0, 0.0 };
  
  init_default_values();
  transition = g_malloc0(sizeof(Transition));

  elem = &transition->element;
  obj = (Object *) transition;
  
  obj->type = &transition_type;
  obj->ops = &transition_ops;

  element_load(elem, obj_node);
  element_init(elem, 10,2);

  transition->rcep_font = load_font(obj_node,"rcep_font",defaults.font);
  transition->rcep_fontheight = load_real(obj_node,"rcep_fontheight",
					defaults.font_size);
  load_color(obj_node,"rcep_color",&transition->rcep_color,
	     &defaults.font_color);

  transition->receptivity = load_boolequation(obj_node,"receptivity",NULL,
					     transition->rcep_font,
					     transition->rcep_fontheight,
					     &transition->rcep_color);

  transition->rcep_value = g_strdup(transition->receptivity->value);

  transition->north.connect_type = HANDLE_CONNECTABLE;
  transition->north.type = HANDLE_MAJOR_CONTROL;
  transition->north.id = HANDLE_NORTH;
  load_point(obj_node,"north_pos",&transition->north.pos,&pos);

  transition->south.connect_type = HANDLE_CONNECTABLE;
  transition->south.type = HANDLE_MAJOR_CONTROL;
  transition->south.id = HANDLE_SOUTH;
  load_point(obj_node,"south_pos",&transition->south.pos,&pos);


  for (i=0;i<2;i++) {
    obj->connections[i] = &transition->connections[i];
    obj->connections[i]->object = obj;
    obj->connections[i]->connected = NULL;
  }

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &transition->north;
  obj->handles[9] = &transition->south;

  transition_update_data(transition);

  return (Object *)transition;
}









