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
#include <string.h>

#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "sheet.h"

#include "pixmaps/attribute.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define FONT_HEIGHT 0.8
#define MULTIVALUE_BORDER_WIDTH_X 0.4
#define MULTIVALUE_BORDER_WIDTH_Y 0.2
#define TEXT_BORDER_WIDTH_X 1.0
#define TEXT_BORDER_WIDTH_Y 0.5

typedef struct _Attribute Attribute;
typedef struct _AttributePropertiesDialog AttributePropertiesDialog;

struct _Attribute {
  Element element;

  Font *font;
  gchar *name;
  real name_width;

  ConnectionPoint connections[8];

  gboolean key;
  gboolean weakkey;
  gboolean derived;
  gboolean multivalue;

  real border_width;
  Color border_color;
  Color inner_color;
 
  AttributePropertiesDialog *properties_dialog;
};

struct _AttributePropertiesDialog {
  GtkWidget *vbox;

  GtkEntry *name;
  GtkToggleButton *key;
  GtkToggleButton *weakkey;
  GtkToggleButton *derived;
  GtkToggleButton *multivalue;
  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
};

static real attribute_distance_from(Attribute *attribute, Point *point);
static void attribute_select(Attribute *attribute, Point *clicked_point,
			   Renderer *interactive_renderer);
static void attribute_move_handle(Attribute *attribute, Handle *handle,
				Point *to, HandleMoveReason reason);
static void attribute_move(Attribute *attribute, Point *to);
static void attribute_draw(Attribute *attribute, Renderer *renderer);
static void attribute_update_data(Attribute *attribute);
static Object *attribute_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void attribute_destroy(Attribute *attribute);
static Object *attribute_copy(Attribute *attribute);
static GtkWidget *attribute_get_properties(Attribute *attribute);
static void attribute_apply_properties(Attribute *attribute);

static void attribute_save(Attribute *attribute, ObjectNode obj_node,
			   const char *filename);
static Object *attribute_load(ObjectNode obj_node, int version,
			      const char *filename);

static ObjectTypeOps attribute_type_ops =
{
  (CreateFunc) attribute_create,
  (LoadFunc)   attribute_load,
  (SaveFunc)   attribute_save
};

ObjectType attribute_type =
{
  "ER - Attribute",   /* name */
  0,                      /* version */
  (char **) attribute_xpm,  /* pixmap */
  
  &attribute_type_ops       /* ops */
};

SheetObject attribute_sheetobj =
{
  "ER - Attribute",  /* type */
  "Attribute",      /* description */
  (char **) attribute_xpm, /* pixmap */

  NULL                /* user_data */
};

ObjectType *_attribute_type = (ObjectType *) &attribute_type;

static ObjectOps attribute_ops = {
  (DestroyFunc)         attribute_destroy,
  (DrawFunc)            attribute_draw,
  (DistanceFunc)        attribute_distance_from,
  (SelectFunc)          attribute_select,
  (CopyFunc)            attribute_copy,
  (MoveFunc)            attribute_move,
  (MoveHandleFunc)      attribute_move_handle,
  (GetPropertiesFunc)   attribute_get_properties,
  (ApplyPropertiesFunc) attribute_apply_properties,
  (IsEmptyFunc)         object_return_false
};

static void
attribute_apply_properties(Attribute *attribute)
{
  AttributePropertiesDialog *prop_dialog;

  prop_dialog = attribute->properties_dialog;

  attribute->border_width = gtk_spin_button_get_value_as_float(prop_dialog->border_width);
  dia_color_selector_get_color(prop_dialog->fg_color, &attribute->border_color);
  dia_color_selector_get_color(prop_dialog->bg_color, &attribute->inner_color);
  g_free(attribute->name);
  attribute->name = g_strdup(gtk_entry_get_text(prop_dialog->name));
  attribute->key = prop_dialog->key->active;
  attribute->weakkey = prop_dialog->weakkey->active;
  attribute->derived = prop_dialog->derived->active;
  attribute->multivalue = prop_dialog->multivalue->active;

  attribute->name_width =
    font_string_width(attribute->name, attribute->font, FONT_HEIGHT);

  attribute_update_data(attribute);
}

static GtkWidget *
attribute_get_properties(Attribute *attribute)
{
  AttributePropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *border_width;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkAdjustment *adj;

  if (attribute->properties_dialog == NULL) {
  
    prop_dialog = g_new(AttributePropertiesDialog, 1);
    attribute->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 5);
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Name:");
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Key");
    prop_dialog->key = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Weak key");
    prop_dialog->weakkey = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Derived");
    prop_dialog->derived = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label("Multivalue");
    prop_dialog->multivalue = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Border width:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    prop_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Foreground color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new("Background color:");
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show_all (vbox);
  }

  prop_dialog = attribute->properties_dialog;
    
  gtk_entry_set_text(prop_dialog->name, attribute->name);
  gtk_toggle_button_set_active(prop_dialog->key, attribute->key);
  gtk_toggle_button_set_active(prop_dialog->weakkey, attribute->weakkey);
  gtk_toggle_button_set_active(prop_dialog->derived, attribute->derived);
  gtk_toggle_button_set_active(prop_dialog->multivalue, attribute->multivalue);
  gtk_spin_button_set_value(prop_dialog->border_width, attribute->border_width);
  dia_color_selector_set_color(prop_dialog->fg_color, &attribute->border_color);
  dia_color_selector_set_color(prop_dialog->bg_color, &attribute->inner_color);
  
  return prop_dialog->vbox;
}


static real
attribute_distance_from(Attribute *attribute, Point *point)
{
  Object *obj = &attribute->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
attribute_select(Attribute *attribute, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  element_update_handles(&attribute->element);
}

static void
attribute_move_handle(Attribute *attribute, Handle *handle,
		    Point *to, HandleMoveReason reason)
{
  assert(attribute!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  element_move_handle(&attribute->element, handle->id, to, reason);
  attribute_update_data(attribute);
}

static void
attribute_move(Attribute *attribute, Point *to)
{
  attribute->element.corner = *to;
  attribute_update_data(attribute);
}

static void
attribute_draw(Attribute *attribute, Renderer *renderer)
{
  Point center;
  Point start, end;
  Point p;
  Element *elem;
  real width;
  
  assert(attribute != NULL);
  assert(renderer != NULL);

  elem = &attribute->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  renderer->ops->fill_ellipse(renderer, &center,
			      elem->width, elem->height,
			      &attribute->inner_color);

  renderer->ops->set_linewidth(renderer, attribute->border_width);
  if (attribute->derived) {
    renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
    renderer->ops->set_dashlength(renderer, 0.3);
  } else {
    renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  }

  renderer->ops->draw_ellipse(renderer, &center,
			      elem->width, elem->height,
			      &attribute->border_color);

  if(attribute->multivalue) {
    renderer->ops->draw_ellipse(renderer, &center,
				elem->width - 2*MULTIVALUE_BORDER_WIDTH_X,
				elem->height - 2*MULTIVALUE_BORDER_WIDTH_Y,
				&attribute->border_color);
  }

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - FONT_HEIGHT)/2.0 +
         font_ascent(attribute->font, FONT_HEIGHT);

  renderer->ops->set_font(renderer,  attribute->font, FONT_HEIGHT);
  renderer->ops->draw_string(renderer, attribute->name, 
			     &p, ALIGN_CENTER, 
			     &color_black);

  if (attribute->key || attribute->weakkey) {
    if (attribute->weakkey) {
      renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
      renderer->ops->set_dashlength(renderer, 0.3);
    } else {
      renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
    }
    width = font_string_width(attribute->name, attribute->font, FONT_HEIGHT);
    start.x = center.x - width / 2;
    start.y = center.y + 0.4;
    end.x = center.x + width / 2;
    end.y = center.y + 0.4;
    renderer->ops->draw_line(renderer, &start, &end, &color_black);
  }
}

static void
attribute_update_data(Attribute *attribute)
{
  Element *elem = &attribute->element;
  Object *obj = (Object *) attribute;
  Point center;
  real half_x, half_y;

  elem->width = attribute->name_width + 2*TEXT_BORDER_WIDTH_X;
  elem->height = FONT_HEIGHT + 2*TEXT_BORDER_WIDTH_Y;

  center.x = elem->corner.x + elem->width / 2.0;
  center.y = elem->corner.y + elem->height / 2.0;
  
  half_x = elem->width * M_SQRT1_2 / 2.0;
  half_y = elem->height * M_SQRT1_2 / 2.0;
    
  /* Update connections: */
  attribute->connections[0].pos.x = center.x - half_x;
  attribute->connections[0].pos.y = center.y - half_y;
  attribute->connections[1].pos.x = center.x;
  attribute->connections[1].pos.y = elem->corner.y;
  attribute->connections[2].pos.x = center.x + half_x;
  attribute->connections[2].pos.y = center.y - half_y;
  attribute->connections[3].pos.x = elem->corner.x;
  attribute->connections[3].pos.y = center.y;
  attribute->connections[4].pos.x = elem->corner.x + elem->width;
  attribute->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  attribute->connections[5].pos.x = center.x - half_x;
  attribute->connections[5].pos.y = center.y + half_y;
  attribute->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  attribute->connections[6].pos.y = elem->corner.y + elem->height;
  attribute->connections[7].pos.x = center.x + half_x;
  attribute->connections[7].pos.y = center.y + half_y;

  element_update_boundingbox(elem);
  /* fix boundingattribute for line_width: */
  obj->bounding_box.top -= attribute->border_width/2;
  obj->bounding_box.left -= attribute->border_width/2;
  obj->bounding_box.bottom += attribute->border_width/2;
  obj->bounding_box.right += attribute->border_width/2;

  obj->position = elem->corner;

  element_update_handles(elem);
  
}

static Object *
attribute_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Attribute *attribute;
  Element *elem;
  Object *obj;
  int i;

  attribute = g_malloc(sizeof(Attribute));
  elem = &attribute->element;
  obj = (Object *) attribute;
  
  obj->type = &attribute_type;
  obj->ops = &attribute_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  attribute->properties_dialog = NULL;

  attribute->border_width =  attributes_get_default_linewidth();
  attribute->border_color = attributes_get_foreground();
  attribute->inner_color = attributes_get_background();

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &attribute->connections[i];
    attribute->connections[i].object = obj;
    attribute->connections[i].connected = NULL;
  }

  attribute->key = FALSE;
  attribute->weakkey = FALSE;
  attribute->derived = FALSE;
  attribute->multivalue = FALSE;
  attribute->font = font_getfont("Courier");
  attribute->name = g_strdup("Attribute");

  attribute->name_width =
    font_string_width(attribute->name, attribute->font, FONT_HEIGHT);

  attribute_update_data(attribute);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];
  return (Object *)attribute;
}

static void
attribute_destroy(Attribute *attribute)
{
  if (attribute->properties_dialog != NULL) {
    gtk_widget_destroy(attribute->properties_dialog->vbox);
    g_free(attribute->properties_dialog);
  }
  element_destroy(&attribute->element);
  g_free(attribute->name);
}

static Object *
attribute_copy(Attribute *attribute)
{
  int i;
  Attribute *newattribute;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &attribute->element;
  
  newattribute = g_malloc(sizeof(Attribute));
  newelem = &newattribute->element;
  newobj = (Object *) newattribute;

  element_copy(elem, newelem);

  newattribute->border_width = attribute->border_width;
  newattribute->border_color = attribute->border_color;
  newattribute->inner_color = attribute->inner_color;

  for (i=0;i<8;i++) {
    newobj->connections[i] = &newattribute->connections[i];
    newattribute->connections[i].object = newobj;
    newattribute->connections[i].connected = NULL;
    newattribute->connections[i].pos = attribute->connections[i].pos;
    newattribute->connections[i].last_pos = attribute->connections[i].last_pos;
  }

  newattribute->font = attribute->font;
  newattribute->name = strdup(attribute->name);
  newattribute->name_width = attribute->name_width;

  newattribute->key = attribute->key;
  newattribute->weakkey = attribute->weakkey;
  newattribute->derived = attribute->derived;
  newattribute->multivalue = attribute->multivalue;

  newattribute->properties_dialog = NULL;

  return (Object *)newattribute;
}


static void
attribute_save(Attribute *attribute, ObjectNode obj_node,
	       const char *filename)
{
  element_save(&attribute->element, obj_node);

  data_add_real(new_attribute(obj_node, "border_width"),
		attribute->border_width);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &attribute->border_color);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &attribute->inner_color);
  data_add_string(new_attribute(obj_node, "name"),
		  attribute->name);
  data_add_boolean(new_attribute(obj_node, "key"),
		   attribute->key);
  data_add_boolean(new_attribute(obj_node, "weak_key"),
		   attribute->weakkey);
  data_add_boolean(new_attribute(obj_node, "derived"),
		   attribute->derived);
  data_add_boolean(new_attribute(obj_node, "multivalued"),
		   attribute->multivalue);
}

static Object *attribute_load(ObjectNode obj_node, int version,
			      const char *filename)
{
  Attribute *attribute;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  attribute = g_malloc(sizeof(Attribute));
  elem = &attribute->element;
  obj = (Object *) attribute;
  
  obj->type = &attribute_type;
  obj->ops = &attribute_ops;

  element_load(elem, obj_node);

  attribute->properties_dialog = NULL;

  attribute->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    attribute->border_width =  data_real( attribute_first_data(attr) );

  attribute->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &attribute->border_color);
  
  attribute->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &attribute->inner_color);
  
  attribute->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    attribute->name = data_string(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "key");
  if (attr != NULL)
    attribute->key = data_boolean(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "weak_key");
  if (attr != NULL)
    attribute->weakkey = data_boolean(attribute_first_data(attr));
  
  attr = object_find_attribute(obj_node, "derived");
  if (attr != NULL)
    attribute->derived = data_boolean(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "multivalued");
  if (attr != NULL)
    attribute->multivalue = data_boolean(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &attribute->connections[i];
    attribute->connections[i].object = obj;
    attribute->connections[i].connected = NULL;
  }

  attribute->font = font_getfont("Courier");

  attribute->name_width = font_string_width(attribute->name, attribute->font, FONT_HEIGHT);

  attribute_update_data(attribute);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)attribute;
}
