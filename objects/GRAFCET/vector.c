/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET charts support for Dia 
 * Copyright (C) 2000 Cyrille Chepelov
 *
 * This file is derived from objects/standard/zigzag.c
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
#include "orth_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"
#include "lazyprops.h"

#include "grafcet.h"
#include "pixmaps/vector.xpm"

#define VECTOR_LINE_WIDTH (GRAFCET_GENERAL_LINE_WIDTH)
#define VECTOR_ARROW_LENGTH .8
#define VECTOR_ARROW_WIDTH .6
#define VECTOR_ARROW_TYPE ARROW_FILLED_TRIANGLE

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _VectorPropertiesDialog VectorPropertiesDialog;
typedef struct _VectorDefaultsDialog VectorDefaultsDialog;
typedef struct _VectorDefaults VectorDefaults;
typedef struct _VectorState VectorState;

struct _VectorState {
  ObjectState obj_state;

  gboolean uparrow;
};

typedef struct _Vector {
  OrthConn orth;

  gboolean uparrow;
} Vector;

struct _VectorPropertiesDialog {
  AttributeDialog dialog;
  Vector *parent;

  BoolAttribute uparrow;
};

struct _VectorDefaults {
  gboolean uparrow;
};

struct _VectorDefaultsDialog {
  AttributeDialog dialog;
  VectorDefaults *parent;

  BoolAttribute uparrow;
};


static VectorPropertiesDialog *vector_properties_dialog;
static VectorDefaultsDialog *vector_defaults_dialog;
static VectorDefaults defaults; 

static void vector_move_handle(Vector *vector, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void vector_move(Vector *vector, Point *to);
static void vector_select(Vector *vector, Point *clicked_point,
			      Renderer *interactive_renderer);
static void vector_draw(Vector *vector, Renderer *renderer);
static Object *vector_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real vector_distance_from(Vector *vector, Point *point);
static void vector_update_data(Vector *vector);
static void vector_destroy(Vector *vector);
static Object *vector_copy(Vector *vector);
static PROPDLG_TYPE vector_get_properties(Vector *vector);
static ObjectChange *vector_apply_properties(Vector *vector);
static DiaMenu *vector_get_object_menu(Vector *vector,
					   Point *clickedpoint);

static VectorState *vector_get_state(Vector *vector);
static void vector_set_state(Vector *vector, VectorState *state);

static void vector_save(Vector *vector, ObjectNode obj_node,
			    const char *filename);
static Object *vector_load(ObjectNode obj_node, int version,
			       const char *filename);
static PROPDLG_TYPE vector_get_defaults(void);
static void vector_apply_defaults(void); 

static ObjectTypeOps vector_type_ops =
{
  (CreateFunc)vector_create,   /* create */
  (LoadFunc)  vector_load,     /* load */
  (SaveFunc)  vector_save,      /* save */
  (GetDefaultsFunc)   vector_get_defaults,
  (ApplyDefaultsFunc) vector_apply_defaults
};

ObjectType vector_type =
{
  "GRAFCET - Vector",   /* name */
  0,                         /* version */
  (char **) vector_xpm,      /* pixmap */
  
  &vector_type_ops       /* ops */
};

static ObjectOps vector_ops = {
  (DestroyFunc)         vector_destroy,
  (DrawFunc)            vector_draw,
  (DistanceFunc)        vector_distance_from,
  (SelectFunc)          vector_select,
  (CopyFunc)            vector_copy,
  (MoveFunc)            vector_move,
  (MoveHandleFunc)      vector_move_handle,
  (GetPropertiesFunc)   vector_get_properties,
  (ApplyPropertiesFunc) vector_apply_properties,
  (ObjectMenuFunc)      vector_get_object_menu
};

static ObjectChange *
vector_apply_properties(Vector *vector)
{
  ObjectState *old_state;
  VectorPropertiesDialog *dlg = vector_properties_dialog;
  
  PROPDLG_SANITY_CHECK(dlg,vector);
  
  old_state = (ObjectState *)vector_get_state(vector);

  PROPDLG_APPLY_BOOL(dlg,uparrow);

  vector_update_data(vector);
  return new_object_state_change(&vector->orth.object, old_state, 
				 (GetStateFunc)vector_get_state,
				 (SetStateFunc)vector_set_state);
}

static PROPDLG_TYPE
vector_get_properties(Vector *vector)
{
  VectorPropertiesDialog *dlg = vector_properties_dialog;
  
  PROPDLG_CREATE(dlg,vector);
  PROPDLG_SHOW_BOOL(dlg,uparrow,_("Draw arrow heads on upward arcs:"));
  PROPDLG_READY(dlg);

  vector_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void
vector_init_defaults(void) {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    defaults.uparrow = TRUE;
    defaults_initialized = 1;
  }
} 

static void
vector_apply_defaults(void)
{
  VectorDefaultsDialog *dlg = vector_defaults_dialog;  

  PROPDLG_APPLY_BOOL(dlg,uparrow);  
}


static PROPDLG_TYPE
vector_get_defaults()
{
  VectorDefaultsDialog *dlg = vector_defaults_dialog;
  vector_init_defaults();

  PROPDLG_CREATE(dlg, &defaults);
  PROPDLG_SHOW_BOOL(dlg,uparrow,_("Draw arrow heads on upward arcs:"));
  PROPDLG_READY(dlg);

  vector_defaults_dialog = dlg;
  PROPDLG_RETURN(dlg);
}

static real
vector_distance_from(Vector *vector, Point *point)
{
  OrthConn *orth = &vector->orth;
  return orthconn_distance_from(orth, point, VECTOR_LINE_WIDTH);
}

static void
vector_select(Vector *vector, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&vector->orth);
}

static void
vector_move_handle(Vector *vector, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(vector!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  orthconn_move_handle(&vector->orth, handle, to, reason);
  vector_update_data(vector);
}


static void
vector_move(Vector *vector, Point *to)
{
  orthconn_move(&vector->orth, to);
  vector_update_data(vector);
}

static void
vector_draw(Vector *vector, Renderer *renderer)
{
  OrthConn *orth = &vector->orth;
  Point *points;
  int n,i;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, VECTOR_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  if (vector->uparrow) {
    for (i=0;i<n-1; i++) {
      if ((points[i].y > points[i+1].y) &&
	  (ABS(points[i+1].y-points[i].y) > 5 * VECTOR_ARROW_LENGTH)) {
	Point m;
	m.x = points[i].x; /* == points[i+1].x */
	m.y = .5 * (points[i].y + points[i+1].y) - (.5 * VECTOR_ARROW_LENGTH);
	arrow_draw(renderer, VECTOR_ARROW_TYPE,
		   &m,&points[i],
		   VECTOR_ARROW_LENGTH, VECTOR_ARROW_WIDTH,
		   VECTOR_LINE_WIDTH,
		   &color_black, &color_white);
      }
    }
  }
}

static Object *
vector_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Vector *vector;
  OrthConn *orth;
  Object *obj;

  vector_init_defaults();
  vector = g_malloc(sizeof(Vector));
  orth = &vector->orth;
  obj = &orth->object;
  
  obj->type = &vector_type;
  obj->ops = &vector_ops;
  
  orthconn_init(orth, startpoint);
  

  vector->uparrow = defaults.uparrow;
  vector_update_data(vector);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &vector->orth.object;
}

static void
vector_destroy(Vector *vector)
{
  orthconn_destroy(&vector->orth);
}

static Object *
vector_copy(Vector *vector)
{
  Vector *newvector;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &vector->orth;
 
  newvector = g_malloc(sizeof(Vector));
  neworth = &newvector->orth;
  newobj = &neworth->object;

  orthconn_copy(orth, neworth);

  newvector->uparrow = vector->uparrow;

  return &newvector->orth.object;
}

static VectorState *
vector_get_state(Vector *vector)
{
  VectorState *state = g_new(VectorState, 1);

  state->obj_state.free = NULL;
  
  state->uparrow = vector->uparrow;

  return state;
}

static void
vector_set_state(Vector *vector, VectorState *state)
{
  vector->uparrow = state->uparrow;

  g_free(state);
  
  vector_update_data(vector);
}

static void
vector_update_data(Vector *vector)
{
  OrthConn *orth = &vector->orth;
  OrthConnBBExtras *extra = &orth->extra_spacing;

  orthconn_update_data(&vector->orth);
  
  extra->start_trans = 
    extra->start_long =
    extra->end_long  = 
    extra->end_trans = VECTOR_LINE_WIDTH/2.0;
  if (vector->uparrow) {
    extra->middle_trans = (VECTOR_LINE_WIDTH + VECTOR_ARROW_WIDTH)/2.0;
  } else {
    extra->middle_trans = VECTOR_LINE_WIDTH/2.0;
  }
    
  orthconn_update_boundingbox(orth);
}


static ObjectChange *
vector_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  vector_update_data((Vector *)obj);
  return change;
}

static ObjectChange *
vector_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  vector_update_data((Vector *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), vector_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), vector_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Vector",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
vector_get_object_menu(Vector *vector, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &vector->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}


static void
vector_save(Vector *vector, ObjectNode obj_node,
		const char *filename)
{
  orthconn_save(&vector->orth, obj_node);

  save_boolean(obj_node,"uparrow",vector->uparrow);
}

static Object *
vector_load(ObjectNode obj_node, int version, const char *filename)
{
  Vector *vector;
  OrthConn *orth;
  Object *obj;

  vector_init_defaults();

  vector = g_malloc(sizeof(Vector));

  orth = &vector->orth;
  obj = &orth->object;
  
  obj->type = &vector_type;
  obj->ops = &vector_ops;

  orthconn_load(orth, obj_node);

  vector->uparrow = load_boolean(obj_node,"uparrow",TRUE);

  vector_update_data(vector);

  return &vector->orth.object;
}
