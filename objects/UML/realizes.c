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
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "sheet.h"
#include "arrows.h"

#include "uml.h"

#include "pixmaps/realizes.xpm"

typedef struct _Realizes Realizes;
typedef struct _RealizesState RealizesState;
typedef struct _RealizesPropertiesDialog RealizesPropertiesDialog;

struct _RealizesState {
  ObjectState obj_state;

  char *name;
  char *stereotype; 

};

struct _Realizes {
  OrthConn orth;

  Point text_pos;
  Alignment text_align;
  real text_width;
  
  char *name;
  char *stereotype; /* including << and >> */

  RealizesPropertiesDialog* properties_dialog;
};

struct _RealizesPropertiesDialog {
  GtkWidget *dialog;
  
  GtkEntry *name;
  GtkEntry *stereotype;
};

#define REALIZES_WIDTH 0.1
#define REALIZES_TRIANGLESIZE 0.8
#define REALIZES_DASHLEN 0.4
#define REALIZES_FONTHEIGHT 0.8

static Font *realize_font = NULL;

static real realizes_distance_from(Realizes *realize, Point *point);
static void realizes_select(Realizes *realize, Point *clicked_point,
			      Renderer *interactive_renderer);
static void realizes_move_handle(Realizes *realize, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void realizes_move(Realizes *realize, Point *to);
static void realizes_draw(Realizes *realize, Renderer *renderer);
static Object *realizes_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void realizes_destroy(Realizes *realize);
static Object *realizes_copy(Realizes *realize);
static GtkWidget *realizes_get_properties(Realizes *realize);
static ObjectChange *realizes_apply_properties(Realizes *realize);
static DiaMenu *realizes_get_object_menu(Realizes *realize,
					 Point *clickedpoint);

static RealizesState *realizes_get_state(Realizes *realize);
static void realizes_set_state(Realizes *realize,
			       RealizesState *state);

static void realizes_save(Realizes *realize, ObjectNode obj_node,
			  const char *filename);
static Object *realizes_load(ObjectNode obj_node, int version,
			     const char *filename);

static void realizes_update_data(Realizes *realize);

static ObjectTypeOps realizes_type_ops =
{
  (CreateFunc) realizes_create,
  (LoadFunc)   realizes_load,
  (SaveFunc)   realizes_save
};

ObjectType realizes_type =
{
  "UML - Realizes",   /* name */
  0,                      /* version */
  (char **) realizes_xpm,  /* pixmap */
  
  &realizes_type_ops       /* ops */
};

SheetObject realizes_sheetobj =
{
  "UML - Realizes",             /* type */
  N_("Realizes, implements a specific interface."),
                              /* description */
  (char **) realizes_xpm,     /* pixmap */

  NULL                        /* user_data */
};

static ObjectOps realizes_ops = {
  (DestroyFunc)         realizes_destroy,
  (DrawFunc)            realizes_draw,
  (DistanceFunc)        realizes_distance_from,
  (SelectFunc)          realizes_select,
  (CopyFunc)            realizes_copy,
  (MoveFunc)            realizes_move,
  (MoveHandleFunc)      realizes_move_handle,
  (GetPropertiesFunc)   realizes_get_properties,
  (ApplyPropertiesFunc) realizes_apply_properties,
  (ObjectMenuFunc)      realizes_get_object_menu
};

static real
realizes_distance_from(Realizes *realize, Point *point)
{
  OrthConn *orth = &realize->orth;
  return orthconn_distance_from(orth, point, REALIZES_WIDTH);
}

static void
realizes_select(Realizes *realize, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  orthconn_update_data(&realize->orth);
}

static void
realizes_move_handle(Realizes *realize, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(realize!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);
  
  orthconn_move_handle(&realize->orth, handle, to, reason);
  realizes_update_data(realize);
}

static void
realizes_move(Realizes *realize, Point *to)
{
  orthconn_move(&realize->orth, to);
  realizes_update_data(realize);
}

static void
realizes_draw(Realizes *realize, Renderer *renderer)
{
  OrthConn *orth = &realize->orth;
  Point *points;
  int n;
  Point pos;
  
  points = &orth->points[0];
  n = orth->numpoints;
  
  renderer->ops->set_linewidth(renderer, REALIZES_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_dashlength(renderer, REALIZES_DASHLEN);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  arrow_draw(renderer, ARROW_HOLLOW_TRIANGLE,
	     &points[0], &points[1],
	     REALIZES_TRIANGLESIZE, REALIZES_TRIANGLESIZE, REALIZES_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, realize_font, REALIZES_FONTHEIGHT);
  pos = realize->text_pos;
  
  if (realize->stereotype != NULL) {
    renderer->ops->draw_string(renderer,
			       realize->stereotype,
			       &pos, realize->text_align,
			       &color_black);

    pos.y += REALIZES_FONTHEIGHT;
  }
  
  if (realize->name != NULL) {
    renderer->ops->draw_string(renderer,
			       realize->name,
			       &pos, realize->text_align,
			       &color_black);
  }
  
}

static void
realizes_update_data(Realizes *realize)
{
  OrthConn *orth = &realize->orth;
  Object *obj = (Object *) realize;
  int num_segm, i;
  Point *points;
  Rectangle rect;
  
  orthconn_update_data(orth);
  
  orthconn_update_boundingbox(orth);
  /* fix boundingrealizes for linewidth and triangle: */
  obj->bounding_box.top -= REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  obj->bounding_box.left -= REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  obj->bounding_box.bottom += REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  obj->bounding_box.right += REALIZES_WIDTH/2.0 + REALIZES_TRIANGLESIZE;
  
  /* Calc text pos: */
  num_segm = realize->orth.numpoints - 1;
  points = realize->orth.points;
  i = num_segm / 2;
  
  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (realize->orth.orientation[i]==VERTICAL)
      i--;
  }

  switch (realize->orth.orientation[i]) {
  case HORIZONTAL:
    realize->text_align = ALIGN_CENTER;
    realize->text_pos.x = 0.5*(points[i].x+points[i+1].x);
    realize->text_pos.y = points[i].y - font_descent(realize_font, REALIZES_FONTHEIGHT);
    break;
  case VERTICAL:
    realize->text_align = ALIGN_LEFT;
    realize->text_pos.x = points[i].x + 0.1;
    realize->text_pos.y =
      0.5*(points[i].y+points[i+1].y) - font_descent(realize_font, REALIZES_FONTHEIGHT);
    break;
  }

  /* Add the text recangle to the bounding box: */
  rect.left = realize->text_pos.x;
  if (realize->text_align == ALIGN_CENTER)
    rect.left -= realize->text_width/2.0;
  rect.right = rect.left + realize->text_width;
  rect.top = realize->text_pos.y - font_ascent(realize_font, REALIZES_FONTHEIGHT);
  rect.bottom = rect.top + 2*REALIZES_FONTHEIGHT;

  rectangle_union(&obj->bounding_box, &rect);
}

static ObjectChange *
realizes_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  realizes_update_data((Realizes *)obj);
  return change;
}

static ObjectChange *
realizes_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  realizes_update_data((Realizes *)obj);
  return change;
}


static DiaMenuItem object_menu_items[] = {
  { N_("Add segment"), realizes_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), realizes_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Realizes",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
realizes_get_object_menu(Realizes *realize, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &realize->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[0].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[1].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}

static Object *
realizes_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Realizes *realize;
  OrthConn *orth;
  Object *obj;

  if (realize_font == NULL) {
    realize_font = font_getfont("Courier");
  }
  
  realize = g_malloc(sizeof(Realizes));
  orth = &realize->orth;
  obj = (Object *) realize;
  
  obj->type = &realizes_type;

  obj->ops = &realizes_ops;

  orthconn_init(orth, startpoint);
  
  realize->name = NULL;
  realize->stereotype = NULL;
  realize->text_width = 0;
  realize->properties_dialog = NULL;

  realizes_update_data(realize);
  
  *handle1 = orth->handles[0];
  *handle2 = orth->handles[orth->numpoints-2];
  return (Object *)realize;
}

static void
realizes_destroy(Realizes *realize)
{
  orthconn_destroy(&realize->orth);

  if (realize->properties_dialog != NULL) {
    gtk_widget_destroy(realize->properties_dialog->dialog);
    g_free(realize->properties_dialog);
  }
}

static Object *
realizes_copy(Realizes *realize)
{
  Realizes *newrealize;
  OrthConn *orth, *neworth;
  
  orth = &realize->orth;
  
  newrealize = g_malloc(sizeof(Realizes));
  neworth = &newrealize->orth;

  orthconn_copy(orth, neworth);

  newrealize->name = (realize->name != NULL)? strdup(realize->name):NULL;
  newrealize->stereotype = (realize->stereotype != NULL)? strdup(realize->stereotype):NULL;
  newrealize->text_width = realize->text_width;
  newrealize->properties_dialog = NULL;
  
  realizes_update_data(newrealize);
  
  return (Object *)newrealize;
}

static void
realizes_state_free(ObjectState *ostate)
{
  RealizesState *state = (RealizesState *)ostate;
  g_free(state->name);
  g_free(state->stereotype);
}

static RealizesState *
realizes_get_state(Realizes *realize)
{
  RealizesState *state = g_new(RealizesState, 1);

  state->obj_state.free = realizes_state_free;

  state->name = g_strdup(realize->name);
  state->stereotype = g_strdup(realize->stereotype);
  
  return state;
}

static void
realizes_set_state(Realizes *realize, RealizesState *state)
{
  g_free(realize->name);
  g_free(realize->stereotype);
  realize->name = state->name;
  realize->stereotype = state->stereotype;
  
  realize->text_width = 0.0;
  if (realize->name != NULL) {
    realize->text_width =
      font_string_width(realize->name, realize_font, REALIZES_FONTHEIGHT);
  }
  if (realize->stereotype != NULL) {
    realize->text_width = MAX(realize->text_width,
			  font_string_width(realize->stereotype, realize_font, REALIZES_FONTHEIGHT));
  }
  
  g_free(state);
  
  realizes_update_data(realize);
}

static void
realizes_save(Realizes *realize, ObjectNode obj_node, const char *filename)
{
  orthconn_save(&realize->orth, obj_node);

  data_add_string(new_attribute(obj_node, "name"),
		  realize->name);
  data_add_string(new_attribute(obj_node, "stereotype"),
		  realize->stereotype);
}

static Object *
realizes_load(ObjectNode obj_node, int version, const char *filename)
{
  Realizes *realize;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;

  if (realize_font == NULL) {
    realize_font = font_getfont("Courier");
  }

  realize = g_new(Realizes, 1);

  orth = &realize->orth;
  obj = (Object *) realize;

  obj->type = &realizes_type;
  obj->ops = &realizes_ops;

  orthconn_load(orth, obj_node);

  realize->name = NULL;
  attr = object_find_attribute(obj_node, "name");
  if (attr != NULL)
    realize->name = data_string(attribute_first_data(attr));
  
  realize->stereotype = NULL;
  attr = object_find_attribute(obj_node, "stereotype");
  if (attr != NULL)
    realize->stereotype = data_string(attribute_first_data(attr));
  
  realize->text_width = 0.0;

  realize->properties_dialog = NULL;
  
  if (realize->name != NULL) {
    realize->text_width =
      font_string_width(realize->name, realize_font, REALIZES_FONTHEIGHT);
  }
  if (realize->stereotype != NULL) {
    realize->text_width = MAX(realize->text_width,
			  font_string_width(realize->stereotype, realize_font, REALIZES_FONTHEIGHT));
  }

  realizes_update_data(realize);

  return (Object *)realize;
}

static ObjectChange *
realizes_apply_properties(Realizes *realize)
{
  RealizesPropertiesDialog *prop_dialog;
  ObjectState *old_state;
  char *str;

  prop_dialog = realize->properties_dialog;

  old_state = (ObjectState *)realizes_get_state(realize);

  /* Read from dialog and put in object: */
  if (realize->name != NULL)
    g_free(realize->name);
  str = gtk_entry_get_text(prop_dialog->name);
  if (strlen(str) != 0)
    realize->name = strdup(str);
  else
    realize->name = NULL;

  if (realize->stereotype != NULL)
    g_free(realize->stereotype);
  
  str = gtk_entry_get_text(prop_dialog->stereotype);
  
  if (strlen(str) != 0) {
    realize->stereotype = g_malloc(sizeof(char)*strlen(str)+2+1);
    realize->stereotype[0] = UML_STEREOTYPE_START;
    realize->stereotype[1] = 0;
    strcat(realize->stereotype, str);
    realize->stereotype[strlen(str)+1] = UML_STEREOTYPE_END;
    realize->stereotype[strlen(str)+2] = 0;
  } else {
    realize->stereotype = NULL;
  }

  realize->text_width = 0.0;

  if (realize->name != NULL) {
    realize->text_width =
      font_string_width(realize->name, realize_font, REALIZES_FONTHEIGHT);
  }
  if (realize->stereotype != NULL) {
    realize->text_width = MAX(realize->text_width,
			  font_string_width(realize->stereotype, realize_font, REALIZES_FONTHEIGHT));
  }
  
  realizes_update_data(realize);
  return new_object_state_change((Object *)realize, old_state, 
				 (GetStateFunc)realizes_get_state,
				 (SetStateFunc)realizes_set_state);
}

static void
fill_in_dialog(Realizes *realize)
{
  RealizesPropertiesDialog *prop_dialog;
  char *str;
  
  prop_dialog = realize->properties_dialog;

  if (realize->name != NULL)
    gtk_entry_set_text(prop_dialog->name, realize->name);
  else 
    gtk_entry_set_text(prop_dialog->name, "");
  
  
  if (realize->stereotype != NULL) {
    str = strdup(realize->stereotype);
    strcpy(str, realize->stereotype+1);
    str[strlen(str)-1] = 0;
    gtk_entry_set_text(prop_dialog->stereotype, str);
    g_free(str);
  } else {
    gtk_entry_set_text(prop_dialog->stereotype, "");
  }
}

static GtkWidget *
realizes_get_properties(Realizes *realize)
{
  RealizesPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (realize->properties_dialog == NULL) {

    prop_dialog = g_new(RealizesPropertiesDialog, 1);
    realize->properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Name:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->name = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    gtk_container_set_border_width (GTK_CONTAINER (hbox), 5);
    label = gtk_label_new(_("Stereotype:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->stereotype = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
  }
  fill_in_dialog(realize);
  gtk_widget_show (realize->properties_dialog->dialog);

  return realize->properties_dialog->dialog;
}




