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

#include "pixmaps/smallpackage.xpm"

typedef struct _SmallPackage SmallPackage;
struct _SmallPackage {
  Element element;

  ConnectionPoint connections[8];

  char *stereotype;
  char *name;
  Text *text;
};

#define SMALLPACKAGE_BORDERWIDTH 0.1
#define SMALLPACKAGE_TOPHEIGHT 0.9
#define SMALLPACKAGE_TOPWIDTH 1.5
#define SMALLPACKAGE_MARGIN_X 0.3
#define SMALLPACKAGE_MARGIN_Y 0.3

static real smallpackage_distance_from(SmallPackage *pkg, Point *point);
static void smallpackage_select(SmallPackage *pkg, Point *clicked_point,
				Renderer *interactive_renderer);
static void smallpackage_move_handle(SmallPackage *pkg, Handle *handle,
				     Point *to, HandleMoveReason reason);
static void smallpackage_move(SmallPackage *pkg, Point *to);
static void smallpackage_draw(SmallPackage *pkg, Renderer *renderer);
static Object *smallpackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void smallpackage_destroy(SmallPackage *pkg);
static Object *smallpackage_copy(SmallPackage *pkg);

static void smallpackage_save(SmallPackage *pkg, ObjectNode obj_node);
static Object *smallpackage_load(ObjectNode obj_node, int version);

static void smallpackage_update_data(SmallPackage *pkg);

static ObjectTypeOps smallpackage_type_ops =
{
  (CreateFunc) smallpackage_create,
  (LoadFunc)   smallpackage_load,
  (SaveFunc)   smallpackage_save
};

ObjectType smallpackage_type =
{
  "UML - SmallPackage",   /* name */
  0,                      /* version */
  (char **) smallpackage_xpm,  /* pixmap */
  
  &smallpackage_type_ops       /* ops */
};

SheetObject smallpackage_sheetobj =
{
  "UML - SmallPackage",             /* type */
  "Create a (small) package",           /* description */
  (char **) smallpackage_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps smallpackage_ops = {
  (DestroyFunc)         smallpackage_destroy,
  (DrawFunc)            smallpackage_draw,
  (DistanceFunc)        smallpackage_distance_from,
  (SelectFunc)          smallpackage_select,
  (CopyFunc)            smallpackage_copy,
  (MoveFunc)            smallpackage_move,
  (MoveHandleFunc)      smallpackage_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

static real
smallpackage_distance_from(SmallPackage *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
smallpackage_select(SmallPackage *pkg, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, (Object *)pkg);
  element_update_handles(&pkg->element);
}

static void
smallpackage_move_handle(SmallPackage *pkg, Handle *handle,
			 Point *to, HandleMoveReason reason)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
smallpackage_move(SmallPackage *pkg, Point *to)
{
  Point p;
  
  pkg->element.corner = *to;

  p = *to;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += pkg->text->ascent + SMALLPACKAGE_MARGIN_Y;
  text_set_position(pkg->text, &p);
  
  smallpackage_update_data(pkg);
}

static void
smallpackage_draw(SmallPackage *pkg, Renderer *renderer)
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
  renderer->ops->set_linewidth(renderer, SMALLPACKAGE_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x= x; p1.y = y-SMALLPACKAGE_TOPHEIGHT;
  p2.x = x+SMALLPACKAGE_TOPWIDTH; p2.y = y;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);


  text_draw(pkg->text, renderer);
}

static void
smallpackage_update_data(SmallPackage *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = (Object *) pkg;
  
  elem->width = pkg->text->max_width + 2*SMALLPACKAGE_MARGIN_X;
  elem->width = MAX(elem->width, SMALLPACKAGE_TOPWIDTH+1.0);
  elem->height =
    pkg->text->height*pkg->text->numlines + 2*SMALLPACKAGE_MARGIN_Y;

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
  /* fix boundingsmallpackage for line width and top rectangle: */
  obj->bounding_box.top -= SMALLPACKAGE_BORDERWIDTH/2.0 + SMALLPACKAGE_TOPHEIGHT;
  obj->bounding_box.left -= SMALLPACKAGE_BORDERWIDTH/2.0;
  obj->bounding_box.bottom += SMALLPACKAGE_BORDERWIDTH/2.0;
  obj->bounding_box.right += SMALLPACKAGE_BORDERWIDTH/2.0;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
smallpackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  SmallPackage *pkg;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  pkg = g_malloc(sizeof(SmallPackage));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &smallpackage_type;

  obj->ops = &smallpackage_ops;

  elem->corner = *startpoint;

  font = font_getfont("Courier");
  p = *startpoint;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y + font_ascent(font, 0.8);
  
  pkg->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)pkg;
}

static void
smallpackage_destroy(SmallPackage *pkg)
{
  text_destroy(pkg->text);

  element_destroy(&pkg->element);
}

static Object *
smallpackage_copy(SmallPackage *pkg)
{
  int i;
  SmallPackage *newpkg;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &pkg->element;
  
  newpkg = g_malloc(sizeof(SmallPackage));
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

  smallpackage_update_data(newpkg);
  
  return (Object *)newpkg;
}


static void
smallpackage_save(SmallPackage *pkg, ObjectNode obj_node)
{
  element_save(&pkg->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text);
}

static Object *
smallpackage_load(ObjectNode obj_node, int version)
{
  SmallPackage *pkg;
  AttributeNode attr;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc(sizeof(SmallPackage));
  elem = &pkg->element;
  obj = (Object *) pkg;
  
  obj->type = &smallpackage_type;
  obj->ops = &smallpackage_ops;

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
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *) pkg;
}




