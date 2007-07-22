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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#ifdef GNOME
#include <gnome.h>
#endif
#include "display.h"
#include "diagram.h"
#include "tool.h"
#include "interface.h"
#include "focus.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "message.h"
#include "intl.h"
#include "magnify.h"
#include "diamenu.h"
#include "preferences.h"
#include "scroll_tool.h"
#include "commands.h"
#include "highlight.h"
#include "textedit.h"
#include "lib/parent.h"

/* This contains the point that was clicked to get this menu */
static Point object_menu_clicked_point;

static void
object_menu_proxy(GtkWidget *widget, gpointer data)
{
  DiaMenuItem *dia_menu_item;
  ObjectChange *obj_change;
  DiaObject *obj;
  DDisplay *ddisp = ddisplay_active();

  if (!ddisp) return;

  obj = (DiaObject *)ddisp->diagram->data->selected->data;
  dia_menu_item = (DiaMenuItem *) data;


  object_add_updates(obj, ddisp->diagram);
  obj_change = (dia_menu_item->callback)(obj, &object_menu_clicked_point,
					 dia_menu_item->callback_data);
  object_add_updates(obj, ddisp->diagram);
  diagram_update_connections_object(ddisp->diagram, obj, TRUE);

  if (obj_change != NULL) {
    undo_object_change(ddisp->diagram, obj, obj_change);
  }
  diagram_modified(ddisp->diagram);

  diagram_update_extents(ddisp->diagram);

  if (obj_change != NULL) {
    undo_set_transactionpoint(ddisp->diagram->undo);
  } else {
    message_warning(_("This object doesn't support Undo/Redo.\n"
		      "Undo information erased."));
    undo_clear(ddisp->diagram->undo);
  }

  diagram_flush(ddisp->diagram);
}

static void
dia_menu_free(DiaMenu *dia_menu) 
{
  if (dia_menu->app_data)
    gtk_object_destroy((GtkObject *)dia_menu->app_data);
  dia_menu->app_data = NULL;
  dia_menu->app_data_free = NULL;
}

/*
  This add a Properties... menu item to the GtkMenu passed, at the
  end and set the callback to raise de properties dialog

  pass TRUE in separator if you want to insert a separator before the poperty
  menu item.
*/
static void 
add_properties_menu_item (GtkMenu *menu, gboolean separator) 
{
  GtkWidget *menu_item = NULL;

  if (separator) {
    menu_item = gtk_menu_item_new();
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
  }

  menu_item = gtk_menu_item_new_with_label(_("Properties..."));
  g_signal_connect(GTK_OBJECT(menu_item), "activate", G_CALLBACK(dialogs_properties_callback), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);
}

static void
create_object_menu(DiaMenu *dia_menu)
{
  int i;
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new();

  if ( dia_menu->title ) {
    menu_item = gtk_menu_item_new_with_label(gettext(dia_menu->title));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
  }

  menu_item = gtk_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);

  for (i=0;i<dia_menu->num_items;i++) {
    DiaMenuItem *item = &dia_menu->items[i];

    if (item->active & DIAMENU_TOGGLE) {
      if (item->text)
        menu_item = gtk_check_menu_item_new_with_label(gettext(item->text));
      else
        menu_item = gtk_check_menu_item_new();
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
                                     item->active & DIAMENU_TOGGLE_ON);
      gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menu_item),
                                          TRUE);
    } else {
      if (item->text)
        menu_item = gtk_menu_item_new_with_label(gettext(item->text));
      else
        menu_item = gtk_menu_item_new();
    }
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
    item->app_data = menu_item;
    if ( dia_menu->items[i].callback ) {
          /* only connect signal handler if there is actually a callback */
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
                         (GtkSignalFunc)object_menu_proxy, &dia_menu->items[i]);
    } else { 
      if ( item->callback_data ) { 
            /* This menu item is a submenu if it has no callback, but does
             * Have callback_data. In this case the callback_data is a
             * DiaMenu pointer for the submenu. */
        if ( ((DiaMenu*)item->callback_data)->app_data == NULL ) {
              /* Create the popup menu items for the submenu. */
          create_object_menu((DiaMenu*)(item->callback_data) ) ;
          gtk_menu_item_set_submenu( GTK_MENU_ITEM (menu_item), 
                                     GTK_WIDGET(((DiaMenu*)(item->callback_data))->app_data));
        }
      }
    }
  }

  /* Finally add a Properties... menu item for objects*/
  add_properties_menu_item(GTK_MENU (menu), i > 0);

  dia_menu->app_data = menu;
  dia_menu->app_data_free = dia_menu_free;
}

static DiaMenuItem empty_menu_items[] = { {0, } };
static DiaMenu empty_menu = {
  NULL,
  sizeof(empty_menu_items)/sizeof(DiaMenuItem),
  empty_menu_items,
  NULL
};

static void
popup_object_menu(DDisplay *ddisp, GdkEventButton *bevent)
{
  Diagram *diagram;
  DiaObject *obj;
  GtkMenu *menu = NULL;
  DiaMenu *dia_menu = NULL;
  GList *selected_list;
  int i;
  int num_items;
  
  diagram = ddisp->diagram;
  if (g_list_length (diagram->data->selected) != 1)
    return;
  
  selected_list = diagram->data->selected;
  
  /* Have to have exactly one selected object */
  if (selected_list == NULL || g_list_next(selected_list) != NULL) {
    message_error("Selected list is %s while selected_count is %d\n",
		  (selected_list?"long":"empty"), g_list_length (diagram->data->selected));
    return;
  }
  
  obj = (DiaObject *)g_list_first(selected_list)->data;
  
  /* Possibly react differently at a handle? */

  /* Get its menu, and remember the # of object-generated items */
  if (obj->ops->get_object_menu == NULL) {
    dia_menu = &empty_menu;
    if (dia_menu->title &&
	(0 != strcmp(dia_menu->title,obj->type->name))) {
      dia_menu->app_data_free(dia_menu);
    }
    dia_menu->title = obj->type->name;
    num_items = 0;
  } else {
    dia_menu = (obj->ops->get_object_menu)(obj, &object_menu_clicked_point);
    num_items = dia_menu->num_items;
  }

  if (dia_menu->app_data == NULL) {
    create_object_menu(dia_menu);
  }
  /* Update active/nonactive menuitems */
  for (i=0;i<num_items;i++) {
    DiaMenuItem *item = &dia_menu->items[i];
    gtk_widget_set_sensitive(GTK_WIDGET(item->app_data),
			     item->active & DIAMENU_ACTIVE);
    if (item->active & DIAMENU_TOGGLE) {
      g_signal_handlers_block_by_func(GTK_CHECK_MENU_ITEM(item->app_data),
				      (GtkSignalFunc)object_menu_proxy, item);
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item->app_data),
				     item->active & DIAMENU_TOGGLE_ON);
      g_signal_handlers_unblock_by_func(GTK_CHECK_MENU_ITEM(item->app_data),
					(GtkSignalFunc)object_menu_proxy, item);
    }
  }

  menu = GTK_MENU(dia_menu->app_data);
  /* add the properties menu item to raise the properties from the contextual menu */
  
  gtk_menu_popup(menu, NULL, NULL, NULL, NULL, bevent->button, bevent->time);
}

gint
ddisplay_focus_in_event(GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  DDisplay *ddisp;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  ddisp = (DDisplay *)data;

  GTK_WIDGET_SET_FLAGS(widget, GTK_HAS_FOCUS);

  gtk_im_context_focus_in(GTK_IM_CONTEXT(ddisp->im_context));
  
  return FALSE;
}

gint
ddisplay_focus_out_event(GtkWidget *widget, GdkEventFocus *event,gpointer data)
{
  DDisplay *ddisp;
  int return_val;
  
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  return_val = FALSE;

  ddisp = (DDisplay *)data;

  GTK_WIDGET_UNSET_FLAGS (widget, GTK_HAS_FOCUS);

  gtk_im_context_focus_out(GTK_IM_CONTEXT(ddisp->im_context));

  return return_val;
}

void
ddisplay_realize(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(data != NULL);

  ddisp = (DDisplay *)data;

  gtk_im_context_set_client_window(GTK_IM_CONTEXT(ddisp->im_context),
                                   GDK_WINDOW(ddisp->shell->window));
}

void
ddisplay_unrealize (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (data != NULL);

  ddisp = (DDisplay *) data;

  if (ddisp->im_context)
    gtk_im_context_set_client_window(GTK_IM_CONTEXT(ddisp->im_context),
                                     GDK_WINDOW(ddisp->shell->window));
}

void
ddisplay_size_allocate (GtkWidget *widget,
			GtkAllocation *allocation,
			gpointer data)
{
  DDisplay *ddisp;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (allocation != NULL);
  g_return_if_fail (data != NULL);

#if 0
  g_print ("ddisp::size_allocate: %d,%d -> %d,%d\n", allocation->width, allocation->height,
	   widget->allocation.width, widget->allocation.height);
#endif
  widget->allocation = *allocation;
  ddisp = (DDisplay *)data;
}

void
ddisplay_popup_menu(DDisplay *ddisp, GdkEventButton *event)
{
  GtkWidget *menu;

  menu = menus_get_display_popup();

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		 event->button, event->time);
}
static void 
handle_key_event(DDisplay *ddisp, Focus *focus, guint keysym,
                 const gchar *str, int strlen) 
{
  DiaObject *obj = focus_get_object(focus);
  Point p = obj->position;
  ObjectChange *obj_change = NULL;
  gboolean modified;

  object_add_updates(obj, ddisp->diagram);
  
  modified = (focus->key_event)(focus, keysym, str, strlen,
				&obj_change);

      /* Make sure object updates its data and its connected: */
  p = obj->position;
  (obj->ops->move)(obj,&p);  
  diagram_update_connections_object(ddisp->diagram,obj,TRUE);
	  
  object_add_updates(obj, ddisp->diagram);

  if (modified) {
    if (obj_change != NULL) {
      undo_object_change(ddisp->diagram, obj, obj_change);
      undo_set_transactionpoint(ddisp->diagram->undo);
    }
    diagram_modified(ddisp->diagram);
  }

  diagram_flush(ddisp->diagram);
}
  

void 
ddisplay_im_context_commit(GtkIMContext *context, const gchar  *str,
                           DDisplay     *ddisp) 
{
      /* When using IM, we'll not get many key events past the IM filter,
         mostly IM Commits.

         regardless of the platform, "str" should be a clean UTF8 string
         (the default IM on X should perform the local->UTF8 conversion)
      */
      
  Focus *focus = active_focus();

  ddisplay_im_context_preedit_reset(ddisp, focus);
  
  if (focus != NULL)
    handle_key_event(ddisp, focus, 0, str, g_utf8_strlen(str,-1));
}

void 
ddisplay_im_context_preedit_changed(GtkIMContext *context,
                                    DDisplay *ddisp) 
{
  gint cursor_pos;
  Focus *focus = active_focus();

  ddisplay_im_context_preedit_reset(ddisp, focus);
  
  gtk_im_context_get_preedit_string(context, &ddisp->preedit_string,
                                    &ddisp->preedit_attrs, &cursor_pos);
  if (ddisp->preedit_string != NULL) {
    if (focus != NULL) {
      handle_key_event(ddisp, focus, 0, ddisp->preedit_string,
                       g_utf8_strlen(ddisp->preedit_string,-1));
    } else {
      ddisplay_im_context_preedit_reset(ddisp, focus);
    }
  }
}

/** Main input handler for a diagram canvas.
 */
gint
ddisplay_canvas_events (GtkWidget *canvas,
			GdkEvent  *event,
			DDisplay *ddisp)
{
  GdkEventExpose *eevent;
  GdkEventMotion *mevent;
  GdkEventButton *bevent;
  GdkEventKey *kevent;
  GdkEventScroll *sevent;
  gint tx, ty;
  GdkModifierType tmask;
  guint state = 0;
  Focus *focus;
  DiaObject *obj;
  Rectangle *visible;
  Point middle;
  int return_val;
  int key_handled;
  int width, height;
  int new_size;
  int im_context_used;
  static gboolean moving = FALSE;
  
  return_val = FALSE;
 
  if (!canvas->window) 
    return FALSE;

  switch (event->type)
  {
      case GDK_EXPOSE:
        eevent = (GdkEventExpose *) event;
        ddisplay_add_display_area(ddisp,
                                  eevent->area.x, eevent->area.y,
                                  eevent->area.x + eevent->area.width,
                                  eevent->area.y + eevent->area.height);
        ddisplay_flush(ddisp);
        break;

      case GDK_SCROLL:
        sevent = (GdkEventScroll *) event;

        switch (sevent->direction)
        {
            case GDK_SCROLL_UP:
              if (sevent->state & GDK_SHIFT_MASK)
                  ddisplay_scroll_left(ddisp);
              else if (sevent->state & GDK_CONTROL_MASK) {
                  ddisplay_untransform_coords(ddisp, sevent->x, sevent->y, &middle.x, &middle.y);
                  ddisplay_zoom(ddisp, &middle, 2);
              }
              else 
                  ddisplay_scroll_up(ddisp);
              break;
            case GDK_SCROLL_DOWN:
              if (sevent->state & GDK_SHIFT_MASK)
                  ddisplay_scroll_right(ddisp);
              else if (sevent->state & GDK_CONTROL_MASK) { 
                    ddisplay_untransform_coords(ddisp, sevent->x, sevent->y, &middle.x, &middle.y);
                    ddisplay_zoom(ddisp, &middle, 0.5);
              }
              else
                  ddisplay_scroll_down(ddisp);
              break;
            case GDK_SCROLL_LEFT:
              ddisplay_scroll_left(ddisp);
              break;
            case GDK_SCROLL_RIGHT:
              ddisplay_scroll_right(ddisp);
              break;
            default:
              break;
        }
        ddisplay_flush (ddisp);
        break;

      case GDK_CONFIGURE:
        if (ddisp->renderer != NULL) {
	  width = dia_renderer_get_width_pixels (ddisp->renderer);
	  height = dia_renderer_get_height_pixels (ddisp->renderer);
          new_size = ((width != ddisp->canvas->allocation.width) ||
                      (height != ddisp->canvas->allocation.height));
        } else {
          new_size = TRUE;
        }
        if (new_size) {
          ddisplay_resize_canvas(ddisp,
                                 ddisp->canvas->allocation.width,
                                 ddisp->canvas->allocation.height);
          ddisplay_update_scrollbars(ddisp);
        }
        /* TODO: Find out if display_set_active is needed for the GDK_CONFIGURE event */
        /* This is prevented for the integrated UI because it causes the diagram 
           showing in the diagram notebook to change on a resize event          */
        if (is_integrated_ui () == 0)
          display_set_active(ddisp);
        break;

      case GDK_FOCUS_CHANGE: {
	GdkEventFocus *focus = (GdkEventFocus*)event;
	if (focus->in) {
	  display_set_active(ddisp);
	  ddisplay_do_update_menu_sensitivity(ddisp);
	}
        break;
      }
      case GDK_2BUTTON_PRESS:
        display_set_active(ddisp);
        bevent = (GdkEventButton *) event;
        state = bevent->state;

        switch (bevent->button)
        {
            case 1:
              if (transient_tool)
                break;
              if (active_tool->double_click_func)
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
        display_set_active(ddisp);
        bevent = (GdkEventButton *) event;
        state = bevent->state;

        ddisplay_untransform_coords(ddisp,
                                    (int)bevent->x, (int)bevent->y,
                                    &object_menu_clicked_point.x,
                                    &object_menu_clicked_point.y);

        switch (bevent->button)
        {
            case 1:
              if (transient_tool)
                break;
                  /* get the focus again, may be lost by zoom combo */
	      moving = TRUE;
              gtk_widget_grab_focus(canvas);
              if (active_tool->button_press_func)
                (*active_tool->button_press_func) (active_tool, bevent, ddisp);
              break;

            case 2:
              if (ddisp->menu_bar == NULL && !is_integrated_ui()) {
                popup_object_menu(ddisp, bevent);
              }
	      else if (!transient_tool) {
		gtk_widget_grab_focus(canvas);
		transient_tool = create_scroll_tool();
		(*transient_tool->button_press_func) (transient_tool, bevent, ddisp);
	      }
              break;

            case 3:
              if (transient_tool)
                break;
              if (ddisp->menu_bar == NULL) {
                if (bevent->state & GDK_CONTROL_MASK || is_integrated_ui ()) {
                      /* for two button mouse users ... */
                  popup_object_menu(ddisp, bevent);
                  break;
                }
                ddisplay_popup_menu(ddisp, bevent);
                break;
              }
              else {
                popup_object_menu(ddisp, bevent);
                break;
              }
            default:
              break;
        }
        break;

      case GDK_BUTTON_RELEASE:
        display_set_active(ddisp);
        bevent = (GdkEventButton *) event;
        state = bevent->state;

        switch (bevent->button)
        {
            case 1:
	      if (moving)
  		moving = FALSE;		      
              if (active_tool->button_release_func)
                (*active_tool->button_release_func) (active_tool,
                                                     bevent, ddisp);
              break;

            case 2:
	      if (transient_tool) {
	        (*transient_tool->button_release_func) (transient_tool,
  	                                             bevent, ddisp);
								
	        tool_free(transient_tool);
	        transient_tool = NULL;
	      }
              break;

            case 3:
              break;

            default:
              break;
        }
        break;

      case GDK_MOTION_NOTIFY:
	/*  get the pointer position  */
	gdk_window_get_pointer (canvas->window, &tx, &ty, &tmask);

        mevent = (GdkEventMotion *) event;
        state = mevent->state;

        if (mevent->is_hint) {
          mevent->x = tx;
          mevent->y = ty;
          mevent->state = tmask;
          mevent->is_hint = FALSE;
        }
        if (transient_tool && (*transient_tool->motion_func)) 
          (*transient_tool->motion_func) (transient_tool, mevent, ddisp);
        else if (active_tool->motion_func)
          (*active_tool->motion_func) (active_tool, mevent, ddisp);
        break;

      case GDK_KEY_PRESS:
	if (moving)	/*Disable Keyboard accels whilst draggin an object*/ 
		break;
        display_set_active(ddisp);
        kevent = (GdkEventKey *)event;
        state = kevent->state;
        key_handled = FALSE;
        im_context_used = FALSE;

        focus = active_focus();
        if (focus != NULL) {
	  /* Keys goes to the active focus. */
	  obj = focus_get_object(focus);
	  if (diagram_is_selected(ddisp->diagram, obj)) {

	    if (!gtk_im_context_filter_keypress(
		  GTK_IM_CONTEXT(ddisp->im_context), kevent)) {
	      
	      if (kevent->keyval == GDK_Tab) {
		focus = textedit_move_focus(ddisp, focus,
					    (state & GDK_SHIFT_MASK) == 0);
		obj = focus_get_object(focus);
	      } else {
		/*! key event not swallowed by the input method ? */
		handle_key_event(ddisp, focus, kevent->keyval,
				 kevent->string, kevent->length);
		
		diagram_flush(ddisp->diagram);
	      }
	    }
	    return_val = key_handled = im_context_used = TRUE;
	  }
	}

#if 0 /* modifier requirment added 2004-07-17, IMO reenabling unmodified keys here
               * shouldn't break im_context handling. How to test?    --hb
               */
        if (!key_handled && (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
#else            
        if (!key_handled) {
#endif
              /* IM doesn't need receive keys, take care of it ourselves. */
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
              case GDK_KP_Add:
              case GDK_plus:
                visible = &ddisp->visible;
                middle.x = visible->left*0.5 + visible->right*0.5;
                middle.y = visible->top*0.5 + visible->bottom*0.5;
	  
                ddisplay_zoom(ddisp, &middle, M_SQRT2);
                break;
              case GDK_KP_Subtract:
              case GDK_minus:
                visible = &ddisp->visible;
                middle.x = visible->left*0.5 + visible->right*0.5;
                middle.y = visible->top*0.5 + visible->bottom*0.5;
	  
                ddisplay_zoom(ddisp, &middle, M_SQRT1_2);
                break;
              case GDK_Shift_L:
              case GDK_Shift_R:
                if (active_tool->type == MAGNIFY_TOOL)
                  set_zoom_out(active_tool);
                break;
              case GDK_Escape:
                view_unfullscreen();
                break;
              default:
                if (kevent->string && kevent->keyval == ' ') {
                  tool_select_former();
                } else if ((kevent->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)) == 0 && 
			   kevent->length != 0) {
		  /* Find first editable */
#ifdef NEW_TEXT_EDIT
		  modify_edit_first_text(ddisp);
                  return_val = FALSE;
#endif
                }
          }
        }

        if (!im_context_used)
          gtk_im_context_reset(GTK_IM_CONTEXT(ddisp->im_context));
        
        break;

      case GDK_KEY_RELEASE:
        kevent = (GdkEventKey *) event;
        state = kevent->state;
        if (gtk_im_context_filter_keypress(GTK_IM_CONTEXT(ddisp->im_context),
                                           kevent)) {
          return_val = TRUE;
        } else {
          switch(kevent->keyval) {
              case GDK_Shift_L:
              case GDK_Shift_R:
                if (active_tool->type == MAGNIFY_TOOL)
                  set_zoom_in(active_tool);
                break;
              default:
                break;
          }
        }
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
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
  return FALSE;
}

gint
ddisplay_vsb_update (GtkAdjustment *adjustment,
		     DDisplay *ddisp)
{
  ddisplay_set_origo(ddisp, ddisp->origo.x, adjustment->value);
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
  return FALSE;
}

gint
ddisplay_delete (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
  DDisplay *ddisp;

  ddisp = (DDisplay *)data;
  
  ddisplay_close(ddisp);
  return TRUE;
}

void
ddisplay_destroy (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  
  ddisp = (DDisplay *) data;

  ddisplay_really_destroy(ddisp);
}

/* returns NULL if object cannot be created */
DiaObject *
ddisplay_drop_object(DDisplay *ddisp, gint x, gint y, DiaObjectType *otype,
		     gpointer user_data)
{
  Point droppoint;
  Point droppoint_orig;
  Handle *handle1, *handle2;
  DiaObject *obj, *p_obj;
  GList *list;
  real click_distance;

  ddisplay_untransform_coords(ddisp, x, y, &droppoint.x, &droppoint.y);

  /* save it before snap_to_grid modifies it */
  droppoint_orig = droppoint;

  snap_to_grid(ddisp, &droppoint.x, &droppoint.y);

  obj = dia_object_default_create (otype, &droppoint,
                                   user_data,
                                   &handle1, &handle2);


  click_distance = ddisplay_untransform_length(ddisp, 3.0);

  /* Notice that using diagram_find_clicked_object doesn't allow any object
   * below the first to be a parent.  This should be fixed.
   * -Lars
   */
  p_obj = diagram_find_clicked_object(ddisp->diagram, &droppoint_orig,
				    click_distance);

  if (p_obj && object_flags_set(p_obj, DIA_OBJECT_CAN_PARENT))
    /* the tool was dropped inside an object that takes children*/
  {
    Rectangle p_ext, c_ext;
    real parent_height, child_height, parent_width, child_width;
    real vadjust = 0.0, hadjust = 0.0;
    Point new_pos;

    obj->parent = p_obj;
    p_obj->children = g_list_append(p_obj->children, obj);

    /* This is not really what we want.  We want the box containing all
     * rendered parts of the object (not the bbox).  But it'll do for now,
     * since we don't have the 'rendered bbox'.
     * -Lars
     */
    parent_handle_extents(p_obj, &p_ext);
    parent_handle_extents(obj, &c_ext);

    parent_height = p_ext.bottom - p_ext.top;
    child_height = c_ext.bottom - c_ext.top;

    parent_width = p_ext.right - p_ext.left;
    child_width = c_ext.right - c_ext.left;

    /* we need the pre-snap position, but must remember that handles can
     * be to the left of the droppoint */
    c_ext.left = droppoint_orig.x - (obj->position.x - c_ext.left);
    c_ext.top  = droppoint_orig.y - (obj->position.y - c_ext.top);
    c_ext.right = c_ext.left + child_width;
    c_ext.bottom = c_ext.top + child_height;

    if (c_ext.left < p_ext.left) {
      hadjust = p_ext.left - c_ext.left;
    } else if (c_ext.right > p_ext.right) {
      hadjust = p_ext.right - c_ext.right;
    }
    if (c_ext.top < p_ext.top) {
      vadjust = p_ext.top - c_ext.top;
    } else if (c_ext.bottom > p_ext.bottom) {
      vadjust = p_ext.bottom - c_ext.bottom;
    }

    if (child_width > parent_width ||
	child_height > parent_height) {
      message_error(_("The object you dropped cannot fit into its parent. \nEither expand the parent object, or drop the object elsewhere."));
      obj->parent->children = g_list_remove(obj->parent->children, obj);
      obj->ops->destroy (obj);
      return NULL;
    }

    if (hadjust || vadjust) {
      new_pos.x = droppoint.x + hadjust;
      new_pos.y = droppoint.y + vadjust;
      obj->ops->move(obj, &new_pos);
    }
  }

  diagram_add_object(ddisp->diagram, obj);
  diagram_remove_all_selected(ddisp->diagram, TRUE); /* unselect all */
  diagram_select(ddisp->diagram, obj);
  obj->ops->selectf(obj, &droppoint, ddisp->renderer);
  textedit_activate_object(ddisp, obj, NULL);

  /* Connect first handle if possible: */
  if ((handle1 != NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1, FALSE);
  }
  object_add_updates(obj, ddisp->diagram);
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);

  list = g_list_prepend(NULL, obj);
  undo_insert_objects(ddisp->diagram, list, 1);
  diagram_update_extents(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
  diagram_modified(ddisp->diagram);
  if (prefs.reset_tools_after_create)
    tool_reset();
  return obj;
}
