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
#include "orth_conn.h"
#include "connectionpoint.h"
#include "render.h"
#include "attributes.h"
#include "widgets.h"
#include "message.h"

#include "pixmaps/zigzag.xpm"

#define DEFAULT_WIDTH 0.15

#define HANDLE_MIDDLE HANDLE_CUSTOM1

typedef struct _ZigzaglinePropertiesDialog ZigzaglinePropertiesDialog;
typedef struct _ZigzaglineDefaultsDialog ZigzaglineDefaultsDialog;
typedef struct _ZigzaglineState ZigzaglineState;

struct _ZigzaglineState {
  ObjectState obj_state;
  
  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
};

typedef struct _Zigzagline {
  OrthConn orth;

  Color line_color;
  LineStyle line_style;
  real dashlength;
  real line_width;
  Arrow start_arrow, end_arrow;
} Zigzagline;

struct _ZigzaglinePropertiesDialog {
  GtkWidget *vbox;

  GtkSpinButton *line_width;
  DiaColorSelector *color;
  DiaLineStyleSelector *line_style;

  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;

  Zigzagline *zigzagline;
};

typedef struct _ZigzaglineProperties {
  Color line_color;
  real line_width;
  LineStyle line_style;  
  real dashlength;
  Arrow start_arrow, end_arrow;
} ZigzaglineProperties;

/*struct _ZigzaglineDefaultsDialog {
  GtkWidget *vbox;

  DiaLineStyleSelector *line_style;
  DiaArrowSelector *start_arrow;
  DiaArrowSelector *end_arrow;
  };*/


static ZigzaglinePropertiesDialog *zigzagline_properties_dialog;
/* static ZigzaglineDefaultsDialog *zigzagline_defaults_dialog;
   static ZigzaglineProperties default_properties; */


static void zigzagline_move_handle(Zigzagline *zigzagline, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
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
static ObjectChange *zigzagline_apply_properties(Zigzagline *zigzagline);
static DiaMenu *zigzagline_get_object_menu(Zigzagline *zigzagline,
					   Point *clickedpoint);

static ZigzaglineState *zigzagline_get_state(Zigzagline *zigzagline);
static void zigzagline_set_state(Zigzagline *zigzagline, ZigzaglineState *state);

static void zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
			    const char *filename);
static Object *zigzagline_load(ObjectNode obj_node, int version,
			       const char *filename);
/* static GtkWidget *zigzagline_get_defaults();
   static void zigzagline_apply_defaults(); */

static ObjectTypeOps zigzagline_type_ops =
{
  (CreateFunc)zigzagline_create,   /* create */
  (LoadFunc)  zigzagline_load,     /* load */
  (SaveFunc)  zigzagline_save,      /* save */
  (GetDefaultsFunc)   NULL /*zigzagline_get_defaults*/,
  (ApplyDefaultsFunc) NULL /*zigzagline_apply_defaults*/
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
  (ObjectMenuFunc)      zigzagline_get_object_menu
};

static ObjectChange *
zigzagline_apply_properties(Zigzagline *zigzagline)
{
  ObjectState *old_state;

  if (zigzagline != zigzagline_properties_dialog->zigzagline) {
    message_warning("Zigzagline dialog problem:  %p != %p\n",
		    zigzagline, zigzagline_properties_dialog->zigzagline);
    zigzagline = zigzagline_properties_dialog->zigzagline;
  }
  
  old_state = (ObjectState *)zigzagline_get_state(zigzagline);

  zigzagline->line_width = gtk_spin_button_get_value_as_float(zigzagline_properties_dialog->line_width);
  dia_color_selector_get_color(zigzagline_properties_dialog->color, &zigzagline->line_color);
  dia_line_style_selector_get_linestyle(zigzagline_properties_dialog->line_style, &zigzagline->line_style, &zigzagline->dashlength);

  zigzagline->start_arrow = dia_arrow_selector_get_arrow(zigzagline_properties_dialog->start_arrow);
  zigzagline->end_arrow = dia_arrow_selector_get_arrow(zigzagline_properties_dialog->end_arrow);
  
  zigzagline_update_data(zigzagline);
  return new_object_state_change((Object *)zigzagline, old_state, 
				 (GetStateFunc)zigzagline_get_state,
				 (SetStateFunc)zigzagline_set_state);
}

static GtkWidget *
zigzagline_get_properties(Zigzagline *zigzagline)
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

  if (zigzagline_properties_dialog == NULL) {
  
    zigzagline_properties_dialog = g_new(ZigzaglinePropertiesDialog, 1);

    vbox = gtk_vbox_new(FALSE, 5);
    zigzagline_properties_dialog->vbox = vbox;

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line width:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    adj = (GtkAdjustment *) gtk_adjustment_new(0.1, 0.00, 10.0, 0.01, 0.0, 0.0);
    line_width = gtk_spin_button_new(adj, 1.0, 2);
    gtk_spin_button_set_wrap(GTK_SPIN_BUTTON(line_width), TRUE);
    gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(line_width), TRUE);
    zigzagline_properties_dialog->line_width = GTK_SPIN_BUTTON(line_width);
    gtk_box_pack_start(GTK_BOX (hbox), line_width, TRUE, TRUE, 0);
    gtk_widget_show (line_width);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Color:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    color = dia_color_selector_new();
    zigzagline_properties_dialog->color = DIACOLORSELECTOR(color);
    gtk_box_pack_start (GTK_BOX (hbox), color, TRUE, TRUE, 0);
    gtk_widget_show (color);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Line style:"));
    align = gtk_alignment_new(0.0,0.0,0.0,0.0);
    gtk_container_add(GTK_CONTAINER(align), label);
    gtk_box_pack_start (GTK_BOX (hbox), align, FALSE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show(align);
    linestyle = dia_line_style_selector_new();
    zigzagline_properties_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    zigzagline_properties_dialog->start_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("End arrow:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    arrow = dia_arrow_selector_new();
    zigzagline_properties_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  zigzagline_properties_dialog->zigzagline = zigzagline;
    
  zigzagline_properties_dialog->zigzagline = zigzagline;
    
  gtk_spin_button_set_value(zigzagline_properties_dialog->line_width,
			    zigzagline->line_width);
  dia_color_selector_set_color(zigzagline_properties_dialog->color,
			       &zigzagline->line_color);
  dia_line_style_selector_set_linestyle(zigzagline_properties_dialog->line_style,
					zigzagline->line_style, zigzagline->dashlength);
  dia_arrow_selector_set_arrow(zigzagline_properties_dialog->start_arrow,
					 zigzagline->start_arrow);
  dia_arrow_selector_set_arrow(zigzagline_properties_dialog->end_arrow,
					 zigzagline->end_arrow);
  
  return zigzagline_properties_dialog->vbox;
}

/*
static void
zigzagline_init_defaults() {
  static int defaults_initialized = 0;

  if (!defaults_initialized) {
    default_properties.start_arrow.length = 0.8;
    default_properties.start_arrow.width = 0.8;
    default_properties.end_arrow.length = 0.8;
    default_properties.end_arrow.width = 0.8;
    default_properties.dashlength = 1.0;
    defaults_initialized = 1;
  }
}

static void
zigzagline_apply_defaults()
{
   dia_line_style_selector_get_linestyle(zigzagline_defaults_dialog->line_style, &default_properties.line_style, &default_properties.dashlength);
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
    label = gtk_label_new(_("Line style:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    gtk_widget_show (label);
    linestyle = dia_line_style_selector_new();
    zigzagline_defaults_dialog->line_style = DIALINESTYLESELECTOR(linestyle);
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
    zigzagline_defaults_dialog->start_arrow = DIAARROWSELECTOR(arrow);
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
    zigzagline_defaults_dialog->end_arrow = DIAARROWSELECTOR(arrow);
    gtk_box_pack_start (GTK_BOX (hbox), arrow, TRUE, TRUE, 0);
    gtk_widget_show (arrow);
    gtk_widget_show(hbox);
    gtk_box_pack_start (GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

    gtk_widget_show (vbox);
  }

  dia_line_style_selector_set_linestyle(zigzagline_defaults_dialog->line_style,
					default_properties.line_style, default_properties.dashlength);
  dia_arrow_selector_set_arrow(zigzagline_defaults_dialog->start_arrow,
					 default_properties.start_arrow);
  dia_arrow_selector_set_arrow(zigzagline_defaults_dialog->end_arrow,
					 default_properties.end_arrow);

  return zigzagline_defaults_dialog->vbox;
}
*/

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
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
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
  
  renderer->ops->set_linewidth(renderer, zigzagline->line_width);
  renderer->ops->set_linestyle(renderer, zigzagline->line_style);
  renderer->ops->set_dashlength(renderer, zigzagline->dashlength);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &zigzagline->line_color);
  
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

  /*zigzagline_init_defaults();*/
  zigzagline = g_malloc(sizeof(Zigzagline));
  orth = &zigzagline->orth;
  obj = (Object *) zigzagline;
  
  obj->type = &zigzagline_type;
  obj->ops = &zigzagline_ops;
  
  orthconn_init(orth, startpoint);

  zigzagline_update_data(zigzagline);

  zigzagline->line_width =  attributes_get_default_linewidth();
  zigzagline->line_color = attributes_get_foreground();
  attributes_get_default_line_style(&zigzagline->line_style,
				    &zigzagline->dashlength);
  zigzagline->start_arrow = attributes_get_default_start_arrow();
  zigzagline->end_arrow = attributes_get_default_end_arrow();
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return (Object *)zigzagline;
}

static void
zigzagline_destroy(Zigzagline *zigzagline)
{
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

  return (Object *)newzigzagline;
}

static ZigzaglineState *
zigzagline_get_state(Zigzagline *zigzagline)
{
  ZigzaglineState *state = g_new(ZigzaglineState, 1);

  state->obj_state.free = NULL;
  
  state->line_color = zigzagline->line_color;
  state->line_style = zigzagline->line_style;
  state->dashlength = zigzagline->dashlength;
  state->line_width = zigzagline->line_width;
  state->start_arrow = zigzagline->start_arrow;
  state->end_arrow = zigzagline->end_arrow;

  return state;
}

static void
zigzagline_set_state(Zigzagline *zigzagline, ZigzaglineState *state)
{
  zigzagline->line_color = state->line_color;
  zigzagline->line_style = state->line_style;
  zigzagline->dashlength = state->dashlength;
  zigzagline->line_width = state->line_width;
  zigzagline->start_arrow = state->start_arrow;
  zigzagline->end_arrow = state->end_arrow;

  g_free(state);
  
  zigzagline_update_data(zigzagline);
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
}

static ObjectChange *
zigzagline_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static ObjectChange *
zigzagline_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  zigzagline_update_data((Zigzagline *)obj);
  return change;
}

static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), zigzagline_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), zigzagline_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Zigzagline",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
zigzagline_get_object_menu(Zigzagline *zigzagline, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &zigzagline->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}


static void
zigzagline_save(Zigzagline *zigzagline, ObjectNode obj_node,
		const char *filename)
{
  orthconn_save(&zigzagline->orth, obj_node);

  if (!color_equals(&zigzagline->line_color, &color_black))
    data_add_color(new_attribute(obj_node, "line_color"),
		   &zigzagline->line_color);
  
  if (zigzagline->line_width != 0.1)
    data_add_real(new_attribute(obj_node, "line_width"),
		  zigzagline->line_width);
  
  if (zigzagline->line_style != LINESTYLE_SOLID)
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

  if (zigzagline->line_style != LINESTYLE_SOLID && 
      zigzagline->dashlength != DEFAULT_LINESTYLE_DASHLEN)
	  data_add_real(new_attribute(obj_node, "dashlength"),
		  zigzagline->dashlength);
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

  zigzagline->dashlength = DEFAULT_LINESTYLE_DASHLEN;
  attr = object_find_attribute(obj_node, "dashlength");
  if (attr != NULL)
	  zigzagline->dashlength = data_real(attribute_first_data(attr));
  
  zigzagline_update_data(zigzagline);

  return (Object *)zigzagline;
}
