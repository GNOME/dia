/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
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

#include "object.h"
#include "poly_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "diamenu.h"

#include "pixmaps/polyline.xpm"

#define DEFAULT_WIDTH 0.15

typedef struct _PolylinePropertiesDialog PolylinePropertiesDialog;
typedef struct _PolylineDefaultsDialog PolylineDefaultsDialog;


typedef struct _Polyline {
  PolyConn poly;

  Color line_color;
  LineStyle line_style;
  real line_width;
  Arrow start_arrow, end_arrow;

  PolylinePropertiesDialog *properties_dialog;

} Polyline;

typedef struct _PolylineProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;
  Arrow start_arrow, end_arrow;
} PolylineProperties;

struct _PolylinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;

  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

struct _PolylineDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

static PolylineDefaultsDialog *polyline_defaults_dialog;
static PolylineProperties default_properties;


static void polyline_move_handle(Polyline *polyline, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void polyline_move(Polyline *polyline, Point *to);
static void polyline_select(Polyline *polyline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void polyline_draw(Polyline *polyline, Renderer *renderer);
static Object *polyline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real polyline_distance_from(Polyline *polyline, Point *point);
static void polyline_update_data(Polyline *polyline);
static void polyline_destroy(Polyline *polyline);
static Object *polyline_copy(Polyline *polyline);
static GtkWidget *polyline_get_properties(Polyline *polyline);
static void polyline_apply_properties(Polyline *polyline);

static void polyline_save(Polyline *polyline, ObjectNode obj_node,
			  const char *filename);
static Object *polyline_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *polyline_get_object_menu(Polyline *polyline, Point *clickedpoint);
static GtkWidget *polyline_get_defaults();
static void polyline_apply_defaults();

static ObjectTypeOps polyline_type_ops =
{
  (CreateFunc)polyline_create,   /* create */
  (LoadFunc)  polyline_load,     /* load */
  (SaveFunc)  polyline_save,      /* save */
  (GetDefaultsFunc)   polyline_get_defaults,
  (ApplyDefaultsFunc) polyline_apply_defaults
};

static ObjectType polyline_type =
{
  "Standard - PolyLine",   /* name */
  0,                         /* version */
  (char **) polyline_xpm,      /* pixmap */
  
  &polyline_type_ops       /* ops */
};

ObjectType *_polyline_type = (ObjectType *) &polyline_type;


static ObjectOps polyline_ops = {
  (DestroyFunc)         polyline_destroy,
  (DrawFunc)            polyline_draw,
  (DistanceFunc)        polyline_distance_from,
  (SelectFunc)          polyline_select,
  (CopyFunc)            polyline_copy,
  (MoveFunc)            polyline_move,
  (MoveHandleFunc)      polyline_move_handle,
  (GetPropertiesFunc)   polyline_get_properties,
  (ApplyPropertiesFunc) polyline_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      polyline_get_object_menu
};

static void
polyline_apply_properties(Polyline *polyline)
{
  PolylinePropertiesDialog *prop_dialog;

  prop_dialog = polyline->properties_dialog;

  polyline->line_width = gtk_spin_button_get_value_as_float(prop_dialog->line_width);
  dia_color_selector_get_color(prop_dialog->color, &polyline->line_color);
  polyline->line_style = dia_line_style_selector_get_linestyle(prop_dialog->line_style);
  polyline->start_arrow = dia_arrow_selector_get_arrow(prop_dialog->start_arrow);
  polyline->end_arrow = dia_arrow_selector_get_arrow(prop_dialog->end_arrow);

  polyline_update_data(polyline);
}

static GtkWidget *
polyline_get_properties(Polyline *polyline)
{
  PolylinePropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *arrow;
  GtkWidget *line_width;
  GtkWidget *align;
  GtkAdjustment *adj;

  if (polyline->properties_dialog == NULL) {
  
    prop_dialog = g_new(PolylinePropertiesDialog, 1);
    polyline->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 5);
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Line width:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    line_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(line_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_width), TRUE);
    prop_dialog->line_width = GTK_SPIN_BUTTON(line_width);
    gtk_box_pack_start(GTK_BOX (hbox), line_width, TRUE, TRUE, 0);
    gtk_widget_show (line_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Line style:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    prop_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Start arrow:");
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    prop_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("End arrow:");
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    prop_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  prop_dialog = polyline->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->line_width, polyline->line_width);
  dia_color_selector_set_color(prop_dialog->color, &polyline->line_color);
  dia_line_style_selector_set_linestyle(prop_dialog->line_style,
					polyline->line_style);
  dia_arrow_selector_set_arrow(prop_dialog->start_arrow,
					 polyline->start_arrow);
  dia_arrow_selector_set_arrow(prop_dialog->end_arrow,
					 polyline->end_arrow);
  
  return prop_dialog->vbox;
}

static void
polyline_init_defaults()
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
polyline_apply_defaults()
{
  polyline_init_defaults();
  default_properties.line_style = dia_line_style_selector_get_linestyle(polyline_defaults_dialog->line_style);
  default_properties.start_arrow = dia_arrow_selector_get_arrow(polyline_defaults_dialog->start_arrow);
  default_properties.end_arrow = dia_arrow_selector_get_arrow(polyline_defaults_dialog->end_arrow);
}

static GtkWidget *
polyline_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *align;
  GtkWidget *linestyle;

  if (polyline_defaults_dialog == NULL) {
  
    polyline_defaults_dialog = g_new(PolylineDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    polyline_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Line style:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    polyline_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Start arrow:");
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    polyline_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("End arrow:");
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    arrow = dia_arrow_selector_new();
    polyline_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(polyline_defaults_dialog->line_style,
					default_properties.line_style);
  dia_arrow_selector_set_arrow(polyline_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(polyline_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return polyline_defaults_dialog->vbox;
}


static real
polyline_distance_from(Polyline *polyline, Point *point)
{
  PolyConn *poly = &polyline->poly;
  return polyconn_distance_from(poly, point, polyline->line_width);
}

static Handle *polyline_closest_handle(Polyline *polyline, Point *point) {
  return polyconn_closest_handle(&polyline->poly, point);
}

static int polyline_closest_segment(Polyline *polyline, Point *point) {
  PolyConn *poly = &polyline->poly;
  return polyconn_closest_segment(poly, point, polyline->line_width);
}

static void
polyline_select(Polyline *polyline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  polyconn_update_data(&polyline->poly);
}

static void
polyline_move_handle(Polyline *polyline, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(polyline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  polyconn_move_handle(&polyline->poly, handle, to, reason);
  polyline_update_data(polyline);
}


static void
polyline_move(Polyline *polyline, Point *to)
{
  polyconn_move(&polyline->poly, to);
  polyline_update_data(polyline);
}

static void
polyline_draw(Polyline *polyline, Renderer *renderer)
{
  PolyConn *poly = &polyline->poly;
  Point *points;
  int n;
  
  points = &poly->points[0];
  n = poly->numpoints;
  
  if (polyline->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, polyline->start_arrow.type,
	       &points[0], &points[1],
	       0.8, 0.8, polyline->line_width,
	       &polyline->line_color, &color_white);
  }
  if (polyline->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, polyline->end_arrow.type,
	       &points[n-1], &points[n-2],
	       polyline->end_arrow.length, polyline->end_arrow.width,
	       polyline->line_width,
	       &polyline->line_color, &color_white);
  }
  renderer->ops->set_linewidth(renderer, polyline->line_width);
  renderer->ops->set_linestyle(renderer, polyline->line_style);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &polyline->line_color);
}

static Object *
polyline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Polyline *polyline;
  PolyConn *poly;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  polyline_init_defaults();
  polyline = g_malloc(sizeof(Polyline));
  poly = &polyline->poly;
  obj = (Object *) polyline;
  
  obj->type = &polyline_type;
  obj->ops = &polyline_ops;

  polyconn_init(poly);

  poly->points[0] = *startpoint;
  poly->points[1] = *startpoint;
  point_add(&poly->points[1], &defaultlen);

  polyline_update_data(polyline);

  polyline->line_width =  attributes_get_default_linewidth();
  polyline->line_color = attributes_get_foreground();
  polyline->line_style = default_properties.line_style;
  polyline->start_arrow = default_properties.start_arrow;
  polyline->end_arrow = default_properties.end_arrow;

  polyline->properties_dialog = NULL;
  
  *handle1 = poly->object.handles[0];
  *handle2 = poly->object.handles[1];
  return (Object *)polyline;
}

static void
polyline_destroy(Polyline *polyline)
{
  if (polyline->properties_dialog != NULL) {
    gtk_widget_destroy(polyline->properties_dialog->vbox);
    g_free(polyline->properties_dialog);
  }
  polyconn_destroy(&polyline->poly);
}

static Object *
polyline_copy(Polyline *polyline)
{
  Polyline *newpolyline;
  PolyConn *poly, *newpoly;
  Object *newobj;
  
  poly = &polyline->poly;
 
  newpolyline = g_malloc(sizeof(Polyline));
  newpoly = &newpolyline->poly;
  newobj = (Object *) newpolyline;

  polyconn_copy(poly, newpoly);

  newpolyline->line_color = polyline->line_color;
  newpolyline->line_width = polyline->line_width;
  newpolyline->line_style = polyline->line_style;
  newpolyline->start_arrow = polyline->start_arrow;
  newpolyline->end_arrow = polyline->end_arrow;

  newpolyline->properties_dialog = NULL;

  return (Object *)newpolyline;
}

static void
polyline_update_data(Polyline *polyline)
{
  PolyConn *poly = &polyline->poly;
  Object *obj = (Object *) polyline;

  polyconn_update_data(&polyline->poly);
    
  polyconn_update_boundingbox(poly);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= polyline->line_width/2;
  obj->bounding_box.left -= polyline->line_width/2;
  obj->bounding_box.bottom += polyline->line_width/2;
  obj->bounding_box.right += polyline->line_width/2;

  /* Fix boundingbox for arrowheads */
  if (polyline->start_arrow.type != ARROW_NONE ||
      polyline->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (polyline->start_arrow.type != ARROW_NONE)
      arrow_width = polyline->start_arrow.width;
    if (polyline->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, polyline->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }

  obj->position = poly->points[0];
}

static void
polyline_save(Polyline *polyline, ObjectNode obj_node,
	      const char *filename)
{
  polyconn_save(&polyline->poly, obj_node);

  data_add_color(new_attribute(obj_node, "line_color"),
		 &polyline->line_color);
  data_add_real(new_attribute(obj_node, "line_width"),
		polyline->line_width);
  data_add_enum(new_attribute(obj_node, "line_style"),
		polyline->line_style);
  if (polyline->start_arrow.type != ARROW_NONE) {
  data_add_enum(new_attribute(obj_node, "start_arrow"),
		  polyline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  polyline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  polyline->start_arrow.width);
  }
  if (polyline->end_arrow.type != ARROW_NONE) {
  data_add_enum(new_attribute(obj_node, "end_arrow"),
		  polyline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  polyline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  polyline->end_arrow.width);
  }
}

static Object *
polyline_load(ObjectNode obj_node, int version, const char *filename)
{
  Polyline *polyline;
  PolyConn *poly;
  Object *obj;
  AttributeNode attr;

  polyline = g_malloc(sizeof(Polyline));

  poly = &polyline->poly;
  obj = (Object *) polyline;
  
  obj->type = &polyline_type;
  obj->ops = &polyline_ops;

  polyline->properties_dialog = NULL;

  polyconn_load(poly, obj_node);

  polyline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &polyline->line_color);

  polyline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    polyline->line_width = data_real(attribute_first_data(attr));

  polyline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    polyline->line_style = data_enum(attribute_first_data(attr));

  polyline->start_arrow.type = ARROW_NONE;
  polyline->start_arrow.length = 0.8;
  polyline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    polyline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    polyline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    polyline->start_arrow.width = data_real(attribute_first_data(attr));

  polyline->end_arrow.type = ARROW_NONE;
  polyline->end_arrow.length = 0.8;
  polyline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    polyline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    polyline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    polyline->end_arrow.width = data_real(attribute_first_data(attr));

  polyline_update_data(polyline);

  return (Object *)polyline;
}

static void
polyline_add_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Polyline *poly = (Polyline*) obj;
  int segment;

  segment = polyline_closest_segment(poly, clicked);
  polyconn_add_point(&poly->poly, segment, clicked);
  polyline_update_data(poly);
}

static void
polyline_delete_corner_callback (Object *obj, Point *clicked, gpointer data)
{
  Handle *handle;
  int handle_nr, i;
  Polyline *poly = (Polyline*) obj;

  handle = polyline_closest_handle(poly, clicked);

  for (i = 0; i < obj->num_handles; i++) {
    if (handle == obj->handles[i]) break;
  }
  handle_nr = i;
  polyconn_remove_point(&poly->poly, handle_nr);
  polyline_update_data(poly);
}


static DiaMenuItem polyline_menu_items[] = {
  { "Add Corner", polyline_add_corner_callback, NULL, 1 },
  { "Delete Corner", polyline_delete_corner_callback, NULL, 1 },
};

static DiaMenu polyline_menu = {
  sizeof(polyline_menu_items)/sizeof(DiaMenuItem),
  polyline_menu_items,
  NULL
};

static DiaMenu *
polyline_get_object_menu(Polyline *polyline, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  polyline_menu_items[0].active = 1;
  polyline_menu_items[1].active = polyline->poly.numpoints > 2;
  return &polyline_menu;
}
