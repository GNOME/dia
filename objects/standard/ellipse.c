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

#include "pixmaps/ellipse.xpm"

#define DEFAULT_WIDTH 2.0
#define DEFAULT_HEIGHT 1.0
#define DEFAULT_BORDER 0.15

typedef struct _Ellipse Ellipse;
typedef struct _EllipsePropertiesDialog EllipsePropertiesDialog;
typedef struct _EllipseDefaultsDialog EllipseDefaultsDialog;
typedef struct _EllipseState EllipseState;

struct _EllipseState {
  ObjectState obj_state;
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
};

struct _Ellipse {
  Element element;

  ConnectionPoint connections[8];
  
  real border_width;
  Color border_color;
  Color inner_color;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
};


typedef struct _EllipseProperties {
  Color *fg_color;
  Color *bg_color;
  real border_width;
  gboolean show_background;
  LineStyle line_style;
  real dashlength;
} EllipseProperties;

struct _EllipsePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *border_width;
  DiaColorSelector *fg_color;
  DiaColorSelector *bg_color;
  GtkToggleButton *show_background;
  DiaLineStyleSelector *line_style;

  Ellipse *ellipse;
};

 struct _EllipseDefaultsDialog {
   GtkWidget *vbox;
 
   GtkToggleButton *show_background;
 };

 
static EllipsePropertiesDialog *ellipse_properties_dialog;
static EllipseDefaultsDialog *ellipse_defaults_dialog;
static EllipseProperties default_properties;

static real ellipse_distance_from(Ellipse *ellipse, Point *point);
static void ellipse_select(Ellipse *ellipse, Point *clicked_point,
			   Renderer *interactive_renderer);
static void ellipse_move_handle(Ellipse *ellipse, Handle *handle,
				Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void ellipse_move(Ellipse *ellipse, Point *to);
static void ellipse_draw(Ellipse *ellipse, Renderer *renderer);
static void ellipse_update_data(Ellipse *ellipse);
static Object *ellipse_create(Point *startpoint,
			      void *user_data,
			      Handle **handle1,
			      Handle **handle2);
static void ellipse_destroy(Ellipse *ellipse);
static Object *ellipse_copy(Ellipse *ellipse);
static GtkWidget *ellipse_get_properties(Ellipse *ellipse);
static ObjectChange *ellipse_apply_properties(Ellipse *ellipse);

static EllipseState *ellipse_get_state(Ellipse *ellipse);
static void ellipse_set_state(Ellipse *ellipse, EllipseState *state);

static void ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename);
static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename);
static GtkWidget *ellipse_get_defaults();
static void ellipse_apply_defaults();

static ObjectTypeOps ellipse_type_ops =
{
  (CreateFunc) ellipse_create,
  (LoadFunc)   ellipse_load,
  (SaveFunc)   ellipse_save,
  (GetDefaultsFunc)   ellipse_get_defaults,
  (ApplyDefaultsFunc) ellipse_apply_defaults
};

ObjectType ellipse_type =
{
  "Standard - Ellipse",   /* name */
  0,                      /* version */
  (char **) ellipse_xpm,  /* pixmap */
  
  &ellipse_type_ops       /* ops */
};

ObjectType *_ellipse_type = (ObjectType *) &ellipse_type;

static ObjectOps ellipse_ops = {
  (DestroyFunc)         ellipse_destroy,
  (DrawFunc)            ellipse_draw,
  (DistanceFunc)        ellipse_distance_from,
  (SelectFunc)          ellipse_select,
  (CopyFunc)            ellipse_copy,
  (MoveFunc)            ellipse_move,
  (MoveHandleFunc)      ellipse_move_handle,
  (GetPropertiesFunc)   ellipse_get_properties,
  (ApplyPropertiesFunc) ellipse_apply_properties,
  (ObjectMenuFunc)      NULL
};

static ObjectChange *
ellipse_apply_properties(Ellipse *ellipse)
{
  ObjectState *old_state;

  if (ellipse != ellipse_properties_dialog->ellipse) {
    message_warning("Ellipse dialog problem:  %p != %p\n",
		    ellipse, ellipse_properties_dialog->ellipse);
    ellipse = ellipse_properties_dialog->ellipse;
  }

  old_state = (ObjectState *)ellipse_get_state(ellipse);

  ellipse->border_width = gtk_spin_button_get_value_as_float(ellipse_properties_dialog->border_width);
  dia_color_selector_get_color(ellipse_properties_dialog->fg_color, &ellipse->border_color);
  dia_color_selector_get_color(ellipse_properties_dialog->bg_color, &ellipse->inner_color);
  ellipse->show_background = gtk_toggle_button_get_active(ellipse_properties_dialog->show_background);
  dia_line_style_selector_get_linestyle(ellipse_properties_dialog->line_style,
					&ellipse->line_style, &ellipse->dashlength);
  
  ellipse_update_data(ellipse);
  return new_object_state_change((Object *)ellipse, old_state, 
				 (GetStateFunc)ellipse_get_state,
				 (SetStateFunc)ellipse_set_state);
}

static GtkWidget *
ellipse_get_properties(Ellipse *ellipse)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *color;
  GtkWidget *linestyle;
  GtkWidget *border_width;
  GtkWidget *checkbox;
  GtkAdjustment *adj;

  if (ellipse_properties_dialog == NULL) {
  
    ellipse_properties_dialog = g_new(EllipsePropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    ellipse_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Border width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    border_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(border_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(border_width), TRUE);
    ellipse_properties_dialog->border_width = GTK_SPIN_BUTTON(border_width);
    gtk_box_pack_start(GTK_BOX (hbox), border_width, TRUE, TRUE, 0);
    gtk_widget_show (border_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);


    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Foreground color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    ellipse_properties_dialog->fg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Background color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    ellipse_properties_dialog->bg_color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Draw background"));
    ellipse_properties_dialog->show_background = GTK_TOGGLE_BUTTON(checkbox);
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
    gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);
 
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    ellipse_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
    gtk_box_pack_start (GTK_BOX (hbox), linestyle, TRUE, TRUE, 0);
    gtk_widget_show (linestyle);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  ellipse_properties_dialog->ellipse = ellipse;
    
  gtk_spin_button_set_value(ellipse_properties_dialog->border_width,
			    ellipse->border_width);
  dia_color_selector_set_color(ellipse_properties_dialog->fg_color,
			       &ellipse->border_color);
  dia_color_selector_set_color(ellipse_properties_dialog->bg_color,
			       &ellipse->inner_color);
  gtk_toggle_button_set_active(ellipse_properties_dialog->show_background,
			       ellipse->show_background);
  dia_line_style_selector_set_linestyle(ellipse_properties_dialog->line_style,
					ellipse->line_style, ellipse->dashlength);
  
  return ellipse_properties_dialog->vbox;
}

static void
ellipse_apply_defaults()
{
  default_properties.show_background = 
    gtk_toggle_button_get_active(ellipse_defaults_dialog->show_background);
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
ellipse_get_defaults()
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *checkbox;
 
  if (ellipse_defaults_dialog == NULL) {

    init_default_values();
 
    ellipse_defaults_dialog = g_new(EllipseDefaultsDialog, 1);
 
    vbox = gtk_vbox_new(FALSE, 5);
    ellipse_defaults_dialog->vbox = vbox;
 
    hbox = gtk_hbox_new(FALSE, 5);
    checkbox = gtk_check_button_new_with_label(_("Draw background"));
    ellipse_defaults_dialog->show_background = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX (hbox), checkbox, TRUE, TRUE, 0);
 
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
    gtk_widget_show (vbox);
    gtk_widget_show (vbox);
  }
 
  gtk_toggle_button_set_active(ellipse_defaults_dialog->show_background, 
			       default_properties.show_background);
 
  return ellipse_defaults_dialog->vbox;
}

static real
ellipse_distance_from(Ellipse *ellipse, Point *point)
{
  Object *obj = &ellipse->element.object;
  return distance_rectangle_point(&obj->bounding_box, point);
}

static void
ellipse_select(Ellipse *ellipse, Point *clicked_point,
	       Renderer *interactive_renderer)
{
  element_update_handles(&ellipse->element);
}

static void
ellipse_move_handle(Ellipse *ellipse, Handle *handle,
		    Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(ellipse!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  assert(handle->id < 8);
  element_move_handle(&ellipse->element, handle->id, to, reason);
  ellipse_update_data(ellipse);
}

static void
ellipse_move(Ellipse *ellipse, Point *to)
{
  ellipse->element.corner = *to;
  ellipse_update_data(ellipse);
}

static void
ellipse_draw(Ellipse *ellipse, Renderer *renderer)
{
  Point center;
  Element *elem;
  
  assert(ellipse != NULL);
  assert(renderer != NULL);

  elem = &ellipse->element;

  center.x = elem->corner.x + elem->width/2;
  center.y = elem->corner.y + elem->height/2;

  if (ellipse->show_background) {
    renderer->ops->set_fillstyle(renderer, FILLSTYLE_SOLID);

    renderer->ops->fill_ellipse(renderer, 
				&center,
				elem->width, elem->height,
				&ellipse->inner_color);
  }

  renderer->ops->set_linewidth(renderer, ellipse->border_width);
  renderer->ops->set_linestyle(renderer, ellipse->line_style);
  renderer->ops->set_dashlength(renderer, ellipse->dashlength);

  renderer->ops->draw_ellipse(renderer, 
			  &center,
			  elem->width, elem->height,
			  &ellipse->border_color);
}

static EllipseState *
ellipse_get_state(Ellipse *ellipse)
{
  EllipseState *state = g_new(EllipseState, 1);

  state->obj_state.free = NULL;
  
  state->border_width = ellipse->border_width;
  state->border_color = ellipse->border_color;
  state->inner_color = ellipse->inner_color;
  state->show_background = ellipse->show_background;
  state->line_style = ellipse->line_style;
  state->dashlength = ellipse->dashlength;

  return state;
}

static void
ellipse_set_state(Ellipse *ellipse, EllipseState *state)
{
  ellipse->border_width = state->border_width;
  ellipse->border_color = state->border_color;
  ellipse->inner_color = state->inner_color;
  ellipse->show_background = state->show_background;
  ellipse->line_style = state->line_style;
  ellipse->dashlength = state->dashlength;

  g_free(state);
  
  ellipse_update_data(ellipse);
}

static void
ellipse_update_data(Ellipse *ellipse)
{
  Element *elem = &ellipse->element;
  Object *obj = (Object *) ellipse;
  Point center;
  real half_x, half_y;

  center.x = elem->corner.x + elem->width / 2.0;
  center.y = elem->corner.y + elem->height / 2.0;
  
  half_x = elem->width * M_SQRT1_2 / 2.0;
  half_y = elem->height * M_SQRT1_2 / 2.0;
    
  /* Update connections: */
  ellipse->connections[0].pos.x = center.x - half_x;
  ellipse->connections[0].pos.y = center.y - half_y;
  ellipse->connections[1].pos.x = center.x;
  ellipse->connections[1].pos.y = elem->corner.y;
  ellipse->connections[2].pos.x = center.x + half_x;
  ellipse->connections[2].pos.y = center.y - half_y;
  ellipse->connections[3].pos.x = elem->corner.x;
  ellipse->connections[3].pos.y = center.y;
  ellipse->connections[4].pos.x = elem->corner.x + elem->width;
  ellipse->connections[4].pos.y = elem->corner.y + elem->height / 2.0;
  ellipse->connections[5].pos.x = center.x - half_x;
  ellipse->connections[5].pos.y = center.y + half_y;
  ellipse->connections[6].pos.x = elem->corner.x + elem->width / 2.0;
  ellipse->connections[6].pos.y = elem->corner.y + elem->height;
  ellipse->connections[7].pos.x = center.x + half_x;
  ellipse->connections[7].pos.y = center.y + half_y;

  element_update_boundingbox(elem);
  /* fix boundingellipse for line_width: */
  obj->bounding_box.top -= ellipse->border_width/2;
  obj->bounding_box.left -= ellipse->border_width/2;
  obj->bounding_box.bottom += ellipse->border_width/2;
  obj->bounding_box.right += ellipse->border_width/2;

  obj->position = elem->corner;

  element_update_handles(elem);
  
}

static Object *
ellipse_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;

  init_default_values();

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &ellipse_type;

  obj->ops = &ellipse_ops;

  elem->corner = *startpoint;
  elem->width = DEFAULT_WIDTH;
  elem->height = DEFAULT_WIDTH;

  ellipse->border_width =  attributes_get_default_linewidth();
  ellipse->border_color = attributes_get_foreground();
  ellipse->inner_color = attributes_get_background();
  ellipse->show_background = default_properties.show_background;
  attributes_get_default_line_style(&ellipse->line_style,
				    &ellipse->dashlength);

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  *handle1 = NULL;
  *handle2 = obj->handles[7];
  return (Object *)ellipse;
}

static void
ellipse_destroy(Ellipse *ellipse)
{
  element_destroy(&ellipse->element);
}

static Object *
ellipse_copy(Ellipse *ellipse)
{
  int i;
  Ellipse *newellipse;
  Element *elem, *newelem;
  Object *newobj;
  
  elem = &ellipse->element;
  
  newellipse = g_malloc(sizeof(Ellipse));
  newelem = &newellipse->element;
  newobj = (Object *) newellipse;

  element_copy(elem, newelem);

  newellipse->border_width = ellipse->border_width;
  newellipse->border_color = ellipse->border_color;
  newellipse->inner_color = ellipse->inner_color;
  newellipse->show_background = ellipse->show_background;
  newellipse->line_style = ellipse->line_style;

  for (i=0;i<8;i++) {
    newobj->connections[i] = &newellipse->connections[i];
    newellipse->connections[i].object = newobj;
    newellipse->connections[i].connected = NULL;
    newellipse->connections[i].pos = ellipse->connections[i].pos;
    newellipse->connections[i].last_pos = ellipse->connections[i].last_pos;
  }

  return (Object *)newellipse;
}


static void
ellipse_save(Ellipse *ellipse, ObjectNode obj_node, const char *filename)
{
  element_save(&ellipse->element, obj_node);

  if (ellipse->border_width != 0.1)
    data_add_real(new_attribute(obj_node, "border_width"),
		  ellipse->border_width);
  
  if (!color_equals(&ellipse->border_color, &color_black))
    data_add_color(new_attribute(obj_node, "border_color"),
		   &ellipse->border_color);
  
  if (!color_equals(&ellipse->inner_color, &color_white))
    data_add_color(new_attribute(obj_node, "inner_color"),
		   &ellipse->inner_color);

  if (!ellipse->show_background)
    data_add_boolean(new_attribute(obj_node, "show_background"),
		     ellipse->show_background);
  
  if (ellipse->line_style != LINESTYLE_SOLID) {
    data_add_enum(new_attribute(obj_node, "line_style"),
		  ellipse->line_style);

    if (ellipse->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	    data_add_real(new_attribute(obj_node, "dashlength"),
			  ellipse->dashlength);
  }
}

static Object *ellipse_load(ObjectNode obj_node, int version, const char *filename)
{
  Ellipse *ellipse;
  Element *elem;
  Object *obj;
  int i;
  AttributeNode attr;

  ellipse = g_malloc(sizeof(Ellipse));
  elem = &ellipse->element;
  obj = (Object *) ellipse;
  
  obj->type = &ellipse_type;
  obj->ops = &ellipse_ops;

  element_load(elem, obj_node);

  ellipse->border_width = 0.1;
  attr = object_find_attribute(obj_node, "border_width");
  if (attr != NULL)
    ellipse->border_width =  data_real( attribute_first_data(attr) );

  ellipse->border_color = color_black;
  attr = object_find_attribute(obj_node, "border_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->border_color);
  
  ellipse->inner_color = color_white;
  attr = object_find_attribute(obj_node, "inner_color");
  if (attr != NULL)
    data_color(attribute_first_data(attr), &ellipse->inner_color);
  
  ellipse->show_background = TRUE;
  attr = object_find_attribute(obj_node, "show_background");
  if (attr != NULL)
    ellipse->show_background = data_boolean(attribute_first_data(attr));

  ellipse->line_style = LINESTYLE_SOLID;
  attr = object_find_attribute(obj_node, "line_style");
  if (attr != NULL)
    ellipse->line_style =  data_enum( attribute_first_data(attr) );

  ellipse->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
	  ellipse->dashlength = data_real(attribute_first_data(attr));

  element_init(elem, 8, 8);

  for (i=0;i<8;i++) {
    obj->connections[i] = &ellipse->connections[i];
    ellipse->connections[i].object = obj;
    ellipse->connections[i].connected = NULL;
  }
  ellipse_update_data(ellipse);

  return (Object *)ellipse;
}




