/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SADT Activity/Data box -- objects for drawing SADT diagrams.
 * Copyright (C) 2000, 2001 Cyrille Chepelov
 *
 * Forked from Flowchart toolbox -- objects for drawing flowcharts.
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

#include "config.h"

#include <glib/gi18n-lib.h>

#include <math.h>
#include <string.h>
#include <glib.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "connpoint_line.h"
#include "color.h"
#include "properties.h"

#include "pixmaps/sadtbox.xpm"

#define DEFAULT_WIDTH 7.0
#define DEFAULT_HEIGHT 5.0
#define DEFAULT_BORDER 0.25
#define SADTBOX_LINE_WIDTH 0.10

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;


typedef struct _Box {
  Element element;

  ConnPointLine *north,*south,*east,*west;

  Text *text;
  char *id;
  double padding;

  Color line_color;
  Color fill_color;
} Box;

static real sadtbox_distance_from(Box *box, Point *point);
static void sadtbox_select(Box *box, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange *sadtbox_move_handle          (Box             *box,
                                                      Handle          *handle,
                                                      Point           *to,
                                                      ConnectionPoint *cp,
                                                      HandleMoveReason  reason,
                                                      ModifierKeys      modifiers);
static DiaObjectChange *sadtbox_move                 (Box              *box,
                                                      Point            *to);
static void sadtbox_draw(Box *box, DiaRenderer *renderer);
static void sadtbox_update_data(Box *box, AnchorShape horix, AnchorShape vert);
static DiaObject *sadtbox_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void sadtbox_destroy(Box *box);
static DiaObject *sadtbox_load(ObjectNode obj_node, int version,
                               DiaContext *ctx);
static DiaMenu *sadtbox_get_object_menu(Box *box, Point *clickedpoint);

static PropDescription *sadtbox_describe_props(Box *box);
static void sadtbox_get_props(Box *box, GPtrArray *props);
static void sadtbox_set_props(Box *box, GPtrArray *props);

static ObjectTypeOps sadtbox_type_ops =
{
  (CreateFunc) sadtbox_create,
  (LoadFunc)   sadtbox_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType sadtbox_type =
{
  "SADT - box",    /* name */
  0,            /* version */
  sadtbox_xpm,   /* pixmap */
  &sadtbox_type_ops /* ops */
};

static ObjectOps sadtbox_ops = {
  (DestroyFunc)         sadtbox_destroy,
  (DrawFunc)            sadtbox_draw,
  (DistanceFunc)        sadtbox_distance_from,
  (SelectFunc)          sadtbox_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            sadtbox_move,
  (MoveHandleFunc)      sadtbox_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      sadtbox_get_object_menu,
  (DescribePropsFunc)   sadtbox_describe_props,
  (GetPropsFunc)        sadtbox_get_props,
  (SetPropsFunc)        sadtbox_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropNumData text_padding_data = { 0.0, 10.0, 0.1 };

static PropDescription box_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "padding",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
    N_("Text padding"), NULL, &text_padding_data},
  { "text", PROP_TYPE_TEXT, 0,NULL,NULL},
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  PROP_STD_TEXT_COLOUR,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_STD_FILL_COLOUR_OPTIONAL,
  { "id", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_DONT_MERGE,
    N_("Activity/Data identifier"),
    N_("The identifier which appears in the lower-right corner of the Box")},
  { "cpl_north",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_west",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_south",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  { "cpl_east",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
  PROP_DESC_END
};

static PropDescription *
sadtbox_describe_props(Box *box)
{
  if (box_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(box_props);
  }
  return box_props;
}

static PropOffset box_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "padding",PROP_TYPE_REAL,offsetof(Box,padding)},
  { "text", PROP_TYPE_TEXT, offsetof(Box,text)},
  { "text_alignment",PROP_TYPE_ENUM,offsetof(Box,text),offsetof(Text,alignment)},
  { "text_font",PROP_TYPE_FONT,offsetof(Box,text),offsetof(Text,font)},
  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Box,text),offsetof(Text,height)},
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Box,text),offsetof(Text,color)},
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Box, line_color) },
  { "fill_colour", PROP_TYPE_COLOUR, offsetof(Box, fill_color) },
  { "id", PROP_TYPE_STRING, offsetof(Box,id)},
  { "cpl_north",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,north)},
  { "cpl_west",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,west)},
  { "cpl_south",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,south)},
  { "cpl_east",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,east)},
  {NULL}
};

static void
sadtbox_get_props(Box *box, GPtrArray *props)
{
  object_get_props_from_offsets(&box->element.object,
                                box_offsets,props);
}

static void
sadtbox_set_props(Box *box, GPtrArray *props)
{
  object_set_props_from_offsets(&box->element.object,
                                box_offsets,props);
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);
}

static real
sadtbox_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - SADTBOX_LINE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + SADTBOX_LINE_WIDTH/2;
  rect.top = elem->corner.y - SADTBOX_LINE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + SADTBOX_LINE_WIDTH/2;
  return distance_rectangle_point(&rect, point);
}


static void
sadtbox_select (Box         *box,
                Point       *clicked_point,
                DiaRenderer *interactive_renderer)
{
  text_set_cursor (box->text, clicked_point, interactive_renderer);
  text_grab_focus (box->text, &box->element.object);
  element_update_handles (&box->element);
}


static DiaObjectChange *
sadtbox_move_handle (Box              *box,
                     Handle           *handle,
                     Point            *to,
                     ConnectionPoint  *cp,
                     HandleMoveReason  reason,
                     ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  g_return_val_if_fail (box != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  element_move_handle (&box->element, handle->id, to, cp, reason, modifiers);

  switch (handle->id) {
    case HANDLE_RESIZE_NW:
      horiz = ANCHOR_END; vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_N:
      vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_NE:
      horiz = ANCHOR_START; vert = ANCHOR_END;
      break;
    case HANDLE_RESIZE_E:
      horiz = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SE:
      horiz = ANCHOR_START; vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_S:
      vert = ANCHOR_START;
      break;
    case HANDLE_RESIZE_SW:
      horiz = ANCHOR_END; vert = ANCHOR_START;
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
  sadtbox_update_data (box, horiz, vert);

  return NULL;
}


static DiaObjectChange*
sadtbox_move (Box *box, Point *to)
{
  box->element.corner = *to;

  sadtbox_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


static void
sadtbox_draw (Box *box, DiaRenderer *renderer)
{
  Point lr_corner,pos;
  Element *elem;
  real idfontheight;

  g_return_if_fail (box != NULL);
  g_return_if_fail (renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);
  dia_renderer_set_linewidth (renderer, SADTBOX_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_rect (renderer,
                          &elem->corner,
                          &lr_corner,
                          &box->fill_color,
                          &box->line_color);


  text_draw (box->text, renderer);

  idfontheight = .75 * box->text->height;
  dia_renderer_set_font (renderer, box->text->font, idfontheight);
  pos = lr_corner;
  pos.x -= .3 * idfontheight;
  pos.y -= .3 * idfontheight;
  dia_renderer_draw_string (renderer,
                            box->id,
                            &pos,
                            DIA_ALIGN_RIGHT,
                            &box->text->color);
}


static void
sadtbox_update_data (Box *box, AnchorShape horiz, AnchorShape vert)
{
  Element *elem = &box->element;
  ElementBBExtras *extra = &elem->extra_spacing;
  DiaObject *obj = &elem->object;
  Point center, bottom_right;
  Point p;
  real width, height;
  Point nw,ne,se,sw;

  /* save starting points */
  center = bottom_right = elem->corner;
  center.x += elem->width/2;
  bottom_right.x += elem->width;
  center.y += elem->height/2;
  bottom_right.y += elem->height;

  text_calc_boundingbox (box->text, NULL);
  width = box->text->max_width + box->padding*2;
  height = box->text->height * box->text->numlines + box->padding*2;

  if (width > elem->width) elem->width = width;
  if (height > elem->height) elem->height = height;

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
  text_set_position (box->text, &p);

  extra->border_trans = SADTBOX_LINE_WIDTH / 2.0;
  element_update_boundingbox (elem);

  obj->position = elem->corner;

  element_update_handles (elem);

  /* Update connections: */
  nw = elem->corner;
  se.x = elem->corner.x + elem->width;
  se.y = elem->corner.y + elem->height;

  ne.x = se.x;
  ne.y = nw.y;
  sw.y = se.y;
  sw.x = nw.x;

  connpointline_update (box->north);
  connpointline_putonaline (box->north, &ne, &nw, DIR_NORTH);
  connpointline_update (box->west);
  connpointline_putonaline (box->west, &nw, &sw, DIR_WEST);
  connpointline_update (box->south);
  connpointline_putonaline (box->south, &sw, &se, DIR_SOUTH);
  connpointline_update (box->east);
  connpointline_putonaline (box->east, &se, &ne, DIR_EAST);
}


static ConnPointLine *
sadtbox_get_clicked_border (Box *box, Point *clicked)
{
  ConnPointLine *cpl;
  real dist,dist2;

  cpl = box->north;
  dist = distance_line_point (&box->north->start,
                              &box->north->end,
                              0,
                              clicked);

  dist2 = distance_line_point (&box->west->start,
                               &box->west->end,
                               0,
                               clicked);
  if (dist2 < dist) {
    cpl = box->west;
    dist = dist2;
  }
  dist2 = distance_line_point (&box->south->start,
                               &box->south->end,
                               0,
                               clicked);
  if (dist2 < dist) {
    cpl = box->south;
    dist = dist2;
  }
  dist2 = distance_line_point (&box->east->start,
                               &box->east->end,
                               0,
                               clicked);
  if (dist2 < dist) {
    cpl = box->east;
    /*dist = dist2;*/
  }
  return cpl;
}


inline static DiaObjectChange *
sadtbox_create_change (Box *box, DiaObjectChange *inner, ConnPointLine *cpl)
{
  return (DiaObjectChange *)inner;
}


static DiaObjectChange *
sadtbox_add_connpoint_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = sadtbox_get_clicked_border (box, clicked);
  change = connpointline_add_point (cpl, clicked);
  sadtbox_update_data ((Box *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return sadtbox_create_change (box, change, cpl);
}


static DiaObjectChange *
sadtbox_remove_connpoint_callback (DiaObject *obj,
                                   Point     *clicked,
                                   gpointer   data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = sadtbox_get_clicked_border (box, clicked);
  change = connpointline_remove_point (cpl, clicked);
  sadtbox_update_data ((Box *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return sadtbox_create_change (box, change, cpl);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), sadtbox_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), sadtbox_remove_connpoint_callback,
    NULL, 1 },
};

static DiaMenu object_menu = {
  N_("SADT box"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
sadtbox_get_object_menu(Box *box, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = sadtbox_get_clicked_border(box,clickedpoint);
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = connpointline_can_add_point(cpl, clickedpoint);
  object_menu_items[1].active = connpointline_can_remove_point(cpl, clickedpoint);
  return &object_menu;
}


static DiaObject *
sadtbox_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  DiaObject *obj;
  Point p;
  DiaFont* font;

  box = g_new0 (Box, 1);
  elem = &box->element;
  obj = &elem->object;

  obj->type = &sadtbox_type;

  obj->ops = &sadtbox_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  box->padding = 0.5; /* default_values.padding; */

  box->line_color = color_black;
  box->fill_color = color_white;

  p = *startpoint;
  p.x += elem->width / 2.0;
  p.y += elem->height / 2.0 + /*default_properties.font_size*/ 0.8 / 2;

  font = dia_font_new_from_style (DIA_FONT_SANS|DIA_FONT_BOLD, 0.8);

  box->text = new_text ("",
                        font,
                        0.8,
                        &p,
                        &color_black,
                        DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  box->id = g_strdup("A0"); /* should be made better.
                               Automatic counting ? */

  element_init(elem, 8, 0);

  box->north = connpointline_create(obj,4);
  box->west = connpointline_create(obj,3);
  box->south = connpointline_create(obj,1);
  box->east = connpointline_create(obj,3);

  box->element.extra_spacing.border_trans = SADTBOX_LINE_WIDTH/2.0;
  sadtbox_update_data(box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return &box->element.object;
}

static void
sadtbox_destroy(Box *box)
{
  text_destroy(box->text);

  connpointline_destroy(box->east);
  connpointline_destroy(box->south);
  connpointline_destroy(box->west);
  connpointline_destroy(box->north);

  g_clear_pointer (&box->id, g_free);

  element_destroy(&box->element);
}


static DiaObject *
sadtbox_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&sadtbox_type,
                                      obj_node,version,ctx);
}

