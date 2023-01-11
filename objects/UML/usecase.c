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

#include "pixmaps/case.xpm"

typedef struct _UsecasePropertiesDialog UsecasePropertiesDialog;
typedef struct _Usecase Usecase;
typedef struct _UsecaseState UsecaseState;

#define NUM_CONNECTIONS 9

struct _Usecase {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];

  Text *text;
  int text_outside;
  int collaboration;

  real line_width;
  Color line_color;
  Color fill_color;
};


struct _UsecasePropertiesDialog {
  GtkWidget *dialog;

  GtkToggleButton *text_out;
  GtkToggleButton *collaboration;
};


#define USECASE_WIDTH 3.25
#define USECASE_HEIGHT 2
#define USECASE_MIN_RATIO 1.5
#define USECASE_MAX_RATIO 3
#define USECASE_LINEWIDTH 0.1
#define USECASE_MARGIN_Y 0.3

static real usecase_distance_from(Usecase *usecase, Point *point);
static void usecase_select(Usecase *usecase, Point *clicked_point,
			   DiaRenderer *interactive_renderer);
static DiaObjectChange* usecase_move_handle(Usecase *usecase, Handle *handle,
					 Point *to, ConnectionPoint *cp,
					 HandleMoveReason reason, ModifierKeys modifiers);
static DiaObjectChange* usecase_move(Usecase *usecase, Point *to);
static void usecase_draw(Usecase *usecase, DiaRenderer *renderer);
static DiaObject *usecase_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void usecase_destroy(Usecase *usecase);
static DiaObject *usecase_load(ObjectNode obj_node, int version,DiaContext *ctx);
static void usecase_update_data(Usecase *usecase);
static PropDescription *usecase_describe_props(Usecase *usecase);
static void usecase_get_props(Usecase *usecase, GPtrArray *props);
static void usecase_set_props(Usecase *usecase, GPtrArray *props);

static ObjectTypeOps usecase_type_ops =
{
  (CreateFunc) usecase_create,
  (LoadFunc)   usecase_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType usecase_type =
{
  "UML - Usecase", /* name */
  0,            /* version */
  case_xpm,      /* pixmap */
  &usecase_type_ops /* ops */
};

static ObjectOps usecase_ops = {
  (DestroyFunc)         usecase_destroy,
  (DrawFunc)            usecase_draw,
  (DistanceFunc)        usecase_distance_from,
  (SelectFunc)          usecase_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            usecase_move,
  (MoveHandleFunc)      usecase_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   usecase_describe_props,
  (GetPropsFunc)        usecase_get_props,
  (SetPropsFunc)        usecase_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription usecase_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "collaboration", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Collaboration"), NULL, NULL },
  { "text_outside", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
  N_("Text outside"), NULL, NULL },
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
usecase_describe_props(Usecase *usecase)
{
  if (usecase_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(usecase_props);
  }
  return usecase_props;
}

static PropOffset usecase_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  {"collaboration", PROP_TYPE_BOOL, offsetof(Usecase, collaboration) },
  {"text_outside", PROP_TYPE_BOOL, offsetof(Usecase, text_outside) },
  {"text",PROP_TYPE_TEXT,offsetof(Usecase,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Usecase,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Usecase,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Usecase,text),offsetof(Text,color)},
  {PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Usecase, line_width)},
  {"line_colour", PROP_TYPE_COLOUR, offsetof(Usecase, line_color) },
  {"fill_colour", PROP_TYPE_COLOUR, offsetof(Usecase, fill_color) },
  { NULL, 0, 0 },
};

static void
usecase_get_props(Usecase * usecase, GPtrArray *props)
{
  object_get_props_from_offsets(&usecase->element.object,
                                usecase_offsets,props);
}

static void
usecase_set_props(Usecase *usecase, GPtrArray *props)
{
  object_set_props_from_offsets(&usecase->element.object,
                                usecase_offsets,props);
  usecase_update_data(usecase);
}

static real
usecase_distance_from(Usecase *usecase, Point *point)
{
  DiaObject *obj = &usecase->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
usecase_select(Usecase *usecase, Point *clicked_point,
	       DiaRenderer *interactive_renderer)
{
  text_set_cursor(usecase->text, clicked_point, interactive_renderer);
  text_grab_focus(usecase->text, &usecase->element.object);
  element_update_handles(&usecase->element);
}


static DiaObjectChange *
usecase_move_handle (Usecase          *usecase,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  g_return_val_if_fail (usecase != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  g_return_val_if_fail (handle->id < 8, NULL);

  return NULL;
}


static DiaObjectChange*
usecase_move(Usecase *usecase, Point *to)
{
  real h;
  Point p;

  usecase->element.corner = *to;
  h = usecase->text->height*usecase->text->numlines;

  p = *to;
  p.x += usecase->element.width/2.0;
  if (usecase->text_outside) {
      p.y += usecase->element.height - h + usecase->text->ascent;
  } else {
      p.y += (usecase->element.height - h)/2.0 + usecase->text->ascent;
  }
  text_set_position(usecase->text, &p);
  usecase_update_data(usecase);

  return NULL;
}


static void
usecase_draw (Usecase *usecase, DiaRenderer *renderer)
{
  Element *elem;
  double x, y, w, h;
  Point c;

  g_return_if_fail (usecase != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &usecase->element;

  x = elem->corner.x;
  y = elem->corner.y;

  if (usecase->text_outside) {
      w = USECASE_WIDTH;
      h = USECASE_HEIGHT;
      c.x = x + elem->width/2.0;
      c.y = y + USECASE_HEIGHT / 2.0;
   } else {
      w = elem->width;
      h = elem->height;
      c.x = x + w/2.0;
      c.y = y + h/2.0;
  }


  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, usecase->line_width);

  if (usecase->collaboration)
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, 1.0);
  else
    dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  dia_renderer_draw_ellipse (renderer,
                             &c,
                             w,
                             h,
                             &usecase->fill_color,
                             &usecase->line_color);

  text_draw (usecase->text, renderer);
}


static void
usecase_update_data(Usecase *usecase)
{
  real w, h, ratio;
  Point c, half, r,p;

  Element *elem = &usecase->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;

  text_calc_boundingbox(usecase->text, NULL);
  w = usecase->text->max_width;
  h = usecase->text->height*usecase->text->numlines;

  if (!usecase->text_outside) {
      ratio = w/h;

      if (ratio > USECASE_MAX_RATIO)
	  ratio = USECASE_MAX_RATIO;

      if (ratio < USECASE_MIN_RATIO) {
	  ratio = USECASE_MIN_RATIO;
	  r.y = w / ratio + h;
	  r.x = r.y * ratio;
      } else {
	  r.x = ratio*h + w;
	  r.y = r.x / ratio;
      }
      if (r.x < USECASE_WIDTH)
	      r.x = USECASE_WIDTH;
      if (r.y < USECASE_HEIGHT)
	      r.y = USECASE_HEIGHT;
  } else {
      r.x = USECASE_WIDTH;
      r.y = USECASE_HEIGHT;
  }

  elem->width = r.x;
  elem->height = r.y;
  extra->border_trans = usecase->line_width / 2.0;

  if (usecase->text_outside) {
	  elem->width = MAX(elem->width, w);
	  elem->height += h + USECASE_MARGIN_Y;
  }

  r.x /= 2.0;
  r.y /= 2.0;
  c.x = elem->corner.x + elem->width / 2.0;
  c.y = elem->corner.y + r.y;
  half.x = r.x * M_SQRT1_2;
  half.y = r.y * M_SQRT1_2;

  /* Update connections: */
  usecase->connections[0].pos.x = c.x - half.x;
  usecase->connections[0].pos.y = c.y - half.y;
  usecase->connections[1].pos.x = c.x;
  usecase->connections[1].pos.y = elem->corner.y;
  usecase->connections[2].pos.x = c.x + half.x;
  usecase->connections[2].pos.y = c.y - half.y;
  usecase->connections[3].pos.x = c.x - r.x;
  usecase->connections[3].pos.y = c.y;
  usecase->connections[4].pos.x = c.x + r.x;
  usecase->connections[4].pos.y = c.y;

  if (usecase->text_outside) {
      usecase->connections[5].pos.x = elem->corner.x;
      usecase->connections[5].pos.y = elem->corner.y + elem->height;
      usecase->connections[6].pos.x = c.x;
      usecase->connections[6].pos.y = elem->corner.y + elem->height;
      usecase->connections[7].pos.x = elem->corner.x + elem->width;
      usecase->connections[7].pos.y = elem->corner.y + elem->height;
  } else {
      usecase->connections[5].pos.x = c.x - half.x;
      usecase->connections[5].pos.y = c.y + half.y;
      usecase->connections[6].pos.x = c.x;
      usecase->connections[6].pos.y = elem->corner.y + elem->height;
      usecase->connections[7].pos.x = c.x + half.x;
      usecase->connections[7].pos.y = c.y + half.y;
  }
  usecase->connections[8].pos.x = c.x;
  usecase->connections[8].pos.y = c.y;

  usecase->connections[0].directions = DIR_NORTH|DIR_WEST;
  usecase->connections[1].directions = DIR_NORTH;
  usecase->connections[2].directions = DIR_NORTH|DIR_EAST;
  usecase->connections[3].directions = DIR_WEST;
  usecase->connections[4].directions = DIR_EAST;
  usecase->connections[5].directions = DIR_SOUTH|DIR_WEST;
  usecase->connections[6].directions = DIR_SOUTH;
  usecase->connections[7].directions = DIR_SOUTH|DIR_EAST;
  usecase->connections[8].directions = DIR_ALL;

  h = usecase->text->height*usecase->text->numlines;
  p = usecase->element.corner;
  p.x += usecase->element.width/2.0;
  if (usecase->text_outside) {
      p.y += usecase->element.height - h + usecase->text->ascent;
  } else {
      p.y += (usecase->element.height - h)/2.0 + usecase->text->ascent;
  }
  text_set_position(usecase->text, &p);

  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);

}

static DiaObject *
usecase_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Usecase *usecase;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont *font;
  int i;

  usecase = g_new0 (Usecase, 1);
  elem = &usecase->element;
  obj = &elem->object;

  obj->type = &usecase_type;
  obj->ops = &usecase_ops;
  elem->corner = *startpoint;
  elem->width = USECASE_WIDTH;
  elem->height = USECASE_HEIGHT;

  usecase->line_width = attributes_get_default_linewidth();
  usecase->line_color = attributes_get_foreground();
  usecase->fill_color = attributes_get_background();

  font = dia_font_new_from_style(DIA_FONT_SANS, 0.8);
  p = *startpoint;
  p.x += USECASE_WIDTH/2.0;
  p.y += USECASE_HEIGHT/2.0;

  usecase->text = new_text ("", font, 0.8, &p, &color_black, DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  usecase->text_outside = 0;
  usecase->collaboration = 0;
  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &usecase->connections[i];
    usecase->connections[i].object = obj;
    usecase->connections[i].connected = NULL;
  }
  usecase->connections[8].flags = CP_FLAGS_MAIN;
  elem->extra_spacing.border_trans = 0.0;
  usecase_update_data(usecase);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = NULL;
  return &usecase->element.object;
}

static void
usecase_destroy(Usecase *usecase)
{
  text_destroy(usecase->text);

  element_destroy(&usecase->element);
}

static DiaObject *
usecase_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&usecase_type,
                                                obj_node,version,ctx);
  AttributeNode attr;
  /* For compatibility with previous dia files. If no line_width, use
   * USECASE_LINEWIDTH, that was the previous line width.
   */
  attr = object_find_attribute(obj_node, PROP_STDNAME_LINE_WIDTH);
  if (attr == NULL)
    ((Usecase*)obj)->line_width = USECASE_LINEWIDTH;

  return obj;
}




