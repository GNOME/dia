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

#include "config.h"
#include "intl.h"
#include "object.h"
#include "element.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"

#include "pixmaps/box.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.25

typedef struct _Box Box;
typedef struct _BoxPropertiesDialog BoxPropertiesDialog;
typedef struct _BoxDefaultsDialog BoxDefaultsDialog;
typedef struct _BoxState BoxState;

struct _BoxState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real corner_radius;
};

struct _Box {
  Element element;

  ConnectionPoint connections[8];

  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
  real corner_radius;
};

typedef struct _BoxProperties {
  Color *fg_color;
  Color *bg_color;
  gboolean show_background;
  real border_width;
  LineStyle line_style;
  real dashlength;
  real corner_radius;
} BoxProperties;

struct _BoxPropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;
  GtkSpinButton *corner_radius;

  Box *box;
};

struct _BoxDefaultsDialog {
  GtkWidget *vbox;

  GtkToggleButton *show_background;
  GtkSpinButton *corner_radius;
};


static BoxPropertiesDialog *box_properties_dialog;
static BoxDefaultsDialog *box_defaults_dialog;
static BoxProperties default_properties;

static real box_distance_from(Box *box, Point *point);
static void box_select(Box *box, Point *clicked_point,
		       Renderer *interactive_renderer);
static void box_move_handle(Box *box, Handle *handle,
			    Point *to, HandleMoveReason reason, 
			    ModifierKeys modifiers);
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
static ObjectChange *box_apply_properties(Box *box);

static BoxState *box_get_state(Box *box);
static void box_set_state(Box *box, BoxState *state);

static void box_save(Box *box, ObjectNode obj_node, const char *filename);
static Object *box_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *box_get_defaults();
static void box_apply_defaults();

static ObjectTypeOps box_type_ops =
{
  (CreateFunc) box_create,
  (LoadFunc)   box_load,
  (SaveFunc)   box_save,
  (GetDefaultsFunc)   box_get_defaults,
  (ApplyDefaultsFunc) box_apply_defaults
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
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
box_apply_properties(Box *box)
{
  ObjectState *old_state;
  if (box != box_properties_dialog->box) {
    message_warning("Box dialog problem:  %p != %p\n", box,
		    box_properties_dialog->box);
    box = box_properties_dialog->box;
  }

  old_state = (ObjectState *)box_get_state(box);
  
  box->border_width = gtk_spin_button_get_value_as_float(box_properties_dialog->border_width);
  dia_color_selector_get_color(box_properties_dialog->fg_color, &box->border_color);
  dia_color_selector_get_color(box_properties_dialog->bg_color, &box->inner_color);
  box->show_background = gtk_toggle_button_get_active(box_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(box_properties_dialog->line_style, &box->line_style, &box->dashlength);
  box->corner_radius = gtk_spin_button_get_value_as_float(box_properties_dialog->corner_radius);
  
  box_update_data(box);
  return new_object_state_change((Object *)box, old_state, 
				 (GetStateFunc)box_get_state,
				 (SetStateFunc)box_set_state);
}

static GtkWidget *
box_get_properties(Box *box)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *checkbox;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkAdjustment *adj;

  if (box_properties_dialog == NULL) {
    box_properties_dialog = g_new(BoxPropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    gtk_object_ref(GTK_OBJECT(vbox));
    gtk_object_sink(GTK_OBJECT(vbox));
    box_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    box_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    box_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    box_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Draw background"));
    box_properties_dialog->show_background = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    box_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Corner rounding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    box_properties_dialog->corner_radius = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
  }

  box_properties_dialog->box = box;
    
  gtk_spin_button_set_value(box_properties_dialog->border_width,
			    box->border_width);
  dia_color_selector_set_color(box_properties_dialog->fg_color,
			       &box->border_color);
  dia_color_selector_set_color(box_properties_dialog->bg_color,
			       &box->inner_color);
  gtk_toggle_button_set_active(box_properties_dialog->show_background, 
			       box->show_background);
  dia_line_style_selector_set_linestyle(box_properties_dialog->line_style,
					box->line_style, box->dashlength);
  gtk_spin_button_set_value(box_properties_dialog->corner_radius,
			    box->corner_radius);
  
  return box_properties_dialog->vbox;
}
static void
box_apply_defaults()
{
  default_properties.corner_radius = gtk_spin_button_get_value_as_float(box_defaults_dialog->corner_radius);
  default_properties.show_background = gtk_toggle_button_get_active(box_defaults_dialog->show_background);
}

static void
init_default_values() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.show_background = 1;
    defaults_initialized = 1;
  }
}

static GtkWidget *
box_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkbox;
  GtkWidget *corner_radius;
  GtkAdjustment *adj;

  if (box_defaults_dialog == NULL) {
  
    init_default_values();

    box_defaults_dialog = g_new(BoxDefaultsDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    box_defaults_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Draw background"));
    box_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Corner rounding:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.0, 10.0, 0.1, 0.0, 0.0);
    corner_radius = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(corner_radius), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(corner_radius), TRUE);
    box_defaults_dialog->corner_radius = GTK_SPIN_BUTTON(corner_radius);
    gtk_box_pack_start(GTK_BOX (hbox), corner_radius, TRUE, TRUE, 0);
    gtk_widget_show (corner_radius);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }

  gtk_toggle_button_set_active(box_defaults_dialog->show_background, 
			       default_properties.show_background);
  gtk_spin_button_set_value(box_defaults_dialog->corner_radius, 
			    default_properties.corner_radius);

  return box_defaults_dialog->vbox;
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
  real radius;

  element_update_handles(&box->element);

  if (box->corner_radius > 0) {
    Element *elem = (Element *)box;
    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    radius *= (1-M_SQRT1_2);

    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
}

static void
box_move_handle(Box *box, Handle *handle,
		Point *to, HandleMoveReason reason, ModifierKeys modifiers)
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
  real radius;
  
  assert(box != NULL);
  assert(renderer != NULL);

  elem = &box->element;

  lr_corner.x = elem->corner.x + elem->width;
  lr_corner.y = elem->corner.y + elem->height;

  if (box->show_background) {
  renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);
  
  /* Problem:  How do we make the fill with rounded corners? */
  if (box->corner_radius > 0) {
    Point start, end, center;

    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    start.x = center.x = elem->corner.x+radius;
    end.x = lr_corner.x-radius;
    start.y = elem->corner.y;
    end.y = lr_corner.y;
    renderer->ops->fill_rect(renderer, &start, &end, &box->inner_color);

    center.y = elem->corner.y+radius;
    renderer->ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    90.0, 180.0, &box->inner_color);
    center.x = end.x;
    renderer->ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    0.0, 90.0, &box->inner_color);

    start.x = elem->corner.x;
    start.y = elem->corner.y+radius;
    end.x = lr_corner.x;
    end.y = center.y = lr_corner.y-radius;
    renderer->ops->fill_rect(renderer, &start, &end, &box->inner_color);

    center.y = lr_corner.y-radius;
    center.x = elem->corner.x+radius;
    renderer->ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    180.0, 270.0, &box->inner_color);
    center.x = lr_corner.x-radius;
    renderer->ops->fill_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    270.0, 360.0, &box->inner_color);
  } else {
  renderer->ops->fill_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->inner_color);
  }
  }

  renderer->ops->set_linewidth(renderer, box->border_width);
  renderer->ops->set_linestyle(renderer, box->line_style);
  renderer->ops->set_dashlength(renderer, box->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);

  if (box->corner_radius > 0) {
    Point start, end, center;

    radius = box->corner_radius;
    radius = MIN(radius, elem->width/2);
    radius = MIN(radius, elem->height/2);
    start.x = center.x = elem->corner.x+radius;
    end.x = lr_corner.x-radius;
    start.y = end.y = elem->corner.y;
    renderer->ops->draw_line(renderer, &start, &end, &box->border_color);
    start.y = end.y = lr_corner.y;
    renderer->ops->draw_line(renderer, &start, &end, &box->border_color);

    center.y = elem->corner.y+radius;
    renderer->ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    90.0, 180.0, &box->border_color);
    center.x = end.x;
    renderer->ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    0.0, 90.0, &box->border_color);

    start.y = elem->corner.y+radius;
    start.x = end.x = elem->corner.x;
    end.y = center.y = lr_corner.y-radius;
    renderer->ops->draw_line(renderer, &start, &end, &box->border_color);
    start.x = end.x = lr_corner.x;
    renderer->ops->draw_line(renderer, &start, &end, &box->border_color);

    center.y = lr_corner.y-radius;
    center.x = elem->corner.x+radius;
    renderer->ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    180.0, 270.0, &box->border_color);
    center.x = lr_corner.x-radius;
    renderer->ops->draw_arc(renderer, &center, 
			    2.0*radius, 2.0*radius,
			    270.0, 360.0, &box->border_color);
  } else {
  renderer->ops->draw_rect(renderer, 
			   &elem->corner,
			   &lr_corner, 
			   &box->border_color);
  }
}

static BoxState *
box_get_state(Box *box)
{
  BoxState *state = g_new(BoxState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = box->border_width;
  state->border_color = box->border_color;
  state->inner_color = box->inner_color;
  state->show_background = box->show_background;
  state->line_style = box->line_style;
  state->dashlength = box->dashlength;
  state->corner_radius = box->corner_radius;

  return state;
}

static void
box_set_state(Box *box, BoxState *state)
{
  box->border_width = state->border_width;
  box->border_color = state->border_color;
  box->inner_color = state->inner_color;
  box->show_background = state->show_background;
  box->line_style = state->line_style;
  box->dashlength = state->dashlength;
  box->corner_radius = state->corner_radius;

  g_free(state);
  
  box_update_data(box);
}


static void
box_update_data(Box *box)
{
  Element *elem = &box->element;
  Object *obj = (Object *) box;
  real radius;
  
  radius = box->corner_radius;
  radius = MIN(radius, elem->width/2);
  radius = MIN(radius, elem->height/2);
  radius *= (1-M_SQRT1_2);
  
  /* Update connections: */
  box->connections[0].pos.x = elem->corner.x + radius;
  box->connections[0].pos.y = elem->corner.y + radius;
  box->connections[1].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[1].pos.y = elem->corner.y;
  box->connections[2].pos.x = elem->corner.x + elem->width - radius;
  box->connections[2].pos.y = elem->corner.y + radius;
  box->connections[3].pos.x = elem->corner.x;
  box->connections[3].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[4].pos.x = elem->corner.x + elem->width;
  box->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  box->connections[5].pos.x = elem->corner.x + radius;
  box->connections[5].pos.y = elem->corner.y + elem->height - radius;
  box->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  box->connections[6].pos.y = elem->corner.y + elem->height;
  box->connections[7].pos.x = elem->corner.x + elem->width - radius;
  box->connections[7].pos.y = elem->corner.y + elem->height - radius;

  element_update_boundingbox(elem);
  /* fix boundingbox for line_width: */
  obj->bounding_box.top -= box->border_width/2;
  obj->bounding_box.left -= box->border_width/2;
  obj->bounding_box.bottom += box->border_width/2;
  obj->bounding_box.right += box->border_width/2;
  
  obj->position = elem->corner;
  
  element_update_handles(elem);

  if (radius > 0.0) {
    /* Fix the handles, too */
    elem->resize_handles[0].pos.x += radius;
    elem->resize_handles[0].pos.y += radius;
    elem->resize_handles[2].pos.x -= radius;
    elem->resize_handles[2].pos.y += radius;
    elem->resize_handles[5].pos.x += radius;
    elem->resize_handles[5].pos.y -= radius;
    elem->resize_handles[7].pos.x -= radius;
    elem->resize_handles[7].pos.y -= radius;
  }
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

  init_default_values();

  box = g_malloc(sizeof(Box));
  elem = &box->element;
  obj = (Object *) box;
  
  obj->type = &box_type;

  obj->ops = &box_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  box->border_width =  attributes_get_default_linewidth();
  box->border_color = attributes_get_foreground();
  box->inner_color = attributes_get_background();
  box->show_background = default_properties.show_background;
  attributes_get_default_line_style(&box->line_style, &box->dashlength);
  box->corner_radius = default_properties.corner_radius;
  
  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box);

  *handle1 = NULL;
  *handle2 = obj->handles[7];  
  return (Object *)box;
}

static void
box_destroy(Box *box)
{
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
  newbox->show_background = box->show_background;
  newbox->line_style = box->line_style;
  newbox->dashlength = box->dashlength;
  newbox->corner_radius = box->corner_radius;
  
  for (i=0;i<8;i++) {
    newobj->connections[i] = &newbox->connections[i];
    newbox->connections[i].object = newobj;
    newbox->connections[i].connected = NULL;
    newbox->connections[i].pos = box->connections[i].pos;
    newbox->connections[i].last_pos = box->connections[i].last_pos;
  }

  return (Object *)newbox;
}

static void
box_save(Box *box, ObjectNode obj_node, const char *filename)
{
  element_save(&box->element, obj_node);

  if (box->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  box->border_width);
  
  if (!color_equals(&box->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &box->border_color);
  
  if (!color_equals(&box->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &box->inner_color);
  
  data_add_boolean(new_attribute(obj_node, "show_background"), box->show_background);

  if (box->line_style != LINESTYLE_SOLID)
    data_add_enum(new_attribute(obj_node, "line_style"),
		  box->line_style);
  
  if (box->line_style != LINESTYLE_SOLID &&
      box->dashlength != DEFAULT_LINESTYLE_DASHLEN)
    data_add_real(new_attribute(obj_node, "dashlength"),
                  box->dashlength);

  if (box->corner_radius > 0.0)
    data_add_real(new_attribute(obj_node, "corner_radius"),
		  box->corner_radius);
}

static Object *
box_load(ObjectNode obj_node, int version, const char *filename)
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
  
  box->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    box->show_background = data_boolean( attribute_first_data(attr) );

  box->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    box->line_style =  data_enum( attribute_first_data(attr) );

  box->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
    box->dashlength = data_real(attribute_first_data(attr));

  box->corner_radius = 0.0;
  attr = object_find_attribute(obj_node, "corner_radius");
  if (attr != NULL)
    box->corner_radius =  data_real( attribute_first_data(attr) );

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &box->connections[i];
    box->connections[i].object = obj;
    box->connections[i].connected = NULL;
  }

  box_update_data(box);

  return (Object *)box;
}
