/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET charts support for Dia 
 * Copyright (C) 2000 Cyrille Chepelov
 *
 * This file has been forked from objects/ER/entity.c
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
#include <gtk/gtk.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "lazyprops.h"

#include "grafcet.h"

#include "pixmaps/etape.xpm"

#define STEP_FONT "Helvetica-Bold"
#define STEP_FONT_HEIGHT 1
#define STEP_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH 
#define STEP_WIDTH 3.0
#define STEP_DECLAREDWIDTH 4.0
#define STEP_HEIGHT 4.0
#define STEP_DOT_RADIUS .35

#define HANDLE_NORTH HANDLE_CUSTOM1
#define HANDLE_SOUTH HANDLE_CUSTOM2


typedef enum {
  STEP_NORMAL,
  STEP_INITIAL,
  STEP_MACROENTRY,
  STEP_MACROEXIT,
  STEP_MACROCALL,
  STEP_SUBPCALL} StepType;

typedef struct _Step Step;
typedef struct _StepPropertiesDialog StepPropertiesDialog;
typedef struct _StepDefaults StepDefaults;
typedef struct _StepDefaultsDialog StepDefaultsDialog;
typedef struct _StepState StepState;

struct _StepState {
  ObjectState obj_state;

  char *id;
  int active;
  StepType type;

  Font *font;
  real font_size;
  Color font_color;
};  

struct _Step {
  Element element;

  ConnectionPoint connections[4];

  char *id;
  int active;
  StepType type;

  Font *font;
  real font_size;
  Color font_color;

  Handle north,south;
  Point SD1,SD2,NU1,NU2;

  /* These are useful points for drawing. 
     Must be in sequence, A first, Z last. */
  Point A,B,C,D,E,F,G,H,I,J,Z;
};

struct _StepPropertiesDialog {
  AttributeDialog dialog;
  Step *parent;

  StringAttribute id;
  BoolAttribute active;
  EnumAttribute type;

  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

struct _StepDefaults {
  Font *font;
  real font_size;
  Color font_color;
};

struct _StepDefaultsDialog {
  AttributeDialog dialog;
  StepDefaults *parent;

  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

static real step_distance_from(Step *step, Point *point);
static void step_select(Step *step, Point *clicked_point,
			Renderer *interactive_renderer);
static void step_move_handle(Step *step, Handle *handle,
			    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void step_move(Step *step, Point *to);
static void step_draw(Step *step, Renderer *renderer);
static void step_update_data(Step *step);
static Object *step_create(Point *startpoint,
			     void *user_data,
			     Handle **handle1,
			     Handle **handle2);
static void step_destroy(Step *step);
static Object *step_copy(Step *step);
static PROPDLG_TYPE step_get_properties(Step *step);
static ObjectChange *step_apply_properties(Step *step);

static void step_save(Step *step, ObjectNode obj_node,
			const char *filename);
static Object *step_load(ObjectNode obj_node, int version,
			   const char *filename);
static StepState *step_get_state(Step *step);
static void step_set_state(Step *step, StepState *state);
static PROPDLG_TYPE step_get_defaults(void);
static void step_apply_defaults(void);
static void step_init_defaults(void);



static ObjectTypeOps step_type_ops =
{
  (CreateFunc) step_create,
  (LoadFunc)   step_load,
  (SaveFunc)   step_save,
  (GetDefaultsFunc)   step_get_defaults, 
  (ApplyDefaultsFunc) step_apply_defaults
};

ObjectType step_type =
{
  "GRAFCET - Step",  /* name */
  0,                 /* version */
  (char **) etape_xpm, /* pixmap */

  &step_type_ops      /* ops */
};

static ObjectOps step_ops = {
  (DestroyFunc)         step_destroy,
  (DrawFunc)            step_draw,
  (DistanceFunc)        step_distance_from,
  (SelectFunc)          step_select,
  (CopyFunc)            step_copy,
  (MoveFunc)            step_move,
  (MoveHandleFunc)      step_move_handle,
  (GetPropertiesFunc)   step_get_properties,
  (ApplyPropertiesFunc) step_apply_properties,
  (ObjectMenuFunc)      NULL
};

static StepPropertiesDialog *step_properties_dialog = NULL;
static StepDefaultsDialog *step_defaults_dialog = NULL;
static StepDefaults defaults;


/* the following two functions try to be clever when allocating 
   step numbers */

static int __stepnum = 0;
static int __Astyle = 0;
static gchar *new_step_name() 
{
  char snum[16];
  char *p = snum;
  
  if (__Astyle) *p++ = 'A';
  
  g_snprintf(p,sizeof(snum)-2,"%d",__stepnum++);
  return g_strdup(snum);
}

static void step_been_renamed(const gchar *sid) 
{
  gchar *endptr;
  long int snum;
  if (!sid) return;
  if (*sid == 'A') {
    sid++; /* for the "A01" numbering style */
    __Astyle = 1;
  } else {
    __Astyle = 0;
  }
  endptr = NULL;
  snum = strtol(sid,&endptr,10);
  if (*endptr == '\0') __stepnum = snum + 1; 
}

static ObjectChange *
step_apply_properties(Step *step)
{
  ObjectState *old_state;
  StepPropertiesDialog *dlg = step_properties_dialog;
  
  PROPDLG_SANITY_CHECK(dlg,step);

  old_state = (ObjectState *)step_get_state(step);

  PROPDLG_APPLY_STRING(dlg,id);
  PROPDLG_APPLY_BOOL(dlg,active);
  PROPDLG_APPLY_ENUM(dlg,type);
  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);

  step_been_renamed(step->id);
  step_update_data(step);
  return new_object_state_change(&step->element.object, 
                                 old_state,
				 (GetStateFunc)step_get_state,
				 (SetStateFunc)step_set_state);
}

PropDlgEnumEntry step_style[] = {
  { N_("Regular step"),STEP_NORMAL,NULL },
  { N_("Initial step"),STEP_INITIAL,NULL },
  { N_("Macro entry step"),STEP_MACROENTRY, NULL },
  { N_("Macro exit step"),STEP_MACROEXIT, NULL },
  { N_("Macro call step"),STEP_MACROCALL, NULL },
  { N_("Subprogram call step"), STEP_SUBPCALL, NULL },
  { NULL}};

static PROPDLG_TYPE
step_get_properties(Step *step)
{
  StepPropertiesDialog *dlg = step_properties_dialog;

  PROPDLG_CREATE(dlg,step);
  PROPDLG_SHOW_STRING(dlg,id,_("Step name"));
  PROPDLG_SHOW_BOOL(dlg,active,_("Active"));
  PROPDLG_SHOW_ENUM(dlg,type,_("Step type"),step_style);
  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Text color:"));
  PROPDLG_READY(dlg);
  step_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static Color color_red = { 1.0f, 0.0f, 0.0f };

static void 
step_init_defaults(void)
{
  static int defaults_initialised = 0;
  if (!defaults_initialised) {
    defaults.font = font_getfont(STEP_FONT);
    defaults.font_size = STEP_FONT_HEIGHT;
    defaults.font_color = color_black;
    defaults_initialised = 1;
  }
}

static void 
step_apply_defaults(void)
{
  StepDefaultsDialog *dlg = step_defaults_dialog;  

  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);
}

static PROPDLG_TYPE
step_get_defaults(void)
{
  StepDefaultsDialog *dlg = step_defaults_dialog;

  step_init_defaults();
  PROPDLG_CREATE(dlg, &defaults);
  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Color:"));
  PROPDLG_READY(dlg);
  step_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}



static real
step_distance_from(Step *step, Point *point)
{
  Element *elem = &step->element;
  Rectangle rect;
  real dist;

  dist = distance_line_point(&step->north.pos,&step->NU1,
			     STEP_LINE_WIDTH,point);
  dist = MIN(dist,distance_line_point(&step->NU1,&step->NU2,
				      STEP_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&step->NU2,&step->A,
				      STEP_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&step->D,&step->SD1,
				      STEP_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&step->SD1,&step->SD2,
				      STEP_LINE_WIDTH,point));
  dist = MIN(dist,distance_line_point(&step->SD2,&step->south.pos,
				      STEP_LINE_WIDTH,point));

  rect.left = elem->corner.x;
  rect.right = elem->corner.x + elem->width;
  rect.top = elem->corner.y;
  rect.bottom = elem->corner.y + elem->height;
  dist = MIN(dist,distance_rectangle_point(&rect, point));
  return dist;
}

static void
step_select(Step *step, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  element_update_handles(&step->element);
}

static void
step_move_handle(Step *step, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(step!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  switch(handle->id) {
  case HANDLE_NORTH:
    step->north.pos = *to;
    if (step->north.pos.y > step->A.y) step->north.pos.y = step->A.y;
    break;
  case HANDLE_SOUTH:
    step->south.pos = *to;
    if (step->south.pos.y < step->D.y) step->south.pos.y = step->D.y;
    break;
  default:
    element_move_handle(&step->element, handle->id, to, reason);
  }

  step_update_data(step);
}

static void
step_move(Step *step, Point *to)
{
  Point delta = *to;
  point_sub(&delta,&step->element.corner);
  step->element.corner = *to;
  point_add(&step->north.pos,&delta);
  point_add(&step->south.pos,&delta);
  
  step_update_data(step);
}


static void
step_draw(Step *step, Renderer *renderer)
{
  Point pts[4];
  assert(step != NULL);
  assert(renderer != NULL);

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, STEP_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  pts[0] = step->north.pos;
  pts[1] = step->NU1;
  pts[2] = step->NU2;
  pts[3] = step->A;
  renderer->ops->draw_polyline(renderer,pts,sizeof(pts)/sizeof(pts[0]),
			       &color_black);
  pts[0] = step->D;
  pts[1] = step->SD1;
  pts[2] = step->SD2;
  pts[3] = step->south.pos;
  renderer->ops->draw_polyline(renderer,pts,sizeof(pts)/sizeof(pts[0]),
			       &color_black);

  if ((step->type == STEP_INITIAL) ||
      (step->type == STEP_MACROCALL) ||
      (step->type == STEP_SUBPCALL)) {
    renderer->ops->fill_rect(renderer, &step->I, &step->J, &color_white);
    renderer->ops->draw_rect(renderer, &step->I, &step->J, &color_black);
  } else {
    renderer->ops->fill_rect(renderer, &step->E, &step->F, &color_white);
  }
  renderer->ops->draw_rect(renderer, &step->E, &step->F, &color_black);
  
  if (step->type != STEP_MACROENTRY)
    renderer->ops->draw_line(renderer,&step->A,&step->B,&color_black);
  if (step->type != STEP_MACROEXIT)
    renderer->ops->draw_line(renderer,&step->C,&step->D,&color_black);
  
  renderer->ops->set_font(renderer, step->font, step->font_size);

  renderer->ops->draw_string(renderer, 
			     step->id, 
			     &step->G, ALIGN_CENTER, 
			     &step->font_color);
  if (step->active) 
    renderer->ops->fill_ellipse(renderer,
			       &step->H,
			       STEP_DOT_RADIUS,STEP_DOT_RADIUS,
			       &color_red);
}

static void
step_update_data(Step *step)
{
  Element *elem = &step->element;
  Object *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;
  Point *p,ulc;

  ulc = elem->corner;
  ulc.x += ((STEP_DECLAREDWIDTH - STEP_WIDTH) / 2.0); /* we cheat a little */

  step->A.x = 0.0 + (STEP_WIDTH / 2.0); step->A.y = 0.0;
  step->D.x = 0.0 + (STEP_WIDTH / 2.0); step->D.y = 0.0 + STEP_HEIGHT;

  step->E.x = 0.0; step->E.y = 0.5;
  step->F.x = STEP_WIDTH; step->F.y = STEP_HEIGHT- 0.5;

  
  switch(step->type) {
  case STEP_INITIAL:
    step->I.x = step->E.x - 2 * STEP_LINE_WIDTH; 
    step->I.y = step->E.y - 2 * STEP_LINE_WIDTH;
    step->J.x = step->F.x + 2 * STEP_LINE_WIDTH; 
    step->J.y = step->F.y + 2 * STEP_LINE_WIDTH;

    step->B.x = step->A.x; step->B.y = step->I.y;
    step->C.x = step->D.x; step->C.y = step->J.y;
    step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
    break;
  case STEP_MACROCALL:
    step->I.x = step->E.x; 
    step->I.y = step->E.y - 2 * STEP_LINE_WIDTH;
    step->J.x = step->F.x; 
    step->J.y = step->F.y + 2 * STEP_LINE_WIDTH;

    step->B.x = step->A.x; step->B.y = step->I.y;
    step->C.x = step->D.x; step->C.y = step->J.y;
    step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
    break;
  case STEP_SUBPCALL:
    step->I.x = step->E.x - 2 * STEP_LINE_WIDTH; 
    step->I.y = step->E.y;
    step->J.x = step->F.x + 2 * STEP_LINE_WIDTH; 
    step->J.y = step->F.y;

    step->B.x = step->A.x; step->B.y = step->I.y;
    step->C.x = step->D.x; step->C.y = step->J.y;
    step->Z.x = step->J.x; step->Z.y = STEP_HEIGHT / 2;
    break;
  default: /* regular or macro end steps */
    step->B.x = step->A.x; step->B.y = step->E.y;
    step->C.x = step->D.x; step->C.y = step->F.y;
    step->Z.x = step->F.x; step->Z.y = STEP_HEIGHT / 2;
  }
  
  step->G.x = step->A.x;
  step->G.y = (STEP_HEIGHT / 2)  + (.3 * step->font_size);
  step->H.x = step->E.x + (1.2 * STEP_DOT_RADIUS);
  step->H.y = step->F.y - (1.2 * STEP_DOT_RADIUS);
    
  for (p=&(step->A); p<=&(step->Z) ; p++)
    point_add(p,&ulc);

  /* Update handles: */
  if (step->north.pos.x == -65536.0) {
    step->north.pos = step->A;
    step->south.pos = step->D;
  }
  step->NU1.x = step->north.pos.x;
  step->NU2.x = step->A.x;
  step->NU1.y = step->NU2.y = (step->north.pos.y + step->A.y) / 2.0;
  step->SD1.x = step->D.x;
  step->SD2.x = step->south.pos.x;
  step->SD1.y = step->SD2.y = (step->south.pos.y + step->D.y) / 2.0;

  /* Update connections: */
#if 0  
  /* Not really a good idea */
  step->connections[0].pos = step->north.pos;
  step->connections[1].pos = step->south.pos;
#else
  step->connections[0].pos = step->A;
  step->connections[1].pos = step->D;
#endif
  step->connections[2].pos = step->Z;
  step->connections[3].pos = step->H;

  /* recalc the bounding box : */
  if ((step->type == STEP_INITIAL) || (step->type == STEP_SUBPCALL)) {
    extra->border_trans = 2.5 * STEP_LINE_WIDTH;
  } else {
    extra->border_trans = STEP_LINE_WIDTH / 2;
  }
  
  element_update_boundingbox(elem);
  rectangle_add_point(&obj->bounding_box,&step->north.pos);
  rectangle_add_point(&obj->bounding_box,&step->south.pos);

  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static StepState *
step_get_state(Step *step)
{
  StepState *state = g_new(StepState, 1);
  state->obj_state.free = NULL;

  state->id = g_strdup(step->id);
  state->type = step->type;
  state->active = step->active;
  state->font = step->font;
  state->font_size = step->font_size;
  state->font_color = step->font_color;

  return state;
}

static void 
step_set_state(Step *step, StepState *state)
{
  if (step->id) g_free(step->id);
  step->id = state->id;
  step->active = state->active;
  step->type = state->type;
  step->font = state->font;
  step->font_size = state->font_size;
  step->font_color = state->font_color;

  g_free(state);

  step_update_data(step);
}

static Object *
step_create(Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  Step *step;
  Element *elem;
  Object *obj;
  int i;
  int type;
  
  step_init_defaults();

  step = g_new0(Step,1);
  elem = &step->element;
  obj = &elem->object;
  
  obj->type = &step_type;
  obj->ops = &step_ops;

  elem->corner = *startpoint;
  elem->width = STEP_DECLAREDWIDTH;
  elem->height = STEP_HEIGHT;

  element_init(elem, 10, 4);

  for (i=0;i<4;i++) {
    obj->connections[i] = &step->connections[i];
    step->connections[i].object = obj;
    step->connections[i].connected = NULL;
  }

  step->id = new_step_name();
  step->active = 0;
  step->font = defaults.font;
  step->font_size = defaults.font_size;
  step->font_color = defaults.font_color;
  
  type = GPOINTER_TO_INT(user_data);
  switch(type) {
  case STEP_NORMAL:
  case STEP_INITIAL:
  case STEP_MACROENTRY:
  case STEP_MACROEXIT:
  case STEP_MACROCALL:
  case STEP_SUBPCALL:
    step->type = type;
    break;
  default:
    step->type = STEP_NORMAL;
  }
  

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &step->north;
  obj->handles[9] = &step->south;
  step->north.connect_type = HANDLE_CONNECTABLE;
  step->north.type = HANDLE_MAJOR_CONTROL;
  step->north.id = HANDLE_NORTH;
  step->south.connect_type = HANDLE_CONNECTABLE;
  step->south.type = HANDLE_MAJOR_CONTROL;
  step->south.id = HANDLE_SOUTH;
  step->north.pos.x = -65536.0; /* magic */

  step_update_data(step);

  *handle1 = NULL;
  *handle2 = obj->handles[0];  
  return &step->element.object;
}

static void
step_destroy(Step *step)
{
  element_destroy(&step->element);
  g_free(step->id);
}

static Object *
step_copy(Step *step)
{
  int i;
  Step *newstep;
  Element *elem, *newelem;
  Object *newobj;

  elem = &step->element;
  
  newstep = g_new0(Step,1);
  newelem = &newstep->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  for (i=0;i<newobj->num_connections;i++) {
    newobj->connections[i] = &newstep->connections[i];
    newstep->connections[i].object = newobj;
    newstep->connections[i].connected = NULL;
    newstep->connections[i].pos = step->connections[i].pos;
    newstep->connections[i].last_pos = step->connections[i].last_pos;
  }
  newobj->handles[8] = &newstep->north;
  newobj->handles[9] = &newstep->south;
  newstep->north.connect_type = HANDLE_CONNECTABLE;
  newstep->north.type = HANDLE_MAJOR_CONTROL;
  newstep->north.id = HANDLE_NORTH;
  newstep->south.connect_type = HANDLE_CONNECTABLE;
  newstep->south.type = HANDLE_MAJOR_CONTROL;
  newstep->south.id = HANDLE_SOUTH;
  newstep->north.pos = newstep->A;
  newstep->south.pos = newstep->B;

  newstep->id = strdup(step->id);
  newstep->active = step->active;
  newstep->type = step->type;
  newstep->font = step->font;
  newstep->font_size = step->font_size;
  newstep->font_color = step->font_color;

  step_update_data(newstep);

  return &newstep->element.object;
}

static void
step_save(Step *step, ObjectNode obj_node, const char *filename)
{
  element_save(&step->element, obj_node);

  save_enum(obj_node,"type",step->type);
  save_string(obj_node,"id",step->id);
  save_boolean(obj_node,"active",step->active);
  save_font(obj_node,"font",step->font);
  save_real(obj_node,"font_size",step->font_size);
  save_color(obj_node,"font_color",&step->font_color);
  save_point(obj_node,"north_pos",&step->north.pos);
  save_point(obj_node,"south_pos",&step->south.pos);
}

static Object *
step_load(ObjectNode obj_node, int version, const char *filename)
{
  Step *step;
  Element *elem;
  Object *obj;
  int i;
  Point pos = { -65536.0, 0.0 };

  step_init_defaults();

  step = g_new0(Step,1);
  elem = &step->element;
  obj = &elem->object;
  
  obj->type = &step_type;
  obj->ops = &step_ops;

  element_load(elem, obj_node);
  
  step->id = load_string(obj_node,"id",NULL);
  if (!step->id) step->id = new_step_name();
  step->type = load_enum(obj_node,"type",STEP_NORMAL);
  step->active = load_boolean(obj_node,"active",0);
  step->font = load_font(obj_node,"font",defaults.font);
  step->font_size = load_real(obj_node,"font_size",defaults.font_size);
  load_color(obj_node,"font_color",&step->font_color,&defaults.font_color);
  
  element_init(elem, 10, 4);

  for (i=0;i<4;i++) {
    obj->connections[i] = &step->connections[i];
    step->connections[i].object = obj;
    step->connections[i].connected = NULL;
  }

  step->north.connect_type = HANDLE_CONNECTABLE;
  step->north.type = HANDLE_MAJOR_CONTROL;
  step->north.id = HANDLE_NORTH;
  load_point(obj_node,"north_pos",&step->north.pos,&pos);

  step->south.connect_type = HANDLE_CONNECTABLE;
  step->south.type = HANDLE_MAJOR_CONTROL;
  step->south.id = HANDLE_SOUTH;
  load_point(obj_node,"south_pos",&step->south.pos,&pos);

  step_update_data(step);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }
  obj->handles[8] = &step->north;
  obj->handles[9] = &step->south;

  return &step->element.object;
}





