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
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "arrows.h"
#include "text.h"

#include "properties.h"
#include "eml.h"

#include "pixmaps/instantiation.xpm"

typedef struct _Instantiation Instantiation;
typedef struct _InstantiationState InstantiationState;

typedef enum {
  INST_SINGLE,
  INST_MULTI
} InstantiationType;

struct _Instantiation {
  OrthConn orth;

  Handle reson_handle;

  Point procnum_pos;
  Alignment procnum_align;
  real procnum_width;
  
  InstantiationType type;
  gchar *procnum;
  Text *reson;
};

#define INSTANTIATION_WIDTH 0.1
#define INSTANTIATION_TRIANGLESIZE 0.8
#define INSTANTIATION_ELLIPSE 0.6
#define INSTANTIATION_FONTHEIGHT 0.8
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static DiaFont *inst_font = NULL;

static real instantiation_distance_from(Instantiation *inst, Point *point);
static void instantiation_select(Instantiation *inst, Point *clicked_point,
			      Renderer *interactive_renderer);
static ObjectChange* instantiation_move_handle(Instantiation *inst, Handle *handle,
					       Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* instantiation_move(Instantiation *inst, Point *to);
static void instantiation_draw(Instantiation *inst, Renderer *renderer);
static DiaObject *instantiation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void instantiation_destroy(Instantiation *inst);

static DiaMenu *instantiation_get_object_menu(Instantiation *inst,
						Point *clickedpoint);

static DiaObject *instantiation_load(ObjectNode obj_node, int version,
				   const char *filename);

static void instantiation_update_data(Instantiation *inst);

static PropDescription *instantiation_describe_props(Instantiation *inst);
static void instantiation_get_props(Instantiation *inst, 
                                    Property *props, guint nprops);
static void instantiation_set_props(Instantiation *inst, 
                                    Property *props, guint nprops);


static ObjectTypeOps instantiation_type_ops =
{
  (CreateFunc) instantiation_create,
  (LoadFunc)   instantiation_load,
  (SaveFunc)   object_save_using_properties,
  (GetDefaultsFunc) NULL,
  (ApplyDefaultsFunc) NULL,
};

ObjectType instantiation_type =
{
  "EML - Instantiation",   /* name */
  0,                      /* version */
  (char **) instantiation_xpm,  /* pixmap */
  
  &instantiation_type_ops       /* ops */
};

static ObjectOps instantiation_ops = {
  (DestroyFunc)         instantiation_destroy,
  (DrawFunc)            instantiation_draw,
  (DistanceFunc)        instantiation_distance_from,
  (SelectFunc)          instantiation_select,
  (CopyFunc)            object_copy_using_properties,
  (MoveFunc)            instantiation_move,
  (MoveHandleFunc)      instantiation_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      instantiation_get_object_menu,
  (DescribePropsFunc)   instantiation_describe_props,
  (GetPropsFunc)        instantiation_get_props,
  (SetPropsFunc)        instantiation_set_props
};

static PropDescription instantiation_props[] = {
  ORTHCONN_COMMON_PROPERTIES,
  { "procnum", PROP_TYPE_STRING, PROP_FLAG_VISIBLE,
    N_("Number of processes:"), NULL },
  { "type", PROP_TYPE_BOOL, PROP_FLAG_VISIBLE,
    N_("multiple"),NULL },
  { "reson",PROP_TYPE_TEXT,0,NULL,NULL},
  PROP_DESC_END
};

static PropDescription *
instantiation_describe_props(Instantiation *inst) 
{
  if (instantiation_props[0].quark == 0) {
    prop_desc_list_calculate_quarks(instantiation_props);
  }
  return instantiation_props;
}    

static PropOffset instantiation_offsets[] = {
  ORTHCONN_COMMON_PROPERTIES_OFFSETS,
  { "procnum", PROP_TYPE_STRING, offsetof(Instantiation,procnum) },
  { "type", PROP_TYPE_BOOL, offsetof(Instantiation,type) },
  { "reson",PROP_TYPE_TEXT, offsetof(Instantiation,reson) },
  { NULL, 0, 0 }
};

static void
instantiation_get_props(Instantiation *inst, Property *props, guint nprops)
{  
  object_get_props_from_offsets(&inst->orth.object,
                                instantiation_offsets,props,nprops);
}

static void
instantiation_set_props(Instantiation *inst, Property *props, guint nprops)
{
  object_set_props_from_offsets(&inst->orth.object,
                                instantiation_offsets,props,nprops);
  instantiation_update_data(inst);
}

static real
instantiation_distance_from(Instantiation *inst, Point *point)
{
  OrthConn *orth = &inst->orth;
  real linedist;
  real textdist;

  linedist = orthconn_distance_from(orth, point, INSTANTIATION_WIDTH);

  textdist = text_distance_from( inst->reson, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
instantiation_select(Instantiation *inst, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  text_set_cursor(inst->reson, clicked_point, interactive_renderer);
  text_grab_focus(inst->reson, &inst->orth.object);

  orthconn_update_data(&inst->orth);
}

static ObjectChange*
instantiation_move_handle(Instantiation *inst, Handle *handle,
                          Point *to, HandleMoveReason reason, 
                          ModifierKeys modifiers)
{
  ObjectChange *change = NULL;
  g_assert(inst!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    inst->reson->position = *to;
  }
  else {
    Point along ;

    along = inst->reson->position ;
    point_sub( &along, &(orthconn_get_middle_handle(&inst->orth)->pos) ) ;

    change = orthconn_move_handle( &inst->orth, handle, to, reason );
    orthconn_update_data( &inst->orth ) ;

    inst->reson->position = orthconn_get_middle_handle(&inst->orth)->pos ;
    point_add( &inst->reson->position, &along ) ;
  }

  instantiation_update_data(inst);
  

  return change;
}

static ObjectChange*
instantiation_move(Instantiation *inst, Point *to)
{
  ObjectChange *change;

  Point *points = &inst->orth.points[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &points[0]);
  point_add(&inst->reson->position, &delta);

  change = orthconn_move( &inst->orth, to ) ;

  instantiation_update_data(inst);
  /*
    Why is this being done twice?
  orthconn_move(&inst->orth, to);
  instantiation_update_data(inst);
  */

  return change;
}

static void
instantiation_draw(Instantiation *inst, Renderer *renderer)
{
  OrthConn *orth = &inst->orth;
  Point *points;
  int n;
  Point secpos;
  Point norm;
  Point *dest;

  points = &orth->points[0];
  n = orth->numpoints;

  dest = &points[1];
  point_copy(&norm, dest);
  point_sub(&norm, &points[0]);
  point_get_normed(&norm, &norm);

  renderer->ops->set_linewidth(renderer, INSTANTIATION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  point_copy(&norm, dest);
  point_sub(&norm, &points[0]);
  point_get_normed(&norm, &norm);

  if (inst->type == INST_MULTI) {
    arrow_draw(renderer, ARROW_FILLED_ELLIPSE,
               &points[0], dest,
               INSTANTIATION_ELLIPSE, INSTANTIATION_ELLIPSE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    point_copy_add_scaled(&secpos, &points[0], &norm, 0.6);
    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    point_copy_add_scaled(&secpos, &points[0], &norm, 1.1);
    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

  }
  else {
    point_copy_add_scaled(&secpos, &points[0], &norm, 0.5);

    arrow_draw(renderer, ARROW_LINES,
               &points[0], dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);
  }
  renderer->ops->set_font(renderer, inst_font, INSTANTIATION_FONTHEIGHT);
  
  text_draw(inst->reson, renderer);
  
  if (inst->procnum != NULL) {
    renderer->ops->draw_string(renderer,
			       inst->procnum,
			       &inst->procnum_pos, inst->procnum_align,
			       &color_black);
  }
  
}

static void
instantiation_update_data(Instantiation *inst)
{
  OrthConn *orth = &inst->orth;
  DiaObject *obj = &orth->object;
  int i, cur_segm;
  real dist;
  Point *points;
  Rectangle rect;

  inst->reson_handle.pos = inst->reson->position;

  orthconn_update_data(orth);
  obj->position = orth->points[0];

  
  orthconn_update_boundingbox(orth);
  
  /* FIXME: this object is really Hibernatus !!! we have to use the BBExtras 
     now */
  
  /* fix boundinginstantiation for linewidth and triangle: */
  obj->bounding_box.top -= INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.left -= INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.bottom += INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.right += INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  
  points = inst->orth.points;
  dist = distance_point_point(&points[0], &points[1]);
  if (dist <= 0.8) {
    cur_segm = 1;
    i = 1;
  }
  else {
    i = 0;
    cur_segm = 0;
  }

  switch (inst->orth.orientation[cur_segm]) {
  case HORIZONTAL:
    inst->procnum_pos.y = points[i].y - font_descent(inst_font, INSTANTIATION_FONTHEIGHT);

    if (points[i].x <= points[i+1].x) {
      inst->procnum_align = ALIGN_LEFT;
      inst->procnum_pos.x = points[i].x + 1.0;
    }
    else {
      inst->procnum_align = ALIGN_RIGHT;
      inst->procnum_pos.x = points[i].x - 1.0;
    }
    break;
  case VERTICAL:
    inst->procnum_align = ALIGN_LEFT;
    inst->procnum_pos.x = points[i].x + 0.8;

    if (points[i].y <= points[i+1].y) {
      inst->procnum_pos.y = points[i].y + 0.8;
    }
    else {
      inst->procnum_pos.y = points[i].y - 0.8;
    }

    break;
  }

  if (inst->procnum_align == ALIGN_RIGHT) {
    rect.right = inst->procnum_pos.x;
    rect.left = rect.right - inst->procnum_width;
  }
  else {
    rect.left = inst->procnum_pos.x;
    rect.right = rect.left + inst->procnum_width;
  }

  rect.top = inst->procnum_pos.y - font_ascent(inst_font, INSTANTIATION_FONTHEIGHT);
  rect.bottom = rect.top + INSTANTIATION_FONTHEIGHT;
    
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for text: */
  text_calc_boundingbox(inst->reson, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);

}

static ObjectChange *
instantiation_add_segment_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  instantiation_update_data((Instantiation *)obj);
  return change;
}

static ObjectChange *
instantiation_delete_segment_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  instantiation_update_data((Instantiation *)obj);
  return change;
}

static ObjectChange *
instantiation_set_type_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  static Property prop = {"type",PROP_TYPE_BOOL};

  prop.d.bool_data = GPOINTER_TO_INT(data) != FALSE;
  
  return object_apply_props(obj,&prop,1);
}

static DiaMenuItem object_menu_items[] = {
  { N_("Single"), instantiation_set_type_callback, 
    GINT_TO_POINTER(INST_SINGLE), 1 },
  { N_("Multiple"), instantiation_set_type_callback,
    GINT_TO_POINTER(INST_MULTI), 1 },
  { N_("Add segment"), instantiation_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), instantiation_delete_segment_callback, NULL, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu object_menu = {
  N_("Instantiation"),
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
instantiation_get_object_menu(Instantiation *inst, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &inst->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[2].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[3].active = orthconn_can_delete_segment(orth, clickedpoint);
  orthconn_update_object_menu(orth, clickedpoint, &object_menu_items[4]);
  return &object_menu;
}

static DiaObject *
instantiation_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Instantiation *inst;
  OrthConn *orth;
  DiaObject *obj;
  Point p;

  if (inst_font == NULL) {
	  /* choose default font name for your locale. see also font_data structure
	     in lib/font.c. if "Courier" works for you, it would be better.  */
	  inst_font = font_getfont(_("Courier"));
  }
  
  inst = g_malloc0(sizeof(Instantiation));
  orth = &inst->orth;
  orthconn_init(orth, startpoint);

  obj = &orth->object;
  
  obj->type = &instantiation_type;

  obj->ops = &instantiation_ops;

  inst->type = INST_SINGLE;
  inst->procnum = NULL;
  inst->procnum_width = 0;

  /* Where to put the text */
  p = *startpoint ;
  p.y += 0.1 * INSTANTIATION_FONTHEIGHT ;

  inst->reson = new_text("", inst_font, INSTANTIATION_FONTHEIGHT, 
			      &p, &color_black, ALIGN_CENTER);

  inst->reson_handle.id = HANDLE_MOVE_TEXT;
  inst->reson_handle.type = HANDLE_MINOR_CONTROL;
  inst->reson_handle.connect_type = HANDLE_NONCONNECTABLE;
  inst->reson_handle.connected_to = NULL;
  object_add_handle( obj, &inst->reson_handle );

  instantiation_update_data(inst);
  
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[2];

  return &inst->orth.object;
}

static void
instantiation_destroy(Instantiation *inst)
{
  orthconn_destroy(&inst->orth);
  text_destroy( inst->reson );
  g_free(inst->procnum);
}

static DiaObject *
instantiation_load(ObjectNode obj_node, int version,
		    const char *filename)
{
  return object_load_using_properties(&instantiation_type,
                                      obj_node,version,filename);
}
