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

#include "pixmaps/generalization.xpm"

typedef struct _Generalization Generalization;

struct _Generalization {
  OrthConn orth;

  Point text_pos;
  DiaAlignment text_align;
  real text_width;

  DiaFont *font;
  real     font_height;
  Color    text_color;

  real     line_width;
  Color    line_color;

  char *name;
  char *stereotype; /* excluding << and >> */
  char *st_stereotype; /* including << and >> */
};

#define GENERALIZATION_WIDTH 0.1
#define GENERALIZATION_TRIANGLESIZE (genlz->font_height)

static real generalization_distance_from(Generalization *genlz, Point *point);
static void generalization_select(Generalization *genlz, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static DiaObjectChange* generalization_move_handle   (Generalization   *genlz,
                                                      Handle           *handle,
                                                      Point            *to,
                                                      ConnectionPoint  *cp,
                                                      HandleMoveReason  reason,
                                                      ModifierKeys      modifiers);
static DiaObjectChange* generalization_move          (Generalization   *genlz,
                                                      Point            *to);
static void generalization_draw(Generalization *genlz, DiaRenderer *renderer);
static DiaObject *generalization_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void generalization_destroy(Generalization *genlz);
static DiaMenu *generalization_get_object_menu(Generalization *genlz,
						Point *clickedpoint);

static PropDescription *generalization_describe_props(Generalization *generalization);
static void generalization_get_props(Generalization * generalization, GPtrArray *props);
static void generalization_set_props(Generalization * generalization, GPtrArray *props);

static DiaObject *generalization_load(ObjectNode obj_node, int version,DiaContext *ctx);

static void generalization_update_data(Generalization *genlz);

static ObjectTypeOps generalization_type_ops =
{
  (CreateFunc) generalization_create,
  (LoadFunc)   generalization_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL,
  (ApplyDefaultsFunc) NULL
};

DiaObjectType generalization_type =
{
  "UML - Generalization",  /* name */
  /* Version 0 had no autorouting and so shouldn't have it set by default. */
  1,                     /* version */
  generalization_xpm,     /* pixmap */
  &generalization_type_ops,  /* ops */
  NULL,              /* pixmap_file */
  0            /* default_user_data */
};

static ObjectOps generalization_ops = {
  (DestroyFunc)         generalization_destroy,
  (DrawFunc)            generalization_draw,
  (DistanceFunc)        generalization_distance_from,
  (SelectFunc)          generalization_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            generalization_move,
  (MoveHandleFunc)      generalization_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      generalization_get_object_menu,
  (DescribePropsFunc)   generalization_describe_props,
  (GetPropsFunc)        generalization_get_props,
  (SetPropsFunc)        generalization_set_props,
  (TextEditFunc) 0,
  (ApplyPropertiesListFunc) object_apply_props,
};

static PropDescription generalization_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
    N_("Name:"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE|PROP_FLAG_OPTIONAL,
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
generalization_describe_props(Generalization *generalization)
{
  if (generalization_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(generalization_props);
  }
  return generalization_props;
}

static PropOffset generalization_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "name", PROP_TYPE_STRING, offsetof(Generalization, name) },
  { "stereotype", PROP_TYPE_STRING, offsetof(Generalization, stereotype) },
  { "text_font", PROP_TYPE_FONT, offsetof(Generalization, font) },
  { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(Generalization, font_height) },
  {"text_colour",PROP_TYPE_COLOUR,offsetof(Generalization,text_color)},
  { PROP_STDNAME_LINE_WIDTH,PROP_TYPE_LENGTH,offsetof(Generalization, line_width) },
  {"line_colour",PROP_TYPE_COLOUR,offsetof(Generalization,line_color)},
  { NULL, 0, 0 }
};

static void
generalization_get_props(Generalization * generalization, GPtrArray *props)
{
  object_get_props_from_offsets(&generalization->orth.object,
                                generalization_offsets,props);
}


static void
generalization_set_props (Generalization *generalization, GPtrArray *props)
{
  object_set_props_from_offsets (&generalization->orth.object,
                                 generalization_offsets, props);
  g_clear_pointer (&generalization->st_stereotype, g_free);
  generalization_update_data (generalization);
}


static double
generalization_distance_from (Generalization *genlz, Point *point)
{
  OrthConn *orth = &genlz->orth;

  return orthconn_distance_from (orth, point, genlz->line_width);
}


static void
generalization_select(Generalization *genlz, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&genlz->orth);
}


static DiaObjectChange *
generalization_move_handle (Generalization   *genlz,
                            Handle           *handle,
                            Point            *to,
                            ConnectionPoint  *cp,
                            HandleMoveReason  reason,
                            ModifierKeys      modifiers)
{
  DiaObjectChange *change;

  g_return_val_if_fail (genlz != NULL, NULL);
  g_return_val_if_fail (handle != NULL, NULL);
  g_return_val_if_fail (to != NULL, NULL);

  change = orthconn_move_handle (&genlz->orth, handle, to, cp, reason, modifiers);
  generalization_update_data (genlz);

  return change;
}


static DiaObjectChange *
generalization_move (Generalization *genlz, Point *to)
{
  DiaObjectChange *change;

  change = orthconn_move(&genlz->orth, to);
  generalization_update_data(genlz);

  return change;
}


static void
generalization_draw (Generalization *genlz, DiaRenderer *renderer)
{
  OrthConn *orth = &genlz->orth;
  Point *points;
  int n;
  Point pos;
  Arrow arrow;

  points = &orth->points[0];
  n = orth->numpoints;

  dia_renderer_set_linewidth (renderer, genlz->line_width);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
  dia_renderer_set_linecaps (renderer, DIA_LINE_CAPS_BUTT);

  arrow.type = ARROW_HOLLOW_TRIANGLE;
  arrow.length = GENERALIZATION_TRIANGLESIZE;
  arrow.width = GENERALIZATION_TRIANGLESIZE;

  dia_renderer_draw_polyline_with_arrows (renderer,
                                          points,
                                          n,
                                          genlz->line_width,
                                          &genlz->line_color,
                                          &arrow, NULL);

  dia_renderer_set_font (renderer, genlz->font, genlz->font_height);
  pos = genlz->text_pos;

  if (genlz->st_stereotype != NULL && genlz->st_stereotype[0] != '\0') {
    dia_renderer_draw_string (renderer,
                              genlz->st_stereotype,
                              &pos,
                              genlz->text_align,
                              &genlz->text_color);

    pos.y += genlz->font_height;
  }

  if (genlz->name != NULL && genlz->name[0] != '\0') {
    dia_renderer_draw_string (renderer,
                              genlz->name,
                              &pos,
                              genlz->text_align,
                              &genlz->text_color);
  }
}

static void
generalization_update_data(Generalization *genlz)
{
  OrthConn *orth = &genlz->orth;
  DiaObject *obj = &orth->object;
  int num_segm, i;
  Point *points;
  DiaRectangle rect;
  PolyBBExtras *extra;
  real descent;
  real ascent;

  orthconn_update_data(orth);

  genlz->stereotype = remove_stereotype_from_string(genlz->stereotype);
  if (!genlz->st_stereotype) {
    genlz->st_stereotype =  string_to_stereotype(genlz->stereotype);
  }

  genlz->text_width = 0.0;
  descent = 0.0;
  ascent = 0.0;

  if (genlz->name) {
    genlz->text_width = dia_font_string_width(genlz->name, genlz->font,
                                              genlz->font_height);
    descent = dia_font_descent(genlz->name,
                               genlz->font,genlz->font_height);
    ascent = dia_font_ascent(genlz->name,
                             genlz->font,genlz->font_height);
  }
  if (genlz->stereotype) {
    genlz->text_width = MAX(genlz->text_width,
                            dia_font_string_width(genlz->stereotype,
                                                  genlz->font,
                                                  genlz->font_height));
    if (!genlz->name) {
      descent = dia_font_descent(genlz->stereotype,
                                genlz->font,genlz->font_height);
    }
    ascent = dia_font_ascent(genlz->stereotype,
                             genlz->font,genlz->font_height);
  }

  extra = &orth->extra_spacing;

  extra->start_trans = genlz->line_width/2.0 + GENERALIZATION_TRIANGLESIZE;
  extra->start_long =
    extra->middle_trans =
    extra->end_trans =
    extra->end_long = genlz->line_width/2.0;

  orthconn_update_boundingbox(orth);

  /* Calc text pos: */
  num_segm = genlz->orth.numpoints - 1;
  points = genlz->orth.points;
  i = num_segm / 2;

  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (genlz->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (genlz->orth.orientation[i]) {
    case HORIZONTAL:
      genlz->text_align = DIA_ALIGN_CENTRE;
      genlz->text_pos.x = 0.5*(points[i].x+points[i+1].x);
      genlz->text_pos.y = points[i].y - descent;
      break;
    case VERTICAL:
      genlz->text_align = DIA_ALIGN_LEFT;
      genlz->text_pos.x = points[i].x + 0.1;
      genlz->text_pos.y = 0.5*(points[i].y+points[i+1].y) - descent;
      break;
    default:
      g_return_if_reached ();
  }

  /* Add the text recangle to the bounding box: */
  rect.left = genlz->text_pos.x;
  if (genlz->text_align == DIA_ALIGN_CENTRE) {
    rect.left -= genlz->text_width / 2.0;
  }
  rect.right = rect.left + genlz->text_width;
  rect.top = genlz->text_pos.y - ascent;
  rect.bottom = rect.top + 2*genlz->font_height;

  rectangle_union(&obj->bounding_box, &rect);
}


static DiaObjectChange *
generalization_add_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_add_segment ((OrthConn *) obj, clicked);
  generalization_update_data ((Generalization *) obj);

  return change;
}


static DiaObjectChange *
generalization_delete_segment_callback (DiaObject *obj, Point *clicked, gpointer data)
{
  DiaObjectChange *change;

  change = orthconn_delete_segment ((OrthConn *) obj, clicked);
  generalization_update_data ((Generalization *) obj);

  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), generalization_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), generalization_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  "Generalization",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
generalization_get_object_menu(Generalization *genlz, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &genlz->orth;

  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[2]);

  return &object_menu;
}

static DiaObject *
generalization_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Generalization *genlz;
  OrthConn *orth;
  DiaObject *obj;

  genlz = g_new0(Generalization, 1);

  /* old defaults */
  genlz->font_height = 0.8;
  genlz->font = dia_font_new_from_style(DIA_FONT_MONOSPACE, genlz->font_height);
  genlz->line_width = 0.1;

  orth = &genlz->orth;
  obj = (DiaObject *)genlz;

  obj->type = &generalization_type;

  obj->ops = &generalization_ops;

  orthconn_init(orth, startpoint);

  genlz->text_color = color_black;
  genlz->line_color = attributes_get_foreground();
  genlz->name = NULL;
  genlz->stereotype = NULL;
  genlz->st_stereotype = NULL;

  generalization_update_data(genlz);

  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return (DiaObject *)genlz;
}


static void
generalization_destroy (Generalization *genlz)
{
  g_clear_pointer (&genlz->name, g_free);
  g_clear_pointer (&genlz->stereotype, g_free);
  g_clear_pointer (&genlz->st_stereotype, g_free);
  g_clear_object (&genlz->font);
  orthconn_destroy (&genlz->orth);
}


static DiaObject *
generalization_load(ObjectNode obj_node, int version,DiaContext *ctx)
{
  DiaObject *obj = object_load_using_properties(&generalization_type,
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
