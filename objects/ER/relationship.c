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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"

#include "pixmaps/relationship.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define IDENTIFYING_BORDER_WIDTH 0.4
#define TEXT_BORDER_WIDTH_X 1.0
#define DIAMOND_RATIO 0.6
#define FONT_HEIGHT 0.8
#define CARDINALITY_DISTANCE 0.3

typedef struct _Relationship Relationship;
typedef struct _RelationshipState RelationshipState;
typedef struct _RelationshipPropertiesDialog RelationshipPropertiesDialog;

struct _RelationshipState {
  ObjectState obj_state;

  gchar *name;
  gchar *left_cardinality;
  gchar *right_cardinality;
  real name_width;
  real left_card_width;
  real right_card_width;

  gboolean identifying;
  gboolean rotate;

  real border_width;
  Color border_color;
  Color inner_color;
};

struct _Relationship {
  Element element;

  Font *font;
  gchar *name;
  gchar *left_cardinality;
  gchar *right_cardinality;
  real name_width;
  real left_card_width;
  real right_card_width;

  gboolean identifying;
  gboolean rotate;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  Color inner_color;

  RelationshipPropertiesDialog *properties_dialog;
};

struct _RelationshipPropertiesDialog {
  GtkWidget *vbox;

  GtkEntry *name;
  GtkEntry *left_cardinality;
  GtkEntry *right_cardinality;
  GtkToggleButton *identifying;
  GtkToggleButton *rotate;
  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
};

static real relationship_distance_from(Relationship *relationship, Point *point);
static void relationship_select(Relationship *relationship, Point *clicked_point,
		       Renderer *interactive_renderer);
static void relationship_move_handle(Relationship *relationship, Handle *handle,
			    Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void relationship_move(Relationship *relationship, Point *to);
static void relationship_draw(Relationship *relationship, Renderer *renderer);
static void relationship_update_data(Relationship *relationship);
static Object *relationship_create(Point *startpoint,
			  void *user_data,
			  Handle **handle1,
			  Handle **handle2);
static void relationship_destroy(Relationship *relationship);
static Object *relationship_copy(Relationship *relationship);
static GtkWidget *relationship_get_properties(Relationship *relationship);
static ObjectChange *relationship_apply_properties(Relationship *relationship);

static void relationship_save(Relationship *relationship,
			      ObjectNode obj_node, const char *filename);
static Object *relationship_load(ObjectNode obj_node, int version,
				 const char *filename);
static RelationshipState *relationship_get_state(Relationship *relationship);
static void relationship_set_state(Relationship *relationship, RelationshipState *state);

static ObjectTypeOps relationship_type_ops =
{
  (CreateFunc) relationship_create,
  (LoadFunc)   relationship_load,
  (SaveFunc)   relationship_save
};

ObjectType relationship_type =
{
  "ER - Relationship",  /* name */
  0,                 /* version */
  (char **) relationship_xpm, /* pixmap */

  &relationship_type_ops      /* ops */
};

ObjectType *_relationship_type = (ObjectType *) &relationship_type;

static ObjectOps relationship_ops = {
  (DestroyFunc)         relationship_destroy,
  (DrawFunc)            relationship_draw,
  (DistanceFunc)        relationship_distance_from,
  (SelectFunc)          relationship_select,
  (CopyFunc)            relationship_copy,
  (MoveFunc)            relationship_move,
  (MoveHandleFunc)      relationship_move_handle,
  (GetPropertiesFunc)   relationship_get_properties,
  (ApplyPropertiesFunc) relationship_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
relationship_apply_properties(Relationship *relationship)
{
  ObjectState *old_state;
  RelationshipPropertiesDialog *prop_dialog;

  prop_dialog = relationship->properties_dialog;

  old_state = (ObjectState *)relationship_get_state(relationship);

  relationship->border_width = gtk_spin_button_get_value_as_float(prop_dialog->border_width);
  dia_color_selector_get_color(prop_dialog->fg_color, &relationship->border_color);
  dia_color_selector_get_color(prop_dialog->bg_color, &relationship->inner_color);
  g_free(relationship->name);
  relationship->name = g_strdup(gtk_entry_get_text(prop_dialog->name));
  relationship->identifying = prop_dialog->identifying->active;
  relationship->rotate = prop_dialog->rotate->active;
  g_free(relationship->left_cardinality);
  relationship->left_cardinality = 
    g_strdup(gtk_entry_get_text(prop_dialog->left_cardinality));
  g_free(relationship->right_cardinality);
  relationship->right_cardinality = 
    g_strdup(gtk_entry_get_text(prop_dialog->right_cardinality));

  relationship->name_width =
    font_string_width(relationship->name, relationship->font, FONT_HEIGHT);
  relationship->left_card_width =
    font_string_width(relationship->left_cardinality, relationship->font, FONT_HEIGHT);
  relationship->right_card_width =
    font_string_width(relationship->right_cardinality, relationship->font, FONT_HEIGHT);

  relationship_update_data(relationship);

  return new_object_state_change((Object *)relationship, old_state,
				 (GetStateFunc)relationship_get_state,
				 (SetStateFunc)relationship_set_state);
}

static RelationshipState *
relationship_get_state(Relationship *relationship)
{
  RelationshipState *state = g_new(RelationshipState, 1);
  
  state->obj_state.free = NULL;

  state->name = g_strdup(relationship->name);
  state->left_cardinality = relationship->left_cardinality;
  state->right_cardinality = relationship->right_cardinality;
  state->name_width = relationship->name_width;
  state->left_card_width = relationship->left_card_width;
  state->right_card_width = relationship->right_card_width;
  state->identifying = relationship->identifying;
  state->rotate = relationship->rotate;
  state->border_width = relationship->border_width;
  state->border_color = relationship->border_color;
  state->inner_color = relationship->inner_color;

  return state;
}

static void
relationship_set_state(Relationship *relationship, RelationshipState *state)
{
  g_free(relationship->name);
  relationship->name = g_strdup(state->name);
  relationship->left_cardinality = state->left_cardinality;
  relationship->right_cardinality = state->right_cardinality;
  relationship->name_width = state->name_width;
  relationship->left_card_width = state->left_card_width;
  relationship->right_card_width = state->right_card_width;
  relationship->identifying = state->identifying;
  relationship->rotate = state->rotate;
  relationship->border_width = state->border_width;
  relationship->border_color = state->border_color;
  relationship->inner_color = state->inner_color;

  g_free(state);

  relationship_update_data(relationship);
}

static GtkWidget *
relationship_get_properties(Relationship *relationship)
{
  RelationshipPropertiesDialog *prop_dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *border_width;
  GtkWidget *entry;
  GtkWidget *checkbox;
  GtkAdjustment *adj;

  if (relationship->properties_dialog == NULL) {
  
    prop_dialog = g_new(RelationshipPropertiesDialog, 1);
    relationship->properties_dialog = prop_dialog;

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
    label = gtk_label_new(_("Left Cardinality:"));
    entry = gtk_entry_new();
    prop_dialog->left_cardinality = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Right Cardinality:"));
    entry = gtk_entry_new();
    prop_dialog->right_cardinality = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Rotate"));
    prop_dialog->rotate = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Identifying"));
    prop_dialog->identifying = GTK_TOGGLE_BUTTON(checkbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    prop_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start (GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    color = dia_color_selector_new();
    prop_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show_all (vbox);
  }

  prop_dialog = relationship->properties_dialog;
    
  gtk_spin_button_set_value(prop_dialog->border_width, relationship->border_width);
  dia_color_selector_set_color(prop_dialog->fg_color, &relationship->border_color);
  dia_color_selector_set_color(prop_dialog->bg_color, &relationship->inner_color);
  gtk_entry_set_text(prop_dialog->left_cardinality, relationship->left_cardinality);
  gtk_entry_set_text(prop_dialog->right_cardinality, relationship->right_cardinality);
  gtk_entry_set_text(prop_dialog->name, relationship->name); 
  gtk_toggle_button_set_active(prop_dialog->rotate, relationship->rotate);
  gtk_toggle_button_set_active(prop_dialog->identifying, relationship->identifying);
 
  return prop_dialog->vbox;
}

static real
relationship_distance_from(Relationship *relationship, Point *point)
{
  Element *elem = &relationship->element;
  Rectangle rect;

  rect.left = elem->corner.x - relationship->border_width/2;
  rect.right = elem->corner.x + elem->width + relationship->border_width/2;
  rect.top = elem->corner.y - relationship->border_width/2;
  rect.bottom = elem->corner.y + elem->height + relationship->border_width/2;
  return distance_rectangle_point(&rect, point);
}

static void
relationship_select(Relationship *relationship, Point *clicked_point,
	   Renderer *interactive_renderer)
{
  element_update_handles(&relationship->element);
}

static void
relationship_move_handle(Relationship *relationship, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(relationship!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  element_move_handle(&relationship->element, handle->id, to, reason);

  relationship_update_data(relationship);
}

static void
relationship_move(Relationship *relationship, Point *to)
{
  relationship->element.corner = *to;
  
  relationship_update_data(relationship);
}

static void
relationship_draw(Relationship *relationship, Renderer *renderer)
{
  Point corners[4];
  Point lc, rc;
  Point p;
  Element *elem;
  coord diff;
  Alignment left_align;

  assert(relationship != NULL);
  assert(renderer != NULL);

  elem = &relationship->element;

  corners[0].x = elem->corner.x;
  corners[0].y = elem->corner.y + elem->height / 2;
  corners[1].x = elem->corner.x + elem->width / 2;
  corners[1].y = elem->corner.y;
  corners[2].x = elem->corner.x + elem->width;
  corners[2].y = elem->corner.y + elem->height / 2;
  corners[3].x = elem->corner.x + elem->width / 2;
  corners[3].y = elem->corner.y + elem->height;

  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

  renderer->ops->fill_polygon(renderer, corners, 4, 
			      &relationship->inner_color);

  renderer->ops->set_linewidth(renderer, relationship->border_width);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  renderer->ops->draw_polygon(renderer, corners, 4, 
			      &relationship->border_color);
  
  if (relationship->rotate) {
    lc.x = corners[1].x + 0.2;
    lc.y = corners[1].y - 0.3;
    rc.x = corners[3].x + 0.2;
    rc.y = corners[3].y + 0.3 + FONT_HEIGHT;
    left_align = ALIGN_LEFT;
  } else {
    lc.x = corners[0].x - CARDINALITY_DISTANCE;
    lc.y = corners[0].y - 0.3;
    rc.x = corners[2].x + CARDINALITY_DISTANCE;
    rc.y = corners[2].y - 0.3;
    left_align = ALIGN_RIGHT;
  }

  if (relationship->identifying) {
    diff = IDENTIFYING_BORDER_WIDTH;
    corners[0].x += diff; 
    corners[1].y += diff*DIAMOND_RATIO;
    corners[2].x -= diff;
    corners[3].y -= diff*DIAMOND_RATIO;

    renderer->ops->draw_polygon(renderer, corners, 4,
				&relationship->border_color);
  }

  renderer->ops->set_font(renderer, relationship->font, FONT_HEIGHT);
  renderer->ops->draw_string(renderer,
			     relationship->left_cardinality,
			     &lc, left_align,
			     &color_black);
  renderer->ops->draw_string(renderer,
			     relationship->right_cardinality,
			     &rc, ALIGN_LEFT,
			     &color_black);

  p.x = elem->corner.x + elem->width / 2.0;
  p.y = elem->corner.y + (elem->height - FONT_HEIGHT)/2.0 +
         font_ascent(relationship->font, FONT_HEIGHT);
  
  renderer->ops->draw_string(renderer, 
			     relationship->name, 
			     &p, ALIGN_CENTER, 
			     &color_black);
}

static void
relationship_update_data(Relationship *relationship)
{
  Element *elem = &relationship->element;
  Object *obj = (Object *) relationship;

  elem->width = relationship->name_width + 2*TEXT_BORDER_WIDTH_X;
  elem->height = elem->width * DIAMOND_RATIO;

  /* Update connections: */
  /*   	
       	2
       	* 
    1  / \  3
      *	  *  
     /	   \ 
  0 *	    * 4
     \	   /   	     
      *	  *    	     
    7  \ /  5
        *    
        6    
   */
  
  relationship->connections[0].pos.x = elem->corner.x;
  relationship->connections[0].pos.y = elem->corner.y + elem->height / 2.0;

  relationship->connections[1].pos.x = elem->corner.x + elem->width / 4.0;
  relationship->connections[1].pos.y = elem->corner.y + elem->height / 4.0;

  relationship->connections[2].pos.x = elem->corner.x + elem->width / 2.0;
  relationship->connections[2].pos.y = elem->corner.y;
  
  relationship->connections[3].pos.x = elem->corner.x + 3.0 * elem->width / 4.0;
  relationship->connections[3].pos.y = elem->corner.y + elem->height / 4.0;
  
  relationship->connections[4].pos.x = elem->corner.x + elem->width;
  relationship->connections[4].pos.y = elem->corner.y + elem->height / 2.0;

  relationship->connections[5].pos.x = elem->corner.x + 3.0 * elem->width / 4.0;
  relationship->connections[5].pos.y = elem->corner.y + 3.0 * elem->height / 4.0;
  
  relationship->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  relationship->connections[6].pos.y = elem->corner.y + elem->height;

  relationship->connections[7].pos.x = elem->corner.x + elem->width / 4.0;
  relationship->connections[7].pos.y = elem->corner.y + 3.0 * elem->height / 4.0;

  element_update_boundingbox(elem);
  
  /* fix boundingrelationship for line_width: */
  if(relationship->rotate) {
    obj->bounding_box.top -= relationship->border_width / 2
      + FONT_HEIGHT + CARDINALITY_DISTANCE;
    obj->bounding_box.left -= relationship->border_width / 2;
    obj->bounding_box.bottom += relationship->border_width / 2
      + FONT_HEIGHT + CARDINALITY_DISTANCE;
    obj->bounding_box.right += relationship->border_width / 2;
  }
  else {
    obj->bounding_box.top -= relationship->border_width / 2;
    obj->bounding_box.left -= relationship->border_width / 2
      + CARDINALITY_DISTANCE + relationship->left_card_width;
    obj->bounding_box.bottom += relationship->border_width / 2;
    obj->bounding_box.right += relationship->border_width / 2
      + CARDINALITY_DISTANCE + relationship->right_card_width;
  }
  obj->position = elem->corner;
  
  element_update_handles(elem);
}

static Object *
relationship_create(Point *startpoint,
	   void *user_data,
	   Handle **handle1,
	   Handle **handle2)
{
  Relationship *relationship;
  Element *elem;
  Object *obj;
  int i;

  relationship = g_malloc(sizeof(Relationship));
  elem = &relationship->element;
  obj = (Object *) relationship;
  
  obj->type = &relationship_type;
  obj->ops = &relationship_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  relationship->properties_dialog = NULL;
  
  relationship->border_width =  attributes_get_default_linewidth();
  relationship->border_color = attributes_get_foreground();
  relationship->inner_color = attributes_get_background();
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &relationship->connections[i];
    relationship->connections[i].object = obj;
    relationship->connections[i].connected = NULL;
  }

  relationship->font = font_getfont("Courier");
  relationship->name = g_strdup(_("Relationship"));
  relationship->left_cardinality = g_strdup("");
  relationship->right_cardinality = g_strdup("");
  relationship->identifying = FALSE;
  relationship->rotate = FALSE;

  relationship->name_width =
    font_string_width(relationship->name, relationship->font, FONT_HEIGHT);
  relationship->left_card_width =
    font_string_width(relationship->left_cardinality, relationship->font, FONT_HEIGHT);
  relationship->right_card_width =
    font_string_width(relationship->right_cardinality, relationship->font, FONT_HEIGHT);

  relationship_update_data(relationship);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  *handle1 = NULL;
  *handle2 = obj->handles[0];  
  return (Object *)relationship;
}

static void
relationship_destroy(Relationship *relationship)
{
  if (relationship->properties_dialog != NULL) {
    gtk_widget_destroy(relationship->properties_dialog->vbox);
    g_free(relationship->properties_dialog);
  }
  element_destroy(&relationship->element);
  g_free(relationship->name);
  g_free(relationship->left_cardinality);
  g_free(relationship->right_cardinality);
}

static Object *
relationship_copy(Relationship *relationship)
{
  int i;
  Relationship *newrelationship;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &relationship->element;
  
  newrelationship = g_malloc(sizeof(Relationship));
  newelem = &newrelationship->element;
  newobj = (Object *) newrelationship;

  element_copy(elem, newelem);

  newrelationship->border_width = relationship->border_width;
  newrelationship->border_color = relationship->border_color;
  newrelationship->inner_color = relationship->inner_color;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newrelationship->connections[i];
    newrelationship->connections[i].object = newobj;
    newrelationship->connections[i].connected = NULL;
    newrelationship->connections[i].pos = relationship->connections[i].pos;
    newrelationship->connections[i].last_pos = relationship->connections[i].last_pos;
  }

  newrelationship->font = relationship->font;
  newrelationship->name = strdup(relationship->name);
  newrelationship->left_cardinality =
    strdup(relationship->left_cardinality);
  newrelationship->right_cardinality =
    strdup(relationship->right_cardinality);
  newrelationship->name_width = relationship->name_width;
  newrelationship->left_card_width = relationship->left_card_width;
  newrelationship->right_card_width = relationship->right_card_width;

  newrelationship->identifying = relationship->identifying;
  newrelationship->rotate = relationship->rotate;
  
  newrelationship->properties_dialog = NULL;

  return (Object *)newrelationship;
}

static void
relationship_save(Relationship *relationship, ObjectNode obj_node,
		  const char *filename)
{
  element_save(&relationship->element, obj_node);

  data_add_real(new_attribute(obj_node, "border_width"),
		relationship->border_width);
  data_add_color(new_attribute(obj_node, "border_color"),
		 &relationship->border_color);
  data_add_color(new_attribute(obj_node, "inner_color"),
		 &relationship->inner_color);
  data_add_string(new_attribute(obj_node, "name"),
		  relationship->name);
  data_add_string(new_attribute(obj_node, "left_card"),
		  relationship->left_cardinality);
  data_add_string(new_attribute(obj_node, "right_card"),
		  relationship->right_cardinality);
  data_add_boolean(new_attribute(obj_node, "identifying"),
		   relationship->identifying);
  data_add_boolean(new_attribute(obj_node, "rotated"),
		   relationship->rotate);
}

static Object *
relationship_load(ObjectNode obj_node, int version, const char *filename)
{
  Relationship *relationship;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  relationship = g_malloc(sizeof(Relationship));
  elem = &relationship->element;
  obj = (Object *) relationship;
  
  obj->type = &relationship_type;
  obj->ops = &relationship_ops;

  element_load(elem, obj_node);
  
  relationship->properties_dialog = NULL;

  relationship->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    relationship->border_width =  data_real( attribute_first_data(attr) );

  relationship->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &relationship->border_color);
  
  relationship->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &relationship->inner_color);

  relationship->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    relationship->name = data_string(attribute_first_data(attr));

  relationship->left_cardinality = NULL;
  attr = object_find_attribute(obj_node, "left_card");
  if (attr != NULL)
    relationship->left_cardinality = data_string(attribute_first_data(attr));

  relationship->right_cardinality = NULL;
  attr = object_find_attribute(obj_node, "right_card");
  if (attr != NULL)
    relationship->right_cardinality = data_string(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "identifying");
  if (attr != NULL)
    relationship->identifying = data_boolean(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "rotated");
  if (attr != NULL)
    relationship->rotate = data_boolean(attribute_first_data(attr));
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &relationship->connections[i];
    relationship->connections[i].object = obj;
    relationship->connections[i].connected = NULL;
  }

  relationship->font = font_getfont("Courier");

  relationship->name_width =
    font_string_width(relationship->name, relationship->font, FONT_HEIGHT);
  relationship->left_card_width =
    font_string_width(relationship->left_cardinality, relationship->font, FONT_HEIGHT);
  relationship->right_card_width =
    font_string_width(relationship->right_cardinality, relationship->font, FONT_HEIGHT);

  relationship_update_data(relationship);

  for (i=0;i<8;i++) {
    obj->handles[i]->type = HANDLE_NON_MOVABLE;
  }

  return (Object *)relationship;
}
