/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * SADT diagram support 
 * Copyright (C) 2000 Cyrille Chepelov
 * 
 * This file has been forked from Alexander Larsson's objects/UML/constraint.c
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
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "intl.h"
#include "object.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "arrows.h"
#include "lazyprops.h"
#include "text.h"

#include "pixmaps/annotation.xpm"

typedef struct _Annotation Annotation;
typedef struct _AnnotationState AnnotationState;
typedef struct _AnnotationDialog AnnotationDialog;
typedef struct _AnnotationDefaultsDialog AnnotationDefaultsDialog;
typedef struct _AnnotationDefaults AnnotationDefaults;

struct _AnnotationState {
  ObjectState obj_state;

  TextAttributes text_attrib;
};

struct _Annotation {
  Connection connection;

  Handle text_handle;

  Text *text;
};

struct _AnnotationDialog {
  AttributeDialog dialog;
  Annotation *parent;
  
  TextFontAttribute text_font;
  TextFontHeightAttribute text_fontheight;
  TextColorAttribute text_color;  
};
  
struct _AnnotationDefaults {
  Font *text_font;
  real text_fontheight;
  Color text_color;
};

struct _AnnotationDefaultsDialog {
  AttributeDialog dialog;
  AnnotationDefaults *parent;
  
  FontAttribute text_font;
  FontHeightAttribute text_fontheight;
  TextColorAttribute text_color;  
};
  
#define ANNOTATION_LINE_WIDTH 0.05
#define ANNOTATION_BORDER 0.2
#define ANNOTATION_FONTHEIGHT 0.8
#define ANNOTATION_ZLEN .25

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static AnnotationDefaults annotation_defaults;
static AnnotationDialog *annotation_dialog;
static AnnotationDefaultsDialog *annotation_defaults_dialog;

static void annotation_move_handle(Annotation *annotation, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void annotation_move(Annotation *annotation, Point *to);
static void annotation_select(Annotation *annotation, Point *clicked_point,
			      Renderer *interactive_renderer);
static void annotation_draw(Annotation *annotation, Renderer *renderer);
static Object *annotation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real annotation_distance_from(Annotation *annotation, Point *point);
static void annotation_update_data(Annotation *annotation);
static void annotation_destroy(Annotation *annotation);
static Object *annotation_copy(Annotation *annotation);
static PROPDLG_TYPE annotation_get_properties(Annotation *annotation);
static ObjectChange *annotation_apply_properties(Annotation *annotation);

static void annotation_init_defaults(void);
static PROPDLG_TYPE annotation_get_defaults(void);
static void annotation_apply_defaults(void);

static AnnotationState *annotation_get_state(Annotation *annotation);
static void annotation_set_state(Annotation *annotation,
				 AnnotationState *state);

static void annotation_save(Annotation *annotation, ObjectNode obj_node,
			    const char *filename);
static Object *annotation_load(ObjectNode obj_node, int version,
			       const char *filename);


static ObjectTypeOps annotation_type_ops =
{
  (CreateFunc) annotation_create,
  (LoadFunc)   annotation_load,
  (SaveFunc)   annotation_save,
  (GetDefaultsFunc)   annotation_get_defaults,
  (ApplyDefaultsFunc) annotation_apply_defaults
};

ObjectType sadtannotation_type =
{
  "SADT - annotation",        /* name */
  0,                         /* version */
  (char **) annotation_xpm,  /* pixmap */
  &annotation_type_ops       /* ops */
};

static ObjectOps annotation_ops = {
  (DestroyFunc)         annotation_destroy,
  (DrawFunc)            annotation_draw,
  (DistanceFunc)        annotation_distance_from,
  (SelectFunc)          annotation_select,
  (CopyFunc)            annotation_copy,
  (MoveFunc)            annotation_move,
  (MoveHandleFunc)      annotation_move_handle,
  (GetPropertiesFunc)   annotation_get_properties,
  (ApplyPropertiesFunc) annotation_apply_properties,
  (ObjectMenuFunc)      NULL
};

static real
annotation_distance_from(Annotation *annotation, Point *point)
{
  Point *endpoints;
  Rectangle bbox;
  endpoints = &annotation->connection.endpoints[0];
  
  text_calc_boundingbox(annotation->text,&bbox);
  return MIN(distance_line_point(&endpoints[0], &endpoints[1], 
				 ANNOTATION_LINE_WIDTH, point),
	     distance_rectangle_point(&bbox,point));
}

static void
annotation_select(Annotation *annotation, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  text_set_cursor(annotation->text, clicked_point, interactive_renderer);
  text_grab_focus(annotation->text, &annotation->connection.object);
  
  connection_update_handles(&annotation->connection);
}

static void
annotation_move_handle(Annotation *annotation, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  Connection *conn = (Connection *)annotation;

  g_assert(annotation!=NULL);
  g_assert(handle!=NULL);
  g_assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    annotation->text->position = *to;
  } else  {
    endpoints = &(conn->endpoints[0]); 
    if (handle->id == HANDLE_MOVE_STARTPOINT) {
      p1 = endpoints[0];
      connection_move_handle(conn, handle->id, to, reason);
      p2 = endpoints[0];
      point_sub(&p2, &p1);
      point_add(&annotation->text->position, &p2);
      point_add(&p2,&(endpoints[1]));
      connection_move_handle(conn, HANDLE_MOVE_ENDPOINT, &p2, reason);   
    } else {      
      p1 = endpoints[1];
      connection_move_handle(conn, handle->id, to, reason);
      p2 = endpoints[1];
      point_sub(&p2, &p1);
      point_add(&annotation->text->position, &p2);
    }
  }
  annotation_update_data(annotation);
}

static void
annotation_move(Annotation *annotation, Point *to)
{
  Point start_to_end;
  Point *endpoints = &annotation->connection.endpoints[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);
 
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&annotation->text->position, &delta);
  
  annotation_update_data(annotation);
}

static void
annotation_draw(Annotation *annotation, Renderer *renderer)
{
  Point *endpoints;
  Point vect,rvect,v1,v2;
  Point pts[4];
  real vlen;

  assert(annotation != NULL);
  assert(renderer != NULL);

  endpoints = &annotation->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, ANNOTATION_LINE_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  vect = annotation->connection.endpoints[1];
  point_sub(&vect,&annotation->connection.endpoints[0]);
  vlen = distance_point_point(&annotation->connection.endpoints[0],
			      &annotation->connection.endpoints[1]);
  if (vlen > 0.0) {
    point_scale(&vect,1/vlen);
    rvect.y = vect.x;
    rvect.x = -vect.y;
  
    pts[0] = annotation->connection.endpoints[0];
    pts[1] = annotation->connection.endpoints[0];
    v1 = vect;
    point_scale(&v1,.5*vlen);
    point_add(&pts[1],&v1);
    pts[2] = pts[1];
    /* pts[1] and pts[2] are currently both at the middle of (pts[0],pts[3]) */
    v1 = vect;
    point_scale(&v1,ANNOTATION_ZLEN);
    v2 = rvect;
    point_scale(&v2,ANNOTATION_ZLEN);
    point_sub(&v1,&v2);
    point_add(&pts[1],&v1);
    point_sub(&pts[2],&v1);
    pts[3] = annotation->connection.endpoints[1];
    renderer->ops->draw_polyline(renderer,
			     pts, sizeof(pts) / sizeof(pts[0]),
			     &color_black);
  }
  text_draw(annotation->text,renderer);
}

static Object *
annotation_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Annotation *annotation;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj; 
  Point offs;
  Point defaultlen = { 1.0, 1.0 };

  annotation_init_defaults();
  
  annotation = g_malloc0(sizeof(Annotation));

  conn = &annotation->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &sadtannotation_type;
  obj->ops = &annotation_ops;
  
  connection_init(conn, 3, 0);

  annotation->text = new_text("", annotation_defaults.text_font,
			      annotation_defaults.text_fontheight,
			      &conn->endpoints[1],
			      &annotation_defaults.text_color,
			      ALIGN_CENTER);
  offs.x = .3 * ANNOTATION_FONTHEIGHT;
  if (conn->endpoints[1].y < conn->endpoints[0].y)
    offs.y = 1.3 * ANNOTATION_FONTHEIGHT;
  else
    offs.y = -.3 * ANNOTATION_FONTHEIGHT;
  point_add(&annotation->text->position,&offs);
  
  annotation->text_handle.id = HANDLE_MOVE_TEXT;
  annotation->text_handle.type = HANDLE_MINOR_CONTROL;
  annotation->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  annotation->text_handle.connected_to = NULL;
  obj->handles[2] = &annotation->text_handle;

  extra->start_trans = 
    extra->end_trans = ANNOTATION_ZLEN;
  extra->start_long = 
    extra->end_long = ANNOTATION_LINE_WIDTH/2.0;
  annotation_update_data(annotation);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return &annotation->connection.object;
}


static void
annotation_destroy(Annotation *annotation)
{
  connection_destroy(&annotation->connection);

  g_free(annotation->text);
}

static Object *
annotation_copy(Annotation *annotation)
{
  Annotation *newannotation;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &annotation->connection;
  
  newannotation = g_malloc0(sizeof(Annotation));
  newconn = &newannotation->connection;
  newobj = &newconn->object;

  connection_copy(conn, newconn);

  newannotation->text_handle = annotation->text_handle;
  newobj->handles[2] = &newannotation->text_handle;


  newannotation->text = text_copy(annotation->text);

  return &newannotation->connection.object;
}

static AnnotationState *
annotation_get_state(Annotation *annotation)
{
  AnnotationState *state = g_new0(AnnotationState, 1);
  state->obj_state.free = NULL;
  text_get_attributes(annotation->text, &state->text_attrib);

  return state;
}

static void
annotation_set_state(Annotation *annotation, AnnotationState *state)
{
  text_set_attributes(annotation->text, &state->text_attrib);
  g_free(state);
  
  annotation_update_data(annotation);
}


static void
annotation_update_data(Annotation *annotation)
{
  Connection *conn = &annotation->connection;
  Object *obj = &conn->object;
  Rectangle textrect;
  
  obj->position = conn->endpoints[0];

  annotation->text_handle.pos = annotation->text->position;

  connection_update_handles(conn);

  connection_update_boundingbox(conn);
  text_calc_boundingbox(annotation->text,&textrect);
  rectangle_union(&obj->bounding_box, &textrect);
}


static void
annotation_save(Annotation *annotation, ObjectNode obj_node,
		const char *filename)
{
  connection_save(&annotation->connection, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		  annotation->text);
}

static Object *
annotation_load(ObjectNode obj_node, int version, const char *filename)
{
  Annotation *annotation;
  Connection *conn;
  ConnectionBBExtras *extra;
  Object *obj;

  annotation_init_defaults();
  
  annotation = g_malloc0(sizeof(Annotation));

  conn = &annotation->connection;
  obj = &conn->object;
  extra = &conn->extra_spacing;

  obj->type = &sadtannotation_type;
  obj->ops = &annotation_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 3, 0);

  annotation->text = load_text(obj_node,"text",NULL);
  if (!annotation->text) {
    annotation->text = new_text("", annotation_defaults.text_font,
				annotation_defaults.text_fontheight,
				&conn->endpoints[1],
				&annotation_defaults.text_color,
				ALIGN_CENTER);
  }
  
  annotation->text_handle.id = HANDLE_MOVE_TEXT;
  annotation->text_handle.type = HANDLE_MINOR_CONTROL;
  annotation->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  annotation->text_handle.connected_to = NULL;
  obj->handles[2] = &annotation->text_handle;
  
  extra->start_trans = 
    extra->end_trans = ANNOTATION_ZLEN;
  extra->start_long = 
    extra->end_long = ANNOTATION_LINE_WIDTH/2.0;
  annotation_update_data(annotation);
  
  return &annotation->connection.object;
}

static void 
annotation_init_defaults(void) {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    annotation_defaults.text_font = font_getfont("Helvetica");
    annotation_defaults.text_fontheight = ANNOTATION_FONTHEIGHT;
    annotation_defaults.text_color = color_black;
    defaults_initialized = 1;
  }
}

static PROPDLG_TYPE
annotation_get_defaults(void)
{
  AnnotationDefaultsDialog *dlg = annotation_defaults_dialog;

  annotation_init_defaults();
  PROPDLG_CREATE(dlg, &annotation_defaults);
  PROPDLG_SHOW_FONT(dlg,text_font,_("Font:"));
  PROPDLG_SHOW_FONTHEIGHT(dlg,text_fontheight,_("Font size:"));
  PROPDLG_SHOW_COLOR(dlg,text_color,_("Font color:"));
  PROPDLG_READY(dlg);
  annotation_defaults_dialog = dlg;

  PROPDLG_RETURN(dlg);
}

static void annotation_apply_defaults(void)
{
  AnnotationDefaultsDialog *dlg = annotation_defaults_dialog;

  PROPDLG_APPLY_FONT(dlg,text_font);
  PROPDLG_APPLY_FONTHEIGHT(dlg,text_fontheight);
  PROPDLG_APPLY_COLOR(dlg,text_color);
}
    
static ObjectChange *
annotation_apply_properties(Annotation *annotation)
{
  AnnotationDialog *dlg = annotation_dialog;
  ObjectState *old_state;
  
  old_state = (ObjectState *)annotation_get_state(annotation);

  PROPDLG_APPLY_TEXTFONT(dlg,text);
  PROPDLG_APPLY_TEXTFONTHEIGHT(dlg,text);
  PROPDLG_APPLY_TEXTCOLOR(dlg,text);

  annotation_update_data(annotation);
  return new_object_state_change(&annotation->connection.object, old_state, 
				 (GetStateFunc)annotation_get_state,
				 (SetStateFunc)annotation_set_state);
}

static PROPDLG_TYPE 
annotation_get_properties(Annotation *annotation)
{
  AnnotationDialog *dlg = annotation_dialog;

  PROPDLG_CREATE(dlg,annotation);
  PROPDLG_SHOW_TEXTFONT(dlg,text,_("Font:"));
  PROPDLG_SHOW_TEXTFONTHEIGHT(dlg,text,_("Font size:"));
  PROPDLG_SHOW_TEXTCOLOR(dlg,text,_("Font color:"));
  PROPDLG_READY(dlg);
  annotation_dialog = dlg;

  PROPDLG_RETURN(dlg);
}










