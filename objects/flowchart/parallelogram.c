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

#include "pixmaps/pgram.xpm"

/* used when resizing to decide which side of the shape to expand/shrink */
typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

#define NUM_CONNECTIONS 17

typedef struct _Pgram Pgram;

struct _Pgram {
  Element element;

  ConnectionPoint connections[NUM_CONNECTIONS];
  double border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  DiaLineStyle line_style;
  double dashlength;
  double shear_angle, shear_grad;

  Text *text;
  double padding;

  DiaTextFitting text_fitting;
};

typedef struct _PgramProperties {
  gboolean show_background;
  real shear_angle;
  real padding;
} PgramProperties;

static PgramProperties default_properties;

static real pgram_distance_from(Pgram *pgram, Point *point);
static void pgram_select(Pgram *pgram, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange *pgram_move_handle      (Pgram            *pgram,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange *pgram_move             (Pgram            *pgram,
                                                Point            *to);
static void pgram_draw(Pgram *pgram, DiaRenderer *renderer);
static void pgram_update_data(Pgram *pgram, AnchorShape h, AnchorShape v);
static DiaObject *pgram_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void pgram_destroy(Pgram *pgram);

static PropDescription *pgram_describe_props(Pgram *pgram);
static void pgram_get_props(Pgram *pgram, GPtrArray *props);
static void pgram_set_props(Pgram *pgram, GPtrArray *props);

static void pgram_save(Pgram *pgram, ObjectNode obj_node, DiaContext *ctx);
static DiaObject *pgram_load(ObjectNode obj_node, int version,DiaContext *ctx);

static ObjectTypeOps pgram_type_ops =
{
  (CreateFunc) pgram_create,
  (LoadFunc)   pgram_load,
  (SaveFunc)   pgram_save,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType pgram_type =
{
  "Flowchart - Parallelogram", /* name */
  0,                        /* version */
  pgram_xpm,                 /* pixmap */
  &pgram_type_ops               /* ops */
};

static ObjectOps pgram_ops = {
  (DestroyFunc)         pgram_destroy,
  (DrawFunc)            pgram_draw,
  (DistanceFunc)        pgram_distance_from,
  (SelectFunc)          pgram_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            pgram_move,
  (MoveHandleFunc)      pgram_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      NULL,
  (DescribePropsFunc)   pgram_describe_props,
  (GetPropsFunc)        pgram_get_props,
  (SetPropsFunc)        pgram_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };
static PropNumData shear_angle_data = { 45.0, 135.0, 1.0 };

static PropDescription pgram_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  PROP_STD_LINE_WIDTH,
  PROP_STD_LINE_COLOUR,
  PROP_STD_FILL_COLOUR,
  PROP_STD_SHOW_BACKGROUND,
  PROP_STD_LINE_STYLE,
  { "shear_angle", PROP_TYPE_REAL, PROP_FLAG_VISIBLE,
    N_("Shear angle"), NULL, &shear_angle_data },
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
pgram_describe_props(Pgram *pgram)
{
  if (pgram_props[0].quark == 0)
    prop_desc_list_calculate_quarks(pgram_props);
  return pgram_props;
}

static PropOffset pgram_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { PROP_STDNAME_LINE_WIDTH, PROP_STDTYPE_LINE_WIDTH, offsetof(Pgram, border_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Pgram, border_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Pgram, inner_color) },
  { "show_background", PROP_TYPE_BOOL, offsetof(Pgram, show_background) },
  { "line_style", PROP_TYPE_LINESTYLE,
    offsetof(Pgram, line_style), offsetof(Pgram, dashlength) },
  { "shear_angle", PROP_TYPE_REAL, offsetof(Pgram, shear_angle) },
  { "padding", PROP_TYPE_REAL, offsetof(Pgram, padding) },
  {"text",PROP_TYPE_TEXT,offsetof(Pgram,text)},
  {"text_font",PROP_TYPE_FONT,offsetof(Pgram,text),offsetof(Text,font)},
  {PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Pgram,text),offsetof(Text,height)},
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Pgram,text),offsetof(Text,color)},
  {"text_alignment",PROP_TYPE_ENUM,offsetof(Pgram,text),offsetof(Text,alignment)},
  {PROP_STDNAME_TEXT_FITTING,PROP_TYPE_ENUM,offsetof(Pgram,text_fitting)},
  { NULL, 0, 0 },
};


static void
pgram_get_props (Pgram *pgram, GPtrArray *props)
{
  object_get_props_from_offsets (DIA_OBJECT (pgram),
                                 pgram_offsets,props);
}


static void
pgram_set_props (Pgram *pgram, GPtrArray *props)
{
  object_set_props_from_offsets (DIA_OBJECT (pgram),
                                 pgram_offsets, props);
  pgram_update_data (pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}


static void
init_default_values (void)
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    default_properties.shear_angle = 70.0;
    default_properties.padding = 0.5;
    defaults_initialized = 1;
  }
}

static real
pgram_distance_from (Pgram *pgram, Point *point)
{
  Element *elem = &pgram->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - pgram->border_width/2;
  rect.right = elem->corner.x + elem->width + pgram->border_width/2;
  rect.top = elem->corner.y - pgram->border_width/2;
  rect.bottom = elem->corner.y + elem->height + pgram->border_width/2;

  /* we do some fiddling with the left/right values to get good accuracy
   * without having to write a new distance checking routine */
  if (rect.top > point->y) {
    /* point above parallelogram */
    if (pgram->shear_grad > 0)
      rect.left  += pgram->shear_grad * (rect.bottom - rect.top);
    else
      rect.right += pgram->shear_grad * (rect.bottom - rect.top);
  } else if (rect.bottom < point->y) {
    /* point below parallelogram */
    if (pgram->shear_grad > 0)
      rect.right -= pgram->shear_grad * (rect.bottom - rect.top);
    else
      rect.left  -= pgram->shear_grad * (rect.bottom - rect.top);
  } else {
    /* point withing vertical interval of parallelogram -- modify
     * left and right sides to `unshear' the parallelogram.  This
     * increases accuracy for points near the  */
    if (pgram->shear_grad > 0) {
      rect.left  += pgram->shear_grad * (rect.bottom - point->y);
      rect.right -= pgram->shear_grad * (point->y - rect.top);
    } else {
      rect.left  -= pgram->shear_grad * (point->y - rect.top);
      rect.right += pgram->shear_grad * (rect.bottom - point->y);
    }
  }

  return distance_rectangle_point (&rect, point);
}

static void
pgram_select(Pgram *pgram, Point *clicked_point,
	   DiaRenderer *interactive_renderer)
{
  text_set_cursor(pgram->text, clicked_point, interactive_renderer);
  text_grab_focus(pgram->text, &pgram->element.object);

  element_update_handles(&pgram->element);
}


static DiaObjectChange *
pgram_move_handle (Pgram            *pgram,
                   Handle           *handle,
                   Point            *to,
                   ConnectionPoint  *cp,
                   HandleMoveReason  reason,
                   ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;
  Point corner;
  real width, height;

  g_return_val_if_fail (pgram != NULL, NULL);
  g_return_val_if_fail (handle!=NULL, NULL);
  g_return_val_if_fail (to!=NULL, NULL);

  /* remember ... */
  corner = pgram->element.corner;
  width = pgram->element.width;
  height = pgram->element.height;

  element_move_handle (&pgram->element, handle->id, to, cp, reason, modifiers);

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
  pgram_update_data (pgram, horiz, vert);

  if (width != pgram->element.width && height != pgram->element.height) {
    return element_change_new (&corner, width, height, &pgram->element);
  }

  return NULL;
}


static DiaObjectChange *
pgram_move (Pgram *pgram, Point *to)
{
  pgram->element.corner = *to;

  pgram_update_data (pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
pgram_draw (Pgram *pgram, DiaRenderer *renderer)
{
  Point pts[4];
  Element *elem;
  real offs;

  g_return_if_fail (pgram != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &pgram->element;

  pts[0] = pts[1] = pts[2] = pts[3] = elem->corner;
  pts[1].x += elem->width;
  pts[2].x += elem->width;
  pts[2].y += elem->height;
  pts[3].y += elem->height;

  offs = elem->height * pgram->shear_grad;
  if (offs > 0) {
    pts[0].x += offs;
    pts[2].x -= offs;
  } else {
    pts[1].x += offs;
    pts[3].x -= offs;
  }

  if (pgram->show_background) {
    dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  }

  dia_renderer_set_linewidth (renderer, pgram->border_width);
  dia_renderer_set_linestyle (renderer, pgram->line_style, pgram->dashlength);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_polygon (renderer,
                             pts,
                             4,
                             (pgram->show_background) ? &pgram->inner_color : NULL,
                             &pgram->border_color);

  text_draw (pgram->text, renderer);
}


static void
pgram_update_data(Pgram *pgram, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &pgram->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  real offs;
  real width, height;
  real avail_width;
  real top_left;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  /* this takes the shearing of the parallelogram into account, so the
   * text can be extend to the edges of the parallelogram */
  text_calc_boundingbox(pgram->text, NULL);
  height = pgram->text->height * pgram->text->numlines + pgram->padding*2 +
    pgram->border_width;
  if (   pgram->text_fitting == DIA_TEXT_FIT_ALWAYS
      || (pgram->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED
          && height > elem->height))
    elem->height = height;

  avail_width = elem->width - (pgram->padding*2 + pgram->border_width +
    fabs(pgram->shear_grad) * (elem->height + pgram->text->height
			       * pgram->text->numlines));
  if (   pgram->text_fitting == DIA_TEXT_FIT_ALWAYS
      || (pgram->text_fitting == DIA_TEXT_FIT_WHEN_NEEDED
          && avail_width < pgram->text->max_width)) {
    elem->width = (elem->width-avail_width) + pgram->text->max_width;
    avail_width = pgram->text->max_width;
  }

  /*
  width = pgram->text->max_width + pgram->padding*2 + pgram->border_width +
    fabs(pgram->shear_grad) * (elem->height + pgram->text->height
			       * pgram->text->numlines);
  if (width > elem->width) elem->width = width;
  */

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
  p.y += elem->height / 2.0 - pgram->text->height * pgram->text->numlines / 2 +
      pgram->text->ascent;
  switch (pgram->text->alignment) {
    case DIA_ALIGN_LEFT:
      p.x -= avail_width / 2;
      break;
    case DIA_ALIGN_RIGHT:
      p.x += avail_width / 2;
      break;
    case DIA_ALIGN_CENTRE:
      break;
    default:
      g_return_if_reached ();
  }

  text_set_position (pgram->text, &p);

  /* 1/4 of how much more to the left the bottom line is */
  offs = -(elem->height / 4.0 * pgram->shear_grad);
  width = elem->width - 4.0*fabs(offs);
  top_left = elem->corner.x;
  if (offs < 0.0) {
    top_left -= 4*offs;
  }

  /* Update connections: */
  connpoint_update(&pgram->connections[0],
		   top_left,
		   elem->corner.y,
		   DIR_NORTHWEST);
  connpoint_update(&pgram->connections[1],
		   top_left + width / 4.0,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&pgram->connections[2],
		   top_left + width / 2.0,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&pgram->connections[3],
		   top_left + width * 3.0 / 4.0,
		   elem->corner.y,
		   DIR_NORTH);
  connpoint_update(&pgram->connections[4],
		   top_left + width,
		   elem->corner.y,
		   DIR_NORTHEAST);
  connpoint_update(&pgram->connections[5],
		   top_left + offs,
		   elem->corner.y + elem->height / 4.0,
		   DIR_WEST);
  connpoint_update(&pgram->connections[6],
		   top_left + width + offs,
		   elem->corner.y + elem->height / 4.0,
		   DIR_EAST);
  connpoint_update(&pgram->connections[7],
		   top_left + 2.0 * offs,
		   elem->corner.y + elem->height / 2.0,
		   DIR_WEST);
  connpoint_update(&pgram->connections[8],
		   top_left + width + 2.0 * offs,
		   elem->corner.y + elem->height / 2.0,
		   DIR_EAST);
  connpoint_update(&pgram->connections[9],
		   top_left + 3.0 * offs,
		   elem->corner.y + elem->height * 3.0 / 4.0,
		   DIR_WEST);
  connpoint_update(&pgram->connections[10],
		   top_left + width + 3.0 * offs,
		   elem->corner.y + elem->height * 3.0 / 4.0,
		   DIR_EAST);
  connpoint_update(&pgram->connections[11],
		   top_left + 4.0 * offs,
		   elem->corner.y + elem->height,
		   DIR_SOUTHWEST);
  connpoint_update(&pgram->connections[12],
		   top_left + 4.0 * offs + width / 4.0,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&pgram->connections[13],
		   top_left + 4.0 * offs + width / 2.0,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&pgram->connections[14],
		   top_left + 4.0 * offs + 3.0 * width / 4.0,
		   elem->corner.y + elem->height,
		   DIR_SOUTH);
  connpoint_update(&pgram->connections[15],
		   top_left + 4.0 * offs + width,
		   elem->corner.y + elem->height,
		   DIR_SOUTHEAST);
  connpoint_update(&pgram->connections[16],
		   top_left + 2.0 * offs + width / 2,
		   elem->corner.y + elem->height / 2,
		   DIR_ALL);

  extra->border_trans = pgram->border_width/2.0;
  element_update_boundingbox(elem);

  obj->position = elem->corner;

  element_update_handles(elem);
}

static DiaObject *
pgram_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Pgram *pgram;
  Element *elem;
  DiaObject *obj;
  Point p;
  int i;
  DiaFont *font = NULL;
  real font_height;

  init_default_values();

  pgram = g_new0 (Pgram, 1);
  elem = &pgram->element;
  obj = &elem->object;

  obj->type = &pgram_type;

  obj->ops = &pgram_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  pgram->border_width =  attributes_get_default_linewidth();
  pgram->border_color = attributes_get_foreground();
  pgram->inner_color = attributes_get_background();
  pgram->show_background = default_properties.show_background;
  attributes_get_default_line_style(&pgram->line_style, &pgram->dashlength);
  pgram->shear_angle = default_properties.shear_angle;
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);

  pgram->padding = default_properties.padding;

  attributes_get_default_font(&font, &font_height);
  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + font_height / 2;
  pgram->text = new_text ("",
                          font,
                          font_height,
                          &p,
                          &pgram->border_color,
                          DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  /* new default: let the user decide the size */
  pgram->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &pgram->connections[i];
    pgram->connections[i].object = obj;
    pgram->connections[i].connected = NULL;
    pgram->connections[i].flags = 0;
  }
  pgram->connections[16].flags = CP_FLAGS_MAIN;

  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &pgram->element.object;
}

static void
pgram_destroy(Pgram *pgram)
{
  text_destroy(pgram->text);

  element_destroy(&pgram->element);
}

static void
pgram_save(Pgram *pgram, ObjectNode obj_node, DiaContext *ctx)
{
  element_save(&pgram->element, obj_node, ctx);

  if (pgram->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  pgram->border_width, ctx);

  if (!color_equals(&pgram->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &pgram->border_color, ctx);

  if (!color_equals(&pgram->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &pgram->inner_color, ctx);

  data_add_boolean(new_attribute(obj_node, "show_background"),
		   pgram->show_background, ctx);

  if (pgram->line_style != DIA_LINE_STYLE_SOLID) {
    data_add_enum (new_attribute (obj_node, "line_style"),
                   pgram->line_style,
                   ctx);
  }

  if (pgram->line_style != DIA_LINE_STYLE_SOLID &&
      pgram->dashlength != DEFAULT_LINESTYLE_DASHLEN) {
    data_add_real (new_attribute (obj_node, "dashlength"),
                   pgram->dashlength,
                   ctx);
  }

  data_add_real(new_attribute(obj_node, "shear_angle"),
		pgram->shear_angle, ctx);

  data_add_real(new_attribute(obj_node, "padding"), pgram->padding, ctx);

  data_add_text (new_attribute (obj_node, "text"), pgram->text, ctx);
  if (pgram->text_fitting != DIA_TEXT_FIT_WHEN_NEEDED) {
    data_add_enum (new_attribute (obj_node, PROP_STDNAME_TEXT_FITTING),
                   pgram->text_fitting,
                   ctx);
  }
}

static DiaObject *
pgram_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  Pgram *pgram;
  Element *elem;
  DiaObject *obj;
  int i;
  AttributeNode attr;

  pgram = g_new0 (Pgram, 1);
  elem = &pgram->element;
  obj = &elem->object;

  obj->type = &pgram_type;
  obj->ops = &pgram_ops;

  element_load(elem, obj_node, ctx);

  pgram->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    pgram->border_width =  data_real(attribute_first_data(attr), ctx);

  pgram->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &pgram->border_color, ctx);

  pgram->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &pgram->inner_color, ctx);

  pgram->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    pgram->show_background = data_boolean(attribute_first_data(attr), ctx);

  pgram->line_style = DIA_LINE_STYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    pgram->line_style =  data_enum(attribute_first_data(attr), ctx);

  pgram->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    pgram->dashlength = data_real(attribute_first_data(attr), ctx);

  pgram->shear_angle = 0.0;
  attr = object_find_attribute(obj_node, "shear_angle");
  if (attr != NULL)
    pgram->shear_angle =  data_real(attribute_first_data(attr), ctx);
  pgram->shear_grad = tan(M_PI/2.0 - M_PI/180.0 * pgram->shear_angle);

  pgram->padding = default_properties.padding;
  attr = object_find_attribute(obj_node, "padding");
  if (attr != NULL)
    pgram->padding =  data_real(attribute_first_data(attr), ctx);

  pgram->text = NULL;
  attr = object_find_attribute (obj_node, "text");
  if (attr != NULL) {
    pgram->text = data_text (attribute_first_data (attr), ctx);
  } else {
    /* paranoid */
    pgram->text = new_text_default (&obj->position,
                                    &pgram->border_color,
                                    DIA_ALIGN_CENTRE);
  }

  /* old default: only growth, manual shrink */
  pgram->text_fitting = DIA_TEXT_FIT_WHEN_NEEDED;
  attr = object_find_attribute (obj_node, PROP_STDNAME_TEXT_FITTING);
  if (attr != NULL) {
    pgram->text_fitting = data_enum (attribute_first_data (attr), ctx);
  }

  element_init(elem, 8, NUM_CONNECTIONS);

  for (i=0;i<NUM_CONNECTIONS;i++) {
    obj->connections[i] = &pgram->connections[i];
    pgram->connections[i].object = obj;
    pgram->connections[i].connected = NULL;
    pgram->connections[i].flags = 0;
  }
  pgram->connections[16].flags = CP_FLAGS_MAIN;

  pgram_update_data(pgram, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return &pgram->element.object;
}
