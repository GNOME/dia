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
#include "pixmaps/chronoref.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0

typedef struct _Chronoref {
  Element element;

  real main_lwidth;
  real light_lwidth;
  Color color;
  real start_time;
  real end_time;
  real time_step;
  real time_lstep;

  DiaFont *font;
  real font_size;
  Color font_color;

  ConnPointLine *scale; /* not saved ; num_connections derived from
			   start_time, end_time, time_step. */
  real majgrad_height,mingrad_height;
  real firstmaj,firstmin; /* in time units */
  real firstmaj_x,firstmin_x,majgrad,mingrad; /* in dia graphic units */
  int  spec;
} Chronoref;

static real chronoref_distance_from(Chronoref *chronoref, Point *point);
static void chronoref_select(Chronoref *chronoref, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange *chronoref_move_handle  (Chronoref        *chronoref,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange *chronoref_move         (Chronoref        *chronoref,
                                                Point            *to);
static void chronoref_draw(Chronoref *chronoref, DiaRenderer *renderer);
static void chronoref_update_data(Chronoref *chronoref);
static DiaObject *chronoref_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void chronoref_destroy(Chronoref *chronoref);
static DiaObject *chronoref_load(ObjectNode obj_node, int version,
				 DiaContext *ctx);
static PropDescription *chronoref_describe_props(Chronoref *chronoref);
static void chronoref_get_props(Chronoref *chronoref,
                                 GPtrArray *props);
static void chronoref_set_props(Chronoref *chronoref,
                                 GPtrArray *props);



static ObjectTypeOps chronoref_type_ops =
{
  (CreateFunc) chronoref_create,
  (LoadFunc)   chronoref_load/*using properties*/,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType chronoref_type =
{
  "chronogram - reference", /* name */
  0,                     /* version */
  chronoref_xpm,          /* pixmap */
  &chronoref_type_ops        /* ops */
};


static ObjectOps chronoref_ops = {
  (DestroyFunc)         chronoref_destroy,
  (DrawFunc)            chronoref_draw,
  (DistanceFunc)        chronoref_distance_from,
  (SelectFunc)          chronoref_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            chronoref_move,
  (MoveHandleFunc)      chronoref_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   chronoref_describe_props,
  (GetPropsFunc)        chronoref_get_props,
  (SetPropsFunc)        chronoref_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData time_range = { -32767.0, 32768.0, 0.1};
static PropNumData step_range = { 0.0, 1000.0, 0.1};

static PropDescription chronoref_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_MULTICOL_BEGIN("chronoref"),

  PROP_MULTICOL_COLUMN("time"),
  PROP_FRAME_BEGIN("time",0,N_("Time data")),
  { "start_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Start time"),NULL,&time_range},
  { "end_time",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("End time"),NULL,&time_range},
  { "time_step",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Major time step"),NULL,&step_range},
  { "time_lstep",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Minor time step"),NULL,&step_range},
  PROP_FRAME_END("time",0),

  PROP_MULTICOL_COLUMN("aspect"),
  PROP_FRAME_BEGIN("aspect",0,N_("Aspect")),
  { "color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Line color"),NULL},
  { "main_lwidth", PROP_TYPE_LENGTH, PROP_FLAG_VISIBLE,
    N_("Line width"),NULL, &prop_std_line_width_data},
  { "light_lwidth", PROP_TYPE_LENGTH, PROP_FLAG_VISIBLE,
    N_("Minor step line width"),NULL, &prop_std_line_width_data},

  { "font", PROP_TYPE_FONT, PROP_FLAG_VISIBLE, N_("Font"), NULL, NULL },
  { "font_size", PROP_TYPE_FONTSIZE, PROP_FLAG_VISIBLE,
    N_("Font size"), NULL, &prop_std_text_height_data },
  { "font_color", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Text color"), NULL, NULL },
  PROP_FRAME_END("aspect",0),

  PROP_MULTICOL_END("chronoref"),
  {NULL}
};

static PropDescription *
chronoref_describe_props(Chronoref *chronoref)
{
  if (chronoref_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(chronoref_props);
  }
  return chronoref_props;
}

static PropOffset chronoref_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  PROP_OFFSET_MULTICOL_BEGIN("chronref"),

  PROP_OFFSET_MULTICOL_COLUMN("time"),
  PROP_OFFSET_FRAME_BEGIN("time"),
  { "start_time",PROP_TYPE_REAL, offsetof(Chronoref,start_time)},
  { "end_time",PROP_TYPE_REAL, offsetof(Chronoref,end_time)},
  { "time_step",PROP_TYPE_REAL, offsetof(Chronoref,time_step)},
  { "time_lstep",PROP_TYPE_REAL, offsetof(Chronoref,time_lstep)},
  PROP_OFFSET_FRAME_END("time"),

  PROP_OFFSET_MULTICOL_COLUMN("aspect"),
  PROP_OFFSET_FRAME_BEGIN("aspect"),
  { "color", PROP_TYPE_COLOUR, offsetof(Chronoref,color)},
  { "main_lwidth", PROP_TYPE_LENGTH, offsetof(Chronoref,main_lwidth)},
  { "light_lwidth", PROP_TYPE_LENGTH, offsetof(Chronoref,light_lwidth)},
  { "font", PROP_TYPE_FONT, offsetof(Chronoref,font)},
  { "font_size", PROP_TYPE_FONTSIZE, offsetof(Chronoref,font_size)},
  { "font_color", PROP_TYPE_COLOUR, offsetof(Chronoref,font_color)},
  PROP_OFFSET_FRAME_END("aspect"),

  PROP_OFFSET_MULTICOL_END("chronref"),
  {NULL}
};

static void
chronoref_get_props(Chronoref *chronoref, GPtrArray *props)
{
  object_get_props_from_offsets(&chronoref->element.object,
                                chronoref_offsets,props);
}

static void
chronoref_set_props(Chronoref *chronoref, GPtrArray *props)
{
  object_set_props_from_offsets(&chronoref->element.object,
                                chronoref_offsets,props);
  chronoref_update_data(chronoref);
}

static real
chronoref_distance_from(Chronoref *chronoref, Point *point)
{
  DiaObject *obj = &chronoref->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
chronoref_select(Chronoref *chronoref, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  element_update_handles(&chronoref->element);
}


static DiaObjectChange *
chronoref_move_handle (Chronoref        *chronoref,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  g_assert(chronoref!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle (&chronoref->element,
                       handle->id,
                       to,
                       cp,
                       reason,
                       modifiers);
  chronoref_update_data (chronoref);

  return NULL;
}


static DiaObjectChange *
chronoref_move (Chronoref *chronoref, Point *to)
{
  chronoref->element.corner = *to;
  chronoref_update_data (chronoref);

  return NULL;
}


static void
chronoref_draw (Chronoref *chronoref, DiaRenderer *renderer)
{
  Element *elem;
  Point lr_corner;
  real t;
  Point p1,p2,p3;

  g_return_if_fail (renderer != NULL);

  elem = &chronoref->element;

  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);


  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  p1.y = p2.y = elem->corner.y;

  dia_renderer_set_font (renderer, chronoref->font, chronoref->font_size);
  p3.y = p2.y + chronoref->majgrad_height +
    dia_font_ascent ("1",chronoref->font, chronoref->font_size);

  dia_renderer_set_linewidth (renderer, chronoref->light_lwidth);
  if (chronoref->time_lstep > 0.0) {
    p2.y = p1.y + chronoref->mingrad_height;
    for (t = chronoref->firstmaj, p1.x = chronoref->firstmin_x;
         p1.x <= lr_corner.x;
         t += chronoref->time_lstep, p1.x += chronoref->mingrad) {
      p2.x = p1.x;

      dia_renderer_draw_line (renderer,&p1,&p2,&chronoref->color);
    }
  }

  dia_renderer_set_linewidth (renderer, chronoref->main_lwidth);
  if (chronoref->time_step > 0.0) {
    p2.y = p1.y + chronoref->majgrad_height;

    for (t = chronoref->firstmaj, p1.x = chronoref->firstmaj_x;
         p1.x <= lr_corner.x;
         t += chronoref->time_step, p1.x += chronoref->majgrad) {
      char time[10];
      p3.x = p2.x = p1.x;

      dia_renderer_draw_line (renderer,&p1,&p2,&chronoref->color);
      g_snprintf (time,sizeof(time),"%.*f",chronoref->spec,t);
      dia_renderer_draw_string (renderer,
                                time,
                                &p3,
                                DIA_ALIGN_CENTRE,
                                &chronoref->font_color);
    }
  }
  p1.x = elem->corner.x;
  p2.x = lr_corner.x;
  p1.y = p2.y = elem->corner.y;

  dia_renderer_draw_line (renderer,&p1,&p2,&chronoref->color);
}

static void
chronoref_update_data(Chronoref *chronoref)
{
  Element *elem = &chronoref->element;
  DiaObject *obj = &elem->object;
  real time_span,t;
  Point p1,p2;
  Point ur_corner;
  int shouldbe,i;
  real labelwidth;
  char biglabel[10];
  ElementBBExtras *extra = &elem->extra_spacing;

  chronoref->majgrad_height = elem->height;
  chronoref->mingrad_height = elem->height / 3.0;

  /* build i = -log_{10}(time_step), then make a %.if format out of it. */
  t = 1;
  i = 0;

  while (t > chronoref->time_step) {
    t /= 10;
    i++;
  }
  chronoref->spec = i; /* update precision */
  g_snprintf(biglabel,sizeof(biglabel),"%.*f", chronoref->spec,
	   MIN(-ABS(chronoref->start_time),-ABS(chronoref->end_time)));

  labelwidth = dia_font_string_width(biglabel,chronoref->font,
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

  extra->border_trans = chronoref->main_lwidth/2;
  element_update_boundingbox(elem);

  /* fix boundingbox for special extras: */
  obj->bounding_box.left -= (chronoref->font_size + labelwidth)/2;
  obj->bounding_box.bottom += chronoref->font_size;
  obj->bounding_box.right += (chronoref->font_size + labelwidth)/2;

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
  connpointline_putonaline(chronoref->scale,&p1,&p2, DIR_SOUTH);
}

static DiaObject *
chronoref_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Chronoref *chronoref;
  Element *elem;
  DiaObject *obj;

  chronoref = g_new0(Chronoref,1);
  elem = &(chronoref->element);

  obj =  &(chronoref->element.object);
  obj->type = &chronoref_type;
  obj->ops = &chronoref_ops;

  chronoref->scale = connpointline_create(obj,0);

  elem->corner = *startpoint;
  elem->width = 20.0;
  elem->height = 1.0;

  element_init(elem, 8, 0);

  chronoref->font = dia_font_new_from_style (DIA_FONT_SANS,1.0);
  chronoref->font_size = 1.0;
  chronoref->font_color = color_black;
  chronoref->start_time = 0.0;
  chronoref->end_time = 20.0;
  chronoref->time_step = 5.0;
  chronoref->time_lstep = 1.0;
  chronoref->color = color_black;
  chronoref->main_lwidth = .1;
  chronoref->light_lwidth = .05;

  chronoref_update_data(chronoref);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &chronoref->element.object;
}

static void
chronoref_destroy(Chronoref *chronoref)
{
  g_clear_object (&chronoref->font);
  connpointline_destroy (chronoref->scale);
  element_destroy (&chronoref->element);
}

static DiaObject *
chronoref_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&chronoref_type,
                                      obj_node,version,ctx);
}
