/* xxxxxx -- an diagram creation/manipulation program
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
#include <stdio.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "display.h"
#include "tool.h"
#include "interface.h"
#include "focus.h"
#include "object_ops.h"

gint
ddisplay_canvas_events (GtkWidget *canvas,
			GdkEvent  *event,
			DDisplay *ddisp)
{
  GdkEventExpose *eevent;
  GdkEventMotion *mevent;
  GdkEventButton *bevent;
  GdkEventKey *kevent;
  gint tx, ty;
  GdkModifierType tmask;
  guint state = 0;
  Focus *focus;
  Object *obj;
  int return_val;
  int key_handled;
  
  return_val = FALSE;
  
  if (!canvas->window) 
    return FALSE;

  /*  get the pointer position  */
  gdk_window_get_pointer (canvas->window, &tx, &ty, &tmask);

  switch (event->type)
    {
    case GDK_EXPOSE:
      /*printf("GDK_EXPOSE\n"); */
      eevent = (GdkEventExpose *) event;
      /*
      redraw (gdisp, eevent->area.x, eevent->area.y,
	      eevent->area.width, eevent->area.height);
      */
      ddisplay_add_display_area(ddisp,
				eevent->area.x, eevent->area.y,
				eevent->area.x + eevent->area.width,
				eevent->area.y + eevent->area.height);
      ddisplay_flush(ddisp);
      break;

    case GDK_CONFIGURE:
      /*printf("GDK_CONFIGURE\n"); */
      if ((ddisp->width != ddisp->canvas->allocation.width) ||
	  (ddisp->height != ddisp->canvas->allocation.height)) {
	ddisplay_resize_canvas(ddisp,
			       ddisp->canvas->allocation.width,
			       ddisp->canvas->allocation.height);
	ddisplay_update_scrollbars(ddisp);
      }
      break;

    case GDK_2BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  (*active_tool->double_click_func) (active_tool, bevent, ddisp);
	  break;

	case 2:
	  break;

	case 3:
 	  break;

	default:
	  break;
	}
      break;

    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  (*active_tool->button_press_func) (active_tool, bevent, ddisp);
	  break;

	case 2:
	  break;

	case 3:
	  popup_shell = ddisp->shell;
          display_set_menu_sensitivity(ddisp);
	  gtk_menu_popup(GTK_MENU(ddisp->popup), NULL, NULL, NULL, NULL, 0, 0);
	  
 	  break;

	default:
	  break;
	}
      break;

    case GDK_BUTTON_RELEASE:
      bevent = (GdkEventButton *) event;
      state = bevent->state;

      switch (bevent->button)
	{
	case 1:
	  (*active_tool->button_release_func) (active_tool, bevent, ddisp);
	  break;

	case 2:
	  break;

	case 3:
	  break;

	default:
	  break;
	}
      break;

    case GDK_MOTION_NOTIFY:
      mevent = (GdkEventMotion *) event;
      state = mevent->state;

      if (mevent->is_hint) {
	mevent->x = tx;
	mevent->y = ty;
	mevent->state = tmask;
	mevent->is_hint = FALSE;
      }
      (*active_tool->motion_func) (active_tool, mevent, ddisp);
      break;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;
      state = kevent->state;
      key_handled = FALSE;
      
      focus = active_focus();
      if ((focus != NULL) &&
	  !(state & (GDK_CONTROL_MASK | GDK_MOD1_MASK | GDK_MOD2_MASK |
		     GDK_MOD3_MASK | GDK_MOD4_MASK | GDK_MOD5_MASK))) {
	/* Keys goes to the active focus. */
	obj = focus->obj;
	if (diagram_is_selected(ddisp->diagram, obj)) {
	  object_add_updates(obj, ddisp->diagram);
	  return_val = (focus->key_event)(focus, kevent->keyval,
					  kevent->string, kevent->length);
	  { /* Make sure object updates its data: */
	    Point p = obj->position;
	    (obj->ops->move)(obj,&p);  }
	  
	  object_add_updates(obj, ddisp->diagram);
	  
	  diagram_flush(ddisp->diagram);

	  key_handled = TRUE;
	}
      }
      if (!key_handled) {
	/* No focus to recive keys, take care of it ourselves. */
	return_val = TRUE;
	switch(kevent->keyval) {
	case GDK_Up:
	  ddisplay_scroll_up(ddisp);
	  ddisplay_flush(ddisp);
	  break;
	case GDK_Down:
	  ddisplay_scroll_down(ddisp);
	  ddisplay_flush(ddisp);
	  break;
	case GDK_Left:
	  ddisplay_scroll_left(ddisp);
	  ddisplay_flush(ddisp);
	  break;
	case GDK_Right:
	  ddisplay_scroll_right(ddisp);
	  ddisplay_flush(ddisp);
	  break;
	default:
	  return_val = FALSE;
	}
      }
      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;
      state = kevent->state;
      break;

    default:
      break;
    }
  
  return return_val;
}

gint
ddisplay_hsb_update (GtkAdjustment *adjustment,
		     DDisplay *ddisp)
{
  ddisplay_set_origo(ddisp, adjustment->value, ddisp->origo.y);
  ddisplay_add_update(ddisp, &ddisp->visible);
  ddisplay_flush(ddisp);
  return FALSE;
}

gint
ddisplay_vsb_update (GtkAdjustment *adjustment,
		     DDisplay *ddisp)
{
  ddisplay_set_origo(ddisp, ddisp->origo.x, adjustment->value);
  ddisplay_add_update(ddisp, &ddisp->visible);
  ddisplay_flush(ddisp);
  return FALSE;
}

gint
ddisplay_delete (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
  DDisplay *ddisp;

  printf("delete\n");
  ddisp = (DDisplay *)data;
  
  ddisplay_close(ddisp);
  return TRUE;
}

void
ddisplay_destroy (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  
  ddisp = (DDisplay *) data;

  if (popup_shell == ddisp->shell) {
    popup_shell = NULL;
  }

  ddisplay_really_destroy(ddisp);
}
