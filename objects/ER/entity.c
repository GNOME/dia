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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/entity.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define TEXT_BORDER_WIDTH_X 0.7
#define TEXT_BORDER_WIDTH_Y 0.5
#define WEAK_BORDER_WIDTH 0.25
#define FONT_HEIGHT 0.8

typedef struct _Entity Entity;
typedef struct _EntityPropertiesDialog EntityPropertiesDialog;
typedef struct _EntityState EntityState;

struct _EntityState {
  ObjectState obj_state;

  real border_width;
  Color border_color;
  Color inner_color;
  char *name;
  real name_width;  
  int weak;
};  

struct _Entity {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  Color inner_color;
  
  Font *font;
  char *name;
  real name_width;
  
  int weak;
  
  EntityPropertiesDialog *properties_dialog;
};

struct _EntityPropertiesDialog {
  GtkWidget *vbox;

  GtkToggleButton *weak;
  GtkEntry *name;
  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
};

static real entity_distance_from(Entity *entity, Point *point);
static void entity_select(Entity *entity, Point *clicked_point,
		       Renderer *interactive_renderer);
static void entity_move_handle(Entity *entity, Handle *handle,
			    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void entity_move(Entity *entity, Point *to);
static void entity_draw(Entity *entity, Renderer *renderer);
static void entity_update_data(Entity *entity);
static Object *entity_create(Point *startpoint,
			     void *user_data,
			     Handle **handle1,
			     Handle **handle2);
static void entity_destroy(Entity *entity);
static Object *entity_copy(Entity *entity);
static GtkWidget *entity_get_properties(Entity *entity);
static ObjectChange *entity_apply_properties(Entity *entity);

static void entity_save(Entity *entity, ObjectNode obj_node,
			const char *filename);
static Object *entity_load(ObjectNode obj_node, int version,
			   const char *filename);
static EntityState *entity_get_state(Entity *Entity);
static void entity_set_state(Entity *Entity, EntityState *state);

static ObjectTypeOps entity_type_ops =
{
  (CreateFunc) entity_create,
  (LoadFunc)   entity_load,
  (SaveFunc)   entity_save
};

ObjectType entity_type =
{
  "ER - Entity",  /* name */
  0,                 /* version */
  (char **) entity_xpm, /* pixmap */

  &entity_type_ops      /* ops */
};

ObjectType *_entity_type = (ObjectType *) &entity_type;

static ObjectOps entity_ops = {
  (DestroyFunc)         entity_destroy,
  (DrawFunc)            entity_draw,
  (DistanceFunc)        entity_distance_from,
  (SelectFunc)          entity_select,
  (CopyFunc)            entity_copy,
  (MoveFunc)            entity_move,
  (MoveHandleFunc)      entity_move_handle,
  (GetPropertiesFunc)   entity_get_properties,
  (ApplyPropertiesFunc) entity_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
entity_apply_properties(Entity *entity)
{
  ObjectState *old_state;
  EntityPropertiesDialog *prop_dialog;

  prop_dialog = entity->properties_dialog;

  old_state = (ObjectState *)entity_get_state(entity);

  entity->border_width = gtk_spin_button_get_value_as_float(prop_dialog->border_width);
  dia_color_selector_get_color(prop_dialog->fg_color, &entity->border_color);
  dia_color_selector_get_color(prop_dialog->bg_color, &entity->inner_color);
  g_free(entity->name);
  entity->name = strdup(gtk_entry_get_text(prop_dialog->name));
  entity->weak = prop_dialog->weak->active;

  entity->name_width =
    font_string_width(entity->name, entity->font, FONT_HEIGHT);
  
  entity_update_data(entity);

  return new_object_state_change(&entity->element.object,
                                 old_state,
				 (GetStateFunc)entity_get_state,
				 (SetStateFunc)entity_set_state);
}

static GtkWidget *
entity_get_properties(Entity *entity)
{
  EntityPropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *border_width;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkAdjustment *adj;

  if (entity->properties_dialog == NULL) {
  
    prop_dialog = g_new(EntityPropertiesDialog, 1);
    entity->properties_dialog = prop_dialog;

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));
    prop_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Name:"));
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Weak"));
    prop_dialog->weak = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.1, 0.1);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    prop_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show_all (vbox);
  }

  prop_dialog = entity->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->border_width, entity->border_width);
  dia_color_selector_set_color(prop_dialog->fg_color, &entity->border_color);
  dia_color_selector_set_color(prop_dialog->bg_color, &entity->inner_color);
  gtk_entry_set_text(prop_dialog->name, entity->name);
  gtk_toggle_button_set_active(prop_dialog->weak, entity->weak);
  
  return prop_dialog->vbox;
}

static real
entity_distance_from(Entity *entity, Point *point)
{
  Element *elem = &entity->element;
  Rectangle rect;

  rect.left = elem->corner.x - entity->border_width/2;
  rect.right = elem->corner.x + elem->width + entity->border_width/2;
  rect.top = elem->corner.y - entity->border_width/2;
  rect.bottom = elem->corner.y + elem->height + entity->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
entity_select(Entity *entity, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  element_update_handles(&entity->element);
}

static void
entity_move_handle(Entity *entity, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(entity!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&entity->element, handle->id, to, reason);

  entity_update_data(entity);
}

static void
entity_move(Entity *entity, Point *to)
{
  entity->element.corner = *to;
  
  entity_update_data(entity);
}

static void
entity_draw(Entity *entity, Renderer *renderer)
{
  Point ul_corner, lr_corner;
  Point p;
  Element *elem;
  coord diff;

  assert(entity != NULL);
  assert(renderer != NULL);

  elem = &entity->element;

  ul_corner.x = elem->corner.x;
  ul_corner.y = elem->corner.y;
  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;
  
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
  renderer->ops->fill_rect(renderer, 
			   &ul_corner,
			   &lr_corner, 
			   &entity->inner_color);

  renderer->ops->set_linewidth(renderer, entity->border_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_rect(renderer, 
			   &ul_corner,
			   &lr_corner, 
			   &entity->border_color);

  if(entity->weak) {
    diff = WEAK_BORDER_WIDTH/*MIN(elem->width / 2.0 * 0.20, elem->height / 2.0 * 0.20)*/;
    ul_corner.x += diff; 
    ul_corner.y += diff;
    lr_corner.x -= diff;
    lr_corner.y -= diff;
    renderer->ops->draw_rect(renderer, 
			     &ul_corner, &lr_corner,
			     &entity->border_color);
  }

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - FONT_HEIGHT)/2.0 + font_ascent(entity->font, FONT_HEIGHT);
  renderer->ops->set_font(renderer, 
			  entity->font, FONT_HEIGHT);
  renderer->ops->draw_string(renderer, 
			     entity->name, 
			     &p, ALIGN_CENTER, 
			     &color_black);
}

static void
entity_update_data(Entity *entity)
{
  Element *elem = &entity->element;
  Object *obj = &elem->object;
  ElementBBExtras *extra = &elem->extra_spacing;

  elem->width = entity->name_width + 2*TEXT_BORDER_WIDTH_X;
  elem->height = FONT_HEIGHT + 2*TEXT_BORDER_WIDTH_Y;

  /* Update connections: */
  entity->connections[0].pos = elem->corner;
  entity->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  entity->connections[1].pos.y = elem->corner.y;
  entity->connections[2].pos.x = elem->corner.x + elem->width;
  entity->connections[2].pos.y = elem->corner.y;
  entity->connections[3].pos.x = elem->corner.x;
  entity->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  entity->connections[4].pos.x = elem->corner.x + elem->width;
  entity->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  entity->connections[5].pos.x = elem->corner.x;
  entity->connections[5].pos.y = elem->corner.y + elem->height;
  entity->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  entity->connections[6].pos.y = elem->corner.y + elem->height;
  entity->connections[7].pos.x = elem->corner.x + elem->width;
  entity->connections[7].pos.y = elem->corner.y + elem->height;

  extra->border_trans = entity->border_width/2.0;
  element_update_boundingbox(elem);
  
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static EntityState *
entity_get_state(Entity *entity)
{
  EntityState *state = g_new(EntityState, 1);
  
  state->obj_state.free = NULL;

  state->border_width = entity->border_width;
  state->border_color = entity->border_color;
  state->inner_color = entity->inner_color;
  state->name = g_strdup(entity->name);
  state->name_width = entity->name_width;
  state->weak = entity->weak;

  return state;
}

static void 
entity_set_state(Entity *entity, EntityState *state)
{
  entity->border_width = state->border_width;
  entity->border_color = state->border_color;
  entity->inner_color = state->inner_color;
  g_free(entity->name);
  entity->name = g_strdup(state->name);
  entity->name_width = state->name_width;
  entity->weak = state->weak;

  g_free(state);

  entity_update_data(entity);
}

static Object *
entity_create(Point *startpoint,
	      void *user_data,
	      Handle **handle1,
	      Handle **handle2)
{
  Entity *entity;
  Element *elem;
  Object *obj;
  int i;

  entity = g_malloc(sizeof(Entity));
  elem = &entity->element;
  obj = &elem->object;
  
  obj->type = &entity_type;

  obj->ops = &entity_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  entity->properties_dialog = NULL;
  
  entity->border_width =  attributes_get_default_linewidth();
  entity->border_color = attributes_get_foreground();
  entity->inner_color = attributes_get_background();
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &entity->connections[i];
    entity->connections[i].object = obj;
    entity->connections[i].connected = NULL;
  }

  entity->weak = GPOINTER_TO_INT(user_data);
  entity->font = font_getfont("Courier");
  entity->name = g_strdup(_("Entity"));

  entity->name_width =
    font_string_width(entity->name, entity->font, FONT_HEIGHT);
  
  entity_update_data(entity);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];  
  return &entity->element.object;
}

static void
entity_destroy(Entity *entity)
{
  if (entity->properties_dialog != NULL) {
    gtk_widget_destroy(entity->properties_dialog->vbox);
    g_free(entity->properties_dialog);
  }
  element_destroy(&entity->element);
  g_free(entity->name);
}

static Object *
entity_copy(Entity *entity)
{
  int i;
  Entity *newentity;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &entity->element;
  
  newentity = g_malloc(sizeof(Entity));
  newelem = &newentity->element;
  newobj = &newelem->object;

  element_copy(elem, newelem);

  newentity->border_width = entity->border_width;
  newentity->border_color = entity->border_color;
  newentity->inner_color = entity->inner_color;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newentity->connections[i];
    newentity->connections[i].object = newobj;
    newentity->connections[i].connected = NULL;
    newentity->connections[i].pos = entity->connections[i].pos;
    newentity->connections[i].last_pos = entity->connections[i].last_pos;
  }

  newentity->font = entity->font;
  newentity->name = strdup(entity->name);
  newentity->name_width = entity->name_width;

  newentity->weak = entity->weak;

  newentity->properties_dialog = NULL;

  return &newentity->element.object;
}

static void
entity_save(Entity *entity, ObjectNode obj_node, const char *filename)
{
  element_save(&entity->element, obj_node);

  data_add_real(new_attribute(obj_node, "border_width"),
		entity->border_width);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &entity->border_color);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &entity->inner_color);
  data_add_string(new_attribute(obj_node, "name"),
		  entity->name);
  data_add_boolean(new_attribute(obj_node, "weak"),
		   entity->weak);
}

static Object *
entity_load(ObjectNode obj_node, int version, const char *filename)
{
  Entity *entity;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  entity = g_malloc(sizeof(Entity));
  elem = &entity->element;
  obj = &elem->object;
  
  obj->type = &entity_type;
  obj->ops = &entity_ops;

  element_load(elem, obj_node);
  
  entity->properties_dialog = NULL;

  entity->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    entity->border_width =  data_real( attribute_first_data(attr) );

  entity->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &entity->border_color);
  
  entity->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &entity->inner_color);
  
  entity->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    entity->name = data_string(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "weak");
  if (attr != NULL)
    entity->weak = data_boolean(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &entity->connections[i];
    entity->connections[i].object = obj;
    entity->connections[i].connected = NULL;
  }

  entity->font = font_getfont("Courier");

  entity->name_width =
    font_string_width(entity->name, entity->font, FONT_HEIGHT);

  entity_update_data(entity);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return &entity->element.object;
}
