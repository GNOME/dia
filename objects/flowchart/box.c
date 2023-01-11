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

#include "pixmaps/box.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

#define NUM_CONNECTIONS 17

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

typedef struct _Box Box;

struct _Box {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];
  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  DiaLineStyle line_style;
  double dashlength;
  double corner_radius;

  Text *text;
  double padding;

  DiaTextFitting text_fitting;
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
static DiaObjectChange *box_move_handle         (Box              *box,
                                                 Handle           *handle,
                                                 Point            *to,
                                                 ConnectionPoint  *cp,
                                                 HandleMoveReason  reason,
                                                 ModifierKeys      modifiers);
static DiaObjectChange *box_move                (Box              *box,
                                                 Point            *to);
static void box_draw(Box *box, DiaRenderer *renderer);
static void box_update_data(Box *box, AnchorShape horix, AnchorShape vert);
static DiaObject *box_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void box_destroy(Box *box);
static PropDescription *box_describe_props(Box *box);
static void box_get_props(Box *box, GPtrArray *props);
static void box_set_props(Box *box, GPtrArray *props);

static void box_save(Box *box, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *box_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps box_type_ops =
{
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType fc_box_type =
{
  "Flowchart - Box", /* name */
  0,              /* version */
  box_xpm,         /* pixmap */
  &box_type_ops       /* ops */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   box_describe_props,
  (GetPropsFunc)        box_get_props,
  (SetPropsFunc)        box_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
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
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FITTING,
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
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Box, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Box, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Box, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Box, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Box, line_style), offsetof(Box, dashlength) },
  { "corner_radius", PROP_TYPE_REAL, offsetof(Box, corner_radius) },
  { "padding", PROP_TYPE_REAL, offsetof(Box, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Box,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Box,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Box,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Box,text),offsetof(Text,color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Box,text),offsetof(Text,alignment)},
  {PROP_STDNAME_TEXT_FITTING,PROP_TYPE_ENUM,offsetof(Box,text_fitting)},
  { NULL, 0, 0 },
};


static void
box_get_props(Box *box, GPtrArray *props)
{
  object_get_props_from_offsets(&box->element.object,
                                box_offsets,props);
}


static void
box_set_props (Box *box, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (box),
                                 box_offsets,props);
  box_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
init_default_values (void)
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.padding = 0.5;
    defaults_initialized = 1;
  }
}


static real
box_distance_from (Box *box, Point *point)
{
  Element *elem = &box->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - box->border_width/2;
  rect.right = elem->corner.x + elem->width + box->border_width/2;
  rect.top = elem->corner.y - box->border_width/2;
  rect.bottom = elem->corner.y + elem->height + box->border_width/2;

  return distance_rectangle_point (&rect, point);
}


static void
box_select (Box         *box,
            Point       *clicked_point,
            DiaRenderer *interactive_renderer)
{
  real radius;

  text_set_cursor (box->text, clicked_point, interactive_renderer);
  text_grab_focus (box->text, &box->element.object);

  element_update_handles (&box->element);

  if (box->corner_radius > 0) {
    Element *elem = (Element *) box;
    radius = box->corner_radius;
    radius = MIN (radius, elem->width/2);
    radius = MIN (radius, elem->height/2);
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


static DiaObjectChange *
box_move_handle (Box              *box,
                 Handle           *handle,
                 Point            *to,
                 ConnectionPoint  *cp,
                 HandleMoveReason  reason,
                 ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  Point corner;
  double width, height;

  g_return_val_if_fail (box != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  /* remember ... */
  corner = box->element.corner;
  width = box->element.width;
  height = box->element.height;

  element_move_handle (&box->element, handle->id, to, cp, reason, modifiers);

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
  box_update_data (box, horiz, vert);

  if (width != box->element.width && height != box->element.height) {
    return element_change_new (&corner, width, height, &box->element);
  }

  return NULL;
}


static DiaObjectChange *
box_move (Box *box, Point *to)
{
  box->element.corner = *to;

  box_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
box_draw (Box *box, DiaRenderer *renderer)
{
  Point lr_corner;
  Element *elem;

  g_return_if_fail (box != NULL);
  g_return_if_fail (renderer != NULL);
  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  if (box->show_background) {
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  }

  dia_renderer_set_linewidth (renderer, box->border_width);
  dia_renderer_set_linestyle (renderer, box->line_style, box->dashlength);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  /* Problem:  How do we make the fill with rounded corners?
   * It's solved in the base class ...
   */
  dia_renderer_draw_rounded_rect (renderer,
                                  &elem->corner,
                                  &lr_corner,
                                  &box->inner_color,
                                  &box->border_color,
                                  box->corner_radius);
  text_draw(box->text, renderer);
}

static void
box_update_data(Box *box, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &box->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
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
   *  set the width to the minimum.  Or else;)
   */
  if (box->text_fitting != DIA_TEXT_FIT_NEVER) {
    if (box->text_fitting == DIA_TEXT_FIT_ALWAYS || width > elem->width) {
      elem->width = width;
    }

    if (box->text_fitting == DIA_TEXT_FIT_ALWAYS || height > elem->height) {
      elem->height = height;
    }
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
  p.y += elem->height / 2.0 - box->text->height * box->text->numlines / 2 +
    box->text->ascent;

  switch (box->text->alignment) {
    case DIA_ALIGN_LEFT:
      p.x -= (elem->width - box->padding * 2 + box->border_width) / 2;
      break;
    case DIA_ALIGN_RIGHT:
      p.x += (elem->width - box->padding * 2 + box->border_width) / 2;
      break;
    case DIA_ALIGN_CENTRE:
    default:
      break;
  }

  text_set_position (box->text, &p);

  radius = box->corner_radius;
  radius = MIN(radius, elem->width/2);
  radius = MIN(radius, elem->height/2);
  radius *= (1-M_SQRT1_2);

  /* Update connections: */
  connpoint_update(&box->connections[0],
		    elem->corner.x + radius,
		    elem->corner.y + radius,
		    DIR_NORTHWEST);
  connpoint_update(&box->connections[1],
		    elem->corner.x + elem->width / 4.0,
		    elem->corner.y,
		    DIR_NORTH);
  connpoint_update(&box->connections[2],
		    elem->corner.x + elem->width / 2.0,
		    elem->corner.y,
		    DIR_NORTH);
  connpoint_update(&box->connections[3],
		    elem->corner.x + elem->width * 3.0 / 4.0,
		    elem->corner.y,
		    DIR_NORTH);
  connpoint_update(&box->connections[4],
		    elem->corner.x + elem->width - radius,
		    elem->corner.y + radius,
		    DIR_NORTHEAST);
  connpoint_update(&box->connections[5],
		    elem->corner.x,
		    elem->corner.y + elem->height / 4.0,
		    DIR_WEST);
  connpoint_update(&box->connections[6],
		    elem->corner.x + elem->width,
		    elem->corner.y + elem->height / 4.0,
		    DIR_EAST);
  connpoint_update(&box->connections[7],
		    elem->corner.x,
		    elem->corner.y + elem->height / 2.0,
		    DIR_WEST);
  connpoint_update(&box->connections[8],
		    elem->corner.x + elem->width,
		    elem->corner.y + elem->height / 2.0,
		    DIR_EAST);
  connpoint_update(&box->connections[9],
		    elem->corner.x,
		    elem->corner.y + elem->height * 3.0 / 4.0,
		    DIR_WEST);
  connpoint_update(&box->connections[10],
		    elem->corner.x + elem->width,
		    elem->corner.y + elem->height * 3.0 / 4.0,
		    DIR_EAST);
  connpoint_update(&box->connections[11],
		    elem->corner.x + radius,
		    elem->corner.y + elem->height - radius,
		    DIR_SOUTHWEST);
  connpoint_update(&box->connections[12],
		    elem->corner.x + elem->width / 4.0,
		    elem->corner.y + elem->height,
		    DIR_SOUTH);
  connpoint_update(&box->connections[13],
		    elem->corner.x + elem->width / 2.0,
		    elem->corner.y + elem->height,
		    DIR_SOUTH);
  connpoint_update(&box->connections[14],
		    elem->corner.x + elem->width * 3.0 / 4.0,
		    elem->corner.y + elem->height,
		    DIR_SOUTH);
  connpoint_update(&box->connections[15],
		    elem->corner.x + elem->width - radius,
		    elem->corner.y + elem->height - radius,
		    DIR_SOUTHEAST);
  connpoint_update(&box->connections[16],
		    elem->corner.x + elem->width / 2,
		    elem->corner.y + elem->height / 2,
		    DIR_ALL);

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

static DiaObject *
box_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  box = g_new0 (Box, 1);
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
  box->text = new_text ("",
                        font,
                        font_height,
                        &p,
                        &box->border_color,
                        DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  /* new default: let the user decide the size */
  box->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
    box->connections[i].flags = 0;
  }
  box->connections[16].flags = CP_FLAGS_MAIN;

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
box_save(Box *box, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&box->element, obj_node, ctx);

  if (box->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  box->border_width, ctx);

  if (!color_equals(&box->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &box->border_color, ctx);

  if (!color_equals(&box->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &box->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
                   box->show_background, ctx);

  if (box->line_style != DIA_LINE_STYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  box->line_style, ctx);

  if (box->line_style != DIA_LINE_STYLE_SOLID &&
      box->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  box->dashlength, ctx);
  if (box->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
		  box->corner_radius, ctx);

  data_add_real(new_attribute(obj_node, "padding"), box->padding, ctx);

  data_add_text(new_attribute(obj_node, "text"), box->text, ctx);

  if (box->text_fitting != DIA_TEXT_FIT_WHEN_NEEDED) {
    data_add_enum (new_attribute (obj_node, PROP_STDNAME_TEXT_FITTING),
                   box->text_fitting,
                   ctx);
  }
}


static DiaObject *
box_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  box = g_new0 (Box, 1);
  elem = &box->element;
  obj = &elem->object;

  obj->type = &fc_box_type;
  obj->ops = &box_ops;

  element_load(elem, obj_node, ctx);

  box->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    box->border_width =  data_real(attribute_first_data(attr), ctx);
  box->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->border_color, ctx);

  box->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->inner_color, ctx);

  box->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    box->show_background = data_boolean(attribute_first_data(attr), ctx);

  box->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum(attribute_first_data(attr), ctx);

  box->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    box->dashlength = data_real(attribute_first_data(attr), ctx);

  box->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    box->corner_radius =  data_real(attribute_first_data(attr), ctx);

  box->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    box->padding =  data_real(attribute_first_data(attr), ctx);

  box->text = NULL;
  attr = object_find_attribute (obj_node, "text");
  if (attr != NULL) {
    box->text = data_text (attribute_first_data (attr), ctx);
  } else {
    /* paranoid */
    box->text = new_text_default (&obj->position,
                                  &box->border_color,
                                  DIA_ALIGN_CENTRE);
  }

  /* old default: only growth, manual shrink */
  box->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;
  attr = object_find_attribute (obj_node, PROP_STDNAME_TEXT_FITTING);
  if (attr != NULL) {
    box->text_fitting = data_enum (attribute_first_data (attr), ctx);
  }

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
    box->connections[i].flags = 0;
  }
  box->connections[16].flags = CP_FLAGS_MAIN;

  box_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &box->element.object;
}
