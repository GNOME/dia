/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998,1999 Alexander Larsson
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
#include <stdio.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"

#include "pixmaps/classicon.xpm"

typedef struct _Classicon Classicon;

struct _Classicon {
  Element element;

  ConnectionPoint connections[8];
  
  int stereotype;
  int is_object;
  Text *text;
  TextAttributes attrs;
};

enum CLassIconStereotype {
    CLASSICON_CONTROL,
    CLASSICON_BOUNDARY,
    CLASSICON_ENTITY
};


#define CLASSICON_LINEWIDTH 0.1
#define CLASSICON_RADIOUS 1
#define CLASSICON_FONTHEIGHT 0.8
#define CLASSICON_MARGIN 0.5
#define CLASSICON_AIR 0.25
#define CLASSICON_ARROW 0.4

static real classicon_distance_from(Classicon *cicon, Point *point);
static void classicon_select(Classicon *cicon, Point *clicked_point,
			     Renderer *interactive_renderer);
static void classicon_move_handle(Classicon *cicon, Handle *handle,
				  Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void classicon_move(Classicon *cicon, Point *to);
static void classicon_draw(Classicon *cicon, Renderer *renderer);
static Object *classicon_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static void classicon_destroy(Classicon *cicon);
static Object *classicon_load(ObjectNode obj_node, int version,
			      const char *filename);
static PropDescription *classicon_describe_props(Classicon *classicon);
static void classicon_get_props(Classicon *classicon, GPtrArray *props);
static void classicon_set_props(Classicon *classicon, GPtrArray *props);
static void classicon_update_data(Classicon *cicon);


static ObjectTypeOps classicon_type_ops =
{
  (CreateFunc) classicon_create,
  (LoadFunc)   classicon_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType classicon_type =
{
  "UML - Classicon",   /* name */
  0,                      /* version */
  (char **) classicon_xpm,  /* pixmap */
  
  &classicon_type_ops       /* ops */
};

static ObjectOps classicon_ops = {
  (DestroyFunc)         classicon_destroy,
  (DrawFunc)            classicon_draw,
  (DistanceFunc)        classicon_distance_from,
  (SelectFunc)          classicon_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            classicon_move,
  (MoveHandleFunc)      classicon_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   classicon_describe_props,
  (GetPropsFunc)        classicon_get_props,
  (SetPropsFunc)        classicon_set_props
};

static PropEnumData prop_classicon_type_data[] = {
  { N_("Control"), CLASSICON_CONTROL },
  { N_("Boundary"), CLASSICON_BOUNDARY },
  { N_("Entity"), CLASSICON_ENTITY },
  { NULL, 0 }
};

static PropDescription classicon_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "stereotype", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL,  prop_classicon_type_data},
  { "is_object", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Is object"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL }, 
  
  PROP_DESC_END
};

static PropDescription *
classicon_describe_props(Classicon *classicon)
{
  return classicon_props;
}

static PropOffset classicon_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "stereotype", PROP_TYPE_ENUM, offsetof(Classicon, stereotype) },
  { "is_object", PROP_TYPE_BOOL, offsetof(Classicon, is_object) },
  { "text",PROP_TYPE_TEXT,offsetof(Classicon,text)},
  { "text_font",PROP_TYPE_FONT,offsetof(Classicon,attrs.font)},
  { "text_height",PROP_TYPE_REAL,offsetof(Classicon,attrs.height)},
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Classicon,attrs.color)},
  { NULL, 0, 0 },
};

static void
classicon_get_props(Classicon * classicon, GPtrArray *props)
{
  text_get_attributes(classicon->text,&classicon->attrs);
  object_get_props_from_offsets(&classicon->element.object,
                                classicon_offsets,props);
}

static void
classicon_set_props(Classicon *classicon, GPtrArray *props)
{
  object_set_props_from_offsets(&classicon->element.object,
                                classicon_offsets,props);
  apply_textattr_properties(props,classicon->text,"text",&classicon->attrs);
  classicon_update_data(classicon);
}

static real
classicon_distance_from(Classicon *cicon, Point *point)
{
  Object *obj = &cicon->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
classicon_select(Classicon *cicon, Point *clicked_point,
		 Renderer *interactive_renderer)
{
  text_set_cursor(cicon->text, clicked_point, interactive_renderer);
  text_grab_focus(cicon->text, &cicon->element.object);
  element_update_handles(&cicon->element);
}

static void
classicon_move_handle(Classicon *cicon, Handle *handle,
		      Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(cicon!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
}

static void
classicon_move(Classicon *cicon, Point *to)
{
  Element *elem = &cicon->element;
  
  elem->corner = *to;
  elem->corner.x -= elem->width/2.0; 
  elem->corner.y -= CLASSICON_RADIOUS + CLASSICON_ARROW;

  if (cicon->stereotype==CLASSICON_BOUNDARY)
    elem->corner.x -= CLASSICON_RADIOUS/2.0;

  classicon_update_data(cicon);
}

static void
classicon_draw(Classicon *icon, Renderer *renderer)
{
  Element *elem;
  real r, x, y, w, h;
  Point center, p1, p2;
  int i;
  
  assert(icon != NULL);
  assert(renderer != NULL);

  elem = &icon->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;
  
  r = CLASSICON_RADIOUS;
  center.x = x + elem->width/2;
  center.y = y + r + CLASSICON_ARROW;

  if (icon->stereotype==CLASSICON_BOUNDARY)
      center.x += r/2.0;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

  renderer->ops->fill_ellipse(renderer,
			      &center,
			      2*r, 2*r,
			      &color_white);

  renderer->ops->set_linewidth(renderer, CLASSICON_LINEWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  renderer->ops->draw_ellipse(renderer,
			      &center,
			      2*r, 2*r,
			      &color_black);


  switch (icon->stereotype) {
  case CLASSICON_CONTROL:
      p1.x = center.x - r*0.258819045102521;
      p1.y = center.y-r*0.965925826289068;

      p2.x = p1.x + CLASSICON_ARROW;
      p2.y = p1.y + CLASSICON_ARROW/1.5;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      p2.x = p1.x + CLASSICON_ARROW;
      p2.y = p1.y - CLASSICON_ARROW/1.5;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      break;

  case CLASSICON_BOUNDARY:
      p1.x = center.x - r;
      p2.x = p1.x - r;
      p1.y = p2.y = center.y;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      p1.x = p2.x;
      p1.y = center.y - r;
      p2.y = center.y + r;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      break;
  case CLASSICON_ENTITY:
      p1.x = center.x - r;
      p2.x = center.x + r;
      p1.y = p2.y = center.y + r;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      break;
  }
  
  text_draw(icon->text, renderer);

  if (icon->is_object) {
    renderer->ops->set_linewidth(renderer, 0.01);
    if (icon->stereotype==CLASSICON_BOUNDARY)
      x += r/2.0;
    p1.y = p2.y = icon->text->position.y + icon->text->descent;
    for (i=0; i<icon->text->numlines; i++) { 
      p1.x = x + (w - icon->text->row_width[i])/2;
      p2.x = p1.x + icon->text->row_width[i];
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      p1.y = p2.y += icon->text->height;
    }
  }
}

static void
classicon_update_data(Classicon *cicon)
{
  Element *elem = &cicon->element;
  Object *obj = &elem->object;
  DiaFont *font;
  Point p1;
  real h, wt, w = 0;
  int is_boundary = (cicon->stereotype==CLASSICON_BOUNDARY);
	
  font = cicon->text->font;
  h = CLASSICON_AIR + CLASSICON_MARGIN + CLASSICON_ARROW + 2*CLASSICON_RADIOUS;

  w = 2*CLASSICON_RADIOUS;
  wt = cicon->text->max_width;

  if (cicon->stereotype==CLASSICON_BOUNDARY) {
      w += 2*CLASSICON_RADIOUS;
      wt += CLASSICON_RADIOUS;
  }

  w = MAX(w, wt) + CLASSICON_AIR;

  p1.y = h + elem->corner.y; 
  h += cicon->text->height*cicon->text->numlines + CLASSICON_AIR;
  
  p1.y += cicon->text->ascent;
  p1.x = elem->corner.x + w/2.0;
  if (cicon->stereotype==CLASSICON_BOUNDARY)
      p1.x += CLASSICON_RADIOUS/2.0;
  text_set_position(cicon->text, &p1);
    
  elem->width = w;
  elem->height = h;
	
  p1.x = elem->corner.x + elem->width / 2.0;
  p1.y = elem->corner.y + CLASSICON_RADIOUS + CLASSICON_ARROW;
  w = CLASSICON_RADIOUS + CLASSICON_ARROW;
  h = (CLASSICON_RADIOUS + CLASSICON_ARROW) * M_SQRT1_2;

  if (is_boundary)
    p1.x += CLASSICON_RADIOUS/2.0;
	
  /* Update connections: */
  cicon->connections[0].pos.x = (is_boundary) ? p1.x-2*w: p1.x - h;
  cicon->connections[0].pos.y = (is_boundary) ? elem->corner.y: p1.y - h;
  cicon->connections[1].pos.x = p1.x;
  cicon->connections[1].pos.y = p1.y - w;
  cicon->connections[2].pos.x = p1.x + h;
  cicon->connections[2].pos.y = p1.y - h; 
	
  cicon->connections[3].pos.x = (is_boundary) ? p1.x-2*w: p1.x - w;
  cicon->connections[3].pos.y = p1.y;
  cicon->connections[4].pos.x = p1.x + w;
  cicon->connections[4].pos.y = p1.y;
  cicon->connections[5].pos.x = elem->corner.x;
  cicon->connections[5].pos.y = elem->corner.y + elem->height;
  cicon->connections[6].pos.x = p1.x;
  cicon->connections[6].pos.y = elem->corner.y + elem->height;
  cicon->connections[7].pos.x = elem->corner.x + elem->width;
  cicon->connections[7].pos.y = elem->corner.y + elem->height;
  
  element_update_boundingbox(elem);

  obj->position = elem->corner;
  obj->position.x += (elem->width + ((is_boundary)?CLASSICON_RADIOUS:0))/2.0;
  obj->position.y += CLASSICON_RADIOUS + CLASSICON_ARROW;

  element_update_handles(elem);
}

static Object *
classicon_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Classicon *cicon;
  Element *elem;
  Object *obj;
  Point p;
  DiaFont *font;
  int i;
  
  cicon = g_malloc0(sizeof(Classicon));
  elem = &cicon->element;
  obj = &elem->object;
  
  obj->type = &classicon_type;

  obj->ops = &classicon_ops;

  elem->corner = *startpoint;

  /* choose default font name for your locale. see also font_data structure
     in lib/font.c. */
  font = font_getfont (_("Helvetica"));
  
  cicon->stereotype = 0;
  cicon->is_object = 0;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  cicon->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  text_get_attributes(cicon->text,&cicon->attrs);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &cicon->connections[i];
    cicon->connections[i].object = obj;
    cicon->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = 0.0;
  classicon_update_data(cicon);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;

  return &cicon->element.object;
}

static void
classicon_destroy(Classicon *cicon)
{
  text_destroy(cicon->text);
  element_destroy(&cicon->element);
}

static Object *
classicon_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&classicon_type,
                                      obj_node,version,filename);
}

