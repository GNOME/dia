/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Chronogram objects support
 * Copyright (C) 2000 Cyrille Chepelov 
 * 
 * Ultimately forked from Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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
#include <glib.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "connpoint_line.h"
#include "color.h"
#include "lazyprops.h"

#include "chronogram.h"
#include "pixmaps/chronoref.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0

typedef struct _Chronoref Chronoref;
typedef struct _ChronorefPropertiesDialog ChronorefPropertiesDialog;
typedef struct _ChronorefDefaultsDialog ChronorefDefaultsDialog;
typedef struct _ChronorefState ChronorefState;
typedef struct _ChronorefDefaults ChronorefDefaults;

struct _ChronorefState {
  ObjectState obj_state;

  real main_lwidth;
  real light_lwidth;
  Color color;
  real start_time;
  real end_time;
  real time_step;
  real time_lstep;
  
  Font *font;
  real font_size;
  Color font_color;
};

struct _Chronoref {
  Element element;

  real main_lwidth;
  real light_lwidth;
  Color color;
  real start_time;
  real end_time;
  real time_step;
  real time_lstep;
  
  Font *font;
  real font_size;
  Color font_color;

  ConnPointLine *scale; /* not saved ; num_connections derived from
			   start_time, end_time, time_step. */
  real majgrad_height,mingrad_height;
  real firstmaj,firstmin; /* in time units */
  real firstmaj_x,firstmin_x,majgrad,mingrad; /* in dia graphic units */
  char spec[10];
};

struct _ChronorefPropertiesDialog {
  AttributeDialog dialog;
  Chronoref *parent;

  RealAttribute main_lwidth;
  RealAttribute light_lwidth;
  ColorAttribute color;
  RealAttribute start_time;
  RealAttribute end_time;
  RealAttribute time_step;
  RealAttribute time_lstep;
  
  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

struct _ChronorefDefaults {
  real main_lwidth;
  real light_lwidth;
  Color color;
  real start_time;
  real end_time;
  real time_step;
  real time_lstep;
  
  Font *font;
  real font_size;
  Color font_color;
};

struct _ChronorefDefaultsDialog {
  AttributeDialog dialog;
  ChronorefDefaults *parent;

  RealAttribute main_lwidth;
  RealAttribute light_lwidth;
  ColorAttribute color;
  RealAttribute start_time;
  RealAttribute end_time;
  RealAttribute time_step;
  RealAttribute time_lstep;
  
  FontAttribute font;
  FontHeightAttribute font_size;
  ColorAttribute font_color;
};

static ChronorefPropertiesDialog *chronoref_properties_dialog;
static ChronorefDefaultsDialog *chronoref_defaults_dialog;
static ChronorefDefaults defaults;

static real chronoref_distance_from(Chronoref *chronoref, Point *point);
static void chronoref_select(Chronoref *chronoref, Point *clicked_point,
		       Renderer *interactive_renderer);
static void chronoref_move_handle(Chronoref *chronoref, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void chronoref_move(Chronoref *chronoref, Point *to);
static void chronoref_draw(Chronoref *chronoref, Renderer *renderer);
static void chronoref_update_data(Chronoref *chronoref);
static Object *chronoref_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void chronoref_destroy(Chronoref *chronoref);
static Object *chronoref_copy(Chronoref *chronoref);
static PROPDLG_TYPE chronoref_get_properties(Chronoref *chronoref);
static ObjectChange *chronoref_apply_properties(Chronoref *chronoref);

static ChronorefState *chronoref_get_state(Chronoref *chronoref);
static void chronoref_set_state(Chronoref *chronoref, ChronorefState *state);

static void chronoref_save(Chronoref *chronoref, ObjectNode obj_node, const char *filename);
static Object *chronoref_load(ObjectNode obj_node, int version, const char *filename);
static PROPDLG_TYPE chronoref_get_defaults();
static void chronoref_apply_defaults();

/*static DiaMenu *chronoref_get_object_menu(Chronoref *chronoref, Point *clickedpoint); */

static ObjectTypeOps chronoref_type_ops =
{
  (CreateFunc) chronoref_create,
  (LoadFunc)   chronoref_load,
  (SaveFunc)   chronoref_save,
  (GetDefaultsFunc)   chronoref_get_defaults,
  (ApplyDefaultsFunc) chronoref_apply_defaults
};

ObjectType chronoref_type =
{
  "chronogram - reference",  /* name */
  0,                 /* version */
  (char **) chronoref_xpm, /* pixmap */

  &chronoref_type_ops      /* ops */
};

static ObjectOps chronoref_ops = {
  (DestroyFunc)         chronoref_destroy,
  (DrawFunc)            chronoref_draw,
  (DistanceFunc)        chronoref_distance_from,
  (SelectFunc)          chronoref_select,
  (CopyFunc)            chronoref_copy,
  (MoveFunc)            chronoref_move,
  (MoveHandleFunc)      chronoref_move_handle,
  (GetPropertiesFunc)   chronoref_get_properties,
  (ApplyPropertiesFunc) chronoref_apply_properties,
  (ObjectMenuFunc)      NULL,
};

static ObjectChange *
chronoref_apply_properties(Chronoref *chronoref)
{
  ObjectState *old_state;
  ChronorefPropertiesDialog *dlg = chronoref_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,chronoref);

  old_state = (ObjectState *)chronoref_get_state(chronoref);

  PROPDLG_APPLY_REAL(dlg,start_time);
  PROPDLG_APPLY_REAL(dlg,end_time);

  PROPDLG_APPLY_REAL(dlg,time_step);
  PROPDLG_APPLY_REAL(dlg,time_lstep);

  PROPDLG_APPLY_COLOR(dlg,color);
  PROPDLG_APPLY_REAL(dlg,main_lwidth);
  PROPDLG_APPLY_REAL(dlg,light_lwidth);
  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);
  
  /* time_lstep should probably be a multiplier of time_step, but the user
     is sovereign. */
  chronoref_update_data(chronoref);
  return new_object_state_change((Object *)chronoref, old_state, 
				 (GetStateFunc)chronoref_get_state,
				 (SetStateFunc)chronoref_set_state);
}

static PROPDLG_TYPE
chronoref_get_properties(Chronoref *chronoref)
{
  ChronorefPropertiesDialog *dlg = chronoref_properties_dialog;
  
  PROPDLG_CREATE(dlg,chronoref);
  
  PROPDLG_SHOW_REAL(dlg,start_time,_("Start time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg,end_time,_("End time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_SEPARATOR(dlg);
  PROPDLG_SHOW_REAL(dlg,time_step,_("Major time step:"),0.1,1000.0,0.1);
  PROPDLG_SHOW_REAL(dlg,time_lstep,_("Minor time step:"),0.1,1000.0,0.1);
  PROPDLG_SHOW_SEPARATOR(dlg);
  PROPDLG_SHOW_COLOR(dlg,color,_("Line color:"));
  PROPDLG_SHOW_REAL(dlg,main_lwidth,_("Line width:"),0.0,10.0,0.1);
  PROPDLG_SHOW_REAL(dlg,light_lwidth,_("Minor step line width:"),
		     0.0,10.0,0.1);
  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Font color:"));

  PROPDLG_READY(dlg);
  chronoref_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}
static void
chronoref_apply_defaults()
{
  ChronorefDefaultsDialog *dlg = chronoref_defaults_dialog;
  PROPDLG_APPLY_REAL(dlg,start_time);
  PROPDLG_APPLY_REAL(dlg,end_time);

  PROPDLG_APPLY_REAL(dlg,time_step);
  PROPDLG_APPLY_REAL(dlg,time_lstep);

  PROPDLG_APPLY_COLOR(dlg,color);
  PROPDLG_APPLY_REAL(dlg,main_lwidth);
  PROPDLG_APPLY_REAL(dlg,light_lwidth);
  PROPDLG_APPLY_FONT(dlg,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,font_size);
  PROPDLG_APPLY_COLOR(dlg,font_color);

}

static void
chronoref_init_defaults() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    defaults.font = font_getfont("Helvetica");
    defaults.font_size = 1.0;
    defaults.font_color = color_black;
    defaults.start_time = 0.0;
    defaults.end_time = 20.0;
    defaults.time_step = 5.0;
    defaults.time_lstep = 1.0;
    defaults.color = color_black;
    defaults.main_lwidth = .1;
    defaults.light_lwidth = .05;

    defaults_initialized = 1;
  }
}

static GtkWidget *
chronoref_get_defaults()
{
  ChronorefDefaultsDialog *dlg = chronoref_defaults_dialog;

  chronoref_init_defaults();
  PROPDLG_CREATE(dlg, &defaults);
  PROPDLG_SHOW_REAL(dlg,start_time,_("Start time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg,end_time,_("End time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_SEPARATOR(dlg);
  PROPDLG_SHOW_REAL(dlg,time_step,_("Major time step:"),0.1,1000.0,0.1);
  PROPDLG_SHOW_REAL(dlg,time_lstep,_("Minor time step:"),0.1,1000.0,0.1);
  PROPDLG_SHOW_SEPARATOR(dlg);
  PROPDLG_SHOW_COLOR(dlg,color,_("Line color:"));
  PROPDLG_SHOW_REAL(dlg,main_lwidth,_("Line width:"),0.0,10.0,0.1);
  PROPDLG_SHOW_REAL(dlg,light_lwidth,_("Minor step line width:"),
		     0.0,10.0,0.1);
  PROPDLG_SHOW_FONT(dlg,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,font_color,_("Font color:"));

  PROPDLG_READY(dlg);
  chronoref_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static real
chronoref_distance_from(Chronoref *chronoref, Point *point)
{
  Object *obj = (Object *)chronoref;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
chronoref_select(Chronoref *chronoref, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  element_update_handles(&chronoref->element);
}

static void
chronoref_move_handle(Chronoref *chronoref, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  g_assert(chronoref!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle(&chronoref->element, handle->id, to, reason);
  chronoref_update_data(chronoref);
}

static void
chronoref_move(Chronoref *chronoref, Point *to)
{
  chronoref->element.corner = *to;
  chronoref_update_data(chronoref);
}

static void
chronoref_draw(Chronoref *chronoref, Renderer *renderer)
{
  Element *elem;
  Point lr_corner;
  real t;
  Point p1,p2,p3;

  assert(renderer != NULL);

  elem = &chronoref->element;
  
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);


  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  p1.y = p2.y = elem->corner.y;

  renderer->ops->set_font(renderer, chronoref->font, chronoref->font_size);
  p3.y = p2.y + chronoref->majgrad_height + 
    font_ascent(chronoref->font, chronoref->font_size);

  renderer->ops->set_linewidth(renderer, chronoref->light_lwidth);
  if (chronoref->time_lstep > 0.0) {
    p2.y = p1.y + chronoref->mingrad_height;
    for (t = chronoref->firstmaj, p1.x = chronoref->firstmin_x; 
	 p1.x <= lr_corner.x; 
	 t += chronoref->time_lstep, p1.x += chronoref->mingrad) {
      p2.x = p1.x;
    
      renderer->ops->draw_line(renderer,&p1,&p2,&chronoref->color);
    }
  }

  renderer->ops->set_linewidth(renderer, chronoref->main_lwidth);
  if (chronoref->time_step > 0.0) {
    p2.y = p1.y + chronoref->majgrad_height;

    for (t = chronoref->firstmaj, p1.x = chronoref->firstmaj_x; 
	 p1.x <= lr_corner.x; 
	 t += chronoref->time_step, p1.x += chronoref->majgrad) {
      char time[10];
      p3.x = p2.x = p1.x;
    
      renderer->ops->draw_line(renderer,&p1,&p2,&chronoref->color);
      snprintf(time,sizeof(time),chronoref->spec,t);
      renderer->ops->draw_string(renderer,time,&p3,ALIGN_CENTER,
				 &chronoref->font_color);
    }
  }
  p1.x = elem->corner.x;
  p2.x = lr_corner.x;
  p1.y = p2.y = elem->corner.y;

  renderer->ops->draw_line(renderer,&p1,&p2,&chronoref->color);
}

void
chronoref_free_state(ObjectState *objstate)
{
  ChronorefState *state G_GNUC_UNUSED= (ChronorefState *)objstate;
  /* actually a useless function ; just a demonstrator. */
  OBJECT_FREE_REAL(state,main_lwidth);
  OBJECT_FREE_REAL(state,light_lwidth);
  OBJECT_FREE_COLOR(state,color);
  OBJECT_FREE_REAL(state,start_time);
  OBJECT_FREE_REAL(state,end_time);
  OBJECT_FREE_REAL(state,time_step);
  OBJECT_FREE_REAL(state,time_lstep);
  OBJECT_FREE_FONT(state,font);
  OBJECT_FREE_FONTHEIGHT(state,font_size);
  OBJECT_FREE_COLOR(state,font_color);
}

static ChronorefState *
chronoref_get_state(Chronoref *chronoref)
{
  ChronorefState *state = g_new(ChronorefState, 1);

  state->obj_state.free = chronoref_free_state; 

  OBJECT_GET_REAL(chronoref,state,main_lwidth);
  OBJECT_GET_REAL(chronoref,state,light_lwidth);
  OBJECT_GET_COLOR(chronoref,state,color);
  OBJECT_GET_REAL(chronoref,state,start_time);
  OBJECT_GET_REAL(chronoref,state,end_time);
  OBJECT_GET_REAL(chronoref,state,time_step);
  OBJECT_GET_REAL(chronoref,state,time_lstep);
  OBJECT_GET_FONT(chronoref,state,font);
  OBJECT_GET_FONTHEIGHT(chronoref,state,font_size);
  OBJECT_GET_COLOR(chronoref,state,font_color);

  return state;
}

static void
chronoref_set_state(Chronoref *chronoref, ChronorefState *state)
{
  OBJECT_SET_REAL(chronoref,state,main_lwidth);
  OBJECT_SET_REAL(chronoref,state,light_lwidth);
  OBJECT_SET_COLOR(chronoref,state,color);
  OBJECT_SET_REAL(chronoref,state,start_time);
  OBJECT_SET_REAL(chronoref,state,end_time);
  OBJECT_SET_REAL(chronoref,state,time_step);
  OBJECT_SET_REAL(chronoref,state,time_lstep);
  OBJECT_SET_FONT(chronoref,state,font);
  OBJECT_SET_FONTHEIGHT(chronoref,state,font_size);
  OBJECT_SET_COLOR(chronoref,state,font_color);

  chronoref_update_data(chronoref);
}


static void
chronoref_update_data(Chronoref *chronoref)
{
  Element *elem = &chronoref->element;
  Object *obj = (Object *) chronoref;
  real time_span,t;
  Point p1,p2;
  Point ur_corner;
  int shouldbe,i;
  real labelwidth;
  char biglabel[10];

  chronoref->majgrad_height = elem->height;
  chronoref->mingrad_height = elem->height / 3.0;

  /* build i = -log_{10}(time_step), then make a %.if format out of it. */
  t = 1;
  i = 0;
  
  while (t > chronoref->time_step) {
    t /= 10;
    i++;
  }
  snprintf(chronoref->spec,sizeof(chronoref->spec),"%%.%df",i);
  snprintf(biglabel,sizeof(biglabel),chronoref->spec,
	   MIN(-ABS(chronoref->start_time),-ABS(chronoref->end_time)));
  
  labelwidth = font_string_width(biglabel,chronoref->font,
				 chronoref->font_size);

  /* Now, update the drawing helper counters */
  time_span = chronoref->end_time - chronoref->start_time;
  if (time_span == 0) {
    chronoref->end_time = chronoref->start_time + .1;
    time_span = .1;
  } else if (time_span < 0) {
    chronoref->start_time = chronoref->end_time;
    time_span = -time_span;
    chronoref->end_time = chronoref->start_time + time_span;
  }

  chronoref->firstmaj = chronoref->time_step * 
    ceil(chronoref->start_time / chronoref->time_step);
  if (chronoref->firstmaj < chronoref->start_time)
    chronoref->firstmaj += chronoref->time_step;
  chronoref->firstmin = chronoref->time_lstep * 
    ceil(chronoref->start_time / chronoref->time_lstep);
  if (chronoref->firstmin < chronoref->start_time)
    chronoref->firstmin += chronoref->time_lstep;

  chronoref->firstmaj_x = elem->corner.x + 
    elem->width*((chronoref->firstmaj-chronoref->start_time)/time_span);
  chronoref->firstmin_x = elem->corner.x + 
    elem->width*((chronoref->firstmin-chronoref->start_time)/time_span);
  chronoref->majgrad = (chronoref->time_step * elem->width) / time_span;
  chronoref->mingrad = (chronoref->time_lstep * elem->width) / time_span;
   
  element_update_boundingbox(elem);
  /* fix boundingchronoref for line_width: */
  obj->bounding_box.top -= chronoref->main_lwidth/2;
  obj->bounding_box.left -= (chronoref->main_lwidth +
			     chronoref->font_size + labelwidth)/2;
  obj->bounding_box.bottom += chronoref->main_lwidth/2 + chronoref->font_size;
  obj->bounding_box.right += (chronoref->main_lwidth + 
			      chronoref->font_size + labelwidth)/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  /* Update connections: */
  ur_corner.x = elem->corner.x + elem->width;
  ur_corner.y = elem->corner.y;

  shouldbe = (int)(ceil((chronoref->end_time-chronoref->firstmin)/
			   chronoref->time_lstep));
  if (shouldbe == 0) shouldbe++;
  if (shouldbe < 0) shouldbe = 0;
  shouldbe++; /* off by one.. */

  connpointline_adjust_count(chronoref->scale,shouldbe,&ur_corner);
  connpointline_update(chronoref->scale);

  point_copy(&p1,&elem->corner); point_copy(&p2,&ur_corner);
  p1.x -= chronoref->mingrad;
  p2.x += chronoref->mingrad; 
  connpointline_putonaline(chronoref->scale,&p1,&p2);
}

static void chronoref_alloc(Chronoref **chronoref,
			    Element **elem,
			    Object **obj)
{
  chronoref_init_defaults();

  *chronoref = g_new0(Chronoref,1);
  *elem = &((*chronoref)->element);

  *obj = (Object *) *chronoref;
  (*obj)->type = &chronoref_type;
  (*obj)->ops = &chronoref_ops;
}

static Object *
chronoref_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Chronoref *chronoref;
  Element *elem;
  Object *obj;

  chronoref_alloc(&chronoref,&elem,&obj);

  chronoref->scale = connpointline_create(obj,0); 

  elem->corner = *startpoint;
  elem->width = 20.0;
  elem->height = 1.0;

  element_init(elem, 8, 0);

  OBJECT_GET_REAL(&defaults,chronoref,main_lwidth);
  OBJECT_GET_REAL(&defaults,chronoref,light_lwidth);
  OBJECT_GET_COLOR(&defaults,chronoref,color);
  OBJECT_GET_REAL(&defaults,chronoref,start_time);
  OBJECT_GET_REAL(&defaults,chronoref,end_time);
  OBJECT_GET_REAL(&defaults,chronoref,time_step);
  OBJECT_GET_REAL(&defaults,chronoref,time_lstep);
  OBJECT_GET_FONT(&defaults,chronoref,font);
  OBJECT_GET_FONTHEIGHT(&defaults,chronoref,font_size);
  OBJECT_GET_COLOR(&defaults,chronoref,font_color);
  
  chronoref_update_data(chronoref);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)chronoref;
}

static void
chronoref_destroy(Chronoref *chronoref)
{
  connpointline_destroy(chronoref->scale);
  element_destroy(&chronoref->element);
}

static Object *
chronoref_copy(Chronoref *chronoref)
{
  Chronoref *newchronoref;
  Element *elem, *newelem;
  Object *newobj, *obj;
  int rcc = 0;

  elem = &chronoref->element; obj = (Object *)chronoref;
  chronoref_alloc(&newchronoref,&newelem,&newobj);
  element_copy(elem, newelem);

  newchronoref->scale = connpointline_copy(newobj,chronoref->scale,&rcc); 
  g_assert(rcc == newobj->num_connections);

  OBJECT_GET_REAL(chronoref,newchronoref,main_lwidth);
  OBJECT_GET_REAL(chronoref,newchronoref,light_lwidth);
  OBJECT_GET_COLOR(chronoref,newchronoref,color);
  OBJECT_GET_REAL(chronoref,newchronoref,start_time);
  OBJECT_GET_REAL(chronoref,newchronoref,end_time);
  OBJECT_GET_REAL(chronoref,newchronoref,time_step);
  OBJECT_GET_REAL(chronoref,newchronoref,time_lstep);
  OBJECT_GET_FONT(chronoref,newchronoref,font);
  OBJECT_GET_FONTHEIGHT(chronoref,newchronoref,font_size);
  OBJECT_GET_COLOR(chronoref,newchronoref,font_color);
  
  chronoref_update_data(newchronoref);

  return (Object *)newchronoref;
}


static void
chronoref_save(Chronoref *chronoref, ObjectNode obj_node, const char *filename)
{
  element_save(&chronoref->element, obj_node);

  SAVE_REAL(obj_node,chronoref,main_lwidth);
  SAVE_REAL(obj_node,chronoref,light_lwidth);
  SAVE_COLOR(obj_node,chronoref,color);
  SAVE_REAL(obj_node,chronoref,start_time);
  SAVE_REAL(obj_node,chronoref,end_time);
  SAVE_REAL(obj_node,chronoref,time_step);
  SAVE_REAL(obj_node,chronoref,time_lstep);
  SAVE_FONT(obj_node,chronoref,font);
  SAVE_FONTHEIGHT(obj_node,chronoref,font_size);
  SAVE_COLOR(obj_node,chronoref,font_color);
}

static Object *
chronoref_load(ObjectNode obj_node, int version, const char *filename)
{
  Chronoref *chronoref;
  Element *elem;
  Object *obj;

  chronoref_alloc(&chronoref,&elem,&obj);
  chronoref->scale = connpointline_create(obj,0);

  element_load(elem, obj_node);
  element_init(elem, 8, 0);

  LOAD_REAL(obj_node,chronoref,&defaults,main_lwidth);
  LOAD_REAL(obj_node,chronoref,&defaults,light_lwidth);
  LOAD_COLOR(obj_node,chronoref,&defaults,color);
  LOAD_REAL(obj_node,chronoref,&defaults,start_time);
  LOAD_REAL(obj_node,chronoref,&defaults,end_time);
  LOAD_REAL(obj_node,chronoref,&defaults,time_step);
  LOAD_REAL(obj_node,chronoref,&defaults,time_lstep);
  LOAD_FONT(obj_node,chronoref,&defaults,font);
  LOAD_FONTHEIGHT(obj_node,chronoref,&defaults,font_size);
  LOAD_COLOR(obj_node,chronoref,&defaults,font_color);

  chronoref_update_data(chronoref);

  return (Object *)chronoref;
}
