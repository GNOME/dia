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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"

#include "uml.h"

#include "pixmaps/node.xpm"

typedef struct _Node Node;

struct _Node
{
  Element element;
  ConnectionPoint connections[8];
  Text *name;
};

static const double NODE_BORDERWIDTH = 0.1;
static const double NODE_DEPTH = 0.5;
static const double NODE_FONTHEIGHT = 0.8;
static const double NODE_LINEWIDTH = 0.05;
static const double NODE_TEXT_MARGIN = 0.5;

static real node_distance_from(Node *node, Point *point);
static void node_select(Node *node, Point *clicked_point,
				Renderer *interactive_renderer);
static void node_move_handle(Node *node, Handle *handle,
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void node_move(Node *node, Point *to);
static void node_draw(Node *node, Renderer *renderer);
static Object *node_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void node_destroy(Node *node);
static Object *node_copy(Node *node);

static void node_save(Node *node, ObjectNode obj_node,
			      const char *filename);
static Object *node_load(ObjectNode obj_node, int version,
				 const char *filename);

static void node_update_data(Node *node);

static ObjectTypeOps node_type_ops =
{
  (CreateFunc) node_create,
  (LoadFunc)   node_load,
  (SaveFunc)   node_save
};

ObjectType node_type =
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
  (CopyFunc)            node_copy,
  (MoveFunc)            node_move,
  (MoveHandleFunc)      node_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (ObjectMenuFunc)      NULL
};

static real
node_distance_from(Node *node, Point *point)
{
  Object *obj = &node->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
node_select(Node *node, Point *clicked_point,
		    Renderer *interactive_renderer)
{
  text_set_cursor(node->name, clicked_point, interactive_renderer);
  text_grab_focus(node->name, (Object *)node);
  element_update_handles(&node->element);
}

static void
node_move_handle(Node *node, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(node!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  
  element_move_handle(&node->element, handle->id, to, reason);
  node_update_data(node);
}

static void
node_move(Node *node, Point *to)
{
  node->element.corner = *to;

  node_update_data(node);
}

static void node_draw(Node *node, Renderer *renderer)
{
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
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, NODE_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  points[0].x = x;     points[0].y = y;
  points[1].x = x + w; points[1].y = y + h;

  renderer->ops->fill_rect(renderer, points, points + 1, &color_white);
  renderer->ops->draw_rect(renderer, points, points + 1, &color_black);

  points[1].x = x + NODE_DEPTH, points[1].y = y - NODE_DEPTH;
  points[2].x = x + w + NODE_DEPTH, points[2].y = y - NODE_DEPTH;
  points[3].x = x + w, points[3].y = y;

  renderer->ops->fill_polygon(renderer, points, 4, &color_white);
  renderer->ops->draw_polygon(renderer, points, 4, &color_black);

  points[0].x = points[3].x, points[0].y = points[3].y;
  points[1].x = points[0].x + NODE_DEPTH, points[1].y = points[0].y - NODE_DEPTH;
  points[2].x = points[0].x + NODE_DEPTH, points[2].y = points[0].y - NODE_DEPTH + h;
  points[3].x = points[0].x, points[3].y = points[0].y + h;

  renderer->ops->fill_polygon(renderer, points, 4, &color_white);
  renderer->ops->draw_polygon(renderer, points, 4, &color_black);

  text_draw(node->name, renderer);
  
  renderer->ops->set_linewidth(renderer, NODE_LINEWIDTH);
  points[0].x = node->name->position.x;
  points[0].y = points[1].y = node->name->position.y + node->name->descent;
  for (i = 0; i < node->name->numlines; i++)
    { 
      points[1].x = points[0].x + node->name->row_width[i];
      renderer->ops->draw_line(renderer, points, points + 1, &color_black);
      points[0].y = points[1].y += node->name->height;
    }
}

static void node_update_data(Node *node)
{
  Element *elem = &node->element;
  Object *obj = (Object *) node;
  Font *font;
  Point p1;
  real h, w = 0;

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
  node->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  node->connections[1].pos.y = elem->corner.y;
  node->connections[2].pos.x = elem->corner.x + elem->width;
  node->connections[2].pos.y = elem->corner.y;
  node->connections[3].pos.x = elem->corner.x;
  node->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  node->connections[4].pos.x = elem->corner.x + elem->width;
  node->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  node->connections[5].pos.x = elem->corner.x;
  node->connections[5].pos.y = elem->corner.y + elem->height;
  node->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  node->connections[6].pos.y = elem->corner.y + elem->height;
  node->connections[7].pos.x = elem->corner.x + elem->width;
  node->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  /* fix boundingnode for line width and depth: */
  obj->bounding_box.top -= NODE_BORDERWIDTH/2.0 + NODE_DEPTH;
  obj->bounding_box.left -= NODE_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += NODE_BORDERWIDTH/2.0;
  obj->bounding_box.right += NODE_BORDERWIDTH/2.0 + NODE_DEPTH;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *node_create(Point *startpoint, void *user_data, Handle **handle1, Handle **handle2)
{
  Node *node;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  node = g_malloc(sizeof(Node));
  elem = &node->element;
  obj = (Object *) node;
  
  obj->type = &node_type;

  obj->ops = &node_ops;

  elem->corner = *startpoint;
  font = font_getfont("Helvetica");
  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  node->name = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);

  element_init(elem, 8, 8);

  for (i=0;i<8;i++)
    {
      obj->connections[i] = &node->connections[i];
      node->connections[i].object = obj;
      node->connections[i].connected = NULL;
    }
  node_update_data(node);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)node;
}

static void node_destroy(Node *node)
{
  text_destroy(node->name);
  element_destroy(&node->element);
}

static Object *node_copy(Node *node)
{
  int i;
  Node *newnode;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &node->element;
  
  newnode = g_malloc(sizeof(Node));
  newelem = &newnode->element;
  newobj = (Object *) newnode;

  element_copy(elem, newelem);
  newnode->name = text_copy(node->name);
  
  for (i=0;i<8;i++)
    {
      newobj->connections[i] = &newnode->connections[i];
      newnode->connections[i].object = newobj;
      newnode->connections[i].connected = NULL;
      newnode->connections[i].pos = node->connections[i].pos;
      newnode->connections[i].last_pos = node->connections[i].last_pos;
    }

  node_update_data(newnode);
  return (Object *)newnode;
}

static void node_save(Node *node, ObjectNode obj_node, const char *filename)
{
  element_save(&node->element, obj_node);
  data_add_text(new_attribute(obj_node, "name"), node->name);
}

static Object *node_load(ObjectNode obj_node, int version, const char *filename)
{
  Node *node;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  node = g_malloc(sizeof(Node));
  elem = &node->element;
  obj = (Object *) node;
  
  obj->type = &node_type;
  obj->ops = &node_ops;

  element_load(elem, obj_node);

  node->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL) node->name = data_text(attribute_first_data(attr));
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++)
    {
      obj->connections[i] = &node->connections[i];
      node->connections[i].object = obj;
      node->connections[i].connected = NULL;
    }
  node_update_data(node);
  return (Object *) node;
}
