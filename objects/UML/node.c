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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"

#include "pixmaps/node.xpm"

typedef struct _Node Node;

struct _Node
{
  Element element;
  ConnectionPoint connections[8];
  Text *name;
  TextAttributes attrs;

  Color line_color;
  Color fill_color;
};

static const double NODE_BORDERWIDTH = 0.1;
static const double NODE_DEPTH = 0.5;
static const double NODE_FONTHEIGHT = 0.8;
static const double NODE_LINEWIDTH = 0.05;
static const double NODE_TEXT_MARGIN = 0.5;

static real node_distance_from(Node *node, Point *point);
static void node_select(Node *node, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static ObjectChange* node_move_handle(Node *node, Handle *handle,
				      Point *to, ConnectionPoint *cp,
				      HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* node_move(Node *node, Point *to);
static void node_draw(Node *node, DiaRenderer *renderer);
static DiaObject *node_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void node_destroy(Node *node);
static DiaObject *node_load(ObjectNode obj_node, int version,
				 const char *filename);

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
  "UML - Node",   /* name */
  0,                      /* version */
  (char **) node_xpm,  /* pixmap */
  
  &node_type_ops       /* ops */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   node_describe_props,
  (GetPropsFunc)        node_get_props,
  (SetPropsFunc)        node_set_props
};

static PropDescription node_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_COLOUR_OPTIONAL, 
  PROP_STD_FILL_COLOUR_OPTIONAL, 
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  { "name", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL }, 
  
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
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Node,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Node,fill_color)},
  {"name",PROP_TYPE_TEXT,offsetof(Node,name)},
  {"text_font",PROP_TYPE_FONT,offsetof(Node,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Node,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Node,attrs.color)},
  { NULL, 0, 0 },
};

static void
node_get_props(Node * node, GPtrArray *props)
{
  text_get_attributes(node->name,&node->attrs);
  object_get_props_from_offsets(&node->element.object,
                                node_offsets,props);
}

static void
node_set_props(Node *node, GPtrArray *props)
{
  object_set_props_from_offsets(&node->element.object,
                                node_offsets,props);
  apply_textattr_properties(props,node->name,"name",&node->attrs);
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

static ObjectChange*
node_move_handle(Node *node, Handle *handle,
		 Point *to, ConnectionPoint *cp,
		 HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(node!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  
  element_move_handle(&node->element, handle->id, to, cp, reason, modifiers);
  node_update_data(node);

  return NULL;
}

static ObjectChange*
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

static void node_draw(Node *node, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Element *elem;
  real x, y, w, h;
  Point points[4];
  int i;
  
  assert(node != NULL);
  assert(renderer != NULL);

  elem = &node->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer_ops->set_linewidth(renderer, NODE_BORDERWIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);

  points[0].x = x;     points[0].y = y;
  points[1].x = x + w; points[1].y = y + h;

  renderer_ops->fill_rect(renderer, points, points + 1, &node->fill_color);
  renderer_ops->draw_rect(renderer, points, points + 1, &node->line_color);

  points[1].x = x + NODE_DEPTH, points[1].y = y - NODE_DEPTH;
  points[2].x = x + w + NODE_DEPTH, points[2].y = y - NODE_DEPTH;
  points[3].x = x + w, points[3].y = y;

  renderer_ops->fill_polygon(renderer, points, 4, &node->fill_color);
  renderer_ops->draw_polygon(renderer, points, 4, &node->line_color);

  points[0].x = points[3].x, points[0].y = points[3].y;
  points[1].x = points[0].x + NODE_DEPTH, points[1].y = points[0].y - NODE_DEPTH;
  points[2].x = points[0].x + NODE_DEPTH, points[2].y = points[0].y - NODE_DEPTH + h;
  points[3].x = points[0].x, points[3].y = points[0].y + h;

  renderer_ops->fill_polygon(renderer, points, 4, &node->fill_color);
  renderer_ops->draw_polygon(renderer, points, 4, &node->line_color);

  text_draw(node->name, renderer);
  
  renderer_ops->set_linewidth(renderer, NODE_LINEWIDTH);
  points[0].x = node->name->position.x;
  points[0].y = points[1].y = node->name->position.y + node->name->descent;
  for (i = 0; i < node->name->numlines; i++)
    { 
      points[1].x = points[0].x + node->name->row_width[i];
      renderer_ops->draw_line(renderer, points, points + 1, &color_black);
      points[0].y = points[1].y += node->name->height;
    }
}

static void 
node_update_data(Node *node)
{
  Element *elem = &node->element;
  DiaObject *obj = &node->element.object;
  DiaFont *font;
  Point p1;
  real h, w = 0;

  text_calc_boundingbox(node->name, NULL);

  font = node->name->font;
  h = elem->corner.y + NODE_TEXT_MARGIN;

  w = node->name->max_width;
  p1.x = elem->corner.x + NODE_TEXT_MARGIN;
  p1.y = h + node->name->ascent;  /* position of text */
  text_set_position(node->name, &p1);
  h += node->name->height * node->name->numlines;

  elem->width = MAX(elem->width, node->name->max_width + 2*NODE_TEXT_MARGIN);
  elem->height = MAX(elem->height, node->name->height * node->name->numlines + 2*NODE_TEXT_MARGIN);
  
  /* Update connections: */
  node->connections[0].pos = elem->corner;
  node->connections[0].directions = DIR_NORTH|DIR_WEST;
  node->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  node->connections[1].pos.y = elem->corner.y;
  node->connections[1].directions = DIR_NORTH;
  node->connections[2].pos.x = elem->corner.x + elem->width;
  node->connections[2].pos.y = elem->corner.y;
  node->connections[2].directions = DIR_NORTH|DIR_EAST;
  node->connections[3].pos.x = elem->corner.x;
  node->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  node->connections[3].directions = DIR_WEST;
  node->connections[4].pos.x = elem->corner.x + elem->width;
  node->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  node->connections[4].directions = DIR_EAST;
  node->connections[5].pos.x = elem->corner.x;
  node->connections[5].pos.y = elem->corner.y + elem->height;
  node->connections[5].directions = DIR_SOUTH|DIR_WEST;
  node->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  node->connections[6].pos.y = elem->corner.y + elem->height;
  node->connections[6].directions = DIR_SOUTH;
  node->connections[7].pos.x = elem->corner.x + elem->width;
  node->connections[7].pos.y = elem->corner.y + elem->height;
  node->connections[7].directions = DIR_SOUTH|DIR_EAST;
  
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
  
  node = g_malloc0(sizeof(Node));
  elem = &node->element;
  obj = &elem->object;
  
  obj->type = &node_type;

  obj->ops = &node_ops;

  elem->corner = *startpoint;

  node->line_color = attributes_get_foreground();
  node->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, 0.8);
  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  node->name = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  text_get_attributes(node->name,&node->attrs);
  dia_font_unref(font);
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &node->connections[i];
    node->connections[i].object = obj;
    node->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = NODE_BORDERWIDTH/2.0;
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

static DiaObject *node_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&node_type,
                                      obj_node,version,filename);
}
