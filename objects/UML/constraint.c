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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "arrows.h"
#include "properties.h"
#include "stereotype.h"

#include "pixmaps/constraint.xpm"

typedef struct _Constraint Constraint;

struct _Constraint {
  Connection connection;

  Handle text_handle;

  char *text;
  char *brtext;
  Point text_pos;
  real text_width;
};

  
#define CONSTRAINT_WIDTH 0.1
#define CONSTRAINT_DASHLEN 0.4
#define CONSTRAINT_FONTHEIGHT 0.8
#define CONSTRAINT_ARROWLEN 0.8
#define CONSTRAINT_ARROWWIDTH 0.5

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static DiaFont *constraint_font = NULL;

static void constraint_move_handle(Constraint *constraint, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void constraint_move(Constraint *constraint, Point *to);
static void constraint_select(Constraint *constraint, Point *clicked_point,
			      Renderer *interactive_renderer);
static void constraint_draw(Constraint *constraint, Renderer *renderer);
static Object *constraint_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real constraint_distance_from(Constraint *constraint, Point *point);
static void constraint_update_data(Constraint *constraint);
static void constraint_destroy(Constraint *constraint);

static PropDescription *constraint_describe_props(Constraint *constraint);
static void constraint_get_props(Constraint * constraint, GPtrArray *props);
static void constraint_set_props(Constraint * constraint, GPtrArray *props);

static Object *constraint_load(ObjectNode obj_node, int version,
			       const char *filename);

static ObjectTypeOps constraint_type_ops =
{
  (CreateFunc) constraint_create,
  (LoadFunc)   constraint_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType constraint_type =
{
  "UML - Constraint",        /* name */
  0,                         /* version */
  (char **) constraint_xpm,  /* pixmap */
  &constraint_type_ops       /* ops */
};

static ObjectOps constraint_ops = {
  (DestroyFunc)         constraint_destroy,
  (DrawFunc)            constraint_draw,
  (DistanceFunc)        constraint_distance_from,
  (SelectFunc)          constraint_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            constraint_move,
  (MoveHandleFunc)      constraint_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   constraint_describe_props,
  (GetPropsFunc)        constraint_get_props,
  (SetPropsFunc)        constraint_set_props
};

static PropDescription constraint_props[] = {
  CONNECTION_COMMON_PROPERTIES,
  { "constraint", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Constraint:"), NULL, NULL },
  { "text_pos", PROP_TYPE_POINT, 0, NULL, NULL, NULL},
  PROP_DESC_END
};

static PropDescription *
constraint_describe_props(Constraint *constraint)
{
  return constraint_props;
}

static PropOffset constraint_offsets[] = {
  CONNECTION_COMMON_PROPERTIES_OFFSETS,
  { "constraint", PROP_TYPE_STRING, offsetof(Constraint, text) },
  { "text_pos", PROP_TYPE_POINT, offsetof(Constraint, text_pos) },
  { NULL, 0, 0 }
};

static void
constraint_get_props(Constraint * constraint, GPtrArray *props)
{
  object_get_props_from_offsets(&constraint->connection.object, 
                                constraint_offsets, props);
}

static void
constraint_set_props(Constraint *constraint, GPtrArray *props)
{
  object_set_props_from_offsets(&constraint->connection.object, 
                                constraint_offsets, props);
  g_free(constraint->brtext);
  constraint->brtext = NULL;
  constraint_update_data(constraint);
}

static real
constraint_distance_from(Constraint *constraint, Point *point)
{
  Point *endpoints;
  real dist;
  
  endpoints = &constraint->connection.endpoints[0];
  
  dist = distance_line_point(&endpoints[0], &endpoints[1], 
                             CONSTRAINT_WIDTH, point);  
  return dist;
}

static void
constraint_select(Constraint *constraint, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&constraint->connection);
}

static void
constraint_move_handle(Constraint *constraint, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  
  assert(constraint!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    constraint->text_pos = *to;
  } else  {
    endpoints = &constraint->connection.endpoints[0]; 
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&constraint->connection, handle->id, to, reason);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&constraint->text_pos, &p2);
  }

  constraint_update_data(constraint);
}

static void
constraint_move(Constraint *constraint, Point *to)
{
  Point start_to_end;
  Point *endpoints = &constraint->connection.endpoints[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);
 
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&constraint->text_pos, &delta);
  
  constraint_update_data(constraint);
}

static void
constraint_draw(Constraint *constraint, Renderer *renderer)
{
  Point *endpoints;
  
  assert(constraint != NULL);
  assert(renderer != NULL);

  endpoints = &constraint->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, CONSTRAINT_WIDTH);
  renderer->ops->set_dashlength(renderer, CONSTRAINT_DASHLEN);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);
  
  arrow_draw(renderer, ARROW_LINES,
	     &endpoints[1], &endpoints[0],
	     CONSTRAINT_ARROWLEN, CONSTRAINT_ARROWWIDTH, CONSTRAINT_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, constraint_font,
			  CONSTRAINT_FONTHEIGHT);
  renderer->ops->draw_string(renderer,
			     constraint->brtext,
			     &constraint->text_pos, ALIGN_LEFT,
			     &color_black);
}

static Object *
constraint_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Constraint *constraint;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  if (constraint_font == NULL) {
	  constraint_font = dia_font_new(BASIC_MONOSPACE_FONT,STYLE_NORMAL,
                                   CONSTRAINT_FONTHEIGHT);
  }
  
  constraint = g_malloc0(sizeof(Constraint));

  conn = &constraint->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = &conn->object;

  obj->type = &constraint_type;
  obj->ops = &constraint_ops;
  
  connection_init(conn, 3, 0);

  constraint->text = g_strdup("");
  constraint->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  constraint->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  constraint->text_handle.id = HANDLE_MOVE_TEXT;
  constraint->text_handle.type = HANDLE_MINOR_CONTROL;
  constraint->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  constraint->text_handle.connected_to = NULL;
  obj->handles[2] = &constraint->text_handle;
  
  constraint->brtext = NULL;
  constraint_update_data(constraint);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &constraint->connection.object;
}


static void
constraint_destroy(Constraint *constraint)
{
  connection_destroy(&constraint->connection);
  g_free(constraint->brtext);
  g_free(constraint->text);
}

static void
constraint_update_data(Constraint *constraint)
{
  Connection *conn = &constraint->connection;
  Object *obj = &conn->object;
  Rectangle rect;
  LineBBExtras *extra;

  if ((constraint->text) && (constraint->text[0] == '{')) {
    /* we might have a string loaded from an older dia. Clean it up. */
    g_free(constraint->brtext);
    constraint->brtext = constraint->text;
    constraint->text = bracketted_to_string(constraint->text,"{","}");
  } else if (!constraint->brtext) {
    constraint->brtext = string_to_bracketted(constraint->text, "{", "}");
  }
  
  obj->position = conn->endpoints[0];

  constraint->text_width = dia_font_string_width(constraint->brtext, 
                                                 constraint_font, 
                                                 CONSTRAINT_FONTHEIGHT);
  
  constraint->text_handle.pos = constraint->text_pos;

  connection_update_handles(conn);

  /* Boundingbox: */
  extra = &conn->extra_spacing;

  extra->start_long = 
    extra->start_trans = 
    extra->end_long = CONSTRAINT_WIDTH/2.0;
  extra->end_trans = MAX(CONSTRAINT_WIDTH,CONSTRAINT_ARROWLEN)/2.0;
  
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = constraint->text_pos.x;
  rect.right = rect.left + constraint->text_width;
  rect.top = constraint->text_pos.y -
      dia_font_ascent(constraint->brtext,
                      constraint_font, CONSTRAINT_FONTHEIGHT);
  rect.bottom = rect.top + CONSTRAINT_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);
}


static Object *
constraint_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&constraint_type,
                                      obj_node,version,filename);
}
