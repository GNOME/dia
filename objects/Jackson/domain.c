/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Jackson diagram -  adapted by Christophe Ponsard
 * This class captures all kind of domains (given, designed, machine)
 * both for generic problems and for problem frames (ie. with domain kinds)
 *
 * based on SADT diagrams copyright (C) 2000, 2001 Cyrille Chepelov
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

#include <stdio.h>
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

#include "pixmaps/given_domain.xpm"

#define DEFAULT_WIDTH 3.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25
#define DEFAULT_PADDING 0.4
#define DEFAULT_FONT 0.7
#define JACKSON_BOX_LINE_WIDTH 0.09
#define JACKSON_BOX_FG_COLOR color_black
#define JACKSON_BOX_BG_COLOR color_white
#define LEFT_SPACE 0.7
#define RIGHT_SPACE 0.3

typedef enum {
  ANCHOR_MIDDLE,
  ANCHOR_START,
  ANCHOR_END
} AnchorShape;

/* Domain Type */

typedef enum {
  DOMAIN_GIVEN,
  DOMAIN_DESIGNED,
  DOMAIN_MACHINE
} DomainType;

static PropEnumData prop_domain_type_data[] = {
  { N_("Given Domain"), DOMAIN_GIVEN },
  { N_("Designed Domain"), DOMAIN_DESIGNED },
  { N_("Machine Domain"), DOMAIN_MACHINE },
  { NULL, 0}
};

/* Domain qualifier */

typedef enum {
  DOMAIN_NONE,
  DOMAIN_CAUSAL,
  DOMAIN_BIDDABLE,
  DOMAIN_LEXICAL
} DomainKind;


static PropEnumData prop_domain_kind_data[] = {
  { N_("None"), DOMAIN_NONE },
  { N_("Causal"), DOMAIN_CAUSAL },
  { N_("Biddable"), DOMAIN_BIDDABLE },
  { N_("Lexical"), DOMAIN_LEXICAL },
  { NULL, 0}
};

typedef struct _Box {
  Element element;

  ConnPointLine *north,*south,*east,*west;

  Text *text;
  real padding;
  DomainType domtype;
  DomainKind domkind;
} Box;

static real jackson_box_distance_from(Box *box, Point *point);
static void jackson_box_select(Box *box, Point *clicked_point,
		       DiaRenderer *interactive_renderer);
static DiaObjectChange *jackson_box_move_handle  (Box             *box,
                                                  Handle          *handle,
                                                  Point           *to,
                                                  ConnectionPoint *cp,
                                                  HandleMoveReason reason,
                                                  ModifierKeys     modifiers);
static DiaObjectChange *jackson_box_move         (Box             *box,
                                                  Point           *to);
static void jackson_box_draw(Box *box, DiaRenderer *renderer);
static void jackson_box_update_data(Box *box, AnchorShape horix, AnchorShape vert);
static DiaObject *jackson_box_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void jackson_box_destroy(Box *box);
static DiaObject *jackson_box_load(ObjectNode obj_node, int version,
				   DiaContext *ctx);
static DiaMenu *jackson_box_get_object_menu(Box *box, Point *clickedpoint);

static PropDescription *jackson_box_describe_props(Box *box);
static void jackson_box_get_props(Box *box, GPtrArray *props);
static void jackson_box_set_props(Box *box, GPtrArray *props);

static ObjectTypeOps jackson_domain_type_ops =
{
  (CreateFunc) jackson_box_create,
  (LoadFunc)   jackson_box_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL,
};

DiaObjectType jackson_domain_type =
{
  "Jackson - domain",       /* name */
  0,                        /* version */
  jackson_given_domain_xpm, /* this is the default pixmap */
  &jackson_domain_type_ops, /* ops */
  NULL, /* pixmap_file */
  NULL, /* default_user_data */
  NULL, /* prop_descs */
  NULL, /* prop_offsets */
  DIA_OBJECT_HAS_VARIANTS /* flags */
};

static ObjectOps jackson_box_ops = {
  (DestroyFunc)         jackson_box_destroy,
  (DrawFunc)            jackson_box_draw,
  (DistanceFunc)        jackson_box_distance_from,
  (SelectFunc)          jackson_box_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            jackson_box_move,
  (MoveHandleFunc)      jackson_box_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      jackson_box_get_object_menu,
  (DescribePropsFunc)   jackson_box_describe_props,
  (GetPropsFunc)        jackson_box_get_props,
  (SetPropsFunc)        jackson_box_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription box_props[] = {
  ELEMENT_COMMON_PROPERTIES,

    { "domtype", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE|PROP_FLAG_NO_DEFAULTS,
    N_("Domain Type"),
    N_("Domain Type"),
    prop_domain_type_data},

    { "domkind", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Domain Kind"),
    N_("Optional kind which appears in the lower-right corner of the Domain"),
    prop_domain_kind_data},

/* hidding style stuff
    { "padding",PROP_TYPE_REAL,PROP_FLAG_VISIBLE,
      N_("Text padding"), NULL, &text_padding_data},
*/
    { "text", PROP_TYPE_TEXT, 0,NULL,NULL},
/*
    PROP_STD_TEXT_ALIGNMENT,
    PROP_STD_TEXT_FONT,
    PROP_STD_TEXT_HEIGHT,
    PROP_STD_TEXT_COLOUR,
*/

    { "cpl_north",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
    { "cpl_west",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
    { "cpl_south",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},
    { "cpl_east",PROP_TYPE_CONNPOINT_LINE, 0, NULL, NULL},

    PROP_DESC_END
};

static PropDescription *
jackson_box_describe_props(Box *box)
{
  if (box_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(box_props);
  }
  return box_props;
}

static PropOffset box_offsets[] = {
  ELEMENT_COMMON_PROPERTIES_OFFSETS,
  { "domtype", PROP_TYPE_ENUM, offsetof(Box,domtype)},
  { "domkind", PROP_TYPE_ENUM, offsetof(Box,domkind)},
  { "text", PROP_TYPE_TEXT, offsetof(Box,text)},
  { "text_alignment",PROP_TYPE_ENUM,offsetof(Box,text),offsetof(Text,alignment)},
  { "text_font",PROP_TYPE_FONT,offsetof(Box,text),offsetof(Text,font)},
  { PROP_STDNAME_TEXT_HEIGHT,PROP_STDTYPE_TEXT_HEIGHT,offsetof(Box,text),offsetof(Text,height)},
  { "text_colour",PROP_TYPE_COLOUR,offsetof(Box,text),offsetof(Text,color)},
  { "cpl_north",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,north)},
  { "cpl_west",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,west)},
  { "cpl_south",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,south)},
  { "cpl_east",PROP_TYPE_CONNPOINT_LINE, offsetof(Box,east)},
  {NULL}
};


static void
jackson_box_get_props (Box *box, GPtrArray *props)
{
  object_get_props_from_offsets (&box->element.object,
                                 box_offsets,props);
}


static void
jackson_box_set_props (Box *box, GPtrArray *props)
{
  object_set_props_from_offsets (&box->element.object,
                                 box_offsets,props);

  jackson_box_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

}


static real
jackson_box_distance_from (Box *box, Point *point)
{
  Element *elem = &box->element;
  DiaRectangle rect;

  rect.left = elem->corner.x - JACKSON_BOX_LINE_WIDTH/2;
  rect.right = elem->corner.x + elem->width + JACKSON_BOX_LINE_WIDTH/2;
  rect.top = elem->corner.y - JACKSON_BOX_LINE_WIDTH/2;
  rect.bottom = elem->corner.y + elem->height + JACKSON_BOX_LINE_WIDTH/2;

  return distance_rectangle_point (&rect, point);
}


static void
jackson_box_select (Box         *box,
                    Point       *clicked_point,
                    DiaRenderer *interactive_renderer)
{
  text_set_cursor (box->text, clicked_point, interactive_renderer);
  text_grab_focus (box->text, &box->element.object);
  element_update_handles (&box->element);
}


static DiaObjectChange *
jackson_box_move_handle (Box              *box,
                         Handle           *handle,
                         Point            *to,
                         ConnectionPoint  *cp,
                         HandleMoveReason  reason,
                         ModifierKeys      modifiers)
{
  AnchorShape horiz = ANCHOR_MIDDLE, vert = ANCHOR_MIDDLE;

  g_return_val_if_fail (box != NULL, NULL);
  g_return_val_if_fail (handle !=NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

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

  jackson_box_update_data (box, horiz, vert);

  return NULL;
}


static DiaObjectChange *
jackson_box_move (Box *box, Point *to)
{
  box->element.corner = *to;

  jackson_box_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return NULL;
}


/* draw method */
static void
jackson_box_draw (Box *box, DiaRenderer *renderer)
{
  Point b0,b1,b2,b3,p1t,p1b,p2t,p2b;
  Element *elem;
  real idfontheight;
  const char* s;

  /* some asserts */
  g_return_if_fail (box != NULL);
  g_return_if_fail (renderer != NULL);

  /* computing positions */
  elem = &box->element;

  b0.x = elem->corner.x;
  b0.y = elem->corner.y;
  b1.x = b0.x + elem->width;
  b1.y = b0.y + elem->height;

  p1t.x= elem->corner.x + LEFT_SPACE/2;
  p1t.y= elem->corner.y;
  p1b.x= p1t.x;
  p1b.y= p1t.y + elem->height;

  p2t.x= elem->corner.x + LEFT_SPACE;
  p2t.y= p1t.y;
  p2b.x= p2t.x;
  p2b.y= p1b.y;

  /* drawing main box */
  dia_renderer_set_fillstyle (renderer, DIA_FILL_STYLE_SOLID);

  dia_renderer_set_linewidth (renderer, JACKSON_BOX_LINE_WIDTH);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);

  dia_renderer_draw_rect (renderer, &b0, &b1,
                          &JACKSON_BOX_BG_COLOR, &JACKSON_BOX_FG_COLOR);

  /* adding lines for designed/machine domains */
  if (box->domtype != DOMAIN_GIVEN) {
    dia_renderer_draw_line (renderer, &p1t, &p1b, &JACKSON_BOX_FG_COLOR);
  }

  if (box->domtype == DOMAIN_MACHINE) {
    dia_renderer_draw_line (renderer, &p2t, &p2b, &JACKSON_BOX_FG_COLOR);
  }

  /* adding corner for optional qualifier */
  idfontheight = box->text->height;
  dia_renderer_set_font (renderer, box->text->font, idfontheight);
  b2 = b3 = b1;
  b2.x -= .2 * idfontheight;
  b2.y -= .2 * idfontheight;
  b3.x -= idfontheight;
  b3.y -= idfontheight;

  switch (box->domkind) {
    case DOMAIN_CAUSAL:
      s = "C";
      break;
    case DOMAIN_BIDDABLE:
      s = "B";
      break;
    case DOMAIN_LEXICAL:
      s = "L";
      break;
    case DOMAIN_NONE:
    default:
      s = NULL;
  }

  if (s != NULL) {
    dia_renderer_draw_rect (renderer, &b3, &b1,
                            NULL, &JACKSON_BOX_FG_COLOR);
    dia_renderer_draw_string (renderer,
                              s,
                              &b2,
                              DIA_ALIGN_RIGHT,
                              &box->text->color);
  }

  text_draw (box->text, renderer);
}


/* resize stuff */
static void
jackson_box_update_data (Box *box, AnchorShape horiz, AnchorShape vert)
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

  text_calc_boundingbox(box->text, NULL);
  width = LEFT_SPACE + box->text->max_width + box->padding*2 + RIGHT_SPACE;
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
  p.x += (LEFT_SPACE+elem->width-RIGHT_SPACE) / 2.0;
  p.y += elem->height / 2.0 - box->text->height * box->text->numlines / 2 + box->text->ascent;
  text_set_position (box->text, &p);

  extra->border_trans = JACKSON_BOX_LINE_WIDTH / 2.0;
  element_update_boundingbox (elem);

  obj->position = elem->corner;

  element_update_handles (elem);

  /* Update connections: */
  nw = elem->corner;
  se.x = nw.x + elem->width;
  se.y = nw.y + elem->height;
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
jackson_box_get_clicked_border (Box *box, Point *clicked)
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
jackson_box_create_change (Box             *box,
                           DiaObjectChange *inner,
                           ConnPointLine   *cpl)
{
  return (DiaObjectChange *) inner;
}


static DiaObjectChange *
jackson_box_add_connpoint_callback (DiaObject *obj,
                                    Point     *clicked,
                                    gpointer   data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = jackson_box_get_clicked_border (box, clicked);
  change = connpointline_add_point (cpl, clicked);
  jackson_box_update_data ((Box *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return jackson_box_create_change (box, change, cpl);
}


static DiaObjectChange *
jackson_box_remove_connpoint_callback (DiaObject *obj,
                                       Point     *clicked,
                                       gpointer   data)
{
  DiaObjectChange *change;
  ConnPointLine *cpl;
  Box *box = (Box *)obj;

  cpl = jackson_box_get_clicked_border (box,clicked);
  change = connpointline_remove_point (cpl, clicked);
  jackson_box_update_data ((Box *) obj, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  return jackson_box_create_change (box, change, cpl);
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add connection point"), jackson_box_add_connpoint_callback, NULL, 1 },
  { N_("Delete connection point"), jackson_box_remove_connpoint_callback,
    NULL, 1 },
};


static DiaMenu object_menu = {
  N_("Jackson domain"),
  sizeof (object_menu_items) / sizeof (DiaMenuItem),
  object_menu_items,
  NULL
};


static DiaMenu *
jackson_box_get_object_menu (Box *box, Point *clickedpoint)
{
  ConnPointLine *cpl;

  cpl = jackson_box_get_clicked_border (box,clickedpoint);
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = connpointline_can_add_point (cpl,
                                                             clickedpoint);
  object_menu_items[1].active = connpointline_can_remove_point (cpl,
                                                                clickedpoint);

  return &object_menu;
}


/* create */
static DiaObject *
jackson_box_create (Point   *startpoint,
                    void    *user_data,
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

  obj->type = &jackson_domain_type;
  obj->ops = &jackson_box_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_HEIGHT;

  box->padding = DEFAULT_PADDING;

  /* text stuff */
  p = *startpoint;
  p.x += (LEFT_SPACE +elem->width) / 2.0;
  p.y += elem->height / 2.0 + DEFAULT_FONT / 2;

  font = dia_font_new_from_style (DIA_FONT_SANS, DEFAULT_FONT);

  box->text = new_text ("", font,
                        DEFAULT_FONT, &p,
                        &color_black,
                        DIA_ALIGN_CENTRE);
  g_clear_object (&font);

  element_init (elem, 8, 0);

  box->north = connpointline_create(obj,3);
  box->west = connpointline_create(obj,1);
  box->south = connpointline_create(obj,3);
  box->east = connpointline_create(obj,1);

  box->element.extra_spacing.border_trans = JACKSON_BOX_LINE_WIDTH/2.0;
  jackson_box_update_data (box, ANCHOR_MIDDLE, ANCHOR_MIDDLE);

  *handle1 = NULL;
  *handle2 = obj->handles[7];

  /* template information here */

  switch (GPOINTER_TO_INT (user_data)) {
    case 1:  box->domtype = DOMAIN_GIVEN; break;
    case 2:  box->domtype = DOMAIN_DESIGNED; break;
    case 3:  box->domtype = DOMAIN_MACHINE; break;
    default: box->domtype = DOMAIN_GIVEN; break;
  }

  box->domkind = DOMAIN_NONE;

  return &box->element.object;
}

static void
jackson_box_destroy(Box *box)
{
  text_destroy(box->text);

  connpointline_destroy(box->east);
  connpointline_destroy(box->south);
  connpointline_destroy(box->west);
  connpointline_destroy(box->north);

  element_destroy(&box->element);
}


static DiaObject *
jackson_box_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
  return object_load_using_properties(&jackson_domain_type,
                                      obj_node,version,ctx);
}
