/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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
#include "sheet.h"
#include "text.h"

#include "pixmaps/component.xpm"

typedef struct _Component Component;
struct _Component {
  Element element;

  ConnectionPoint connections[8];

  char *stereotype;
  char *name;
  Text *text;
};

#define COMPONENT_BORDERWIDTH 0.1
#define COMPONENT_CHEIGHT 0.7
#define COMPONENT_CWIDTH 2.0
#define COMPONENT_MARGIN_X 0.4
#define COMPONENT_MARGIN_Y 0.3

static real component_distance_from(Component *cmp, Point *point);
static void component_select(Component *cmp, Point *clicked_point,
				Renderer *interactive_renderer);
static void component_move_handle(Component *cmp, Handle *handle,
				     Point *to, HandleMoveReason reason);
static void component_move(Component *cmp, Point *to);
static void component_draw(Component *cmp, Renderer *renderer);
static Object *component_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void component_destroy(Component *cmp);
static Object *component_copy(Component *cmp);

static void component_save(Component *cmp, ObjectNode obj_node,
			      const char *filename);
static Object *component_load(ObjectNode obj_node, int version,
				 const char *filename);

static void component_update_data(Component *cmp);

static ObjectTypeOps component_type_ops =
{
  (CreateFunc) component_create,
  (LoadFunc)   component_load,
  (SaveFunc)   component_save
};

ObjectType component_type =
{
  "UML - Component",   /* name */
  0,                      /* version */
  (char **) component_xpm,  /* pixmap */
  
  &component_type_ops       /* ops */
};

SheetObject component_sheetobj =
{
  "UML - Component",             /* type */
  N_("Create a component"),           /* description */
  (char **) component_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps component_ops = {
  (DestroyFunc)         component_destroy,
  (DrawFunc)            component_draw,
  (DistanceFunc)        component_distance_from,
  (SelectFunc)          component_select,
  (CopyFunc)            component_copy,
  (MoveFunc)            component_move,
  (MoveHandleFunc)      component_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static real
component_distance_from(Component *cmp, Point *point)
{
  Object *obj = &cmp->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
component_select(Component *cmp, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(cmp->text, clicked_point, interactive_renderer);
  text_grab_focus(cmp->text, (Object *)cmp);
  element_update_handles(&cmp->element);
}

static void
component_move_handle(Component *cmp, Handle *handle,
			 Point *to, HandleMoveReason reason)
{
  assert(cmp!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
component_move(Component *cmp, Point *to)
{
  Point p;
  
  cmp->element.corner = *to;

  p = *to;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += 2*COMPONENT_CHEIGHT;
  text_set_position(cmp->text, &p);
  
  component_update_data(cmp);
}

static void
component_draw(Component *cmp, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  
  assert(cmp != NULL);
  assert(renderer != NULL);

  elem = &cmp->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, COMPONENT_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x + COMPONENT_CWIDTH/2; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x= x; p1.y = y +(h - 3*COMPONENT_CHEIGHT)/2.0;
  p2.x = x+COMPONENT_CWIDTH; p2.y = p1.y + COMPONENT_CHEIGHT;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);
  
  p1.y = p2.y + COMPONENT_CHEIGHT;
  p2.y = p1.y + COMPONENT_CHEIGHT;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);



  text_draw(cmp->text, renderer);
}

static void
component_update_data(Component *cmp)
{
  Element *elem = &cmp->element;
  Object *obj = (Object *) cmp;
  
  elem->width = cmp->text->max_width + 2*COMPONENT_MARGIN_X + COMPONENT_CWIDTH;
  elem->width = MAX(elem->width, 2*COMPONENT_CWIDTH);
  elem->height =  cmp->text->height*cmp->text->numlines +
    cmp->text->descent + 0.1 + 2*COMPONENT_MARGIN_Y ;
  elem->height = MAX(elem->height, 5*COMPONENT_CHEIGHT);

  /* Update connections: */
  cmp->connections[0].pos = elem->corner;
  cmp->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  cmp->connections[1].pos.y = elem->corner.y;
  cmp->connections[2].pos.x = elem->corner.x + elem->width;
  cmp->connections[2].pos.y = elem->corner.y;
  cmp->connections[3].pos.x = elem->corner.x;
  cmp->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  cmp->connections[4].pos.x = elem->corner.x + elem->width;
  cmp->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  cmp->connections[5].pos.x = elem->corner.x;
  cmp->connections[5].pos.y = elem->corner.y + elem->height;
  cmp->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  cmp->connections[6].pos.y = elem->corner.y + elem->height;
  cmp->connections[7].pos.x = elem->corner.x + elem->width;
  cmp->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  /* fix boundingcomponent for line width and top rectangle: */
  obj->bounding_box.top -= COMPONENT_BORDERWIDTH/2.0;
  obj->bounding_box.left -= COMPONENT_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += COMPONENT_BORDERWIDTH/2.0;
  obj->bounding_box.right += COMPONENT_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
component_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Component *cmp;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  cmp = g_malloc(sizeof(Component));
  elem = &cmp->element;
  obj = (Object *) cmp;
  
  obj->type = &component_type;

  obj->ops = &component_ops;

  elem->corner = *startpoint;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += 2*COMPONENT_CHEIGHT;
  
  cmp->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &cmp->connections[i];
    cmp->connections[i].object = obj;
    cmp->connections[i].connected = NULL;
  }
  component_update_data(cmp);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)cmp;
}

static void
component_destroy(Component *cmp)
{
  text_destroy(cmp->text);

  element_destroy(&cmp->element);
}

static Object *
component_copy(Component *cmp)
{
  int i;
  Component *newcmp;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &cmp->element;
  
  newcmp = g_malloc(sizeof(Component));
  newelem = &newcmp->element;
  newobj = (Object *) newcmp;

  element_copy(elem, newelem);

  newcmp->text = text_copy(cmp->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newcmp->connections[i];
    newcmp->connections[i].object = newobj;
    newcmp->connections[i].connected = NULL;
    newcmp->connections[i].pos = cmp->connections[i].pos;
    newcmp->connections[i].last_pos = cmp->connections[i].last_pos;
  }

  component_update_data(newcmp);
  
  return (Object *)newcmp;
}


static void
component_save(Component *cmp, ObjectNode obj_node,
		  const char *filename)
{
  element_save(&cmp->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		cmp->text);
}

static Object *
component_load(ObjectNode obj_node, int version, const char *filename)
{
  Component *cmp;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  cmp = g_malloc(sizeof(Component));
  elem = &cmp->element;
  obj = (Object *) cmp;
  
  obj->type = &component_type;
  obj->ops = &component_ops;

  element_load(elem, obj_node);
  
  cmp->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    cmp->text = data_text(attribute_first_data(attr));
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &cmp->connections[i];
    cmp->connections[i].object = obj;
    cmp->connections[i].connected = NULL;
  }
  component_update_data(cmp);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *) cmp;
}




