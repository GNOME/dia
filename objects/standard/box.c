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
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/box.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

typedef struct _Box Box;
typedef struct _BoxPropertiesDialog BoxPropertiesDialog;

struct _Box {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  Color inner_color;
  LineStyle line_style;
  
  BoxPropertiesDialog *properties_dialog;
};

struct _BoxPropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  DiaLineStyleSelector *line_style;
};

static real box_distance_from(Box *box, Point *point);
static void box_select(Box *box, Point *clicked_point,
		       Renderer *interactive_renderer);
static void box_move_handle(Box *box, Handle *handle,
			    Point *to, HandleMoveReason reason);
static void box_move(Box *box, Point *to);
static void box_draw(Box *box, Renderer *renderer);
static void box_update_data(Box *box);
static Object *box_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void box_destroy(Box *box);
static Object *box_copy(Box *box);
static GtkWidget *box_get_properties(Box *box);
static void box_apply_properties(Box *box);

static void box_save(Box *box, ObjectNode obj_node );
static Object *box_load(ObjectNode obj_node, int version);

static ObjectTypeOps box_type_ops =
{
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save
};

ObjectType box_type =
{
  "Standard - Box",  /* name */
  0,                 /* version */
  (char **) box_xpm, /* pixmap */

  &box_type_ops      /* ops */
};

ObjectType *_box_type = (ObjectType *) &box_type;

static ObjectOps box_ops = {
  (DestroyFunc)         box_destroy,
  (DrawFunc)            box_draw,
  (DistanceFunc)        box_distance_from,
  (SelectFunc)          box_select,
  (CopyFunc)            box_copy,
  (MoveFunc)            box_move,
  (MoveHandleFunc)      box_move_handle,
  (GetPropertiesFunc)   box_get_properties,
  (ApplyPropertiesFunc) box_apply_properties,
  (IsEmptyFunc)         object_return_false
};

static void
box_apply_properties(Box *box)
{
  BoxPropertiesDialog *prop_dialog;

  prop_dialog = box->properties_dialog;

  box->border_width = gtk_spin_button_get_value_as_float(prop_dialog->border_width);
  dia_color_selector_get_color(prop_dialog->fg_color, &box->border_color);
  dia_color_selector_get_color(prop_dialog->bg_color, &box->inner_color);
  box->line_style = dia_line_style_selector_get_linestyle(prop_dialog->line_style);
  
  box_update_data(box);
}

static GtkWidget *
box_get_properties(Box *box)
{
  BoxPropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkAdjustment *adj;

  if (box->properties_dialog == NULL) {
  
    prop_dialog = g_new(BoxPropertiesDialog, 1);
    box->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 5);
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Border width:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    prop_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Foreground color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Background color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    prop_dialog->bg_color = DIACOLORSELECTOR(color);
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

  prop_dialog = box->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->border_width, box->border_width);
  dia_color_selector_set_color(prop_dialog->fg_color, &box->border_color);
  dia_color_selector_set_color(prop_dialog->bg_color, &box->inner_color);
  dia_line_style_selector_set_linestyle(prop_dialog->line_style,
					box->line_style);
  
  return prop_dialog->vbox;
}

static real
box_distance_from(Box *box, Point *point)
{
  Element *elem = &box->element;
  Rectangle rect;

  rect.left = elem->corner.x - box->border_width/2;
  rect.right = elem->corner.x + elem->width + box->border_width/2;
  rect.top = elem->corner.y - box->border_width/2;
  rect.bottom = elem->corner.y + elem->height + box->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
box_select(Box *box, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  element_update_handles(&box->element);
}

static void
box_move_handle(Box *box, Handle *handle,
		Point *to, HandleMoveReason reason)
{
  assert(box!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&box->element, handle->id, to, reason);

  box_update_data(box);
}

static void
box_move(Box *box, Point *to)
{
  box->element.corner = *to;
  
  box_update_data(box);
}

static void
box_draw(Box *box, Renderer *renderer)
{
  Point lr_corner;
  Element *elem;
  
  assert(box != NULL);
  assert(renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
  renderer->ops->fill_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->inner_color);

  renderer->ops->set_linewidth(renderer, box->border_width);
  renderer->ops->set_linestyle(renderer, box->line_style);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->border_color);
}

static void
box_update_data(Box *box)
{
  Element *elem = &box->element;
  Object *obj = (Object *) box;
  
  /* Update connections: */
  box->connections[0].pos = elem->corner;
  box->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[1].pos.y = elem->corner.y;
  box->connections[2].pos.x = elem->corner.x + elem->width;
  box->connections[2].pos.y = elem->corner.y;
  box->connections[3].pos.x = elem->corner.x;
  box->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[4].pos.x = elem->corner.x + elem->width;
  box->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[5].pos.x = elem->corner.x;
  box->connections[5].pos.y = elem->corner.y + elem->height;
  box->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[6].pos.y = elem->corner.y + elem->height;
  box->connections[7].pos.x = elem->corner.x + elem->width;
  box->connections[7].pos.y = elem->corner.y + elem->height;

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= box->border_width/2;
  obj->bounding_box.left -= box->border_width/2;
  obj->bounding_box.bottom += box->border_width/2;
  obj->bounding_box.right += box->border_width/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
box_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Box *box;
  Element *elem;
  Object *obj;
  int i;

  box = g_malloc(sizeof(Box));
  elem = &box->element;
  obj = (Object *) box;
  
  obj->type = &box_type;

  obj->ops = &box_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  box->properties_dialog = NULL;
  
  box->border_width =  attributes_get_default_linewidth();
  box->border_color = attributes_get_foreground();
  box->inner_color = attributes_get_background();
  box->line_style = LINESTYLE_SOLID;
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box);

  *handle1 = NULL;
  *handle2 = obj->handles[0];  
  return (Object *)box;
}

static void
box_destroy(Box *box)
{
  if (box->properties_dialog != NULL) {
    gtk_widget_destroy(box->properties_dialog->vbox);
    g_free(box->properties_dialog);
  }
  element_destroy(&box->element);
}

static Object *
box_copy(Box *box)
{
  int i;
  Box *newbox;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &box->element;
  
  newbox = g_malloc(sizeof(Box));
  newelem = &newbox->element;
  newobj = (Object *) newbox;

  element_copy(elem, newelem);

  newbox->border_width = box->border_width;
  newbox->border_color = box->border_color;
  newbox->inner_color = box->inner_color;
  newbox->line_style = box->line_style;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newbox->connections[i];
    newbox->connections[i].object = newobj;
    newbox->connections[i].connected = NULL;
    newbox->connections[i].pos = box->connections[i].pos;
    newbox->connections[i].last_pos = box->connections[i].last_pos;
  }

  newbox->properties_dialog = NULL;

  return (Object *)newbox;
}

static void
box_save(Box *box, ObjectNode obj_node)
{
  element_save(&box->element, obj_node);

  data_add_real(new_attribute(obj_node, "border_width"),
		box->border_width);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &box->border_color);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &box->inner_color);
  data_add_enum(new_attribute(obj_node, "line_style"),
		box->line_style);
}

static Object *
box_load(ObjectNode obj_node, int version)
{
  Box *box;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  box = g_malloc(sizeof(Box));
  elem = &box->element;
  obj = (Object *) box;
  
  obj->type = &box_type;
  obj->ops = &box_ops;

  element_load(elem, obj_node);
  
  box->properties_dialog = NULL;

  box->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    box->border_width =  data_real( attribute_first_data(attr) );

  box->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->border_color);
  
  box->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &box->inner_color);
  
  box->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum( attribute_first_data(attr) );

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box);

  return (Object *)box;
}
