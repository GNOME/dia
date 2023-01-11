/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * node type for UML diagrams
 * Copyright (C) 2000 Stefan Seefeld <stefan@berlin-consortium.org>
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

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"

#include "pixmaps/node.xpm"

typedef struct _Node Node;

#define NUM_CONNECTIONS 9

struct _Node
{
  Element element;
  ConnectionPoint connections[NUM_CONNECTIONS];
  Text *name;

  Color line_color;
  Color fill_color;

  real  line_width;
};

static const double NODE_BORDERWIDTH = 0.1;
static const double NODE_DEPTH = 0.5;
static const double NODE_FONTHEIGHT = 0.8;
static const double NODE_LINEWIDTH = 0.05;
static const double NODE_TEXT_MARGIN = 0.5;

static real node_distance_from(Node *node, Point *point);
static void node_select(Node *node, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static DiaObjectChange* node_move_handle(Node *node, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* node_move(Node *node, Point *to);
static void node_draw(Node *node, DiaRenderer *renderer);
static DiaObject *node_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void node_destroy(Node *node);
static DiaObject *node_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *node_describe_props(Node *node);
static void node_get_props(Node *node, GPtrArray *props);
static void node_set_props(Node *node, GPtrArray *props);

static void node_update_data(Node *node);

static ObjectTypeOps node_type_ops =
{
  (CreateFunc) node_create,
  (LoadFunc)   node_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType node_type =
{
  "UML - Node",  /* name */
  0,             /* version */
  node_xpm,      /* pixmap */
  &node_type_ops /* ops */
};

static ObjectOps node_ops =
{
  (DestroyFunc)         node_destroy,
  (DrawFunc)            node_draw,
  (DistanceFunc)        node_distance_from,
  (SelectFunc)          node_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            node_move,
  (MoveHandleFunc)      node_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   node_describe_props,
  (GetPropsFunc)        node_get_props,
  (SetPropsFunc)        node_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription node_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  /* how it used to be before 0.96+SVN */
  { "name", PROP_TYPE_TEXT, PROP_FLAG_OPTIONAL, N_("Text"), NULL, NULL },
  /* new name matching "same name, same type"  rule */
  { "text", PROP_TYPE_TEXT, PROP_FLAG_NO_DEFAULTS|PROP_FLAG_LOAD_ONLY|PROP_FLAG_OPTIONAL, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,

  PROP_DESC_END
};


static PropDescription *
node_describe_props(Node *node)
{
  if (node_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(node_props);
  }
  return node_props;
}

static PropOffset node_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  /* backward compatibility */
  {"name",PROP_TYPE_TEXT,offsetof(Node,name)},
  /* new name matching "same name, same type"  rule */
  {"text",PROP_TYPE_TEXT,offsetof(Node,name)},
  {"text_font",PROP_TYPE_FONT,offsetof(Node,name),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Node,name),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Node,name),offsetof(Text,color)},
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Node, line_width) },
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Node,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Node,fill_color)},
  { NULL, 0, 0 },
};

static void
node_get_props(Node * node, GPtrArray *props)
{
  object_get_props_from_offsets(&node->element.object,
                                node_offsets,props);
}

static void
node_set_props(Node *node, GPtrArray *props)
{
  object_set_props_from_offsets(&node->element.object,
                                node_offsets,props);
  node_update_data(node);
}


static real
node_distance_from(Node *node, Point *point)
{
  DiaObject *obj = &node->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
node_select(Node *node, Point *clicked_point,
		    DiaRenderer *interactive_renderer)
{
  text_set_cursor(node->name, clicked_point, interactive_renderer);
  text_grab_focus(node->name, &node->element.object);
  element_update_handles(&node->element);
}


static DiaObjectChange *
node_move_handle (Node             *node,
                  Handle           *handle,
                  Point            *to,
                  ConnectionPoint  *cp,
                  HandleMoveReason  reason,
                  ModifierKeys      modifiers)
{
  g_return_val_if_fail (node != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  element_move_handle (&node->element, handle->id, to, cp, reason, modifiers);
  node_update_data (node);

  return NULL;
}


static DiaObjectChange*
node_move(Node *node, Point *to)
{
  Point p;

  node->element.corner = *to;

  p = *to;
  p.x += NODE_TEXT_MARGIN;
  p.y += node->name->ascent + NODE_TEXT_MARGIN;
  text_set_position(node->name, &p);

  node_update_data(node);

  return NULL;
}


static void
node_draw (Node *node, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point points[7];
  int i;

  g_return_if_fail (node != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &node->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, node->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  /* Draw outer box */
  points[0].x = x;                  points[0].y = y;
  points[1].x = x + NODE_DEPTH;     points[1].y = y - NODE_DEPTH;
  points[2].x = x + w + NODE_DEPTH; points[2].y = y - NODE_DEPTH;
  points[3].x = x + w + NODE_DEPTH; points[3].y = y + h - NODE_DEPTH;
  points[4].x = x + w;              points[4].y = y + h;
  points[5].x = x;                  points[5].y = y + h;
  points[6].x = x;                  points[6].y = y;

  dia_renderer_draw_polygon (renderer, points, 7, &node->fill_color, &node->line_color);

  /* Draw interior lines */
  points[0].x = x;                  points[0].y = y;
  points[1].x = x + w;              points[1].y = y;
  dia_renderer_draw_line (renderer, &points[0], &points[1], &node->line_color);

  points[0].x = x + w;              points[0].y = y;
  points[1].x = x + w + NODE_DEPTH; points[1].y = y - NODE_DEPTH;
  dia_renderer_draw_line (renderer, &points[0], &points[1], &node->line_color);

  points[0].x = x + w;              points[0].y = y;
  points[1].x = x + w;              points[1].y = y + h;
  dia_renderer_draw_line (renderer, &points[0], &points[1], &node->line_color);

  /* Draw text */
  text_draw (node->name, renderer);

  /* Draw underlines (!) */
  dia_renderer_set_linewidth (renderer, NODE_LINEWIDTH);
  points[0].x = node->name->position.x;
  points[0].y = points[1].y = node->name->position.y + node->name->descent;
  for (i = 0; i < node->name->numlines; i++) {
    points[1].x = points[0].x + text_get_line_width (node->name, i);
    dia_renderer_draw_line (renderer, points, points + 1, &node->name->color);
    points[0].y = points[1].y += node->name->height;
  }
}

static void
node_update_data(Node *node)
{
  Element *elem = &node->element;
  DiaObject *obj = &node->element.object;
  Point p1;
  real h;

  text_calc_boundingbox(node->name, NULL);

  h = elem->corner.y + NODE_TEXT_MARGIN;

  p1.x = elem->corner.x + NODE_TEXT_MARGIN;
  p1.y = h + node->name->ascent;  /* position of text */
  text_set_position(node->name, &p1);

  elem->width = MAX(elem->width, node->name->max_width + 2*NODE_TEXT_MARGIN);
  elem->height = MAX(elem->height, node->name->height * node->name->numlines + 2*NODE_TEXT_MARGIN);

  /* Update connections: */
  element_update_connections_rectangle(elem, node->connections);

  element_update_boundingbox(elem);
  /* fix boundingbox for depth: */
  obj->bounding_box.top -= NODE_DEPTH;
  obj->bounding_box.right += NODE_DEPTH;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *node_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  Node *node;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  node = g_new0 (Node, 1);

  /* old defaults */
  node->line_width = NODE_BORDERWIDTH;

  elem = &node->element;
  obj = &elem->object;

  obj->type = &node_type;

  obj->ops = &node_ops;

  elem->corner = *startpoint;

  node->line_color = attributes_get_foreground();
  node->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, NODE_FONTHEIGHT);
  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  node->name = new_text ("",
                         font,
                         NODE_FONTHEIGHT,
                         &p,
                         &color_black,
                         DIA_ALIGN_LEFT);
  g_clear_object (&font);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &node->connections[i];
    node->connections[i].object = obj;
    node->connections[i].connected = NULL;
  }
  node->connections[8].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = node->line_width/2.0;
  node_update_data(node);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &node->element.object;
}

static void node_destroy(Node *node)
{
  text_destroy(node->name);
  element_destroy(&node->element);
}

static DiaObject *node_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&node_type,
                                      obj_node,version,ctx);
}
