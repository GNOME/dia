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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

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
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void smallpackage_move(SmallPackage *pkg, Point *to);
static void smallpackage_draw(SmallPackage *pkg, Renderer *renderer);
static Object *smallpackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void smallpackage_destroy(SmallPackage *pkg);
static Object *smallpackage_copy(SmallPackage *pkg);

static void smallpackage_save(SmallPackage *pkg, ObjectNode obj_node,
			      const char *filename);
static Object *smallpackage_load(ObjectNode obj_node, int version,
				 const char *filename);

static PropDescription *smallpackage_describe_props(SmallPackage *smallpackage);
static void smallpackage_get_props(SmallPackage *smallpackage, Property *props, guint nprops);
static void smallpackage_set_props(SmallPackage *smallpackage, Property *props, guint nprops);

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
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   smallpackage_describe_props,
  (GetPropsFunc)        smallpackage_get_props,
  (SetPropsFunc)        smallpackage_set_props,
};

static PropDescription smallpackage_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Name"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_TEXT,
  
  PROP_DESC_END
};

static PropDescription *
smallpackage_describe_props(SmallPackage *smallpackage)
{
  if (smallpackage_props[0].quark == 0)
    prop_desc_list_calculate_quarks(smallpackage_props);
  return smallpackage_props;
}

static PropOffset smallpackage_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(SmallPackage, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(SmallPackage, stereotype) },
  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" }
};

static void
smallpackage_get_props(SmallPackage * smallpackage, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets((Object *)smallpackage, smallpackage_offsets, props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < 4; i++)
      quarks[i].q = g_quark_from_static_string(quarks[i].name);
  for (i = 0; i < nprops; i++) {
    GQuark pquark = g_quark_from_string(props[i].name);

    if (pquark == quarks[0].q) {
      props[i].type = PROP_TYPE_FONT;
      PROP_VALUE_FONT(props[i]) = smallpackage->text->font;
    } else if (pquark == quarks[1].q) {
      props[i].type = PROP_TYPE_REAL;
      PROP_VALUE_REAL(props[i]) = smallpackage->text->height;
    } else if (pquark == quarks[2].q) {
      props[i].type = PROP_TYPE_COLOUR;
      PROP_VALUE_COLOUR(props[i]) = smallpackage->text->color;
    } else if (pquark == quarks[3].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      PROP_VALUE_STRING(props[i]) = text_get_string_copy(smallpackage->text);
    }
  }
}

static void
smallpackage_set_props(SmallPackage *smallpackage, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets((Object *)smallpackage, smallpackage_offsets,
		     props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < 4; i++)
	quarks[i].q = g_quark_from_static_string(quarks[i].name);

    for (i = 0; i < nprops; i++) {
      GQuark pquark = g_quark_from_string(props[i].name);

      if (pquark == quarks[0].q && props[i].type == PROP_TYPE_FONT) {
	text_set_font(smallpackage->text, PROP_VALUE_FONT(props[i]));
      } else if (pquark == quarks[1].q && props[i].type == PROP_TYPE_REAL) {
	text_set_height(smallpackage->text, PROP_VALUE_REAL(props[i]));
      } else if (pquark == quarks[2].q && props[i].type == PROP_TYPE_COLOUR) {
	text_set_color(smallpackage->text, &PROP_VALUE_COLOUR(props[i]));
      } else if (pquark == quarks[3].q && props[i].type == PROP_TYPE_STRING) {
	text_set_string(smallpackage->text, PROP_VALUE_STRING(props[i]));
      }
    }
  }
  smallpackage_update_data(smallpackage);
}

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
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
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
smallpackage_save(SmallPackage *pkg, ObjectNode obj_node,
		  const char *filename)
{
  element_save(&pkg->element, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		pkg->text);
}

static Object *
smallpackage_load(ObjectNode obj_node, int version, const char *filename)
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




