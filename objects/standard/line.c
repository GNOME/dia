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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "widgets.h"
#include "arrows.h"
#include "connpoint_line.h"
#include "properties.h"

#include "tool-icons.h"

#define DEFAULT_WIDTH 0.25

typedef struct _LineProperties LineProperties;

typedef struct _Line {
  Connection connection;

  ConnPointLine *cpl;

  Color line_color;
  real line_width;
  LineStyle line_style;  
  Arrow start_arrow, end_arrow;
  real dashlength;
  real absolute_start_gap, absolute_end_gap;
} Line;

struct _LineProperties {
  real absolute_start_gap, absolute_end_gap;
};

static LineProperties default_properties;

static ObjectChange* line_move_handle(Line *line, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason, 
			     ModifierKeys modifiers);
static ObjectChange* line_move(Line *line, Point *to);
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

static PropDescription *line_describe_props(Line *line);
static void line_get_props(Line *line, GPtrArray *props);
static void line_set_props(Line *line, GPtrArray *props);

static void line_save(Line *line, ObjectNode obj_node, const char *filename);
static DiaObject *line_load(ObjectNode obj_node, int version, const char *filename);
static DiaMenu *line_get_object_menu(Line *line, Point *clickedpoint);

void line_adjust_for_autogap(Line *line);
void line_adjust_for_absolute_gap(Line *line, Point *gap_endpoints);

static ObjectTypeOps line_type_ops =
{
  (CreateFunc) line_create,
  (LoadFunc)   line_load,
  (SaveFunc)   line_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType line_type =
{
  "Standard - Line",   /* name */
  0,                   /* version */
  (char **) line_icon,  /* pixmap */
  &line_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      line_get_object_menu,
  (DescribePropsFunc)   line_describe_props,
  (GetPropsFunc)        line_get_props,
  (SetPropsFunc)        line_set_props
};

static PropNumData gap_range = { -G_MAXFLOAT, G_MAXFLOAT, 0.1};

static PropDescription line_props[] = {
  OBJECT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_LINE_STYLE,
  PROP_FRAME_BEGIN("arrows",PROP_FLAG_STANDARD,N_("Arrows")),
  PROP_STD_START_ARROW,
  PROP_STD_END_ARROW,
  PROP_FRAME_END("arrows",PROP_FLAG_STANDARD),
  { "start_point", PROP_TYPE_POINT, 0,
    N_("Start point"), NULL },
  { "end_point", PROP_TYPE_POINT, 0,
    N_("End point"), NULL },
  
  PROP_FRAME_BEGIN("gaps",0,N_("Line gaps")),
  { "absolute_start_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Absolute start gap"), NULL, &gap_range },
  { "absolute_end_gap", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Absolute end gap"), NULL, &gap_range },
  PROP_FRAME_END("gaps",0),
 
  PROP_DESC_END
};

static PropDescription *
line_describe_props(Line *line)
{
  if (line_props[0].quark == 0)
    prop_desc_list_calculate_quarks(line_props);
  return line_props;
}

static PropOffset line_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Line, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Line, line_color) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Line, line_style), offsetof(Line, dashlength) },
  { "start_arrow", PROP_TYPE_ARROW, offsetof(Line, start_arrow) },
  { "end_arrow", PROP_TYPE_ARROW, offsetof(Line, end_arrow) },
  { "start_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[0]) },
  { "end_point", PROP_TYPE_POINT, offsetof(Connection, endpoints[1]) },
  { "absolute_start_gap", PROP_TYPE_REAL, offsetof(Line, absolute_start_gap) },
  { "absolute_end_gap", PROP_TYPE_REAL, offsetof(Line, absolute_end_gap) },
  { NULL, 0, 0 }
};

static void
line_get_props(Line *line, GPtrArray *props)
{
  object_get_props_from_offsets(&line->connection.object, 
                                line_offsets, props);
}

static void
line_set_props(Line *line, GPtrArray *props)
{
  object_set_props_from_offsets(&line->connection.object, 
                                line_offsets, props);
  line_update_data(line);
}

static void
line_init_defaults() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.absolute_start_gap = 0.0;
    default_properties.absolute_end_gap = 0.0;
    defaults_initialized = 1;
  }
}

static ObjectChange *
line_add_connpoint_callback(DiaObject *obj, Point *clicked, gpointer data) 
{
  ObjectChange *oc;
  oc = connpointline_add_point(((Line *)obj)->cpl,clicked);
  line_update_data((Line *)obj);
  return oc;
}

static ObjectChange *
line_remove_connpoint_callback(DiaObject *obj, Point *clicked, gpointer data) 
{
  ObjectChange *oc;
  oc = connpointline_remove_point(((Line *)obj)->cpl,clicked);
  line_update_data((Line *)obj);
  return oc;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), line_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), line_remove_connpoint_callback, 
    NULL, 1 },
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

/*
  About start_gap, end_gap, auto/absolute
  
  Place positive reals (try 1.0, for instance) in absolute 
  to create gaps on line end/start.  
  If auto_gap is false, these gaps are of that length.
  If a gap is negative, the line extends past the handle.
  If auto_gap is true, the gap length is computed so it touches 
  the connected object edge then the absolute gap is added
  */
  
Point
calculate_object_edge(Point *objmid, Point *end, DiaObject *obj) 
{
#define MAXITER 25
#ifdef TRACE_DIST
  Point trace[MAXITER];
  real disttrace[MAXITER];
#endif
  Point mid1, mid2, mid3;
  real dist;
  int i = 0;

  mid1 = *objmid;
  mid2.x = (objmid->x+end->x)/2;
  mid2.y = (objmid->y+end->y)/2;
  mid3 = *end;

  /* If the other end is inside the object */
  dist = obj->ops->distance_from(obj, &mid3);
  if (dist < 0.001) return mid1;


  do {
    dist = obj->ops->distance_from(obj, &mid2);
    if (dist < 0.0000001) {
      mid1 = mid2;
    } else {
      mid3 = mid2;
    }
    mid2.x = (mid1.x + mid3.x)/2;
    mid2.y = (mid1.y + mid3.y)/2;
#ifdef TRACE_DIST
    trace[i] = mid2;
    disttrace[i] = dist;
#endif
    i++;
  } while (i < MAXITER && (dist < 0.0000001 || dist > 0.001));
  
#ifdef TRACE_DIST
  if (i == MAXITER) {
    for (i = 0; i < MAXITER; i++) {
      printf("%d: %f, %f: %f\n", i, trace[i].x, trace[i].y, disttrace[i]);
    }
    printf("i = %d, dist = %f\n", i, dist);
  }
#endif

  return mid2;
}


/** Adjust line endings for autogap.  This function actually moves the
 * ends of the line, but only when the end is connected to 
 * a mainpoint.
 */
void
line_adjust_for_autogap(Line *line)
{
  Point endpoints[2];
  ConnectionPoint *start_cp, *end_cp;

  start_cp = line->connection.endpoint_handles[0].connected_to;
  end_cp = line->connection.endpoint_handles[1].connected_to;

  endpoints[0] = line->connection.endpoints[0];
  endpoints[1] = line->connection.endpoints[1];

  if (connpoint_is_autogap(start_cp)) {
    endpoints[0] = start_cp->pos;
  }
  if (connpoint_is_autogap(end_cp)) {    
    endpoints[1] = end_cp->pos;
  }

  if (connpoint_is_autogap(start_cp)) {
    line->connection.endpoints[0] = calculate_object_edge(&endpoints[0],
							  &endpoints[1],
							  start_cp->object);
  }
  if (connpoint_is_autogap(end_cp)) {    
    line->connection.endpoints[1] = calculate_object_edge(&endpoints[1],
							  &endpoints[0],
							  end_cp->object);
  }
}

/** Calculate the absolute gap -- this gap is 'transient', in that
 * the actual end of the line is not moved, but it is made to look like
 * it is shorter.
 */
void
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

static ObjectChange*
line_move_handle(Line *line, Handle *handle,
		 Point *to, ConnectionPoint *cp,
		 HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(line!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  connection_move_handle(&line->connection, handle->id, to, cp, reason, modifiers);

  line_update_data(line);

  return NULL;
}

static ObjectChange*
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
line_draw(Line *line, DiaRenderer *renderer)
{
  Point gap_endpoints[2];

  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  
  assert(line != NULL);
  assert(renderer != NULL);

  renderer_ops->set_linewidth(renderer, line->line_width);
  renderer_ops->set_linestyle(renderer, line->line_style);
  renderer_ops->set_dashlength(renderer, line->dashlength);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (line->absolute_start_gap || line->absolute_end_gap ) {
    line_adjust_for_absolute_gap(line, gap_endpoints);

    renderer_ops->draw_line_with_arrows(renderer,
					&gap_endpoints[0], &gap_endpoints[1],
					line->line_width,
					&line->line_color,
					&line->start_arrow,
					&line->end_arrow);
  } else {
        renderer_ops->draw_line_with_arrows(renderer,
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

  line = g_malloc0(sizeof(Line));

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
  
  newline = g_malloc0(sizeof(Line));
  newconn = &newline->connection;
  newobj = &newconn->object;
  
  connection_copy(conn, newconn);

  newline->cpl = connpointline_copy(newobj,line->cpl,&rcc);
  
  newline->line_color = line->line_color;
  newline->line_width = line->line_width;
  newline->line_style = line->line_style;
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

  extra->start_trans = (line->line_width / 2.0);
  extra->end_trans   = (line->line_width / 2.0);
  extra->start_long  = (line->line_width / 2.0);
  extra->end_long    = (line->line_width / 2.0);
  if (line->start_arrow.type != ARROW_NONE) 
    extra->start_trans = MAX(extra->start_trans,line->start_arrow.width);
  if (line->end_arrow.type != ARROW_NONE) 
    extra->end_trans = MAX(extra->end_trans,line->end_arrow.width);

  if (connpoint_is_autogap(line->connection.endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(line->connection.endpoint_handles[1].connected_to)) {
    line_adjust_for_autogap(line);
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

  obj->position = conn->endpoints[0];

  connpointline_update(line->cpl);
  connpointline_putonaline(line->cpl,&start, &end);
  
  connection_update_handles(conn);
}


static void
line_save(Line *line, ObjectNode obj_node, const char *filename)
{
  connection_save(&line->connection, obj_node);

  connpointline_save(line->cpl,obj_node,"numcp");

  if (!color_equals(&line->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &line->line_color);
  
  if (line->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  line->line_width);
  
  if (line->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  line->line_style);
  
  if (line->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  line->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  line->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  line->start_arrow.width);
  }
  
  if (line->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  line->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  line->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  line->end_arrow.width);
  }
 
  if (line->absolute_start_gap)
    data_add_real(new_attribute(obj_node, "absolute_start_gap"),
                 line->absolute_start_gap);
  if (line->absolute_end_gap)
    data_add_real(new_attribute(obj_node, "absolute_end_gap"),
                 line->absolute_end_gap);

  if (line->line_style != LINESTYLE_SOLID && line->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  line->dashlength);
}

static DiaObject *
line_load(ObjectNode obj_node, int version, const char *filename)
{
  Line *line;
  Connection *conn;
  DiaObject *obj;
  AttributeNode attr;

  line = g_malloc0(sizeof(Line));

  conn = &line->connection;
  obj = &conn->object;

  obj->type = &line_type;
  obj->ops = &line_ops;

  connection_load(conn, obj_node);

  line->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &line->line_color);

  line->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    line->line_width = data_real(attribute_first_data(attr));

  line->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    line->line_style = data_enum(attribute_first_data(attr));

  line->start_arrow.type = ARROW_NONE;
  line->start_arrow.length = DEFAULT_ARROW_LENGTH;
  line->start_arrow.width = DEFAULT_ARROW_WIDTH;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    line->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    line->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    line->start_arrow.width = data_real(attribute_first_data(attr));

  line->end_arrow.type = ARROW_NONE;
  line->end_arrow.length = DEFAULT_ARROW_LENGTH;
  line->end_arrow.width = DEFAULT_ARROW_WIDTH;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    line->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    line->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    line->end_arrow.width = data_real(attribute_first_data(attr));

  line->absolute_start_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_start_gap");
  if (attr != NULL)
    line->absolute_start_gap =  data_real( attribute_first_data(attr) );
  line->absolute_end_gap = 0.0;
  attr = object_find_attribute(obj_node, "absolute_end_gap");
  if (attr != NULL)
    line->absolute_end_gap =  data_real( attribute_first_data(attr) );

  line->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    line->dashlength = data_real(attribute_first_data(attr));

  connection_init(conn, 2, 0);

  line->cpl = connpointline_load(obj,obj_node,"numcp",1,NULL);
  line_update_data(line);

  return &line->connection.object;
}
