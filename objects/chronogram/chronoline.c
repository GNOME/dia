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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <glib.h>

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
#include "chronoline_event.h"

#include "pixmaps/chronoline.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0

typedef struct _Chronoline Chronoline;
typedef struct _ChronolinePropertiesDialog ChronolinePropertiesDialog;
typedef struct _ChronolineDefaultsDialog ChronolineDefaultsDialog;
typedef struct _ChronolineState ChronolineState;
typedef struct _ChronolineDefaults ChronolineDefaults;

struct _ChronolineState {
  ObjectState obj_state;

  real main_lwidth;
  Color color;
  real start_time;
  real end_time;
  real data_lwidth;
  Color data_color;
  char *events;
  char *name;
  real rise_time;
  real fall_time;
  gboolean multibit;

  Font *font;
  real font_size;
  Color font_color;
};

struct _Chronoline {
  Element element;

  real main_lwidth;
  Color color;
  real start_time;
  real end_time;
  real data_lwidth;
  Color data_color;
  char *events;
  char *name;
  real rise_time;
  real fall_time;
  gboolean multibit;
  Font *font;
  real font_size;
  Color font_color;
  
  ConnPointLine *snap; /* not saved ; num_connections derived from
			  the event string. */
  CLEventList *evtlist;

  int checksum;
  /* computed values : */
  real labelwidth;
  real y_down,y_up;
  Color gray, datagray;
};

struct _ChronolinePropertiesDialog {
  AttributeDialog dialog;
  Chronoline *parent;
  AttributeNotebook notebook;

  struct {
    AttributePage dialog;
    Chronoline *parent;

    MultiStringAttribute events;
    StringAttribute name;
  } *data_page;
  struct {
    AttributePage dialog;
    Chronoline *parent;

    BoolAttribute multibit;
    RealAttribute start_time;
    RealAttribute end_time;
    RealAttribute rise_time;
    RealAttribute fall_time;
  } *parameter_page;
  struct {
    AttributePage dialog;
    Chronoline *parent;

    RealAttribute main_lwidth;
    RealAttribute data_lwidth;
    ColorAttribute data_color;
    ColorAttribute color;
    FontAttribute font;
    FontHeightAttribute font_size;
    ColorAttribute font_color;
  } *aspect_page;
};

struct _ChronolineDefaults {
  char *name;   /* not edited */
  char *events; /* not edited */

  real main_lwidth;
  real data_lwidth;
  Color color;
  Color data_color;

  real start_time;
  real end_time;
  real rise_time;
  real fall_time;
  gboolean multibit;
   
  Font *font;
  real font_size;
  Color font_color;
};

struct _ChronolineDefaultsDialog {
  AttributeDialog dialog;
  ChronolineDefaults *parent;
  AttributeNotebook notebook;

  struct {
    AttributePage dialog;
    ChronolineDefaults *parent;

    BoolAttribute multibit;
    RealAttribute start_time;
    RealAttribute end_time;
    RealAttribute rise_time;
    RealAttribute fall_time;
  } *parameter_page;
  struct {
    AttributePage dialog;
    ChronolineDefaults *parent;

    RealAttribute main_lwidth;
    RealAttribute data_lwidth;
    ColorAttribute data_color;
    ColorAttribute color;
    FontAttribute font;
    FontHeightAttribute font_size;
    ColorAttribute font_color;
  } *aspect_page;
};

static ChronolinePropertiesDialog *chronoline_properties_dialog;
static ChronolineDefaultsDialog *chronoline_defaults_dialog;
static ChronolineDefaults defaults;

static real chronoline_distance_from(Chronoline *chronoline, Point *point);
static void chronoline_select(Chronoline *chronoline, Point *clicked_point,
		       Renderer *interactive_renderer);
static void chronoline_move_handle(Chronoline *chronoline, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void chronoline_move(Chronoline *chronoline, Point *to);
static void chronoline_draw(Chronoline *chronoline, Renderer *renderer);
static void chronoline_update_data(Chronoline *chronoline);
static Object *chronoline_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void chronoline_destroy(Chronoline *chronoline);
static Object *chronoline_copy(Chronoline *chronoline);
static PROPDLG_TYPE chronoline_get_properties(Chronoline *chronoline);
static ObjectChange *chronoline_apply_properties(Chronoline *chronoline);

static ChronolineState *chronoline_get_state(Chronoline *chronoline);
static void chronoline_set_state(Chronoline *chronoline, ChronolineState *state);

static void chronoline_save(Chronoline *chronoline, ObjectNode obj_node, const char *filename);
static Object *chronoline_load(ObjectNode obj_node, int version, const char *filename);
static PROPDLG_TYPE chronoline_get_defaults(void);
static void chronoline_apply_defaults(void);
static void chronoline_free_state(ObjectState *objstate);

/*static DiaMenu *chronoline_get_object_menu(Chronoline *chronoline, Point *clickedpoint); */

static ObjectTypeOps chronoline_type_ops =
{
  (CreateFunc) chronoline_create,
  (LoadFunc)   chronoline_load,
  (SaveFunc)   chronoline_save,
  (GetDefaultsFunc)   chronoline_get_defaults,
  (ApplyDefaultsFunc) chronoline_apply_defaults
};

ObjectType chronoline_type =
{
  "chronogram - line",  /* name */
  0,                 /* version */
  (char **) chronoline_xpm, /* pixmap */

  &chronoline_type_ops      /* ops */
};

static ObjectOps chronoline_ops = {
  (DestroyFunc)         chronoline_destroy,
  (DrawFunc)            chronoline_draw,
  (DistanceFunc)        chronoline_distance_from,
  (SelectFunc)          chronoline_select,
  (CopyFunc)            chronoline_copy,
  (MoveFunc)            chronoline_move,
  (MoveHandleFunc)      chronoline_move_handle,
  (GetPropertiesFunc)   chronoline_get_properties,
  (ApplyPropertiesFunc) chronoline_apply_properties,
  (ObjectMenuFunc)      NULL,
};

static ObjectChange *
chronoline_apply_properties(Chronoline *chronoline)
{
  ObjectState *old_state;
  ChronolinePropertiesDialog *dlg = chronoline_properties_dialog;

  PROPDLG_SANITY_CHECK(dlg,chronoline);

  old_state = (ObjectState *)chronoline_get_state(chronoline);
  
  PROPDLG_APPLY_STRING(dlg->data_page,name);
  PROPDLG_APPLY_MULTISTRING(dlg->data_page,events);

  PROPDLG_APPLY_REAL(dlg->parameter_page,start_time);
  PROPDLG_APPLY_REAL(dlg->parameter_page,end_time);

  PROPDLG_APPLY_REAL(dlg->parameter_page,rise_time);
  PROPDLG_APPLY_REAL(dlg->parameter_page,fall_time);

  PROPDLG_APPLY_BOOL(dlg->parameter_page,multibit);

  PROPDLG_APPLY_REAL(dlg->aspect_page,data_lwidth);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,data_color);
  PROPDLG_APPLY_REAL(dlg->aspect_page,main_lwidth);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,color);

  PROPDLG_APPLY_FONT(dlg->aspect_page,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg->aspect_page,font_size);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,font_color);
  
  /* time_lstep should probably be a multiplier of time_step, but the user
     is sovereign. */
  chronoline_update_data(chronoline);

  return new_object_state_change(&chronoline->element.object, 
                                 old_state, 
				 (GetStateFunc)chronoline_get_state,
				 (SetStateFunc)chronoline_set_state);
}

static PROPDLG_TYPE
chronoline_get_properties(Chronoline *chronoline)
{
  ChronolinePropertiesDialog *dlg = chronoline_properties_dialog;
  
  PROPDLG_CREATE(dlg,chronoline);
  PROPDLG_NOTEBOOK_CREATE(dlg,notebook);
  PROPDLG_PAGE_CREATE(dlg,notebook,aspect_page,_("Aspect"));
  PROPDLG_PAGE_CREATE(dlg,notebook,parameter_page,_("Parameters"));
  PROPDLG_PAGE_CREATE(dlg,notebook,data_page,_("Data"));

  PROPDLG_SHOW_STRING(dlg->data_page,name,_("Data name:"));
  PROPDLG_SHOW_SEPARATOR(dlg->data_page);
  PROPDLG_SHOW_MULTISTRING(dlg->data_page,events,_("Events:"),5);
  PROPDLG_SHOW_STATIC(dlg->data_page,_("Event specification:\n"
			    "@ time    set the pointer at an absolute time.\n"
			    "( duration  sets the signal up, then wait 'duration'.\n"
			    ") duration  sets the signal down, then wait 'duration'.\n" 
			    "u duration  sets the signal to \"unknown\" state, then wait 'duration'.\n"
			    "example : @ 1.0 (2.0)1.0(2.0)\n" ));

  PROPDLG_SHOW_REAL(dlg->parameter_page,start_time,
		    _("Start time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,end_time,
		    _("End time:"),-32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,rise_time,
		    _("Rise time:"),0.0,1000.0,.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,fall_time,
		    _("Fall time:"),0.0,1000.0,.1);
  PROPDLG_SHOW_BOOL(dlg->parameter_page,multibit,
		    _("Multi-bit data"));

  PROPDLG_SHOW_COLOR(dlg->aspect_page,data_color,_("Data color:"));
  PROPDLG_SHOW_REAL(dlg->aspect_page,data_lwidth,
		    _("Data line width:"),0.0,10.0,0.1);
  PROPDLG_SHOW_COLOR(dlg->aspect_page,color,_("Line color:"));
  PROPDLG_SHOW_REAL(dlg->aspect_page,main_lwidth,
		    _("Line width:"),0.0,10.0,0.1);
  PROPDLG_SHOW_FONT(dlg->aspect_page,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg->aspect_page,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg->aspect_page,font_color,_("Font color:"));

  PROPDLG_PAGE_READY(dlg,notebook,data_page);
  PROPDLG_PAGE_READY(dlg,notebook,parameter_page);
  PROPDLG_PAGE_READY(dlg,notebook,aspect_page);
  PROPDLG_NOTEBOOK_READY(dlg,notebook);

  PROPDLG_READY(dlg);
  chronoline_properties_dialog = dlg;

  PROPDLG_RETURN(dlg);
}
static void
chronoline_apply_defaults(void)
{
  ChronolineDefaultsDialog *dlg = chronoline_defaults_dialog;

  PROPDLG_APPLY_REAL(dlg->parameter_page,start_time);
  PROPDLG_APPLY_REAL(dlg->parameter_page,end_time);

  PROPDLG_APPLY_REAL(dlg->parameter_page,rise_time);
  PROPDLG_APPLY_REAL(dlg->parameter_page,fall_time);

  PROPDLG_APPLY_BOOL(dlg->parameter_page,multibit);

  PROPDLG_APPLY_REAL(dlg->aspect_page,data_lwidth);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,data_color);
  PROPDLG_APPLY_REAL(dlg->aspect_page,main_lwidth);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,color);

  PROPDLG_APPLY_FONT(dlg->aspect_page,font);
  PROPDLG_APPLY_FONTHEIGHT(dlg->aspect_page,font_size);
  PROPDLG_APPLY_COLOR(dlg->aspect_page,font_color);
}

static void
chronoline_init_defaults(void) {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    defaults.name = "";
    defaults.events = "";

    defaults.font = font_getfont("Helvetica");
    defaults.font_size = 1.0;
    defaults.font_color = color_black;
    defaults.start_time = 0.0;
    defaults.end_time = 20.0;
    defaults.rise_time = .3; 
    defaults.fall_time = .3;
    defaults.color = color_black;
    defaults.main_lwidth = .1;
    defaults.data_lwidth = .1;
    defaults.data_color.red = 1.0;
    defaults.data_color.green = 0.0;
    defaults.data_color.blue = 0.0;
    defaults.multibit = FALSE;

    defaults_initialized = 1;
  }
}

static GtkWidget *
chronoline_get_defaults(void)
{
  ChronolineDefaultsDialog *dlg = chronoline_defaults_dialog;

  chronoline_init_defaults();
  PROPDLG_CREATE(dlg, &defaults);
  PROPDLG_NOTEBOOK_CREATE(dlg,notebook);
  PROPDLG_PAGE_CREATE(dlg,notebook,aspect_page,_("Aspect"));
  PROPDLG_PAGE_CREATE(dlg,notebook,parameter_page,_("Parameters"));

  PROPDLG_SHOW_REAL(dlg->parameter_page,start_time,_("Start time:"),
		    -32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,end_time,_("End time:"),
		    -32767.0,32768.0,0.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,rise_time,_("Rise time:"),
		    0.0,1000.0,.1);
  PROPDLG_SHOW_REAL(dlg->parameter_page,fall_time,_("Fall time:"),
		    0.0,1000.0,.1);
  PROPDLG_SHOW_BOOL(dlg->parameter_page,multibit,_("Multi-bit data"));


  PROPDLG_SHOW_COLOR(dlg->aspect_page,data_color,_("Data color:"));
  PROPDLG_SHOW_REAL(dlg->aspect_page,data_lwidth,_("Data line width:"),
		    0.0,10.0,0.1);
  PROPDLG_SHOW_COLOR(dlg->aspect_page,color,_("Line color:"));
  PROPDLG_SHOW_REAL(dlg->aspect_page,main_lwidth,_("Line width:"),
		    0.0,10.0,0.1);
  PROPDLG_SHOW_FONT(dlg->aspect_page,font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg->aspect_page,font_size,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg->aspect_page,font_color,_("Font color:"));

  PROPDLG_PAGE_READY(dlg,notebook,parameter_page);
  PROPDLG_PAGE_READY(dlg,notebook,aspect_page);
  PROPDLG_NOTEBOOK_READY(dlg,notebook);
  PROPDLG_READY(dlg);
  chronoline_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static real
chronoline_distance_from(Chronoline *chronoline, Point *point)
{
  Object *obj = &chronoline->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
chronoline_select(Chronoline *chronoline, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  element_update_handles(&chronoline->element);
}

static void
chronoline_move_handle(Chronoline *chronoline, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  g_assert(chronoline!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle(&chronoline->element, handle->id, to, reason);
  chronoline_update_data(chronoline);
}

static void
chronoline_move(Chronoline *chronoline, Point *to)
{
  chronoline->element.corner = *to;
  chronoline_update_data(chronoline);
}

static void 
cld_onebit(Chronoline *chronoline,
	   Renderer *renderer,
	   real x1,CLEventType s1,
	   real x2,CLEventType s2,
	   gboolean fill)
{
  Point pts[4];
  real y_down = chronoline->y_down;
  real y_up = chronoline->y_up;
  
  pts[0].x = pts[1].x = x1;
  pts[2].x = pts[3].x = x2;

  pts[0].y = pts[3].y = chronoline->y_down;
  pts[1].y = (s1 == CLE_OFF)?y_down:y_up;
  pts[2].y = (s2 == CLE_OFF)?y_down:y_up;

  if (fill) {
    if ((s1 == CLE_UNKNOWN) || (s2 == CLE_UNKNOWN)) {
      renderer->ops->fill_polygon(renderer,pts,sizeof(pts)/sizeof(pts[0]),
				  &chronoline->datagray);
    } else {
      renderer->ops->fill_polygon(renderer,pts,sizeof(pts)/sizeof(pts[0]),
				  &color_white);
    }    
  } else {
    renderer->ops->draw_line(renderer,&pts[1],&pts[2],
			     &chronoline->data_color);
  }
}


static void 
cld_multibit(Chronoline *chronoline,
	     Renderer *renderer,
	     real x1,CLEventType s1,
	     real x2,CLEventType s2,
	     gboolean fill)
{
  Point pts[4];
  real ymid = .5 * (chronoline->y_down + chronoline->y_up);
  real y_down = chronoline->y_down;
  real y_up = chronoline->y_up;

  pts[0].x = pts[1].x = x1;
  pts[2].x = pts[3].x = x2;

  if (s1 == CLE_OFF) {
    pts[0].y = pts[1].y = ymid;
  } else {
    pts[0].y = y_down;
    pts[1].y = y_up;
  }
  if (s2 == CLE_OFF) {
    pts[2].y = pts[3].y = ymid;
  } else {
    pts[3].y = y_down;
    pts[2].y = y_up;
  }

  if (fill) { 
    if ((s1 == CLE_UNKNOWN) || (s2 == CLE_UNKNOWN)) {
      renderer->ops->fill_polygon(renderer,pts,sizeof(pts)/sizeof(pts[0]),
				  &chronoline->datagray);
    } else {
      renderer->ops->fill_polygon(renderer,pts,sizeof(pts)/sizeof(pts[0]),
				  &color_white);
    }    
  } else {
    renderer->ops->draw_line(renderer,&pts[1],&pts[2],
			     &chronoline->data_color);
    renderer->ops->draw_line(renderer,&pts[0],&pts[3],
			     &chronoline->data_color);
  }
}



inline static void
chronoline_draw_really(Chronoline *chronoline, Renderer *renderer, 
		       gboolean fill)
{
  Element *elem = &chronoline->element;
  real oldx,newx;

  gboolean finished = FALSE;
  CLEventType state = CLE_UNKNOWN;

  CLEventList *lst;
  CLEvent *evt;
  real start_time = chronoline->start_time;
  real end_time = chronoline->end_time;
  
  oldx = elem->corner.x;

  lst = chronoline->evtlist;
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linewidth(renderer,chronoline->data_lwidth);

  while (lst) {
    evt = (CLEvent *)lst->data;
    g_assert(evt);
    if (evt->time >= start_time) {
      if (evt->time <= end_time) {
	/* regular point */
	newx = evt->x;
	
	if (chronoline->multibit) 
	  cld_multibit(chronoline,renderer,oldx,state,newx,evt->type,fill);
	else
	  cld_onebit(chronoline,renderer,oldx,state,newx,evt->type,fill);
	oldx = newx;
      } else {
	newx = elem->corner.x + elem->width;
	if (!finished) {
	  if (chronoline->multibit) 
	    cld_multibit(chronoline,renderer,oldx,state,newx,evt->type,fill);
	  else
	    cld_onebit(chronoline,renderer,oldx,state,newx,evt->type,fill);
	  finished = TRUE;
	} 
      }	
    }
    state = evt->type;
    lst = g_slist_next(lst);
  }
  if (!finished) {
    newx = chronoline->element.corner.x+chronoline->element.width;
    if (chronoline->multibit) 
      cld_multibit(chronoline,renderer,oldx,state,newx,state,fill);
    else
      cld_onebit(chronoline,renderer,oldx,state,newx,state,fill);
  }    
}

static void
chronoline_draw(Chronoline *chronoline, Renderer *renderer)
{
  Element *elem;
  Point lr_corner;
  Point p1,p2,p3;

  g_assert(chronoline != NULL);
  g_assert(renderer != NULL);

  elem = &chronoline->element;

  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DOTTED);
  renderer->ops->set_linewidth(renderer, chronoline->main_lwidth);
  p1.x = elem->corner.x + elem->width;
  p1.y = elem->corner.y;
  renderer->ops->draw_line(renderer,&elem->corner,&p1,&chronoline->gray);

  chronoline_draw_really(chronoline,renderer,TRUE);
  chronoline_draw_really(chronoline,renderer,FALSE);

  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  p1.x = elem->corner.x;
  p2.x = lr_corner.x;
  p1.y = p2.y = chronoline->y_down;

  renderer->ops->set_linewidth(renderer, chronoline->main_lwidth);
  renderer->ops->draw_line(renderer,&p1,&p2,&chronoline->color);
  p2.x = p1.x = elem->corner.x;
  p1.y = chronoline->y_down;
  p2.y = chronoline->y_up;
  renderer->ops->draw_line(renderer,&p1,&p2,&chronoline->color);

  renderer->ops->set_font(renderer, chronoline->font, 
			  chronoline->font_size);
  p3.y = lr_corner.y - chronoline->font_size 
    + font_ascent(chronoline->font, 
		  chronoline->font_size);
  p3.x = p1.x - chronoline->main_lwidth;
  renderer->ops->draw_string(renderer,chronoline->name,&p3,ALIGN_RIGHT,
			     &chronoline->color);
}

static void
chronoline_free_state(ObjectState *objstate)
{
  ChronolineState *state G_GNUC_UNUSED= (ChronolineState *)objstate;

  OBJECT_FREE_STRING(state,name);
  OBJECT_FREE_STRING(state,events);

  OBJECT_FREE_REAL(state,start_time);
  OBJECT_FREE_REAL(state,end_time);

  OBJECT_FREE_REAL(state,rise_time);
  OBJECT_FREE_REAL(state,fall_time);

  OBJECT_FREE_BOOL(state,multibit);

  OBJECT_FREE_REAL(state,data_lwidth);
  OBJECT_FREE_COLOR(state,data_color);
  OBJECT_FREE_REAL(state,main_lwidth);
  OBJECT_FREE_COLOR(state,color);

  OBJECT_FREE_FONT(state,font);
  OBJECT_FREE_FONTHEIGHT(state,font_size);
  OBJECT_FREE_COLOR(state,font_color);
}

static ChronolineState *
chronoline_get_state(Chronoline *chronoline)
{
  ChronolineState *state = g_new0(ChronolineState, 1);

  state->obj_state.free = chronoline_free_state; 

  OBJECT_GET_STRING(chronoline,state,name);
  OBJECT_GET_STRING(chronoline,state,events);

  OBJECT_GET_REAL(chronoline,state,start_time);
  OBJECT_GET_REAL(chronoline,state,end_time);

  OBJECT_GET_REAL(chronoline,state,rise_time);
  OBJECT_GET_REAL(chronoline,state,fall_time);

  OBJECT_GET_BOOL(chronoline,state,multibit);

  OBJECT_GET_REAL(chronoline,state,data_lwidth);
  OBJECT_GET_COLOR(chronoline,state,data_color);
  OBJECT_GET_REAL(chronoline,state,main_lwidth);
  OBJECT_GET_COLOR(chronoline,state,color);

  OBJECT_GET_FONT(chronoline,state,font);
  OBJECT_GET_FONTHEIGHT(chronoline,state,font_size);
  OBJECT_GET_COLOR(chronoline,state,font_color);

  return state;
}

static void
chronoline_set_state(Chronoline *chronoline, ChronolineState *state)
{
  OBJECT_SET_STRING(chronoline,state,name);
  OBJECT_SET_STRING(chronoline,state,events);

  OBJECT_SET_REAL(chronoline,state,start_time);
  OBJECT_SET_REAL(chronoline,state,end_time);

  OBJECT_SET_REAL(chronoline,state,rise_time);
  OBJECT_SET_REAL(chronoline,state,fall_time);

  OBJECT_SET_BOOL(chronoline,state,multibit);

  OBJECT_SET_REAL(chronoline,state,data_lwidth);
  OBJECT_SET_COLOR(chronoline,state,data_color);
  OBJECT_SET_REAL(chronoline,state,main_lwidth);
  OBJECT_SET_COLOR(chronoline,state,color);

  OBJECT_SET_FONT(chronoline,state,font);
  OBJECT_SET_FONTHEIGHT(chronoline,state,font_size);
  OBJECT_SET_COLOR(chronoline,state,font_color);

  chronoline_update_data(chronoline);
}


inline static void grayify(Color *col,Color *src)
{
  col->red = .5 * (src->red + color_white.red);
  col->green = .5 * (src->green + color_white.green);
  col->blue = .5 * (src->blue + color_white.blue);
}

static void
chronoline_update_data(Chronoline *chronoline)
{
  Element *elem = &chronoline->element;
  Object *obj = &elem->object;
  real time_span;
  Point ur_corner;
  int shouldbe,i;
  real realheight;
  CLEventList *lst;
  CLEvent *evt;  
  GSList *conn_elem;
  ElementBBExtras *extra = &elem->extra_spacing;

  grayify(&chronoline->datagray,&chronoline->data_color);
  grayify(&chronoline->gray,&chronoline->color);

  chronoline->labelwidth = font_string_width(chronoline->name,
					     chronoline->font,
					     chronoline->font_size);

  chronoline->y_up = elem->corner.y;
  chronoline->y_down = elem->corner.y + elem->height;
  
  /* Now, update the drawing helper counters */
  time_span = chronoline->end_time - chronoline->start_time;
  if (time_span == 0) {
    chronoline->end_time = chronoline->start_time + .1;
    time_span = .1;
  } else if (time_span < 0) {
    chronoline->start_time = chronoline->end_time;
    time_span = -time_span;
    chronoline->end_time = chronoline->start_time + time_span;
  }

  extra->border_trans = chronoline->main_lwidth / 2;
  element_update_boundingbox(elem);

  /* fix boundingbox for special extras: */
  realheight = obj->bounding_box.bottom - obj->bounding_box.top;
  realheight = MAX(realheight,chronoline->font_size);
  
  obj->bounding_box.left -= chronoline->labelwidth;
  obj->bounding_box.bottom = obj->bounding_box.top + realheight + 
    chronoline->main_lwidth;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  /* Update connections: */
  ur_corner.x = elem->corner.x + elem->width;
  ur_corner.y = elem->corner.y;

  /* Update the events : count those which fit in [start_time,end_time].
   */
  
  reparse_clevent(chronoline->events,
		  &chronoline->evtlist,
		  &chronoline->checksum,
		  chronoline->rise_time,
		  chronoline->fall_time,
		  chronoline->end_time);
  shouldbe = 0;
  lst = chronoline->evtlist;

  while (lst) {
    evt = (CLEvent *)lst->data;
    
    if ((evt->time >= chronoline->start_time) && 
	(evt->time <= chronoline->end_time))
      shouldbe++;
    lst = g_slist_next(lst);
  }

  connpointline_adjust_count(chronoline->snap,shouldbe,&ur_corner);
  connpointline_update(chronoline->snap);
  /* connpointline_putonaline(chronoline->snap,&elem->corner,&ur_corner); */

  /* Now fix the actual connection point positions : */
  lst = chronoline->evtlist;
  conn_elem = chronoline->snap->connections;
  i = 0;
  while (lst && lst->data && conn_elem && conn_elem->data) {
    ConnectionPoint *cp = (ConnectionPoint *)(conn_elem->data);
    evt = (CLEvent *)lst->data;
    
    if ((evt->time >= chronoline->start_time) && 
	(evt->time <= chronoline->end_time)) {
      evt->x = elem->corner.x + 
	elem->width*(evt->time-chronoline->start_time)/time_span;
      g_assert(cp);
      g_assert(i < chronoline->snap->num_connections);
      cp->pos.x = evt->x;
      if (chronoline->multibit) {
	cp->pos.y = .5 * (chronoline->y_down + chronoline->y_up);
      } else {
	cp->pos.y = (evt->type==CLE_OFF?
		     chronoline->y_down:chronoline->y_up);
      }
      i++;
      conn_elem = g_slist_next(conn_elem);
    } else if (evt->time >= chronoline->start_time) {
      evt->x = elem->corner.x; 
    } else if (evt->time <= chronoline->end_time) {
      evt->x = elem->corner.x + elem->width;
    }
    lst = g_slist_next(lst);
  }
}

static void chronoline_alloc(Chronoline **chronoline,
			    Element **elem,
			    Object **obj)
{
  chronoline_init_defaults();

  *chronoline = g_new0(Chronoline,1);
  *elem = &((*chronoline)->element);

  *obj = &((*chronoline)->element.object);
  (*obj)->type = &chronoline_type;
  (*obj)->ops = &chronoline_ops;
}

static Object *
chronoline_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Chronoline *chronoline;
  Element *elem;
  Object *obj;

  chronoline_alloc(&chronoline,&elem,&obj);

  chronoline->snap = connpointline_create(obj,0); 

  elem->corner = *startpoint;
  elem->width = 20.0;
  elem->height = 3.0;

  element_init(elem, 8, 0);

  OBJECT_GET_STRING(&defaults,chronoline,name);
  OBJECT_GET_STRING(&defaults,chronoline,events);

  OBJECT_GET_REAL(&defaults,chronoline,start_time);
  OBJECT_GET_REAL(&defaults,chronoline,end_time);

  OBJECT_GET_REAL(&defaults,chronoline,rise_time);
  OBJECT_GET_REAL(&defaults,chronoline,fall_time);

  OBJECT_GET_BOOL(&defaults,chronoline,multibit);

  OBJECT_GET_REAL(&defaults,chronoline,data_lwidth);
  OBJECT_GET_COLOR(&defaults,chronoline,data_color);
  OBJECT_GET_REAL(&defaults,chronoline,main_lwidth);
  OBJECT_GET_COLOR(&defaults,chronoline,color);

  OBJECT_GET_FONT(&defaults,chronoline,font);
  OBJECT_GET_FONTHEIGHT(&defaults,chronoline,font_size);
  OBJECT_GET_COLOR(&defaults,chronoline,font_color);
  
  chronoline->evtlist = NULL;
  chronoline_update_data(chronoline);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &chronoline->element.object;
}

static void
chronoline_destroy(Chronoline *chronoline)
{
  connpointline_destroy(chronoline->snap);
  destroy_clevent_list(chronoline->evtlist);
  element_destroy(&chronoline->element);
}

static Object *
chronoline_copy(Chronoline *chronoline)
{
  Chronoline *newchronoline;
  Element *elem, *newelem;
  Object *newobj, *obj;
  int rcc = 0;

  elem = &chronoline->element; 
  obj = &elem->object;
  chronoline_alloc(&newchronoline,&newelem,&newobj);
  element_copy(elem, newelem);

  newchronoline->snap = connpointline_copy(newobj,chronoline->snap,&rcc); 
  g_assert(rcc == newobj->num_connections);


  OBJECT_GET_STRING(chronoline,newchronoline,name);
  OBJECT_GET_STRING(chronoline,newchronoline,events);

  OBJECT_GET_REAL(chronoline,newchronoline,start_time);
  OBJECT_GET_REAL(chronoline,newchronoline,end_time);

  OBJECT_GET_REAL(chronoline,newchronoline,rise_time);
  OBJECT_GET_REAL(chronoline,newchronoline,fall_time);

  OBJECT_GET_BOOL(chronoline,newchronoline,multibit);

  OBJECT_GET_REAL(chronoline,newchronoline,data_lwidth);
  OBJECT_GET_COLOR(chronoline,newchronoline,data_color);
  OBJECT_GET_REAL(chronoline,newchronoline,main_lwidth);
  OBJECT_GET_COLOR(chronoline,newchronoline,color);

  OBJECT_GET_FONT(chronoline,newchronoline,font);
  OBJECT_GET_FONTHEIGHT(chronoline,newchronoline,font_size);
  OBJECT_GET_COLOR(chronoline,newchronoline,font_color);

  newchronoline->evtlist = NULL;

  chronoline_update_data(newchronoline);

  return &newchronoline->element.object;
}


static void
chronoline_save(Chronoline *chronoline, ObjectNode obj_node, const char *filename)
{
  element_save(&chronoline->element, obj_node);

  SAVE_STRING(obj_node,chronoline,name);
  SAVE_STRING(obj_node,chronoline,events);

  SAVE_REAL(obj_node,chronoline,start_time);
  SAVE_REAL(obj_node,chronoline,end_time);

  SAVE_REAL(obj_node,chronoline,rise_time);
  SAVE_REAL(obj_node,chronoline,fall_time);

  SAVE_BOOL(obj_node,chronoline,multibit);

  SAVE_REAL(obj_node,chronoline,data_lwidth);
  SAVE_COLOR(obj_node,chronoline,data_color);
  SAVE_REAL(obj_node,chronoline,main_lwidth);
  SAVE_COLOR(obj_node,chronoline,color);

  SAVE_FONT(obj_node,chronoline,font);
  SAVE_FONTHEIGHT(obj_node,chronoline,font_size);
  SAVE_COLOR(obj_node,chronoline,font_color);
}

static Object *
chronoline_load(ObjectNode obj_node, int version, const char *filename)
{
  Chronoline *chronoline;
  Element *elem;
  Object *obj;

  chronoline_alloc(&chronoline,&elem,&obj);
  chronoline->snap = connpointline_create(obj,0);

  element_load(elem, obj_node);
  element_init(elem, 8, 0);

  LOAD_STRING(obj_node,chronoline,&defaults,name);
  LOAD_STRING(obj_node,chronoline,&defaults,events);

  LOAD_REAL(obj_node,chronoline,&defaults,start_time);
  LOAD_REAL(obj_node,chronoline,&defaults,end_time);

  LOAD_REAL(obj_node,chronoline,&defaults,rise_time);
  LOAD_REAL(obj_node,chronoline,&defaults,fall_time);

  LOAD_BOOL(obj_node,chronoline,&defaults,multibit);

  LOAD_REAL(obj_node,chronoline,&defaults,data_lwidth);
  LOAD_COLOR(obj_node,chronoline,&defaults,data_color);
  LOAD_REAL(obj_node,chronoline,&defaults,main_lwidth);
  LOAD_COLOR(obj_node,chronoline,&defaults,color);

  LOAD_FONT(obj_node,chronoline,&defaults,font);
  LOAD_FONTHEIGHT(obj_node,chronoline,&defaults,font_size);
  LOAD_COLOR(obj_node,chronoline,&defaults,font_color);

  chronoline_update_data(chronoline);

  return &chronoline->element.object;
}
