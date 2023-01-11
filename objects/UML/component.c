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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "stereotype.h"

#include "pixmaps/component.xpm"

#define NUM_CONNECTIONS 11

typedef struct _Component Component;
struct _Component {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  char *stereotype;
  Text *text;

  char *st_stereotype;

  Color line_color;
  Color fill_color;
};

#define COMPONENT_BORDERWIDTH 0.1
#define COMPONENT_CHEIGHT 0.7
#define COMPONENT_CWIDTH 2.0
#define COMPONENT_MARGIN_X 0.4
#define COMPONENT_MARGIN_Y 0.3

static real component_distance_from(Component *cmp, Point *point);
static void component_select(Component *cmp, Point *clicked_point,
				DiaRenderer *interactive_renderer);
static DiaObjectChange* component_move_handle(Component *cmp, Handle *handle,
					   Point *to, ConnectionPoint *cp,
					   HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* component_move(Component *cmp, Point *to);
static void component_draw(Component *cmp, DiaRenderer *renderer);
static DiaObject *component_create(Point *startpoint,
				   void *user_data,
				   Handle **handle1,
				   Handle **handle2);
static void component_destroy(Component *cmp);
static DiaObject *component_load(ObjectNode obj_node, int version,DiaContext *ctx);

static PropDescription *component_describe_props(Component *component);
static void component_get_props(Component *component, GPtrArray *props);
static void component_set_props(Component *component, GPtrArray *props);

static void component_update_data(Component *cmp);

static ObjectTypeOps component_type_ops =
{
  (CreateFunc) component_create,
  (LoadFunc)   component_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType component_type =
{
  "UML - Component", /* name */
  0,              /* version */
  component_xpm,   /* pixmap */
  &component_type_ops, /* ops */

  NULL, /* pixmap_file: fallback if pixmap is NULL */
  NULL, /* default_user_data: use this if no user data is specified in the .sheet file */
  NULL, /* prop_descs: property descriptions */
  NULL, /* prop_offsets: DiaObject struct offsets */
  DIA_OBJECT_CAN_PARENT /* flags */
};

static ObjectOps component_ops = {
  (DestroyFunc)         component_destroy,
  (DrawFunc)            component_draw,
  (DistanceFunc)        component_distance_from,
  (SelectFunc)          component_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            component_move,
  (MoveHandleFunc)      component_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   component_describe_props,
  (GetPropsFunc)        component_get_props,
  (SetPropsFunc)        component_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription component_props[] = {
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
component_describe_props(Component *component)
{
  if (component_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(component_props);
  }
  return component_props;
}

static PropOffset component_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Component,line_color)},
  {"fill_colour",PROP_TYPE_COLOUR,offsetof(Component,fill_color)},
  {"stereotype", PROP_TYPE_STRING, offsetof(Component , stereotype) },
  {"text",PROP_TYPE_TEXT,offsetof(Component,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Component,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Component,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Component,text),offsetof(Text,color)},
  { NULL, 0, 0 },
};


static void
component_get_props(Component * component, GPtrArray *props)
{
  object_get_props_from_offsets(&component->element.object,
                                component_offsets,props);
}

static void
component_set_props(Component *component, GPtrArray *props)
{
  object_set_props_from_offsets(&component->element.object,
                                component_offsets, props);
  g_clear_pointer (&component->st_stereotype, g_free);
  component_update_data(component);
}

static real
component_distance_from(Component *cmp, Point *point)
{
  Element *elem = &cmp->element;
  real x = elem->corner.x;
  real y = elem->corner.y;
  real cw2 = COMPONENT_CWIDTH/2.0;
  real h2 = elem->height / 2.0;
  DiaRectangle r1 = { x + cw2, y, x + elem->width - cw2, y + elem->height };
  DiaRectangle r2 = { x, y + h2 - COMPONENT_CHEIGHT * 1.5, x + cw2, y + h2 - COMPONENT_CHEIGHT * 0.5 };
  DiaRectangle r3 = { x, y + h2 + COMPONENT_CHEIGHT * 0.5, x + cw2, y + h2 + COMPONENT_CHEIGHT * 1.5 };
  real d1 = distance_rectangle_point(&r1, point);
  real d2 = distance_rectangle_point(&r2, point);
  real d3 = distance_rectangle_point(&r3, point);
  return MIN(d1, MIN(d2, d3));
}

static void
component_select(Component *cmp, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(cmp->text, clicked_point, interactive_renderer);
  text_grab_focus(cmp->text, &cmp->element.object);
  element_update_handles(&cmp->element);
}


static DiaObjectChange *
component_move_handle (Component        *cmp,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  g_return_val_if_fail (cmp != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}

static DiaObjectChange*
component_move(Component *cmp, Point *to)
{
  cmp->element.corner = *to;

  component_update_data(cmp);

  return NULL;
}


static void
component_draw (Component *cmp, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point p1, p2;

  g_return_if_fail (cmp != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &cmp->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;
  h = elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, COMPONENT_BORDERWIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  p1.x = x + COMPONENT_CWIDTH/2; p1.y = y;
  p2.x = x+w; p2.y = y+h;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &cmp->fill_color,
                          &cmp->line_color);

  p1.x= x; p1.y = y +(h - 3*COMPONENT_CHEIGHT)/2.0;
  p2.x = x+COMPONENT_CWIDTH; p2.y = p1.y + COMPONENT_CHEIGHT;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &cmp->fill_color,
                          &cmp->line_color);

  p1.y = p2.y + COMPONENT_CHEIGHT;
  p2.y = p1.y + COMPONENT_CHEIGHT;

  dia_renderer_draw_rect (renderer,
                          &p1,
                          &p2,
                          &cmp->fill_color,
                          &cmp->line_color);

  if (cmp->st_stereotype != NULL &&
      cmp->st_stereotype[0] != '\0') {
    p1 = cmp->text->position;
    p1.y -= cmp->text->height;
    dia_renderer_set_font (renderer, cmp->text->font, cmp->text->height);
    dia_renderer_draw_string (renderer,
                              cmp->st_stereotype,
                              &p1,
                              DIA_ALIGN_LEFT,
                              &cmp->text->color);
  }

  text_draw (cmp->text, renderer);
}

static void
component_update_data(Component *cmp)
{
  Element *elem = &cmp->element;
  DiaObject *obj = &elem->object;
  Point p;
  real cw2, ch;

  cmp->stereotype = remove_stereotype_from_string(cmp->stereotype);
  if (!cmp->st_stereotype) {
    cmp->st_stereotype =  string_to_stereotype(cmp->stereotype);
  }

  text_calc_boundingbox(cmp->text, NULL);
  elem->width = cmp->text->max_width + 2*COMPONENT_MARGIN_X + COMPONENT_CWIDTH;
  elem->width = MAX(elem->width, 2*COMPONENT_CWIDTH);
  elem->height =  cmp->text->height*cmp->text->numlines +
    cmp->text->descent + 0.1 + 2*COMPONENT_MARGIN_Y ;
  elem->height = MAX(elem->height, 5*COMPONENT_CHEIGHT);

  p = elem->corner;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += COMPONENT_CHEIGHT;
  p.y += cmp->text->ascent;
  if (cmp->stereotype &&
      cmp->stereotype[0] != '\0') {
    p.y += cmp->text->height;
  }
  text_set_position(cmp->text, &p);

  if (cmp->st_stereotype &&
      cmp->st_stereotype[0] != '\0') {
    DiaFont *font;
    font = cmp->text->font;
    elem->height += cmp->text->height;
    elem->width = MAX(elem->width, dia_font_string_width(cmp->st_stereotype,
						     font, cmp->text->height) +
		      2*COMPONENT_MARGIN_X + COMPONENT_CWIDTH);
  }

  cw2 = COMPONENT_CWIDTH/2;
  ch = COMPONENT_CHEIGHT;
  /* Update connections: */
  connpoint_update(&cmp->connections[0],
		   elem->corner.x + cw2,
		   elem->corner.y,
		   DIR_NORTH|DIR_WEST);
  connpoint_update(&cmp->connections[1],
		   elem->corner.x + cw2 + (elem->width - cw2) / 2,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&cmp->connections[2],
		   elem->corner.x + elem->width,
		   elem->corner.y,
		   DIR_NORTH|DIR_EAST);
  connpoint_update(&cmp->connections[3],
		   elem->corner.x + cw2,
		   elem->corner.y + elem->height / 2.0,
		   DIR_WEST);
  connpoint_update(&cmp->connections[4],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height / 2.0,
		   DIR_EAST);
  connpoint_update(&cmp->connections[5],
		   elem->corner.x + cw2,
		   elem->corner.y + elem->height,
		   DIR_SOUTH|DIR_WEST);
  connpoint_update(&cmp->connections[6],
		   elem->corner.x + cw2 + (elem->width - cw2)/2,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&cmp->connections[7],
		   elem->corner.x + elem->width,
		   elem->corner.y + elem->height,
		   DIR_SOUTH|DIR_EAST);
  connpoint_update(&cmp->connections[8],
		   elem->corner.x,
		   elem->corner.y + elem->height / 2.0 - ch,
		   DIR_WEST);
  connpoint_update(&cmp->connections[9],
		   elem->corner.x,
		   elem->corner.y + elem->height / 2.0 + ch,
		   DIR_WEST);
  connpoint_update(&cmp->connections[10],
		   elem->corner.x + cw2 + (elem->width - cw2) / 2,
		   elem->corner.y + elem->height / 2.0,
		   DIR_ALL);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
component_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Component *cmp;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  cmp = g_new0 (Component, 1);
  elem = &cmp->element;
  obj = &elem->object;

  obj->type = &component_type;

  obj->ops = &component_ops;

  elem->corner = *startpoint;
  cmp->line_color = attributes_get_foreground();
  cmp->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, 0.8);
  p = *startpoint;
  p.x += COMPONENT_CWIDTH + COMPONENT_MARGIN_X;
  p.y += 2*COMPONENT_CHEIGHT;

  cmp->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_LEFT);

  g_clear_object (&font);

  element_init (elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &cmp->connections[i];
    cmp->connections[i].object = obj;
    cmp->connections[i].connected = NULL;
  }
  cmp->connections[10].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = COMPONENT_BORDERWIDTH/2.0;
  cmp->stereotype = NULL;
  cmp->st_stereotype = NULL;
  component_update_data(cmp);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &cmp->element.object;
}

static void
component_destroy(Component *cmp)
{
  text_destroy(cmp->text);
  g_clear_pointer (&cmp->stereotype, g_free);
  g_clear_pointer (&cmp->st_stereotype, g_free);
  element_destroy(&cmp->element);
}

static DiaObject *
component_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&component_type,
                                      obj_node,version,ctx);
}





