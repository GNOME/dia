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
#include "connection.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/line.xpm"

#define DEFAULT_WIDTH 0.25

typedef struct _LinePropertiesDialog LinePropertiesDialog;

typedef struct _Line {
  Connection connection;

  ConnectionPoint middle_point;

  Color line_color;
  real line_width;
  LineStyle line_style;

  LinePropertiesDialog *properties_dialog;
} Line;

struct _LinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;
};

static void line_move_handle(Line *line, Handle *handle,
			     Point *to, HandleMoveReason reason);
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

static void line_save(Line *line, ObjectNode obj_node);
static Object *line_load(ObjectNode obj_node, int version);

static ObjectTypeOps line_type_ops =
{
  (CreateFunc) line_create,
  (LoadFunc)   line_load,
  (SaveFunc)   line_save
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
  (IsEmptyFunc)         object_return_false
};

static void
line_apply_properties(Line *line)
{
  LinePropertiesDialog *prop_dialog;

  prop_dialog = line->properties_dialog;

  line->line_width = gtk_spin_button_get_value_as_float(prop_dialog->line_width);
  dia_color_selector_get_color(prop_dialog->color, &line->line_color);
  line->line_style = dia_line_style_selector_get_linestyle(prop_dialog->line_style);
  
  line_update_data(line);
}

static GtkWidget *
line_get_properties(Line *line)
{
  LinePropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *line_width;
  GtkAdjustment *adj;

  if (line->properties_dialog == NULL) {
  
    prop_dialog = g_new(LinePropertiesDialog, 1);
    line->properties_dialog = prop_dialog;

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

    gtk_widget_show (vbox);
  }

  prop_dialog = line->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->line_width, line->line_width);
  dia_color_selector_set_color(prop_dialog->color, &line->line_color);
  dia_line_style_selector_set_linestyle(prop_dialog->line_style,
					line->line_style);
  
  return prop_dialog->vbox;
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
		 Point *to, HandleMoveReason reason)
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

  line->properties_dialog = NULL;
  
  connection_init(conn, 2, 1);

  obj->connections[0] = &line->middle_point;
  line->middle_point.object = obj;
  line->middle_point.connected = NULL;
  line->line_style = LINESTYLE_SOLID;
  line_update_data(line);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)line;
}

static void
line_destroy(Line *line)
{
  if (line->properties_dialog != NULL) {
    gtk_widget_destroy(line->properties_dialog->vbox);
    g_free(line->properties_dialog);
  }
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

  newline->properties_dialog = NULL;

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

  obj->position = conn->endpoints[0];
  
  connection_update_handles(conn);
}


static void
line_save(Line *line, ObjectNode obj_node)
{
  connection_save(&line->connection, obj_node);

  data_add_color(new_attribute(obj_node, "line_color"),
		 &line->line_color);
  data_add_real(new_attribute(obj_node, "line_width"),
		line->line_width);
  data_add_enum(new_attribute(obj_node, "line_style"),
		line->line_style);
}

static Object *
line_load(ObjectNode obj_node, int version)
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

  line->properties_dialog = NULL;

  connection_init(conn, 2, 1);

  obj->connections[0] = &line->middle_point;
  line->middle_point.object = obj;
  line->middle_point.connected = NULL;
  line_update_data(line);

  return (Object *)line;
}
