/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

/* Copyright 2003, W. Borgert <debacle@debian.org>
   copied a lot from UML/large_package.c and standard/polygon.c */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "polyshape.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "network.h"

#include "pixmaps/radiocell.xpm"

/* TODO? no visual effect ATM, but useful anyway */
typedef enum {
  MACRO_CELL,
  MICRO_CELL,
  PICO_CELL,
} CellType;

/* TODO: add different cell technologies, like GSM, UMTS, ... */

typedef struct _RadioCell RadioCell;

struct _RadioCell {
  PolyShape poly;		/* always 1st! */
  CellType celltype;
  real radius;			/* pseudo-radius */
  ConnectionPoint cp;		/* connection point in the center */
  Color line_colour;
  LineStyle line_style;
  real dashlength;
  real line_width;
  gboolean show_background;
  Color fill_colour;
  Text *text;
  TextAttributes attrs;
  int subscribers;		/* number of subscribers in this cell,
				   always >= 0, but check is missing */
};

#define RADIOCELL_LINEWIDTH  0.1
#define RADIOCELL_FONTHEIGHT 0.8

static real radiocell_distance_from(RadioCell *radiocell, Point *point);
static void radiocell_select(RadioCell *radiocell, Point *clicked_point,
			     DiaRenderer *interactive_renderer);
static ObjectChange* radiocell_move_handle(RadioCell *radiocell,
					   Handle *handle,
					   Point *to, ConnectionPoint *cp,
					   HandleMoveReason reason,
					   ModifierKeys modifiers);
static ObjectChange* radiocell_move(RadioCell *radiocell, Point *to);
static void radiocell_draw(RadioCell *radiocell, DiaRenderer *renderer);
static Object *radiocell_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static void radiocell_destroy(RadioCell *radiocell);
static void radiocell_update_data(RadioCell *radiocell);
static PropDescription *radiocell_describe_props(RadioCell *radiocell);
static void radiocell_get_props(RadioCell *radiocell, GPtrArray *props);
static void radiocell_set_props(RadioCell *radiocell, GPtrArray *props);
static Object *radiocell_load(ObjectNode obj_node, int version,
			      const char *filename);

static ObjectTypeOps radiocell_type_ops =
{
  (CreateFunc)        radiocell_create,
  (LoadFunc)          radiocell_load,
  (SaveFunc)          object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType radiocell_type =
{
  "Network - Radio Cell",	/* name */
  0,				/* version */
  (char **) radiocell_xpm,	/* pixmap */

  &radiocell_type_ops		/* ops */
};

static ObjectOps radiocell_ops = {
  (DestroyFunc)         radiocell_destroy,
  (DrawFunc)            radiocell_draw,
  (DistanceFunc)        radiocell_distance_from,
  (SelectFunc)          radiocell_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            radiocell_move,
  (MoveHandleFunc)      radiocell_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   radiocell_describe_props,
  (GetPropsFunc)        radiocell_get_props,
  (SetPropsFunc)        radiocell_set_props
};

static PropEnumData prop_cell_type_data[] = {
  { N_("Macro Cell"), MACRO_CELL },
  { N_("Micro Cell"), MICRO_CELL },
  { N_("Pico Cell"),  PICO_CELL },
  { NULL, 0}
};

static PropDescription radiocell_props[] = {
  POLYSHAPE_COMMON_PROPERTIES,
  { "radius", PROP_TYPE_REAL, 0, N_("Radius"), NULL, NULL },
  { "celltype", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Cell Type:"), NULL, prop_cell_type_data },
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT_ALIGNMENT,
  { "subscribers", PROP_TYPE_INT, PROP_FLAG_VISIBLE,
    N_("Subscribers"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
radiocell_describe_props(RadioCell *radiocell)
{
  if (radiocell_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(radiocell_props);
  }
  return radiocell_props;
}

static PropOffset radiocell_offsets[] = {
  POLYSHAPE_COMMON_PROPERTIES_OFFSETS,
  { "radius", PROP_TYPE_REAL, offsetof(RadioCell, radius) },
  { "celltype", PROP_TYPE_ENUM, offsetof(RadioCell, celltype) },
  { "line_width", PROP_TYPE_REAL, offsetof(RadioCell, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(RadioCell, line_colour) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(RadioCell, line_style), offsetof(RadioCell, dashlength) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(RadioCell, fill_colour) },
  { "show_background", PROP_TYPE_BOOL,
    offsetof(RadioCell, show_background) },
  { "text", PROP_TYPE_TEXT, offsetof(RadioCell, text) },
  { "text_font", PROP_TYPE_FONT, offsetof(RadioCell, attrs.font) },
  { "text_height", PROP_TYPE_REAL, offsetof(RadioCell, attrs.height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(RadioCell, attrs.color) },
  { "text_alignment", PROP_TYPE_ENUM,
    offsetof(RadioCell, attrs.alignment) },
  { "subscribers", PROP_TYPE_INT, offsetof(RadioCell, subscribers) },
  { NULL, 0, 0 },
};

static void
radiocell_get_props(RadioCell *radiocell, GPtrArray *props)
{
  text_get_attributes(radiocell->text, &radiocell->attrs);
  object_get_props_from_offsets(&radiocell->poly.object,
                                radiocell_offsets, props);
}

static void
radiocell_set_props(RadioCell *radiocell, GPtrArray *props)
{
  object_set_props_from_offsets(&radiocell->poly.object,
                                radiocell_offsets, props);
  apply_textattr_properties(props, radiocell->text,
			    "text", &radiocell->attrs);
  radiocell_update_data(radiocell);
}

static real
radiocell_distance_from(RadioCell *radiocell, Point *point)
{
  return polyshape_distance_from(&radiocell->poly, point,
				 radiocell->line_width);
}

static void
radiocell_select(RadioCell *radiocell, Point *clicked_point,
		 DiaRenderer *interactive_renderer)
{
  text_set_cursor(radiocell->text, clicked_point, interactive_renderer);
  text_grab_focus(radiocell->text, &radiocell->poly.object);
  polyshape_update_data(&radiocell->poly);
}

static ObjectChange*
radiocell_move_handle(RadioCell *radiocell, Handle *handle,
		      Point *to, ConnectionPoint *cp,
		      HandleMoveReason reason, ModifierKeys modifiers)
{
  real distance = distance_point_point(&handle->pos, to);
  gboolean larger = distance_point_point(&handle->pos, &radiocell->cp.pos) <
    distance_point_point(to, &radiocell->cp.pos);

  /* TODO: this flickers terribly */
  radiocell->radius += distance * (larger? 1: -1);
  if (radiocell->radius < 1.)
    radiocell->radius = 1.;
  radiocell_update_data(radiocell);

  return NULL;
}

static ObjectChange*
radiocell_move(RadioCell *radiocell, Point *to)
{
  polyshape_move(&radiocell->poly, to);
  radiocell->cp.pos = *to;
  radiocell->cp.pos.x -= radiocell->radius;
  radiocell_update_data(radiocell);

  return NULL;
}

static void
radiocell_draw(RadioCell *radiocell, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  PolyShape *poly;
  Point *points;
  int n;

  assert(radiocell != NULL);
  assert(renderer != NULL);

  poly = &radiocell->poly;
  points = &poly->points[0];
  n = poly->numpoints;

  if (radiocell->show_background) {
    renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
    renderer_ops->fill_polygon(renderer, points, n, &radiocell->fill_colour);
  }
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linestyle(renderer, radiocell->line_style);
  renderer_ops->set_linewidth(renderer, radiocell->line_width);
  renderer_ops->set_dashlength(renderer, radiocell->dashlength);
  renderer_ops->draw_polygon(renderer, points, n, &radiocell->line_colour);

  text_draw(radiocell->text, renderer);
}

static void
radiocell_update_data(RadioCell *radiocell)
{
  PolyShape *poly = &radiocell->poly;
  Object *obj = &poly->object;
  ElementBBExtras *extra = &poly->extra_spacing;
  Rectangle text_box;
  Point textpos;
  int i;
  /* not exactly a regular hexagon, but this fits better in the grid */
  Point points[] = { {  1., 0. }, {  .5,  .75 }, { -.5,  .75 },
		     { -1., 0. }, { -.5, -.75 }, {  .5, -.75 } };

  /* TODO: the CP is invisible and does not yet work */
  radiocell->cp.pos.x = (poly->points[0].x + poly->points[3].x) / 2.;
  radiocell->cp.pos.y = poly->points[0].y;

  for (i = 0; i < 6; i++) {
    poly->points[i] = radiocell->cp.pos;
    poly->points[i].x += radiocell->radius * points[i].x;
    poly->points[i].y += radiocell->radius * points[i].y;
  }

  /* Add bounding box for text: */
  text_calc_boundingbox(radiocell->text, NULL);
  textpos.x = (poly->points[0].x + poly->points[3].x) / 2.;
  textpos.y = poly->points[0].y -
    (radiocell->text->height * (radiocell->text->numlines - 1) +
     radiocell->text->descent) / 2.;
  text_set_position(radiocell->text, &textpos);
  text_calc_boundingbox(radiocell->text, &text_box);
  polyshape_update_data(poly);
  extra->border_trans = radiocell->line_width / 2.0;
  polyshape_update_boundingbox(poly);
  rectangle_union(&obj->bounding_box, &text_box);
  obj->position = poly->points[0];
}

static Object *
radiocell_create(Point *startpoint,
		 void *user_data,
		 Handle **handle1,
		 Handle **handle2)
{
  RadioCell *radiocell;
  PolyShape *poly;
  Object *obj;
  DiaFont *font;

  radiocell = g_new0(RadioCell, 1);
  poly = &radiocell->poly;
  obj = &poly->object;
  obj->type = &radiocell_type;
  obj->ops = &radiocell_ops;
  obj->can_parent = TRUE;

  radiocell->celltype = MACRO_CELL;
  radiocell->radius = 4.;
  radiocell->subscribers = 1000;

  /* do not use default_properties.show_background here */
  radiocell->show_background = FALSE;
  radiocell->fill_colour = color_white;
  radiocell->line_colour = color_black;
  radiocell->line_width = RADIOCELL_LINEWIDTH;
  attributes_get_default_line_style(&radiocell->line_style,
				    &radiocell->dashlength);

  font = dia_font_new_from_style(DIA_FONT_MONOSPACE, RADIOCELL_FONTHEIGHT);
  radiocell->text = new_text("", font, RADIOCELL_FONTHEIGHT, startpoint,
			     &color_black, ALIGN_CENTER);
  dia_font_unref(font);
  text_get_attributes(radiocell->text, &radiocell->attrs);

  polyshape_init(poly, 6);

  object_add_connectionpoint(&poly->object, &radiocell->cp);
  obj->connections[0] = &radiocell->cp;
  radiocell->cp.object = obj;
  radiocell->cp.connected = NULL;
  radiocell->cp.directions = DIR_ALL;
  radiocell->cp.pos = *startpoint;
  radiocell->cp.pos.x -= radiocell->radius;

  radiocell_update_data(radiocell);
  *handle1 = poly->object.handles[0];
  *handle2 = poly->object.handles[2];
  return &radiocell->poly.object;
}

static void
radiocell_destroy(RadioCell *radiocell)
{
  text_destroy(radiocell->text);
  polyshape_destroy(&radiocell->poly);
}

static Object *
radiocell_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&radiocell_type,
                                      obj_node, version, filename);
}
