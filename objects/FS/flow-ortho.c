/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
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

/* If you have a problem with the Function Structure (FS) components,
 * please send e-mail to David Thompson <dcthomp@mail.utexas.edu>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "intl.h"
#include "object.h"
#include "objchange.h"
#include "connection.h"
#include "diarenderer.h"
#include "handle.h"
#include "arrows.h"
#include "diamenu.h"
#include "text.h"
#include "orth_conn.h"
#include "element.h"
#include "properties.h"

#include "pixmaps/orthflow.xpm"


typedef struct _Orthflow Orthflow;
typedef struct _OrthflowChange OrthflowChange;

typedef enum {
  ORTHFLOW_ENERGY,
  ORTHFLOW_MATERIAL,
  ORTHFLOW_SIGNAL
} OrthflowType;

struct _Orthflow {
  OrthConn orth ;

  Handle text_handle;

  Text* text;
  TextAttributes attrs;
  OrthflowType type;
  Point textpos; /* This is the master position, only overridden in load */
};

enum OrthflowChangeType {
  TEXT_EDIT=1,
  FLOW_TYPE=2,
  BOTH=3
};

struct _OrthflowChange {
  ObjectChange			obj_change ;
  enum OrthflowChangeType	change_type ;
  OrthflowType			type ;
  char*				text ;
};

Color orthflow_color_energy   = { 1.0f, 0.0f, 0.0f };
Color orthflow_color_material = { 0.8f, 0.0f, 0.8f };
Color orthflow_color_signal   = { 0.0f, 0.0f, 1.0f };

  
#define ORTHFLOW_WIDTH 0.1
#define ORTHFLOW_MATERIAL_WIDTH 0.2
#define ORTHFLOW_DASHLEN 0.4
#define ORTHFLOW_FONTHEIGHT 0.6
#define ORTHFLOW_ARROWLEN 0.8
#define ORTHFLOW_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static DiaFont *orthflow_font = NULL;

static ObjectChange* orthflow_move_handle(Orthflow *orthflow, Handle *handle,
					  Point *to, ConnectionPoint *cp,
					  HandleMoveReason reason, 
					  ModifierKeys modifiers);
static ObjectChange* orthflow_move(Orthflow *orthflow, Point *to);
static void orthflow_select(Orthflow *orthflow, Point *clicked_point,
			    DiaRenderer *interactive_renderer);
static void orthflow_draw(Orthflow *orthflow, DiaRenderer *renderer);
static DiaObject *orthflow_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static real orthflow_distance_from(Orthflow *orthflow, Point *point);
static void orthflow_update_data(Orthflow *orthflow);
static void orthflow_destroy(Orthflow *orthflow);
static DiaObject *orthflow_copy(Orthflow *orthflow);
static PropDescription *orthflow_describe_props(Orthflow *mes);
static void
orthflow_get_props(Orthflow * orthflow, GPtrArray *props);
static void
orthflow_set_props(Orthflow * orthflow, GPtrArray *props);
static void orthflow_save(Orthflow *orthflow, ObjectNode obj_node,
			  const char *filename);
static DiaObject *orthflow_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *orthflow_get_object_menu(Orthflow *orthflow, Point *clickedpoint) ;


static ObjectTypeOps orthflow_type_ops =
{
  (CreateFunc)		orthflow_create,
  (LoadFunc)		orthflow_load,
  (SaveFunc)		orthflow_save,
  (GetDefaultsFunc)	NULL,
  (ApplyDefaultsFunc)	NULL,
  
} ;

DiaObjectType orthflow_type =
{
  "FS - Orthflow",		/* name */
  0,				/* version */
  (char **) orthflow_xpm,	/* pixmap */
  &orthflow_type_ops		/* ops */
};

static ObjectOps orthflow_ops = {
  (DestroyFunc)         orthflow_destroy,
  (DrawFunc)            orthflow_draw,
  (DistanceFunc)        orthflow_distance_from,
  (SelectFunc)          orthflow_select,
  (CopyFunc)            orthflow_copy,
  (MoveFunc)            orthflow_move,
  (MoveHandleFunc)      orthflow_move_handle,
  (GetPropertiesFunc)   object_create_props_dialog,
  (ApplyPropertiesFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)      orthflow_get_object_menu,
  (DescribePropsFunc)   orthflow_describe_props,
  (GetPropsFunc)        orthflow_get_props,
  (SetPropsFunc)        orthflow_set_props,
};

static PropEnumData prop_orthflow_type_data[] = {
  { N_("Energy"),ORTHFLOW_ENERGY },
  { N_("Material"),ORTHFLOW_MATERIAL },
  { N_("Signal"),ORTHFLOW_SIGNAL },
  { NULL, 0 } 
};

static PropDescription orthflow_props[] = {
  ELEMENT_COMMON_PROPERTIES,
  { "type", PROP_TYPE_ENUM, PROP_FLAG_VISIBLE,
    N_("Type:"), NULL, prop_orthflow_type_data },
  { "text", PROP_TYPE_TEXT, 0, NULL, NULL },
  PROP_STD_TEXT_ALIGNMENT,
  PROP_STD_TEXT_FONT,
  PROP_STD_TEXT_HEIGHT,
  /* Colour determined from type, don't show */
  { "text_colour", PROP_TYPE_COLOUR, PROP_FLAG_DONT_SAVE, },
  PROP_DESC_END
};

static PropDescription *
orthflow_describe_props(Orthflow *mes)
{
  if (orthflow_props[0].quark == 0)
    prop_desc_list_calculate_quarks(orthflow_props);
  return orthflow_props;

}

static PropOffset orthflow_offsets[] = {
  OBJECT_COMMON_PROPERTIES_OFFSETS,
  { "type", PROP_TYPE_ENUM, offsetof(Orthflow, type) },
  { "text", PROP_TYPE_TEXT, offsetof (Orthflow, text) },
  { "text_alignment", PROP_TYPE_REAL, offsetof (Orthflow, attrs.alignment) },
  { "text_font", PROP_TYPE_FONT, offsetof (Orthflow, attrs.font) },
  { "text_height", PROP_TYPE_REAL, offsetof (Orthflow, attrs.height) },
  { "text_colour", PROP_TYPE_COLOUR, offsetof (Orthflow, attrs.color) },
  { NULL, 0, 0 }
};

static void
orthflow_get_props(Orthflow * orthflow, GPtrArray *props)
{
  text_get_attributes (orthflow->text, &orthflow->attrs);
  object_get_props_from_offsets(&orthflow->orth.object, 
                                orthflow_offsets, props);
}

static void
orthflow_set_props(Orthflow *orthflow, GPtrArray *props)
{
  object_set_props_from_offsets(&orthflow->orth.object, 
                                orthflow_offsets, props);
  apply_textattr_properties (props, orthflow->text, "text", &orthflow->attrs);
  orthflow_update_data(orthflow);
}



static void
orthflow_change_apply_revert(ObjectChange* objchg, DiaObject* obj)
{
  struct _OrthflowChange* change = (struct _OrthflowChange*) objchg ;
  Orthflow* oflow = (Orthflow*) obj ;

  if ( change->change_type == FLOW_TYPE || change->change_type == BOTH ) {
    OrthflowType type = oflow->type ;
    oflow->type = change->type ;
    change->type = type ;
    orthflow_update_data(oflow) ;
  }

  if ( change->change_type & TEXT_EDIT  || change->change_type == BOTH ) {
    char* tmp = text_get_string_copy( oflow->text ) ;
    text_set_string( oflow->text, change->text ) ;
    g_free( change->text ) ;
    change->text = tmp ;
  }
}

static void
orthflow_change_free(ObjectChange* objchg)
{
  struct _OrthflowChange* change = (struct _OrthflowChange*) objchg ;

  if (change->change_type & TEXT_EDIT  || change->change_type == BOTH ) {
    g_free(change->text) ;
  }
}

static ObjectChange*
orthflow_create_change( enum OrthflowChangeType change_type,
			OrthflowType type, Text* text )
{
  struct _OrthflowChange* change ;
  change = g_new0( struct _OrthflowChange, 1 ) ;
  change->obj_change.apply = (ObjectChangeApplyFunc) orthflow_change_apply_revert ;
  change->obj_change.revert =  (ObjectChangeRevertFunc) orthflow_change_apply_revert ;
  change->obj_change.free =  (ObjectChangeFreeFunc) orthflow_change_free ;
  change->change_type = change_type ;

  change->type = type ;
  if ( text ) {
    change->text = text_get_string_copy( text ) ;
  }

  return (ObjectChange*) change ;
}

static real
orthflow_distance_from(Orthflow *orthflow, Point *point)
{
  real linedist;
  real textdist;

  linedist = orthconn_distance_from( &orthflow->orth, point, 
				     orthflow->type == ORTHFLOW_MATERIAL ? 
				     ORTHFLOW_MATERIAL_WIDTH : 
				     ORTHFLOW_WIDTH ) ;
  textdist = text_distance_from( orthflow->text, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
orthflow_select(Orthflow *orthflow, Point *clicked_point,
		DiaRenderer *interactive_renderer)
{
  text_set_cursor(orthflow->text, clicked_point, interactive_renderer);
  text_grab_focus(orthflow->text, &orthflow->orth.object);

  orthconn_update_data(&orthflow->orth);
}

static ObjectChange*
orthflow_move_handle(Orthflow *orthflow, Handle *handle,
		     Point *to, ConnectionPoint *cp,
		     HandleMoveReason reason, ModifierKeys modifiers)
{
  ObjectChange *change = NULL;
  assert(orthflow!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    orthflow->textpos = *to;
  } else {
    Point along ;

    along = orthflow->textpos ;
    point_sub( &along, &(orthconn_get_middle_handle(&orthflow->orth)->pos) ) ;

    change = orthconn_move_handle( &orthflow->orth, handle, to, cp, 
				   reason, modifiers);
    orthconn_update_data( &orthflow->orth ) ;

    orthflow->textpos = orthconn_get_middle_handle(&orthflow->orth)->pos ;
    point_add( &orthflow->textpos, &along ) ;
  }

  orthflow_update_data(orthflow);

  return change;
}

static ObjectChange*
orthflow_move(Orthflow *orthflow, Point *to)
{
  ObjectChange *change;

  Point *points = &orthflow->orth.points[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &points[0]);
  point_add(&orthflow->textpos, &delta);

  change = orthconn_move( &orthflow->orth, to ) ;

  orthflow_update_data(orthflow);

  return change;
}

static void
orthflow_draw(Orthflow *orthflow, DiaRenderer *renderer)
{
  DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  int n = orthflow->orth.numpoints ;
  Color* render_color = &orthflow_color_signal;
  Point *points;
  real linewidth;
  Arrow arrow;

  assert(orthflow != NULL);
  assert(renderer != NULL);

  arrow.type = ARROW_FILLED_TRIANGLE;
  arrow.width = ORTHFLOW_ARROWWIDTH;
  arrow.length = ORTHFLOW_ARROWLEN;
 
  points = &orthflow->orth.points[0];

  renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);

  switch (orthflow->type) {
  case ORTHFLOW_SIGNAL:
    linewidth = ORTHFLOW_WIDTH;
    renderer_ops->set_dashlength(renderer, ORTHFLOW_DASHLEN);
    renderer_ops->set_linestyle(renderer, LINESTYLE_DASHED);
    render_color = &orthflow_color_signal ;
    break ;
  case ORTHFLOW_MATERIAL:
    linewidth = ORTHFLOW_MATERIAL_WIDTH;
    renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
    render_color = &orthflow_color_material ;
    break ;
  case ORTHFLOW_ENERGY:
    linewidth = ORTHFLOW_WIDTH;
    renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID);
    render_color = &orthflow_color_energy ;
    break ;
  default:
    linewidth = 0.001;
    break;
  }

  renderer_ops->set_linewidth(renderer, linewidth);
  renderer_ops->draw_polyline_with_arrows(renderer, points, n,
					  ORTHFLOW_WIDTH,
					  render_color,
					  NULL, &arrow); 
#if 0
  arrow_draw(renderer, 
	     &points[n-1], &points[n-2],
	     ORTHFLOW_ARROWLEN, ORTHFLOW_ARROWWIDTH, 
	     render_color, &color_white);
#endif

  renderer_ops->set_font(renderer, orthflow_font,
                          ORTHFLOW_FONTHEIGHT);

  text_draw(orthflow->text, renderer);
}

static DiaObject *
orthflow_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Orthflow *orthflow;
  OrthConn *orth;
  DiaObject *obj;
  Point p;
  PolyBBExtras *extra;
  DiaFont *font;

  orthflow = g_new0(Orthflow,1);
  orth = &orthflow->orth ;
  orthconn_init( orth, startpoint ) ;
 
  obj = &orth->object;
  extra = &orth->extra_spacing;
  
  obj->type = &orthflow_type;
  obj->ops = &orthflow_ops;

  /* Where to put the text */
  p = *startpoint ;
  p.y += 0.1 * ORTHFLOW_FONTHEIGHT ;
  orthflow->textpos = p;
  font = dia_font_new_from_style(DIA_FONT_SANS, 0.8);

  orthflow->text = new_text("", font, 0.8, &p, &color_black, ALIGN_CENTER);
  dia_font_unref(font);  
  text_get_attributes(orthflow->text, &orthflow->attrs);

#if 0
  if ( orthflow_default_label ) {
    orthflow->text = text_copy( orthflow_default_label ) ;
    text_set_position( orthflow->text, &p ) ;
  } else {
    Color* color = &orthflow_color_signal;

    if (orthflow_font == NULL) {
	    orthflow_font = dia_font_new_from_style(DIA_FONT_SANS|DIA_FONT_ITALIC,
                                              1.0);
    }

    switch (orthflow->type) {
    case ORTHFLOW_ENERGY:
      color = &orthflow_color_energy ;
      break ;
    case ORTHFLOW_MATERIAL:
      color = &orthflow_color_material ;
      break ;
    case ORTHFLOW_SIGNAL:
      color = &orthflow_color_signal ;
      break ;
    }

    orthflow->text = new_text("", orthflow_font, ORTHFLOW_FONTHEIGHT, 
                              &p, color, ALIGN_CENTER);
  }
#endif

  orthflow->text_handle.id = HANDLE_MOVE_TEXT;
  orthflow->text_handle.type = HANDLE_MINOR_CONTROL;
  orthflow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  orthflow->text_handle.connected_to = NULL;
  object_add_handle( obj, &orthflow->text_handle ) ;

  extra->start_long = 
    extra->start_trans = 
    extra->middle_trans = ORTHFLOW_WIDTH/2.0;
  extra->end_long = 
    extra->end_trans = ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
    
  orthflow_update_data(orthflow);

  /*obj->handles[1] = orth->handles[2] ;*/
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &orthflow->orth.object;
}


static void
orthflow_destroy(Orthflow *orthflow)
{
  orthconn_destroy( &orthflow->orth ) ;
  text_destroy( orthflow->text ) ;
}

static DiaObject *
orthflow_copy(Orthflow *orthflow)
{
  Orthflow *neworthflow;
  OrthConn *orth, *neworth;
  DiaObject *newobj;

  orth = &orthflow->orth;
  
  neworthflow = g_malloc0(sizeof(Orthflow));
  neworth = &neworthflow->orth;
  newobj = &neworth->object;

  orthconn_copy(orth, neworth);

  neworthflow->text_handle = orthflow->text_handle;
  newobj->handles[orth->numpoints-1] = &neworthflow->text_handle;

  neworthflow->text = text_copy(orthflow->text);
  neworthflow->type = orthflow->type;

  return &neworthflow->orth.object;
}


static void
orthflow_update_data(Orthflow *orthflow)
{
  OrthConn *orth = &orthflow->orth ;
  DiaObject *obj = &orth->object;
  Rectangle rect;
  Color* color = &orthflow_color_signal;
  
  switch (orthflow->type) {
  case ORTHFLOW_ENERGY:
    color = &orthflow_color_energy ;
    break ;
  case ORTHFLOW_MATERIAL:
    color = &orthflow_color_material ;
    break ;
  case ORTHFLOW_SIGNAL:
    color = &orthflow_color_signal ;
    break ;
  }
  text_set_color( orthflow->text, color ) ;

  text_set_position( orthflow->text, &orthflow->textpos ) ;
  orthflow->text_handle.pos = orthflow->textpos;

  orthconn_update_data(orth);
  obj->position = orth->points[0];

  /* Boundingbox: */
  orthconn_update_boundingbox(orth);

  /* Add boundingbox for text: */
  text_calc_boundingbox(orthflow->text, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);
}


static void
orthflow_save(Orthflow *orthflow, ObjectNode obj_node, const char *filename)
{
  orthconn_save(&orthflow->orth, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		orthflow->text) ;
  data_add_int(new_attribute(obj_node, "type"),
	       orthflow->type);
}

static DiaObject *
orthflow_load(ObjectNode obj_node, int version, const char *filename)
{
  Orthflow *orthflow;
  AttributeNode attr;
  OrthConn *orth;
  DiaObject *obj;
  PolyBBExtras *extra;

  if (orthflow_font == NULL) {
    orthflow_font = dia_font_new_from_style(DIA_FONT_SANS|DIA_FONT_ITALIC,
                                            1.0);
  }

  orthflow = g_malloc0(sizeof(Orthflow));

  orth = &orthflow->orth;
  obj = &orth->object;
  extra = &orth->extra_spacing;
  
  obj->type = &orthflow_type;
  obj->ops = &orthflow_ops;

  orthconn_load(orth, obj_node);

  orthflow->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    orthflow->text = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    orthflow->type = (OrthflowType)data_int(attribute_first_data(attr));

  orthflow->text_handle.id = HANDLE_MOVE_TEXT;
  orthflow->text_handle.type = HANDLE_MINOR_CONTROL;
  orthflow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  orthflow->text_handle.connected_to = NULL;
  /* Mein Gott!  The extra handle was never added */
  object_add_handle(obj, &orthflow->text_handle);
  obj->handles[orth->numpoints-1] = &orthflow->text_handle;

  extra->start_long = 
    extra->start_trans = 
    extra->middle_trans = ORTHFLOW_WIDTH/2.0;
  extra->end_long = 
    extra->end_trans = ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
  orthflow->textpos = orthflow->text->position;

  orthflow_update_data(orthflow);
  
  return &orthflow->orth.object;
}

static ObjectChange *
orthflow_set_type_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  ObjectChange* change ;
  change = orthflow_create_change( FLOW_TYPE, ((Orthflow*)obj)->type, 0 ) ;

  ((Orthflow*)obj)->type = (int) data ;
  orthflow_update_data((Orthflow*)obj);

  return change;
}

static ObjectChange *
orthflow_segment_callback (DiaObject* obj, Point* clicked, gpointer data)
{
  if ( (int)data )
     return orthconn_add_segment( (OrthConn*)obj, clicked ) ;

  return orthconn_delete_segment( (OrthConn*)obj, clicked ) ;
}

static DiaMenuItem orthflow_menu_items[] = {
  { N_("Energy"), orthflow_set_type_callback, (void*)ORTHFLOW_ENERGY, 1 },
  { N_("Material"), orthflow_set_type_callback, (void*)ORTHFLOW_MATERIAL, 1 },
  { N_("Signal"), orthflow_set_type_callback, (void*)ORTHFLOW_SIGNAL, 1 },
  { N_("Add segment"), orthflow_segment_callback, (void*)1, 1 },
  { N_("Delete segment"), orthflow_segment_callback, (void*)0, 1 },
  ORTHCONN_COMMON_MENUS,
};

static DiaMenu orthflow_menu = {
  "Orthflow",
  sizeof(orthflow_menu_items)/sizeof(DiaMenuItem),
  orthflow_menu_items,
  NULL
};

static DiaMenu *
orthflow_get_object_menu(Orthflow *orthflow, Point *clickedpoint)
{
  orthflow_menu_items[4].active = orthflow->orth.numpoints > 3;
  orthconn_update_object_menu((OrthConn*)orthflow,
			      clickedpoint, &orthflow_menu_items[5]);
  return &orthflow_menu;
}

