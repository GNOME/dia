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

#include "pixmaps/ellipse.xpm"

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

typedef struct _Ellipse Ellipse;

struct _Ellipse {
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

typedef struct _EllipseProperties {
  gboolean show_background;
  real padding;
} EllipseProperties;

static EllipseProperties default_properties;

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange* ellipse_move_handle    (Ellipse          *ellipse,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange* ellipse_move           (Ellipse          *ellipse,
                                                Point            *to);
static void ellipse_draw(Ellipse *ellipse, DiaRenderer *renderer);
static void ellipse_update_data(Ellipse *ellipse, AnchorShape h,AnchorShape v);
static DiaObject *ellipse_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);

static PropDescription *ellipse_describe_props(Ellipse *ellipse);
static void ellipse_get_props(Ellipse *ellipse, GPtrArray *props);
static void ellipse_set_props(Ellipse *ellipse, GPtrArray *props);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *ellipse_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType fc_ellipse_type =
{
  "Flowchart - Ellipse", /* name */
  0,                  /* version */
  ellipse_xpm,         /* pixmap */
  &ellipse_type_ops       /* ops */
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
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   ellipse_describe_props,
  (GetPropsFunc)        ellipse_get_props,
  (SetPropsFunc)        ellipse_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
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
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FITTING,
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
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof (Ellipse, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof (Ellipse, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof (Ellipse, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof (Ellipse, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof (Ellipse, line_style), offsetof (Ellipse, dashlength) },
  { "padding", PROP_TYPE_REAL, offsetof (Ellipse, padding) },
  { "text", PROP_TYPE_TEXT, offsetof (Ellipse, text) },
  { "text_font", PROP_TYPE_FONT, offsetof (Ellipse, text), offsetof(Text, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT,
    offsetof (Ellipse, text), offsetof (Text, height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof (Ellipse, text), offsetof (Text, color) },
  { "text_alignment", PROP_TYPE_ENUM, offsetof (Ellipse, text), offsetof(Text, alignment) },
  { PROP_STDNAME_TEXT_FITTING, PROP_TYPE_ENUM, offsetof (Ellipse, text_fitting) },
  { NULL, 0, 0 },
};


static void
ellipse_get_props (Ellipse *ellipse, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (ellipse),
                                 ellipse_offsets, props);
}


static void
ellipse_set_props (Ellipse *ellipse, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (ellipse),
                                 ellipse_offsets, props);
  ellipse_update_data (ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
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


/* returns the radius of the ellipse along the ray from the centre of the
 * ellipse to the point (px, py) */
static real
ellipse_radius (Ellipse *ellipse, real px, real py)
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

  if (px <= 0.0 && py <= 0.0) {
    return 0; /* avoid division by zero */
  }

  scale = w2 * h2 / (4*h2*px + 4*w2*py);

  return sqrt ((px + py) * scale);
}


static real
ellipse_distance_from (Ellipse *ellipse, Point *point)
{
  Element *elem = &ellipse->element;
  Point c;

  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height/ 2;

  return distance_ellipse_point (&c, elem->width, elem->height,
                                 ellipse->border_width, point);
}


static void
ellipse_select (Ellipse     *ellipse,
                Point       *clicked_point,
                DiaRenderer *interactive_renderer)
{
  text_set_cursor (ellipse->text, clicked_point, interactive_renderer);
  text_grab_focus (ellipse->text, &ellipse->element.object);

  element_update_handles (&ellipse->element);
}


static DiaObjectChange *
ellipse_move_handle (Ellipse          *ellipse,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  Point corner;
  real width, height;

  g_return_val_if_fail (ellipse != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  /* remember ... */
  corner = ellipse->element.corner;
  width = ellipse->element.width;
  height = ellipse->element.height;

  element_move_handle (&ellipse->element, handle->id, to, cp,
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
  ellipse_update_data (ellipse, horiz, vert);

  if (width != ellipse->element.width || height != ellipse->element.height) {
    return element_change_new (&corner, width, height, &ellipse->element);
  }

  return NULL;
}


static DiaObjectChange *
ellipse_move (Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;

  ellipse_update_data (ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
ellipse_draw (Ellipse *ellipse, DiaRenderer *renderer)
{
  Element *elem;
  Point center;

  g_return_if_fail (ellipse != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->show_background) {
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  }
  dia_renderer_set_linewidth (renderer, ellipse->border_width);
  dia_renderer_set_linestyle (renderer, ellipse->line_style, ellipse->dashlength);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_ellipse (renderer,
                             &center,
                             elem->width,
                             elem->height,
                             (ellipse->show_background) ? &ellipse->inner_color : NULL,
                             &ellipse->border_color);

  text_draw (ellipse->text, renderer);
}


static void
ellipse_update_data(Ellipse *ellipse, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &ellipse->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
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

  if (   (radius1 < radius2 && ellipse->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED)
      /* stop infinite resizing with 5% tolerance obsvered with _test_movement */
      || (fabs(1.0 - radius2 / radius1) > 0.05 && ellipse->text_fitting == DIA_TEXT_FIT_ALWAYS)) {
    /* increase size of the ellipse while keeping its aspect ratio */
    elem->width  *= radius2 / radius1;
    elem->height *= radius2 / radius1;
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
  p.y += elem->height / 2.0 - ellipse->text->height*ellipse->text->numlines/2 +
    ellipse->text->ascent;
  switch (ellipse->text->alignment) {
    case DIA_ALIGN_LEFT:
      p.x -= (elem->width - 2*(ellipse->padding + ellipse->border_width))/2;
      break;
    case DIA_ALIGN_RIGHT:
      p.x += (elem->width - 2*(ellipse->padding + ellipse->border_width))/2;
      break;
    case DIA_ALIGN_CENTRE:
      break;
    default:
      g_return_if_reached ();
  }
  text_set_position (ellipse->text, &p);

  /* Update connections: */
  c.x = elem->corner.x + elem->width / 2;
  c.y = elem->corner.y + elem->height / 2;
  dw = elem->width  / 2.0;
  dh = elem->height / 2.0;
  for (i = 0; i < NUM_CONNECTIONS-1; i++) {
    real theta = M_PI / 8.0 * i;
    real costheta = cos(theta);
    real sintheta = sin(theta);
    connpoint_update(&ellipse->connections[i],
		      c.x + dw * costheta,
		      c.y - dh * sintheta,
		      (costheta > .5?DIR_EAST:(costheta < -.5?DIR_WEST:0))|
		      (sintheta > .5?DIR_NORTH:(sintheta < -.5?DIR_SOUTH:0)));
  }
  connpoint_update(&ellipse->connections[16],
		   c.x, c.y, DIR_ALL);

  extra->border_trans = ellipse->border_width / 2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
ellipse_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  DiaObject *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  ellipse = g_new0 (Ellipse, 1);
  elem = &ellipse->element;
  obj = &elem->object;

  obj->type = &fc_ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

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
  ellipse->text = new_text ("",
                            font,
                            font_height,
                            &p,
                            &ellipse->border_color,
                            DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  /* new default: let the user decide the size */
  ellipse->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
    ellipse->connections[i].flags = 0;
  }
  ellipse->connections[16].flags = CP_FLAGS_MAIN;

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
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&ellipse->element, obj_node, ctx);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width, ctx);

  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color, ctx);

  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
		   ellipse->show_background, ctx);

  if (ellipse->line_style != DIA_LINE_STYLE_SOLID) {
    data_add_enum (new_attribute (obj_node, "line_style"),
                   ellipse->line_style,
                   ctx);
  }

  if (ellipse->line_style != DIA_LINE_STYLE_SOLID &&
      ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN) {
    data_add_real (new_attribute (obj_node, "dashlength"),
                   ellipse->dashlength,
                   ctx);
  }

  data_add_real(new_attribute(obj_node, "padding"), ellipse->padding, ctx);

  data_add_text(new_attribute(obj_node, "text"), ellipse->text, ctx);

  if (ellipse->text_fitting != DIA_TEXT_FIT_WHEN_NEEDED) {
    data_add_enum (new_attribute (obj_node, PROP_STDNAME_TEXT_FITTING),
                   ellipse->text_fitting,
                   ctx);
  }
}

static DiaObject *
ellipse_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Ellipse *ellipse;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  ellipse = g_new0 (Ellipse, 1);
  elem = &ellipse->element;
  obj = &elem->object;

  obj->type = &fc_ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node, ctx);

  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real(attribute_first_data(attr), ctx);

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color, ctx);

  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color, ctx);

  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean( attribute_first_data(attr), ctx);

  ellipse->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum(attribute_first_data(attr), ctx);

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    ellipse->dashlength = data_real(attribute_first_data(attr), ctx);

  ellipse->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    ellipse->padding =  data_real(attribute_first_data(attr), ctx);

  ellipse->text = NULL;
  attr = object_find_attribute (obj_node, "text");
  if (attr != NULL) {
    ellipse->text = data_text (attribute_first_data (attr), ctx);
  } else {
    ellipse->text = new_text_default (&obj->position,
                                      &ellipse->border_color,
                                      DIA_ALIGN_CENTRE);
  }

  /* old default: only growth, manual shrink */
  ellipse->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;
  attr = object_find_attribute (obj_node, PROP_STDNAME_TEXT_FITTING);
  if (attr != NULL) {
    ellipse->text_fitting = data_enum (attribute_first_data (attr), ctx);
  }

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
    ellipse->connections[i].flags = 0;
  }
  ellipse->connections[16].flags = CP_FLAGS_MAIN;

  ellipse_update_data(ellipse, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &ellipse->element.object;
}
