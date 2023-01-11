/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Chronogram objects support
 * Copyright (C) 2000, 2001 Cyrille Chepelov
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <glib.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "connpoint_line.h"
#include "color.h"
#include "properties.h"

#include "chronogram.h"
#include "chronoline_event.h"

#include "pixmaps/chronoline.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0

typedef struct _Chronoline {
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
  DiaFont *font;
  real font_size;
  Color font_color;

  /* computed values : */
  ConnPointLine *snap; /* not saved ; num_connections derived from
			  the event string. */
  CLEventList *evtlist;

  int checksum;
  double labelwidth;
  double y_down,y_up;
  Color gray, datagray;
} Chronoline;


static real chronoline_distance_from(Chronoline *chronoline, Point *point);
static void chronoline_select(Chronoline *chronoline, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange *chronoline_move_handle    (Chronoline       *chronoline,
                                                   Handle           *handle,
                                                   Point            *to,
                                                   ConnectionPoint  *cp,
                                                   HandleMoveReason  reason,
                                                   ModifierKeys      modifiers);
static DiaObjectChange  *chronoline_move          (Chronoline       *chronoline,
                                                   Point            *to);
static void chronoline_draw(Chronoline *chronoline, DiaRenderer *renderer);
static void chronoline_update_data(Chronoline *chronoline);
static DiaObject *chronoline_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void chronoline_destroy(Chronoline *chronoline);
static DiaObject *chronoline_load(ObjectNode obj_node, int version,
				  DiaContext *ctx);
static PropDescription *chronoline_describe_props(Chronoline *chronoline);
static void chronoline_get_props(Chronoline *chronoline,
                                 GPtrArray *props);
static void chronoline_set_props(Chronoline *chronoline,
                                 GPtrArray *props);

static ObjectTypeOps chronoline_type_ops =
{
  (CreateFunc) chronoline_create,
  (LoadFunc)   chronoline_load/*using properties*/,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType chronoline_type =
{
  "chronogram - line", /* name */
  0,                /* version */
  chronoline_xpm,    /* pixmap */
  &chronoline_type_ops  /* ops */
};

static ObjectOps chronoline_ops = {
  (DestroyFunc)         chronoline_destroy,
  (DrawFunc)            chronoline_draw,
  (DistanceFunc)        chronoline_distance_from,
  (SelectFunc)          chronoline_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            chronoline_move,
  (MoveHandleFunc)      chronoline_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   chronoline_describe_props,
  (GetPropsFunc)        chronoline_get_props,
  (SetPropsFunc)        chronoline_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData time_range = { -32767.0, 32768.0, 0.1};
static PropNumData edge_range = { 0.0, 1000.0, 0.1};

static PropDescription chronoline_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_NOTEBOOK_BEGIN,
  PROP_NOTEBOOK_PAGE("data",PROP_FLAG_DONT_MERGE,N_("Data")),
  { "name", PROP_TYPE_STRING,PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Data name"),NULL },
  { "events", PROP_TYPE_MULTISTRING,PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Events"), NULL,GINT_TO_POINTER(5) },
  { "help", PROP_TYPE_STATIC,
    PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE|PROP_FLAG_DONT_MERGE,
    N_("Event specification"),N_(
   "@ time    set the pointer to an absolute time.\n"
   "( duration  set the signal up, then wait 'duration'.\n"
   ") duration  set the signal down, then wait 'duration'.\n"
   "u duration  set the signal to \"unknown\" state, then wait 'duration'.\n"
   "Example: @ 1.0 (2.0)1.0(2.0)\n" )},

  PROP_NOTEBOOK_PAGE("parameters",0,N_("Parameters")),
  { "start_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Start time"),NULL,&time_range},
  { "end_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("End time"),NULL,&time_range},
  { "rise_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Rise time"),NULL,&edge_range},
  { "fall_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Fall time"),NULL,&edge_range},
  { "multibit",PROP_TYPE_BOOL,PROP_FLAG_VISIBLE, N_("Multi-bit data"),NULL},

  PROP_NOTEBOOK_PAGE("aspect",0,N_("Aspect")),
  { "data_color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Data color"),NULL},
  { "data_lwidth", PROP_TYPE_LENGTH, PROP_FLAG_VISIBLE,
    N_("Data line width"),NULL, &prop_std_line_width_data},
  { "color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Line color"),NULL},
  { "main_lwidth", PROP_TYPE_LENGTH, PROP_FLAG_VISIBLE,
    N_("Line width"),NULL, &prop_std_line_width_data},

  { "font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE, N_("Font"), NULL, NULL },
  { "font_size", PROP_TYPE_FONTSIZE, PROP_FLAG_VISIBLE,
    N_("Font size"), NULL, &prop_std_text_height_data },
  { "font_color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Text color"), NULL, NULL },

  PROP_STD_NOTEBOOK_END,
  {NULL}
};

static PropDescription *
chronoline_describe_props(Chronoline *chronoline)
{
  if (chronoline_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(chronoline_props);
  }
  return chronoline_props;
}

static PropOffset chronoline_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  PROP_OFFSET_STD_NOTEBOOK_BEGIN,

  PROP_OFFSET_NOTEBOOK_PAGE("data"),
  { "name", PROP_TYPE_STRING, offsetof(Chronoline,name)},
  { "events", PROP_TYPE_MULTISTRING, offsetof(Chronoline,events)},
  { "help", PROP_TYPE_STATIC, 0},

  PROP_OFFSET_NOTEBOOK_PAGE("parameters"),
  { "start_time",PROP_TYPE_REAL, offsetof(Chronoline,start_time)},
  { "end_time",PROP_TYPE_REAL, offsetof(Chronoline,end_time)},
  { "rise_time",PROP_TYPE_REAL, offsetof(Chronoline,rise_time)},
  { "fall_time",PROP_TYPE_REAL, offsetof(Chronoline,fall_time)},
  { "multibit",PROP_TYPE_BOOL, offsetof(Chronoline,multibit)},

  PROP_OFFSET_NOTEBOOK_PAGE("aspect"),
  { "data_color", PROP_TYPE_COLOUR, offsetof(Chronoline,data_color)},
  { "data_lwidth", PROP_TYPE_LENGTH, offsetof(Chronoline,data_lwidth)},
  { "color", PROP_TYPE_COLOUR, offsetof(Chronoline,color)},
  { "main_lwidth", PROP_TYPE_LENGTH, offsetof(Chronoline,main_lwidth)},
  { "font", PROP_TYPE_FONT, offsetof(Chronoline,font)},
  { "font_size", PROP_TYPE_FONTSIZE, offsetof(Chronoline,font_size)},
  { "font_color", PROP_TYPE_COLOUR, offsetof(Chronoline,font_color)},

  PROP_OFFSET_STD_NOTEBOOK_END,
  {NULL}
};

static void
chronoline_get_props(Chronoline *chronoline, GPtrArray *props)
{
  object_get_props_from_offsets(&chronoline->element.object,
                                chronoline_offsets,props);
}

static void
chronoline_set_props(Chronoline *chronoline, GPtrArray *props)
{
  object_set_props_from_offsets(&chronoline->element.object,
                                chronoline_offsets,props);
  chronoline_update_data(chronoline);
}

static real
chronoline_distance_from(Chronoline *chronoline, Point *point)
{
  DiaObject *obj = &chronoline->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
chronoline_select(Chronoline *chronoline, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  element_update_handles(&chronoline->element);
}


static DiaObjectChange *
chronoline_move_handle (Chronoline       *chronoline,
                        Handle           *handle,
                        Point            *to,
                        ConnectionPoint  *cp,
                        HandleMoveReason  reason,
                        ModifierKeys      modifiers)
{
  g_assert(chronoline!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle (&chronoline->element,
                       handle->id,
                       to,
                       cp,
                       reason,
                       modifiers);
  chronoline_update_data (chronoline);

  return NULL;
}


static DiaObjectChange*
chronoline_move (Chronoline *chronoline, Point *to)
{
  chronoline->element.corner = *to;
  chronoline_update_data (chronoline);

  return NULL;
}


static void
cld_onebit (Chronoline  *chronoline,
            DiaRenderer *renderer,
            real         x1,
            CLEventType  s1,
            real         x2,
            CLEventType  s2,
            gboolean     fill)
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
      dia_renderer_draw_polygon (renderer,
                                 pts,
                                 sizeof(pts)/sizeof(pts[0]),
                                 &chronoline->datagray,
                                 NULL);
    } else {
      dia_renderer_draw_polygon (renderer,
                                 pts,
                                 sizeof(pts)/sizeof(pts[0]),
                                 &color_white,
                                 NULL);
    }
  } else {
    dia_renderer_draw_line (renderer,
                            &pts[1],
                            &pts[2],
                            &chronoline->data_color);
  }
}


static void
cld_multibit (Chronoline  *chronoline,
              DiaRenderer *renderer,
              real         x1,
              CLEventType  s1,
              real         x2,
              CLEventType  s2,
              gboolean     fill)
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
      dia_renderer_draw_polygon (renderer,
                                 pts,
                                 sizeof(pts)/sizeof(pts[0]),
                                 &chronoline->datagray,
                                 NULL);
    } else {
      dia_renderer_draw_polygon (renderer,
                                 pts,
                                 sizeof(pts)/sizeof(pts[0]),
                                 &color_white,
                                 NULL);
    }
  } else {
    dia_renderer_draw_line (renderer,
                            &pts[1],
                            &pts[2],
                            &chronoline->data_color);
    dia_renderer_draw_line (renderer,
                            &pts[0],
                            &pts[3],
                            &chronoline->data_color);
  }
}



inline static void
chronoline_draw_really (Chronoline  *chronoline,
                        DiaRenderer *renderer,
                        gboolean     fill)
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
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linewidth (renderer, chronoline->data_lwidth);

  while (lst) {
    evt = (CLEvent *)lst->data;
    g_assert(evt);
    if (evt->time >= start_time) {
      if (evt->time <= end_time) {
        /* regular point */
        newx = evt->x;

        if (chronoline->multibit) {
          cld_multibit(chronoline,renderer,oldx,state,newx,evt->type,fill);
        } else {
          cld_onebit(chronoline,renderer,oldx,state,newx,evt->type,fill);
        }
        oldx = newx;
      } else {
        newx = elem->corner.x + elem->width;
        if (!finished) {
          if (chronoline->multibit) {
            cld_multibit (chronoline,renderer,oldx,state,newx,evt->type,fill);
          } else {
            cld_onebit (chronoline,renderer,oldx,state,newx,evt->type,fill);
          }
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
chronoline_draw (Chronoline *chronoline, DiaRenderer *renderer)
{
  Element *elem;
  Point lr_corner;
  Point p1,p2,p3;

  g_assert(chronoline != NULL);
  g_assert(renderer != NULL);

  elem = &chronoline->element;

  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DOTTED, 1.0);
  dia_renderer_set_linewidth (renderer, chronoline->main_lwidth);
  p1.x = elem->corner.x + elem->width;
  p1.y = elem->corner.y;
  dia_renderer_draw_line (renderer,&elem->corner,&p1,&chronoline->gray);

  chronoline_draw_really (chronoline,renderer,TRUE);
  chronoline_draw_really (chronoline,renderer,FALSE);

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  p1.x = elem->corner.x;
  p2.x = lr_corner.x;
  p1.y = p2.y = chronoline->y_down;

  dia_renderer_set_linewidth (renderer, chronoline->main_lwidth);
  dia_renderer_draw_line (renderer,&p1,&p2,&chronoline->color);
  p2.x = p1.x = elem->corner.x;
  p1.y = chronoline->y_down;
  p2.y = chronoline->y_up;
  dia_renderer_draw_line (renderer,&p1,&p2,&chronoline->color);

  dia_renderer_set_font (renderer,
                         chronoline->font,
                         chronoline->font_size);
  p3.y = lr_corner.y - chronoline->font_size
      + dia_font_ascent (chronoline->name,
                         chronoline->font,
                         chronoline->font_size);
  p3.x = p1.x - chronoline->main_lwidth;
  dia_renderer_draw_string (renderer,
                            chronoline->name,
                            &p3,
                            DIA_ALIGN_RIGHT,
                            &chronoline->font_color);
}

inline static void grayify(Color *col,Color *src)
{
  col->red = .5 * (src->red + color_white.red);
  col->green = .5 * (src->green + color_white.green);
  col->blue = .5 * (src->blue + color_white.blue);
  col->alpha = .5 * (src->alpha + color_white.alpha);
}

static void
chronoline_update_data(Chronoline *chronoline)
{
  Element *elem = &chronoline->element;
  DiaObject *obj = &elem->object;
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

  chronoline->labelwidth = dia_font_string_width(chronoline->name,
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
	cp->directions = DIR_ALL;
      } else {
	cp->pos.y = (evt->type==CLE_OFF?
		     chronoline->y_down:chronoline->y_up);
	cp->directions = (evt->type==CLE_OFF?DIR_SOUTH:DIR_NORTH);
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

static DiaObject *
chronoline_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Chronoline *chronoline;
  Element *elem;
  DiaObject *obj;

  chronoline = g_new0(Chronoline,1);
  elem = &(chronoline->element);

  obj = &(chronoline->element.object);
  obj->type = &chronoline_type;
  obj->ops = &chronoline_ops;

  chronoline->snap = connpointline_create(obj,0);

  elem->corner = *startpoint;
  elem->width = 20.0;
  elem->height = 3.0;

  element_init(elem, 8, 0);

  chronoline->name = g_strdup("");
  chronoline->events = g_strdup("");

  chronoline->font = dia_font_new_from_style (DIA_FONT_SANS,1.0);
  chronoline->font_size = 1.0;
  chronoline->font_color = color_black;
  chronoline->start_time = 0.0;
  chronoline->end_time = 20.0;
  chronoline->rise_time = .3;
  chronoline->fall_time = .3;
  chronoline->color = color_black;
  chronoline->main_lwidth = .1;
  chronoline->data_lwidth = .1;
  chronoline->data_color.red = 1.0;
  chronoline->data_color.green = 0.0;
  chronoline->data_color.blue = 0.0;
  chronoline->data_color.alpha = 1.0;
  chronoline->multibit = FALSE;

  chronoline->evtlist = NULL;
  chronoline_update_data(chronoline);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &chronoline->element.object;
}

static void
chronoline_destroy (Chronoline *chronoline)
{
  g_clear_pointer (&chronoline->name, g_free);
  g_clear_pointer (&chronoline->events, g_free);
  g_clear_object (&chronoline->font);
  connpointline_destroy (chronoline->snap);
  destroy_clevent_list (chronoline->evtlist);
  element_destroy (&chronoline->element);
}

static DiaObject *
chronoline_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&chronoline_type,
                                      obj_node,version,ctx);
}
