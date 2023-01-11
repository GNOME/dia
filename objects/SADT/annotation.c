/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SADT diagram support
 * Copyright (C) 2000 Cyrille Chepelov
 *
 * This file has been forked from Alexander Larsson's objects/UML/constraint.c
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
#include <stdlib.h>

#include "object.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"
#include "text.h"

#include "pixmaps/annotation.xpm"

typedef struct _Annotation {
  Connection connection;

  Handle text_handle;

  Text *text;

  Color line_color;
} Annotation;


#define ANNOTATION_LINE_WIDTH 0.05
#define ANNOTATION_BORDER 0.2
#define ANNOTATION_FONTHEIGHT 0.8
#define ANNOTATION_ZLEN .25

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)

static DiaObjectChange* annotation_move_handle(Annotation *annotation, Handle *handle,
					    Point *to, ConnectionPoint *cp,
					    HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* annotation_move(Annotation *annotation, Point *to);
static void annotation_select(Annotation *annotation, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void annotation_draw(Annotation *annotation, DiaRenderer *renderer);
static DiaObject *annotation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real annotation_distance_from(Annotation *annotation, Point *point);
static void annotation_update_data(Annotation *annotation);
static void annotation_destroy(Annotation *annotation);
static DiaObject *annotation_load(ObjectNode obj_node, int version,DiaContext *ctx);
static PropDescription *annotation_describe_props(Annotation *annotation);
static void annotation_get_props(Annotation *annotation,
                                 GPtrArray *props);
static void annotation_set_props(Annotation *annotation,
                                 GPtrArray *props);

static ObjectTypeOps annotation_type_ops =
{
  (CreateFunc) annotation_create,
  (LoadFunc)   annotation_load,/*using_properties*/
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType sadtannotation_type =
{
  "SADT - annotation", /* name */
  0,                /* version */
  annotation_xpm,    /* pixmap */
  &annotation_type_ops  /* ops */
};

static ObjectOps annotation_ops = {
  (DestroyFunc)         annotation_destroy,
  (DrawFunc)            annotation_draw,
  (DistanceFunc)        annotation_distance_from,
  (SelectFunc)          annotation_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            annotation_move,
  (MoveHandleFunc)      annotation_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   annotation_describe_props,
  (GetPropsFunc)        annotation_get_props,
  (SetPropsFunc)        annotation_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

#undef TEMPORARY_EVENT_TEST

#ifdef TEMPORARY_EVENT_TEST
static gboolean
handle_btn1(Annotation *annotation, Property *prop) {
  Color col;
  col = annotation->text->color;
  /* g_message("in handle_btn1 for object %p col=%.2f:%.2f:%.2f",
     annotation,col.red,col.green,col.blue); */
  col.red = g_random_double();
  col.green = g_random_double();
  col.blue = g_random_double();
  col.alpha = 1.0;
  annotation->text->color = col;
  /* g_message("end of handle_btn1 for object %p col=%.2f:%.2f:%.2f",
     annotation,col.red,col.green,col.blue); */
  return TRUE;
}
#endif

static PropDescription annotation_props[] = {
  CONNECTION_COMMON_PROPERTIES,
#ifdef TEMPORARY_EVENT_TEST
  {"btn1", PROP_TYPE_BUTTON, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_SAVE,
   NULL, "Click Me !", NULL,
   (PropEventHandler)handle_btn1},
#endif
  { "text", PROP_TYPE_TEXT, 0,NULL,NULL},
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  {"pos", PROP_TYPE_POINT, PROP_FLAG_DONT_SAVE},
  PROP_DESC_END
};

static PropDescription *
annotation_describe_props(Annotation *annotation)
{
  if (annotation_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(annotation_props);
  }
  return annotation_props;
}

static PropOffset annotation_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  {"text",PROP_TYPE_TEXT,offsetof(Annotation,text)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Annotation,text),offsetof(Text,alignment)},
  {"text_font",PROP_TYPE_FONT,offsetof(Annotation,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Annotation,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Annotation,text),offsetof(Text,color)},
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Annotation, line_color) },
  {"pos", PROP_TYPE_POINT, offsetof(Annotation,text_handle.pos)},
  { NULL,0,0 }
};

static void
annotation_get_props(Annotation *annotation, GPtrArray *props)
{
  object_get_props_from_offsets(&annotation->connection.object,
                                annotation_offsets,props);
}

static void
annotation_set_props(Annotation *annotation, GPtrArray *props)
{
  object_set_props_from_offsets(&annotation->connection.object,
                                annotation_offsets,props);
  annotation_update_data(annotation);
}

static real
annotation_distance_from(Annotation *annotation, Point *point)
{
  Point *endpoints;
  DiaRectangle bbox;
  endpoints = &annotation->connection.endpoints[0];

  text_calc_boundingbox(annotation->text,&bbox);
  return MIN(distance_line_point(&endpoints[0], &endpoints[1],
				 ANNOTATION_LINE_WIDTH, point),
	     distance_rectangle_point(&bbox,point));
}

static void
annotation_select(Annotation *annotation, Point *clicked_point,
	    DiaRenderer *interactive_renderer)
{
  text_set_cursor(annotation->text, clicked_point, interactive_renderer);
  text_grab_focus(annotation->text, &annotation->connection.object);

  connection_update_handles(&annotation->connection);
}

static DiaObjectChange*
annotation_move_handle(Annotation *annotation, Handle *handle,
		       Point *to, ConnectionPoint *cp,
		       HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  Connection *conn = (Connection *)annotation;

  g_assert(annotation!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    annotation->text->position = *to;
  } else  {
    endpoints = &(conn->endpoints[0]);
    if (handle->id == HANDLE_MOVE_STARTPOINT) {
      p1 = endpoints[0];
      connection_move_handle(conn, handle->id, to, cp, reason, modifiers);
      connection_adjust_for_autogap(conn);
      p2 = endpoints[0];
      point_sub(&p2, &p1);
      point_add(&annotation->text->position, &p2);
      point_add(&p2,&(endpoints[1]));
      connection_move_handle(conn, HANDLE_MOVE_ENDPOINT, &p2, NULL, reason, 0);
    } else {
      p1 = endpoints[1];
      connection_move_handle(conn, handle->id, to, cp, reason, modifiers);
      connection_adjust_for_autogap(conn);
      p2 = endpoints[1];
      point_sub(&p2, &p1);
      point_add(&annotation->text->position, &p2);
    }
  }
  annotation_update_data(annotation);

  return NULL;
}

static DiaObjectChange*
annotation_move(Annotation *annotation, Point *to)
{
  Point start_to_end;
  Point *endpoints = &annotation->connection.endpoints[0];
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&annotation->text->position, &delta);

  annotation_update_data(annotation);

  return NULL;
}

static void
annotation_draw (Annotation *annotation, DiaRenderer *renderer)
{
  Point vect,rvect,v1,v2;
  Point pts[4];
  real vlen;

  g_return_if_fail (annotation != NULL);
  g_return_if_fail (renderer != NULL);

  dia_renderer_set_linewidth (renderer, ANNOTATION_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  vect = annotation->connection.endpoints[1];
  point_sub (&vect,&annotation->connection.endpoints[0]);
  vlen = distance_point_point (&annotation->connection.endpoints[0],
                               &annotation->connection.endpoints[1]);
  if (vlen > 0.0) {
    /* draw the squiggle */
    point_scale (&vect,1/vlen);
    rvect.y = vect.x;
    rvect.x = -vect.y;

    pts[0] = annotation->connection.endpoints[0];
    pts[1] = annotation->connection.endpoints[0];
    v1 = vect;
    point_scale (&v1,.5*vlen);
    point_add (&pts[1],&v1);
    pts[2] = pts[1];
    /* pts[1] and pts[2] are currently both at the middle of (pts[0],pts[3]) */
    v1 = vect;
    point_scale (&v1,ANNOTATION_ZLEN);
    v2 = rvect;
    point_scale (&v2,ANNOTATION_ZLEN);
    point_sub (&v1,&v2);
    point_add (&pts[1],&v1);
    point_sub (&pts[2],&v1);
    pts[3] = annotation->connection.endpoints[1];
    dia_renderer_draw_polyline (renderer,
                                pts,
                                sizeof(pts) / sizeof(pts[0]),
                                &annotation->line_color);
  }
  text_draw (annotation->text,renderer);
}

static DiaObject *
annotation_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Annotation *annotation;
  Connection *conn;
  LineBBExtras *extra;
  DiaObject *obj;
  Point offs;
  Point defaultlen = { 1.0, 1.0 };
  DiaFont* font;

  annotation = g_new0 (Annotation, 1);

  conn = &annotation->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);

  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &sadtannotation_type;
  obj->ops = &annotation_ops;

  connection_init(conn, 3, 0);

  annotation->line_color = color_black;

  font = dia_font_new_from_style (DIA_FONT_SANS, ANNOTATION_FONTHEIGHT);
  annotation->text = new_text ("",
                               font,
                               ANNOTATION_FONTHEIGHT,
                               &conn->endpoints[1],
                               &color_black,
                               DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  offs.x = .3 * ANNOTATION_FONTHEIGHT;
  if (conn->endpoints[1].y < conn->endpoints[0].y)
    offs.y = 1.3 * ANNOTATION_FONTHEIGHT;
  else
    offs.y = -.3 * ANNOTATION_FONTHEIGHT;
  point_add(&annotation->text->position,&offs);

  annotation->text_handle.id = HANDLE_MOVE_TEXT;
  annotation->text_handle.type = HANDLE_MINOR_CONTROL;
  annotation->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  annotation->text_handle.connected_to = NULL;
  obj->handles[2] = &annotation->text_handle;

  extra->start_trans =
    extra->end_trans = ANNOTATION_ZLEN;
  extra->start_long =
    extra->end_long = ANNOTATION_LINE_WIDTH/2.0;
  annotation_update_data(annotation);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &annotation->connection.object;
}


static void
annotation_destroy(Annotation *annotation)
{
  connection_destroy(&annotation->connection);

  text_destroy(annotation->text);
}

static void
annotation_update_data(Annotation *annotation)
{
  Connection *conn = &annotation->connection;
  DiaObject *obj = &conn->object;
  DiaRectangle textrect;

  if (connpoint_is_autogap(conn->endpoint_handles[0].connected_to) ||
      connpoint_is_autogap(conn->endpoint_handles[1].connected_to)) {
    connection_adjust_for_autogap(conn);
  }
  obj->position = conn->endpoints[0];

  annotation->text_handle.pos = annotation->text->position;

  connection_update_handles(conn);

  connection_update_boundingbox(conn);
  text_calc_boundingbox(annotation->text,&textrect);
  rectangle_union(&obj->bounding_box, &textrect);
}

static DiaObject *
annotation_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&sadtannotation_type,
                                      obj_node,version,ctx);
}
