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
#include "diarenderer.h"
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
  
  char *st_stereotype;
  TextAttributes attrs;

  Color line_color;
  Color fill_color;
};

#define SMALLPACKAGE_BORDERWIDTH 0.1
#define SMALLPACKAGE_TOPHEIGHT 0.9
#define SMALLPACKAGE_TOPWIDTH 1.5
#define SMALLPACKAGE_MARGIN_X 0.3
#define SMALLPACKAGE_MARGIN_Y 0.3

static real smallpackage_distance_from(SmallPackage *pkg, Point *point);
static void smallpackage_select(SmallPackage *pkg, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static ObjectChange* smallpackage_move_handle(SmallPackage *pkg, Handle *handle,
					      Point *to, ConnectionPoint *cp,
					      HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* smallpackage_move(SmallPackage *pkg, Point *to);
static void smallpackage_draw(SmallPackage *pkg, DiaRenderer *renderer);
static DiaObject *smallpackage_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void smallpackage_destroy(SmallPackage *pkg);
static DiaObject *smallpackage_load(ObjectNode obj_node, int version,
				 const char *filename);

static PropDescription *smallpackage_describe_props(SmallPackage *smallpackage);
static void smallpackage_get_props(SmallPackage *smallpackage, GPtrArray *props);
static void smallpackage_set_props(SmallPackage *smallpackage, GPtrArray *props);

static void smallpackage_update_data(SmallPackage *pkg);

static ObjectTypeOps smallpackage_type_ops =
{
  (CreateFunc) smallpackage_create,
  (LoadFunc)   smallpackage_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

DiaObjectType smallpackage_type =
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
  (CopyFunc)            object_copy_using_properties,
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
  PROP_STD_LINE_COLOUR_OPTIONAL, 
  PROP_STD_FILL_COLOUR_OPTIONAL, 
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
  N_("Stereotype"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },     
  PROP_DESC_END
};

static PropDescription *
smallpackage_describe_props(SmallPackage *smallpackage)
{
  if (smallpackage_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(smallpackage_props);
  }
  return smallpackage_props;
}

static PropOffset smallpackage_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"line_colour", PROP_TYPE_COLOUR, offsetof(SmallPackage, line_color) },
  {"fill_colour", PROP_TYPE_COLOUR, offsetof(SmallPackage, fill_color) },
  {"stereotype", PROP_TYPE_STRING, offsetof(SmallPackage , stereotype) },
  {"text",PROP_TYPE_TEXT,offsetof(SmallPackage,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(SmallPackage,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(SmallPackage,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(SmallPackage,attrs.color)},  
  { NULL, 0, 0 },
};

static void
smallpackage_get_props(SmallPackage * smallpackage, 
                       GPtrArray *props)
{
  text_get_attributes(smallpackage->text,&smallpackage->attrs);
  object_get_props_from_offsets(&smallpackage->element.object,
                                smallpackage_offsets,props);
}

static void
smallpackage_set_props(SmallPackage *smallpackage, 
                       GPtrArray *props)
{
  object_set_props_from_offsets(&smallpackage->element.object, 
                                smallpackage_offsets, props);
  apply_textattr_properties(props,smallpackage->text,
                            "text",&smallpackage->attrs);
  g_free(smallpackage->st_stereotype);
  smallpackage->st_stereotype = NULL;
  smallpackage_update_data(smallpackage);
}

static real
smallpackage_distance_from(SmallPackage *pkg, Point *point)
{
  DiaObject *obj = &pkg->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
smallpackage_select(SmallPackage *pkg, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(pkg->text, clicked_point, interactive_renderer);
  text_grab_focus(pkg->text, &pkg->element.object);
  element_update_handles(&pkg->element);
}

static ObjectChange*
smallpackage_move_handle(SmallPackage *pkg, Handle *handle,
			 Point *to, ConnectionPoint *cp,
			 HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(pkg!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);

  return NULL;
}

static ObjectChange*
smallpackage_move(SmallPackage *pkg, Point *to)
{
  Point p;
  
  pkg->element.corner = *to;

  p = *to;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += pkg->text->ascent + SMALLPACKAGE_MARGIN_Y;
  text_set_position(pkg->text, &p);
  
  smallpackage_update_data(pkg);

  return NULL;
}

static void
smallpackage_draw(SmallPackage *pkg, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
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
  
  renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer_ops->set_linewidth(renderer, SMALLPACKAGE_BORDERWIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);


  p1.x = x; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  renderer_ops->fill_rect(renderer, 
			   &p1, &p2,
			   &pkg->fill_color);
  renderer_ops->draw_rect(renderer, 
			   &p1, &p2,
			   &pkg->line_color);

  p1.x= x; p1.y = y-SMALLPACKAGE_TOPHEIGHT;
  p2.x = x+SMALLPACKAGE_TOPWIDTH; p2.y = y;

  renderer_ops->fill_rect(renderer, 
			   &p1, &p2,
			   &pkg->fill_color);
  
  renderer_ops->draw_rect(renderer, 
			   &p1, &p2,
			   &pkg->line_color);

  text_draw(pkg->text, renderer);

  if ((pkg->st_stereotype != NULL) && (pkg->st_stereotype[0] != '\0')) {
    p1 = pkg->text->position;
    p1.y -= pkg->text->height;
    renderer_ops->draw_string(renderer, pkg->st_stereotype, &p1, 
			       ALIGN_LEFT, &pkg->attrs.color);
  }
}

static void
smallpackage_update_data(SmallPackage *pkg)
{
  Element *elem = &pkg->element;
  DiaObject *obj = &elem->object;
  Point p;
  DiaFont *font;

  pkg->stereotype = remove_stereotype_from_string(pkg->stereotype);
  if (!pkg->st_stereotype) {
    pkg->st_stereotype =  string_to_stereotype(pkg->stereotype);
  }

  text_calc_boundingbox(pkg->text, NULL);
  elem->width = pkg->text->max_width + 2*SMALLPACKAGE_MARGIN_X;
  elem->width = MAX(elem->width, SMALLPACKAGE_TOPWIDTH+1.0);
  elem->height =
    pkg->text->height*pkg->text->numlines + 2*SMALLPACKAGE_MARGIN_Y;

  p = elem->corner;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y + pkg->text->ascent;

  if ((pkg->stereotype != NULL) && (pkg->stereotype[0] != '\0')) {
    font = pkg->text->font;
    elem->height += pkg->text->height;
    elem->width = MAX(elem->width,
                      dia_font_string_width(pkg->st_stereotype,
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
  
  pkg->connections[0].directions = DIR_NORTH|DIR_WEST;
  pkg->connections[1].directions = DIR_NORTH;
  pkg->connections[2].directions = DIR_NORTH|DIR_EAST;
  pkg->connections[3].directions = DIR_WEST;
  pkg->connections[4].directions = DIR_EAST;
  pkg->connections[5].directions = DIR_SOUTH|DIR_WEST;
  pkg->connections[6].directions = DIR_SOUTH;
  pkg->connections[7].directions = DIR_SOUTH|DIR_EAST;
                                                                                          
  element_update_boundingbox(elem);
  /* fix boundingbox for top rectangle: */
  obj->bounding_box.top -= SMALLPACKAGE_TOPHEIGHT;

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
smallpackage_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  SmallPackage *pkg;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;
  
  pkg = g_malloc0(sizeof(SmallPackage));
  elem = &pkg->element;
  obj = &elem->object;
  
  obj->type = &smallpackage_type;

  obj->ops = &smallpackage_ops;

  elem->corner = *startpoint;

  font = dia_font_new_from_style(DIA_FONT_MONOSPACE, 0.8);
  p = *startpoint;
  p.x += SMALLPACKAGE_MARGIN_X;
  p.y += SMALLPACKAGE_MARGIN_Y+ dia_font_ascent("A",font, 0.8);
  
  pkg->text = new_text("", font, 0.8, &p, &color_black, ALIGN_LEFT);
  dia_font_unref(font);
  text_get_attributes(pkg->text,&pkg->attrs);
  
  element_init(elem, 8, 8);
  
  for (i=0;i<8;i++) {
    obj->connections[i] = &pkg->connections[i];
    pkg->connections[i].object = obj;
    pkg->connections[i].connected = NULL;
  }
  elem->extra_spacing.border_trans = SMALLPACKAGE_BORDERWIDTH/2.0;

  pkg->line_color = attributes_get_foreground();
  pkg->fill_color = attributes_get_background();

  pkg->stereotype = NULL;
  pkg->st_stereotype = NULL;
  smallpackage_update_data(pkg);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &pkg->element.object;
}

static void
smallpackage_destroy(SmallPackage *pkg)
{
  text_destroy(pkg->text);
  
  g_free(pkg->stereotype);
  g_free(pkg->st_stereotype);

  element_destroy(&pkg->element);
}

static DiaObject *
smallpackage_load(ObjectNode obj_node, int version, const char *filename)
{
  return object_load_using_properties(&smallpackage_type,
                                      obj_node,version,filename);
}





