/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Analog clock object
 * Copyright (C) 2002 Cyrille Chepelov
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
#include <time.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "color.h"
#include "properties.h"
#include "dynamic_obj.h"

#include "pixmaps/analog_clock.xpm"

typedef struct _Chronoline {
  Element element;

  ConnectionPoint hours[12];
  ConnectionPoint hour_tip, min_tip, sec_tip;
  ConnectionPoint center_cp;

  Color border_color;
  real border_line_width;
  Color inner_color;
  gboolean show_background;
  Color arrow_color;
  real arrow_line_width;
  Color sec_arrow_color;
  real sec_arrow_line_width;
  gboolean show_ticks;

  Point centre; /* computed */
  real radius;  /* computed */
} Analog_Clock;


static real analog_clock_distance_from(Analog_Clock *analog_clock,
                                       Point *point);

static void analog_clock_select(Analog_Clock *analog_clock,
                                Point *clicked_point,
                                DiaRenderer *interactive_renderer);
static DiaObjectChange* analog_clock_move_handle(Analog_Clock *analog_clock,
					      Handle *handle, Point *to,
					      ConnectionPoint *cp, HandleMoveReason reason,
                                     ModifierKeys modifiers);
static DiaObjectChange* analog_clock_move(Analog_Clock *analog_clock, Point *to);
static void analog_clock_draw(Analog_Clock *analog_clock, DiaRenderer *renderer);
static void analog_clock_update_data(Analog_Clock *analog_clock);
static DiaObject *analog_clock_create(Point *startpoint,
                                   void *user_data,
                                   Handle **handle1,
                                   Handle **handle2);
static void analog_clock_destroy(Analog_Clock *analog_clock);
static DiaObject *analog_clock_load(ObjectNode obj_node, int version,
                                    DiaContext *ctx);
static PropDescription *analog_clock_describe_props(
  Analog_Clock *analog_clock);
static void analog_clock_get_props(Analog_Clock *analog_clock,
                                   GPtrArray *props);
static void analog_clock_set_props(Analog_Clock *analog_clock,
                                   GPtrArray *props);

static ObjectTypeOps analog_clock_type_ops =
{
  (CreateFunc) analog_clock_create,
  (LoadFunc)   analog_clock_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType analog_clock_type =
{
  "Misc - Analog Clock", /* name */
  0,                  /* version */
  analog_clock_xpm,    /* pixmap */
  &analog_clock_type_ops  /* ops */
};

static ObjectOps analog_clock_ops = {
  (DestroyFunc)         analog_clock_destroy,
  (DrawFunc)            analog_clock_draw,
  (DistanceFunc)        analog_clock_distance_from,
  (SelectFunc)          analog_clock_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            analog_clock_move,
  (MoveHandleFunc)      analog_clock_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   analog_clock_describe_props,
  (GetPropsFunc)        analog_clock_get_props,
  (SetPropsFunc)        analog_clock_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription analog_clock_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,

  { "arrow_colour", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Arrow color"), NULL, NULL },
  { "arrow_line_width", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Arrow line width"), NULL,NULL },
  { "sec_arrow_colour", PROP_TYPE_COLOUR, PROP_FLAG_VISIBLE,
    N_("Seconds arrow color"), NULL, NULL },
  { "sec_arrow_line_width", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Seconds arrow line width"), NULL,NULL },
  { "show_ticks", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("Show hours"), NULL, NULL },

  {NULL}
};

static PropDescription *
analog_clock_describe_props(Analog_Clock *analog_clock)
{
  if (analog_clock_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(analog_clock_props);
  }
  return analog_clock_props;
}

static PropOffset analog_clock_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Analog_Clock, border_line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Analog_Clock, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Analog_Clock,inner_color) },
  { "show_background", PROP_TYPE_BOOL,offsetof(Analog_Clock,show_background) },
  { "arrow_colour", PROP_TYPE_COLOUR, offsetof(Analog_Clock, arrow_color) },
  { "arrow_line_width", PROP_TYPE_REAL, offsetof(Analog_Clock,
                                                 arrow_line_width) },
  { "sec_arrow_colour", PROP_TYPE_COLOUR,
    offsetof(Analog_Clock, sec_arrow_color) },
  { "sec_arrow_line_width", PROP_TYPE_REAL,
    offsetof(Analog_Clock, sec_arrow_line_width) },

  { "show_ticks", PROP_TYPE_BOOL,offsetof(Analog_Clock,show_ticks) },

  {NULL}
};

static void
analog_clock_get_props(Analog_Clock *analog_clock, GPtrArray *props)
{
  object_get_props_from_offsets(&analog_clock->element.object,
                                analog_clock_offsets,props);
}

static void
analog_clock_set_props(Analog_Clock *analog_clock, GPtrArray *props)
{
  object_set_props_from_offsets(&analog_clock->element.object,
                                analog_clock_offsets,props);
  analog_clock_update_data(analog_clock);
}

static real
analog_clock_distance_from(Analog_Clock *analog_clock, Point *point)
{
  DiaObject *obj = &analog_clock->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
analog_clock_select(Analog_Clock *analog_clock, Point *clicked_point,
		    DiaRenderer *interactive_renderer)
{
  element_update_handles(&analog_clock->element);
}

static DiaObjectChange*
analog_clock_move_handle(Analog_Clock *analog_clock, Handle *handle,
			 Point *to, ConnectionPoint *cp,
			 HandleMoveReason reason, ModifierKeys modifiers)
{
  g_assert(analog_clock!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  element_move_handle(&analog_clock->element, handle->id, to, cp,
		      reason, modifiers);
  analog_clock_update_data(analog_clock);

  return NULL;
}

static DiaObjectChange*
analog_clock_move(Analog_Clock *analog_clock, Point *to)
{
  analog_clock->element.corner = *to;
  analog_clock_update_data(analog_clock);

  return NULL;
}

static void make_angle(const Point *centre, real degrees, real radius,
                       Point *pt)
{
  real radians = ((90 - degrees) * M_PI) / 180.0;
  pt->x = centre->x + radius * cos( radians );
  pt->y = centre->y - radius * sin( radians );
}

static void make_hours(const Point *centre, unsigned hours, unsigned minutes, real radius,
                       Point *pt)
{
  while (hours > 11) hours -= 12;

  make_angle(centre,((real)hours) * 360.0 / 12.0 + ((real)minutes) * 360.0 / 12.0 / 60.0 ,radius,pt);
}

static void make_minutes(const Point *centre, unsigned minutes,
                         real radius, Point *pt)
{
  make_angle(centre,((real)minutes) * 360.0 / 60.0,radius,pt);
}

static void
analog_clock_update_arrow_tips(Analog_Clock *analog_clock)
{
  time_t now;
  struct tm *local;

  now = time(NULL);
  local = localtime(&now);
  analog_clock->hour_tip.directions = DIR_ALL;
  analog_clock->min_tip.directions = DIR_ALL;
  analog_clock->sec_tip.directions = DIR_ALL;
  if (local) {
    make_hours(&analog_clock->centre,local->tm_hour,local->tm_min,
               0.50 * analog_clock->radius, &analog_clock->hour_tip.pos);
    make_minutes(&analog_clock->centre,local->tm_min,
                 0.80 * analog_clock->radius, &analog_clock->min_tip.pos);
    make_minutes(&analog_clock->centre,local->tm_sec,
                 0.85 * analog_clock->radius, &analog_clock->sec_tip.pos);
  } else {
        /* Highly unlikely */
    point_copy(&analog_clock->hour_tip.pos,&analog_clock->centre);
    point_copy(&analog_clock->min_tip.pos,&analog_clock->centre);
    point_copy(&analog_clock->sec_tip.pos,&analog_clock->centre);
  }
}


static void
analog_clock_update_data(Analog_Clock *analog_clock)
{
  Element *elem = &analog_clock->element;
  DiaObject *obj = &elem->object;
  int i;
  ElementBBExtras *extra = &elem->extra_spacing;

  extra->border_trans = analog_clock->border_line_width / 2;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

  analog_clock->centre.x = obj->position.x + elem->width/2;
  analog_clock->centre.y = obj->position.y + elem->height/2;

  analog_clock->radius = MIN(elem->width/2,elem->height/2);

  /* Update connections: */
  for (i = 0; i < 12; ++i)
  {
    make_hours(&analog_clock->centre, i+1, 0, analog_clock->radius,
               &analog_clock->hours[i].pos);
    analog_clock->hours[i].directions = DIR_ALL;
  }
  analog_clock->center_cp.pos.x = elem->corner.x + elem->width/2;
  analog_clock->center_cp.pos.y = elem->corner.y + elem->height/2;

  analog_clock_update_arrow_tips(analog_clock);
}

static void
analog_clock_draw (Analog_Clock *analog_clock, DiaRenderer *renderer)
{
  g_assert(analog_clock != NULL);
  g_assert(renderer != NULL);

  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0);
  dia_renderer_set_linewidth (renderer, analog_clock->border_line_width);

  dia_renderer_draw_ellipse (renderer,
                             &analog_clock->centre,
                             2*analog_clock->radius,2*analog_clock->radius,
                             (analog_clock->show_background) ? &analog_clock->inner_color : NULL,
                             &analog_clock->border_color);
  if (analog_clock->show_ticks) {
    Point out, in;
    unsigned i;

    for (i = 0; i < 12; ++i) {
      real ticklen;
      switch(i) {
        case 0:
          ticklen = 3.5 * analog_clock->border_line_width; break;
        case 3: case 6: case 9:
          ticklen = 3 * analog_clock->border_line_width; break;
        default:
          ticklen = 2 * analog_clock->border_line_width; break;
      }
      make_hours (&analog_clock->centre, i, 0,
                  analog_clock->radius, &out);
      make_hours (&analog_clock->centre, i, 0,
                  analog_clock->radius-ticklen, &in);
      dia_renderer_draw_line (renderer, &out, &in, &analog_clock->border_color);
    }
  }

  analog_clock_update_arrow_tips (analog_clock);

  dia_renderer_set_linewidth (renderer, analog_clock->arrow_line_width);
  dia_renderer_draw_line (renderer,
                          &analog_clock->hour_tip.pos,
                          &analog_clock->centre,
                          &analog_clock->arrow_color);
  dia_renderer_draw_line (renderer,
                          &analog_clock->min_tip.pos,
                          &analog_clock->centre,
                          &analog_clock->arrow_color);

  dia_renderer_set_linewidth (renderer, analog_clock->sec_arrow_line_width);
  dia_renderer_draw_line (renderer,
                          &analog_clock->sec_tip.pos,
                          &analog_clock->centre,
                          &analog_clock->sec_arrow_color);
  dia_renderer_draw_ellipse (renderer,
                             &analog_clock->centre,
                             analog_clock->arrow_line_width*2.25,
                             analog_clock->arrow_line_width*2.25,
                             &analog_clock->sec_arrow_color, NULL);
}


static DiaObject *
analog_clock_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Analog_Clock *analog_clock;
  Element *elem;
  DiaObject *obj;
  unsigned i;

  analog_clock = g_new0(Analog_Clock,1);
  elem = &(analog_clock->element);

  obj = &(analog_clock->element.object);
  obj->type = &analog_clock_type;
  obj->ops = &analog_clock_ops;

  elem->corner = *startpoint;
  elem->width = 4.0;
  elem->height = 4.0;

  element_init(elem, 8, 16);

  analog_clock->border_color = attributes_get_foreground();
  analog_clock->border_line_width = attributes_get_default_linewidth();
  analog_clock->inner_color = attributes_get_background();
  analog_clock->show_background = TRUE;
  analog_clock->arrow_color.red = 0.0;
  analog_clock->arrow_color.green = 0.0;
  analog_clock->arrow_color.blue = 0.5;
  analog_clock->arrow_color.alpha = 1.0;
  analog_clock->arrow_line_width = attributes_get_default_linewidth();
  analog_clock->sec_arrow_color.red = 1.0;
  analog_clock->sec_arrow_color.green = 0.0;
  analog_clock->sec_arrow_color.blue = 0.0;
  analog_clock->sec_arrow_color.alpha = 1.0;
  analog_clock->sec_arrow_line_width = attributes_get_default_linewidth()/3;
  analog_clock->show_ticks = TRUE;

  for (i = 0; i < 12; ++i)
  {
    obj->connections[i] = &analog_clock->hours[i];
    analog_clock->hours[i].object = obj;
    analog_clock->hours[i].connected = NULL;
  }
  obj->connections[12] = &analog_clock->hour_tip;
  analog_clock->hour_tip.object = obj;
  analog_clock->hour_tip.connected = NULL;
  obj->connections[13] = &analog_clock->min_tip;
  analog_clock->min_tip.object = obj;
  analog_clock->min_tip.connected = NULL;
  obj->connections[14] = &analog_clock->sec_tip;
  analog_clock->sec_tip.object = obj;
  analog_clock->sec_tip.connected = NULL;
  obj->connections[15] = &analog_clock->center_cp;
  analog_clock->center_cp.object = obj;
  analog_clock->center_cp.connected = NULL;
  analog_clock->center_cp.flags = CP_FLAGS_MAIN;

  analog_clock->hours[0].directions = DIR_NORTH;
  analog_clock->hours[1].directions = DIR_NORTH|DIR_EAST;
  analog_clock->hours[2].directions = DIR_NORTH|DIR_EAST;
  analog_clock->hours[3].directions = DIR_EAST;
  analog_clock->hours[4].directions = DIR_EAST|DIR_SOUTH;
  analog_clock->hours[5].directions = DIR_EAST|DIR_SOUTH;
  analog_clock->hours[6].directions = DIR_SOUTH;
  analog_clock->hours[7].directions = DIR_SOUTH|DIR_WEST;
  analog_clock->hours[8].directions = DIR_SOUTH|DIR_WEST;
  analog_clock->hours[9].directions = DIR_WEST;
  analog_clock->hours[10].directions = DIR_WEST|DIR_NORTH;
  analog_clock->hours[11].directions = DIR_WEST|DIR_NORTH;
  analog_clock->center_cp.directions = DIR_ALL;

  analog_clock_update_data(analog_clock);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

      /* We are an animated object -- special case ! */
  dynobj_list_add_object(&analog_clock->element.object,1000);

  return &analog_clock->element.object;
}

static void
analog_clock_destroy(Analog_Clock *analog_clock)
{
      /* We are an animated object -- special case ! */
    dynobj_list_remove_object(&analog_clock->element.object);
    element_destroy(&analog_clock->element);
}

static DiaObject *
analog_clock_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&analog_clock_type,
                                      obj_node,version,ctx);
}
