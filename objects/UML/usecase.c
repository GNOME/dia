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
#include "files.h"
#include "sheet.h"
#include "text.h"

#include "pixmaps/case.xpm"

typedef struct _Usecase Usecase;
struct _Usecase {
  Element element;

  ConnectionPoint connections[8];

  Text *text;
};

#define USECASE_WIDTH 3
#define USECASE_HEIGHT 1.2
#define USECASE_RATIO 1.7
#define USECASE_LINEWIDTH 0.1

static real usecase_distance_from(Usecase *usecase, Point *point);
static void usecase_select(Usecase *usecase, Point *clicked_point,
			Renderer *interactive_renderer);
static void usecase_move_handle(Usecase *usecase, Handle *handle,
			     Point *to, HandleMoveReason reason);
static void usecase_move(Usecase *usecase, Point *to);
static void usecase_draw(Usecase *usecase, Renderer *renderer);
static Object *usecase_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static void usecase_destroy(Usecase *usecase);
static Object *usecase_copy(Usecase *usecase);

static void usecase_save(Usecase *usecase, int fd);
static Object *usecase_load(int fd, int version);

static void usecase_update_data(Usecase *usecase);

static ObjectTypeOps usecase_type_ops =
{
  (CreateFunc) usecase_create,
  (LoadFunc)   usecase_load,
  (SaveFunc)   usecase_save
};

ObjectType usecase_type =
{
  "UML - Usecase",   /* name */
  0,                      /* version */
  (char **) case_xpm,  /* pixmap */
  
  &usecase_type_ops       /* ops */
};

SheetObject usecase_sheetobj =
{
  "UML - Usecase",             /* type */
  "Create a use case",           /* description */
  (char **) case_xpm,     /* pixmap */

  NULL                       /* user_data */
};

static ObjectOps usecase_ops = {
  (DestroyFunc)         usecase_destroy,
  (DrawFunc)            usecase_draw,
  (DistanceFunc)        usecase_distance_from,
  (SelectFunc)          usecase_select,
  (CopyFunc)            usecase_copy,
  (MoveFunc)            usecase_move,
  (MoveHandleFunc)      usecase_move_handle,
  (GetPropertiesFunc)   object_return_null,
  (ApplyPropertiesFunc) object_return_void,
  (IsEmptyFunc)         object_return_false
};

static real
usecase_distance_from(Usecase *usecase, Point *point)
{
  Object *obj = &usecase->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
usecase_select(Usecase *usecase, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  text_set_cursor(usecase->text, clicked_point, interactive_renderer);
  text_grab_focus(usecase->text, (Object *)usecase);
  element_update_handles(&usecase->element);
}

static void
usecase_move_handle(Usecase *usecase, Handle *handle,
		 Point *to, HandleMoveReason reason)
{
  assert(usecase!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
usecase_move(Usecase *usecase, Point *to)
{
  real h;
  Point p;
  
  usecase->element.corner = *to;
  if (usecase->text->numlines>1)  
      h = usecase->text->height*usecase->text->numlines/2.0;
  else
      h = -usecase->text->ascent;
  p = *to;
  p.x += usecase->element.width/2.0;
  p.y +=  (usecase->element.height - h)/2.0 ;
  text_set_position(usecase->text, &p);
  usecase_update_data(usecase);
}

static void
usecase_draw(Usecase *usecase, Renderer *renderer)
{
  Element *elem;
  real x, y, w, h;
  Point c;

  assert(usecase != NULL);
  assert(renderer != NULL);

  elem = &usecase->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  c.x = x + w/2.0;
  c.y = y + h/2.0;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->set_linewidth(renderer, USECASE_LINEWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  renderer->ops->fill_ellipse(renderer, 
			     &c,
			     w, h,
			     &color_white);
  renderer->ops->draw_ellipse(renderer, 
			     &c,
			     w, h,
			     &color_black);  
  
  text_draw(usecase->text, renderer);
}


static void
usecase_update_data(Usecase *usecase)
{
  real w, h, rx, ry;

  Element *elem = &usecase->element;
  Object *obj = (Object *) usecase;
  
  w = usecase->text->max_width;
  h = usecase->text->height*usecase->text->numlines;
  
  rx = USECASE_RATIO*h + w;
  ry = rx / USECASE_RATIO;

  elem->width = (rx > USECASE_WIDTH) ? rx: USECASE_WIDTH;
  elem->height = (ry > USECASE_HEIGHT) ? ry: USECASE_HEIGHT;

  /* Update connections: */
  usecase->connections[0].pos = elem->corner;
  usecase->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  usecase->connections[1].pos.y = elem->corner.y;
  usecase->connections[2].pos.x = elem->corner.x + elem->width;
  usecase->connections[2].pos.y = elem->corner.y;
  usecase->connections[3].pos.x = elem->corner.x;
  usecase->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  usecase->connections[4].pos.x = elem->corner.x + elem->width;
  usecase->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  usecase->connections[5].pos.x = elem->corner.x;
  usecase->connections[5].pos.y = elem->corner.y + elem->height;
  usecase->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  usecase->connections[6].pos.y = elem->corner.y + elem->height;
  usecase->connections[7].pos.x = elem->corner.x + elem->width;
  usecase->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
usecase_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Usecase *usecase;
  Element *elem;
  Object *obj;
  Point p;
  Font *font;
  int i;
  
  usecase = g_malloc(sizeof(Usecase));
  elem = &usecase->element;
  obj = (Object *) usecase;
  
  obj->type = &usecase_type;
  obj->ops = &usecase_ops;
  elem->corner = *startpoint;
  elem->width = USECASE_WIDTH;
  elem->height = USECASE_HEIGHT;

  font = font_getfont("Helvetica");
  p = *startpoint;
  p.x += USECASE_WIDTH/2.0;
  p.y += USECASE_HEIGHT/2.0;
  
  usecase->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &usecase->connections[i];
    usecase->connections[i].object = obj;
    usecase->connections[i].connected = NULL;
  }
  usecase_update_data(usecase);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return (Object *)usecase;
}

static void
usecase_destroy(Usecase *usecase)
{
  text_destroy(usecase->text);

  element_destroy(&usecase->element);
}

static Object *
usecase_copy(Usecase *usecase)
{
  int i;
  Usecase *newusecase;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &usecase->element;
  
  newusecase = g_malloc(sizeof(Usecase));
  newelem = &newusecase->element;
  newobj = (Object *) newusecase;

  element_copy(elem, newelem);

  newusecase->text = text_copy(usecase->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newusecase->connections[i];
    newusecase->connections[i].object = newobj;
    newusecase->connections[i].connected = NULL;
    newusecase->connections[i].pos = usecase->connections[i].pos;
    newusecase->connections[i].last_pos = usecase->connections[i].last_pos;
  }

  usecase_update_data(newusecase);
  
  return (Object *)newusecase;
}


static void
usecase_save(Usecase *usecase, int fd)
{
  element_save(&usecase->element, fd);

  write_text(fd, usecase->text);
}

static Object *
usecase_load(int fd, int version)
{
  Usecase *usecase;
  Element *elem;
  Object *obj;
  int i;
  
  usecase = g_malloc(sizeof(Usecase));
  elem = &usecase->element;
  obj = (Object *) usecase;
  
  obj->type = &usecase_type;
  obj->ops = &usecase_ops;

  element_load(elem, fd);
  
  usecase->text = read_text(fd);
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &usecase->connections[i];
    usecase->connections[i].object = obj;
    usecase->connections[i].connected = NULL;
  }
  usecase_update_data(usecase);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)usecase;
}




