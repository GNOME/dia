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

#include "stereotype.h"

#include "pixmaps/smallpackage.xpm"

typedef struct _SmallPackage SmallPackage;
struct _SmallPackage {
  Element element;

  ConnectionPoint connections[8];

  char *stereotype;
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
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   smallpackage_describe_props,
  (GetPropsFunc)        smallpackage_get_props,
  (SetPropsFunc)        smallpackage_set_props,
};

static PropDescription smallpackage_props[] = {
  ELEMENT_COMMON_PROPERTIES,
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
  /*  { "stereotype", PROP_TYPE_STRING, offsetof(SmallPackage, stereotype) },*/
  { NULL, 0, 0 },
};

static struct { const gchar *name; GQuark q; } quarks[] = {
  { "text_font" },
  { "text_height" },
  { "text_colour" },
  { "text" },
  { "stereotype" }
};

static void
smallpackage_get_props(SmallPackage * smallpackage, Property *props, guint nprops)
{
  guint i;

  if (object_get_props_from_offsets(&smallpackage->element.object, 
                                    smallpackage_offsets, props, nprops))
    return;
  /* these props can't be handled as easily */
  if (quarks[0].q == 0)
    for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
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
    } else if (pquark == quarks[4].q) {
      props[i].type = PROP_TYPE_STRING;
      g_free(PROP_VALUE_STRING(props[i]));
      if (smallpackage->stereotype != NULL)
	PROP_VALUE_STRING(props[i]) =
	  stereotype_to_string(smallpackage->stereotype);
      else
	PROP_VALUE_STRING(props[i]) = strdup("");	
    }
  }
}

static void
smallpackage_set_props(SmallPackage *smallpackage, Property *props, guint nprops)
{
  if (!object_set_props_from_offsets(&smallpackage->element.object, 
                                     smallpackage_offsets, props, nprops)) {
    guint i;

    if (quarks[0].q == 0)
      for (i = 0; i < sizeof(quarks)/sizeof(*quarks); i++)
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
      } else if (pquark == quarks[4].q && props[i].type == PROP_TYPE_STRING) {
	if (smallpackage->stereotype != NULL)
	  g_free(smallpackage->stereotype);
	if (PROP_VALUE_STRING(props[i]) != NULL &&
	    strlen(PROP_VALUE_STRING(props[i])) > 0)
	  smallpackage->stereotype =
	    string_to_stereotype(PROP_VALUE_STRING(props[i]));
	else 
	  smallpackage->stereotype = NULL;
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
  text_grab_focus(pkg->text, &pkg->element.object);
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

  if (pkg->stereotype != NULL) {
    p1 = pkg->text->position;
    p1.y -= pkg->text->height;
    renderer->ops->draw_string(renderer, pkg->stereotype, &p1, 
			       ALIGN_LEFT, &color_black);
  }
}

static void
smallpackage_update_data(SmallPackage *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = &elem->object;
  Point p;
  Font *font;

  elem->width = pkg->text->max_width + 2*SMALLPACKAGE_MARGIN_X;
  elem->width = MAX(elem->width, SMALLPACKAGE_TOPWIDTH+1.0);
  elem->height =
    pkg->text->height*pkg->text->numlines + 2*SMALLPACKAGE_MARGIN_Y;

  p = elem->corner;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y + pkg->text->ascent;

  if (pkg->stereotype != NULL) {
    font = pkg->text->font;
    elem->height += pkg->text->height;
    elem->width = MAX(elem->width, font_string_width(pkg->stereotype,
						     font, pkg->text->height)+
		      2*SMALLPACKAGE_MARGIN_X);
    p.y += pkg->text->height;
  }

  pkg->text->position = p;

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
  /* fix boundingbox for top rectangle: */
  obj->bounding_box.top -= SMALLPACKAGE_TOPHEIGHT;

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
  obj = &elem->object;
  
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
  elem->extra_spacing.border_trans = SMALLPACKAGE_BORDERWIDTH/2.0;
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  pkg->stereotype = NULL;

  *handle1 = NULL;
  *handle2 = NULL;
  return &pkg->element.object;
}

static void
smallpackage_destroy(SmallPackage *pkg)
{
  text_destroy(pkg->text);

  if (pkg->stereotype)
    g_free(pkg->stereotype);

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
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newpkg->text = text_copy(pkg->text);
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newpkg->connections[i];
    newpkg->connections[i].object = newobj;
    newpkg->connections[i].connected = NULL;
    newpkg->connections[i].pos = pkg->connections[i].pos;
    newpkg->connections[i].last_pos = pkg->connections[i].last_pos;
  }

  if (pkg->stereotype != NULL) 
    newpkg->stereotype = strdup(pkg->stereotype);
  else
    newpkg->stereotype = NULL;

  smallpackage_update_data(newpkg);
  
  return &newpkg->element.object;
}


static void
smallpackage_save(SmallPackage *pkg, ObjectNode obj_node,
		  const char *filename)
{
  element_save(&pkg->element, obj_node);

  if (pkg->stereotype != NULL)
    data_add_string(new_attribute(obj_node, "stereotype"),
		    pkg->stereotype);

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
  obj = &elem->object;
  
  obj->type = &smallpackage_type;
  obj->ops = &smallpackage_ops;

  element_load(elem, obj_node);
  
  pkg->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    pkg->stereotype = data_string(attribute_first_data(attr));
  else
    pkg->stereotype = NULL;

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
  elem->extra_spacing.border_trans = SMALLPACKAGE_BORDERWIDTH/2.0;
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &pkg->element.object;
}




