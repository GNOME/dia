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
#include "render.h"
#include "attributes.h"
#include "text.h"
#include "widgets.h"
#include "message.h"
#include "properties.h"

#include "pixmaps/ellipse.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Ellipse Ellipse;

struct _Ellipse {
  Element element;

  ConnectionPoint connections[16];
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;

  Text *text;
  TextAttributes attrs;
  real padding;
};

typedef struct _EllipseProperties {
  gboolean show_background;
  real padding;
} EllipseProperties;

static EllipseProperties default_properties;

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
		       Renderer *interactive_renderer);
static void ellipse_move_handle(Ellipse *ellipse, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
static void ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, Renderer *renderer);
static void ellipse_update_data(Ellipse *ellipse, AnchorShape h,AnchorShape v);
static Object *ellipse_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);

static PropDescription *ellipse_describe_props(Ellipse *ellipse);
static void ellipse_get_props(Ellipse *ellipse, GPtrArray *props);
static void ellipse_set_props(Ellipse *ellipse, GPtrArray *props);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename);
static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

ObjectType fc_ellipse_type =
{
  "Flowchart - Ellipse",  /* name */
  0,                 /* version */
  (char **) ellipse_xpm, /* pixmap */

  &ellipse_type_ops      /* ops */
};

static ObjectOps ellipse_ops = {
  (DestroyFunc)         ellipse_destroy,
  (DrawFunc)            ellipse_draw,
  (DistanceFunc)        ellipse_distance_from,
  (SelectFunc)          ellipse_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            ellipse_move,
  (MoveHandleFunc)      ellipse_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   ellipse_describe_props,
  (GetPropsFunc)        ellipse_get_props,
  (SetPropsFunc)        ellipse_set_props,
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription ellipse_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "padding", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Text padding"), NULL, &text_padding_data },
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_SAVED_TEXT,
  
  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
ellipse_describe_props(Ellipse *ellipse)
{
  if (ellipse_props[0].quark == 0)
    prop_desc_list_calculate_quarks(ellipse_props);
  return ellipse_props;
}

static PropOffset ellipse_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "line_width", PROP_TYPE_REAL, offsetof(Ellipse, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Ellipse, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Ellipse, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Ellipse, line_style), offsetof(Ellipse, dashlength) },
  { "padding", PROP_TYPE_REAL, offsetof(Ellipse, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Ellipse,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Ellipse,attrs.font)},
  {"text_height",PROP_TYPE_REAL,offsetof(Ellipse,attrs.height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Ellipse,attrs.color)},
  { NULL, 0, 0 },
};

static void
ellipse_get_props(Ellipse *ellipse, GPtrArray *props)
{
  text_get_attributes(ellipse->text,&ellipse->attrs);
  object_get_props_from_offsets(&ellipse->element.object,
                                ellipse_offsets,props);
}

static void
ellipse_set_props(Ellipse *ellipse, GPtrArray *props)
{
  object_set_props_from_offsets(&ellipse->element.object,
                                ellipse_offsets,props);
  apply_textattr_properties(props,ellipse->text,"text",&ellipse->attrs);
  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5 * M_SQRT1_2;
    defaults_initialized = 1;
  }
}

/* returns the radius of the ellipse along the ray from the centre of the
 * ellipse to the point (px, py) */
static real
ellipse_radius(Ellipse *ellipse, real px, real py)
{
  Element *elem = &ellipse->element;
  real w2 = elem->width * elem->width;
  real h2 = elem->height * elem->height;
  real scale;

  /* find the point of intersection between line (x=cx+(px-cx)t; y=cy+(py-cy)t)
   * and ellipse ((x-cx)^2)/(w/2)^2 + ((y-cy)^2)/(h/2)^2 = 1 */
  /* radius along ray is sqrt((px-cx)^2 * t^2 + (py-cy)^2 * t^2) */

  /* normalize coordinates ... */
  px -= elem->corner.x + elem->width  / 2;
  py -= elem->corner.y + elem->height / 2;
  /* square them ... */
  px *= px;
  py *= py;

  scale = w2 * h2 / (4*h2*px + 4*w2*py);
  return sqrt((px + py)*scale);
}

static real
ellipse_distance_from(Ellipse *ellipse, Point *point)
{
  Element *elem = &ellipse->element;
  Point c;
  real dist, rad;

  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height/ 2;
  dist = distance_point_point(point, &c);
  rad = ellipse_radius(ellipse, point->x, point->y) + ellipse->border_width/2;

  if (dist <= rad)
    return 0;
  return dist - rad;
}

static void
ellipse_select(Ellipse *ellipse, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  text_set_cursor(ellipse->text, clicked_point, interactive_renderer);
  text_grab_focus(ellipse->text, &ellipse->element.object);

  element_update_handles(&ellipse->element);
}

static void
ellipse_move_handle(Ellipse *ellipse, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  assert(ellipse!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&ellipse->element, handle->id, to, reason);

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
  ellipse_update_data(ellipse, horiz, vert);
}

static void
ellipse_move(Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  
  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static void
ellipse_draw(Ellipse *ellipse, Renderer *renderer)
{
  Element *elem;
  Point center;
  
  assert(ellipse != NULL);
  assert(renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
    renderer->ops->fill_ellipse(renderer, &center,
				elem->width, elem->height,
				&ellipse->inner_color);
  }

  renderer->ops->set_linewidth(renderer, ellipse->border_width);
  renderer->ops->set_linestyle(renderer, ellipse->line_style);
  renderer->ops->set_dashlength(renderer, ellipse->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_ellipse(renderer, &center,
			      elem->width, elem->height,
			      &ellipse->border_color);

  text_draw(ellipse->text, renderer);
}


static void
ellipse_update_data(Ellipse *ellipse, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &ellipse->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  Object *obj = &elem->object;
  Point center, bottom_right;
  Point p, c;
  real dw, dh;
  real width, height;
  real radius1, radius2;
  int i;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox(ellipse->text, NULL);
  width = ellipse->text->max_width + 2 * ellipse->padding;
  height = ellipse->text->height * ellipse->text->numlines +
    2 * ellipse->padding;

  /* stop ellipse from getting infinite width/height */
  if (elem->width / elem->height > 4)
    elem->width = elem->height * 4;
  else if (elem->height / elem->width > 4)
    elem->height = elem->width * 4;

  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height / 2;
  p.x = c.x - width  / 2;
  p.y = c.y - height / 2;
  radius1 = ellipse_radius(ellipse, p.x, p.y) - ellipse->border_width/2;
  radius2 = distance_point_point(&c, &p);
  
  if (radius1 < radius2) {
    /* increase size of the ellipse while keeping its aspect ratio */
    elem->width  *= radius2 / radius1;
    elem->height *= radius2 / radius1;
  }

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
  p.y += elem->height / 2.0 - ellipse->text->height*ellipse->text->numlines/2 +
    ellipse->text->ascent;
  text_set_position(ellipse->text, &p);

  /* Update connections: */
  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height / 2;
  dw = elem->width  / 2.0;
  dh = elem->height / 2.0;
  for (i = 0; i < 16; i++) {
    real theta = M_PI / 8.0 * i;
    ellipse->connections[i].pos.x = c.x + dw * cos(theta);
    ellipse->connections[i].pos.y = c.y - dh * sin(theta);
  }

  extra->border_trans = ellipse->border_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
ellipse_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  ellipse = g_malloc0(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = &elem->object;
  
  obj->type = &fc_ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();
  ellipse->show_background = default_properties.show_background;
  attributes_get_default_line_style(&ellipse->line_style,&ellipse->dashlength);

  ellipse->padding = default_properties.padding;

  attributes_get_default_font(&font, &font_height);
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + font_height / 2;
  ellipse->text = new_text("", font, font_height, &p, &ellipse->border_color,
			   ALIGN_CENTER);
  text_get_attributes(ellipse->text,&ellipse->attrs);
  dia_font_unref(font);
  
  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }

  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return &ellipse->element.object;
}

static void
ellipse_destroy(Ellipse *ellipse)
{
  text_destroy(ellipse->text);

  element_destroy(&ellipse->element);
}

static void
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename)
{
  element_save(&ellipse->element, obj_node);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width);
  
  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color);
  
  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), ellipse->show_background);

  if (ellipse->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style);

  if (ellipse->line_style != LINESTYLE_SOLID &&
      ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  ellipse->dashlength);

  data_add_real(new_attribute(obj_node, "padding"), ellipse->padding);

  data_add_text(new_attribute(obj_node, "text"), ellipse->text);
}

static Object *
ellipse_load(ObjectNode obj_node, int version, const char *filename)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  ellipse = g_malloc0(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = &elem->object;
  
  obj->type = &fc_ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node);
  
  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real( attribute_first_data(attr) );

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color);
  
  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color);
  
  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean( attribute_first_data(attr) );

  ellipse->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum( attribute_first_data(attr) );

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    ellipse->dashlength = data_real(attribute_first_data(attr));

  ellipse->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    ellipse->padding =  data_real( attribute_first_data(attr) );
  
  ellipse->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    ellipse->text = data_text(attribute_first_data(attr));

  element_init(elem, 8, 16);

  for (i=0;i<16;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }

  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &ellipse->element.object;
}
