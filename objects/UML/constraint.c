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
#include "connection.h"
#include "render.h"
#include "sheet.h"
#include "handle.h"
#include "arrows.h"

#include "pixmaps/constraint.xpm"

typedef struct _Constraint Constraint;
typedef struct _ConstraintDialog ConstraintDialog;

struct _Constraint {
  Connection connection;

  Handle text_handle;

  char *text;
  Point text_pos;
  real text_width;

  ConstraintDialog *properties_dialog;
};

struct _ConstraintDialog {
  GtkWidget *dialog;
  
  GtkEntry *text;
};
  
#define CONSTRAINT_WIDTH 0.1
#define CONSTRAINT_DASHLEN 0.4
#define CONSTRAINT_FONTHEIGHT 0.8
#define CONSTRAINT_ARROWLEN 0.8
#define CONSTRAINT_ARROWWIDTH 0.5

#define HANDLE_MOVE_TEXT (HANDLE_CUSTOM1)


static Font *constraint_font = NULL;

static void constraint_move_handle(Constraint *constraint, Handle *handle,
				   Point *to, HandleMoveReason reason, ModifierKeys modifiers);
static void constraint_move(Constraint *constraint, Point *to);
static void constraint_select(Constraint *constraint, Point *clicked_point,
			      Renderer *interactive_renderer);
static void constraint_draw(Constraint *constraint, Renderer *renderer);
static Object *constraint_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real constraint_distance_from(Constraint *constraint, Point *point);
static void constraint_update_data(Constraint *constraint);
static void constraint_destroy(Constraint *constraint);
static Object *constraint_copy(Constraint *constraint);
static GtkWidget *constraint_get_properties(Constraint *constraint);
static void constraint_apply_properties(Constraint *constraint);
static void constraint_save(Constraint *constraint, ObjectNode obj_node,
			    const char *filename);
static Object *constraint_load(ObjectNode obj_node, int version,
			       const char *filename);


static ObjectTypeOps constraint_type_ops =
{
  (CreateFunc) constraint_create,
  (LoadFunc)   constraint_load,
  (SaveFunc)   constraint_save
};

ObjectType constraint_type =
{
  "UML - Constraint",        /* name */
  0,                         /* version */
  (char **) constraint_xpm,  /* pixmap */
  &constraint_type_ops       /* ops */
};

SheetObject constraint_sheetobj =
{
  "UML - Constraint",             /* type */
  N_("Constraint, place a constraint on something."),
                                /* description */
  (char **) constraint_xpm,     /* pixmap */
  NULL                          /* user_data */
};

static ObjectOps constraint_ops = {
  (DestroyFunc)         constraint_destroy,
  (DrawFunc)            constraint_draw,
  (DistanceFunc)        constraint_distance_from,
  (SelectFunc)          constraint_select,
  (CopyFunc)            constraint_copy,
  (MoveFunc)            constraint_move,
  (MoveHandleFunc)      constraint_move_handle,
  (GetPropertiesFunc)   constraint_get_properties,
  (ApplyPropertiesFunc) constraint_apply_properties,
  (ObjectMenuFunc)      NULL
};

static real
constraint_distance_from(Constraint *constraint, Point *point)
{
  Point *endpoints;
  real dist;
  
  endpoints = &constraint->connection.endpoints[0];
  
  dist = distance_line_point(&endpoints[0], &endpoints[1], CONSTRAINT_WIDTH, point);
  
  return dist;
}

static void
constraint_select(Constraint *constraint, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&constraint->connection);
}

static void
constraint_move_handle(Constraint *constraint, Handle *handle,
		 Point *to, HandleMoveReason reason, ModifierKeys modifiers)
{
  Point p1, p2;
  Point *endpoints;
  
  assert(constraint!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  if (handle->id == HANDLE_MOVE_TEXT) {
    constraint->text_pos = *to;
  } else  {
    endpoints = &constraint->connection.endpoints[0]; 
    p1.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p1.y = 0.5*(endpoints[0].y + endpoints[1].y);
    connection_move_handle(&constraint->connection, handle->id, to, reason);
    p2.x = 0.5*(endpoints[0].x + endpoints[1].x);
    p2.y = 0.5*(endpoints[0].y + endpoints[1].y);
    point_sub(&p2, &p1);
    point_add(&constraint->text_pos, &p2);
  }

  constraint_update_data(constraint);
}

static void
constraint_move(Constraint *constraint, Point *to)
{
  Point start_to_end;
  Point *endpoints = &constraint->connection.endpoints[0]; 
  Point delta;

  delta = *to;
  point_sub(&delta, &endpoints[0]);
 
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);

  point_add(&constraint->text_pos, &delta);
  
  constraint_update_data(constraint);
}

static void
constraint_draw(Constraint *constraint, Renderer *renderer)
{
  Point *endpoints;
  
  assert(constraint != NULL);
  assert(renderer != NULL);

  endpoints = &constraint->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, CONSTRAINT_WIDTH);
  renderer->ops->set_dashlength(renderer, CONSTRAINT_DASHLEN);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);
  renderer->ops->set_linecaps(renderer, LINECAPS_BUTT);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);
  
  arrow_draw(renderer, ARROW_LINES,
	     &endpoints[1], &endpoints[0],
	     CONSTRAINT_ARROWLEN, CONSTRAINT_ARROWWIDTH, CONSTRAINT_WIDTH,
	     &color_black, &color_white);

  renderer->ops->set_font(renderer, constraint_font,
			  CONSTRAINT_FONTHEIGHT);
  renderer->ops->draw_string(renderer,
			     constraint->text,
			     &constraint->text_pos, ALIGN_LEFT,
			     &color_black);
}

static Object *
constraint_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Constraint *constraint;
  Connection *conn;
  Object *obj;
  Point defaultlen = { 1.0, 1.0 };

  if (constraint_font == NULL)
    constraint_font = font_getfont("Courier");
  
  constraint = g_malloc(sizeof(Constraint));

  conn = &constraint->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[1] = *startpoint;
  point_add(&conn->endpoints[1], &defaultlen);
 
  obj = (Object *) constraint;
  
  obj->type = &constraint_type;
  obj->ops = &constraint_ops;
  
  connection_init(conn, 3, 0);

  constraint->text = strdup("{}");
  constraint->text_width =
      font_string_width(constraint->text, constraint_font, CONSTRAINT_FONTHEIGHT);
  constraint->text_width = 0.0;
  constraint->text_pos.x = 0.5*(conn->endpoints[0].x + conn->endpoints[1].x);
  constraint->text_pos.y = 0.5*(conn->endpoints[0].y + conn->endpoints[1].y);

  constraint->text_handle.id = HANDLE_MOVE_TEXT;
  constraint->text_handle.type = HANDLE_MINOR_CONTROL;
  constraint->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  constraint->text_handle.connected_to = NULL;
  obj->handles[2] = &constraint->text_handle;
  
  constraint->properties_dialog = NULL;
  
  constraint_update_data(constraint);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];
  return (Object *)constraint;
}


static void
constraint_destroy(Constraint *constraint)
{
  connection_destroy(&constraint->connection);

  g_free(constraint->text);

  if (constraint->properties_dialog != NULL) {
    gtk_widget_destroy(constraint->properties_dialog->dialog);
    g_free(constraint->properties_dialog);
  }
}

static Object *
constraint_copy(Constraint *constraint)
{
  Constraint *newconstraint;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &constraint->connection;
  
  newconstraint = g_malloc(sizeof(Constraint));
  newconn = &newconstraint->connection;
  newobj = (Object *) newconstraint;

  connection_copy(conn, newconn);

  newconstraint->text_handle = constraint->text_handle;
  newobj->handles[2] = &newconstraint->text_handle;

  newconstraint->text = strdup(constraint->text);
  newconstraint->text_pos = constraint->text_pos;
  newconstraint->text_width = constraint->text_width;

  newconstraint->properties_dialog = NULL;

  return (Object *)newconstraint;
}


static void
constraint_update_data(Constraint *constraint)
{
  Connection *conn = &constraint->connection;
  Object *obj = (Object *) constraint;
  Rectangle rect;
  
  obj->position = conn->endpoints[0];

  constraint->text_handle.pos = constraint->text_pos;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  /* Add boundingbox for text: */
  rect.left = constraint->text_pos.x;
  rect.right = rect.left + constraint->text_width;
  rect.top = constraint->text_pos.y - font_ascent(constraint_font, CONSTRAINT_FONTHEIGHT);
  rect.bottom = rect.top + CONSTRAINT_FONTHEIGHT;
  rectangle_union(&obj->bounding_box, &rect);

  /* fix boundingbox for constraint_width and arrow: */
  obj->bounding_box.top -= CONSTRAINT_WIDTH/2 + CONSTRAINT_ARROWLEN;
  obj->bounding_box.left -= CONSTRAINT_WIDTH/2 + CONSTRAINT_ARROWLEN;
  obj->bounding_box.bottom += CONSTRAINT_WIDTH/2 + CONSTRAINT_ARROWLEN;
  obj->bounding_box.right += CONSTRAINT_WIDTH/2 + CONSTRAINT_ARROWLEN;
}


static void
constraint_save(Constraint *constraint, ObjectNode obj_node,
		const char *filename)
{
  connection_save(&constraint->connection, obj_node);

  data_add_string(new_attribute(obj_node, "text"),
		  constraint->text);
  data_add_point(new_attribute(obj_node, "text_pos"),
		 &constraint->text_pos);
}

static Object *
constraint_load(ObjectNode obj_node, int version, const char *filename)
{
  Constraint *constraint;
  AttributeNode attr;
  Connection *conn;
  Object *obj;

  if (constraint_font == NULL)
    constraint_font = font_getfont("Courier");

  constraint = g_malloc(sizeof(Constraint));

  conn = &constraint->connection;
  obj = (Object *) constraint;

  obj->type = &constraint_type;
  obj->ops = &constraint_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 3, 0);

  constraint->text = NULL;
  attr = object_find_attribute(obj_node, "text");
  if (attr != NULL)
    constraint->text = data_string(attribute_first_data(attr));

  attr = object_find_attribute(obj_node, "text_pos");
  if (attr != NULL)
    data_point(attribute_first_data(attr), &constraint->text_pos);

  constraint->text_width =
      font_string_width(constraint->text, constraint_font, CONSTRAINT_FONTHEIGHT);

  constraint->text_handle.id = HANDLE_MOVE_TEXT;
  constraint->text_handle.type = HANDLE_MINOR_CONTROL;
  constraint->text_handle.connect_type = HANDLE_NONCONNECTABLE;
  constraint->text_handle.connected_to = NULL;
  obj->handles[2] = &constraint->text_handle;
  
  constraint->properties_dialog = NULL;
  
  constraint_update_data(constraint);
  
  return (Object *)constraint;
}


static void
constraint_apply_properties(Constraint *constraint)
{
  ConstraintDialog *prop_dialog;
  char *str;
  
  prop_dialog = constraint->properties_dialog;

  /* Read from dialog and put in object: */
  g_free(constraint->text);
  str = strdup(gtk_entry_get_text(prop_dialog->text));
  constraint->text = g_malloc(sizeof(char)*strlen(str)+2+1);
  strcpy(constraint->text, "{");
  strcat(constraint->text, str);
  strcat(constraint->text, "}");
    
  constraint->text_width =
      font_string_width(constraint->text, constraint_font,
			CONSTRAINT_FONTHEIGHT);
  
  constraint_update_data(constraint);
}

static void
fill_in_dialog(Constraint *constraint)
{
  ConstraintDialog *prop_dialog;
  char *str;
    
  prop_dialog = constraint->properties_dialog;

  str = strdup(constraint->text);
  strcpy(str, constraint->text+1);
  str[strlen(str)-1] = 0;
  gtk_entry_set_text(prop_dialog->text, str);
  g_free(str);
}

static GtkWidget *
constraint_get_properties(Constraint *constraint)
{
  ConstraintDialog *prop_dialog;
  GtkWidget *dialog;
  GtkWidget *entry;
  GtkWidget *hbox;
  GtkWidget *label;

  if (constraint->properties_dialog == NULL) {

    prop_dialog = g_new(ConstraintDialog, 1);
    constraint->properties_dialog = prop_dialog;
    
    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    hbox = gtk_hbox_new(FALSE, 5);

    label = gtk_label_new(_("Constraint:"));
    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, TRUE, 0);
    entry = gtk_entry_new();
    prop_dialog->text = GTK_ENTRY(entry);
    gtk_box_pack_start (GTK_BOX (hbox), entry, TRUE, TRUE, 0);
    gtk_widget_show (label);
    gtk_widget_show (entry);
    gtk_box_pack_start (GTK_BOX (dialog), hbox, TRUE, TRUE, 0);
    gtk_widget_show(hbox);
  }
  
  fill_in_dialog(constraint);
  gtk_widget_show (constraint->properties_dialog->dialog);

  return constraint->properties_dialog->dialog;
}
