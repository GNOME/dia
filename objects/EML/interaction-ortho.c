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
#include "interaction.h"
#include "pixmaps/interaction-ortho.xpm"

typedef struct _InteractionOrtho InteractionOrtho;

struct _InteractionOrtho {
  OrthConn orth;

  Handle text_handle;
  
  InteractionType type;
  Text *text;

  InteractionDialog* properties_dialog;
};

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM2)

static Font *inter_font = NULL;

static real interaction_ortho_distance_from(InteractionOrtho*inter, Point *point);
static void interaction_ortho_select(InteractionOrtho*inter, Point *clicked_point,
			      Renderer *interactive_renderer);
static void interaction_ortho_move_handle(InteractionOrtho*inter, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void interaction_ortho_move(InteractionOrtho*inter, Point *to);
static void interaction_ortho_draw(InteractionOrtho*inter, Renderer *renderer);
static Object *interaction_ortho_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static void interaction_ortho_destroy(InteractionOrtho*inter);
static Object *interaction_ortho_copy(InteractionOrtho*inter);
static GtkWidget *interaction_ortho_get_properties(InteractionOrtho*inter);
static ObjectChange *interaction_ortho_apply_properties(InteractionOrtho*inter);
static DiaMenu *interaction_ortho_get_object_menu(InteractionOrtho*inter,
						Point *clickedpoint);

static InteractionState *interaction_ortho_get_state(InteractionOrtho*inter);
static void interaction_ortho_set_state(InteractionOrtho*inter,
				     InteractionState *state);

static void interaction_ortho_save(InteractionOrtho*inter, ObjectNode obj_node,
				const char *filename);
static Object *interaction_ortho_load(ObjectNode obj_node, int version,
				   const char *filename);

static void interaction_ortho_update_data(InteractionOrtho*inter);

static ObjectTypeOps interaction_ortho_type_ops =
{
  (CreateFunc) interaction_ortho_create,
  (LoadFunc)   interaction_ortho_load,
  (SaveFunc)   interaction_ortho_save
};

ObjectType interaction_ortho_type =
{
  "EML - Interaction orthorthogonal",   /* name */
  0,                      /* version */
  (char **) interaction_ortho_xpm,  /* pixmap */
  
  &interaction_ortho_type_ops       /* ops */
};

static ObjectOps interaction_ortho_ops = {
  (DestroyFunc)         interaction_ortho_destroy,
  (DrawFunc)            interaction_ortho_draw,
  (DistanceFunc)        interaction_ortho_distance_from,
  (SelectFunc)          interaction_ortho_select,
  (CopyFunc)            interaction_ortho_copy,
  (MoveFunc)            interaction_ortho_move,
  (MoveHandleFunc)      interaction_ortho_move_handle,
  (GetPropertiesFunc)   interaction_ortho_get_properties,
  (ApplyPropertiesFunc) interaction_ortho_apply_properties,
  (ObjectMenuFunc)      interaction_ortho_get_object_menu
};

static real
interaction_ortho_distance_from(InteractionOrtho*inter, Point *point)
{
  OrthConn *orth = &inter->orth;
  real linedist;
  real textdist;

  linedist = orthconn_distance_from(orth, point, INTERACTION_WIDTH);

  textdist = text_distance_from( inter->text, point ) ;
  
  return linedist > textdist ? textdist : linedist ;
}

static void
interaction_ortho_select(InteractionOrtho*inter, Point *clicked_point,
		  Renderer *interactive_renderer)
{
  text_set_cursor(inter->text, clicked_point, interactive_renderer);
  text_grab_focus(inter->text, &inter->orth.object);

  orthconn_update_data(&inter->orth);
}

static void
interaction_ortho_move_handle(InteractionOrtho*inter, Handle *handle,
		       Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  assert(inter!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    inter->text->position = *to;
  }
  else {
    Point along ;

    along = inter->text->position ;
    point_sub( &along, &(orthconn_get_middle_handle(&inter->orth)->pos) ) ;

    orthconn_move_handle( &inter->orth, handle, to, reason );
    orthconn_update_data( &inter->orth ) ;

    inter->text->position = orthconn_get_middle_handle(&inter->orth)->pos ;
    point_add( &inter->text->position, &along ) ;
  }

  interaction_ortho_update_data(inter);
  
}

static void
interaction_ortho_move(InteractionOrtho*inter, Point *to)
{
  Point *points = &inter->orth.points[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &points[0]);
  point_add(&inter->text->position, &delta);

  orthconn_move( &inter->orth, to ) ;

  interaction_ortho_update_data(inter);

  orthconn_move(&inter->orth, to);
  interaction_ortho_update_data(inter);
}

static void
interaction_ortho_draw(InteractionOrtho *inter, Renderer *renderer)
{
  OrthConn *orth = &inter->orth;
  Point *points;
  Point *point;
  int n;

  points = &orth->points[0];
  n = orth->numpoints;

  renderer->ops->set_linewidth(renderer, INTERACTION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_polyline(renderer, points, n, &color_black);

  if (inter->type == INTER_UNIDIR) {
      arrow_draw(renderer, ARROW_LINES,
	     &points[0], &points[1],
	     INTERACTION_ARROWLEN, INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
	     &color_black, &color_white);
  }
  else {
    point = fabs(points[n-1].y - points[n-2].y) <= 0.1
      ? &points[n-3] : &points[n-2];
    arrow_draw(renderer, ARROW_HALF_HEAD,
	     &points[n-1], point,
	     INTERACTION_ARROWLEN, INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
	     &color_black, &color_white);

    interaction_draw_buttom_halfhead(renderer, &points[0], &points[1],
                                     INTERACTION_ARROWLEN,
                                     INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
                                     &color_black);
  }
  renderer->ops->set_font(renderer, inter_font, INTERACTION_FONTHEIGHT);
  
  text_draw(inter->text, renderer);
  
  
}

static void
interaction_ortho_update_data(InteractionOrtho*inter)
{
  OrthConn *orth = &inter->orth;
  Object *obj = (Object *) inter;
  int num_segm, i;
  Point *points;
  Rectangle rect;

  inter->text_handle.pos = inter->text->position;

  orthconn_update_data(orth);
  obj->position = orth->points[0];

  
  orthconn_update_boundingbox(orth);
  /* fix boundinginteraction for linewidth and triangle: */
  obj->bounding_box.top -= INTERACTION_WIDTH/2.0 + INTERACTION_ARROWWIDTH;
  obj->bounding_box.left -= INTERACTION_WIDTH/2.0 + INTERACTION_ARROWWIDTH;
  obj->bounding_box.bottom += INTERACTION_WIDTH/2.0 + INTERACTION_ARROWWIDTH;
  obj->bounding_box.right += INTERACTION_WIDTH/2.0 + INTERACTION_ARROWWIDTH;
  
  /* Calc text pos: */
  num_segm = inter->orth.numpoints - 1;
  points = inter->orth.points;
  i = num_segm / 2;

  if ((num_segm % 2) == 0) { /* If no middle segment, use horizontal */
    if (inter->orth.orientation[i]==VERTICAL)
      i--;
  }

  /* Add boundingbox for text: */
  text_calc_boundingbox(inter->text, &rect) ;
  rectangle_union(&obj->bounding_box, &rect);

}

static ObjectChange *
interaction_ortho_add_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_add_segment((OrthConn *)obj, clicked);
  interaction_ortho_update_data((InteractionOrtho*)obj);
  return change;
}

static ObjectChange *
interaction_ortho_delete_segment_callback(Object *obj, Point *clicked, gpointer data)
{
  ObjectChange *change;
  change = orthconn_delete_segment((OrthConn *)obj, clicked);
  interaction_ortho_update_data((InteractionOrtho*)obj);
  return change;
}

static ObjectChange *
interaction_ortho_set_type_callback(Object *obj, Point *clicked, gpointer data)
{
  InteractionOrtho *inter;
  ObjectState *old_state;

  inter = (InteractionOrtho*) obj;
  old_state = (ObjectState *)interaction_ortho_get_state(inter);

  inter->type = (int) data ;
  interaction_ortho_update_data(inter);

  return new_object_state_change((Object *)inter, old_state, 
                                 (GetStateFunc)interaction_ortho_get_state,
                                 (SetStateFunc)interaction_ortho_set_state);

}

static DiaMenuItem object_menu_items[] = {
  { N_("Unidirectional"), interaction_ortho_set_type_callback, (gpointer) INTER_UNIDIR, 1 },
  { N_("Bidirectional"), interaction_ortho_set_type_callback,(gpointer) INTER_BIDIR, 1 },
  { N_("Add segment"), interaction_ortho_add_segment_callback, NULL, 1 },
  { N_("Delete segment"), interaction_ortho_delete_segment_callback, NULL, 1 },
};

static DiaMenu object_menu = {
  "Interaction",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
interaction_ortho_get_object_menu(InteractionOrtho*inter, Point *clickedpoint)
{
  OrthConn *orth;

  orth = &inter->orth;
  /* Set entries sensitive/selected etc here */
  object_menu_items[2].active = orthconn_can_add_segment(orth, clickedpoint);
  object_menu_items[3].active = orthconn_can_delete_segment(orth, clickedpoint);
  return &object_menu;
}

static Object *
interaction_ortho_create(Point *startpoint,
	       void *user_data,
  	       Handle **handle1,
	       Handle **handle2)
{
  InteractionOrtho*inter;
  OrthConn *orth;
  Object *obj;
  Point p;

  if (inter_font == NULL) {
    inter_font = font_getfont("Courier");
  }
  
  inter = g_malloc(sizeof(InteractionOrtho));
  orth = &inter->orth;
  obj = (Object *) inter;
  
  obj->type = &interaction_ortho_type;

  obj->ops = &interaction_ortho_ops;

  orthconn_init(orth, startpoint);

  inter->type = INTER_UNIDIR;
  inter->properties_dialog = NULL;

  /* Where to put the text */
  p = *startpoint ;
  p.y += 0.1 * INTERACTION_FONTHEIGHT ;

  inter->text = new_text("", inter_font, INTERACTION_FONTHEIGHT, 
			      &p, &color_black, ALIGN_CENTER);

  inter->text_handle.id = HANDLE_MOVE_TEXT;
  inter->text_handle.type = HANDLE_MINOR_CONTROL;
  inter->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  inter->text_handle.connected_to = NULL;
  object_add_handle( obj, &inter->text_handle );

  interaction_ortho_update_data(inter);
  
  *handle1 = obj->handles[0];
  *handle2 = obj->handles[2];

  return (Object *)inter;
}

static void
interaction_ortho_destroy(InteractionOrtho*inter)
{
  orthconn_destroy(&inter->orth);

  text_destroy( inter->text ) ;

  if (inter->properties_dialog != NULL) {
    gtk_widget_destroy(inter->properties_dialog->dialog);
    g_free(inter->properties_dialog);
  }
}

static Object *
interaction_ortho_copy(InteractionOrtho*inter)
{
  InteractionOrtho*newinter;
  OrthConn *orth, *neworth;
  Object *newobj;
  
  orth = &inter->orth;
  
  newinter = g_malloc(sizeof(InteractionOrtho));
  neworth = &newinter->orth;
  newobj = (Object *) newinter;

  orthconn_copy(orth, neworth);

  newinter->text_handle = inter->text_handle;
  newinter->text = text_copy(inter->text);
  newobj->handles[3] = &newinter->text_handle;

  newinter->type = inter->type;
  newinter->properties_dialog = NULL;
  
  interaction_ortho_update_data(newinter);
  
  return (Object *)newinter;
}

static void
interaction_ortho_state_free(ObjectState *ostate)
{
  InteractionState *state = (InteractionState *)ostate;
  g_free(state->text);
}

static InteractionState *
interaction_ortho_get_state(InteractionOrtho*inter)
{
  InteractionState *state = g_new(InteractionState, 1);

  state->obj_state.free = interaction_ortho_state_free;

  state->text = text_get_string_copy( inter->text );
  state->type = inter->type;

  return state;
}

static void
interaction_ortho_set_state(InteractionOrtho*inter, InteractionState *state)
{
  text_set_string(inter->text, state->text);
  inter->type = state->type;
  
  g_free(state);
  
  interaction_ortho_update_data(inter);
}

static void
interaction_ortho_save(InteractionOrtho*inter, ObjectNode obj_node,
		    const char *filename)
{
  orthconn_save(&inter->orth, obj_node);

  data_add_text(new_attribute(obj_node, "text"),
                inter->text);
  data_add_int(new_attribute(obj_node, "type"),
               inter->type);
}

static Object *
interaction_ortho_load(ObjectNode obj_node, int version,
		    const char *filename)
{
  InteractionOrtho*inter;
  AttributeNode attr;
  OrthConn *orth;
  Object *obj;

  if (inter_font == NULL) {
    inter_font = font_getfont("Courier");
  }

  inter = g_new(InteractionOrtho, 1);

  orth = &inter->orth;
  obj = (Object *) inter;

  obj->type = &interaction_ortho_type;
  obj->ops = &interaction_ortho_ops;

  orthconn_load(orth, obj_node);

  inter->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    inter->text = data_text(attribute_first_data(attr));

  inter->type = INTER_UNIDIR;
  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    inter->type = data_int(attribute_first_data(attr));

  inter->properties_dialog = NULL;
  
  inter->text_handle.id = HANDLE_MOVE_TEXT;
  inter->text_handle.type = HANDLE_MINOR_CONTROL;
  inter->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  inter->text_handle.connected_to = NULL;
  object_add_handle( obj, &inter->text_handle );

  interaction_ortho_update_data(inter);

  return (Object *)inter;
}

static ObjectChange *
interaction_ortho_apply_properties(InteractionOrtho*inter)
{
  InteractionDialog *prop_dialog;
  ObjectState *old_state;

  prop_dialog = inter->properties_dialog;

  old_state = (ObjectState *)interaction_ortho_get_state(inter);

  /* Read from dialog and put in object: */
  if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(prop_dialog->type)))
    inter->type = INTER_BIDIR;
  else
    inter->type = INTER_UNIDIR;

  interaction_ortho_update_data(inter);
  return new_object_state_change((Object *)inter, old_state, 
                                 (GetStateFunc)interaction_ortho_get_state,
                                 (SetStateFunc)interaction_ortho_set_state);
}

static void
fill_in_dialog(InteractionOrtho*inter)
{
  InteractionDialog *prop_dialog;
  
  prop_dialog = inter->properties_dialog;

  if (inter->type == INTER_BIDIR)
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), TRUE);
  else
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), FALSE);

}

static GtkWidget *
interaction_ortho_get_properties(InteractionOrtho*inter)
{
  InteractionDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *hbox;
  GtkWidget *cbutton;

  if (inter->properties_dialog == NULL) {

    prop_dialog = g_new(InteractionDialog, 1);
    inter->properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(dialog));
    gtk_object_sink(GTK_OBJECT(dialog));
    prop_dialog->dialog = dialog;

    hbox = gtk_hbox_new(FALSE, 5);
    cbutton = gtk_check_button_new_with_label(_("multiple"));
    prop_dialog->type = GTK_TOGGLE_BUTTON(cbutton);
    gtk_box_pack_start (GTK_BOX (hbox), cbutton, TRUE, TRUE, 0);
    gtk_widget_show (cbutton);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

  }
  
  fill_in_dialog(inter);
  gtk_widget_show (inter->properties_dialog->dialog);

  return inter->properties_dialog->dialog;
}
