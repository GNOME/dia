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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>

#include "intl.h"
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
  Alignment text_align;
  real text_width;
  
  char *name;
  char *stereotype; /* excluding << and >> */
  char *st_stereotype; /* including << and >> */
};

#define GENERALIZATION_WIDTH 0.1
#define GENERALIZATION_TRIANGLESIZE 0.8
#define GENERALIZATION_FONTHEIGHT 0.8

static DiaFont *genlz_font = NULL;

static real generalization_distance_from(Generalization *genlz, Point *point);
static void generalization_select(Generalization *genlz, Point *clicked_point,
			      DiaRenderer *interactive_renderer);
static void generalization_move_handle(Generalization *genlz, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void generalization_move(Generalization *genlz, Point *to);
static void generalization_draw(Generalization *genlz, DiaRenderer *renderer);
static Object *generalization_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void generalization_destroy(Generalization *genlz);
static DiaMenu *generalization_get_object_menu(Generalization *genlz,
						Point *clickedpoint);

static PropDescription *generalization_describe_props(Generalization *generalization);
static void generalization_get_props(Generalization * generalization, GPtrArray *props);
static void generalization_set_props(Generalization * generalization, GPtrArray *props);

static Object *generalization_load(ObjectNode obj_node, int version,
				   const char *filename);

static void generalization_update_data(Generalization *genlz);

static ObjectTypeOps generalization_type_ops =
{
  (CreateFunc) generalization_create,
  (LoadFunc)   generalization_load,/*using_properties*/     /* load */
  (SaveFunc)   object_save_using_properties,      /* save */
  (GetDefaultsFunc)   NULL, 
  (ApplyDefaultsFunc) NULL
};

ObjectType generalization_type =
{
  "UML - Generalization",   /* name */
  0,                      /* version */
  (char **) generalization_xpm,  /* pixmap */
  
  &generalization_type_ops,      /* ops */
  NULL,                 /* pixmap_file */
  0                     /* default_user_data */
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
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      generalization_get_object_menu,
  (DescribePropsFunc)   generalization_describe_props,
  (GetPropsFunc)        generalization_get_props,
  (SetPropsFunc)        generalization_set_props
};

static PropDescription generalization_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "name", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Name:"), NULL, NULL },
  { "stereotype", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Stereotype:"), NULL, NULL },
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
  { NULL, 0, 0 }
};

static void
generalization_get_props(Generalization * generalization, GPtrArray *props)
{
  object_get_props_from_offsets(&generalization->orth.object,
                                generalization_offsets,props);
}

static void
generalization_set_props(Generalization *generalization, GPtrArray *props)
{
  object_set_props_from_offsets(&generalization->orth.object, 
                                generalization_offsets, props);
  g_free(generalization->st_stereotype);
  generalization->st_stereotype = NULL;
  generalization_update_data(generalization);
}

static real
generalization_distance_from(Generalization *genlz, Point *point)
{
  OrthConn *orth = &genlz->orth;
  return orthconn_distance_from(orth, point, GENERALIZATION_WIDTH);
}

static void
generalization_select(Generalization *genlz, Point *clicked_point,
		  DiaRenderer *interactive_renderer)
{
  orthconn_update_data(&genlz->orth);
}

static void
generalization_move_handle(Generalization *genlz, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(genlz!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&genlz->orth, handle, to, reason);
  generalization_update_data(genlz);
}

static void
generalization_move(Generalization *genlz, Point *to)
{
  orthconn_move(&genlz->orth, to);
  generalization_update_data(genlz);
}

static void
generalization_draw(Generalization *genlz, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  OrthConn *orth = &genlz->orth;
  Point *points;
  int n;
  Point pos;
  Arrow arrow;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer_ops->set_linewidth(renderer, GENERALIZATION_WIDTH);
  renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  arrow.type = ARROW_HOLLOW_TRIANGLE;
  arrow.length = GENERALIZATION_TRIANGLESIZE;
  arrow.width = GENERALIZATION_TRIANGLESIZE;

  renderer_ops->draw_polyline_with_arrows(renderer,
					   points, n,
					   GENERALIZATION_WIDTH,
					   &color_black,
					   &arrow, NULL);

  renderer_ops->set_font(renderer, genlz_font, GENERALIZATION_FONTHEIGHT);
  pos = genlz->text_pos;
  
  if (genlz->st_stereotype != NULL && genlz->st_stereotype[0] != '\0') {
    renderer_ops->draw_string(renderer,
			       genlz->st_stereotype,
			       &pos, genlz->text_align,
			       &color_black);

    pos.y += GENERALIZATION_FONTHEIGHT;
  }
  
  if (genlz->name != NULL && genlz->name[0] != '\0') {
    renderer_ops->draw_string(renderer,
			       genlz->name,
			       &pos, genlz->text_align,
			       &color_black);
  }
  
}

static void
generalization_update_data(Generalization *genlz)
{
  OrthConn *orth = &genlz->orth;
  Object *obj = &orth->object;
  int num_segm, i;
  Point *points;
  Rectangle rect;
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
    genlz->text_width = dia_font_string_width(genlz->name, genlz_font,
                                              GENERALIZATION_FONTHEIGHT);
    descent = dia_font_descent(genlz->name,
                               genlz_font,GENERALIZATION_FONTHEIGHT);
    ascent = dia_font_ascent(genlz->name,
                             genlz_font,GENERALIZATION_FONTHEIGHT);
  }
  if (genlz->stereotype) {
    genlz->text_width = MAX(genlz->text_width,
                            dia_font_string_width(genlz->stereotype,
                                                  genlz_font,
                                                  GENERALIZATION_FONTHEIGHT));
    if (!genlz->name) {
      descent = dia_font_descent(genlz->stereotype,
                                genlz_font,GENERALIZATION_FONTHEIGHT);
    }
    ascent = dia_font_ascent(genlz->stereotype,
                             genlz_font,GENERALIZATION_FONTHEIGHT);
  }
  
  extra = &orth->extra_spacing;
  
  extra->start_trans = GENERALIZATION_WIDTH/2.0 + GENERALIZATION_TRIANGLESIZE;
  extra->start_long = 
    extra->middle_trans = 
    extra->end_trans = 
    extra->end_long = GENERALIZATION_WIDTH/2.0;
  
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
    genlz->text_align = ALIGN_CENTER;
    genlz->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    genlz->text_pos.y = points[i].y - descent;
    break;
  case VERTICAL:
    genlz->text_align = ALIGN_LEFT;
    genlz->text_pos.x = points[i].x + 0.1;
    genlz->text_pos.y = 0.5*(points[i].y+points[i+1].y) - descent;
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = genlz->text_pos.x;
  if (genlz->text_align == ALIGN_CENTER)
    rect.left -= genlz->text_width/2.0;
  rect.right = rect.left + genlz->text_width;
  rect.top = genlz->text_pos.y - ascent;
  rect.bottom = rect.top + 2*GENERALIZATION_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static ObjectChange *
generalization_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  generalization_update_data((Generalization *)obj);
  return change;
}

static ObjectChange *
generalization_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  generalization_update_data((Generalization *)obj);
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

static Object *
generalization_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Generalization *genlz;
  OrthConn *orth;
  Object *obj;
  PolyBBExtras *extra;

  if (genlz_font == NULL) {
    genlz_font = dia_font_new_from_style(DIA_FONT_MONOSPACE, GENERALIZATION_FONTHEIGHT);
  }
  
  genlz = g_new0(Generalization, 1);
  orth = &genlz->orth;
  obj = (Object *)genlz;
  extra = &orth->extra_spacing;

  obj->type = &generalization_type;

  obj->ops = &generalization_ops;

  orthconn_init(orth, startpoint);

  genlz->name = NULL;
  genlz->stereotype = NULL;
  genlz->st_stereotype = NULL;

  generalization_update_data(genlz);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];

  return (Object *)genlz;
}

static void
generalization_destroy(Generalization *genlz)
{
  g_free(genlz->name);
  g_free(genlz->stereotype);
  g_free(genlz->st_stereotype);

  orthconn_destroy(&genlz->orth);
}

static Object *
generalization_load(ObjectNode obj_node, int version,
		    const char *filename)
{
  return object_load_using_properties(&generalization_type,
                                      obj_node,version,filename);
}
