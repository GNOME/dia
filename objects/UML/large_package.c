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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"
#include "stereotype.h"

#include "pixmaps/largepackage.xpm"

typedef struct _LargePackage LargePackage;

struct _LargePackage {
  Element element;

  ConnectionPoint connections[8];

  char *name;
  char *stereotype; /* Can be NULL, including << and >> */
  char *st_stereotype; 

  DiaFont *font;
  
  real topwidth;
  real topheight;
};

#define LARGEPACKAGE_BORDERWIDTH 0.1
#define LARGEPACKAGE_FONTHEIGHT 0.8

static real largepackage_distance_from(LargePackage *pkg, Point *point);
static void largepackage_select(LargePackage *pkg, Point *clicked_point,
				Renderer *interactive_renderer);
static void largepackage_move_handle(LargePackage *pkg, Handle *handle,
				     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void largepackage_move(LargePackage *pkg, Point *to);
static void largepackage_draw(LargePackage *pkg, Renderer *renderer);
static Object *largepackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void largepackage_destroy(LargePackage *pkg);

static void largepackage_update_data(LargePackage *pkg);

static PropDescription *largepackage_describe_props(LargePackage *largepackage);
static void largepackage_get_props(LargePackage *largepackage, GPtrArray *props);
static void largepackage_set_props(LargePackage *largepackage, GPtrArray *props);
static Object *largepackage_load(ObjectNode obj_node, int version, 
                                 const char *filename);

static ObjectTypeOps largepackage_type_ops =
{
  (CreateFunc) largepackage_create,
  (LoadFunc)   largepackage_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType largepackage_type =
{
  "UML - LargePackage",   /* name */
  0,                      /* version */
  (char **) largepackage_xpm,  /* pixmap */
  
  &largepackage_type_ops       /* ops */
};

static ObjectOps largepackage_ops = {
  (DestroyFunc)         largepackage_destroy,
  (DrawFunc)            largepackage_draw,
  (DistanceFunc)        largepackage_distance_from,
  (SelectFunc)          largepackage_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            largepackage_move,
  (MoveHandleFunc)      largepackage_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   largepackage_describe_props,
  (GetPropsFunc)        largepackage_get_props,
  (SetPropsFunc)        largepackage_set_props
};

static PropDescription largepackage_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Name"), NULL, NULL },
  PROP_DESC_END
};

static PropDescription *
largepackage_describe_props(LargePackage *largepackage)
{
  return largepackage_props;
}

static PropOffset largepackage_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"stereotype", PROP_TYPE_STRING, offsetof(LargePackage , stereotype) },
  {"name", PROP_TYPE_STRING, offsetof(LargePackage , name) },

  { NULL, 0, 0 },
};

static void
largepackage_get_props(LargePackage * largepackage, GPtrArray *props)
{
  object_get_props_from_offsets(&largepackage->element.object, 
                                largepackage_offsets, props);
}

static void
largepackage_set_props(LargePackage *largepackage, GPtrArray *props)
{
  object_set_props_from_offsets(&largepackage->element.object, 
                                largepackage_offsets, props);
  g_free(largepackage->st_stereotype);
  largepackage->st_stereotype = NULL;
  largepackage_update_data(largepackage);
}

static real
largepackage_distance_from(LargePackage *pkg, Point *point)
{
  Object *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
largepackage_select(LargePackage *pkg, Point *clicked_point,
		    Renderer *interactive_renderer)
{
  element_update_handles(&pkg->element);
}

static void
largepackage_move_handle(LargePackage *pkg, Handle *handle,
			 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  
  element_move_handle(&pkg->element, handle->id, to, reason);
  largepackage_update_data(pkg);
}

static void
largepackage_move(LargePackage *pkg, Point *to)
{
  pkg->element.corner = *to;

  largepackage_update_data(pkg);
}

static void
largepackage_draw(LargePackage *pkg, Renderer *renderer)
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
  renderer->ops->set_linewidth(renderer, LARGEPACKAGE_BORDERWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);

  p1.x= x; p1.y = y - pkg->topheight;
  p2.x = x + pkg->topwidth; p2.y = y;

  renderer->ops->fill_rect(renderer, 
			   &p1, &p2,
			   &color_white);
  
  renderer->ops->draw_rect(renderer, 
			   &p1, &p2,
			   &color_black);


  renderer->ops->set_font(renderer, pkg->font, LARGEPACKAGE_FONTHEIGHT);

  p1.x = x + 0.1;
  p1.y = y - LARGEPACKAGE_FONTHEIGHT -
      dia_font_descent(pkg->st_stereotype,
                       pkg->font, LARGEPACKAGE_FONTHEIGHT) - 0.1;



  if (pkg->st_stereotype) {
    renderer->ops->draw_string(renderer, pkg->st_stereotype, &p1,
			       ALIGN_LEFT, &color_black);
  }
  p1.y += LARGEPACKAGE_FONTHEIGHT;

  if (pkg->name)
    renderer->ops->draw_string(renderer, pkg->name, &p1,
			       ALIGN_LEFT, &color_black);
}

static void
largepackage_update_data(LargePackage *pkg)
{
  Element *elem = &pkg->element;
  Object *obj = &elem->object;

  pkg->stereotype = remove_stereotype_from_string(pkg->stereotype);
  if (!pkg->st_stereotype) {
    pkg->st_stereotype =  string_to_stereotype(pkg->stereotype);
  }
  
  pkg->topwidth = 2.0;
  if (pkg->name != NULL)
    pkg->topwidth = MAX(pkg->topwidth,
                        dia_font_string_width(pkg->name, pkg->font,
                                          LARGEPACKAGE_FONTHEIGHT)+2*0.1);
  if (pkg->st_stereotype != NULL)
    pkg->topwidth = MAX(pkg->topwidth,
                        dia_font_string_width(pkg->st_stereotype, pkg->font,
                                              LARGEPACKAGE_FONTHEIGHT)+2*0.1);

  if (elem->width < (pkg->topwidth + 0.2))
    elem->width = pkg->topwidth + 0.2;
  if (elem->height < 1.0)
    elem->height = 1.0;
  
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
  obj->bounding_box.top -= pkg->topheight;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static Object *
largepackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  LargePackage *pkg;
  Element *elem;
  Object *obj;
  int i;
  
  pkg = g_malloc0(sizeof(LargePackage));
  elem = &pkg->element;
  obj = &elem->object;
  
  obj->type = &largepackage_type;

  obj->ops = &largepackage_ops;

  elem->corner = *startpoint;

  element_init(elem, 8, 8);

  elem->width = 4.0;
  elem->height = 4.0;
  
  pkg->font = dia_font_new_from_style(DIA_FONT_MONOSPACE,
                                      LARGEPACKAGE_FONTHEIGHT);
  
  pkg->stereotype = NULL;
  pkg->st_stereotype = NULL;
  pkg->name = strdup("");

  pkg->topwidth = 2.0;
  pkg->topheight = LARGEPACKAGE_FONTHEIGHT*2 + 0.1*2;

  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  pkg->element.extra_spacing.border_trans = LARGEPACKAGE_BORDERWIDTH/2.0;
  largepackage_update_data(pkg);

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return &pkg->element.object;
}

static void
largepackage_destroy(LargePackage *pkg)
{
  dia_font_unref(pkg->font);
  g_free(pkg->stereotype);
  g_free(pkg->st_stereotype);
  g_free(pkg->name);
  
  element_destroy(&pkg->element);
}

static Object *
largepackage_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&largepackage_type,
                                      obj_node,version,filename);
}




