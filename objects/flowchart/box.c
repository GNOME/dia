/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Flowchart toolbox -- objects for drawing flowcharts.
 * Copyright (C) 1999 James Henstridge.
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

/* DO NOT USE THIS OBJECT AS A BASIS FOR A NEW OBJECT. */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>

#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/box.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Box Box;

struct _Box {
  Element element;

  ConnectionPoint connections[16];
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real corner_radius;

  Text *text;
  TextAttributes attrs;
  real padding;
};

typedef struct _BoxProperties {
  gboolean show_background;
  real corner_radius;
  real padding;
} BoxProperties;

static BoxProperties default_properties;

static real box_distance_from(Box *box, Point *point);
static void box_select(Box *box, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static void box_move_handle(Box *box, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void box_move(Box *box, Point *to);
static void box_draw(Box *box, DiaRenderer *renderer);
static void box_update_data(Box *box, AnchorShape horix, AnchorShape vert);
static Object *box_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void box_destroy(Box *box);
static PropDescription *box_describe_props(Box *box);
static void box_get_props(Box *box, GPtrArray *props);
static void box_set_props(Box *box, GPtrArray *props);

static void box_save(Box *box, ObjectNode obj_node, const char *filename);
static Object *box_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps box_type_ops =
{
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType fc_box_type =
{
  "Flowchart - Box",  /* name */
  0,                 /* version */
  (char **) box_xpm, /* pixmap */

  &box_type_ops      /* ops */
};

static ObjectOps box_ops = {
  (DestroyFunc)         box_destroy,
  (DrawFunc)            box_draw,
  (DistanceFunc)        box_distance_from,
  (SelectFunc)          box_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            box_move,
  (MoveHandleFunc)      box_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   box_describe_props,
  (GetPropsFunc)        box_get_props,
  (SetPropsFunc)        box_set_props,
};

static PropNumData corner_radius_data = { 0.0, 10.0, 0.1 };
static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription box_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "corner_radius", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Corner radius"), NULL, &corner_radius_data },
  { "padding", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Text padding"), NULL, &text_padding_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_SAVED_TEXT,
  
  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
box_describe_props(Box *box)
{
  if (box_props[0].quark == 0)
    prop_desc_list_calculate_quarks(box_props);
  return box_props;
}

static PropOffset box_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Box, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Box, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Box, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Box, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Box, line_style), offsetof(Box, dashlength) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Box, corner_radius) },
  { "padding", PROP_TYPE_REAL, offsetof(Box, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Box,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Box,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Box,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Box,attrs.color)},
  { NULL, 0, 0 },
};

static void
box_get_props(Box *box, GPtrArray *props)
{
  text_get_attributes(box->text,&box->attrs);
  object_get_props_from_offsets(&box->element.object,
                                box_offsets,props);
}

static void
box_set_props(Box *box, GPtrArray *props)
{
  object_set_props_from_offsets(&box->element.object,
                                box_offsets,props);
  apply_textattr_properties(props,box->text,"text",&box->attrs);
  box_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5;
    defaults_initialized = 1;
  }
}

static real
box_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;
  Rectangle rect;

  rect.left = elem->corner.x - box->border_width/2;
  rect.right = elem->corner.x + elem->width + box->border_width/2;
  rect.top = elem->corner.y - box->border_width/2;
  rect.bottom = elem->corner.y + elem->height + box->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
box_select(Box *box, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  real radius;

  text_set_cursor(box->text, clicked_point, interactive_renderer);
  text_grab_focus(box->text, &box->element.object);

  element_update_handles(&box->element);

  if (box->corner_radius > 0) {
    Element *elem = (Element *)box;
    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    radius *= (1-M_SQRT1_2);

    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
}

static void
box_move_handle(Box *box, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(box!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&box->element, handle->id, to, reason);

  switch (handle->id) {
  case HANDLE_RESIZE_NW:
    horiz = ANCHOR_END; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_N:
    vert = ANCHOR_END; break;
  case HANDLE_RESIZE_NE:
    horiz = ANCHOR_START; vert = ANCHOR_END; break;
  case HANDLE_RESIZE_E:
    horiz = ANCHOR_START; break;
  case HANDLE_RESIZE_SE:
    horiz = ANCHOR_START; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_S:
    vert = ANCHOR_START; break;
  case HANDLE_RESIZE_SW:
    horiz = ANCHOR_END; vert = ANCHOR_START; break;
  case HANDLE_RESIZE_W:
    horiz = ANCHOR_END; break;
  default:
    break;
  }
  box_update_data(box, horiz, vert);
}

static void
box_move(Box *box, Point *to)
{
  box->element.corner = *to;
  
  box_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
box_draw(Box *box, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  Point lr_corner;
  Element *elem;
  real radius;
  
  assert(box != NULL);
  assert(renderer != NULL);
  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  if (box->show_background) {
  renderer_ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
  /* Problem:  How do we make the fill with rounded corners? */
  if (box->corner_radius > 0) {
    Point start, end, center;

    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    start.x = center.x = elem->corner.x+radius;
    end.x = lr_corner.x-radius;
    start.y = elem->corner.y;
    end.y = lr_corner.y;
    renderer_ops->fill_rect(renderer, &start, &end, &box->inner_color);

    center.y = elem->corner.y+radius;
    renderer_ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    90.0, 180.0, &box->inner_color);
    center.x = end.x;
    renderer_ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    0.0, 90.0, &box->inner_color);

    start.x = elem->corner.x;
    start.y = elem->corner.y+radius;
    end.x = lr_corner.x;
    end.y = center.y = lr_corner.y-radius;
    renderer_ops->fill_rect(renderer, &start, &end, &box->inner_color);

    center.y = lr_corner.y-radius;
    center.x = elem->corner.x+radius;
    renderer_ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    180.0, 270.0, &box->inner_color);
    center.x = lr_corner.x-radius;
    renderer_ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    270.0, 360.0, &box->inner_color);
  } else {
  renderer_ops->fill_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->inner_color);
  }
  }

  renderer_ops->set_linewidth(renderer, box->border_width);
  renderer_ops->set_linestyle(renderer, box->line_style);
  renderer_ops->set_dashlength(renderer, box->dashlength);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);

  if (box->corner_radius > 0) {
    Point start, end, center;

    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    start.x = center.x = elem->corner.x+radius;
    end.x = lr_corner.x-radius;
    start.y = end.y = elem->corner.y;
    renderer_ops->draw_line(renderer, &start, &end, &box->border_color);
    start.y = end.y = lr_corner.y;
    renderer_ops->draw_line(renderer, &start, &end, &box->border_color);

    center.y = elem->corner.y+radius;
    renderer_ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    90.0, 180.0, &box->border_color);
    center.x = end.x;
    renderer_ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    0.0, 90.0, &box->border_color);

    start.y = elem->corner.y+radius;
    start.x = end.x = elem->corner.x;
    end.y = center.y = lr_corner.y-radius;
    renderer_ops->draw_line(renderer, &start, &end, &box->border_color);
    start.x = end.x = lr_corner.x;
    renderer_ops->draw_line(renderer, &start, &end, &box->border_color);

    center.y = lr_corner.y-radius;
    center.x = elem->corner.x+radius;
    renderer_ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    180.0, 270.0, &box->border_color);
    center.x = lr_corner.x-radius;
    renderer_ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    270.0, 360.0, &box->border_color);
  } else {
  renderer_ops->draw_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->border_color);
  }
  text_draw(box->text, renderer);
}

static void
box_update_data(Box *box, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &box->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  Object *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  real radius;
  real width, height;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox(box->text, NULL);
  width = box->text->max_width + box->padding*2 + box->border_width;
  height = box->text->height * box->text->numlines + box->padding*2 +
    box->border_width;

  /*
   *  If elem->width (e.g. the new requested dimensions of this object
   *  from move_handle()) is smaller than the minimum width (i.e. the
   *  width calculated from text-width, padding and border), then
   *  set the width to the minimum.  Else, keep the width.
   */
  if (width > elem->width) elem->width = width;
  if (height > elem->height) elem->height = height;

  /* move shape if necessary ... */
  switch (horiz) {
  case ANCHOR_MIDDLE:
    elem->corner.x = center.x - elem->width/2; break;
  case ANCHOR_END:
    elem->corner.x = bottom_right.x - elem->width; break;
  default:
    break;
  }
  switch (vert) {
  case ANCHOR_MIDDLE:
    elem->corner.y = center.y - elem->height/2; break;
  case ANCHOR_END:
    elem->corner.y = bottom_right.y - elem->height; break;
  default:
    break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - box->text->height * box->text->numlines / 2 +
    box->text->ascent;
  text_set_position(box->text, &p);

  radius = box->corner_radius;
  radius = MIN(radius, elem->width/2);
  radius = MIN(radius, elem->height/2);
  radius *= (1-M_SQRT1_2);
  
  /* Update connections: */
  box->connections[0].pos.x = elem->corner.x + radius;
  box->connections[0].pos.y = elem->corner.y + radius;
  box->connections[1].pos.x = elem->corner.x + elem->width / 4.0;
  box->connections[1].pos.y = elem->corner.y;
  box->connections[2].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[2].pos.y = elem->corner.y;
  box->connections[3].pos.x = elem->corner.x + elem->width * 3.0 / 4.0;
  box->connections[3].pos.y = elem->corner.y;
  box->connections[4].pos.x = elem->corner.x + elem->width - radius;
  box->connections[4].pos.y = elem->corner.y + radius;
  box->connections[5].pos.x = elem->corner.x;
  box->connections[5].pos.y = elem->corner.y + elem->height / 4.0;
  box->connections[6].pos.x = elem->corner.x + elem->width;
  box->connections[6].pos.y = elem->corner.y + elem->height / 4.0;
  box->connections[7].pos.x = elem->corner.x;
  box->connections[7].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[8].pos.x = elem->corner.x + elem->width;
  box->connections[8].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[9].pos.x = elem->corner.x;
  box->connections[9].pos.y = elem->corner.y + elem->height * 3.0 / 4.0;
  box->connections[10].pos.x = elem->corner.x + elem->width;
  box->connections[10].pos.y = elem->corner.y + elem->height * 3.0 / 4.0;
  box->connections[11].pos.x = elem->corner.x + radius;
  box->connections[11].pos.y = elem->corner.y + elem->height - radius;
  box->connections[12].pos.x = elem->corner.x + elem->width / 4.0;
  box->connections[12].pos.y = elem->corner.y + elem->height;
  box->connections[13].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[13].pos.y = elem->corner.y + elem->height;
  box->connections[14].pos.x = elem->corner.x + elem->width * 3.0 / 4.0;
  box->connections[14].pos.y = elem->corner.y + elem->height;
  box->connections[15].pos.x = elem->corner.x + elem->width - radius;
  box->connections[15].pos.y = elem->corner.y + elem->height - radius;

  extra->border_trans = box->border_width / 2.0;
  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  if (radius > 0.0) {
    /* Fix the handles, too */
    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
}

static Object *
box_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  Object *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  box = g_malloc0(sizeof(Box));
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &fc_box_type;

  obj->ops = &box_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  box->border_width =  attributes_get_default_linewidth();
  box->border_color = attributes_get_foreground();
  box->inner_color = attributes_get_background();
  box->show_background = default_properties.show_background;
  attributes_get_default_line_style(&box->line_style, &box->dashlength);
  box->corner_radius = default_properties.corner_radius;

  box->padding = default_properties.padding;

  attributes_get_default_font(&font, &font_height);
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + font_height / 2;
  box->text = new_text("", font, font_height, &p, &box->border_color,
                       ALIGN_CENTER);
  text_get_attributes(box->text,&box->attrs);
  dia_font_unref(font);
  
  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &box->element.object;
}

static void
box_destroy(Box *box)
{
  text_destroy(box->text);

  element_destroy(&box->element);
}

static void
box_save(Box *box, ObjectNode obj_node, const char *filename)
{
  element_save(&box->element, obj_node);

  if (box->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  box->border_width);
  
  if (!color_equals(&box->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &box->border_color);
   
  if (!color_equals(&box->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &box->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), 
                   box->show_background);

  if (box->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  box->line_style);
  
  if (box->line_style != LINESTYLE_SOLID &&
      box->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  box->dashlength);
  if (box->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
		  box->corner_radius);

  data_add_real(new_attribute(obj_node, "padding"), box->padding);
  
  data_add_text(new_attribute(obj_node, "text"), box->text);
}

static Object *
box_load(ObjectNode obj_node, int version, const char *filename)
{
  Box *box;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  box = g_new(Box, 1);
  elem = &box->element;
  obj = &elem->object;
  
  obj->type = &fc_box_type;
  obj->ops = &box_ops;

  element_load(elem, obj_node);

  box->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    box->border_width =  data_real( attribute_first_data(attr) );
  box->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->border_color);
  
  box->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->inner_color);
  
  box->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    box->show_background = data_boolean( attribute_first_data(attr) );

  box->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum( attribute_first_data(attr) );

  box->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    box->dashlength = data_real(attribute_first_data(attr));

  box->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    box->corner_radius =  data_real( attribute_first_data(attr) );

  box->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    box->padding =  data_real( attribute_first_data(attr) );
  

  box->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    box->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &box->element.object;
}
