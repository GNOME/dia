/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * Copyright (C) 2002 David Hoover
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

/*! \file line.c -- Implements the "Standard - Line" object */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "arrows.h"
#include "connpoint_line.h"
#include "properties.h"
#include "create.h"

#define DEFAULT_WIDTH 0.25

typedef struct _LineProperties LineProperties;

/*!
 * \brief Standard - Line: a straight _Connection
 *
 * The Standard - Line object implements a straight connection between two points.
 * The object can grow addtional _ConnectionPoint though the internal use
 * of ConnPointLine.
 *
 * \extends _Connection
 * \ingroup StandardObjects
 */
typedef struct _Line {
  Connection connection;

  ConnPointLine *cpl;

  Color line_color;
  double line_width;
  DiaLineStyle line_style;
  DiaLineCaps line_caps;
  Arrow start_arrow, end_arrow;
  double dashlength;
  double absolute_start_gap, absolute_end_gap;
} Line;

struct _LineProperties {
  double absolute_start_gap, absolute_end_gap;
};

static LineProperties default_properties;

static DiaObjectChange* line_move_handle(Line *line, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason,
			     ModifierKeys modifiers);
static DiaObjectChange* line_move(Line *line, Point *to);
static void line_select(Line *line, Point *clicked_point,
			DiaRenderer *interactive_renderer);
static void line_draw(Line *line, DiaRenderer *renderer);
static DiaObject *line_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static real line_distance_from(Line *line, Point *point);
static void line_update_data(Line *line);
static void line_destroy(Line *line);
static DiaObject *line_copy(Line *line);

static void line_set_props(Line *line, GPtrArray *props);

static void line_save(Line *line, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *line_load(ObjectNode obj_node, int version, DiaContext *ctx);
static DiaMenu *line_get_object_menu(Line *line, Point *clickedpoint);
static gboolean line_transform(Line *line, const DiaMatrix *m);

void Line_adjust_for_absolute_gap(Line *line, Point *gap_endpoints);

static ObjectTypeOps line_type_ops =
{
  (CreateFunc) line_create,
  (LoadFunc)   line_load,
  (SaveFunc)   line_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

static PropNumData gap_range = { -G_MAXFLOAT, G_MAXFLOAT, 0.1};

static PropDescription line_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_STD_LINE_CAPS_OPTIONAL,
  PROP_FRAME_BEGIN("arrows",PROP_FLAG_STANDARD,N_("Arrows")),
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_FRAME_END("arrows",PROP_FLAG_STANDARD),
  { "start_point", PROP_TYPE_POINT, 0,
    N_("Start point"), NULL },
  { "end_point", PROP_TYPE_POINT, 0,
    N_("End point"), NULL },

  PROP_FRAME_BEGIN("gaps",0,N_("Line gaps")),
  { "absolute_start_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Absolute start gap"), NULL, &gap_range },
  { "absolute_end_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE | PROP_FLAG_OPTIONAL,
    N_("Absolute end gap"), NULL, &gap_range },
  PROP_FRAME_END("gaps",0),

  PROP_DESC_END
};

static PropOffset line_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Line, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Line, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Line, line_style), offsetof(Line, dashlength) },
  { "line_caps", PROP_TYPE_ENUM, offsetof(Line, line_caps) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Line, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Line, end_arrow) },
  { "start_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[0]) },
  { "end_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[1]) },
  PROP_OFFSET_FRAME_BEGIN("gaps"),
  { "absolute_start_gap", PROP_TYPE_REAL, offsetof(Line, absolute_start_gap) },
  { "absolute_end_gap", PROP_TYPE_REAL, offsetof(Line, absolute_end_gap) },
  PROP_OFFSET_FRAME_END("gaps"),
  { NULL, 0, 0 }
};

DiaObjectType line_type =
{
  "Standard - Line",   /* name */
  0,                   /* version */
  (const char **) "res:/org/gnome/Dia/objects/standard/line.png",
  &line_type_ops,      /* ops */
  NULL,
  0,
  line_props,
  line_offsets
};

DiaObjectType *_line_type = (DiaObjectType *) &line_type;

static ObjectOps line_ops = {
  (DestroyFunc)         line_destroy,
  (DrawFunc)            line_draw,
  (DistanceFunc)        line_distance_from,
  (SelectFunc)          line_select,
  (CopyFunc)            line_copy,
  (MoveFunc)            line_move,
  (MoveHandleFunc)      line_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      line_get_object_menu,
  (DescribePropsFunc)   object_describe_props,
  (GetPropsFunc)        object_get_props,
  (SetPropsFunc)        line_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
  (TransformFunc)       line_transform,
};


static void
line_set_props (Line *line, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (line),
                                 line_offsets, props);
  line_update_data (line);
}


static void
line_init_defaults (void)
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.absolute_start_gap = 0.0;
    default_properties.absolute_end_gap = 0.0;
    defaults_initialized = 1;
  }
}


/**
 * line_add_connpoint_callback:
 *
 * Add a connection point to the line
 */
static DiaObjectChange *
line_add_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *oc;

  oc = connpointline_add_point (((Line *) obj)->cpl, clicked);
  line_update_data ((Line *) obj);

  return oc;
}


/*!
 * \brief Remove a connection point from the line
 *
 * \memberof Line
 */
static DiaObjectChange *
line_remove_connpoint_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *oc;

  oc = connpointline_remove_point (((Line *)obj)->cpl, clicked);
  line_update_data ((Line *) obj);

  return oc;
}


/**
 * _convert_to_polyline_callback:
 * @obj: self pointer
 * @clicked: last clicked point on canvas or %NULL
 * @data: here unuesed user_data pointer
 *
 *
 * Upgrade the #Line to a #Polyline
 *
 * Convert the #Line to a Polyline with the position clicked as third point.
 * Further object properties are preserved by the use of object_substitute()
 *
 * Returns: an #DiaObjectChange to support undo/redo
 */
static DiaObjectChange *
_convert_to_polyline_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObject *poly;
  Line *line = (Line *)obj;
  Point points[3];

  points[0] = line->connection.endpoints[0];
  points[2] = line->connection.endpoints[1];
  if (clicked) {
    points[1] = *clicked;
  } else {
    points[1].x = (points[0].x + points[2].x) / 2;
    points[1].y = (points[0].y + points[2].y) / 2;
  }

  poly = create_standard_polyline (3, points, &line->end_arrow, &line->start_arrow);
  g_return_val_if_fail (poly != NULL, NULL);
  return object_substitute (obj, poly);
}


/**
 * _convert_to_zigzagline_callback:
 * @obj: self pointer
 * @clicked: last clicked point on canvas or %NULL
 * @data: here unuesed user_data pointer
 *
 * Upgrade the #Line to a #Zigzagline
 *
 * Convert the #Line to a #Zigzagline with the position clicked (if near enough)
 * for the new segment. The result of this function is more favorable for connected
 * lines by autorouting.
 *
 * Further object properties are preserved by the use of object_substitute()
 *
 * Returns: an #DiaObjectChange to support undo/redo
 */
static DiaObjectChange *
_convert_to_zigzagline_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObject *zigzag;
  Line *line = (Line *) obj;
  Point points[4];

  if (clicked) {
    points[0] = line->connection.endpoints[0];
    points[3] = line->connection.endpoints[1];
    /* not sure if we really want to give it a direction at all */
    if (fabs(((points[0].x + points[3].x)/2) - clicked->x) > fabs(((points[0].y + points[3].y)/2) - clicked->y)) {
      points[1].x = points[2].x = clicked->x;
      points[1].y = points[0].y;
      points[2].y = points[3].y;
    } else {
      points[1].y = points[2].y = clicked->y;
      points[1].x = points[0].x;
      points[2].x = points[3].x;
    }
    zigzag = create_standard_zigzagline (4, points, &line->end_arrow, &line->start_arrow);
  } else {
    points[0] = line->connection.endpoints[0];
    points[3] = line->connection.endpoints[1];
    points[1].x = points[2].x = (points[0].x + points[3].x) / 2.0;
    points[1].y = points[0].y;
    points[2].y = points[3].y;
    zigzag = create_standard_zigzagline (4, points, &line->end_arrow, &line->start_arrow);
  }

  g_return_val_if_fail (zigzag != NULL, NULL);
  return object_substitute (obj, zigzag);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), line_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), line_remove_connpoint_callback, NULL, 1 },
  { N_("Upgrade to Polyline"), _convert_to_polyline_callback, NULL, 1 },
  { N_("Upgrade to Zigzagline"), _convert_to_zigzagline_callback, NULL, 1 }
};

static DiaMenu object_menu = {
  N_("Line"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
line_get_object_menu(Line *line, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = line->cpl;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active =
    connpointline_can_add_point(cpl, clickedpoint);
  object_menu_items[1].active =
    connpointline_can_remove_point(cpl,clickedpoint);
  return &object_menu;
}

static gboolean
line_transform(Line *line, const DiaMatrix *m)
{
  int i;

  g_return_val_if_fail (m != NULL, FALSE);

  for (i = 0; i < 2; i++)
    transform_point (&line->connection.endpoints[i], m);

  line_update_data(line);
  return TRUE;
}

/*!
 * \brief Gap calculation for _Line
 *
 * Calculate the absolute gap -- this gap is 'transient', in that
 * the actual end of the line is not moved, but it is made to look like
 * it is shorter.
 *
 * \protected \memberof Line
 */
static void
line_adjust_for_absolute_gap(Line *line, Point *gap_endpoints)
{
  Point endpoints[2];
  real line_length;

  endpoints[0] = line->connection.endpoints[0];
  endpoints[1] = line->connection.endpoints[1];

  line_length = distance_point_point(&endpoints[0], &endpoints[1]);

  /* puts new 0 to x% of  0->1  */
  point_convex(&gap_endpoints[0], &endpoints[0], &endpoints[1],
              1 - line->absolute_start_gap/line_length);

  /* puts new 1 to x% of  1->0  */
  point_convex(&gap_endpoints[1], &endpoints[1], &endpoints[0],
              1 - line->absolute_end_gap/line_length);
}

static real
line_distance_from(Line *line, Point *point)
{
  Point *endpoints;

  endpoints = &line->connection.endpoints[0];

  if (line->absolute_start_gap || line->absolute_end_gap ) {
    Point gap_endpoints[2];  /* Visible endpoints of line */

    line_adjust_for_absolute_gap(line, gap_endpoints);
    return distance_line_point( &gap_endpoints[0], &gap_endpoints[1],
                               line->line_width, point);
  } else {
    return distance_line_point( &endpoints[0], &endpoints[1],
                               line->line_width, point);
  }
}

static void
line_select(Line *line, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  connection_update_handles(&line->connection);
}


static DiaObjectChange *
line_move_handle (Line             *line,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  g_return_val_if_fail (line != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  connection_move_handle (&line->connection,
                          handle->id,
                          to,
                          cp,
                          reason,
                          modifiers);
  connection_adjust_for_autogap (&line->connection);

  line_update_data (line);

  return NULL;
}


static DiaObjectChange*
line_move(Line *line, Point *to)
{
  Point start_to_end;
  Point *endpoints = &line->connection.endpoints[0];

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  line_update_data(line);

  return NULL;
}


static void
line_draw (Line *line, DiaRenderer *renderer)
{
  Point gap_endpoints[2];

  g_return_if_fail (line != NULL);
  g_return_if_fail (renderer != NULL);

  dia_renderer_set_linewidth (renderer, line->line_width);
  dia_renderer_set_linestyle (renderer, line->line_style, line->dashlength);
  dia_renderer_set_linecaps (renderer, line->line_caps);

  if (line->absolute_start_gap || line->absolute_end_gap ) {
    line_adjust_for_absolute_gap (line, gap_endpoints);

    dia_renderer_draw_line_with_arrows (renderer,
                                        &gap_endpoints[0],
                                        &gap_endpoints[1],
                                        line->line_width,
                                        &line->line_color,
                                        &line->start_arrow,
                                        &line->end_arrow);
  } else {
    dia_renderer_draw_line_with_arrows (renderer,
                                        &line->connection.endpoints[0],
                                        &line->connection.endpoints[1],
                                        line->line_width,
                                        &line->line_color,
                                        &line->start_arrow,
                                        &line->end_arrow);
  }
}

static DiaObject *
line_create(Point *startpoint,
	    void *user_data,
	    Handle **handle1,
	    Handle **handle2)
{
  Line *line;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  line_init_defaults();

  line = g_new0 (Line, 1);

  line->line_width = attributes_get_default_linewidth();
  line->line_color = attributes_get_foreground();
  line->absolute_start_gap = default_properties.absolute_start_gap;
  line->absolute_end_gap = default_properties.absolute_end_gap;

  conn = &line->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;

  obj->type = &line_type;
  obj->ops = &line_ops;

  connection_init(conn, 2, 0);

  line->cpl = connpointline_create(obj,1);

  attributes_get_default_line_style(&line->line_style, &line->dashlength);
  line->line_caps = DIA_LINE_CAPS_BUTT;
  line->start_arrow = attributes_get_default_start_arrow();
  line->end_arrow = attributes_get_default_end_arrow();
  line_update_data(line);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &line->connection.object;
}

static void
line_destroy(Line *line)
{
  connpointline_destroy(line->cpl);
  connection_destroy(&line->connection);
}

static DiaObject *
line_copy(Line *line)
{
  Line *newline;
  Connection *conn, *newconn;
  DiaObject *newobj;
  int rcc = 0;

  conn = &line->connection;

  newline = g_new0 (Line, 1);
  newconn = &newline->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newline->cpl = connpointline_copy(newobj,line->cpl,&rcc);

  newline->line_color = line->line_color;
  newline->line_width = line->line_width;
  newline->line_style = line->line_style;
  newline->line_caps = line->line_caps;
  newline->dashlength = line->dashlength;
  newline->start_arrow = line->start_arrow;
  newline->end_arrow = line->end_arrow;
  newline->absolute_start_gap = line->absolute_start_gap;
  newline->absolute_end_gap = line->absolute_end_gap;

  line_update_data(line);

  return &newline->connection.object;
}

static void
line_update_data(Line *line)
{
  Connection *conn = &line->connection;
  DiaObject *obj = &conn->object;
  LineBBExtras *extra = &conn->extra_spacing;
  Point start, end;

  extra->start_trans =
  extra->end_trans   =
  extra->start_long  =
  extra->end_long    = (line->line_width / 2.0);

  if (connpoint_is_autogap(line->connection.endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(line->connection.endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  if (line->absolute_start_gap || line->absolute_end_gap ) {
    Point gap_endpoints[2];

    line_adjust_for_absolute_gap(line, gap_endpoints);
    line_bbox(&gap_endpoints[0],&gap_endpoints[1],
	      &conn->extra_spacing,&conn->object.bounding_box);
    start = gap_endpoints[0];
    end = gap_endpoints[1];
  } else {
    connection_update_boundingbox(conn);
    start = conn->endpoints[0];
    end = conn->endpoints[1];
  }
  if (line->start_arrow.type != ARROW_NONE) {
    DiaRectangle bbox;
    Point move_arrow, move_line;
    Point to = start;
    Point from = end;
    calculate_arrow_point(&line->start_arrow, &to, &from,
                          &move_arrow, &move_line, line->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&line->start_arrow, line->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }
  if (line->end_arrow.type != ARROW_NONE) {
    DiaRectangle bbox;
    Point move_arrow, move_line;
    Point to = end;
    Point from = start;
    calculate_arrow_point(&line->end_arrow, &to, &from,
                          &move_arrow, &move_line, line->line_width);
    /* move them */
    point_sub(&to, &move_arrow);
    point_sub(&from, &move_line);

    arrow_bbox (&line->end_arrow, line->line_width, &to, &from, &bbox);
    rectangle_union (&obj->bounding_box, &bbox);
  }

  obj->position = conn->endpoints[0];

  connpointline_update(line->cpl);
  connpointline_putonaline(line->cpl, &start, &end, DIR_ALL);

  connection_update_handles(conn);
}


static void
line_save(Line *line, ObjectNode obj_node, DiaContext *ctx)
{
#ifdef DEBUG
  dia_object_sanity_check((DiaObject*)line, "Saving line");
#endif

  connection_save(&line->connection, obj_node, ctx);

  connpointline_save(line->cpl, obj_node, "numcp", ctx);

  if (!color_equals(&line->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &line->line_color, ctx);

  if (line->line_width != 0.1)
    data_add_real(new_attribute(obj_node, PROP_STDNAME_LINE_WIDTH),
		  line->line_width, ctx);

  if (line->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  line->line_style, ctx);

  if (line->line_caps != DIA_LINE_CAPS_BUTT) {
    data_add_enum (new_attribute (obj_node, "line_caps"),
                   line->line_caps,
                   ctx);
  }

  if (line->start_arrow.type != ARROW_NONE) {
    dia_arrow_save (&line->start_arrow,
                    obj_node,
                    "start_arrow",
                    "start_arrow_length",
                    "start_arrow_width",
                    ctx);
  }

  if (line->end_arrow.type != ARROW_NONE) {
    dia_arrow_save (&line->end_arrow,
                    obj_node,
                    "end_arrow",
                    "end_arrow_length",
                    "end_arrow_width",
                    ctx);
  }

  if (line->absolute_start_gap)
    data_add_real(new_attribute(obj_node, "absolute_start_gap"),
                 line->absolute_start_gap, ctx);
  if (line->absolute_end_gap)
    data_add_real(new_attribute(obj_node, "absolute_end_gap"),
                 line->absolute_end_gap, ctx);

  if (line->line_style != DIA_LINE_STYLE_SOLID && line->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  line->dashlength, ctx);
}

static DiaObject *
line_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  Line *line;
  Connection *conn;
  DiaObject *obj;
  AttributeNode attr;

  line = g_new0 (Line, 1);

  conn = &line->connection;
  obj = &conn->object;

  obj->type = &line_type;
  obj->ops = &line_ops;

  connection_load(conn, obj_node, ctx);

  line->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &line->line_color, ctx);

  line->line_width = 0.1;
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr != NULL)
    line->line_width = data_real(attribute_first_data(attr), ctx);

  line->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    line->line_style = data_enum(attribute_first_data(attr), ctx);

  line->line_caps = DIA_LINE_CAPS_BUTT;
  attr = object_find_attribute (obj_node, "line_caps");
  if (attr != NULL) {
    line->line_caps = data_enum (attribute_first_data (attr), ctx);
  }

  dia_arrow_load (&line->start_arrow,
                  obj_node,
                  "start_arrow",
                  "start_arrow_length",
                  "start_arrow_width",
                  ctx);

  dia_arrow_load (&line->end_arrow,
                  obj_node,
                  "end_arrow",
                  "end_arrow_length",
                  "end_arrow_width",
                  ctx);

  line->absolute_start_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_start_gap");
  if (attr != NULL)
    line->absolute_start_gap =  data_real(attribute_first_data(attr), ctx);
  line->absolute_end_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_end_gap");
  if (attr != NULL)
    line->absolute_end_gap =  data_real(attribute_first_data(attr), ctx);

  line->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    line->dashlength = data_real(attribute_first_data(attr), ctx);

  connection_init(conn, 2, 0);

  line->cpl = connpointline_load(obj,obj_node,"numcp",1,NULL, ctx);
  line_update_data(line);

  return &line->connection.object;
}
