/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * Bezier object  Copyright (C) 1999 Lars R. Clausen
 * Modifications by James Henstridge.
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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "poly_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"
#include "message.h"

#include "pixmaps/bezier.xpm"

#define DEFAULT_WIDTH 0.15

typedef struct _BezierlinePropertiesDialog BezierlinePropertiesDialog;
typedef struct _BezierlineDefaultsDialog BezierlineDefaultsDialog;
typedef struct _BezierlineState BezierlineState;

struct _BezierlineState {
  ObjectState obj_state;

  Color line_color;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;
};


typedef struct _Bezierline {
  PolyConn poly;

  BezPoint *points;
  int numpoints;
  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
} Bezierline;

typedef struct _BezierlineProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;
  real dashlength;
  Arrow start_arrow, end_arrow;
} BezierlineProperties;

struct _BezierlinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;

  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;

  Bezierline *bezierline;
};

/*struct _BezierlineDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
  };*/


static BezierlinePropertiesDialog *bezierline_properties_dialog;
/* static BezierlineDefaultsDialog *bezierline_defaults_dialog;
   static BezierlineProperties default_properties; */


static void bezierline_move_handle(Bezierline *bezierline, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void bezierline_move(Bezierline *bezierline, Point *to);
static void bezierline_select(Bezierline *bezierline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void bezierline_draw(Bezierline *bezierline, Renderer *renderer);
static Object *bezierline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real bezierline_distance_from(Bezierline *bezierline, Point *point);
static void bezierline_update_data(Bezierline *bezierline);
static void bezierline_destroy(Bezierline *bezierline);
static Object *bezierline_copy(Bezierline *bezierline);
static GtkWidget *bezierline_get_properties(Bezierline *bezierline);
static ObjectChange *bezierline_apply_properties(Bezierline *bezierline);

static BezierlineState *bezierline_get_state(Bezierline *bezierline);
static void bezierline_set_state(Bezierline *bezierline, BezierlineState *state);

static void bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
			  const char *filename);
static Object *bezierline_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint);
/* static GtkWidget *bezierline_get_defaults();
   static void bezierline_apply_defaults(); */

static ObjectTypeOps bezierline_type_ops =
{
  (CreateFunc)bezierline_create,   /* create */
  (LoadFunc)  bezierline_load,     /* load */
  (SaveFunc)  bezierline_save,      /* save */
  (GetDefaultsFunc)   NULL /*bezierline_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*bezierline_apply_defaults*/
};

static ObjectType bezierline_type =
{
  "Standard - BezierLine",   /* name */
  0,                         /* version */
  (char **) bezier_xpm,      /* pixmap */
  
  &bezierline_type_ops       /* ops */
};

ObjectType *_bezierline_type = (ObjectType *) &bezierline_type;


static ObjectOps bezierline_ops = {
  (DestroyFunc)         bezierline_destroy,
  (DrawFunc)            bezierline_draw,
  (DistanceFunc)        bezierline_distance_from,
  (SelectFunc)          bezierline_select,
  (CopyFunc)            bezierline_copy,
  (MoveFunc)            bezierline_move,
  (MoveHandleFunc)      bezierline_move_handle,
  (GetPropertiesFunc)   bezierline_get_properties,
  (ApplyPropertiesFunc) bezierline_apply_properties,
  (ObjectMenuFunc)      bezierline_get_object_menu
};

static ObjectChange *
bezierline_apply_properties(Bezierline *bezierline)
{
  ObjectState *old_state;
  
  if (bezierline != bezierline_properties_dialog->bezierline) {
    g_message("Bezierline dialog problem:  %p != %p",
		    bezierline, bezierline_properties_dialog->bezierline);
    bezierline = bezierline_properties_dialog->bezierline;
  }

  old_state = (ObjectState *)bezierline_get_state(bezierline);

  bezierline->line_width = 
    gtk_spin_button_get_value_as_float(bezierline_properties_dialog->line_width);
  dia_color_selector_get_color(bezierline_properties_dialog->color,
			       &bezierline->line_color);
  dia_line_style_selector_get_linestyle(bezierline_properties_dialog->line_style, &bezierline->line_style, &bezierline->dashlength);
  bezierline->start_arrow = dia_arrow_selector_get_arrow(bezierline_properties_dialog->start_arrow);
  bezierline->end_arrow = dia_arrow_selector_get_arrow(bezierline_properties_dialog->end_arrow);

  bezierline_update_data(bezierline);
  return new_object_state_change((Object *)bezierline, old_state, 
				 (GetStateFunc)bezierline_get_state,
				 (SetStateFunc)bezierline_set_state);

}

static GtkWidget *
bezierline_get_properties(Bezierline *bezierline)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *arrow;
  GtkWidget *line_width;
  GtkWidget *align;
  GtkAdjustment *adj;

  if (bezierline_properties_dialog == NULL) {
    bezierline_properties_dialog = g_new(BezierlinePropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    bezierline_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    line_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(line_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_width), TRUE);
    bezierline_properties_dialog->line_width = GTK_SPIN_BUTTON(line_width);
    gtk_box_pack_start(GTK_BOX (hbox), line_width, TRUE, TRUE, 0);
    gtk_widget_show (line_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    bezierline_properties_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    bezierline_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Start arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    bezierline_properties_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("End arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    bezierline_properties_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  bezierline_properties_dialog->bezierline = bezierline;
    
  gtk_spin_button_set_value(bezierline_properties_dialog->line_width,
			    bezierline->line_width);
  dia_color_selector_set_color(bezierline_properties_dialog->color,
			       &bezierline->line_color);
  dia_line_style_selector_set_linestyle(bezierline_properties_dialog->line_style,
					bezierline->line_style,
					bezierline->dashlength);
  dia_arrow_selector_set_arrow(bezierline_properties_dialog->start_arrow,
					 bezierline->start_arrow);
  dia_arrow_selector_set_arrow(bezierline_properties_dialog->end_arrow,
					 bezierline->end_arrow);
  
  return bezierline_properties_dialog->vbox;
}

/*
static void
bezierline_init_defaults()
{
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.start_arrow.length = 0.5;
    default_properties.start_arrow.width = 0.5;
    default_properties.end_arrow.length = 0.5;
    default_properties.end_arrow.width = 0.5;
    defaults_initialized = 1;
  }
}

static void
bezierline_apply_defaults()
{
  bezierline_init_defaults();
  dia_line_style_selector_get_linestyle(bezierline_defaults_dialog->line_style, &default_properties.line_style, NULL);
  default_properties.start_arrow = dia_arrow_selector_get_arrow(bezierline_defaults_dialog->start_arrow);
  default_properties.end_arrow = dia_arrow_selector_get_arrow(bezierline_defaults_dialog->end_arrow);
}

static GtkWidget *
bezierline_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *align;
  GtkWidget *linestyle;

  if (bezierline_defaults_dialog == NULL) {
  
    bezierline_defaults_dialog = g_new(BezierlineDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    bezierline_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    bezierline_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Start arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    bezierline_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("End arrow:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    bezierline_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(bezierline_defaults_dialog->line_style,
					default_properties.line_style, 1.0);
  dia_arrow_selector_set_arrow(bezierline_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(bezierline_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return bezierline_defaults_dialog->vbox;
}
*/

static real
bezierline_distance_from(Bezierline *bezierline, Point *point)
{
  PolyConn *poly = &bezierline->poly;
  return polyconn_distance_from(poly, point, bezierline->line_width);
}

static Handle *bezierline_closest_handle(Bezierline *bezierline, Point *point) {
  return polyconn_closest_handle(&bezierline->poly, point);
}

static int bezierline_closest_segment(Bezierline *bezierline, Point *point) {
  PolyConn *poly = &bezierline->poly;
  return polyconn_closest_segment(poly, point, bezierline->line_width);
}

static void
bezierline_select(Bezierline *bezierline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  polyconn_update_data(&bezierline->poly);
}

static void
bezierline_move_handle(Bezierline *bezierline, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point delta, cto;
  gboolean is_major;
  Handle *major = NULL, *minor1 = NULL, *minor2 = NULL, *other = NULL;
  int i;
  Object *obj;

  assert(bezierline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  g_message("Move bezier handle to %f, %f", to->x, to->y);

  /* get the movement delta ... */
  delta = *to;
  point_sub(&delta, &handle->pos);

  /* Find surrounding handles that may need update */
  obj = (Object *)bezierline;
  for (i = 0; i < obj->num_handles; i++) {
    if (obj->handles[i] == handle) {
	g_message("Handle %d", i);
	break;
    }
  }
  switch (i % 3) {
  case 0:
      is_major = TRUE;
      major = obj->handles[i];
      if (i > 0)                  minor1 = obj->handles[i-1];
      if (i < obj->num_handles-1) minor2 = obj->handles[i+1];
      break;
  case 1:
      assert(i >= 1);
      is_major = FALSE;
      major = obj->handles[i-1];
      if (i > 2) other = obj->handles[i-2];
      break;
  case 2:
      assert(i < obj->num_handles - 1);
      is_major = FALSE;
      major = obj->handles[i+1];
      if (i < obj->num_handles-2) other = obj->handles[i+2];
      break;
  }
  assert(major != NULL);


  polyconn_move_handle(&bezierline->poly, handle, to, reason);
  if (is_major) {
      /* move adjacent handles */
      if (minor1) {
	  cto = minor1->pos;
	  point_add(&cto, &delta);
	  polyconn_move_handle(&bezierline->poly, minor1, &cto,
			       HANDLE_MOVE_CONNECTED);
      }
      if (minor2) {
	  cto = minor2->pos;
	  point_add(&cto, &delta);
	  polyconn_move_handle(&bezierline->poly, minor2, &cto,
			       HANDLE_MOVE_CONNECTED);
      }
  } else {
      /* move matching handle */
      if (other) {
	  cto = major->pos;
	  point_sub(&cto, &handle->pos);
	  point_add(&cto, &major->pos);
	  polyconn_move_handle(&bezierline->poly, other, &cto,
			       HANDLE_MOVE_CONNECTED);
      }
  }
  bezierline_update_data(bezierline);
}


static void
bezierline_move(Bezierline *bezierline, Point *to)
{
  polyconn_move(&bezierline->poly, to);
  bezierline_update_data(bezierline);
}

static void
bezierline_draw(Bezierline *bezierline, Renderer *renderer)
{
  PolyConn *poly = &bezierline->poly;
  int bn;
  
  bn = bezierline->numpoints;

  renderer->ops->set_linewidth(renderer, bezierline->line_width);
  renderer->ops->set_linestyle(renderer, bezierline->line_style);
  renderer->ops->set_dashlength(renderer, bezierline->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_bezier(renderer, bezierline->points, 
			     bn, &bezierline->line_color);

  if (bezierline->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->start_arrow.type,
	       &bezierline->points[0].p1, &bezierline->points[1].p1,
	       0.8, 0.8, bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }
  if (bezierline->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, bezierline->end_arrow.type,
	       &bezierline->points[bn-1].p3, &bezierline->points[bn-1].p2,
	       bezierline->end_arrow.length, bezierline->end_arrow.width,
	       bezierline->line_width,
	       &bezierline->line_color, &color_white);
  }
}

static Object *
bezierline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Bezierline *bezierline;
  PolyConn *poly;
  Object *obj;
  Point defaultlen = { .3, .3 };

  /*bezierline_init_defaults();*/
  bezierline = g_malloc(sizeof(Bezierline));
  bezierline->points = NULL;
  bezierline->numpoints = 0;
  poly = &bezierline->poly;
  obj = (Object *) bezierline;
  
  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  polyconn_init(poly);
  polyconn_add_point(poly, 0, NULL);
  polyconn_add_point(poly, 1, NULL);
  
  poly->points[0] = *startpoint;
  poly->points[1] = *startpoint;
  point_add(&poly->points[1], &defaultlen);
  poly->points[2] = poly->points[1];
  point_add(&poly->points[2], &defaultlen);
  poly->points[3] = poly->points[2];
  point_add(&poly->points[3], &defaultlen);

  bezierline_update_data(bezierline);

  bezierline->line_width =  attributes_get_default_linewidth();
  bezierline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&bezierline->line_style,
				    &bezierline->dashlength);
  bezierline->start_arrow = attributes_get_default_start_arrow();
  bezierline->end_arrow = attributes_get_default_end_arrow();

  *handle1 = poly->object.handles[0];
  *handle2 = poly->object.handles[3];
  return (Object *)bezierline;
}

static void
bezierline_destroy(Bezierline *bezierline)
{
  polyconn_destroy(&bezierline->poly);
}

static Object *
bezierline_copy(Bezierline *bezierline)
{
  Bezierline *newbezierline;
  PolyConn *poly, *newpoly;
  Object *newobj;
  
  poly = &bezierline->poly;
 
  newbezierline = g_malloc(sizeof(Bezierline));
  newpoly = &newbezierline->poly;
  newobj = (Object *) newbezierline;

  polyconn_copy(poly, newpoly);

  newbezierline->line_color = bezierline->line_color;
  newbezierline->line_width = bezierline->line_width;
  newbezierline->line_style = bezierline->line_style;
  newbezierline->dashlength = bezierline->dashlength;
  newbezierline->start_arrow = bezierline->start_arrow;
  newbezierline->end_arrow = bezierline->end_arrow;

  return (Object *)newbezierline;
}


static BezierlineState *
bezierline_get_state(Bezierline *bezierline)
{
  BezierlineState *state = g_new(BezierlineState, 1);

  state->obj_state.free = NULL;
  
  state->line_color = bezierline->line_color;
  state->line_width = bezierline->line_width;
  state->line_style = bezierline->line_style;
  state->dashlength = bezierline->dashlength;
  state->start_arrow = bezierline->start_arrow;
  state->end_arrow = bezierline->end_arrow;

  return state;
}

static void
bezierline_set_state(Bezierline *bezierline, BezierlineState *state)
{
  bezierline->line_color = state->line_color;
  bezierline->line_width = state->line_width;
  bezierline->line_style = state->line_style;
  bezierline->dashlength = state->dashlength;
  bezierline->start_arrow = state->start_arrow;
  bezierline->end_arrow = state->end_arrow;

  g_free(state);
  
  bezierline_update_data(bezierline);
}

static void
bezierline_update_data(Bezierline *bezierline)
{
  PolyConn *poly = &bezierline->poly;
  Object *obj = (Object *) bezierline;
  int i;

  polyconn_update_data(&bezierline->poly);
    
  polyconn_update_boundingbox(poly);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= bezierline->line_width/2;
  obj->bounding_box.left -= bezierline->line_width/2;
  obj->bounding_box.bottom += bezierline->line_width/2;
  obj->bounding_box.right += bezierline->line_width/2;

  /* Fix boundingbox for arrowheads */
  if (bezierline->start_arrow.type != ARROW_NONE ||
      bezierline->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (bezierline->start_arrow.type != ARROW_NONE)
      arrow_width = bezierline->start_arrow.width;
    if (bezierline->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, bezierline->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }

  obj->position = poly->points[0];

  /* Update the bezier points */
  if ((poly->numpoints+2) != bezierline->numpoints*3) {
    g_free(bezierline->points);
    bezierline->numpoints = (poly->numpoints+2)/3;
    bezierline->points = g_new(BezPoint, bezierline->numpoints);
  }
  g_message("%d bez components", bezierline->numpoints);
  bezierline->points[0].type = BEZ_MOVE_TO;
  bezierline->points[0].p1 = poly->points[0];
  for (i = 1; i < bezierline->numpoints; i++) {
    bezierline->points[i].type = BEZ_CURVE_TO;
    bezierline->points[i].p1 = poly->points[i*3-2];
    bezierline->points[i].p2 = poly->points[i*3-1];
    bezierline->points[i].p3 = poly->points[i*3];
  }
}

static void
bezierline_save(Bezierline *bezierline, ObjectNode obj_node,
	      const char *filename)
{
  polyconn_save(&bezierline->poly, obj_node);

  if (!color_equals(&bezierline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &bezierline->line_color);
  
  if (bezierline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  bezierline->line_width);
  
  if (bezierline->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  bezierline->line_style);

  if (bezierline->line_style != LINESTYLE_SOLID &&
      bezierline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
		  bezierline->dashlength);
  
  if (bezierline->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  bezierline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  bezierline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  bezierline->start_arrow.width);
  }
  
  if (bezierline->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  bezierline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  bezierline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  bezierline->end_arrow.width);
  }
}

static Object *
bezierline_load(ObjectNode obj_node, int version, const char *filename)
{
  Bezierline *bezierline;
  PolyConn *poly;
  Object *obj;
  AttributeNode attr;

  bezierline = g_malloc(sizeof(Bezierline));

  poly = &bezierline->poly;
  obj = (Object *) bezierline;
  
  obj->type = &bezierline_type;
  obj->ops = &bezierline_ops;

  polyconn_load(poly, obj_node);

  bezierline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &bezierline->line_color);

  bezierline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    bezierline->line_width = data_real(attribute_first_data(attr));

  bezierline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    bezierline->line_style = data_enum(attribute_first_data(attr));

  bezierline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    bezierline->dashlength = data_real(attribute_first_data(attr));

  bezierline->start_arrow.type = ARROW_NONE;
  bezierline->start_arrow.length = 0.8;
  bezierline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    bezierline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    bezierline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    bezierline->start_arrow.width = data_real(attribute_first_data(attr));

  bezierline->end_arrow.type = ARROW_NONE;
  bezierline->end_arrow.length = 0.8;
  bezierline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    bezierline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    bezierline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    bezierline->end_arrow.width = data_real(attribute_first_data(attr));

  bezierline_update_data(bezierline);

  return (Object *)bezierline;
}

static ObjectChange *
bezierline_add_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Bezierline *poly = (Bezierline*) obj;
  int segment;
  ObjectChange *change;

  segment = bezierline_closest_segment(poly, clicked);
  change = polyconn_add_point(&poly->poly, segment, clicked);
  bezierline_update_data(poly);
  return change;
}

static ObjectChange *
bezierline_delete_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Handle *handle;
  int handle_nr, i;
  Bezierline *poly = (Bezierline*) obj;
  ObjectChange *change;
  
  handle = bezierline_closest_handle(poly, clicked);

  for (i = 0; i < obj->num_handles; i++) {
    if (handle == obj->handles[i]) break;
  }
  handle_nr = i;
  change = polyconn_remove_point(&poly->poly, handle_nr);
  bezierline_update_data(poly);
  return change;
}


static DiaMenuItem bezierline_menu_items[] = {
  { N_("Add Corner"), bezierline_add_corner_callback, NULL, 1 },
  { N_("Delete Corner"), bezierline_delete_corner_callback, NULL, 1 },
};

static DiaMenu bezierline_menu = {
  "Bezierline",
  sizeof(bezierline_menu_items)/sizeof(DiaMenuItem),
  bezierline_menu_items,
  NULL
};

static DiaMenu *
bezierline_get_object_menu(Bezierline *bezierline, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  bezierline_menu_items[0].active = 1;
  bezierline_menu_items[1].active = bezierline->poly.numpoints > 2;
  return &bezierline_menu;
}
