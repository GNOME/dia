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
#include <stdio.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "connection.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "arrows.h"

#include "pixmaps/line.xpm"

#define DEFAULT_WIDTH 0.25

typedef struct _LinePropertiesDialog LinePropertiesDialog;
typedef struct _LineDefaultsDialog LineDefaultsDialog;

typedef struct _LineProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;
  Arrow start_arrow, end_arrow;
} LineProperties;

typedef struct _Line {
  Connection connection;

  ConnectionPoint middle_point;

  Color line_color;
  real line_width;
  LineStyle line_style;
  Arrow start_arrow, end_arrow;
} Line;

struct _LinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;

  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;

  Line *line;
};

struct _LineDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
};

static LinePropertiesDialog *line_properties_dialog;
static LineDefaultsDialog *line_defaults_dialog;
static LineProperties default_properties;

static void line_move_handle(Line *line, Handle *handle,
			     Point *to, HandleMoveReason reason, 
			     ModifierKeys modifiers);
static void line_move(Line *line, Point *to);
static void line_select(Line *line, Point *clicked_point,
			Renderer *interactive_renderer);
static void line_draw(Line *line, Renderer *renderer);
static Object *line_create(Point *startpoint,
			   void *user_data,
			   Handle **handle1,
			   Handle **handle2);
static real line_distance_from(Line *line, Point *point);
static void line_update_data(Line *line);
static void line_destroy(Line *line);
static Object *line_copy(Line *line);
static GtkWidget *line_get_properties(Line *line);
static void line_apply_properties(Line *line);
static GtkWidget *line_get_defaults();
static void line_apply_defaults();

static void line_save(Line *line, ObjectNode obj_node, const char *filename);
static Object *line_load(ObjectNode obj_node, int version, const char *filename);

static ObjectTypeOps line_type_ops =
{
  (CreateFunc) line_create,
  (LoadFunc)   line_load,
  (SaveFunc)   line_save,
  (GetDefaultsFunc)   line_get_defaults,
  (ApplyDefaultsFunc) line_apply_defaults
};

ObjectType line_type =
{
  "Standard - Line",   /* name */
  0,                   /* version */
  (char **) line_xpm,  /* pixmap */
  &line_type_ops       /* ops */
};

ObjectType *_line_type = (ObjectType *) &line_type;

static ObjectOps line_ops = {
  (DestroyFunc)         line_destroy,
  (DrawFunc)            line_draw,
  (DistanceFunc)        line_distance_from,
  (SelectFunc)          line_select,
  (CopyFunc)            line_copy,
  (MoveFunc)            line_move,
  (MoveHandleFunc)      line_move_handle,
  (GetPropertiesFunc)   line_get_properties,
  (ApplyPropertiesFunc) line_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static void
line_apply_properties(Line *line)
{
  if (line != line_properties_dialog->line) {
    printf("Dialog problem:  %p != %p\n", line, line_properties_dialog->line);
    line = line_properties_dialog->line;
  }

  line->line_width = gtk_spin_button_get_value_as_float(line_properties_dialog->line_width);
  dia_color_selector_get_color(line_properties_dialog->color, &line->line_color);
  line->line_style = dia_line_style_selector_get_linestyle(line_properties_dialog->line_style);

  line->start_arrow = dia_arrow_selector_get_arrow(line_properties_dialog->start_arrow);
  line->end_arrow = dia_arrow_selector_get_arrow(line_properties_dialog->end_arrow);
  
  line_update_data(line);
}

static GtkWidget *
line_get_properties(Line *line)
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

  if (line_properties_dialog == NULL) {
  
    line_properties_dialog = g_new(LinePropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    line_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    line_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(line_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_width), TRUE);
    line_properties_dialog->line_width = GTK_SPIN_BUTTON(line_width);
    gtk_box_pack_start(GTK_BOX (hbox), line_width, TRUE, TRUE, 0);
    gtk_widget_show (line_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    line_properties_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    line_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    line_properties_dialog->start_arrow = DIAARROWSELECTOR(arrow);
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
    line_properties_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  line_properties_dialog->line = line;

  gtk_spin_button_set_value(line_properties_dialog->line_width, line->line_width);
  dia_color_selector_set_color(line_properties_dialog->color, &line->line_color);
  dia_line_style_selector_set_linestyle(line_properties_dialog->line_style,
					line->line_style);
  dia_arrow_selector_set_arrow(line_properties_dialog->start_arrow,
			       line->start_arrow);
  dia_arrow_selector_set_arrow(line_properties_dialog->end_arrow,
			       line->end_arrow);
  
  return line_properties_dialog->vbox;
}

static void
line_init_defaults() {
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
line_apply_defaults()
{
  default_properties.line_style = dia_line_style_selector_get_linestyle(line_defaults_dialog->line_style);
  default_properties.start_arrow = dia_arrow_selector_get_arrow(line_defaults_dialog->start_arrow);
  default_properties.end_arrow = dia_arrow_selector_get_arrow(line_defaults_dialog->end_arrow);
}

static GtkWidget *
line_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *arrow;
  GtkWidget *linestyle;
  GtkWidget *align;

  if (line_defaults_dialog == NULL) {
    line_init_defaults();
  
    line_defaults_dialog = g_new(LineDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    line_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    line_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    line_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
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
    line_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(line_defaults_dialog->line_style,
					default_properties.line_style);
  dia_arrow_selector_set_arrow(line_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(line_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return line_defaults_dialog->vbox;
}

static real
line_distance_from(Line *line, Point *point)
{
  Point *endpoints;

  endpoints = &line->connection.endpoints[0]; 
  return distance_line_point( &endpoints[0], &endpoints[1],
			      line->line_width, point);
}

static void
line_select(Line *line, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&line->connection);
}

static void
line_move_handle(Line *line, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(line!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  connection_move_handle(&line->connection, handle->id, to, reason);

  line_update_data(line);
}

static void
line_move(Line *line, Point *to)
{
  Point start_to_end;
  Point *endpoints = &line->connection.endpoints[0]; 

  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  line_update_data(line);
}

static void
line_draw(Line *line, Renderer *renderer)
{
  Point *endpoints;
  
  assert(line != NULL);
  assert(renderer != NULL);

  endpoints = &line->connection.endpoints[0];

  renderer->ops->set_linewidth(renderer, line->line_width);
  renderer->ops->set_linestyle(renderer, line->line_style);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &line->line_color);

  if (line->start_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, line->start_arrow.type,
	       &endpoints[0], &endpoints[1],
	       line->start_arrow.length, line->start_arrow.width, 
	       line->line_width,
	       &line->line_color, &color_white);
  }
  if (line->end_arrow.type != ARROW_NONE) {
    arrow_draw(renderer, line->end_arrow.type,
	       &endpoints[1], &endpoints[0],
	       line->end_arrow.length, line->end_arrow.width, 
	       line->line_width,
	       &line->line_color, &color_white);
  }
}

static Object *
line_create(Point *startpoint,
	    void *user_data,
	    Handle **handle1,
	    Handle **handle2)
{
  Line *line;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  line_init_defaults();

  line = g_malloc(sizeof(Line));

  line->line_width = attributes_get_default_linewidth();
  line->line_color = attributes_get_foreground();
  
  conn = &line->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (Object *) line;
  
  obj->type = &line_type;
  obj->ops = &line_ops;

  connection_init(conn, 2, 1);

  obj->connections[0] = &line->middle_point;
  line->middle_point.object = obj;
  line->middle_point.connected = NULL;
  line->line_style = default_properties.line_style;
  line->start_arrow = default_properties.start_arrow;
  line->end_arrow = default_properties.end_arrow;
  line_update_data(line);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)line;
}

static void
line_destroy(Line *line)
{
  connection_destroy(&line->connection);
}

static Object *
line_copy(Line *line)
{
  Line *newline;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &line->connection;
  
  newline = g_malloc(sizeof(Line));
  newconn = &newline->connection;
  newobj = (Object *) newline;
  
  connection_copy(conn, newconn);

  newobj->connections[0] = &newline->middle_point;
  newline->middle_point.object = newobj;
  newline->middle_point.connected = NULL;
  newline->middle_point.pos = line->middle_point.pos;
  newline->middle_point.last_pos = line->middle_point.last_pos;
  
  newline->line_color = line->line_color;
  newline->line_width = line->line_width;
  newline->line_style = line->line_style;
  newline->start_arrow = line->start_arrow;
  newline->end_arrow = line->end_arrow;

  return (Object *)newline;
}


static void
line_update_data(Line *line)
{
  Connection *conn = &line->connection;
  Object *obj = (Object *) line;
  
  line->middle_point.pos.x =
    conn->endpoints[0].x*0.5 + conn->endpoints[1].x*0.5;
  line->middle_point.pos.y =
    conn->endpoints[0].y*0.5 + conn->endpoints[1].y*0.5;

  connection_update_boundingbox(conn);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= line->line_width/2;
  obj->bounding_box.left -= line->line_width/2;
  obj->bounding_box.bottom += line->line_width/2;
  obj->bounding_box.right += line->line_width/2;

  /* Fix boundingbox for arrowheads */
  if (line->start_arrow.type != ARROW_NONE ||
      line->end_arrow.type != ARROW_NONE) {
    real arrow_width = 0.0;
    if (line->start_arrow.type != ARROW_NONE)
      arrow_width = line->start_arrow.width;
    if (line->end_arrow.type != ARROW_NONE)
      arrow_width = MAX(arrow_width, line->start_arrow.width);

    obj->bounding_box.top -= arrow_width;
    obj->bounding_box.left -= arrow_width;
    obj->bounding_box.bottom += arrow_width;
    obj->bounding_box.right += arrow_width;
  }

  obj->position = conn->endpoints[0];
  
  connection_update_handles(conn);
}


static void
line_save(Line *line, ObjectNode obj_node, const char *filename)
{
  connection_save(&line->connection, obj_node);

  if (!color_equals(&line->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &line->line_color);
  
  if (line->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  line->line_width);
  
  if (line->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  line->line_style);
  
  if (line->start_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "start_arrow"),
		  line->start_arrow.type);
    data_add_real(new_attribute(obj_node, "start_arrow_length"),
		  line->start_arrow.length);
    data_add_real(new_attribute(obj_node, "start_arrow_width"),
		  line->start_arrow.width);
  }
  
  if (line->end_arrow.type != ARROW_NONE) {
    data_add_enum(new_attribute(obj_node, "end_arrow"),
		  line->end_arrow.type);
    data_add_real(new_attribute(obj_node, "end_arrow_length"),
		  line->end_arrow.length);
    data_add_real(new_attribute(obj_node, "end_arrow_width"),
		  line->end_arrow.width);
  }
}

static Object *
line_load(ObjectNode obj_node, int version, const char *filename)
{
  Line *line;
  Connection *conn;
  Object *obj;
  AttributeNode attr;

  line = g_malloc(sizeof(Line));

  conn = &line->connection;
  obj = (Object *) line;

  obj->type = &line_type;
  obj->ops = &line_ops;

  connection_load(conn, obj_node);

  line->line_color = color_black;
  attr = object_find_attribute(obj_node, "line_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &line->line_color);

  line->line_width = 0.1;
  attr = object_find_attribute(obj_node, "line_width");
  if (attr != NULL)
    line->line_width = data_real(attribute_first_data(attr));

  line->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    line->line_style = data_enum(attribute_first_data(attr));

  line->start_arrow.type = ARROW_NONE;
  line->start_arrow.length = 0.8;
  line->start_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "start_arrow");
  if (attr != NULL)
    line->start_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_length");
  if (attr != NULL)
    line->start_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "start_arrow_width");
  if (attr != NULL)
    line->start_arrow.width = data_real(attribute_first_data(attr));

  line->end_arrow.type = ARROW_NONE;
  line->end_arrow.length = 0.8;
  line->end_arrow.width = 0.8;
  attr = object_find_attribute(obj_node, "end_arrow");
  if (attr != NULL)
    line->end_arrow.type = data_enum(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_length");
  if (attr != NULL)
    line->end_arrow.length = data_real(attribute_first_data(attr));
  attr = object_find_attribute(obj_node, "end_arrow_width");
  if (attr != NULL)
    line->end_arrow.width = data_real(attribute_first_data(attr));

  connection_init(conn, 2, 1);

  obj->connections[0] = &line->middle_point;
  line->middle_point.object = obj;
  line->middle_point.connected = NULL;
  line_update_data(line);

  return (Object *)line;
}
