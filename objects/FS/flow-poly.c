/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
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
#include <stdio.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "arrows.h"
#include "diamenu.h"
#include "text.h"

#include "pixmaps/flow.xpm"

Color flow_color_energy   = { 1.0f, 0.0f, 0.0f };
Color flow_color_material = { 0.8f, 0.0f, 0.8f };
Color flow_color_signal   = { 0.0f, 0.0f, 1.0f };

typedef struct _Flow Flow;
typedef struct _FlowDialog FlowDialog;

typedef enum {
    FLOW_ENERGY,
    FLOW_MATERIAL,
    FLOW_SIGNAL
} FlowType;

struct _Flow {
  Connection connection;

  Handle text_handle;

  Text* text;
  FlowType type;
};

struct _FlowDialog {
  GtkWidget *dialog;
  
  GtkWidget *text;

  GtkWidget *m_energy;
  GtkWidget *m_material;
  GtkWidget *m_signal;
};
  
#define FLOW_WIDTH 0.1
#define FLOW_MATERIAL_WIDTH 0.2
#define FLOW_DASHLEN 0.4
#define FLOW_FONTHEIGHT 0.6
#define FLOW_ARROWLEN 0.8
#define FLOW_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)

static Font *flow_font = NULL;

static FlowDialog *properties_dialog;
static FlowDialog *defaults_dialog;

/* Remember the most recently applied flow type and use it to
   set the type for any newly created flows
 */
static FlowType flow_most_recent_type = FLOW_ENERGY ;
static Text* flow_default_label = 0 ;

static void flow_move_handle(Flow *flow, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void flow_move(Flow *flow, Point *to);
static void flow_select(Flow *flow, Point *clicked_point,
			      Renderer *interactive_renderer);
static void flow_draw(Flow *flow, Renderer *renderer);
static Object *flow_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real flow_distance_from(Flow *flow, Point *point);
static void flow_update_data(Flow *flow);
static void flow_destroy(Flow *flow);
static Object *flow_copy(Flow *flow);
static GtkWidget *flow_get_properties(Flow *flow);
static ObjectChange *flow_apply_properties(Flow *flow);
static GtkWidget *flow_get_defaults();
static void flow_apply_defaults();
static void flow_save(Flow *flow, ObjectNode obj_node,
			 const char *filename);
static Object *flow_load(ObjectNode obj_node, int version,
			    const char *filename);
static DiaMenu *flow_get_object_menu(Flow *flow, Point *clickedpoint) ;


static ObjectTypeOps flow_type_ops =
{
  (CreateFunc)		flow_create,
  (LoadFunc)		flow_load,
  (SaveFunc)		flow_save,
  (GetDefaultsFunc)	flow_get_defaults,
  (ApplyDefaultsFunc)	flow_apply_defaults
} ;

ObjectType flow_type =
{
  "FS - Flow",         /* name */
  0,                   /* version */
  (char **) flow_xpm,  /* pixmap */
  &flow_type_ops       /* ops */
};

static ObjectOps flow_ops = {
  (DestroyFunc)         flow_destroy,
  (DrawFunc)            flow_draw,
  (DistanceFunc)        flow_distance_from,
  (SelectFunc)          flow_select,
  (CopyFunc)            flow_copy,
  (MoveFunc)            flow_move,
  (MoveHandleFunc)      flow_move_handle,
  (GetPropertiesFunc)   flow_get_properties,
  (ApplyPropertiesFunc) flow_apply_properties,
  (ObjectMenuFunc)      flow_get_object_menu,
};

static real
flow_distance_from(Flow *flow, Point *point)
{
  Point *endpoints;
  real linedist;
  real textdist;
  
  endpoints = &flow->connection.endpoints[0];
  
  linedist = distance_line_point(&endpoints[0], &endpoints[1], 
				 flow->type == FLOW_MATERIAL ? FLOW_MATERIAL_WIDTH : FLOW_WIDTH, 
				 point);
  textdist = text_distance_from( flow->text, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
flow_select(Flow *flow, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  text_set_cursor(flow->text, clicked_point, interactive_renderer);
  text_grab_focus(flow->text, (Object *)flow);

  connection_update_handles(&flow->connection);
}

static void
flow_move_handle(Flow *flow, Handle *handle,
		 Point *to, HandleMoveReason reason)
{
  Point p1, p2;
  Point *endpoints;
  
  assert(flow!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    flow->text->position = *to;
  } else  {
    real dest_length ;
    real orig_length2 ;
    real along_mag, norm_mag ;
    Point along ;

    endpoints = &flow->connection.endpoints[0]; 
    p1 = flow->text->position ;
    point_sub( &p1, &endpoints[0] ) ;

    p2 = endpoints[1] ;
    point_sub( &p2, &endpoints[0] ) ;
    orig_length2= point_dot( &p2, &p2 ) ;
    along = p2 ;
    if ( orig_length2 > 1e-5 ) {
       along_mag = point_dot( &p2, &p1 )/sqrt(orig_length2) ;
       along_mag *= along_mag ;
       norm_mag = point_dot( &p1, &p1 ) ;
       norm_mag = (real)sqrt( (double)( norm_mag - along_mag ) ) ;
       along_mag = (real)sqrt( along_mag/orig_length2 ) ;
       if ( p1.x*p2.y - p1.y*p2.x > 0.0 )
	  norm_mag = -norm_mag ;
    } else {
       along_mag = 0.5 ;
       norm_mag = (real)sqrt( (double) point_dot( &p1, &p1 ) ) ;
    }

    connection_move_handle(&flow->connection, handle->id, to, reason);

    p2 = endpoints[1] ;
    point_sub( &p2, &endpoints[0] ) ;
    flow->text->position = endpoints[0] ;
    along = p2 ;
    p2.x = -along.y ;
    p2.y = along.x ;
    dest_length = point_dot( &p2, &p2 ) ;
    if ( dest_length > 1e-5 ) {
       point_normalize( &p2 ) ;
    } else {
       p2.x = 0.0 ; p2.y = -1.0 ; 
    }
    point_scale( &p2, norm_mag ) ;
    point_scale( &along, along_mag ) ;
    point_add( &flow->text->position, &p2 ) ;
    point_add( &flow->text->position, &along ) ;
  }

  flow_update_data(flow);
}

static void
flow_move(Flow *flow, Point *to)
{
  Point start_to_end;
  Point *endpoints = &flow->connection.endpoints[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);
 
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&flow->text->position, &delta);
  
  flow_update_data(flow);
}

static void
flow_draw(Flow *flow, Renderer *renderer)
{
  Point *endpoints, p1, p2, px;
  ArrowType arrow_type;
  int n1 = 1, n2 = 0;
  char *mname;
  Color* render_color ;

  assert(flow != NULL);
  assert(renderer != NULL);

  arrow_type = ARROW_FILLED_TRIANGLE;
 
  endpoints = &flow->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, FLOW_WIDTH);

  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);


  switch (flow->type) {
  case FLOW_SIGNAL:
      renderer->ops->set_dashlength(renderer, FLOW_DASHLEN);
      renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
      render_color = &flow_color_signal ;
      break ;
  case FLOW_MATERIAL:
      renderer->ops->set_linewidth(renderer, FLOW_MATERIAL_WIDTH ) ;
      renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
      render_color = &flow_color_material ;
      break ;
  case FLOW_ENERGY:
      render_color = &flow_color_energy ;
      renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  }

  p1 = endpoints[n1];
  p2 = endpoints[n2];

  renderer->ops->draw_line(renderer,
			   &p1, &p2,
			   render_color); 

  arrow_draw(renderer, arrow_type,
	     &p1, &p2,
	     FLOW_ARROWLEN, FLOW_ARROWWIDTH, FLOW_WIDTH,
	     render_color, &color_white);

  renderer->ops->set_font(renderer, flow_font,
			  FLOW_FONTHEIGHT);

  text_draw(flow->text, renderer);
}

static Object *
flow_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Flow *flow;
  Connection *conn;
  Object *obj;
  Point p ;
  Point n ;

  flow = g_malloc(sizeof(Flow));

  conn = &flow->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  conn->endpoints[1].x += 1.5;
 
  obj = (Object *) flow;
  
  obj->type = &flow_type;
  obj->ops = &flow_ops;
  
  connection_init(conn, 3, 0);

  flow->type = flow_most_recent_type ;

  p = conn->endpoints[1] ;
  point_sub( &p, &conn->endpoints[0] ) ;
  point_scale( &p, 0.5 ) ;
  n.x = p.y ;
  n.y = -p.x ;
  point_normalize( &n ) ;
  point_scale( &n, 0.5*FLOW_FONTHEIGHT ) ;
  point_add( &p, &n ) ;
  point_add( &p, &conn->endpoints[0] ) ;

  if ( flow_default_label ) {
    flow->text = text_copy( flow_default_label ) ;
    text_set_position( flow->text, &p ) ;
  } else {
    Color* color ;

    if (flow_font == NULL)
      flow_font = font_getfont("Helvetica-Oblique");

    switch (flow->type) {
    case FLOW_ENERGY:
      color = &flow_color_energy ;
      break ;
    case FLOW_MATERIAL:
      color = &flow_color_material ;
      break ;
    case FLOW_SIGNAL:
      color = &flow_color_signal ;
      break ;
    }

    flow->text = new_text("", flow_font, FLOW_FONTHEIGHT, 
			  &p, color, ALIGN_CENTER);
  }

  flow->text_handle.id = HANDLE_MOVE_TEXT;
  flow->text_handle.type = HANDLE_MINOR_CONTROL;
  flow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  flow->text_handle.connected_to = NULL;
  obj->handles[2] = &flow->text_handle;
  
  flow_update_data(flow);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)flow;
}


static void
flow_destroy(Flow *flow)
{
  connection_destroy(&flow->connection);
  text_destroy(flow->text) ;
}

static Object *
flow_copy(Flow *flow)
{
  Flow *newflow;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &flow->connection;
  
  newflow = g_malloc(sizeof(Flow));
  newconn = &newflow->connection;
  newobj = (Object *) newflow;

  connection_copy(conn, newconn);

  newflow->text_handle = flow->text_handle;
  newobj->handles[2] = &newflow->text_handle;

  newflow->text = text_copy(flow->text);
  newflow->type = flow->type;

  return (Object *)newflow;
}


static void
flow_update_data(Flow *flow)
{
  Connection *conn = &flow->connection;
  Object *obj = (Object *) flow;
  Rectangle rect;
  Color* color ;
  
  obj->position = conn->endpoints[0];

  switch (flow->type) {
  case FLOW_ENERGY:
	color = &flow_color_energy ;
	break ;
  case FLOW_MATERIAL:
     color = &flow_color_material ;
     break ;
  case FLOW_SIGNAL:
     color = &flow_color_signal ;
     break ;
  }
  text_set_color( flow->text, color ) ;

  flow->text_handle.pos = flow->text->position;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  text_calc_boundingbox(flow->text, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);

  /* fix boundingbox for flow_width and arrow: */
  obj->bounding_box.top -= FLOW_WIDTH/2 + FLOW_ARROWLEN;
  obj->bounding_box.left -= FLOW_WIDTH/2 + FLOW_ARROWLEN;
  obj->bounding_box.bottom += FLOW_WIDTH/2 + FLOW_ARROWLEN;
  obj->bounding_box.right += FLOW_WIDTH/2 + FLOW_ARROWLEN;
}


static void
flow_save(Flow *flow, ObjectNode obj_node, const char *filename)
{
  connection_save(&flow->connection, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		flow->text) ;
  data_add_int(new_attribute(obj_node, "type"),
		   flow->type);
}

static Object *
flow_load(ObjectNode obj_node, int version, const char *filename)
{
  Flow *flow;
  AttributeNode attr;
  Connection *conn;
  Object *obj;

  if (flow_font == NULL)
    flow_font = font_getfont("Helvetica-Oblique");

  flow = g_malloc(sizeof(Flow));

  conn = &flow->connection;
  obj = (Object *) flow;

  obj->type = &flow_type;
  obj->ops = &flow_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 3, 0);

  flow->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    flow->text = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    flow->type = (FlowType)data_int(attribute_first_data(attr));

  flow->text_handle.id = HANDLE_MOVE_TEXT;
  flow->text_handle.type = HANDLE_MINOR_CONTROL;
  flow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  flow->text_handle.connected_to = NULL;
  obj->handles[2] = &flow->text_handle;
  
  flow_update_data(flow);
  
  return (Object *)flow;
}


static ObjectChange *
flow_apply_properties(Flow *flow)
{
  FlowDialog *prop_dialog;
  
  prop_dialog = properties_dialog;

  text_set_string(flow->text,
                  gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),
                                                 0, -1));

  if (GTK_TOGGLE_BUTTON(prop_dialog->m_energy)->active)
      flow->type = FLOW_ENERGY;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_material )->active) 
      flow->type = FLOW_MATERIAL;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_signal )->active) 
      flow->type = FLOW_SIGNAL;

  flow_update_data(flow);

  return NULL;
}

static void
fill_in_dialog(Flow *flow)
{
  FlowDialog *prop_dialog;
  char *str;
  GtkToggleButton *button=NULL;

  prop_dialog = properties_dialog;

  gtk_text_set_point( GTK_TEXT(prop_dialog->text), 0 ) ;
  gtk_text_forward_delete( GTK_TEXT(prop_dialog->text), 
			     gtk_text_get_length(GTK_TEXT(prop_dialog->text))) ;
  gtk_text_insert( GTK_TEXT(prop_dialog->text),
                   NULL, NULL, NULL,
                   text_get_string_copy(flow->text),
                   -1);

  switch (flow->type) {
  case FLOW_ENERGY:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_energy);
      break;
  case FLOW_MATERIAL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_material);
      break;
  case FLOW_SIGNAL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_signal);
      break;
  }
  if (button)
    gtk_toggle_button_set_active(button, TRUE);
}


static void
fill_in_defaults_dialog()
{
  FlowDialog *prop_dialog;
  char *str;
  GtkToggleButton *button=NULL;

  prop_dialog = defaults_dialog;

  if ( flow_default_label ) {
    gtk_text_set_point( GTK_TEXT(prop_dialog->text), 0 ) ;
    gtk_text_forward_delete( GTK_TEXT(prop_dialog->text), 
			     gtk_text_get_length(GTK_TEXT(prop_dialog->text))) ;
    gtk_text_insert( GTK_TEXT(prop_dialog->text),
		     NULL, NULL, NULL,
		     text_get_string_copy(flow_default_label),
		     -1) ;
  }

  switch (flow_most_recent_type) {
  case FLOW_ENERGY:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_energy);
      break;
  case FLOW_MATERIAL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_material);
      break;
  case FLOW_SIGNAL:
      button = GTK_TOGGLE_BUTTON(prop_dialog->m_signal);
      break;
  }
  if (button)
    gtk_toggle_button_set_active(button, TRUE);
}


static GtkWidget *
flow_get_properties(Flow *flow)
{
  FlowDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(FlowDialog, 1);
    properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Flow:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_text_new(NULL, NULL);
    prop_dialog->text = entry ;
    gtk_text_set_editable(GTK_TEXT(entry), TRUE);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    label = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    label = gtk_label_new(_("Flow type:"));
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    /* */
    prop_dialog->m_energy = gtk_radio_button_new_with_label (NULL, _("Energy"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_energy, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_energy);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop_dialog->m_energy), TRUE);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_energy));

    prop_dialog->m_material = gtk_radio_button_new_with_label(group, _("Material"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_material, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_material);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_material));

    prop_dialog->m_signal = gtk_radio_button_new_with_label(group, _("Signal"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_signal, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_signal);

#if 0
    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_signal));
#endif
  }
  
  fill_in_dialog(flow);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}

static void
flow_update_defaults( Flow* flow, char update_text )
{
  FlowDialog *prop_dialog = defaults_dialog ;

  flow_most_recent_type = flow->type ;

  if (update_text) {
    if ( flow_default_label )
      text_destroy( flow_default_label ) ;
    flow_default_label = text_copy( flow->text ) ;
  }
}

static ObjectChange *
flow_set_type_callback (Object* obj, Point* clicked, gpointer data)
{
  ((Flow*)obj)->type = (int) data ;
  flow_update_defaults( (Flow*) obj, 1 ) ;
  if ( defaults_dialog )
    fill_in_defaults_dialog() ;
  flow_update_data((Flow*)obj);

  return NULL;
}

static DiaMenuItem flow_menu_items[] = {
  { N_("Energy"), flow_set_type_callback, (void*)FLOW_ENERGY, 1 },
  { N_("Material"), flow_set_type_callback, (void*)FLOW_MATERIAL, 1 },
  { N_("Signal"), flow_set_type_callback, (void*)FLOW_SIGNAL, 1 },
};

static DiaMenu flow_menu = {
  "Flow",
  sizeof(flow_menu_items)/sizeof(DiaMenuItem),
  flow_menu_items,
  NULL
};

static DiaMenu *
flow_get_object_menu(Flow *flow, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  flow_menu_items[0].active = 1;
  return &flow_menu;
}

static GtkWidget *
flow_get_defaults()
{
  FlowDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;

  if (defaults_dialog == NULL) {

    prop_dialog = g_new(FlowDialog, 1);
    defaults_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;

    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Flow:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_text_new(NULL, NULL);
    prop_dialog->text = entry ;
    gtk_text_set_editable(GTK_TEXT(entry), TRUE);
    gtk_widget_set_usize( GTK_WIDGET(entry), 100, 50 ) ;
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    label = gtk_hseparator_new ();
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    label = gtk_label_new(_("Flow type:"));
    gtk_box_pack_start (GTK_BOX (dialog), label, FALSE, TRUE, 0);
    gtk_widget_show (label);

    /* */
    prop_dialog->m_energy = gtk_radio_button_new_with_label (NULL, _("Energy"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_energy, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_energy);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (prop_dialog->m_energy), TRUE);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_energy));

    prop_dialog->m_material = gtk_radio_button_new_with_label(group, _("Material"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_material, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_material);

    group = gtk_radio_button_group (GTK_RADIO_BUTTON (prop_dialog->m_material));

    prop_dialog->m_signal = gtk_radio_button_new_with_label(group, _("Signal"));
    gtk_box_pack_start (GTK_BOX (dialog), prop_dialog->m_signal, TRUE, TRUE, 0);
    gtk_widget_show (prop_dialog->m_signal);
  }
  
  fill_in_defaults_dialog();
  gtk_widget_show (defaults_dialog->dialog);

  return defaults_dialog->dialog;
}

static void
flow_apply_defaults()
{
  FlowDialog *prop_dialog;
  Color* color = 0 ;

  prop_dialog = defaults_dialog;

  if (GTK_TOGGLE_BUTTON(prop_dialog->m_energy)->active) {
    flow_most_recent_type = FLOW_ENERGY;
    color = &flow_color_energy ;
  } else if (GTK_TOGGLE_BUTTON( prop_dialog->m_material )->active) {
    flow_most_recent_type = FLOW_MATERIAL;
    color = &flow_color_material ;
  } else if (GTK_TOGGLE_BUTTON( prop_dialog->m_signal )->active) {
    flow_most_recent_type = FLOW_SIGNAL;
    color = &flow_color_signal ;
  }

  if ( ! flow_default_label ) {
    Point p ;

    if (flow_font == NULL)
      flow_font = font_getfont("Helvetica-Oblique");
    flow_default_label =
      new_text(
	       gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),0,-1), 
	       flow_font, FLOW_FONTHEIGHT, &p, color, ALIGN_CENTER
	       ) ;
  } else {
    text_set_string(flow_default_label,
		    gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),
					    0, -1));
    text_set_color( flow_default_label, color ) ;
  }

}

