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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "handle.h"
#include "properties.h"
#include "text.h"
#include "arrows.h"

#include "pixmaps/facet.xpm"

typedef struct _Compfeat Compfeat;

typedef enum {
  COMPPROP_FACET,
  COMPPROP_RECEPTACLE,
  COMPPROP_EVENTSOURCE,
  COMPPROP_EVENTSINK,
} CompRole;

static int compprop_arrow[] = {
  ARROW_HOLLOW_ELLIPSE,
  ARROW_OPEN_ROUNDED,
  ARROW_HOLLOW_DIAMOND,
  ARROW_HALF_DIAMOND,
};

struct _Compfeat {
  OrthConn orth;

  ConnectionPoint cp;

  CompRole role;
  CompRole roletmp;

  Text *text;
  Point text_pos;
  Handle text_handle;

  Color line_color;
  real  line_width;
};

#define COMPPROP_FONTHEIGHT 0.8
#define COMPPROP_DIAMETER 0.8
#define COMPPROP_DEFAULTLEN 2.0
#define COMPPROP_TEXTOFFSET 1.0
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static DiaObjectChange *compfeat_move_handle     (Compfeat         *compfeat,
                                                  Handle           *handle,
                                                  Point            *to,
                                                  ConnectionPoint  *cp,
                                                  HandleMoveReason  reason,
                                                  ModifierKeys      modifiers);
static DiaObjectChange *compfeat_move            (Compfeat         *compfeat,
                                                  Point            *to);
static void compfeat_select(Compfeat *compfeat, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static void compfeat_draw(Compfeat *compfeat, DiaRenderer *renderer);
static DiaObject *compfeat_create(Point *startpoint,
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
static DiaObject *compfeat_load(ObjectNode obj_node, int version,DiaContext *ctx);


static ObjectTypeOps compfeat_type_ops =
{
  (CreateFunc) compfeat_create,
  (LoadFunc)   compfeat_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType compfeat_type =
{
  "UML - Component Feature", /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                         /* version */
  facet_xpm,		     /* pixmap */
  &compfeat_type_ops,        /* ops */
  NULL,	                     /* pixmap file */
  0,                         /* default user data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      compfeat_get_object_menu,
  (DescribePropsFunc)   compfeat_describe_props,
  (GetPropsFunc)        compfeat_get_props,
  (SetPropsFunc)        compfeat_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
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
  { "role", PROP_TYPE_ENUM, PROP_FLAG_NO_DEFAULTS, NULL, NULL, prop_compfeat_type_data },
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT_ALIGNMENT,
  { "text_pos", PROP_TYPE_POINT, 0, NULL, NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_DESC_END
};


static DiaObjectChange *
compfeat_add_segment_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  compfeat_update_data ((Compfeat *) obj);

  return change;
}


static DiaObjectChange *
compfeat_delete_segment_callback (DiaObject *obj,
                                  Point     *clicked,
                                  gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  compfeat_update_data ((Compfeat *) obj);

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
  { "text_font", PROP_TYPE_FONT, offsetof(Compfeat,text),offsetof(Text,font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Compfeat,text),offsetof(Text,height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(Compfeat,text),offsetof(Text,color) },
  { "text_alignment", PROP_TYPE_ENUM, offsetof(Compfeat,text),offsetof(Text,alignment) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Compfeat, text_pos) },
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Compfeat, line_width) },
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Compfeat, line_color) },
  { NULL, 0, 0 }
};

static void
compfeat_get_props(Compfeat *compfeat, GPtrArray *props)
{
  if (compfeat->roletmp)
    compfeat->role = compfeat->roletmp;
  text_set_position(compfeat->text, &compfeat->text_pos);
  object_get_props_from_offsets(&compfeat->orth.object,
                                compfeat_offsets, props);
}

static void
compfeat_set_props(Compfeat *compfeat, GPtrArray *props)
{
  object_set_props_from_offsets(&compfeat->orth.object,
                                compfeat_offsets, props);
  compfeat->text_handle.pos = compfeat->text_pos;
  text_set_position(compfeat->text, &compfeat->text_handle.pos);
  compfeat_update_data(compfeat);
}

static real
compfeat_distance_from(Compfeat *compfeat, Point *point)
{
  OrthConn *orth = &compfeat->orth;
  return orthconn_distance_from(orth, point, compfeat->line_width);
}

static void
compfeat_select(Compfeat *compfeat, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  text_set_cursor(compfeat->text, clicked_point, interactive_renderer);
  text_grab_focus(compfeat->text, &compfeat->orth.object);
  orthconn_update_data(&compfeat->orth);
}


static DiaObjectChange *
compfeat_move_handle (Compfeat         *compfeat,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (compfeat != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    text_set_position(compfeat->text, to);
    change = NULL;
  } else  {
    change = orthconn_move_handle(&compfeat->orth, handle, to, cp,
				  reason, modifiers);
  }
  compfeat_update_data(compfeat);

  return change;
}


static DiaObjectChange *
compfeat_move (Compfeat *compfeat, Point *to)
{
  DiaObjectChange *change;
  Point delta = *to;

  delta = *to;
  point_sub(&delta, &compfeat->orth.points[0]);

  /* I don't understand this, but the text position is wrong directly
     after compfeat_create()! */
  point_add(&delta, &compfeat->text->position);
  text_set_position(compfeat->text, &delta);
  change = orthconn_move(&compfeat->orth, to);
  compfeat_update_data(compfeat);

  return change;
}


static void
compfeat_draw (Compfeat *compfeat, DiaRenderer *renderer)
{
  Point *points;
  OrthConn *orth = &compfeat->orth;
  int n;
  char directions;
  Arrow startarrow, endarrow;

  g_return_if_fail (compfeat != NULL);
  g_return_if_fail (renderer != NULL);

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, compfeat->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  if (compfeat->orth.orientation[orth->numorient - 1] == HORIZONTAL) {
    directions = (points[n - 1].x > points[n - 2].x)? DIR_EAST: DIR_WEST;
  } else {
    directions = (points[n - 1].y > points[n - 2].y)? DIR_SOUTH: DIR_NORTH;
  }

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE)
    compfeat->cp.directions = directions;

  startarrow.type = ARROW_NONE;
  startarrow.length = COMPPROP_DIAMETER;
  startarrow.width = COMPPROP_DIAMETER;
  endarrow.length = COMPPROP_DIAMETER;
  endarrow.width = COMPPROP_DIAMETER;
  endarrow.type = compprop_arrow[compfeat->role];
  dia_renderer_draw_polyline_with_arrows (renderer, points, n,
                                          compfeat->line_width,
                                          &compfeat->line_color,
                                          &startarrow, &endarrow);

  text_draw (compfeat->text, renderer);
}

static DiaObject *
compfeat_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Compfeat *compfeat;
  OrthConn *orth;
  DiaObject *obj;
  Point p;
  DiaFont *font;

  font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);

  compfeat = g_new0(Compfeat, 1);
  compfeat->role = compfeat->roletmp = GPOINTER_TO_INT(user_data);
  compfeat->line_width = 0.1;

  orth = &compfeat->orth;
  obj = &orth->object;

  obj->type = &compfeat_type;
  obj->ops = &compfeat_ops;

  orthconn_init(orth, startpoint);

  p = *startpoint;
  p.y -= COMPPROP_TEXTOFFSET;

  compfeat->line_color = attributes_get_foreground();
  compfeat->text = new_text ("", font,
                             COMPPROP_FONTHEIGHT,
                             &p,
                             &compfeat->line_color,
                             DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  compfeat->text_handle.id = HANDLE_MOVE_TEXT;
  compfeat->text_handle.type = HANDLE_MINOR_CONTROL;
  compfeat->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  compfeat->text_handle.connected_to = NULL;
  compfeat->text_handle.pos = compfeat->text_pos = p;
  object_add_handle(obj, &compfeat->text_handle);

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE) {
    int pos = obj->num_connections;
    object_add_connectionpoint(&orth->object, &compfeat->cp);
    obj->connections[pos] = &compfeat->cp;
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
  OrthConn *orth = &compfeat->orth;

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE) {
    object_remove_connectionpoint(&orth->object, &compfeat->cp);
  }
  text_destroy(compfeat->text);
  orthconn_destroy(&compfeat->orth);
}

static void
compfeat_update_data(Compfeat *compfeat)
{
  OrthConn *orth = &compfeat->orth;
  PolyBBExtras *extra = &orth->extra_spacing;
  int n;
  DiaObject *obj = &orth->object;
  DiaRectangle rect;
  Point *points;

  points = &orth->points[0];
  n = orth->numpoints;

  obj->position = points[0];

  if (compfeat->role == COMPPROP_FACET
      || compfeat->role == COMPPROP_EVENTSOURCE)
    compfeat->cp.pos = points[n - 1];

  compfeat->text_pos = compfeat->text_handle.pos = compfeat->text->position;

  orthconn_update_data(orth);

  /* Boundingbox: */
  extra->start_long =
    extra->start_trans =
    extra->end_long = compfeat->line_width + COMPPROP_DIAMETER;
  extra->end_trans = compfeat->line_width + COMPPROP_DIAMETER;

  orthconn_update_boundingbox(orth);
  text_calc_boundingbox(compfeat->text, &rect);
  rectangle_union(&obj->bounding_box, &rect);
}

static DiaObject *
compfeat_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&compfeat_type,
                                                obj_node,version,ctx);
  if (version == 0) {
    AttributeNode attr;
    /* In old objects with no autorouting, set it to false. */
    attr = object_find_attribute(obj_node, "autorouting");
    if (attr == NULL)
      ((OrthConn*)obj)->autorouting = FALSE;
  }
  return obj;
}
