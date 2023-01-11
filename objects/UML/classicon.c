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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <stdio.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "uml.h"

#include "pixmaps/classicon.xpm"

typedef struct _Classicon Classicon;

#define NUM_CONNECTIONS 9

struct _Classicon {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  int stereotype;
  int is_object;
  Text *text;
  Color line_color;
  Color fill_color;

  real line_width;
};

enum CLassIconStereotype {
    CLASSICON_CONTROL,
    CLASSICON_BOUNDARY,
    CLASSICON_ENTITY
};


#define CLASSICON_RADIOUS 1
#define CLASSICON_FONTHEIGHT 0.8
#define CLASSICON_MARGIN 0.5
#define CLASSICON_AIR 0.25
#define CLASSICON_ARROW 0.4

static real classicon_distance_from(Classicon *cicon, Point *point);
static void classicon_select(Classicon *cicon, Point *clicked_point,
			     DiaRenderer *interactive_renderer);
static DiaObjectChange* classicon_move_handle(Classicon *cicon, Handle *handle,
					   Point *to, ConnectionPoint *cp,
					   HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* classicon_move(Classicon *cicon, Point *to);
static void classicon_draw(Classicon *cicon, DiaRenderer *renderer);
static DiaObject *classicon_create(Point *startpoint,
				void *user_data,
				Handle **handle1,
				Handle **handle2);
static void classicon_destroy(Classicon *cicon);
static DiaObject *classicon_load(ObjectNode obj_node, int version,DiaContext *ctx);
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

DiaObjectType classicon_type =
{
  "UML - Classicon", /* name */
  0,              /* version */
  classicon_xpm,   /* pixmap */
  &classicon_type_ops /* ops */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   classicon_describe_props,
  (GetPropsFunc)        classicon_get_props,
  (SetPropsFunc)        classicon_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropEnumData prop_classicon_type_data[] = {
  { N_("Control"), CLASSICON_CONTROL },
  { N_("Boundary"), CLASSICON_BOUNDARY },
  { N_("Entity"), CLASSICON_ENTITY },
  { NULL, 0 }
};

static PropDescription classicon_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  /* how it used to be before 0.96+SVN */
  { "stereotype", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL, N_("Stereotype"), NULL,  prop_classicon_type_data},
  /* one name, one type: but breaks forward-compatibiliy so kind of reverted */
  { "type", PROP_TYPE_ENUM, PROP_FLAG_NO_DEFAULTS|PROP_FLAG_LOAD_ONLY|PROP_FLAG_OPTIONAL, N_("Stereotype"), NULL,  prop_classicon_type_data},

  { "is_object", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Is object"), NULL, NULL },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR_OPTIONAL,
  { "text", PROP_TYPE_TEXT, 0, N_("Text"), NULL, NULL },
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,

  PROP_DESC_END
};

static PropDescription *
classicon_describe_props(Classicon *classicon)
{
  if (classicon_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(classicon_props);
  }
  return classicon_props;
}

static PropOffset classicon_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  /* backward compatibility */
  { "stereotype", PROP_TYPE_ENUM, offsetof(Classicon, stereotype) },
  /* one name, one type! */
  { "type", PROP_TYPE_ENUM, offsetof(Classicon, stereotype) },
  { "is_object", PROP_TYPE_BOOL, offsetof(Classicon, is_object) },
  { "text",PROP_TYPE_TEXT,offsetof(Classicon,text)},
  { "text_font",PROP_TYPE_FONT,offsetof(Classicon,text),offsetof(Text,font)},
  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Classicon,text),offsetof(Text,height)},
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Classicon, line_width) },
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Classicon,text),offsetof(Text,color)},
  { "line_colour",PROP_TYPE_COLOUR,offsetof(Classicon,line_color) },
  { "fill_colour",PROP_TYPE_COLOUR,offsetof(Classicon,fill_color) },
  { NULL, 0, 0 },
};

static void
classicon_get_props(Classicon * classicon, GPtrArray *props)
{
  object_get_props_from_offsets(&classicon->element.object,
                                classicon_offsets,props);
}

static void
classicon_set_props(Classicon *classicon, GPtrArray *props)
{
  object_set_props_from_offsets(&classicon->element.object,
                                classicon_offsets,props);
  classicon_update_data(classicon);
}

static real
classicon_distance_from(Classicon *cicon, Point *point)
{
  DiaObject *obj = &cicon->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
classicon_select(Classicon *cicon, Point *clicked_point,
		 DiaRenderer *interactive_renderer)
{
  text_set_cursor(cicon->text, clicked_point, interactive_renderer);
  text_grab_focus(cicon->text, &cicon->element.object);
  element_update_handles(&cicon->element);
}


static DiaObjectChange *
classicon_move_handle (Classicon        *cicon,
                       Handle           *handle,
                       Point            *to,
                       ConnectionPoint  *cp,
                       HandleMoveReason  reason,
                       ModifierKeys      modifiers)
{
  g_return_val_if_fail (cicon != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
classicon_move(Classicon *cicon, Point *to)
{
  Element *elem = &cicon->element;

  elem->corner = *to;
  elem->corner.x -= elem->width/2.0;
  elem->corner.y -= CLASSICON_RADIOUS + CLASSICON_ARROW;

  if (cicon->stereotype==CLASSICON_BOUNDARY)
    elem->corner.x -= CLASSICON_RADIOUS/2.0;

  classicon_update_data(cicon);

  return NULL;
}

static void
classicon_draw (Classicon *icon, DiaRenderer *renderer)
{
  Element *elem;
  double r, x, y, w;
  Point center, p1, p2;
  int i;

  g_return_if_fail (icon != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &icon->element;

  x = elem->corner.x;
  y = elem->corner.y;
  w = elem->width;

  r = CLASSICON_RADIOUS;
  center.x = x + elem->width/2;
  center.y = y + r + CLASSICON_ARROW;

  if (icon->stereotype==CLASSICON_BOUNDARY) {
    center.x += r/2.0;
  }

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, icon->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  dia_renderer_draw_ellipse (renderer,
                             &center,
                             2*r, 2*r,
                             &icon->fill_color,
                             &icon->line_color);

  switch (icon->stereotype) {
    case CLASSICON_CONTROL:
      p1.x = center.x - r*0.258819045102521;
      p1.y = center.y-r*0.965925826289068;

      p2.x = p1.x + CLASSICON_ARROW;
      p2.y = p1.y + CLASSICON_ARROW/1.5;
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      p2.x = p1.x + CLASSICON_ARROW;
      p2.y = p1.y - CLASSICON_ARROW/1.5;
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      break;

    case CLASSICON_BOUNDARY:
      p1.x = center.x - r;
      p2.x = p1.x - r;
      p1.y = p2.y = center.y;
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      p1.x = p2.x;
      p1.y = center.y - r;
      p2.y = center.y + r;
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      break;
    case CLASSICON_ENTITY:
      p1.x = center.x - r;
      p2.x = center.x + r;
      p1.y = p2.y = center.y + r;
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      break;
    default:
      g_return_if_reached ();
  }

  text_draw (icon->text, renderer);

  if (icon->is_object) {
    dia_renderer_set_linewidth (renderer, 0.01);
    if (icon->stereotype==CLASSICON_BOUNDARY) {
      x += r/2.0;
    }
    p1.y = p2.y = icon->text->position.y + text_get_descent (icon->text);
    for (i=0; i<icon->text->numlines; i++) {
      p1.x = x + (w - text_get_line_width (icon->text, i))/2;
      p2.x = p1.x + text_get_line_width (icon->text, i);
      dia_renderer_draw_line (renderer,
                              &p1, &p2,
                              &icon->line_color);
      p1.y = p2.y += icon->text->height;
    }
  }
}

static void
classicon_update_data(Classicon *cicon)
{
  Element *elem = &cicon->element;
  DiaObject *obj = &elem->object;
  Point p1;
  real h, wt, w = 0;
  int is_boundary = (cicon->stereotype==CLASSICON_BOUNDARY);

  text_calc_boundingbox(cicon->text, NULL);
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
  cicon->connections[0].directions = DIR_WEST|DIR_NORTH;
  cicon->connections[1].pos.x = p1.x;
  cicon->connections[1].pos.y = p1.y - w;
  cicon->connections[1].directions = DIR_NORTH;
  cicon->connections[2].pos.x = p1.x + h;
  cicon->connections[2].pos.y = p1.y - h;
  cicon->connections[2].directions = DIR_NORTH|DIR_EAST;

  cicon->connections[3].pos.x = (is_boundary) ? p1.x-2*w: p1.x - w;
  cicon->connections[3].pos.y = p1.y;
  cicon->connections[3].directions = DIR_WEST;
  cicon->connections[4].pos.x = p1.x + w;
  cicon->connections[4].pos.y = p1.y;
  cicon->connections[4].directions = DIR_EAST;
  cicon->connections[5].pos.x = elem->corner.x;
  cicon->connections[5].pos.y = elem->corner.y + elem->height;
  cicon->connections[5].directions = DIR_SOUTH|DIR_WEST;
  cicon->connections[6].pos.x = p1.x;
  cicon->connections[6].pos.y = elem->corner.y + elem->height;
  cicon->connections[6].directions = DIR_SOUTH;
  cicon->connections[7].pos.x = elem->corner.x + elem->width;
  cicon->connections[7].pos.y = elem->corner.y + elem->height;
  cicon->connections[7].directions = DIR_SOUTH|DIR_EAST;
  cicon->connections[8].pos.x = elem->corner.x + elem->width/2;
  cicon->connections[8].pos.y = elem->corner.y + elem->height/2;
  cicon->connections[8].directions = DIR_ALL;

  element_update_boundingbox(elem);

  obj->position = elem->corner;
  obj->position.x += (elem->width + ((is_boundary)?CLASSICON_RADIOUS:0))/2.0;
  obj->position.y += CLASSICON_RADIOUS + CLASSICON_ARROW;

  element_update_handles(elem);
}

static DiaObject *
classicon_create(Point *startpoint,
		    void *user_data,
		    Handle **handle1,
		    Handle **handle2)
{
  Classicon *cicon;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  cicon = g_new0 (Classicon, 1);

  /* old default */
  cicon->line_width = 0.1;

  elem = &cicon->element;
  obj = &elem->object;

  obj->type = &classicon_type;

  obj->ops = &classicon_ops;

  elem->corner = *startpoint;
  cicon->line_color = attributes_get_foreground();
  cicon->fill_color = attributes_get_background();

  font = dia_font_new_from_style (DIA_FONT_SANS, 0.8);

  cicon->stereotype = 0;
  cicon->is_object = 0;

  /* The text position is recalculated later */
  p.x = 0.0;
  p.y = 0.0;
  cicon->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_CENTRE);

  g_clear_object (&font);

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &cicon->connections[i];
    cicon->connections[i].object = obj;
    cicon->connections[i].connected = NULL;
  }
  cicon->connections[8].flags = CP_FLAGS_MAIN;
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

static DiaObject *
classicon_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  return object_load_using_properties(&classicon_type,
                                      obj_node,version,ctx);
}


