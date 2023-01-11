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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#include "pixmaps/diamond.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

#define NUM_CONNECTIONS 17

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Diamond Diamond;

struct _Diamond {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];
  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  DiaLineStyle line_style;
  double dashlength;

  Text *text;
  double padding;

  DiaTextFitting text_fitting;
};

typedef struct _DiamondProperties {
  gboolean show_background;

  real padding;
} DiamondProperties;

static DiamondProperties default_properties;

static real diamond_distance_from(Diamond *diamond, Point *point);
static void diamond_select(Diamond *diamond, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange* diamond_move_handle     (Diamond          *diamond,
                                                 Handle           *handle,
                                                 Point            *to,
                                                 ConnectionPoint  *cp,
                                                 HandleMoveReason  reason,
                                                 ModifierKeys      modifiers);
static DiaObjectChange* diamond_move            (Diamond          *diamond,
                                                 Point            *to);
static void diamond_draw(Diamond *diamond, DiaRenderer *renderer);
static void diamond_update_data(Diamond *diamond, AnchorShape h,AnchorShape v);
static DiaObject *diamond_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void diamond_destroy(Diamond *diamond);

static PropDescription *diamond_describe_props(Diamond *diamond);
static void diamond_get_props(Diamond *diamond, GPtrArray *props);
static void diamond_set_props(Diamond *diamond, GPtrArray *props);

static void diamond_save(Diamond *diamond, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *diamond_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps diamond_type_ops =
{
  (CreateFunc) diamond_create,
  (LoadFunc)   diamond_load,
  (SaveFunc)   diamond_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType diamond_type =
{
  "Flowchart - Diamond", /* name */
  0,                  /* version */
  diamond_xpm,         /* pixmap */
  &diamond_type_ops       /* ops */
};

static ObjectOps diamond_ops = {
  (DestroyFunc)         diamond_destroy,
  (DrawFunc)            diamond_draw,
  (DistanceFunc)        diamond_distance_from,
  (SelectFunc)          diamond_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            diamond_move,
  (MoveHandleFunc)      diamond_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   diamond_describe_props,
  (GetPropsFunc)        diamond_get_props,
  (SetPropsFunc)        diamond_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription diamond_props[] = {
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
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FITTING,
  PROP_STD_SAVED_TEXT,

  { NULL, 0, 0, NULL, NULL, NULL, 0}
};

static PropDescription *
diamond_describe_props(Diamond *diamond)
{
  if (diamond_props[0].quark == 0)
    prop_desc_list_calculate_quarks(diamond_props);
  return diamond_props;
}

static PropOffset diamond_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Diamond, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Diamond, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Diamond, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Diamond, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Diamond, line_style), offsetof(Diamond, dashlength) },
  { "padding", PROP_TYPE_REAL, offsetof(Diamond, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Diamond,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Diamond,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Diamond,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Diamond,text),offsetof(Text,color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Diamond,text),offsetof(Text,alignment)},
  {PROP_STDNAME_TEXT_FITTING,PROP_TYPE_ENUM,offsetof(Diamond,text_fitting)},
  { NULL, 0, 0 },
};

static void
diamond_get_props(Diamond *diamond, GPtrArray *props)
{
  object_get_props_from_offsets(&diamond->element.object,
                                diamond_offsets,props);
}

static void
diamond_set_props(Diamond *diamond, GPtrArray *props)
{
  object_set_props_from_offsets(&diamond->element.object,
                                diamond_offsets,props);
  diamond_update_data(diamond,ANCHOR_MIDDLE,ANCHOR_MIDDLE);
}


static void
init_default_values (void)
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5 * M_SQRT1_2;
    defaults_initialized = 1;
  }
}


static real
diamond_distance_from (Diamond *diamond, Point *point)
{
  Element *elem = &diamond->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - diamond->border_width/2;
  rect.right = elem->corner.x + elem->width + diamond->border_width/2;
  rect.top = elem->corner.y - diamond->border_width/2;
  rect.bottom = elem->corner.y + elem->height + diamond->border_width/2;

  if (rect.top > point->y) {
    return rect.top - point->y +
      fabs(point->x - elem->corner.x + elem->width / 2.0);
  } else if (point->y > rect.bottom) {
    return point->y - rect.bottom +
      fabs(point->x - elem->corner.x + elem->width / 2.0);
  } else if (rect.left > point->x) {
    return rect.left - point->x +
      fabs(point->y - elem->corner.y + elem->height / 2.0);
  } else if (point->x > rect.right) {
    return point->x - rect.right +
      fabs(point->y - elem->corner.y + elem->height / 2.0);
  } else {
    /* inside the bounding box of diamond ... this is where it gets harder */
    real x = point->x, y = point->y;
    real dx, dy;

    /* reflect point into upper left quadrant of diamond */
    if (x > elem->corner.x + elem->width / 2.0) {
      x = 2 * (elem->corner.x + elem->width / 2.0) - x;
    }
    if (y > elem->corner.y + elem->height / 2.0) {
      y = 2 * (elem->corner.y + elem->height / 2.0) - y;
    }

    dx = -x + elem->corner.x + elem->width / 2.0 -
      elem->width/elem->height * (y-elem->corner.y) - diamond->border_width/2;
    dy = -y + elem->corner.y + elem->height / 2.0 -
      elem->height/elem->width * (x-elem->corner.x) - diamond->border_width/2;

    if (dx <= 0 || dy <= 0)
      return 0;

    return MIN (dx, dy);
  }
}


static void
diamond_select (Diamond     *diamond,
                Point       *clicked_point,
                DiaRenderer *interactive_renderer)
{
  text_set_cursor (diamond->text, clicked_point, interactive_renderer);
  text_grab_focus (diamond->text, &diamond->element.object);

  element_update_handles (&diamond->element);
}


static DiaObjectChange *
diamond_move_handle (Diamond          *diamond,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  Point corner;
  double width, height;

  g_return_val_if_fail (diamond != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  /* remember ... */
  corner = diamond->element.corner;
  width = diamond->element.width;
  height = diamond->element.height;

  element_move_handle (&diamond->element, handle->id, to, cp,
                       reason, modifiers);

  switch (handle->id) {
    case HANDLE_RESIZE_NW:
      horiz = ANCHOR_END;
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_N:
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_NE:
      horiz = ANCHOR_START;
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_E:
      horiz = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SE:
      horiz = ANCHOR_START;
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_S:
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SW:
      horiz = ANCHOR_END;
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_W:
      horiz = ANCHOR_END;
      break;
    case HANDLE_MOVE_STARTPOINT:
    case HANDLE_MOVE_ENDPOINT:
    case HANDLE_CUSTOM1:
    case HANDLE_CUSTOM2:
    case HANDLE_CUSTOM3:
    case HANDLE_CUSTOM4:
    case HANDLE_CUSTOM5:
    case HANDLE_CUSTOM6:
    case HANDLE_CUSTOM7:
    case HANDLE_CUSTOM8:
    case HANDLE_CUSTOM9:
    default:
      break;
  }
  diamond_update_data (diamond, horiz, vert);

  if (width != diamond->element.width && height != diamond->element.height) {
    return element_change_new (&corner, width, height, &diamond->element);
  }

  return NULL;
}


static DiaObjectChange*
diamond_move (Diamond *diamond, Point *to)
{
  diamond->element.corner = *to;

  diamond_update_data (diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
diamond_draw (Diamond *diamond, DiaRenderer *renderer)
{
  Point pts[4];
  Element *elem;

  g_return_if_fail (diamond != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &diamond->element;

  pts[0] = pts[1] = pts[2] = pts[3] = elem->corner;
  pts[0].x += elem->width / 2.0;
  pts[1].x += elem->width;
  pts[1].y += elem->height / 2.0;
  pts[2].x += elem->width / 2.0;
  pts[2].y += elem->height;
  pts[3].y += elem->height / 2.0;

  if (diamond->show_background) {
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  }

  dia_renderer_set_linewidth (renderer, diamond->border_width);
  dia_renderer_set_linestyle (renderer, diamond->line_style, diamond->dashlength);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_polygon (renderer,
                             pts,
                             4,
                             (diamond->show_background) ? &diamond->inner_color : NULL,
                             &diamond->border_color);

  text_draw (diamond->text, renderer);
}

static void
diamond_update_data(Diamond *diamond, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &diamond->element;
  DiaObject *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;
  Point center, bottom_right;
  Point p;
  real dw, dh;
  real width, height;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox(diamond->text, NULL);
  width = diamond->text->max_width + 2*diamond->padding+diamond->border_width;
  height = diamond->text->height * diamond->text->numlines +
    2 * diamond->padding + diamond->border_width;

  if (diamond->text_fitting == DIA_TEXT_FIT_ALWAYS
      || (   diamond->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED
          && height > (elem->width - width) * elem->height / elem->width)) {
    /* increase size of the diamond while keeping its aspect ratio */
    real grad = elem->width/elem->height;
    if (grad < 1.0/4) grad = 1.0/4;
    if (grad > 4)     grad = 4;
    elem->width  = width  + height * grad;
    elem->height = height + width  / grad;
  } else {
    /* Need this for text alignment soon */
    real grad = elem->width/elem->height;
    if (grad < 1.0/4) grad = 1.0/4;
    if (grad > 4)     grad = 4;
    width = elem->width - height*grad;
  }

  /* move shape if necessary ... */
  switch (horiz) {
    case ANCHOR_MIDDLE:
      elem->corner.x = center.x - elem->width/2;
      break;
    case ANCHOR_END:
      elem->corner.x = bottom_right.x - elem->width;
      break;
    case ANCHOR_START:
    default:
      break;
  }

  switch (vert) {
    case ANCHOR_MIDDLE:
      elem->corner.y = center.y - elem->height/2;
      break;
    case ANCHOR_END:
      elem->corner.y = bottom_right.y - elem->height;
      break;
    case ANCHOR_START:
    default:
      break;
  }

  p = elem->corner;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 - diamond->text->height*diamond->text->numlines/2 +
      diamond->text->ascent;
  switch (diamond->text->alignment) {
    case DIA_ALIGN_LEFT:
      p.x -= width / 2;
      break;
    case DIA_ALIGN_RIGHT:
      p.x += width / 2;
      break;
    case DIA_ALIGN_CENTRE:
    default:
      break;
  }
  text_set_position (diamond->text, &p);

  dw = elem->width / 8.0;
  dh = elem->height / 8.0;
  /* Update connections: */
  diamond->connections[0].pos.x = elem->corner.x + 4*dw;
  diamond->connections[0].pos.y = elem->corner.y;
  diamond->connections[1].pos.x = elem->corner.x + 5*dw;
  diamond->connections[1].pos.y = elem->corner.y + dh;
  diamond->connections[2].pos.x = elem->corner.x + 6*dw;
  diamond->connections[2].pos.y = elem->corner.y + 2*dh;
  diamond->connections[3].pos.x = elem->corner.x + 7*dw;
  diamond->connections[3].pos.y = elem->corner.y + 3*dh;
  diamond->connections[4].pos.x = elem->corner.x + elem->width;
  diamond->connections[4].pos.y = elem->corner.y + 4*dh;
  diamond->connections[5].pos.x = elem->corner.x + 7*dw;
  diamond->connections[5].pos.y = elem->corner.y + 5*dh;
  diamond->connections[6].pos.x = elem->corner.x + 6*dw;
  diamond->connections[6].pos.y = elem->corner.y + 6*dh;
  diamond->connections[7].pos.x = elem->corner.x + 5*dw;
  diamond->connections[7].pos.y = elem->corner.y + 7*dh;
  diamond->connections[8].pos.x = elem->corner.x + 4*dw;
  diamond->connections[8].pos.y = elem->corner.y + elem->height;
  diamond->connections[9].pos.x = elem->corner.x + 3*dw;
  diamond->connections[9].pos.y = elem->corner.y + 7*dh;
  diamond->connections[10].pos.x = elem->corner.x + 2*dw;
  diamond->connections[10].pos.y = elem->corner.y + 6*dh;
  diamond->connections[11].pos.x = elem->corner.x + dw;
  diamond->connections[11].pos.y = elem->corner.y + 5*dh;
  diamond->connections[12].pos.x = elem->corner.x;
  diamond->connections[12].pos.y = elem->corner.y + 4*dh;
  diamond->connections[13].pos.x = elem->corner.x + dw;
  diamond->connections[13].pos.y = elem->corner.y + 3*dh;
  diamond->connections[14].pos.x = elem->corner.x + 2*dw;
  diamond->connections[14].pos.y = elem->corner.y + 2*dh;
  diamond->connections[15].pos.x = elem->corner.x + 3*dw;
  diamond->connections[15].pos.y = elem->corner.y + dh;
  diamond->connections[16].pos.x = elem->corner.x + 4*dw;
  diamond->connections[16].pos.y = elem->corner.y + 4*dh;

  /* help autorouting */
  element_update_connections_directions (elem, diamond->connections);

  extra->border_trans = diamond->border_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
diamond_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Diamond *diamond;
  Element *elem;
  DiaObject *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  diamond = g_new0 (Diamond, 1);
  elem = &diamond->element;
  obj = &elem->object;

  obj->type = &diamond_type;

  obj->ops = &diamond_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  diamond->border_width =  attributes_get_default_linewidth();
  diamond->border_color = attributes_get_foreground();
  diamond->inner_color = attributes_get_background();
  diamond->show_background = default_properties.show_background;
  attributes_get_default_line_style(&diamond->line_style,&diamond->dashlength);

  diamond->padding = default_properties.padding;

  attributes_get_default_font(&font, &font_height);
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + font_height / 2;
  diamond->text = new_text ("",
                            font,
                            font_height,
                            &p,
                            &diamond->border_color,
                            DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  /* new default: let the user decide the size */
  diamond->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &diamond->connections[i];
    diamond->connections[i].object = obj;
    diamond->connections[i].connected = NULL;
    diamond->connections[i].flags = 0;
  }
  diamond->connections[16].flags = CP_FLAGS_MAIN;

  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &diamond->element.object;
}

static void
diamond_destroy(Diamond *diamond)
{
  text_destroy(diamond->text);

  element_destroy(&diamond->element);
}


static void
diamond_save(Diamond *diamond, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&diamond->element, obj_node, ctx);

  if (diamond->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  diamond->border_width, ctx);

  if (!color_equals(&diamond->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &diamond->border_color, ctx);

  if (!color_equals(&diamond->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &diamond->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
		   diamond->show_background, ctx);

  if (diamond->line_style != DIA_LINE_STYLE_SOLID) {
    data_add_enum (new_attribute (obj_node, "line_style"),
                   diamond->line_style,
                   ctx);
  }

  if (diamond->line_style != DIA_LINE_STYLE_SOLID &&
      diamond->dashlength != DEFAULT_LINESTYLE_DASHLEN) {
    data_add_real (new_attribute (obj_node, "dashlength"),
                   diamond->dashlength,
                   ctx);
  }

  data_add_real(new_attribute(obj_node, "padding"), diamond->padding, ctx);

  data_add_text(new_attribute(obj_node, "text"), diamond->text, ctx);

  if (diamond->text_fitting != DIA_TEXT_FIT_WHEN_NEEDED) {
    data_add_enum (new_attribute (obj_node, PROP_STDNAME_TEXT_FITTING),
                   diamond->text_fitting,
                   ctx);
  }
}

static DiaObject *
diamond_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Diamond *diamond;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  diamond = g_new0 (Diamond, 1);
  elem = &diamond->element;
  obj = &elem->object;

  obj->type = &diamond_type;
  obj->ops = &diamond_ops;

  element_load(elem, obj_node, ctx);

  diamond->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    diamond->border_width =  data_real(attribute_first_data(attr), ctx);

  diamond->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &diamond->border_color, ctx);

  diamond->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &diamond->inner_color, ctx);

  diamond->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    diamond->show_background = data_boolean(attribute_first_data(attr), ctx);

  diamond->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    diamond->line_style =  data_enum(attribute_first_data(attr), ctx);

  diamond->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    diamond->dashlength = data_real(attribute_first_data(attr), ctx);

  diamond->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    diamond->padding =  data_real(attribute_first_data(attr), ctx);

  diamond->text = NULL;
  attr = object_find_attribute (obj_node, "text");
  if (attr != NULL) {
    diamond->text = data_text (attribute_first_data (attr), ctx);
  } else {
    /* paranoid */
    diamond->text = new_text_default (&obj->position,
                                      &diamond->border_color,
                                      DIA_ALIGN_CENTRE);
  }

  /* old default: only growth, manual shrink */
  diamond->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;
  attr = object_find_attribute (obj_node, PROP_STDNAME_TEXT_FITTING);
  if (attr != NULL) {
    diamond->text_fitting = data_enum (attribute_first_data (attr), ctx);
  }

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &diamond->connections[i];
    diamond->connections[i].object = obj;
    diamond->connections[i].connected = NULL;
    diamond->connections[i].flags = 0;
  }
  diamond->connections[16].flags = CP_FLAGS_MAIN;

  diamond_update_data(diamond, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &diamond->element.object;
}
