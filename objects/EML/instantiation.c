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
#include "arrows.h"
#include "text.h"

#include "eml.h"

#include "pixmaps/instantiation.xpm"

typedef struct _Instantiation Instantiation;
typedef struct _InstantiationState InstantiationState;
typedef struct _InstantiationPropertiesDialog InstantiationPropertiesDialog;

typedef enum {
  INST_SINGLE,
  INST_MULTI
} InstantiationType;

struct _InstantiationState {
  ObjectState obj_state;

  InstantiationType type;
  gchar *procnum;
  gchar *reson; 
};
struct _Instantiation {
  OrthConn orth;

  Handle reson_handle;

  Point procnum_pos;
  Alignment procnum_align;
  real procnum_width;
  
  InstantiationType type;
  gchar *procnum;
  Text *reson;

  InstantiationPropertiesDialog* properties_dialog;
};

struct _InstantiationPropertiesDialog {
  GtkWidget *dialog;
  
  GtkToggleButton *type;
  GtkEntry *procnum;
  GtkEntry *reson;
};

#define INSTANTIATION_WIDTH 0.1
#define INSTANTIATION_TRIANGLESIZE 0.8
#define INSTANTIATION_ELLIPSE 0.6
#define INSTANTIATION_FONTHEIGHT 0.8
#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static Font *inst_font = NULL;

static real instantiation_distance_from(Instantiation *inst, Point *point);
static void instantiation_select(Instantiation *inst, Point *clicked_point,
			      Renderer *interactive_renderer);
static void instantiation_move_handle(Instantiation *inst, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void instantiation_move(Instantiation *inst, Point *to);
static void instantiation_draw(Instantiation *inst, Renderer *renderer);
static Object *instantiation_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void instantiation_destroy(Instantiation *inst);
static Object *instantiation_copy(Instantiation *inst);
static GtkWidget *instantiation_get_properties(Instantiation *inst);
static ObjectChange *instantiation_apply_properties(Instantiation *inst);
static DiaMenu *instantiation_get_object_menu(Instantiation *inst,
						Point *clickedpoint);

static InstantiationState *instantiation_get_state(Instantiation *inst);
static void instantiation_set_state(Instantiation *inst,
				     InstantiationState *state);

static void instantiation_save(Instantiation *inst, ObjectNode obj_node,
				const char *filename);
static Object *instantiation_load(ObjectNode obj_node, int version,
				   const char *filename);

static void instantiation_update_data(Instantiation *inst);

static ObjectTypeOps instantiation_type_ops =
{
  (CreateFunc) instantiation_create,
  (LoadFunc)   instantiation_load,
  (SaveFunc)   instantiation_save
};

ObjectType instantiation_type =
{
  "EML - Instantiation",   /* name */
  0,                      /* version */
  (char **) instantiation_xpm,  /* pixmap */
  
  &instantiation_type_ops       /* ops */
};

static ObjectOps instantiation_ops = {
  (DestroyFunc)         instantiation_destroy,
  (DrawFunc)            instantiation_draw,
  (DistanceFunc)        instantiation_distance_from,
  (SelectFunc)          instantiation_select,
  (CopyFunc)            instantiation_copy,
  (MoveFunc)            instantiation_move,
  (MoveHandleFunc)      instantiation_move_handle,
  (GetPropertiesFunc)   instantiation_get_properties,
  (ApplyPropertiesFunc) instantiation_apply_properties,
  (ObjectMenuFunc)      instantiation_get_object_menu
};

static real
instantiation_distance_from(Instantiation *inst, Point *point)
{
  OrthConn *orth = &inst->orth;
  real linedist;
  real textdist;

  linedist = orthconn_distance_from(orth, point, INSTANTIATION_WIDTH);

  textdist = text_distance_from( inst->reson, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
instantiation_select(Instantiation *inst, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  text_set_cursor(inst->reson, clicked_point, interactive_renderer);
  text_grab_focus(inst->reson, &inst->orth.object);

  orthconn_update_data(&inst->orth);
}

static void
instantiation_move_handle(Instantiation *inst, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(inst!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    inst->reson->position = *to;
  }
  else {
    Point along ;

    along = inst->reson->position ;
    point_sub( &along, &(orthconn_get_middle_handle(&inst->orth)->pos) ) ;

    orthconn_move_handle( &inst->orth, handle, to, reason );
    orthconn_update_data( &inst->orth ) ;

    inst->reson->position = orthconn_get_middle_handle(&inst->orth)->pos ;
    point_add( &inst->reson->position, &along ) ;
  }

  instantiation_update_data(inst);
  
}

static void
instantiation_move(Instantiation *inst, Point *to)
{
  Point *points = &inst->orth.points[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &points[0]);
  point_add(&inst->reson->position, &delta);

  orthconn_move( &inst->orth, to ) ;

  instantiation_update_data(inst);

  orthconn_move(&inst->orth, to);
  instantiation_update_data(inst);
}

static void
instantiation_draw(Instantiation *inst, Renderer *renderer)
{
  OrthConn *orth = &inst->orth;
  Point *points;
  int n;
  Point secpos;
  Point norm;
  Point *dest;

  points = &orth->points[0];
  n = orth->numpoints;

  dest = &points[1];
  point_copy(&norm, dest);
  point_sub(&norm, &points[0]);
  point_get_normed(&norm, &norm);

  renderer->ops->set_linewidth(renderer, INSTANTIATION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  point_copy(&norm, dest);
  point_sub(&norm, &points[0]);
  point_get_normed(&norm, &norm);

  if (inst->type == INST_MULTI) {
    arrow_draw(renderer, ARROW_FILLED_ELLIPSE,
               &points[0], dest,
               INSTANTIATION_ELLIPSE, INSTANTIATION_ELLIPSE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    point_copy_add_scaled(&secpos, &points[0], &norm, 0.6);
    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    point_copy_add_scaled(&secpos, &points[0], &norm, 1.1);
    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

  }
  else {
    point_copy_add_scaled(&secpos, &points[0], &norm, 0.5);

    arrow_draw(renderer, ARROW_LINES,
               &points[0], dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);

    arrow_draw(renderer, ARROW_LINES,
               &secpos, dest,
               INSTANTIATION_TRIANGLESIZE, INSTANTIATION_TRIANGLESIZE,
               INSTANTIATION_WIDTH,
               &color_black, &color_white);
  }
  renderer->ops->set_font(renderer, inst_font, INSTANTIATION_FONTHEIGHT);
  
  text_draw(inst->reson, renderer);
  
  if (inst->procnum != NULL) {
    renderer->ops->draw_string(renderer,
			       inst->procnum,
			       &inst->procnum_pos, inst->procnum_align,
			       &color_black);
  }
  
}

static void
instantiation_update_data(Instantiation *inst)
{
  OrthConn *orth = &inst->orth;
  Object *obj = &orth->object;
  int i, cur_segm;
  real dist;
  Point *points;
  Rectangle rect;

  inst->reson_handle.pos = inst->reson->position;

  orthconn_update_data(orth);
  obj->position = orth->points[0];

  
  orthconn_update_boundingbox(orth);
  /* fix boundinginstantiation for linewidth and triangle: */
  obj->bounding_box.top -= INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.left -= INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.bottom += INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  obj->bounding_box.right += INSTANTIATION_WIDTH/2.0 + INSTANTIATION_TRIANGLESIZE;
  
  points = inst->orth.points;
  dist = distance_point_point(&points[0], &points[1]);
  if (dist <= 0.8) {
    cur_segm = 1;
    i = 1;
  }
  else {
    i = 0;
    cur_segm = 0;
  }

  switch (inst->orth.orientation[cur_segm]) {
  case HORIZONTAL:
    inst->procnum_pos.y = points[i].y - font_descent(inst_font, INSTANTIATION_FONTHEIGHT);

    if (points[i].x <= points[i+1].x) {
      inst->procnum_align = ALIGN_LEFT;
      inst->procnum_pos.x = points[i].x + 1.0;
    }
    else {
      inst->procnum_align = ALIGN_RIGHT;
      inst->procnum_pos.x = points[i].x - 1.0;
    }
    break;
  case VERTICAL:
    inst->procnum_align = ALIGN_LEFT;
    inst->procnum_pos.x = points[i].x + 0.8;

    if (points[i].y <= points[i+1].y) {
      inst->procnum_pos.y = points[i].y + 0.8;
    }
    else {
      inst->procnum_pos.y = points[i].y - 0.8;
    }

    break;
  }

  if (inst->procnum_align == ALIGN_RIGHT) {
    rect.right = inst->procnum_pos.x;
    rect.left = rect.right - inst->procnum_width;
  }
  else {
    rect.left = inst->procnum_pos.x;
    rect.right = rect.left + inst->procnum_width;
  }

  rect.top = inst->procnum_pos.y - font_ascent(inst_font, INSTANTIATION_FONTHEIGHT);
  rect.bottom = rect.top + INSTANTIATION_FONTHEIGHT;
    
  rectangle_union(&obj->bounding_box, &rect);

  /* Add boundingbox for text: */
  text_calc_boundingbox(inst->reson, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);

}

static ObjectChange *
instantiation_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  instantiation_update_data((Instantiation *)obj);
  return change;
}

static ObjectChange *
instantiation_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  instantiation_update_data((Instantiation *)obj);
  return change;
}

static ObjectChange *
instantiation_set_type_callback(Object *obj, Point *clicked, gpointer data)
{
  Instantiation *inst;
  ObjectState *old_state;

  inst = (Instantiation *) obj;
  old_state = (ObjectState *)instantiation_get_state(inst);

  inst->type = (int) data ;
  instantiation_update_data(inst);

  return new_object_state_change((Object *)inst, old_state, 
                                 (GetStateFunc)instantiation_get_state,
                                 (SetStateFunc)instantiation_set_state);

}

static DiaMenuItem object_menu_items[] = {
  { N_("Single"), instantiation_set_type_callback, (gpointer) INST_SINGLE, 1 },
  { N_("Multiple"), instantiation_set_type_callback,(gpointer) INST_MULTI, 1 },
  { N_("Add segment"), instantiation_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), instantiation_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Instantiation",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
instantiation_get_object_menu(Instantiation *inst, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &inst->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[2].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[3].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}

static Object *
instantiation_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  Instantiation *inst;
  OrthConn *orth;
  Object *obj;
  Point p;

  if (inst_font == NULL) {
    inst_font = font_getfont("Courier");
  }
  
  inst = g_malloc0(sizeof(Instantiation));
  orth = &inst->orth;
  orthconn_init(orth, startpoint);

  obj = &orth->object;
  
  obj->type = &instantiation_type;

  obj->ops = &instantiation_ops;

  inst->type = INST_SINGLE;
  inst->procnum = NULL;
  inst->procnum_width = 0;
  inst->properties_dialog = NULL;

  /* Where to put the text */
  p = *startpoint ;
  p.y += 0.1 * INSTANTIATION_FONTHEIGHT ;

  inst->reson = new_text("", inst_font, INSTANTIATION_FONTHEIGHT, 
			      &p, &color_black, ALIGN_CENTER);

  inst->reson_handle.id = HANDLE_MOVE_TEXT;
  inst->reson_handle.type = HANDLE_MINOR_CONTROL;
  inst->reson_handle.connect_type = HANDLE_NONCONNECTABLE;
  inst->reson_handle.connected_to = NULL;
  object_add_handle( obj, &inst->reson_handle );

  instantiation_update_data(inst);
  
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[2];

  return &inst->orth.object;
}

static void
instantiation_destroy(Instantiation *inst)
{
  orthconn_destroy(&inst->orth);

  text_destroy( inst->reson ) ;
  g_free(inst->procnum);

  if (inst->properties_dialog != NULL) {
    gtk_widget_destroy(inst->properties_dialog->dialog);
    g_free(inst->properties_dialog);
  }
}

static Object *
instantiation_copy(Instantiation *inst)
{
  Instantiation *newinst;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &inst->orth;
  
  newinst = g_malloc0(sizeof(Instantiation));
  neworth = &newinst->orth;
  newobj = &neworth->object;

  orthconn_copy(orth, neworth);

  newinst->reson_handle.id           = inst->reson_handle.id;
  newinst->reson_handle.type         = inst->reson_handle.type;        
  newinst->reson_handle.connect_type = inst->reson_handle.connect_type;
  newinst->reson_handle.connected_to = NULL;

  newinst->reson = text_copy(inst->reson);
  newobj->handles[3] = &newinst->reson_handle;

  newinst->procnum = (inst->procnum != NULL)? strdup(inst->procnum):NULL;
  newinst->procnum_width = inst->procnum_width;
  newinst->type = inst->type;
  newinst->properties_dialog = NULL;
  
  instantiation_update_data(newinst);
  
  return (Object *)newinst;
}

static void
instantiation_state_free(ObjectState *ostate)
{
  InstantiationState *state = (InstantiationState *)ostate;
  g_free(state->reson);
}

static InstantiationState *
instantiation_get_state(Instantiation *inst)
{
  InstantiationState *state = g_new(InstantiationState, 1);

  state->obj_state.free = instantiation_state_free;

  state->procnum = g_strdup(inst->procnum);
  state->reson = text_get_string_copy( inst->reson );
  state->type = inst->type;

  return state;
}

static void
instantiation_set_state(Instantiation *inst, InstantiationState *state)
{
  g_free(inst->procnum);
  inst->procnum = state->procnum;
  text_set_string(inst->reson, state->reson);
  inst->type = state->type;
  
  inst->procnum_width = 0.0;
  if (inst->procnum != NULL) {
    inst->procnum_width =
      font_string_width(inst->procnum, inst_font, INSTANTIATION_FONTHEIGHT);
  }

  g_free(state);
  
  instantiation_update_data(inst);
}

static void
instantiation_save(Instantiation *inst, ObjectNode obj_node,
		    const char *filename)
{
  orthconn_save(&inst->orth, obj_node);

  data_add_string(new_attribute(obj_node, "procnum"),
		  inst->procnum);
  data_add_text(new_attribute(obj_node, "reson"),
                inst->reson);
  data_add_int(new_attribute(obj_node, "type"),
               inst->type);
}

static Object *
instantiation_load(ObjectNode obj_node, int version,
		    const char *filename)
{
  Instantiation *inst;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;

  if (inst_font == NULL) {
    inst_font = font_getfont("Courier");
  }

  inst = g_new0(Instantiation, 1);

  orth = &inst->orth;
  obj = &orth->object;

  obj->type = &instantiation_type;
  obj->ops = &instantiation_ops;

  orthconn_load(orth, obj_node);

  inst->procnum = NULL;
  attr = object_find_attribute(obj_node, "procnum");
  if (attr != NULL)
    inst->procnum = data_string(attribute_first_data(attr));
  
  inst->reson = NULL;
  attr = object_find_attribute(obj_node, "reson");
  if (attr != NULL)
    inst->reson = data_text(attribute_first_data(attr));

  inst->type = INST_SINGLE;
  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    inst->type = data_int(attribute_first_data(attr));

  inst->procnum_width = 0.0;

  inst->properties_dialog = NULL;
  
  if (inst->procnum != NULL) {
    inst->procnum_width =
      font_string_width(inst->procnum, inst_font, INSTANTIATION_FONTHEIGHT);
  }

  inst->reson_handle.id = HANDLE_MOVE_TEXT;
  inst->reson_handle.type = HANDLE_MINOR_CONTROL;
  inst->reson_handle.connect_type = HANDLE_NONCONNECTABLE;
  inst->reson_handle.connected_to = NULL;
  object_add_handle( obj, &inst->reson_handle );

  instantiation_update_data(inst);

  return (Object *)inst;
}

static ObjectChange *
instantiation_apply_properties(Instantiation *inst)
{
  InstantiationPropertiesDialog *prop_dialog;
  ObjectState *old_state;
  char *str;

  prop_dialog = inst->properties_dialog;

  old_state = (ObjectState *)instantiation_get_state(inst);

  /* Read from dialog and put in object: */
  if (inst->procnum != NULL)
    g_free(inst->procnum);
  str = gtk_entry_get_text(prop_dialog->procnum);
  if (strlen(str) != 0)
    inst->procnum = strdup(str);
  else
    inst->procnum = NULL;

  if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(prop_dialog->type)))
    inst->type = INST_MULTI;
  else
    inst->type = INST_SINGLE;

  inst->procnum_width = 0.0;

  if (inst->procnum != NULL) {
    inst->procnum_width =
      font_string_width(inst->procnum, inst_font, INSTANTIATION_FONTHEIGHT);
  }

  instantiation_update_data(inst);
  return new_object_state_change((Object *)inst, old_state, 
                                 (GetStateFunc)instantiation_get_state,
                                 (SetStateFunc)instantiation_set_state);
}

static void
fill_in_dialog(Instantiation *inst)
{
  InstantiationPropertiesDialog *prop_dialog;
  
  prop_dialog = inst->properties_dialog;

  if (inst->procnum != NULL)
    gtk_entry_set_text(prop_dialog->procnum, inst->procnum);
  else 
    gtk_entry_set_text(prop_dialog->procnum, "");
  
  if (inst->type == INST_MULTI)
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), TRUE);
  else
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), FALSE);

}

static GtkWidget *
instantiation_get_properties(Instantiation *inst)
{
  InstantiationPropertiesDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *cbutton;

  if (inst->properties_dialog == NULL) {

    prop_dialog = g_new(InstantiationPropertiesDialog, 1);
    inst->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(dialog));
    gtk_object_sink(GTK_OBJECT(dialog));
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);
    label = gtk_label_new(_("Number of processes:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->procnum = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    cbutton = gtk_check_button_new_with_label(_("multiple"));
    prop_dialog->type = GTK_TOGGLE_BUTTON(cbutton);
    gtk_box_pack_start (GTK_BOX (hbox), cbutton, TRUE, TRUE, 0);
    gtk_widget_show (cbutton);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

  }
  
  fill_in_dialog(inst);
  gtk_widget_show (inst->properties_dialog->dialog);

  return inst->properties_dialog->dialog;
}
