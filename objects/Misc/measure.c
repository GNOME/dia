/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * measure.c -- shape to measure distance or angle
 * Copyright (C) 2008  Hans Breuer, <Hans@Breuer.Org>
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

#include "object.h"
#include "diarenderer.h"
#include "attributes.h"
#include "properties.h"
#include "boundingbox.h"
#include "connection.h"
#include "units.h"

#include "pixmaps/measure.xpm"

/* Object definition */
typedef struct _Measure {
  Connection connection;

  DiaFont *font;
  real font_height;

  Color   line_color;
  real    line_width;
  real    scale;
  DiaUnit unit;
  int     precision;

  /* caclculated data */
  char *name; /* the calculated measurment */
  Point text_pos;
} Measure;

/* Type definition */
static DiaObject *measure_create (Point *startpoint,
				  void *user_data,
				  Handle **handle1,
				  Handle **handle2);
static DiaObject *
measure_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps measure_type_ops =
{
  (CreateFunc)measure_create,
  (LoadFunc)  measure_load,
  (SaveFunc)  object_save_using_properties, /* measure_save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType measure_type =
{
  "Misc - Measure", /* name */
  0,             /* version */
  measure_xpm,    /* pixmap */
  &measure_type_ops, /* ops */
  NULL,      /* pixmap_file */
  0    /* default_user_data */
};

/* make accessible from the outside for type registration */
DiaObjectType *_measure_type = (DiaObjectType *) &measure_type;

/* Class definition */
static DiaObjectChange* measure_move_handle (Measure *measure,
                                          Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* measure_move (Measure *measure, Point *to);
static void measure_select(Measure *measure, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static void measure_draw(Measure *measure, DiaRenderer *renderer);
static real measure_distance_from (Measure *measure, Point *point);
static void measure_update_data (Measure *measure);
static void measure_destroy (Measure *measure);
static DiaMenu *measure_get_object_menu(Measure *measure,
					Point *clickedpoint);
static const PropDescription *measure_describe_props(Measure *measure);
static void measure_get_props(Measure *measure, GPtrArray *props);
static void measure_set_props(Measure *measure, GPtrArray *props);

static ObjectOps measure_ops = {
  (DestroyFunc)         measure_destroy,
  (DrawFunc)            measure_draw,
  (DistanceFunc)        measure_distance_from,
  (SelectFunc)          measure_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            measure_move,
  (MoveHandleFunc)      measure_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      measure_get_object_menu,
  (DescribePropsFunc)   measure_describe_props,
  (GetPropsFunc)        measure_get_props,
  (SetPropsFunc)        measure_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

/* Type implementation */
/*! Factory function - create default object */
static DiaObject *
measure_create (Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Measure *measure;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  measure = g_new0 (Measure,1);
  obj = &measure->connection.object;
  obj->type = &measure_type;
  obj->ops = &measure_ops;

  conn = &measure->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  connection_init(conn, 2, 0);

  /* kind of measurement can only be set via user data */

  attributes_get_default_font (&measure->font, &measure->font_height);

  measure->line_width = attributes_get_default_linewidth();
  measure->line_color = attributes_get_foreground();
  measure->scale = 1.0;
  measure->unit = DIA_UNIT_CENTIMETER;
  measure->precision = 3;

  measure_update_data(measure);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return obj;
}
static DiaObject *
measure_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&measure_type,
                                      obj_node,version,ctx);
}
static PropNumData scale_range = {1e-9, 1e9, 1 };
static PropEnumData unit_data[] = {
  { NC_("length unit", "cm"), DIA_UNIT_CENTIMETER },
  { NC_("length unit", "dm"), DIA_UNIT_DECIMETER },
  { NC_("length unit", "ft"), DIA_UNIT_FEET },
  { NC_("length unit", "in"), DIA_UNIT_INCH },
  { NC_("length unit", "m"),  DIA_UNIT_METER },
  { NC_("length unit", "mm"), DIA_UNIT_MILLIMETER },
  { NC_("length unit", "pt"), DIA_UNIT_POINT },
  { NC_("length unit", "pi"), DIA_UNIT_PICA },
  { NULL, }
};
static PropNumData precision_data = {1, 9, 1};

/* Class/Object implementation */
static PropDescription measure_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING,/*PROP_FLAG_VISIBLE|*/PROP_FLAG_DONT_MERGE,
    N_("Measurement"),NULL }, /* FIXME: mark read-only */
  { "scale", PROP_TYPE_REAL, PROP_FLAG_VISIBLE, N_("Scale"), NULL, &scale_range},
  { "unit", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE, N_("Unit"), NULL, &unit_data },
  { "precision", PROP_TYPE_INT, PROP_FLAG_VISIBLE, N_("Precision"), NULL, &precision_data },
  /* the default PROP_STD_TEXT_FONT has PROP_FLAG_DONT_SAVE, we need saving */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE),
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_DESC_END
};
static PropOffset measure_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Measure, name) },
  { "scale", PROP_TYPE_REAL, offsetof(Measure, scale) },
  { "unit", PROP_TYPE_ENUM, offsetof(Measure, unit) },
  { "precision", PROP_TYPE_INT, offsetof(Measure, precision) },
  { "text_font",PROP_TYPE_FONT,offsetof(Measure, font) },
  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Measure, font_height) },
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Measure, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Measure, line_color) },
  { NULL, 0, 0 }
};
#define MEASURE_ARROW(measure) { ARROW_FILLED_TRIANGLE, measure->font_height, measure->font_height/2 }
/*! Not in the object interface but very important anyway. Used to recalculate the object data after a change  */
static void
measure_update_data (Measure *measure)
{
  Connection *conn = &measure->connection;
  DiaObject *obj = &measure->connection.object;
  real value;
  Point *ends = measure->connection.endpoints;
  LineBBExtras *extra = &conn->extra_spacing;
  DiaRectangle bbox;
  Arrow arrow = MEASURE_ARROW(measure);
  real ascent, width, theta;

  g_return_if_fail (obj->handles != NULL);
  connection_update_handles(conn);

  extra->start_trans =
  extra->end_trans   =
  extra->start_long  =
  extra->end_long    = (measure->line_width / 2.0);

  g_clear_pointer (&measure->name, g_free);
  value = distance_point_point (&ends[0], &ends[1]);
  value *= measure->scale;
  value *= (28.346457 / dia_unit_get_factor (measure->unit));
  measure->name = g_strdup_printf ("%.*g %s", measure->precision, value, dia_unit_get_symbol (measure->unit));

  ascent = dia_font_ascent (measure->name, measure->font, measure->font_height);
  width = dia_font_string_width (measure->name, measure->font, measure->font_height);
  theta = atan2(ends[1].y-ends[0].y, ends[1].x-ends[0].x);
  theta = (0 >= theta ? theta + M_PI : theta);

  if (theta >= M_PI * 3/4) {
    measure->text_pos.x = (ends[0].x + ends[1].x) / 2 - sin(theta) * measure->font_height/2 - width * (2.5 - 2/M_PI * ( theta - 3/4 * M_PI ));
    measure->text_pos.y = (ends[0].y + ends[1].y) / 2 + cos(theta) * measure->font_height/2;
  } else  {
    measure->text_pos.x = (ends[0].x + ends[1].x) / 2 + sin(theta) * measure->font_height/2;
    measure->text_pos.y = (ends[0].y + ends[1].y) / 2 - cos(theta) * measure->font_height/2;
  }

  line_bbox (&ends[0], &ends[0], &conn->extra_spacing,&conn->object.bounding_box);
  arrow_bbox (&arrow, measure->line_width, &ends[0], &ends[1], &bbox);
  rectangle_union(&obj->bounding_box, &bbox);
  arrow_bbox (&arrow, measure->line_width, &ends[1], &ends[0], &bbox);
  rectangle_union(&obj->bounding_box, &bbox);

  bbox.left = measure->text_pos.x;
  bbox.top = measure->text_pos.y - ascent;
  bbox.bottom = bbox.top + measure->font_height;
  bbox.right = bbox.left + width;
  rectangle_union(&obj->bounding_box, &bbox);

  obj->position = conn->endpoints[0];
}

static void
measure_draw (Measure *measure, DiaRenderer *renderer)
{
  Arrow arrow = MEASURE_ARROW(measure);

  dia_renderer_set_linewidth (renderer, measure->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_ROUND);

  dia_renderer_draw_line_with_arrows (renderer,
                                      &measure->connection.endpoints[0],
                                      &measure->connection.endpoints[1],
                                      measure->line_width,
                                      &measure->line_color,
                                      &arrow,
                                      &arrow);

  dia_renderer_set_font (renderer, measure->font, measure->font_height);
  dia_renderer_draw_string (renderer,
                            measure->name,
                            &measure->text_pos,
                            DIA_ALIGN_LEFT,
                            &measure->line_color);
}

/*! A standard props compliant object needs to describe its parameters */
static const PropDescription *
measure_describe_props (Measure *measure)
{
  if (measure_props[0].quark == 0)
    prop_desc_list_calculate_quarks(measure_props);
  return measure_props;
}
static void
measure_get_props (Measure *measure, GPtrArray *props)
{
  object_get_props_from_offsets(&measure->connection.object, measure_offsets, props);
}
static void
measure_set_props (Measure *measure, GPtrArray *props)
{
  object_set_props_from_offsets(&measure->connection.object, measure_offsets, props);
  measure_update_data (measure);
}
static real
measure_distance_from (Measure *measure, Point *point)
{
  return distance_line_point (
		&measure->connection.endpoints[0],
                &measure->connection.endpoints[1],
		measure->line_width, point);
}
static void
measure_select(Measure *measure, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  connection_update_handles(&measure->connection);
}
static DiaObjectChange*
measure_move_handle (Measure *measure,
                     Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason, ModifierKeys modifiers)
{
  connection_move_handle(&measure->connection, handle->id, to, cp, reason, modifiers);
  measure_update_data(measure);
  return NULL;
}
static DiaMenu *
measure_get_object_menu(Measure *measure,
                        Point *clickedpoint)
{
  return NULL;
}

static DiaObjectChange*
measure_move (Measure *measure, Point *to)
{
  Point start_to_end;
  Point *ends = &measure->connection.endpoints[0];

  start_to_end = ends[1];
  point_sub(&start_to_end, &ends[0]);

  ends[1] = ends[0] = *to;
  point_add(&ends[1], &start_to_end);

  measure_update_data (measure);
  return NULL;
}
static void
measure_destroy(Measure *measure)
{
  connection_destroy(&measure->connection);
}
