/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * GRAFCET chart support 
 * Copyright (C) 2000 Cyrille Chepelov
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

#include "intl.h"
#include "object.h"
#include "connection.h"
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

#include "pixmaps/condition.xpm"

#define CONDITION_LINE_WIDTH GRAFCET_GENERAL_LINE_WIDTH
#define CONDITION_FONT "Helvetica-Bold"
#define CONDITION_FONT_HEIGHT 0.8
#define CONDITION_LENGTH (1.5)
#define CONDITION_ARROW_SIZE 0.0 /* XXX The norm says there's no arrow head.
                                    a lot of people do put it, though. */

typedef struct _ConditionPropertiesDialog ConditionPropertiesDialog;
typedef struct _ConditionDefaultsDialog ConditionDefaultsDialog;
typedef struct _ConditionState ConditionState;

struct _ConditionState {
  ObjectState obj_state;
  
  Font *cond_font;
  real cond_fontheight;
  Color cond_color;

  gchar *cond_value;
};

typedef struct _Condition {
  Connection connection;
  
  Boolequation *cond;

  gchar *cond_value;
  Font *cond_font;
  real cond_fontheight;
  Color cond_color;

  /* computed values : */
  Rectangle labelbb;
} Condition;

struct _ConditionPropertiesDialog {
  AttributeDialog dialog;
  Condition *parent;

  StringAttribute cond_value;
  FontAttribute cond_font;
  FontHeightAttribute cond_fontheight;
  TextColorAttribute cond_color;
};

typedef struct _ConditionDefaults {
  Font *font;
  real font_size;
  Color font_color;
} ConditionDefaults;

struct _ConditionDefaultsDialog {
  AttributeDialog dialog;
  ConditionDefaults *parent;

  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

static ConditionPropertiesDialog *condition_properties_dialog;
static ConditionDefaultsDialog *condition_defaults_dialog;
static ConditionDefaults defaults;

static void condition_move_handle(Condition *condition, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void condition_move(Condition *condition, Point *to);
static void condition_select(Condition *condition, Point *clicked_point,
			      Renderer *interactive_renderer);
static void condition_draw(Condition *condition, Renderer *renderer);
static Object *condition_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real condition_distance_from(Condition *condition, Point *point);
static void condition_update_data(Condition *condition);
static void condition_destroy(Condition *condition);
static Object *condition_copy(Condition *condition);
static GtkWidget *condition_get_properties(Condition *condition);
static ObjectChange *condition_apply_properties(Condition *condition);

static ConditionState *condition_get_state(Condition *condition);
static void condition_set_state(Condition *condition, ConditionState *state);

static void condition_save(Condition *condition, ObjectNode obj_node,
			    const char *filename);
static Object *condition_load(ObjectNode obj_node, int version,
			       const char *filename);

static GtkWidget *condition_get_defaults(void);
static void condition_apply_defaults(void);

static ObjectTypeOps condition_type_ops =
{
  (CreateFunc)condition_create,   /* create */
  (LoadFunc)  condition_load,     /* load */
  (SaveFunc)  condition_save,      /* save */
  (GetDefaultsFunc)   condition_get_defaults, 
  (ApplyDefaultsFunc) condition_apply_defaults
};

ObjectType condition_type =
{
  "GRAFCET - Condition",   /* name */
  0,                         /* version */
  (char **) condition_xpm,      /* pixmap */
  
  &condition_type_ops       /* ops */
};


static ObjectOps condition_ops = {
  (DestroyFunc)         condition_destroy,
  (DrawFunc)            condition_draw,
  (DistanceFunc)        condition_distance_from,
  (SelectFunc)          condition_select,
  (CopyFunc)            condition_copy,
  (MoveFunc)            condition_move,
  (MoveHandleFunc)      condition_move_handle,
  (GetPropertiesFunc)   condition_get_properties,
  (ApplyPropertiesFunc) condition_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
condition_apply_properties(Condition *condition)
{
  ObjectState *old_state;
  ConditionPropertiesDialog *dlg = condition_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,condition);
  
  old_state = (ObjectState *)condition_get_state(condition);

  PROPDLG_APPLY_FONT(dlg,cond_font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,cond_fontheight);
  PROPDLG_APPLY_COLOR(dlg,cond_color);
  PROPDLG_APPLY_STRING(dlg,cond_value);
  boolequation_set_value(condition->cond,condition->cond_value);
  condition->cond->font = condition->cond_font;
  condition->cond->fontheight = condition->cond_fontheight;
  condition->cond->color = condition->cond_color;
  
  condition_update_data(condition);
  return new_object_state_change(&condition->connection.object, old_state, 
				 (GetStateFunc)condition_get_state,
				 (SetStateFunc)condition_set_state);
}

static PROPDLG_TYPE
condition_get_properties(Condition *condition)
{
  ConditionPropertiesDialog *dlg = condition_properties_dialog;
  
  PROPDLG_CREATE(dlg,condition);
  PROPDLG_SHOW_STRING(dlg,cond_value,_("Condition:"));
  PROPDLG_SHOW_SEPARATOR(dlg);
  PROPDLG_SHOW_FONT(dlg,cond_font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,cond_fontheight,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,cond_color,_("Text color:"));
  PROPDLG_READY(dlg);
  
  condition_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void 
condition_apply_defaults(void)
{
  ConditionDefaultsDialog *dlg = condition_defaults_dialog;  

  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);


}

static void
init_default_values(void) {
  static int defaults_initialised = 0;
  
  if (!defaults_initialised) {
    defaults.font = font_getfont(CONDITION_FONT);
    defaults.font_size = CONDITION_FONT_HEIGHT;
    defaults.font_color = color_black;

    defaults_initialised = 1;
  }
}

static PROPDLG_TYPE
condition_get_defaults(void)
{
  ConditionDefaultsDialog *dlg = condition_defaults_dialog;
  init_default_values();
  PROPDLG_CREATE(dlg, &defaults);

  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Text color:"));
  PROPDLG_READY(dlg);

  condition_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}


static real
condition_distance_from(Condition *condition, Point *point)
{
  Connection *conn = &condition->connection;
  real dist; 
  dist = distance_rectangle_point(&condition->labelbb,point);
  dist = MIN(dist,distance_line_point(&conn->endpoints[0],
				      &conn->endpoints[1],
				      CONDITION_LINE_WIDTH,point));
  return dist;
}

static void
condition_select(Condition *condition, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  condition_update_data(condition);
}

static void
condition_move_handle(Condition *condition, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point s,e,v;
  int horiz;

  g_assert(condition!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);
  
  switch (handle->id) {
  case HANDLE_MOVE_STARTPOINT:
    point_copy(&s,to);
    point_copy(&e,&condition->connection.endpoints[1]);
    point_copy(&v,&e);
    point_sub(&v,&s);
    
    horiz = ABS(v.x) > ABS(v.y);
    if (horiz) {
      v.y = 0.0;
    } else {
      v.x = 0.0;
    }

    point_copy(&s,&e);
    point_sub(&s,&v);
    /* XXX: fix e to make it look good (what's good ?) V is a good hint ? */
    connection_move_handle(&condition->connection, HANDLE_MOVE_STARTPOINT,
			   &s,reason);    
    break;
  case HANDLE_MOVE_ENDPOINT:
    point_copy(&s,&condition->connection.endpoints[0]);
    point_copy(&e,&condition->connection.endpoints[1]);
    point_copy(&v,&e);
    point_sub(&v,&s);
    connection_move_handle(&condition->connection, HANDLE_MOVE_ENDPOINT,
			   to,reason);
    point_copy(&s,to);
    point_sub(&s,&v);
    connection_move_handle(&condition->connection, HANDLE_MOVE_STARTPOINT,
			   &s,reason);    
    break;
  default:
    g_assert_not_reached();
  }
  condition_update_data(condition);
}


static void
condition_move(Condition *condition, Point *to)
{
  Point start_to_end;
  Point *endpoints = &condition->connection.endpoints[0]; 

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);
  point_copy(&endpoints[0],to);
  point_copy(&endpoints[1],to);
  point_add(&endpoints[1], &start_to_end);

  condition_update_data(condition);
}

static void
condition_update_data(Condition *condition)
{
  Connection *conn = &condition->connection;
  Object *obj = &conn->object;

  obj->position = conn->endpoints[0];
  connection_update_boundingbox(conn);

  /* compute the label's width and bounding box */
  condition->cond->pos.x = conn->endpoints[0].x + 
    (.5 * font_string_width("a", condition->cond->font,
			    condition->cond->fontheight));
  condition->cond->pos.y = conn->endpoints[0].y + condition->cond->fontheight;
  
  boolequation_calc_boundingbox(condition->cond, &condition->labelbb);
  rectangle_union(&obj->bounding_box,&condition->labelbb);

  connection_update_handles(conn);
}


static void 
condition_draw(Condition *condition, Renderer *renderer)
{
  Connection *conn = &condition->connection;

  renderer->ops->set_linewidth(renderer, CONDITION_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &conn->endpoints[0],&conn->endpoints[1],
			   &color_black);
  if (CONDITION_ARROW_SIZE > (CONDITION_LINE_WIDTH/2.0)) {
    arrow_draw(renderer, ARROW_FILLED_TRIANGLE,
               &conn->endpoints[1],&conn->endpoints[0],
               CONDITION_ARROW_SIZE,CONDITION_ARROW_SIZE/2,
               CONDITION_LINE_WIDTH,
               &color_black,&color_white);
  }

  boolequation_draw(condition->cond,renderer);
}

static Object *
condition_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Condition *condition;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj;
  Point defaultlen  = {0.0,CONDITION_ARROW_SIZE}, pos;

  init_default_values();
  condition = g_malloc0(sizeof(Condition));
  conn = &condition->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &condition_type;
  obj->ops = &condition_ops;
  
  point_copy(&conn->endpoints[0],startpoint);
  point_copy(&conn->endpoints[1],startpoint);
  point_add(&conn->endpoints[1], &defaultlen);

  connection_init(conn, 2,0);

  pos = conn->endpoints[1];
  condition->cond = boolequation_create("",defaults.font,defaults.font_size,
				 &defaults.font_color);
  condition->cond_value = g_strdup("");
  condition->cond_font = defaults.font;
  condition->cond_fontheight = defaults.font_size;
  condition->cond_color = defaults.font_color;

  extra->start_trans = 
    extra->start_long = 
    extra->end_long = CONDITION_LINE_WIDTH/2.0;
  extra->end_trans = MAX(CONDITION_LINE_WIDTH,CONDITION_ARROW_SIZE) / 2.0;

  condition_update_data(condition);

  conn->endpoint_handles[0].connect_type = HANDLE_NONCONNECTABLE;
  *handle1 = &conn->endpoint_handles[0];
  *handle2 = &conn->endpoint_handles[1];

  return &condition->connection.object;
}

static void
condition_destroy(Condition *condition)
{
  boolequation_destroy(condition->cond);
  g_free(condition->cond_value);
  connection_destroy(&condition->connection);
}

static Object *
condition_copy(Condition *condition)
{
  Condition *newcondition;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &condition->connection;
 
  newcondition = g_malloc0(sizeof(Condition));
  newconn = &newcondition->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);
  
  newcondition->cond = boolequation_create(condition->cond_value,
					   condition->cond_font,
					   condition->cond_fontheight,
					   &condition->cond_color);
  newcondition->cond_value = g_strdup(condition->cond_value);
  newcondition->cond_font = condition->cond_font;
  newcondition->cond_fontheight = condition->cond_fontheight;
  newcondition->cond_color = condition->cond_color;

  return &newcondition->connection.object;
}

static ConditionState *
condition_get_state(Condition *condition)
{
  ConditionState *state = g_new0(ConditionState, 1);

  OBJECT_GET_STRING(condition,state,cond_value);
  OBJECT_GET_FONT(condition,state,cond_font);
  OBJECT_GET_COLOR(condition,state,cond_color);
  OBJECT_GET_FONTHEIGHT(condition,state,cond_fontheight);
  
  return state;
}

static void
condition_set_state(Condition *condition, ConditionState *state)
{
  OBJECT_SET_STRING(condition,state,cond_value);
  OBJECT_SET_FONT(condition,state,cond_font);
  OBJECT_SET_COLOR(condition,state,cond_color);
  OBJECT_SET_FONTHEIGHT(condition,state,cond_fontheight);
  boolequation_set_value(condition->cond,condition->cond_value);
  condition->cond->font = condition->cond_font;
  condition->cond->fontheight = condition->cond_fontheight;
  condition->cond->color = condition->cond_color;

  g_free(state);
  condition_update_data(condition);
}


static void
condition_save(Condition *condition, ObjectNode obj_node,
		const char *filename)
{
  connection_save(&condition->connection, obj_node);

  save_boolequation(obj_node,"condition",condition->cond);
  save_font(obj_node,"cond_font",condition->cond_font);
  save_real(obj_node,"cond_fontheight",condition->cond_fontheight);
  save_color(obj_node,"cond_color",&condition->cond_color);
}

static Object *
condition_load(ObjectNode obj_node, int version, const char *filename)
{
  Condition *condition;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj;

  init_default_values();
  condition = g_malloc0(sizeof(Condition));

  conn = &condition->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &condition_type;
  obj->ops = &condition_ops;

  connection_load(conn, obj_node);
  connection_init(conn, 2,0);

  condition->cond_font = load_font(obj_node,"cond_font",defaults.font);
  condition->cond_fontheight = load_real(obj_node,"cond_fontheight",
                                        defaults.font_size);
  load_color(obj_node,"cond_color",&condition->cond_color,
             &defaults.font_color);

  condition->cond = load_boolequation(obj_node,"condition",NULL,
                                             condition->cond_font,
                                             condition->cond_fontheight,
                                             &condition->cond_color);

  condition->cond_value = g_strdup(condition->cond->value);

  extra->start_trans = 
    extra->start_long = 
    extra->end_long = CONDITION_LINE_WIDTH/2.0;
  extra->end_trans = MAX(CONDITION_LINE_WIDTH,CONDITION_ARROW_SIZE) / 2.0;

  condition_update_data(condition);

  return &condition->connection.object;
}











