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
#include "intl.h"

#include "pixmaps/interaction.xpm"

typedef struct _Interaction Interaction;

struct _Interaction {
  Connection connection;

  Handle text_handle;

  InteractionType type;
  char *text;
  Point text_pos;
  real text_width;

  InteractionDialog *properties_dialog;
};

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static DiaFont *interaction_font = NULL;

static ObjectChange* interaction_move_handle(Interaction *interaction, Handle *handle,
					     Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static ObjectChange* interaction_move(Interaction *interaction, Point *to);
static void interaction_select(Interaction *interaction, Point *clicked_point,
			      Renderer *interactive_renderer);
static void interaction_draw(Interaction *interaction, Renderer *renderer);
static DiaObject *interaction_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real interaction_distance_from(Interaction *interaction, Point *point);
static void interaction_update_data(Interaction *interaction);
static void interaction_destroy(Interaction *interaction);
static DiaObject *interaction_copy(Interaction *interaction);
static GtkWidget *interaction_get_properties(Interaction *interaction);
static ObjectChange *interaction_apply_properties(Interaction *interaction);
static DiaMenu *interaction_get_object_menu(Interaction *interaction,
						Point *clickedpoint);

static InteractionState *interaction_get_state(Interaction *interaction);
static void interaction_set_state(Interaction *interaction,
				 InteractionState *state);

static void interaction_save(Interaction *interaction, ObjectNode obj_node,
			    const char *filename);
static DiaObject *interaction_load(ObjectNode obj_node, int version,
			       const char *filename);

static ObjectTypeOps interaction_type_ops =
{
  (CreateFunc) interaction_create,
  (LoadFunc)   interaction_load,
  (SaveFunc)   interaction_save
};

ObjectType interaction_type =
{
  "EML - Interaction",        /* name */
  0,                         /* version */
  (char **) interaction_xpm,  /* pixmap */
  &interaction_type_ops       /* ops */
};

static ObjectOps interaction_ops = {
  (DestroyFunc)         interaction_destroy,
  (DrawFunc)            interaction_draw,
  (DistanceFunc)        interaction_distance_from,
  (SelectFunc)          interaction_select,
  (CopyFunc)            interaction_copy,
  (MoveFunc)            interaction_move,
  (MoveHandleFunc)      interaction_move_handle,
  (GetPropertiesFunc)   interaction_get_properties,
  (ApplyPropertiesFunc) interaction_apply_properties,
  (ObjectMenuFunc)      interaction_get_object_menu
};

static real
interaction_distance_from(Interaction *interaction, Point *point)
{
  Point *endpoints;
  real dist;
  
  endpoints = &interaction->connection.endpoints[0];
  
  dist = distance_line_point(&endpoints[0], &endpoints[1], INTERACTION_WIDTH, point);
  
  return dist;
}

static void
interaction_select(Interaction *interaction, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&interaction->connection);
}

static ObjectChange*
interaction_move_handle(Interaction *interaction, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  
  assert(interaction!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    interaction->text_pos = *to;
  } else  {
    endpoints = &interaction->connection.endpoints[0]; 
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&interaction->connection, handle->id, to, reason);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&interaction->text_pos, &p2);
  }

  interaction_update_data(interaction);

  return NULL;
}

static ObjectChange*
interaction_move(Interaction *interaction, Point *to)
{
  Point start_to_end;
  Point *endpoints = &interaction->connection.endpoints[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);
 
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&interaction->text_pos, &delta);
  
  interaction_update_data(interaction);

  return NULL;
}

static
void
calculate_arrow(Point *poly/*[3]*/, Point *to, Point *from,
		real length, real width)
{
  Point delta;
  Point orth_delta;
  real len;
  
  delta = *to;
  point_sub(&delta, from);
  len = point_len(&delta);
  if (len <= 0.0001) {
    delta.x=1.0;
    delta.y=0.0;
  } else {
    delta.x/=len;
    delta.y/=len;
  }

  orth_delta.x = delta.y;
  orth_delta.y = -delta.x;

  point_scale(&delta, length);
  point_scale(&orth_delta, width/2.0);

  poly[0] = *to;
  point_sub(&poly[0], &delta);
  point_sub(&poly[0], &orth_delta);
  poly[1] = *to;
  poly[2] = *to;
  point_sub(&poly[2], &delta);
  point_add(&poly[2], &orth_delta);
}

void
interaction_draw_buttom_halfhead(Renderer *renderer, Point *to, Point *from,
	   real length, real width, real linewidth,
	   Color *color)
{
  Point poly[3];

  calculate_arrow(poly, to, from, length, width);
  
  renderer->ops->set_linewidth(renderer, linewidth);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  if (poly[2].y < poly[0].y) 
      poly[2] = poly[0];
    
  renderer->ops->draw_line(renderer, &poly[2], &poly[1], color);
}

static void
interaction_draw(Interaction *interaction, Renderer *renderer)
{
  Point *endpoints;
  
  assert(interaction != NULL);
  assert(renderer != NULL);

  endpoints = &interaction->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, INTERACTION_WIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);
  renderer->ops->set_linejoin(renderer, LINEJOIN_MITER);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);

  if (interaction->type == INTER_UNIDIR)  
      arrow_draw(renderer, ARROW_LINES,
	     &endpoints[1], &endpoints[0],
	     INTERACTION_ARROWLEN, INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
	     &color_black, &color_white);
  else {

    arrow_draw(renderer, ARROW_HALF_HEAD,
	     &endpoints[1], &endpoints[0],
	     INTERACTION_ARROWLEN, INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
	     &color_black, &color_white);

    interaction_draw_buttom_halfhead(renderer, &endpoints[0], &endpoints[1],
                                     INTERACTION_ARROWLEN,
                                     INTERACTION_ARROWWIDTH, INTERACTION_WIDTH,
                                     &color_black);
  }

  renderer->ops->set_font(renderer, interaction_font,
			  INTERACTION_FONTHEIGHT);

  if (interaction->text != NULL)
      renderer->ops->draw_string(renderer,
                                 interaction->text,
                                 &interaction->text_pos, ALIGN_LEFT,
                                 &color_black);
}

static DiaObject *
interaction_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Interaction *interaction;
  Connection *conn;
  DiaObject *obj;
  Point defaultlen = { 1.0, 1.0 };

  if (interaction_font == NULL) {
	  /* choose default font name for your locale. see also font_data structure
	     in lib/font.c. if "Courier" works for you, it would be better.  */
	  interaction_font = font_getfont(_("Courier"));
  }
  
  interaction = g_malloc0(sizeof(Interaction));

  conn = &interaction->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (DiaObject *) interaction;
  
  obj->type = &interaction_type;
  obj->ops = &interaction_ops;
  
  connection_init(conn, 3, 0);

  interaction->type = INTER_UNIDIR;
  interaction->text = g_strdup("");
  interaction->text_width =
      font_string_width(interaction->text, interaction_font, INTERACTION_FONTHEIGHT);
  interaction->text_width = 0.0;
  interaction->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  interaction->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  interaction->text_handle.id = HANDLE_MOVE_TEXT;
  interaction->text_handle.type = HANDLE_MINOR_CONTROL;
  interaction->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  interaction->text_handle.connected_to = NULL;
  obj->handles[2] = &interaction->text_handle;
  
  interaction->properties_dialog = NULL;
  
  interaction_update_data(interaction);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (DiaObject *)interaction;
}


static void
interaction_destroy(Interaction *interaction)
{
  connection_destroy(&interaction->connection);

  g_free(interaction->text);

  if (interaction->properties_dialog != NULL) {
    gtk_widget_destroy(interaction->properties_dialog->dialog);
    g_free(interaction->properties_dialog);
  }
}

static DiaObject *
interaction_copy(Interaction *interaction)
{
  Interaction *newinteraction;
  Connection *conn, *newconn;
  DiaObject *newobj;
  
  conn = &interaction->connection;
  
  newinteraction = g_malloc0(sizeof(Interaction));
  newconn = &newinteraction->connection;
  newobj = (DiaObject *) newinteraction;

  connection_copy(conn, newconn);

  newinteraction->type = interaction->type;
  newinteraction->text_handle = interaction->text_handle;
  newobj->handles[2] = &newinteraction->text_handle;

  newinteraction->text = strdup(interaction->text);
  newinteraction->text_pos = interaction->text_pos;
  newinteraction->text_width = interaction->text_width;

  newinteraction->properties_dialog = NULL;

  return (DiaObject *)newinteraction;
}

static void
interaction_state_free(ObjectState *ostate)
{
  InteractionState *state = (InteractionState *)ostate;
  g_free(state->text);
}

static InteractionState *
interaction_get_state(Interaction *interaction)
{
  InteractionState *state = g_new(InteractionState, 1);

  state->obj_state.free = interaction_state_free;

  state->type = interaction->type;
  state->text = g_strdup(interaction->text);

  return state;
}

static void
interaction_set_state(Interaction *interaction, InteractionState *state)
{
  interaction->type = state->type;
  g_free(interaction->text);
  interaction->text = state->text;
  interaction->text_width = 0.0;
  if (interaction->text != NULL) {
    interaction->text_width =
      font_string_width(interaction->text, interaction_font, INTERACTION_FONTHEIGHT);
  } 
  
  g_free(state);
  
  interaction_update_data(interaction);
}


static void
interaction_update_data(Interaction *interaction)
{
  Connection *conn = &interaction->connection;
  DiaObject *obj = (Object *) interaction;
  Rectangle rect;
  
  obj->position = conn->endpoints[0];

  interaction->text_handle.pos = interaction->text_pos;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = interaction->text_pos.x;
  rect.right = rect.left + interaction->text_width;
  rect.top = interaction->text_pos.y - font_ascent(interaction_font, INTERACTION_FONTHEIGHT);
  rect.bottom = rect.top + INTERACTION_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);

  /* fix boundingbox for interaction_width and arrow: */
  obj->bounding_box.top -= INTERACTION_WIDTH/2 + INTERACTION_ARROWLEN;
  obj->bounding_box.left -= INTERACTION_WIDTH/2 + INTERACTION_ARROWLEN;
  obj->bounding_box.bottom += INTERACTION_WIDTH/2 + INTERACTION_ARROWLEN;
  obj->bounding_box.right += INTERACTION_WIDTH/2 + INTERACTION_ARROWLEN;
}

static
ObjectChange *
interaction_set_type_callback(DiaObject *obj, Point *clicked, gpointer data)
{
  Interaction *inter;
  ObjectState *old_state;

  inter = (Interaction*) obj;
  old_state = (ObjectState *)interaction_get_state(inter);

  inter->type = (int) data ;
  interaction_update_data(inter);

  return new_object_state_change((DiaObject *)inter, old_state, 
                                 (GetStateFunc)interaction_get_state,
                                 (SetStateFunc)interaction_set_state);

}

static DiaMenuItem object_menu_items[] = {
  { N_("Unidirectional"), interaction_set_type_callback, (gpointer) INTER_UNIDIR, 1 },
  { N_("Bidirectional"), interaction_set_type_callback,(gpointer) INTER_BIDIR, 1 },
};

static DiaMenu object_menu = {
  "Interaction",
  sizeof(object_menu_items)/sizeof(DiaMenuItem),
  object_menu_items,
  NULL
};

static DiaMenu *
interaction_get_object_menu(Interaction *interaction, Point *clickedpoint)
{
  return &object_menu;
}

static void
interaction_save(Interaction *interaction, ObjectNode obj_node,
		const char *filename)
{
  connection_save(&interaction->connection, obj_node);

  data_add_int(new_attribute(obj_node, "type"),
               interaction->type);
  data_add_string(new_attribute(obj_node, "text"),
		  interaction->text);
  data_add_point(new_attribute(obj_node, "text_pos"),
		 &interaction->text_pos);
}

static DiaObject *
interaction_load(ObjectNode obj_node, int version, const char *filename)
{
  Interaction *interaction;
  AttributeNode attr;
  Connection *conn;
  DiaObject *obj;

  if (interaction_font == NULL) {
	  /* choose default font name for your locale. see also font_data structure
	     in lib/font.c. if "Courier" works for you, it would be better.  */
	  interaction_font = font_getfont(_("Courier"));
  }

  interaction = g_malloc0(sizeof(Interaction));

  conn = &interaction->connection;
  obj = (DiaObject *) interaction;

  obj->type = &interaction_type;
  obj->ops = &interaction_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 3, 0);

  interaction->type = INTER_UNIDIR;
  attr = object_find_attribute(obj_node, "type");
  if (attr != NULL)
    interaction->type = data_int(attribute_first_data(attr));

  interaction->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    interaction->text = data_string(attribute_first_data(attr));
  else
    interaction->text = g_strdup("");

  attr = object_find_attribute(obj_node, "text_pos");
  if (attr != NULL)
    data_point(attribute_first_data(attr), &interaction->text_pos);

  interaction->text_width =
      font_string_width(interaction->text, interaction_font, INTERACTION_FONTHEIGHT);

  interaction->text_handle.id = HANDLE_MOVE_TEXT;
  interaction->text_handle.type = HANDLE_MINOR_CONTROL;
  interaction->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  interaction->text_handle.connected_to = NULL;
  obj->handles[2] = &interaction->text_handle;
  
  interaction->properties_dialog = NULL;
  
  interaction_update_data(interaction);
  
  return (DiaObject *)interaction;
}


static ObjectChange *
interaction_apply_properties(Interaction *interaction)
{
  InteractionDialog *prop_dialog;
  ObjectState *old_state;
  
  prop_dialog = interaction->properties_dialog;

  old_state = (ObjectState *)interaction_get_state(interaction);

  /* Read from dialog and put in object: */
  if (gtk_toggle_button_get_active( GTK_TOGGLE_BUTTON(prop_dialog->type)))
    interaction->type = INTER_BIDIR;
  else
    interaction->type = INTER_UNIDIR;

  g_free(interaction->text);
  interaction->text = strdup(gtk_entry_get_text(prop_dialog->text));
    
  interaction->text_width =
      font_string_width(interaction->text, interaction_font,
			INTERACTION_FONTHEIGHT);
  
  interaction_update_data(interaction);
  return new_object_state_change((DiaObject *)interaction, old_state, 
				 (GetStateFunc)interaction_get_state,
				 (SetStateFunc)interaction_set_state);

}

static void
fill_in_dialog(Interaction *interaction)
{
  InteractionDialog *prop_dialog;
    
  prop_dialog = interaction->properties_dialog;

  if (interaction->type == INTER_BIDIR)
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), TRUE);
  else
    gtk_toggle_button_set_active( GTK_TOGGLE_BUTTON(prop_dialog->type), FALSE);

  if (interaction->text != NULL)
    gtk_entry_set_text(prop_dialog->text, interaction->text);
}

static GtkWidget *
interaction_get_properties(Interaction *interaction)
{
  InteractionDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *cbutton;

  if (interaction->properties_dialog == NULL) {

    prop_dialog = g_new(InteractionDialog, 1);
    interaction->properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    gtk_object_ref(GTK_OBJECT(dialog));
    gtk_object_sink(GTK_OBJECT(dialog));
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Interaction name:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->text = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 5);
    cbutton = gtk_check_button_new_with_label(_("bidirectional"));
    prop_dialog->type = GTK_TOGGLE_BUTTON(cbutton);
    gtk_box_pack_start (GTK_BOX (hbox), cbutton, TRUE, TRUE, 0);
    gtk_widget_show (cbutton);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);

  }
  
  fill_in_dialog(interaction);
  gtk_widget_show (interaction->properties_dialog->dialog);

  return interaction->properties_dialog->dialog;
}
