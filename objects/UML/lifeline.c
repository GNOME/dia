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
#include <stdio.h>

#include "object.h"
#include "connection.h"
#include "render.h"
#include "sheet.h"
#include "handle.h"

#include "pixmaps/lifeline.xpm"

typedef struct _Lifeline Lifeline;
typedef struct _LifelineDialog LifelineDialog;

struct _Lifeline {
  Connection connection;  

  ConnectionPoint connections[6];

  Handle boxbot_handle;
  Handle boxtop_handle;

  real rtop, rbot;
    
  int draw_focuscontrol;
  int draw_cross;
};

struct _LifelineDialog {
  GtkWidget *dialog;
  
  GtkToggleButton *draw_focus;
  GtkToggleButton *draw_cross;
};

#define LIFELINE_LINEWIDTH 0.05
#define LIFELINE_BOXWIDTH 0.1
#define LIFELINE_WIDTH 1.0
#define LIFELINE_HEIGHT 3.0
#define LIFELINE_BOXMINHEIGHT 0.5
#define LIFELINE_DASHLEN 0.4
#define LIFELINE_CROSSWIDTH 0.12
#define LIFELINE_CROSSLEN 0.8

#define HANDLE_BOXTOP (HANDLE_CUSTOM1)
#define HANDLE_BOXBOT (HANDLE_CUSTOM2)


static LifelineDialog* properties_dialog;
static void lifeline_move_handle(Lifeline *lifeline, Handle *handle,
				   Point *to, HandleMoveReason reason);
static void lifeline_move(Lifeline *lifeline, Point *to);
static void lifeline_select(Lifeline *lifeline, Point *clicked_point,
			      Renderer *interactive_renderer);
static void lifeline_draw(Lifeline *lifeline, Renderer *renderer);
static Object *lifeline_create(Point *startpoint,
				 void *user_data,
				 Handle **handle1,
				 Handle **handle2);
static real lifeline_distance_from(Lifeline *lifeline, Point *point);
static void lifeline_update_data(Lifeline *lifeline);
static void lifeline_destroy(Lifeline *lifeline);
static Object *lifeline_copy(Lifeline *lifeline);
static void lifeline_save(Lifeline *lifeline, ObjectNode obj_node,
			  const char *filename);
static Object *lifeline_load(ObjectNode obj_node, int version,
			     const char *filename);
static void lifeline_apply_properties(Lifeline *lif);
static GtkWidget *lifeline_get_properties(Lifeline *lif);


static ObjectTypeOps lifeline_type_ops =
{
  (CreateFunc) lifeline_create,
  (LoadFunc)   lifeline_load,
  (SaveFunc)   lifeline_save
};

ObjectType lifeline_type =
{
  "UML - Lifeline",   /* name */
  0,                   /* version */
  (char **) lifeline_xpm,  /* pixmap */
  &lifeline_type_ops       /* ops */
};

SheetObject lifeline_sheetobj =
{
  "UML - Lifeline",             /* type */
  "Create a lifeline",
                                /* description */
  (char **) lifeline_xpm, /* pixmap */
  NULL                          /* user_data */
};

static ObjectOps lifeline_ops = {
  (DestroyFunc)         lifeline_destroy,
  (DrawFunc)            lifeline_draw,
  (DistanceFunc)        lifeline_distance_from,
  (SelectFunc)          lifeline_select,
  (CopyFunc)            lifeline_copy,
  (MoveFunc)            lifeline_move,
  (MoveHandleFunc)      lifeline_move_handle,
  (GetPropertiesFunc)   lifeline_get_properties,
  (ApplyPropertiesFunc) lifeline_apply_properties,
  (IsEmptyFunc)         object_return_false,
  (ObjectMenuFunc)      NULL
};

static real
lifeline_distance_from(Lifeline *lifeline, Point *point)
{
  Point *endpoints;
  real dist1, dist2;
  
  endpoints = &lifeline->connection.endpoints[0];
  dist1 = distance_line_point( &endpoints[0], &endpoints[1],
			      LIFELINE_WIDTH, point);
  dist2 = dist1;
  
  return MIN(dist1, dist2);
}

static void
lifeline_select(Lifeline *lifeline, Point *clicked_point,
	    Renderer *interactive_renderer)
{
  connection_update_handles(&lifeline->connection);
}

static void
lifeline_move_handle(Lifeline *lifeline, Handle *handle,
		 Point *to, HandleMoveReason reason)
{
  real s, t;
  Connection *conn;

  assert(lifeline!=NULL);
  assert(handle!=NULL);
  assert(to!=NULL);

  conn = &lifeline->connection;
  if (handle->id == HANDLE_BOXBOT) {
      t = to->y - conn->endpoints[0].y;
      if (t > LIFELINE_BOXMINHEIGHT && 
	  t < conn->endpoints[1].y - conn->endpoints[0].y) {
	  lifeline->rbot = t;
	  if (t < lifeline->rtop + LIFELINE_BOXMINHEIGHT)
	      lifeline->rtop = t - LIFELINE_BOXMINHEIGHT;
      }
  } else if (handle->id == HANDLE_BOXTOP) {
      t = to->y - conn->endpoints[0].y;
      if (t > 0 && 
	  t < conn->endpoints[1].y-conn->endpoints[0].y-LIFELINE_BOXMINHEIGHT) {
	  lifeline->rtop = t;	
	  if (t > lifeline->rbot - LIFELINE_BOXMINHEIGHT)
	      lifeline->rbot = t + LIFELINE_BOXMINHEIGHT;
      }
  } else {
    /* move horizontally only if startpoint is moved */
    if (handle->id==HANDLE_MOVE_STARTPOINT) {
	conn->endpoints[0].x = conn->endpoints[1].x = to->x;
    } else {
	to->x = conn->endpoints[0].x;
    }
    /* If connected don't change size */  
    t = (reason==HANDLE_MOVE_CONNECTED) ? 
	conn->endpoints[1].y - conn->endpoints[0].y:
	lifeline->rbot;
    connection_move_handle(conn, handle->id, to, reason);
    s = conn->endpoints[1].y - conn->endpoints[0].y;
    if (handle->id==HANDLE_MOVE_ENDPOINT && s < t && s > lifeline->rtop + LIFELINE_BOXMINHEIGHT)
	lifeline->rbot = s;
    else if (reason==HANDLE_MOVE_CONNECTED || s < t)
	conn->endpoints[1].y = conn->endpoints[0].y + t;
  }

  lifeline_update_data(lifeline);
}

static void
lifeline_move(Lifeline *lifeline, Point *to)
{
  Point start_to_end;
  Point delta;
  Point *endpoints = &lifeline->connection.endpoints[0]; 

  delta = *to;
  point_sub(&delta, &endpoints[0]);
  
  start_to_end = endpoints[1];
  point_sub(&start_to_end, &endpoints[0]);

  endpoints[1] = endpoints[0] = *to;
  point_add(&endpoints[1], &start_to_end);
  
  lifeline_update_data(lifeline);
}

static void
lifeline_draw(Lifeline *lifeline, Renderer *renderer)
{
  Point *endpoints, p1, p2;
  
  assert(lifeline != NULL);
  assert(renderer != NULL);

  endpoints = &lifeline->connection.endpoints[0];
  
  renderer->ops->set_linewidth(renderer, LIFELINE_LINEWIDTH);    
  renderer->ops->set_dashlength(renderer, LIFELINE_DASHLEN);
  renderer->ops->set_linestyle(renderer, LINESTYLE_DASHED);

  renderer->ops->draw_line(renderer,
			   &endpoints[0], &endpoints[1],
			   &color_black);


  renderer->ops->set_linewidth(renderer, LIFELINE_BOXWIDTH);
  renderer->ops->set_linestyle(renderer, LINESTYLE_SOLID);

  p1.x = endpoints[0].x - LIFELINE_WIDTH/2.0;
  p1.y = endpoints[0].y + lifeline->rtop;
  p2.x = endpoints[0].x + LIFELINE_WIDTH/2.0;
  p2.y = endpoints[0].y + lifeline->rbot;

  if (lifeline->draw_focuscontrol) {  
      renderer->ops->fill_rect(renderer, 
			       &p1, &p2,
			       &color_white);
  
      renderer->ops->draw_rect(renderer, 
			       &p1, &p2,
			       &color_black);

  }
    
  if (lifeline->draw_cross) {      
      renderer->ops->set_linewidth(renderer, LIFELINE_CROSSWIDTH);
      p1.x = endpoints[1].x + LIFELINE_CROSSLEN;
      p2.x = endpoints[1].x - LIFELINE_CROSSLEN;
      p1.y = endpoints[1].y + LIFELINE_CROSSLEN;
      p2.y = endpoints[1].y - LIFELINE_CROSSLEN;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      p1.y = p2.y;
      p2.y = endpoints[1].y + LIFELINE_CROSSLEN;
      renderer->ops->draw_line(renderer,
			       &p1, &p2,
			       &color_black);
      
  }
}

static Object *
lifeline_create(Point *startpoint,
		  void *user_data,
		  Handle **handle1,
		  Handle **handle2)
{
  Lifeline *lifeline;
  Connection *conn;
  Object *obj;
  int i;

  lifeline = g_malloc(sizeof(Lifeline));

  conn = &lifeline->connection;
  conn->endpoints[0] = *startpoint;
  conn->endpoints[0].x += LIFELINE_WIDTH/2;
  conn->endpoints[1] = conn->endpoints[0];
  conn->endpoints[1].y += LIFELINE_HEIGHT; 
 
  obj = (Object *) lifeline;
  
  obj->type = &lifeline_type;
  obj->ops = &lifeline_ops;

  connection_init(conn, 4, 6);

  lifeline->rtop = LIFELINE_HEIGHT/3;
  lifeline->rbot = lifeline->rtop+0.7;
  lifeline->draw_focuscontrol = 1;
  lifeline->draw_cross = 0;
    
  lifeline->boxbot_handle.id = HANDLE_BOXBOT;
  lifeline->boxbot_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxbot_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxbot_handle.connected_to = NULL;
  obj->handles[2] = &lifeline->boxbot_handle;
  
  lifeline->boxtop_handle.id = HANDLE_BOXTOP;
  lifeline->boxtop_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxtop_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxtop_handle.connected_to = NULL;
  obj->handles[3] = &lifeline->boxtop_handle;

  /* Only the start point should be connectable */
  obj->handles[1]->connect_type = HANDLE_NONCONNECTABLE;

  /* Connection points */
  for (i=0;i<6;i++) {
    obj->connections[i] = &lifeline->connections[i];
    lifeline->connections[i].object = obj;
    lifeline->connections[i].connected = NULL;
  }

  lifeline_update_data(lifeline);

  *handle1 = obj->handles[0];
  *handle2 = obj->handles[1];

  return (Object *)lifeline;
}


static void
lifeline_destroy(Lifeline *lifeline)
{
  connection_destroy(&lifeline->connection);
}

static Object *
lifeline_copy(Lifeline *lifeline)
{
  Lifeline *newlifeline;
  Connection *conn, *newconn;
  Object *newobj;
  
  conn = &lifeline->connection;
  
  newlifeline = g_malloc(sizeof(Lifeline));
  newconn = &newlifeline->connection;
  newobj = (Object *) newlifeline;

  connection_copy(conn, newconn);

  newlifeline->boxbot_handle = lifeline->boxbot_handle;
  newobj->handles[2] = &newlifeline->boxbot_handle;
  newlifeline->boxtop_handle = lifeline->boxtop_handle;
  newobj->handles[3] = &newlifeline->boxtop_handle;

  newlifeline->rtop = lifeline->rtop;
  newlifeline->rbot = lifeline->rbot;

   newlifeline->draw_focuscontrol = lifeline->draw_focuscontrol;
   newlifeline->draw_cross = lifeline->draw_cross;

  return (Object *)newlifeline;
}


static void
lifeline_update_data(Lifeline *lifeline)
{
  Connection *conn = &lifeline->connection;
  Object *obj = (Object *) lifeline;
  Point p1, p2;
  real r;
  int i;

  obj->position = conn->endpoints[0];

  /* box handles: */
  p1.x = conn->endpoints[0].x;
  p1.y = conn->endpoints[0].y + lifeline->rtop;
  lifeline->boxtop_handle.pos = p1;
  p2.x = p1.x;
  p2.y = conn->endpoints[0].y + lifeline->rbot;
  lifeline->boxbot_handle.pos = p2;

  connection_update_handles(conn);

  /* Boundingbox: */
  connection_update_boundingbox(conn);

  if (lifeline->draw_focuscontrol) {  
      /* fix boundingbox for lifeline_width: */
      obj->bounding_box.top -= LIFELINE_LINEWIDTH/2;
      obj->bounding_box.left -= LIFELINE_WIDTH;
      obj->bounding_box.bottom += LIFELINE_LINEWIDTH/2;
      obj->bounding_box.right += LIFELINE_WIDTH;

      p1.x -= LIFELINE_WIDTH/2.0;
      p2.x += LIFELINE_WIDTH/2.0; 
      /* Update connections: */      
      lifeline->connections[0].pos = p1;
      lifeline->connections[1].pos.x = p2.x;
      lifeline->connections[1].pos.y = p1.y;
      lifeline->connections[2].pos.x = p2.x;
      lifeline->connections[2].pos.y = (p1.y + p2.y)/2.0;
      lifeline->connections[3].pos.x = p2.x;
      lifeline->connections[3].pos.y = p2.y;
      lifeline->connections[4].pos.x = p1.x;
      lifeline->connections[4].pos.y = (p1.y + p2.y)/2.0;
      lifeline->connections[5].pos.x = p1.x;
      lifeline->connections[5].pos.y = p2.y;
  } else {     
      /* without focus of control, the points are over the line */
      r = (p2.y - p1.y)/5.0; 
      for (i = 0; i < 6; i++) {
	  lifeline->connections[i].pos.x = p1.x;
	  lifeline->connections[i].pos.y = p1.y + i*r;
      }
  }

  if (lifeline->draw_cross) {
      obj->bounding_box.bottom += LIFELINE_CROSSLEN;
      obj->bounding_box.left -= LIFELINE_CROSSLEN;
      obj->bounding_box.right += LIFELINE_CROSSLEN;
  }
}


static void
lifeline_save(Lifeline *lifeline, ObjectNode obj_node,
	      const char *filename)
{
  connection_save(&lifeline->connection, obj_node);

  data_add_real(new_attribute(obj_node, "rtop"),
		lifeline->rtop);
  data_add_real(new_attribute(obj_node, "rbot"),
		lifeline->rbot);
  data_add_boolean(new_attribute(obj_node, "draw_focus"),
		   lifeline->draw_focuscontrol);
  data_add_boolean(new_attribute(obj_node, "draw_cross"),
		   lifeline->draw_cross);
}

static Object *
lifeline_load(ObjectNode obj_node, int version, const char *filename)
{
  Lifeline *lifeline;
  AttributeNode attr;
  Connection *conn;
  Object *obj;
  int i;

  lifeline = g_malloc(sizeof(Lifeline));

  conn = &lifeline->connection;
  obj = (Object *) lifeline;

  obj->type = &lifeline_type;
  obj->ops = &lifeline_ops;

  connection_load(conn, obj_node);
  
  connection_init(conn, 4, 6);

  attr = object_find_attribute(obj_node, "rtop");
  if (attr != NULL)
    lifeline->rtop = data_real(attribute_first_data(attr));
  else
    lifeline->rtop = LIFELINE_HEIGHT/3;

  attr = object_find_attribute(obj_node, "rbot");
  if (attr != NULL)
    lifeline->rbot = data_real(attribute_first_data(attr));
  else
    lifeline->rbot = lifeline->rtop+0.7;

  attr = object_find_attribute(obj_node, "draw_focus");
  if (attr != NULL)
    lifeline->draw_focuscontrol = data_boolean(attribute_first_data(attr));
  else
    lifeline->draw_focuscontrol = 1;

  attr = object_find_attribute(obj_node, "draw_cross");
  if (attr != NULL)
    lifeline->draw_cross = data_boolean(attribute_first_data(attr));
  else
    lifeline->draw_cross = 0;

  /* Connection points */
  for (i=0;i<6;i++) {
    obj->connections[i] = &lifeline->connections[i];
    lifeline->connections[i].object = obj;
    lifeline->connections[i].connected = NULL;
  }

  lifeline->boxbot_handle.id = HANDLE_BOXBOT;
  lifeline->boxbot_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxbot_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxbot_handle.connected_to = NULL;
  obj->handles[2] = &lifeline->boxbot_handle;
  
  lifeline->boxtop_handle.id = HANDLE_BOXTOP;
  lifeline->boxtop_handle.type = HANDLE_MINOR_CONTROL;
  lifeline->boxtop_handle.connect_type = HANDLE_NONCONNECTABLE;
  lifeline->boxtop_handle.connected_to = NULL;
  obj->handles[3] = &lifeline->boxtop_handle;
  
  lifeline_update_data(lifeline);
  
  return (Object *)lifeline;
}


static void
lifeline_apply_properties(Lifeline *lif)
{
  LifelineDialog *prop_dialog;

  prop_dialog = properties_dialog;

  /* Read from dialog and put in object: */

  lif->draw_focuscontrol = prop_dialog->draw_focus->active;
  lif->draw_cross = prop_dialog->draw_cross->active;
  
  lifeline_update_data(lif);
}

static void
fill_in_dialog(Lifeline *lif)
{
  LifelineDialog *prop_dialog;

  prop_dialog = properties_dialog;

  gtk_toggle_button_set_active(prop_dialog->draw_focus, lif->draw_focuscontrol);
  gtk_toggle_button_set_active(prop_dialog->draw_cross, lif->draw_cross);
}

static GtkWidget *
lifeline_get_properties(Lifeline *lif)
{
  LifelineDialog *prop_dialog;
  
  GtkWidget *dialog;
  GtkWidget *checkbox;

  if (properties_dialog == NULL) {

    prop_dialog = g_new(LifelineDialog, 1);
    properties_dialog = prop_dialog;

    dialog = gtk_vbox_new(FALSE, 0);
    prop_dialog->dialog = dialog;
    
    checkbox = gtk_check_button_new_with_label("Show focus of control:");
    prop_dialog->draw_focus = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_box_pack_start (GTK_BOX (dialog), checkbox, TRUE, TRUE, 0);

    checkbox = gtk_check_button_new_with_label("Show destruction mark:");
    prop_dialog->draw_cross = GTK_TOGGLE_BUTTON( checkbox );
    gtk_widget_show(checkbox);
    gtk_box_pack_start (GTK_BOX (dialog), checkbox, TRUE, TRUE, 0);

  }
  
  fill_in_dialog(lif);
  gtk_widget_show (properties_dialog->dialog);

  return properties_dialog->dialog;
}
