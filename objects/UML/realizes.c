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
#include "orth_conn.h"
#include "diarenderer.h"
#include "attributes.h"
#include "arrows.h"
#include "properties.h"
#include "stereotype.h"
#include "uml.h"

#include "pixmaps/realizes.xpm"

typedef struct _Realizes Realizes;

struct _Realizes {
  OrthConn orth;

  Point text_pos;
  DiaAlignment text_align;
  real text_width;

  Color text_color;
  Color line_color;

  DiaFont *font;
  real     font_height;
  real     line_width;

  char *name;
  char *stereotype; /* excluding << and >> */
  char *st_stereotype; /* including << and >> */
};


#define REALIZES_TRIANGLESIZE (realize->font_height)
#define REALIZES_DASHLEN 0.4

static real realizes_distance_from(Realizes *realize, Point *point);
static void realizes_select(Realizes *realize, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static DiaObjectChange *realizes_move_handle   (Realizes         *realize,
                                                Handle           *handle,
                                                Point            *to,
                                                ConnectionPoint  *cp,
                                                HandleMoveReason  reason,
                                                ModifierKeys      modifiers);
static DiaObjectChange *realizes_move          (Realizes         *realize,
                                                Point            *to);
static void realizes_draw(Realizes *realize, DiaRenderer *renderer);
static DiaObject *realizes_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void realizes_destroy(Realizes *realize);
static DiaMenu *realizes_get_object_menu(Realizes *realize,
					 Point *clickedpoint);

static PropDescription *realizes_describe_props(Realizes *realizes);
static void realizes_get_props(Realizes * realizes, GPtrArray *props);
static void realizes_set_props(Realizes * realizes, GPtrArray *props);

static DiaObject *realizes_load(ObjectNode obj_node, int version,DiaContext *ctx);

static void realizes_update_data(Realizes *realize);

static ObjectTypeOps realizes_type_ops =
{
  (CreateFunc) realizes_create,
  (LoadFunc)   realizes_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType realizes_type =
{
  "UML - Realizes",   /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                  /* version */
  realizes_xpm,       /* pixmap */
  &realizes_type_ops, /* ops */
  NULL,               /* pixmap_file */
  0                   /* default_user_data */
};

static ObjectOps realizes_ops = {
  (DestroyFunc)         realizes_destroy,
  (DrawFunc)            realizes_draw,
  (DistanceFunc)        realizes_distance_from,
  (SelectFunc)          realizes_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            realizes_move,
  (MoveHandleFunc)      realizes_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      realizes_get_object_menu,
  (DescribePropsFunc)   realizes_describe_props,
  (GetPropsFunc)        realizes_get_props,
  (SetPropsFunc)        realizes_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription realizes_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype:"), NULL, NULL },
  /* can't use PROP_STD_TEXT_COLOUR_OPTIONAL cause it has PROP_FLAG_DONT_SAVE. It is designed to fill the Text object - not some subset */
  PROP_STD_TEXT_FONT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_HEIGHT_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_TEXT_COLOUR_OPTIONS(PROP_FLAG_VISIBLE|PROP_FLAG_STANDARD|PROP_FLAG_OPTIONAL),
  PROP_STD_LINE_WIDTH_OPTIONAL,
  PROP_STD_LINE_COLOUR_OPTIONAL,
  PROP_DESC_END
};

static PropDescription *
realizes_describe_props(Realizes *realizes)
{
  if (realizes_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(realizes_props);
  }
  return realizes_props;
}

static PropOffset realizes_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Realizes, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(Realizes, stereotype) },
  { "text_font", PROP_TYPE_FONT, offsetof(Realizes, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Realizes, font_height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof(Realizes, text_color) },
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Realizes, line_width) },
  { "line_colour", PROP_TYPE_COLOUR, offsetof(Realizes, line_color) },
  { NULL, 0, 0 }
};

static void
realizes_get_props(Realizes * realizes, GPtrArray *props)
{
  object_get_props_from_offsets(&realizes->orth.object,
                                realizes_offsets,props);
}


static void
realizes_set_props (Realizes *realizes, GPtrArray *props)
{
  object_set_props_from_offsets (&realizes->orth.object,
                                 realizes_offsets,
                                 props);
  g_clear_pointer (&realizes->st_stereotype, g_free);
  realizes_update_data(realizes);
}


static double
realizes_distance_from (Realizes *realize, Point *point)
{
  OrthConn *orth = &realize->orth;

  return orthconn_distance_from (orth, point, realize->line_width);
}


static void
realizes_select(Realizes *realize, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&realize->orth);
}


static DiaObjectChange *
realizes_move_handle (Realizes         *realize,
                      Handle           *handle,
                      Point            *to,
                      ConnectionPoint  *cp,
                      HandleMoveReason  reason,
                      ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (realize != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  change = orthconn_move_handle (&realize->orth,
                                 handle,
                                 to,
                                 cp,
                                 reason,
                                 modifiers);
  realizes_update_data (realize);

  return change;
}


static DiaObjectChange *
realizes_move (Realizes *realize, Point *to)
{
  orthconn_move (&realize->orth, to);
  realizes_update_data (realize);

  return NULL;
}


static void
realizes_draw (Realizes *realize, DiaRenderer *renderer)
{
  OrthConn *orth = &realize->orth;
  Point *points;
  int n;
  Point pos;
  Arrow arrow;

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, realize->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_DASHED, REALIZES_DASHLEN);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  arrow.type = ARROW_HOLLOW_TRIANGLE;
  arrow.width = REALIZES_TRIANGLESIZE;
  arrow.length = REALIZES_TRIANGLESIZE;
  dia_renderer_draw_polyline_with_arrows (renderer,
                                          points,
                                          n,
                                          realize->line_width,
                                          &realize->line_color,
                                          &arrow,
                                          NULL);

  dia_renderer_set_font (renderer, realize->font, realize->font_height);
  pos = realize->text_pos;

  if (realize->st_stereotype != NULL && realize->st_stereotype[0] != '\0') {
    dia_renderer_draw_string (renderer,
                              realize->st_stereotype,
                              &pos,
                              realize->text_align,
                              &realize->text_color);

    pos.y += realize->font_height;
  }

  if (realize->name != NULL && realize->name[0] != '\0') {
    dia_renderer_draw_string (renderer,
                              realize->name,
                              &pos,
                              realize->text_align,
                              &realize->text_color);
  }
}

static void
realizes_update_data(Realizes *realize)
{
  OrthConn *orth = &realize->orth;
  DiaObject *obj = &orth->object;
  int num_segm, i;
  Point *points;
  DiaRectangle rect;
  PolyBBExtras *extra;

  orthconn_update_data(orth);

  realize->text_width = 0.0;

  realize->stereotype = remove_stereotype_from_string(realize->stereotype);
  if (!realize->st_stereotype) {
    realize->st_stereotype =  string_to_stereotype(realize->stereotype);
  }

  if (realize->name)
    realize->text_width = dia_font_string_width(realize->name, realize->font,
					    realize->font_height);
  if (realize->stereotype)
    realize->text_width = MAX(realize->text_width,
			      dia_font_string_width(realize->stereotype,
						realize->font,
						realize->font_height));

  extra = &orth->extra_spacing;

  extra->start_trans = realize->line_width/2.0 + REALIZES_TRIANGLESIZE;
  extra->start_long =
    extra->middle_trans =
    extra->end_trans =
    extra->end_long = realize->line_width/2.0;

  orthconn_update_boundingbox(orth);

  /* Calc text pos: */
  num_segm = realize->orth.numpoints - 1;
  points = realize->orth.points;
  i = num_segm / 2;

  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (realize->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (realize->orth.orientation[i]) {
    case HORIZONTAL:
      realize->text_align = DIA_ALIGN_CENTRE;
      realize->text_pos.x = 0.5*(points[i].x+points[i+1].x);
      realize->text_pos.y = points[i].y;
      if (realize->name)
        realize->text_pos.y -=
          dia_font_descent (realize->name,realize->font, realize->font_height);
      break;
    case VERTICAL:
      realize->text_align = DIA_ALIGN_LEFT;
      realize->text_pos.x = points[i].x + 0.1;
      realize->text_pos.y = 0.5*(points[i].y+points[i+1].y);
      if (realize->name)
        realize->text_pos.y -=
          dia_font_descent (realize->name, realize->font, realize->font_height);
      break;
    default:
      g_return_if_reached ();
  }

  /* Add the text recangle to the bounding box: */
  rect.left = realize->text_pos.x;
  if (realize->text_align == DIA_ALIGN_CENTRE) {
    rect.left -= realize->text_width / 2.0;
  }
  rect.right = rect.left + realize->text_width;
  rect.top = realize->text_pos.y;
  if (realize->name)
    rect.top -= dia_font_ascent(realize->name,realize->font, realize->font_height);
  rect.bottom = rect.top + 2*realize->font_height;

  rectangle_union(&obj->bounding_box, &rect);
}


static DiaObjectChange *
realizes_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  realizes_update_data ((Realizes *) obj);

  return change;
}


static DiaObjectChange *
realizes_delete_segment_callback (DiaObject *obj,
                                  Point     *clicked,
                                  gpointer   data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  realizes_update_data ((Realizes *) obj);

  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), realizes_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), realizes_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Realizes",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
realizes_get_object_menu(Realizes *realize, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &realize->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);
  return &object_menu;
}

static DiaObject *
realizes_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Realizes *realize;
  OrthConn *orth;
  DiaObject *obj;
  PolyBBExtras *extra;

  realize = g_new0 (Realizes, 1);
  /* old defaults */
  realize->font_height = 0.8;
  realize->font =
      dia_font_new_from_style (DIA_FONT_MONOSPACE, realize->font_height);
  realize->line_width = 0.1;

  orth = &realize->orth;
  obj = &orth->object;
  extra = &orth->extra_spacing;

  obj->type = &realizes_type;

  obj->ops = &realizes_ops;

  orthconn_init(orth, startpoint);

  realize->text_color = color_black;
  realize->line_color = attributes_get_foreground();

  realize->name = NULL;
  realize->stereotype = NULL;
  realize->st_stereotype = NULL;
  realize->text_width = 0;

  extra->start_trans = realize->line_width/2.0 + REALIZES_TRIANGLESIZE;
  extra->start_long =
    extra->middle_trans =
    extra->end_trans =
    extra->end_long = realize->line_width/2.0;

  realizes_update_data(realize);

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return &realize->orth.object;
}


static void
realizes_destroy (Realizes *realize)
{
  g_clear_pointer (&realize->name, g_free);
  g_clear_pointer (&realize->stereotype, g_free);
  g_clear_pointer (&realize->st_stereotype, g_free);
  g_clear_object (&realize->font);
  orthconn_destroy (&realize->orth);
}


static DiaObject *
realizes_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&realizes_type,
                                                obj_node,version,ctx);
  if (version == 0) {
    AttributeNode attr;
    /* In old objects with no autorouting, set it to false. */
    attr = object_find_attribute(obj_node, "autorouting");
    if (attr == NULL)
      ((OrthConn*)obj)->autorouting = FALSE;
  }
  return obj;
}
