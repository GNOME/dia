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
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "object.h"
#include "orth_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/zigzag.xpm"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _ZigzaglinePropertiesDialog ZigzaglinePropertiesDialog;
typedef struct _ZigzaglineDefaultsDialog ZigzaglineDefaultsDialog;

typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  LineStyle line_style;
  real line_width;
  Arrow start_arrow, end_arrow;

  ZigzaglinePropertiesDialog *properties_dialog;

} Zigzagline;

struct _ZigzaglinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;

  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

typedef struct _ZigzaglineProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;
  Arrow start_arrow, end_arrow;
} ZigzaglineProperties;

struct _ZigzaglineDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

static ZigzaglineDefaultsDialog *zigzagline_defaults_dialog;
static ZigzaglineProperties default_properties;


static void zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void zigzagline_move(Zigzagline *zigzagline, Point *to);
static void zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void zigzagline_draw(Zigzagline *zigzagline, Renderer *renderer);
static Object *zigzagline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real zigzagline_distance_from(Zigzagline *zigzagline, Point *point);
static void zigzagline_update_data(Zigzagline *zigzagline);
static void zigzagline_destroy(Zigzagline *zigzagline);
static Object *zigzagline_copy(Zigzagline *zigzagline);
static GtkWidget *zigzagline_get_properties(Zigzagline *zigzagline);
static void zigzagline_apply_properties(Zigzagline *zigzagline);

static void zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
			    const char *filename);
static Object *zigzagline_load(ObjectNode obj_node, int version,
			       const char *filename);
static GtkWidget *zigzagline_get_defaults();
static void zigzagline_apply_defaults();

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save,      /* save */
  (GetDefaultsFunc)   zigzagline_get_defaults,
  (ApplyDefaultsFunc) zigzagline_apply_defaults
};

static ObjectType zigzagline_type =
{
  "Standard - ZigZagLine",   /* name */
  0,                         /* version */
  (char **) zigzag_xpm,      /* pixmap */
  
  &zigzagline_type_ops       /* ops */
};

ObjectType *_zigzagline_type = (ObjectType *) &zigzagline_type;


static ObjectOps zigzagline_ops = {
  (DestroyFunc)         zigzagline_destroy,
  (DrawFunc)            zigzagline_draw,
  (DistanceFunc)        zigzagline_distance_from,
  (SelectFunc)          zigzagline_select,
  (CopyFunc)            zigzagline_copy,
  (MoveFunc)            zigzagline_move,
  (MoveHandleFunc)      zigzagline_move_handle,
  (GetPropertiesFunc)   zigzagline_get_properties,
  (ApplyPropertiesFunc) zigzagline_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static void
zigzagline_apply_properties(Zigzagline *zigzagline)
{
  ZigzaglinePropertiesDialog *prop_dialog;

  prop_dialog = zigzagline->properties_dialog;

  zigzagline->line_width = gtk_spin_button_get_value_as_float(prop_dialog->line_width);
  dia_color_selector_get_color(prop_dialog->color, &zigzagline->line_color);
  zigzagline->line_style = dia_line_style_selector_get_linestyle(prop_dialog->line_style);

  zigzagline->start_arrow = dia_arrow_selector_get_arrow(prop_dialog->start_arrow);
  zigzagline->end_arrow = dia_arrow_selector_get_arrow(prop_dialog->end_arrow);
  
  zigzagline_update_data(zigzagline);
}

static GtkWidget *
zigzagline_get_properties(Zigzagline *zigzagline)
{
  ZigzaglinePropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *arrow;
  GtkWidget *line_width;
  GtkWidget *align;
  GtkAdjustment *adj;

  if (zigzagline->properties_dialog == NULL) {
  
    prop_dialog = g_new(ZigzaglinePropertiesDialog, 1);
    zigzagline->properties_dialog = prop_dialog;

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
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
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
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    arrow = dia_arrow_selector_new();
    prop_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  prop_dialog = zigzagline->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->line_width, zigzagline->line_width);
  dia_color_selector_set_color(prop_dialog->color, &zigzagline->line_color);
  dia_line_style_selector_set_linestyle(prop_dialog->line_style,
					zigzagline->line_style);
  dia_arrow_selector_set_arrow(prop_dialog->start_arrow,
					 zigzagline->start_arrow);
  dia_arrow_selector_set_arrow(prop_dialog->end_arrow,
					 zigzagline->end_arrow);
  
  return prop_dialog->vbox;
}

static void
zigzagline_init_defaults() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.start_arrow.length = 0.8;
    default_properties.start_arrow.width = 0.8;
    default_properties.end_arrow.length = 0.8;
    default_properties.end_arrow.width = 0.8;
    defaults_initialized = 1;
  }
}

static void
zigzagline_apply_defaults()
{
  default_properties.line_style = dia_line_style_selector_get_linestyle(zigzagline_defaults_dialog->line_style);
  default_properties.start_arrow = dia_arrow_selector_get_arrow(zigzagline_defaults_dialog->start_arrow);
  default_properties.end_arrow = dia_arrow_selector_get_arrow(zigzagline_defaults_dialog->end_arrow);
}

static GtkWidget *
zigzagline_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *align;
  GtkWidget *linestyle;

  if (zigzagline_defaults_dialog == NULL) {
    zigzagline_init_defaults();
  
    zigzagline_defaults_dialog = g_new(ZigzaglineDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    zigzagline_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Line style:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    zigzagline_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    zigzagline_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
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
    zigzagline_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(zigzagline_defaults_dialog->line_style,
					default_properties.line_style);
  dia_arrow_selector_set_arrow(zigzagline_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(zigzagline_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return zigzagline_defaults_dialog->vbox;
}


static real
zigzagline_distance_from(Zigzagline *zigzagline, Point *point)
{
  OrthConn *orth = &zigzagline->orth;
  return orthconn_distance_from(orth, point, zigzagline->line_width);
}

static void
zigzagline_select(Zigzagline *zigzagline, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&zigzagline->orth);
}

static void
zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
		       Point *to, HandleMoveReason reason)
{
  assert(zigzagline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  orthconn_move_handle(&zigzagline->orth, handle, to, reason);
  zigzagline_update_data(zigzagline);
}


static void
zigzagline_move(Zigzagline *zigzagline, Point *to)
{
  orthconn_move(&zigzagline->orth, to);
  zigzagline_update_data(zigzagline);
}

static void
zigzagline_draw(Zigzagline *zigzagline, Renderer *renderer)
{
  OrthConn *orth = &zigzagline->orth;
  Point *points;
  int n;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  if (zigzagline->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, zigzagline->start_arrow.type,
	       &points[0], &points[1],
	       zigzagline->start_arrow.length, zigzagline->start_arrow.width, 
	       zigzagline->line_width,
	       &zigzagline->line_color, &color_white);
  }
  if (zigzagline->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, zigzagline->end_arrow.type,
	       &points[n-1], &points[n-2],
	       zigzagline->end_arrow.length, zigzagline->end_arrow.width, 
	       zigzagline->line_width,
	       &zigzagline->line_color, &color_white);
  }
  renderer->ops->set_linewidth(renderer, zigzagline->line_width);
  renderer->ops->set_linestyle(renderer, zigzagline->line_style);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &zigzagline->line_color);
}

static Object *
zigzagline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  Object *obj;

  zigzagline_init_defaults();
  zigzagline = g_malloc(sizeof(Zigzagline));
  orth = &zigzagline->orth;
  obj = (Object *) zigzagline;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;
  
  orthconn_init(orth, startpoint);

  zigzagline_update_data(zigzagline);

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  zigzagline->line_style = default_properties.line_style;
  zigzagline->start_arrow = default_properties.start_arrow;
  zigzagline->end_arrow = default_properties.end_arrow;
  
  zigzagline->properties_dialog = NULL;
  
  *handle1 = &orth->endpoint_handles[0];
  *handle2 = &orth->endpoint_handles[1];
  return (Object *)zigzagline;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
  if (zigzagline->properties_dialog != NULL) {
    gtk_widget_destroy(zigzagline->properties_dialog->vbox);
    g_free(zigzagline->properties_dialog);
  }
  orthconn_destroy(&zigzagline->orth);
}

static Object *
zigzagline_copy(Zigzagline *zigzagline)
{
  Zigzagline *newzigzagline;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &zigzagline->orth;
 
  newzigzagline = g_malloc(sizeof(Zigzagline));
  neworth = &newzigzagline->orth;
  newobj = (Object *) newzigzagline;

  orthconn_copy(orth, neworth);

  newzigzagline->line_color = zigzagline->line_color;
  newzigzagline->line_width = zigzagline->line_width;
  newzigzagline->line_style = zigzagline->line_style;
  newzigzagline->start_arrow = zigzagline->start_arrow;
  newzigzagline->end_arrow = zigzagline->end_arrow;

  newzigzagline->properties_dialog = NULL;

  return (Object *)newzigzagline;
}

static void
zigzagline_update_data(Zigzagline *zigzagline)
{
  OrthConn *orth = &zigzagline->orth;
  Object *obj = (Object *) zigzagline;

  orthconn_update_data(&zigzagline->orth);
    
  orthconn_update_boundingbox(orth);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= zigzagline->line_width/2;
  obj->bounding_box.left -= zigzagline->line_width/2;
  obj->bounding_box.bottom += zigzagline->line_width/2;
  obj->bounding_box.right += zigzagline->line_width/2;

  /* Fix boundingbox for arrowheads */
  if (zigzagline->start_arrow.type != ARROW_NONE ||
      zigzagline->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (zigzagline->start_arrow.type != ARROW_NONE)
      arrow_width = zigzagline->start_arrow.width;
    if (zigzagline->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, zigzagline->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }

  obj->position = orth->points[0];
}

static void
zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
		const char *filename)
{
  orthconn_save(&zigzagline->orth, obj_node);

  data_add_color(new_attribute(obj_node, "line_color"),
		 &zigzagline->line_color);
  data_add_real(new_attribute(obj_node, "line_width"),
		zigzagline->line_width);
  data_add_enum(new_attribute(obj_node, "line_style"),
		zigzagline->line_style);
  if (zigzagline->start_arrow.type != ARROW_NONE) {
  data_add_enum(new_attribute(obj_node, "start_arrow"),
		  zigzagline->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  zigzagline->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  zigzagline->start_arrow.width);
  }
  if (zigzagline->end_arrow.type != ARROW_NONE) {
  data_add_enum(new_attribute(obj_node, "end_arrow"),
		  zigzagline->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  zigzagline->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  zigzagline->end_arrow.width);
  }
}

static Object *
zigzagline_load(ObjectNode obj_node, int version, const char *filename)
{
  Zigzagline *zigzagline;
  OrthConn *orth;
  Object *obj;
  AttributeNode attr;

  zigzagline = g_malloc(sizeof(Zigzagline));

  orth = &zigzagline->orth;
  obj = (Object *) zigzagline;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;

  zigzagline->properties_dialog = NULL;

  orthconn_load(orth, obj_node);

  zigzagline->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &zigzagline->line_color);

  zigzagline->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    zigzagline->line_width = data_real(attribute_first_data(attr));

  zigzagline->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    zigzagline->line_style = data_enum(attribute_first_data(attr));

  zigzagline->start_arrow.type = ARROW_NONE;
  zigzagline->start_arrow.length = 0.8;
  zigzagline->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    zigzagline->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    zigzagline->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    zigzagline->start_arrow.width = data_real(attribute_first_data(attr));

  zigzagline->end_arrow.type = ARROW_NONE;
  zigzagline->end_arrow.length = 0.8;
  zigzagline->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    zigzagline->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    zigzagline->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    zigzagline->end_arrow.width = data_real(attribute_first_data(attr));

  zigzagline_update_data(zigzagline);

  return (Object *)zigzagline;
}
