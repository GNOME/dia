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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "text.h"

#include "pixmaps/object.xpm"

typedef struct _Objet Objet;
struct _Objet {
  Element element;

  ConnectionPoint connections[8];

  char *stereotype;
  char *name;
  Text *text;
};

#define OBJET_BORDERWIDTH 0.1
#define OBJET_LINEWIDTH 0.05
#define OBJET_MARGIN_X 0.5
#define OBJET_MARGIN_Y 0.5

static real objet_distance_from(Objet *pkg, Point *point);
static void objet_select(Objet *pkg, Point *clicked_point,
				Renderer *interactive_renderer);
static void objet_move_handle(Objet *pkg, Handle *handle,
				     Point *to, HandleMoveReason reason);
static void objet_move(Objet *pkg, Point *to);
static void objet_draw(Objet *pkg, Renderer *renderer);
static Object *objet_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void objet_destroy(Objet *pkg);
static Object *objet_copy(Objet *pkg);

static void objet_save(Objet *pkg, ObjectNode obj_node);
static Object *objet_load(ObjectNode obj_node, int version);

static void objet_update_data(Objet *pkg);

static ObjectTypeOps objet_type_ops =
{
  (CreateFunc) objet_create,
  (LoadFunc)   objet_load,
  (SaveFunc)   objet_save
};

ObjectType objet_type =
{
  "UML - Objet",   /* name */
  0,                      /* version */
  (char **) object_xpm,  /* pixmap */
  
  &objet_type_ops       /* ops */
};

SheetObject objet_sheetobj =
{
  "UML - Objet",             /* type */
  "Create an object",           /* description */
  (char **) object_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps objet_ops = {
  (DestroyFunc)         objet_destroy,
  (DrawFunc)            objet_draw,
  (DistanceFunc)        objet_distance_from,
  (SelectFunc)          objet_select,
  (CopyFunc)            objet_copy,
  (MoveFunc)            objet_move,
  (MoveHandleFunc)      objet_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

static real
objet_distance_from(Objet *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
objet_select(Objet *pkg, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, (Object *)pkg);
  element_update_handles(&pkg->element);
}

static void
objet_move_handle(Objet *pkg, Handle *handle,
			 Point *to, HandleMoveReason reason)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
objet_move(Objet *pkg, Point *to)
{
  Point p;
  
  pkg->element.corner = *to;

  p = *to;
  p.x += OBJET_MARGIN_X;
  p.y += pkg->text->ascent + OBJET_MARGIN_Y;
  text_set_position(pkg->text, &p);
  
  objet_update_data(pkg);
}

static void
objet_draw(Objet *pkg, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point p1, p2;
  
  assert(pkg != NULL);
  assert(renderer != NULL);

  elem = &pkg->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, OBJET_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  text_draw(pkg->text, renderer);

  /* Is there a better way to underline? */
  p1 = pkg->text->position;
  p1.y += pkg->text->descent;
  p2.x = p1.x + pkg->text->max_width;
  p2.y = p1.y;
  
  renderer->ops->set_linewidth(renderer, OBJET_LINEWIDTH);
  renderer->ops->draw_line(renderer,
			   &p1, &p2,
			   &color_black);
}

static void
objet_update_data(Objet *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = (Object *) pkg;
  
  elem->width = pkg->text->max_width + 2*OBJET_MARGIN_X;
  elem->height =
    pkg->text->height*pkg->text->numlines + 2*OBJET_MARGIN_Y;

  /* Update connections: */
  pkg->connections[0].pos = elem->corner;
  pkg->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[1].pos.y = elem->corner.y;
  pkg->connections[2].pos.x = elem->corner.x + elem->width;
  pkg->connections[2].pos.y = elem->corner.y;
  pkg->connections[3].pos.x = elem->corner.x;
  pkg->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[4].pos.x = elem->corner.x + elem->width;
  pkg->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  pkg->connections[5].pos.x = elem->corner.x;
  pkg->connections[5].pos.y = elem->corner.y + elem->height;
  pkg->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  pkg->connections[6].pos.y = elem->corner.y + elem->height;
  pkg->connections[7].pos.x = elem->corner.x + elem->width;
  pkg->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);
  /* fix boundingobjet for line width and top rectangle: */
  obj->bounding_box.top -= OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.left -= OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += OBJET_BORDERWIDTH/2.0;
  obj->bounding_box.right += OBJET_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
objet_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Objet *pkg;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  pkg = g_malloc(sizeof(Objet));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &objet_type;

  obj->ops = &objet_ops;

  elem->corner = *startpoint;

  font = font_getfont("Courier");
  p = *startpoint;
  p.x += OBJET_MARGIN_X;
  p.y += OBJET_MARGIN_Y + font_ascent(font, 0.8);
  
  pkg->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  objet_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)pkg;
}

static void
objet_destroy(Objet *pkg)
{
  text_destroy(pkg->text);

  element_destroy(&pkg->element);
}

static Object *
objet_copy(Objet *pkg)
{
  int i;
  Objet *newpkg;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pkg->element;
  
  newpkg = g_malloc(sizeof(Objet));
  newelem = &newpkg->element;
  newobj = (Object *) newpkg;

  element_copy(elem, newelem);

  newpkg->text = text_copy(pkg->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].last_pos = pkg->connections[i].last_pos;
  }

  objet_update_data(newpkg);
  
  return (Object *)newpkg;
}


static void
objet_save(Objet *pkg, ObjectNode obj_node)
{
  element_save(&pkg->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text);
}

static Object *
objet_load(ObjectNode obj_node, int version)
{
  Objet *pkg;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc(sizeof(Objet));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &objet_type;
  obj->ops = &objet_ops;

  element_load(elem, obj_node);
  
  pkg->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    pkg->text = data_text(attribute_first_data(attr));
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  objet_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *) pkg;
}




