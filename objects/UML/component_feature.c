/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Copyright (C) 2003 W. Borgert <debacle@debian.org>
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

/*
 * Derived from association.c and implements.c
 * 2003-08-13, 2003-08-15: W. Borgert <debacle@debian.org>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "handle.h"
#include "properties.h"
#include "text.h"

#include "pixmaps/facet.xpm"

typedef struct _Compfeat Compfeat;

typedef enum {
  COMPPROP_FACET,
  COMPPROP_RECEPTACLE,
  COMPPROP_EVENTSOURCE,
  COMPPROP_EVENTSINK,
} CompRole;

struct _Compfeat {
  OrthConn orth;

  ConnectionPoint cp;

  CompRole role;
  CompRole roletmp;

  Text *text;
  TextAttributes attrs;
};

#define COMPPROP_WIDTH 0.1
#define COMPPROP_FONTHEIGHT 0.8
#define COMPPROP_DIAMETER 0.8
#define COMPPROP_DEFAULTLEN 2.0
#define COMPPROP_TEXTOFFSET 0.8

static ObjectChange* compfeat_move_handle(Compfeat *compfeat,
					  Handle *handle,
					  Point *to,
					  ConnectionPoint *cp,
					  HandleMoveReason reason,
					  ModifierKeys modifiers);
static ObjectChange* compfeat_move(Compfeat *compfeat, Point *to);
static void compfeat_select(Compfeat *compfeat, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static void compfeat_draw(Compfeat *compfeat, DiaRenderer *renderer);
static Object *compfeat_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static real compfeat_distance_from(Compfeat *compfeat, Point *point);
static void compfeat_update_data(Compfeat *compfeat);
static void compfeat_destroy(Compfeat *compfeat);
static DiaMenu *compfeat_get_object_menu(Compfeat *compfeat,
					 Point *clickedpoint);
static PropDescription *compfeat_describe_props(Compfeat *compfeat);
static void compfeat_get_props(Compfeat * compfeat, GPtrArray *props);
static void compfeat_set_props(Compfeat * compfeat, GPtrArray *props);
static Object *compfeat_load(ObjectNode obj_node, int version,
			     const char *filename);


static ObjectTypeOps compfeat_type_ops =
{
  (CreateFunc) compfeat_create,
  (LoadFunc)   compfeat_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType compfeat_type =
{
  "UML - Component Feature",	/* name */
  0,				/* version */
  (char **) facet_xpm,		/* pixmap */
  &compfeat_type_ops,		/* ops */
  NULL,				/* pixmap file */
  0				/* default user data */
};

static ObjectOps compfeat_ops = {
  (DestroyFunc)         compfeat_destroy,
  (DrawFunc)            compfeat_draw,
  (DistanceFunc)        compfeat_distance_from,
  (SelectFunc)          compfeat_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            compfeat_move,
  (MoveHandleFunc)      compfeat_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      compfeat_get_object_menu,
  (DescribePropsFunc)   compfeat_describe_props,
  (GetPropsFunc)        compfeat_get_props,
  (SetPropsFunc)        compfeat_set_props
};

static PropEnumData prop_compfeat_type_data[] = {
  { N_("Facet"), COMPPROP_FACET },
  { N_("Receptacle"), COMPPROP_RECEPTACLE },
  { N_("Event Source"), COMPPROP_EVENTSOURCE },
  { N_("Event Sink"), COMPPROP_EVENTSINK },
  { NULL, 0}
};

static PropDescription compfeat_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "role", PROP_TYPE_ENUM, 0, NULL, NULL, prop_compfeat_type_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_DESC_END
};

static ObjectChange *
compfeat_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  compfeat_update_data((Compfeat *)obj);
  return change;
}

static ObjectChange *
compfeat_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  compfeat_update_data((Compfeat *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), compfeat_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), compfeat_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Association",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
compfeat_get_object_menu(Compfeat *compfeat, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &compfeat->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active =
    orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active =
    orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);
  return &object_menu;
}

static PropDescription *
compfeat_describe_props(Compfeat *compfeat)
{
  if (compfeat_props[0].quark == 0)
    prop_desc_list_calculate_quarks(compfeat_props);
  return compfeat_props;
}

static PropOffset compfeat_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "role", PROP_TYPE_ENUM, offsetof(Compfeat, role) },
  { "text", PROP_TYPE_TEXT, offsetof(Compfeat, text) },
  { "text_font", PROP_TYPE_FONT, offsetof(Compfeat, attrs.font) },
  { "text_height", PROP_TYPE_REAL, offsetof(Compfeat, attrs.height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(Compfeat, attrs.color) },
  { NULL, 0, 0 }
};

static void
compfeat_get_props(Compfeat *compfeat, GPtrArray *props)
{
  if (compfeat->roletmp)
    compfeat->role = compfeat->roletmp;
  text_get_attributes(compfeat->text, &compfeat->attrs);
  object_get_props_from_offsets(&compfeat->orth.object,
                                compfeat_offsets, props);
}

static void
compfeat_set_props(Compfeat *compfeat, GPtrArray *props)
{
  object_set_props_from_offsets(&compfeat->orth.object,
                                compfeat_offsets, props);
  apply_textattr_properties(props, compfeat->text, "text", &compfeat->attrs);
  compfeat_update_data(compfeat);
}

static real
compfeat_distance_from(Compfeat *compfeat, Point *point)
{
  OrthConn *orth = &compfeat->orth;
  return orthconn_distance_from(orth, point, COMPPROP_WIDTH);
}

static void
compfeat_select(Compfeat *compfeat, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  text_set_cursor(compfeat->text, clicked_point, interactive_renderer);
  text_grab_focus(compfeat->text, &compfeat->orth.object);
  orthconn_update_data(&compfeat->orth);
}

static ObjectChange*
compfeat_move_handle(Compfeat *compfeat, Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason,
		     ModifierKeys modifiers)
{
  ObjectChange *change;

  assert(compfeat!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  change = orthconn_move_handle(&compfeat->orth, handle, to, cp, 
				reason, modifiers);
  compfeat_update_data(compfeat);

  return change;
}

static ObjectChange*
compfeat_move(Compfeat *compfeat, Point *to)
{
  ObjectChange *change;

  change = orthconn_move(&compfeat->orth, to);
  compfeat_update_data(compfeat);

  return change;
}

static void
compfeat_draw(Compfeat *compfeat, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point *points;
  Point p;
  Point save0;
  OrthConn *orth = &compfeat->orth;
  int n;
  gchar directions;

  assert(compfeat != NULL);
  assert(renderer != NULL);

  points = &orth->points[0];
  n = orth->numpoints;

  renderer_ops->set_linewidth(renderer, COMPPROP_WIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  save0 = points[0];
  if (compfeat->orth.orientation[0] == HORIZONTAL) {
    directions = (save0.x > points[1].x)? DIR_EAST: DIR_WEST;
  } else {
    directions = (save0.y > points[1].y)? DIR_SOUTH: DIR_NORTH;
  }

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE)
    compfeat->cp.directions = directions;

  switch (directions) {
  case DIR_EAST:
    points[0].x -= COMPPROP_DIAMETER;
    p = save0;
    p.x -= COMPPROP_DIAMETER/2.0;
    break;
  case DIR_WEST:
    points[0].x += COMPPROP_DIAMETER;
    p = save0;
    p.x += COMPPROP_DIAMETER/2.0;
    break;
  case DIR_SOUTH:
    points[0].y -= COMPPROP_DIAMETER;
    p = save0;
    p.y -= COMPPROP_DIAMETER/2.0;
    break;
  case DIR_NORTH:
    points[0].y += COMPPROP_DIAMETER;
    p = save0;
    p.y += COMPPROP_DIAMETER/2.0;
    break;
  }

  renderer_ops->draw_polyline(renderer, points, n, &color_black);

  if (compfeat->role == COMPPROP_FACET) {
    renderer_ops->fill_ellipse(renderer, &p,
			       COMPPROP_DIAMETER, COMPPROP_DIAMETER,
			       &color_white);
    renderer_ops->draw_ellipse(renderer, &p,
			       COMPPROP_DIAMETER, COMPPROP_DIAMETER,
			       &color_black);
  } else if (compfeat->role == COMPPROP_RECEPTACLE) {
    real start = 90.0, end = 270.0;

    switch (directions) {
    case DIR_WEST:
      start = 270.0;
      end = 90.0;
      break;
    case DIR_SOUTH:
      start = 0.0;
      end = 180.0;
      break;
    case DIR_NORTH:
      start = 180.0;
      end = 0.0;
      break;
    }
    renderer_ops->draw_arc(renderer, &p,
			   COMPPROP_DIAMETER, COMPPROP_DIAMETER,
			   start, end, &color_black);
  } else if (compfeat->role == COMPPROP_EVENTSOURCE) {
    Point poly[4];

    poly[0].x = p.x + COMPPROP_DIAMETER/2.0;
    poly[0].y = p.y;
    poly[1].x = p.x;
    poly[1].y = p.y - COMPPROP_DIAMETER/2.0;
    poly[2].x = p.x - COMPPROP_DIAMETER/2.0;
    poly[2].y = p.y;
    poly[3].x = p.x;
    poly[3].y = p.y + COMPPROP_DIAMETER/2.0;
    renderer_ops->fill_polygon(renderer, poly, 4, &color_white);
    renderer_ops->draw_polygon(renderer, poly, 4, &color_black);
  } else if (compfeat->role == COMPPROP_EVENTSINK) {
    Point poly[3];

    poly[0] = poly[2] = p;
    poly[1] = points[0];
    switch (directions) {
    case DIR_EAST:
      poly[0].y -= COMPPROP_DIAMETER/2.0;
      poly[2].y += COMPPROP_DIAMETER/2.0;
      break;
    case DIR_WEST:
      poly[0].y += COMPPROP_DIAMETER/2.0;
      poly[2].y -= COMPPROP_DIAMETER/2.0;
      break;
    case DIR_SOUTH:
      poly[0].x += COMPPROP_DIAMETER/2.0;
      poly[2].x -= COMPPROP_DIAMETER/2.0;
      break;
    case DIR_NORTH:
      poly[0].x -= COMPPROP_DIAMETER/2.0;
      poly[2].x += COMPPROP_DIAMETER/2.0;
      break;
    }
    renderer_ops->draw_polyline(renderer, poly, 3, &color_black);
  }

  text_draw(compfeat->text, renderer);

  /* restore old value */
  points[0] = save0;
}

static Object *
compfeat_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Compfeat *compfeat;
  OrthConn *orth;
  Object *obj;
  Point p;
  DiaFont *font;

  font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);

  compfeat = g_new0(Compfeat, 1);
  compfeat->role = compfeat->roletmp = GPOINTER_TO_INT(user_data);

  orth = &compfeat->orth;
  obj = &orth->object;

  obj->type = &compfeat_type;
  obj->ops = &compfeat_ops;

  orthconn_init(orth, startpoint);

  p = *startpoint;

  compfeat->text = new_text("", font,
			    COMPPROP_FONTHEIGHT, &p, &color_black,
			    ALIGN_CENTER);
  dia_font_unref(font);
  text_get_attributes(compfeat->text, &compfeat->attrs);

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE) {
    object_add_connectionpoint(&orth->object, &compfeat->cp);
    obj->connections[0] = &compfeat->cp;
    compfeat->cp.object = obj;
    compfeat->cp.connected = NULL;
  }

  compfeat_update_data(compfeat);

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return &compfeat->orth.object;
}


static void
compfeat_destroy(Compfeat *compfeat)
{
  text_destroy(compfeat->text);
  orthconn_destroy(&compfeat->orth);
}

static void
compfeat_update_data(Compfeat *compfeat)
{
  OrthConn *orth = &compfeat->orth;
  PolyBBExtras *extra = &orth->extra_spacing;
  int n;
  Object *obj = &orth->object;
  Rectangle rect;
  Point p;
  Point *points;

  points = &orth->points[0];
  n = orth->numpoints;

  obj->position = points[0];

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE)
    compfeat->cp.pos = points[0];

  orthconn_update_data(orth);

  /* Boundingbox: */
  extra->start_long =
    extra->start_trans =
    extra->end_long = COMPPROP_WIDTH + COMPPROP_DIAMETER;
  extra->end_trans = COMPPROP_WIDTH + COMPPROP_DIAMETER;

  orthconn_update_boundingbox(orth);

  /* Add boundingbox for text: */
  p.x = (points[0].x < points[1].x)?
    points[0].x + COMPPROP_TEXTOFFSET:
    points[0].x - COMPPROP_TEXTOFFSET;
  p.y = points[0].y
    - (compfeat->text->height * compfeat->text->numlines);
  text_set_alignment(compfeat->text,
                     (points[0].x < points[1].x)?
                     ALIGN_LEFT: ALIGN_RIGHT);
  text_set_position(compfeat->text, &p);
  text_calc_boundingbox(compfeat->text, &rect);
  rectangle_union(&obj->bounding_box, &rect);
}

static Object *
compfeat_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&compfeat_type,
				      obj_node, version, filename);
}
