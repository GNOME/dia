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

#include "config.h"

#include <glib/gi18n-lib.h>

#include "connectionpoint_ops.h"
#include "object_ops.h"
#include "color.h"
#include "object.h"
#include "connectionpoint.h"
#include "diainteractiverenderer.h"

#define CONNECTIONPOINT_SIZE 7
#define CHANGED_TRESHOLD 0.001

static Color connectionpoint_color = { 0.4, 0.4, 1.0, 1.0 };

#define CP_SZ (CONNECTIONPOINT_SIZE/2)

static void
connectionpoint_draw (ConnectionPoint *conpoint,
                      DDisplay        *ddisp,
                      DiaRenderer     *renderer,
                      Color           *color)
{
  int x,y;
  Point *point = &conpoint->pos;

  ddisplay_transform_coords (ddisp, point->x, point->y, &x, &y);

  dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                            x - CP_SZ, y - CP_SZ,
                                            x + CP_SZ, y + CP_SZ,
                                            color);

  dia_interactive_renderer_draw_pixel_line (DIA_INTERACTIVE_RENDERER (renderer),
                                            x + CP_SZ, y - CP_SZ,
                                            x - CP_SZ, y + CP_SZ,
                                            color);
}

void
object_draw_connectionpoints (DiaObject *obj, DDisplay *ddisp)
{
  int i;
  static Color midpoint_color = { 1.0, 0.0, 0.0, 1.0 };
  DiaRenderer *renderer = ddisp->renderer;

  /* this does not change for any of the points */
  dia_renderer_set_linewidth (renderer, 0.0);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

  /* optimization to only draw the connection points at all if the size
   * of the object (bounding box) is bigger than the summed size of the
   * connection points - or some variation thereof ;)
   */
  if (dia_object_get_num_connections (obj) > 1) {
    const DiaRectangle *bbox = dia_object_get_bounding_box (obj);
    real w = ddisplay_transform_length (ddisp, bbox->right - bbox->left);
    real h = ddisplay_transform_length (ddisp, bbox->bottom - bbox->top);
    int n = dia_object_get_num_connections (obj);

    /* just comparing the sizes is still drawing more CPs than useful - try 50% */
    if (w * h < n * CONNECTIONPOINT_SIZE * CONNECTIONPOINT_SIZE * 2) {
      if (ddisp->mainpoint_magnetism) {
        return;
      }
      /* just draw the main point */
      for (i = 0; i < n; ++i) {
        if (obj->connections[i]->flags & CP_FLAG_ANYPLACE) {
          connectionpoint_draw (obj->connections[i], ddisp, renderer, &midpoint_color);
        }
      }
      return;
    }
  }

  for (i = 0; i < dia_object_get_num_connections (obj); i++) {
    if ((obj->connections[i]->flags & CP_FLAG_ANYPLACE) == 0) {
      connectionpoint_draw (obj->connections[i], ddisp, renderer, &connectionpoint_color);
    } else if (!ddisp->mainpoint_magnetism) {
      /* draw the "whole object"/center connpoints, but only when we don't
       * have snap-to-grid */
      connectionpoint_draw (obj->connections[i], ddisp, renderer, &midpoint_color);
    }
  }
}

void
connectionpoint_add_update (ConnectionPoint *conpoint,
                            Diagram         *dia)
{
  diagram_add_update_pixels (dia,
                             &conpoint->pos,
                             CONNECTIONPOINT_SIZE,
                             CONNECTIONPOINT_SIZE);
}

/* run diagram_update_connections_object on all selected objects. */
void
diagram_update_connections_selection(Diagram *dia)
{
  GList *list = dia->data->selected;

  while (list!=NULL) {
    DiaObject * selected_obj = (DiaObject *) list->data;

    diagram_update_connections_object(dia, selected_obj, TRUE);

    list = g_list_next(list);
  }
}

/* Updates all objects connected to the 'obj' object.
   Calls this function recursively for objects modified.

   If update_nonmoved is TRUE, also objects that have not
   moved since last time is updated. This is not propagated
   in the recursion.
 */
void
diagram_update_connections_object(Diagram *dia, DiaObject *obj,
				  int update_nonmoved)
{
  int i,j;
  GList *list;

  for (i=0;i<dia_object_get_num_connections(obj);i++) {
    ConnectionPoint *cp = obj->connections[i];
    if (cp->connected) {
      list = cp->connected;
      while (list!=NULL) {
	gboolean any_move = FALSE;
	DiaObject *connected_obj = (DiaObject *) list->data;

	object_add_updates(connected_obj, dia);
	for (j=0;j<connected_obj->num_handles;j++) {
	  if (connected_obj->handles[j]->connected_to == cp) {
	    Handle *handle = connected_obj->handles[j];
	    if (distance_point_point_manhattan(&cp->pos, &handle->pos) > CHANGED_TRESHOLD) {
	      connected_obj->ops->move_handle(connected_obj, handle, &cp->pos,
					      cp, HANDLE_MOVE_CONNECTED,0);
	      any_move = TRUE;
	    }
	  }
	}
	if (update_nonmoved || any_move) {
	  object_add_updates(connected_obj, dia);

	  diagram_update_connections_object(dia, connected_obj, FALSE);
	}
	list = g_list_next(list);
      }
    }
  }
  if (obj->children) {
    GList *child;
    for (child = obj->children; child != NULL; child = child->next) {
      DiaObject *child_obj = (DiaObject *)child->data;
      diagram_update_connections_object(dia, child_obj, update_nonmoved);
    }
  }
}

void
ddisplay_connect_selected(DDisplay *ddisp)
{
  GList *list;

  list = ddisp->diagram->data->selected;

  while (list!=NULL) {
    DiaObject *selected_obj = (DiaObject *) list->data;
    int i;

    for (i=0; i<selected_obj->num_handles; i++) {
      if (selected_obj->handles[i]->connect_type != HANDLE_NONCONNECTABLE) {
	object_connect_display(ddisp, selected_obj, selected_obj->handles[i], FALSE);
      }
    }

    list = g_list_next(list);
  }
}

void
diagram_unconnect_selected(Diagram *dia)
{
  GList *list;

  list = dia->data->selected;

  while (list!=NULL) {
    DiaObject *selected_obj = (DiaObject *) list->data;
    int i;

    for (i=0; i<selected_obj->num_handles; i++) {
      Handle *handle = selected_obj->handles[i];

      if ((handle->connected_to != NULL) &&
          (handle->connect_type == HANDLE_CONNECTABLE)){
          /* don't do this if type is HANDLE_CONNECTABLE_BREAK */
        if (!diagram_is_selected(dia, handle->connected_to->object)) {
          DiaChange *change = dia_unconnect_change_new (dia, selected_obj, handle);
          dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
        }
      }
    }

    list = g_list_next(list);
  }
}
