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

/* If you have a problem with the Function Structure (FS) components,
 * please send e-mail to David Thompson <dcthomp@mail.utexas.edu>
 */

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "objchange.h"
#include "connection.h"
#include "render.h"
#include "handle.h"
#include "arrows.h"
#include "diamenu.h"
#include "text.h"
#include "orth_conn.h"

#include "pixmaps/orthflow.xpm"


typedef struct _Orthflow Orthflow;
typedef struct _OrthflowDialog OrthflowDialog;
typedef struct _OrthflowChange OrthflowChange;

typedef enum {
  ORTHFLOW_ENERGY,
  ORTHFLOW_MATERIAL,
  ORTHFLOW_SIGNAL
} OrthflowType;

struct _Orthflow {
  OrthConn orth ;

  Handle text_handle;

  Text* text;
  OrthflowType type;
};

struct _OrthflowDialog {
  GtkWidget *dialog;
  
  GtkWidget *text;

  GtkWidget *m_energy;
  GtkWidget *m_material;
  GtkWidget *m_signal;
};

enum OrthflowChangeType {
  TEXT_EDIT=1,
  FLOW_TYPE=2,
  BOTH=3
};

struct _OrthflowChange {
  ObjectChange			obj_change ;
  enum OrthflowChangeType	change_type ;
  OrthflowType			type ;
  char*				text ;
};

Color orthflow_color_energy   = { 1.0f, 0.0f, 0.0f };
Color orthflow_color_material = { 0.8f, 0.0f, 0.8f };
Color orthflow_color_signal   = { 0.0f, 0.0f, 1.0f };

  
#define ORTHFLOW_WIDTH 0.1
#define ORTHFLOW_MATERIAL_WIDTH 0.2
#define ORTHFLOW_DASHLEN 0.4
#define ORTHFLOW_FONTHEIGHT 0.6
#define ORTHFLOW_ARROWLEN 0.8
#define ORTHFLOW_ARROWWIDTH 0.5
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static Font *orthflow_font = NULL;

static OrthflowDialog *properties_dialog;
static OrthflowDialog *defaults_dialog;

/* Remember the most recently applied orthflow type and use it to
   set the type for any newly created orthflows
 */
static OrthflowType orthflow_most_recent_type = ORTHFLOW_ENERGY ;
static Text* orthflow_default_label = 0 ;

static void orthflow_move_handle(Orthflow *orthflow, Handle *handle,
				 Point *to, HandleMoveReason reason);
static void orthflow_move(Orthflow *orthflow, Point *to);
static void orthflow_select(Orthflow *orthflow, Point *clicked_point,
			    Renderer *interactive_renderer);
static void orthflow_draw(Orthflow *orthflow, Renderer *renderer);
static Object *orthflow_create(Point *startpoint,
			       void *user_data,
			       Handle **handle1,
			       Handle **handle2);
static real orthflow_distance_from(Orthflow *orthflow, Point *point);
static void orthflow_update_data(Orthflow *orthflow);
static void orthflow_destroy(Orthflow *orthflow);
static Object *orthflow_copy(Orthflow *orthflow);
static GtkWidget *orthflow_get_properties(Orthflow *orthflow);
static ObjectChange *orthflow_apply_properties(Orthflow *orthflow);
static GtkWidget *orthflow_get_defaults(void);
static void orthflow_apply_defaults(void);
static void orthflow_save(Orthflow *orthflow, ObjectNode obj_node,
			  const char *filename);
static Object *orthflow_load(ObjectNode obj_node, int version,
			     const char *filename);
static DiaMenu *orthflow_get_object_menu(Orthflow *orthflow, Point *clickedpoint) ;


static ObjectTypeOps orthflow_type_ops =
{
  (CreateFunc)		orthflow_create,
  (LoadFunc)		orthflow_load,
  (SaveFunc)		orthflow_save,
  (GetDefaultsFunc)	orthflow_get_defaults,
  (ApplyDefaultsFunc)	orthflow_apply_defaults
  
} ;

ObjectType orthflow_type =
{
  "FS - Orthflow",		/* name */
  0,				/* version */
  (char **) orthflow_xpm,	/* pixmap */
  &orthflow_type_ops		/* ops */
};

static ObjectOps orthflow_ops = {
  (DestroyFunc)         orthflow_destroy,
  (DrawFunc)            orthflow_draw,
  (DistanceFunc)        orthflow_distance_from,
  (SelectFunc)          orthflow_select,
  (CopyFunc)            orthflow_copy,
  (MoveFunc)            orthflow_move,
  (MoveHandleFunc)      orthflow_move_handle,
  (GetPropertiesFunc)   orthflow_get_properties,
  (ApplyPropertiesFunc) orthflow_apply_properties,
  (ObjectMenuFunc)      orthflow_get_object_menu,
};

static void
orthflow_change_apply_revert(ObjectChange* objchg, Object* obj)
{
  struct _OrthflowChange* change = (struct _OrthflowChange*) objchg ;
  Orthflow* oflow = (Orthflow*) obj ;

  if ( change->change_type == FLOW_TYPE || change->change_type == BOTH ) {
    OrthflowType type = oflow->type ;
    oflow->type = change->type ;
    change->type = type ;
    orthflow_update_data(oflow) ;
  }

  if ( change->change_type & TEXT_EDIT  || change->change_type == BOTH ) {
    char* tmp = text_get_string_copy( oflow->text ) ;
    text_set_string( oflow->text, change->text ) ;
    g_free( change->text ) ;
    change->text = tmp ;
  }
}

static void
orthflow_change_free(ObjectChange* objchg)
{
  struct _OrthflowChange* change = (struct _OrthflowChange*) objchg ;

  if (change->change_type & TEXT_EDIT  || change->change_type == BOTH ) {
    g_free(change->text) ;
  }
}

static ObjectChange*
orthflow_create_change( enum OrthflowChangeType change_type,
			OrthflowType type, Text* text )
{
  struct _OrthflowChange* change ;
  change = g_new( struct _OrthflowChange, 1 ) ;
  change->obj_change.apply = (ObjectChangeApplyFunc) orthflow_change_apply_revert ;
  change->obj_change.revert =  (ObjectChangeRevertFunc) orthflow_change_apply_revert ;
  change->obj_change.free =  (ObjectChangeFreeFunc) orthflow_change_free ;
  change->change_type = change_type ;

  change->type = type ;
  if ( text ) {
    change->text = text_get_string_copy( text ) ;
  }

  return (ObjectChange*) change ;
}

static real
orthflow_distance_from(Orthflow *orthflow, Point *point)
{
  real linedist;
  real textdist;

  linedist = orthconn_distance_from( &orthflow->orth, point, 
				     orthflow->type == ORTHFLOW_MATERIAL ? 
				     ORTHFLOW_MATERIAL_WIDTH : 
				     ORTHFLOW_WIDTH ) ;
  textdist = text_distance_from( orthflow->text, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
orthflow_select(Orthflow *orthflow, Point *clicked_point,
		Renderer *interactive_renderer)
{
  text_set_cursor(orthflow->text, clicked_point, interactive_renderer);
  text_grab_focus(orthflow->text, (Object *)orthflow);

  orthconn_update_data(&orthflow->orth);
}

static void
orthflow_move_handle(Orthflow *orthflow, Handle *handle,
		     Point *to, HandleMoveReason reason)
{
  assert(orthflow!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    orthflow->text->position = *to;
  } else {
    Point along ;

    along = orthflow->text->position ;
    point_sub( &along, &(orthconn_get_middle_handle(&orthflow->orth)->pos) ) ;

    orthconn_move_handle( &orthflow->orth, handle, to, reason );
    orthconn_update_data( &orthflow->orth ) ;

    orthflow->text->position = orthconn_get_middle_handle(&orthflow->orth)->pos ;
    point_add( &orthflow->text->position, &along ) ;
  }

  orthflow_update_data(orthflow);
}

static void
orthflow_move(Orthflow *orthflow, Point *to)
{
  Point *points = &orthflow->orth.points[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &points[0]);
  point_add(&orthflow->text->position, &delta);

  orthconn_move( &orthflow->orth, to ) ;

  orthflow_update_data(orthflow);
}

static void
orthflow_draw(Orthflow *orthflow, Renderer *renderer)
{
  ArrowType arrow_type;
  int n = orthflow->orth.numpoints ;
  Color* render_color = &orthflow_color_signal;
  Point *points;
  
  assert(orthflow != NULL);
  assert(renderer != NULL);

  arrow_type = ARROW_FILLED_TRIANGLE;
 
  points = &orthflow->orth.points[0];

  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  switch (orthflow->type) {
  case ORTHFLOW_SIGNAL:
    renderer->ops->set_linewidth(renderer, ORTHFLOW_WIDTH);
    renderer->ops->set_dashlength(renderer, ORTHFLOW_DASHLEN);
    renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
    render_color = &orthflow_color_signal ;
    break ;
  case ORTHFLOW_MATERIAL:
    renderer->ops->set_linewidth(renderer, ORTHFLOW_MATERIAL_WIDTH ) ;
    renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
    render_color = &orthflow_color_material ;
    break ;
  case ORTHFLOW_ENERGY:
    renderer->ops->set_linewidth(renderer, ORTHFLOW_WIDTH);
    renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
    render_color = &orthflow_color_energy ;
  }

  renderer->ops->draw_polyline(renderer, points, n,
			       render_color); 

  arrow_draw(renderer, arrow_type,
	     &points[n-1], &points[n-2],
	     ORTHFLOW_ARROWLEN, ORTHFLOW_ARROWWIDTH, ORTHFLOW_WIDTH,
	     render_color, &color_white);

  renderer->ops->set_font(renderer, orthflow_font,
			  ORTHFLOW_FONTHEIGHT);

  text_draw(orthflow->text, renderer);
}

static Object *
orthflow_create(Point *startpoint,
		void *user_data,
		Handle **handle1,
		Handle **handle2)
{
  Orthflow *orthflow;
  OrthConn *orth;
  Object *obj;
  Point p;

  orthflow = g_new(Orthflow,1);

  orth = &orthflow->orth ;
  orthconn_init( orth, startpoint ) ;
 
  obj = (Object *) orthflow;
  
  obj->type = &orthflow_type;
  obj->ops = &orthflow_ops;

  orthflow->type = orthflow_most_recent_type ;

  /* Where to put the text */
  p = *startpoint ;
  p.y += 0.1 * ORTHFLOW_FONTHEIGHT ;

  if ( orthflow_default_label ) {
    orthflow->text = text_copy( orthflow_default_label ) ;
    text_set_position( orthflow->text, &p ) ;
  } else {
    Color* color = &orthflow_color_signal;

    if (orthflow_font == NULL)
      orthflow_font = font_getfont("Helvetica-Oblique");

    switch (orthflow->type) {
    case ORTHFLOW_ENERGY:
      color = &orthflow_color_energy ;
      break ;
    case ORTHFLOW_MATERIAL:
      color = &orthflow_color_material ;
      break ;
    case ORTHFLOW_SIGNAL:
      color = &orthflow_color_signal ;
      break ;
    }

    orthflow->text = new_text("", orthflow_font, ORTHFLOW_FONTHEIGHT, 
			      &p, color, ALIGN_CENTER);
  }

  orthflow->text_handle.id = HANDLE_MOVE_TEXT;
  orthflow->text_handle.type = HANDLE_MINOR_CONTROL;
  orthflow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  orthflow->text_handle.connected_to = NULL;
  object_add_handle( obj, &orthflow->text_handle ) ;
  
  orthflow_update_data(orthflow);

  /*obj->handles[1] = orth->handles[2] ;*/
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[2];
  return (Object *)orthflow;
}


static void
orthflow_destroy(Orthflow *orthflow)
{
  orthconn_destroy( &orthflow->orth ) ;
  text_destroy( orthflow->text ) ;
}

static Object *
orthflow_copy(Orthflow *orthflow)
{
  Orthflow *neworthflow;
  OrthConn *orth, *neworth;
  Object *newobj;

  orth = &orthflow->orth;
  
  neworthflow = g_malloc(sizeof(Orthflow));
  neworth = &neworthflow->orth;
  newobj = (Object *) neworthflow;

  orthconn_copy(orth, neworth);

  neworthflow->text_handle = orthflow->text_handle;
  newobj->handles[2] = &neworthflow->text_handle;

  neworthflow->text = text_copy(orthflow->text);
  neworthflow->type = orthflow->type;

  return (Object *)neworthflow;
}


static void
orthflow_update_data(Orthflow *orthflow)
{
  OrthConn *orth = &orthflow->orth ;
  Object *obj = (Object *) orthflow;
  Rectangle rect;
  Color* color = &orthflow_color_signal;
  
  switch (orthflow->type) {
  case ORTHFLOW_ENERGY:
    color = &orthflow_color_energy ;
    break ;
  case ORTHFLOW_MATERIAL:
    color = &orthflow_color_material ;
    break ;
  case ORTHFLOW_SIGNAL:
    color = &orthflow_color_signal ;
    break ;
  }
  text_set_color( orthflow->text, color ) ;

  orthflow->text_handle.pos = orthflow->text->position;

  orthconn_update_data(orth);
  obj->position = orth->points[0];

  /* Boundingbox: */
  orthconn_update_boundingbox(orth);

  /* Add boundingbox for text: */
  text_calc_boundingbox(orthflow->text, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);

  /* fix boundingbox for orthflow_width and arrow: */
  obj->bounding_box.top -= ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
  obj->bounding_box.left -= ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
  obj->bounding_box.bottom += ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
  obj->bounding_box.right += ORTHFLOW_WIDTH/2 + ORTHFLOW_ARROWLEN;
}


static void
orthflow_save(Orthflow *orthflow, ObjectNode obj_node, const char *filename)
{
  orthconn_save(&orthflow->orth, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
		orthflow->text) ;
  data_add_int(new_attribute(obj_node, "type"),
	       orthflow->type);
}

static Object *
orthflow_load(ObjectNode obj_node, int version, const char *filename)
{
  Orthflow *orthflow;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;

  if (orthflow_font == NULL)
    orthflow_font = font_getfont("Helvetica-Oblique");

  orthflow = g_malloc(sizeof(Orthflow));

  orth = &orthflow->orth;
  obj = (Object *) orthflow;

  obj->type = &orthflow_type;
  obj->ops = &orthflow_ops;

  orthconn_load(orth, obj_node);

  orthflow->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    orthflow->text = data_text(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    orthflow->type = (OrthflowType)data_int(attribute_first_data(attr));

  orthflow->text_handle.id = HANDLE_MOVE_TEXT;
  orthflow->text_handle.type = HANDLE_MINOR_CONTROL;
  orthflow->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  orthflow->text_handle.connected_to = NULL;
  obj->handles[2] = &orthflow->text_handle;
  
  orthflow_update_data(orthflow);
  
  return (Object *)orthflow;
}


static ObjectChange *
orthflow_apply_properties(Orthflow *orthflow)
{
  OrthflowDialog *prop_dialog;
  ObjectChange* change ;
  
  prop_dialog = properties_dialog;

  change = orthflow_create_change( BOTH,
				   orthflow->type, orthflow->text ) ;
  text_set_string(orthflow->text,
                  gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),
					  0, -1));

  if (GTK_TOGGLE_BUTTON(prop_dialog->m_energy)->active)
    orthflow->type = ORTHFLOW_ENERGY;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_material )->active) 
    orthflow->type = ORTHFLOW_MATERIAL;
  else if (GTK_TOGGLE_BUTTON( prop_dialog->m_signal )->active) 
    orthflow->type = ORTHFLOW_SIGNAL;

  orthflow_update_data(orthflow);

  return change ;
}

static void
fill_in_dialog(Orthflow *orthflow)
{
  OrthflowDialog *prop_dialog;
  GtkToggleButton *button=NULL;

  prop_dialog = properties_dialog;

  gtk_text_set_point( GTK_TEXT(prop_dialog->text), 0 ) ;
  gtk_text_forward_delete( GTK_TEXT(prop_dialog->text), 
			   gtk_text_get_length(GTK_TEXT(prop_dialog->text))) ;
  gtk_text_insert( GTK_TEXT(prop_dialog->text),
                   NULL, NULL, NULL,
                   text_get_string_copy(orthflow->text),
                   -1);

  switch (orthflow->type) {
  case ORTHFLOW_ENERGY:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_energy);
    break;
  case ORTHFLOW_MATERIAL:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_material);
    break;
  case ORTHFLOW_SIGNAL:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_signal);
    break;
  }
  if (button)
    gtk_toggle_button_set_active(button, TRUE);
}


static void
fill_in_defaults_dialog()
{
  OrthflowDialog *prop_dialog;
  GtkToggleButton *button=NULL;

  prop_dialog = defaults_dialog;

  if ( orthflow_default_label ) {
    gtk_text_set_point( GTK_TEXT(prop_dialog->text), 0 ) ;
    gtk_text_forward_delete( GTK_TEXT(prop_dialog->text), 
			     gtk_text_get_length(GTK_TEXT(prop_dialog->text))) ;
    gtk_text_insert( GTK_TEXT(prop_dialog->text),
		     NULL, NULL, NULL,
		     text_get_string_copy(orthflow_default_label),
		     -1) ;
  }

  switch (orthflow_most_recent_type) {
  case ORTHFLOW_ENERGY:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_energy);
    break;
  case ORTHFLOW_MATERIAL:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_material);
    break;
  case ORTHFLOW_SIGNAL:
    button = GTK_TOGGLE_BUTTON(prop_dialog->m_signal);
    break;
  }
  if (button)
    gtk_toggle_button_set_active(button, TRUE);
}


static GtkWidget *
orthflow_get_properties(Orthflow *orthflow)
{
  OrthflowDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(OrthflowDialog, 1);
    properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(dialog));
    gtk_object_sink(GTK_OBJECT(dialog));
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
  
  fill_in_dialog(orthflow);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}

static void
orthflow_update_defaults( Orthflow* orthflow, char update_text )
{
  orthflow_most_recent_type = orthflow->type ;

  if (update_text) {
    if ( orthflow_default_label )
      text_destroy( orthflow_default_label ) ;
    orthflow_default_label = text_copy( orthflow->text ) ;
  }
}

static ObjectChange *
orthflow_set_type_callback (Object* obj, Point* clicked, gpointer data)
{
  ObjectChange* change ;
  change = orthflow_create_change( FLOW_TYPE, ((Orthflow*)obj)->type, 0 ) ;

  ((Orthflow*)obj)->type = (int) data ;
  orthflow_update_defaults( (Orthflow*) obj, 1 ) ;
  if ( defaults_dialog )
    fill_in_defaults_dialog() ;
  orthflow_update_data((Orthflow*)obj);

  return change;
}

static ObjectChange *
orthflow_segment_callback (Object* obj, Point* clicked, gpointer data)
{
  if ( (int)data )
     return orthconn_add_segment( (OrthConn*)obj, clicked ) ;

  return orthconn_delete_segment( (OrthConn*)obj, clicked ) ;
}

static DiaMenuItem orthflow_menu_items[] = {
  { N_("Energy"), orthflow_set_type_callback, (void*)ORTHFLOW_ENERGY, 1 },
  { N_("Material"), orthflow_set_type_callback, (void*)ORTHFLOW_MATERIAL, 1 },
  { N_("Signal"), orthflow_set_type_callback, (void*)ORTHFLOW_SIGNAL, 1 },
  { N_("Add segment"), orthflow_segment_callback, (void*)1, 1 },
  { N_("Delete segment"), orthflow_segment_callback, (void*)0, 1 }
};

static DiaMenu orthflow_menu = {
  "Orthflow",
  sizeof(orthflow_menu_items)/sizeof(DiaMenuItem),
  orthflow_menu_items,
  NULL
};

static DiaMenu *
orthflow_get_object_menu(Orthflow *orthflow, Point *clickedpoint)
{
  /* Set entries sensitive/selected etc here */
  return &orthflow_menu;
}

static GtkWidget *
orthflow_get_defaults(void)
{
  OrthflowDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GSList *group;

  if (defaults_dialog == NULL) {

    prop_dialog = g_new(OrthflowDialog, 1);
    defaults_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;

    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Orthflow:"));
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

    label = gtk_label_new(_("Orthflow type:"));
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
orthflow_apply_defaults(void)
{
  OrthflowDialog *prop_dialog;
  Color* color = 0 ;

  prop_dialog = defaults_dialog;

  if (GTK_TOGGLE_BUTTON(prop_dialog->m_energy)->active) {
    orthflow_most_recent_type = ORTHFLOW_ENERGY;
    color = &orthflow_color_energy ;
  } else if (GTK_TOGGLE_BUTTON( prop_dialog->m_material )->active) {
    orthflow_most_recent_type = ORTHFLOW_MATERIAL;
    color = &orthflow_color_material ;
  } else if (GTK_TOGGLE_BUTTON( prop_dialog->m_signal )->active) {
    orthflow_most_recent_type = ORTHFLOW_SIGNAL;
    color = &orthflow_color_signal ;
  }

  if ( ! orthflow_default_label ) {
    Point p ;

    if (orthflow_font == NULL)
      orthflow_font = font_getfont("Helvetica-Oblique");
    orthflow_default_label =
      new_text(
	       gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),0,-1), 
	       orthflow_font, ORTHFLOW_FONTHEIGHT, &p, color, ALIGN_CENTER
	       ) ;
  } else {
    text_set_string(orthflow_default_label,
		    gtk_editable_get_chars( GTK_EDITABLE(prop_dialog->text),
					    0, -1));
    text_set_color( orthflow_default_label, color ) ;
  }
}
