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

#include "connectionpoint_ops.h"
#include "object_ops.h"
#include "color.h"

static Color connectionpoint_color = { 0.4, 0.4, 1.0 };

#define CP_SZ (CONNECTIONPOINT_SIZE/2)

void
connectionpoint_draw(ConnectionPoint *conpoint,
		     DDisplay *ddisp)
{
  int x,y;
  Point *point = &conpoint->pos;
  Renderer *renderer = &ddisp->renderer->renderer;
  
  ddisplay_transform_coords(ddisp, point->x, point->y, &x, &y);

  (renderer->ops->set_linewidth)(renderer, 0.0);
  (renderer->ops->set_linestyle)(renderer, LINESTYLE_SOLID);

  (renderer->interactive_ops->draw_pixel_line)(renderer,
					       x-CP_SZ,y-CP_SZ,
					       x+CP_SZ+1,y+CP_SZ+1,
					       &connectionpoint_color);

  (renderer->interactive_ops->draw_pixel_line)(renderer,
					       x+CP_SZ+1,y-CP_SZ-1,
					       x-CP_SZ,y+CP_SZ,
					       &connectionpoint_color);

}

void
connectionpoint_add_update(ConnectionPoint *conpoint,
			   Diagram *dia)
{
  diagram_add_update_pixels(dia, &conpoint->pos,
			    CONNECTIONPOINT_SIZE, CONNECTIONPOINT_SIZE);
}

/* run diagram_update_connections_object on all selected objects. */
void
diagram_update_connections_selection(Diagram *dia)
{
  GList *list = dia->selected;

  while (list!=NULL) {
    Object * selected_obj = (Object *) list->data;
    
    diagram_update_connections_object(dia, selected_obj);
    
    list = g_list_next(list);
  }
}

/* Updates all objects connected to the 'obj' object.
   Calls this function recursively for objects modified.
 */
void
diagram_update_connections_object(Diagram *dia, Object *obj)
{
  int i,j;
  ConnectionPoint *cp;
  GList *list;
  Object *connected_obj;
  Handle *handle;

  for (i=0;i<obj->num_connections;i++) {
    cp = obj->connections[i];
    if (distance_point_point_manhattan(&cp->pos, &cp->last_pos) > CHANGED_TRESHOLD){
      cp->last_pos = cp->pos;

      list = cp->connected;
      while (list!=NULL) {
	connected_obj = (Object *) list->data;

	handle = NULL;
	for (j=0;j<connected_obj->num_handles;j++) {
	  if (connected_obj->handles[j]->connected_to == cp)
	    handle = connected_obj->handles[j];
	}
	assert(handle!=NULL);
	
	object_add_updates(connected_obj, dia);
	connected_obj->ops->move_handle(connected_obj, handle ,
					&cp->pos, HANDLE_MOVE_CONNECTED);
	object_add_updates(connected_obj, dia);

	diagram_update_connections_object(dia, connected_obj);
	
	list = g_list_next(list);
      }
      
    }
    
  }
}

void
ddisplay_connect_selected(DDisplay *ddisp)
{
  GList *list;
  Object *selected_obj;
  int i;

  list = ddisp->diagram->selected;
    
  while (list!=NULL) {
    selected_obj = (Object *) list->data;
    
    for (i=0; i<selected_obj->num_handles; i++) {
      if (selected_obj->handles[i]->connect_type != HANDLE_NONCONNECTABLE) {
	object_connect_display(ddisp, selected_obj, selected_obj->handles[i]);
      }
    }
    
    list = g_list_next(list);
  }
}

void
diagram_unconnect_selected(Diagram *dia)
{
  GList *list;
  Object *selected_obj;
  Handle *handle;
  int i;

  list = dia->selected;
    
  while (list!=NULL) {
    selected_obj = (Object *) list->data;
    
    for (i=0; i<selected_obj->num_handles; i++) {
      handle = selected_obj->handles[i];
      
      if ((handle->connected_to != NULL) &&
	  (handle->connect_type == HANDLE_CONNECTABLE)){
	  /* don't do this if type is HANDLE_CONNECTABLE_BREAK */
	if (!diagram_is_selected(dia, handle->connected_to->object)) {
	  object_unconnect(selected_obj, handle);
	}
      }
    }

    list = g_list_next(list);
  }
}


