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

/* This contains the point that was clicked to get this menu */
static Point object_menu_clicked_point;

static void
object_menu_proxy(GtkWidget *widget, gpointer data)
{
  DiaMenuItem *dia_menu_item;
  ObjectChange *obj_change;

  DDisplay *ddisp = ddisplay_active();
  Object *obj = (Object *)ddisp->diagram->data->selected->data;

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
dia_menu_free(DiaMenu *dia_menu) {
  if (dia_menu->app_data)
    gtk_object_destroy((GtkObject *)dia_menu->app_data);
  dia_menu->app_data = NULL;
  dia_menu->app_data_free = NULL;
}

static void
create_object_menu(DiaMenu *dia_menu)
{
  int i;
  GtkWidget *menu;
  GtkWidget *menu_item;

  menu = gtk_menu_new();
      //FIXME?: gtk_menu_ensure_uline_accel_group (GTK_MENU (menu)) ;

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

  dia_menu->app_data = menu;
  dia_menu->app_data_free = dia_menu_free;
}

static void
popup_object_menu(DDisplay *ddisp, GdkEventButton *bevent)
{
  Diagram *diagram;
  Object *obj;
  GtkMenu *menu = NULL;
  DiaMenu *dia_menu = NULL;
  GtkWidget *menu_item;
  GList *selected_list;
  static GtkWidget *no_menu = NULL;
  int i;
  
  diagram = ddisp->diagram;
  if (diagram->data->selected_count != 1)
    return;
  
  selected_list = diagram->data->selected;
  
  /* Have to have exactly one selected object */
  if (selected_list == NULL || g_list_next(selected_list) != NULL) {
    message_error("Selected list is %s while selected_count is %d\n",
		  (selected_list?"long":"empty"), diagram->data->selected_count);
    return;
  }
  
  obj = (Object *)g_list_first(selected_list)->data;
  
  /* Possibly react differently at a handle? */

  /* Get its menu */
  if (obj->ops->get_object_menu == NULL) {
    dia_menu = NULL;
  } else {
    dia_menu = (obj->ops->get_object_menu)(obj, &object_menu_clicked_point);
  }

  if (dia_menu != NULL) {
    if (dia_menu->app_data == NULL)
      create_object_menu(dia_menu);
    
    /* Update active/nonactive menuitems */
    for (i=0;i<dia_menu->num_items;i++) {
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
  } else {
    if (no_menu == NULL) {
      no_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label(_("No object menu"));
      gtk_menu_shell_append (GTK_MENU_SHELL (no_menu), menu_item);
      gtk_widget_show(menu_item);
      gtk_widget_set_sensitive(menu_item, FALSE);
    }
    menu = GTK_MENU(no_menu);
  }
  
  popup_shell = ddisp->shell;
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
      /* FIXME?: gtk_widget_draw_focus(widget); */
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

  widget->allocation = *allocation;
  ddisp = (DDisplay *)data;
}

void
ddisplay_popup_menu(DDisplay *ddisp, GdkEventButton *event)
{
  GtkWidget *menu;

  popup_shell = ddisp->shell;
  menus_get_image_menu(&menu, NULL);

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		 event->button, event->time);
}

static void handle_key_event(DDisplay *ddisp, Focus *focus, guint keysym,
                             const gchar *str, int strlen) {
  Object *obj = focus->obj;
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
    diagram_modified(ddisp->diagram);
    if (obj_change != NULL) {
      undo_object_change(ddisp->diagram, obj, obj_change);
      undo_set_transactionpoint(ddisp->diagram->undo);
    }
  }

  diagram_flush(ddisp->diagram);
}
  

void ddisplay_im_context_commit(GtkIMContext *context, const gchar  *str,
                                DDisplay     *ddisp) {
      /* When using IM, we'll not get many key events past the IM filter,
         mostly IM Commits.

         regardless of the platform, "str" should be a clean UTF8 string
         (the default IM on X should perform the local->UTF8 conversion)
      */
      
  handle_key_event(ddisp, active_focus(), 0, str, g_utf8_strlen(str,-1));
}

void ddisplay_im_context_preedit_changed(GtkIMContext *context,
                                         DDisplay *ddisp) {
/*  char *str;
  PangoAttrList *attrs;
  gint cursor_pos;
    
  gtk_im_context_get_preedit_string(ddisp->im_context,
                                    &str,&attrs,&cursor_pos);

  g_message("received a 'preedit changed'; str=%s cursor_pos=%i",
            str,cursor_pos);

  g_free(str);
  pango_attr_list_unref(attrs);
*/
}

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
  Object *obj;
  Rectangle *visible;
  Point middle;
  int return_val;
  int key_handled;
  int width, height;
  int new_size;

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
              ddisplay_scroll_up(ddisp);
              break;
            case GDK_SCROLL_DOWN:
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
              if (*active_tool->double_click_func)
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
                  /* get the focus again, may be lost by zoom combo */
              gtk_widget_grab_focus(canvas);
              if (*active_tool->button_press_func)
                (*active_tool->button_press_func) (active_tool, bevent, ddisp);
              break;

            case 2:
              if (ddisp->menu_bar == NULL) {
                popup_object_menu(ddisp, bevent);
              }
              break;

            case 3:
              if (ddisp->menu_bar == NULL) {
                if (bevent->state & GDK_CONTROL_MASK) {
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
              if (*active_tool->button_release_func)
                (*active_tool->button_release_func) (active_tool,
                                                     bevent, ddisp);
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
        if (*active_tool->motion_func)
          (*active_tool->motion_func) (active_tool, mevent, ddisp);
        break;

      case GDK_KEY_PRESS:
        display_set_active(ddisp);
        kevent = (GdkEventKey *)event;
        state = kevent->state;
        key_handled = FALSE;

        focus = active_focus();
        if ((focus != NULL) &&
            !(state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) ) {
              /* Keys goes to the active focus. */
          obj = focus->obj;
          if (diagram_is_selected(ddisp->diagram, obj)) {

            if (!gtk_im_context_filter_keypress(
                  GTK_IM_CONTEXT(ddisp->im_context), kevent)) {
              
                  /*! key event not swallowed by the input method ? */
              handle_key_event(ddisp, focus, kevent->keyval,
                               kevent->string, kevent->length);

              diagram_flush(ddisp->diagram);
            }
          }
          return_val = key_handled = TRUE;
        }

        if (!key_handled) {
              /* No focus to receive keys, take care of it ourselves. */
          return_val = TRUE;
          gtk_im_context_reset(GTK_IM_CONTEXT(ddisp->im_context));
          
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
              default:
                if (kevent->string && 0 == strcmp(" ",kevent->string)) {
                  tool_select_former();
                } else { 
                  return_val = FALSE;
                }
          }
        }
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

  if (popup_shell == ddisp->shell) {
    popup_shell = NULL;
  }

  ddisplay_really_destroy(ddisp);
}

inline int 
round_up (double x)
{
  if (x - (int) x > 0.001)
    return (int) x + 1;
  else
    return (int) x ;
}


/* returns NULL if object cannot be created */
Object *
ddisplay_drop_object(DDisplay *ddisp, gint x, gint y, ObjectType *otype,
		     gpointer user_data)
{
  Point droppoint;
  Point droppoint_orig;
  Handle *handle1, *handle2;
  Object *obj, *p_obj;
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

  p_obj = diagram_find_clicked_object(ddisp->diagram, &droppoint_orig,
				    click_distance);

  if (p_obj && p_obj->can_parent) /* the tool was dropped inside an object that takes children*/
  {
    Rectangle *p_ext, *c_ext;
    int new_height = 0, new_width = 0;
    real parent_height, child_height, parent_width, child_width;
    Point new_pos;

    obj->parent = p_obj;
    p_obj->children = g_list_append(p_obj->children, obj);

    p_ext = parent_handle_extents(p_obj);
    c_ext = parent_handle_extents(obj);

    parent_height = p_ext->bottom - p_ext->top;
    child_height = c_ext->bottom - c_ext->top;

    parent_width = p_ext->right - p_ext->left;
    child_width = c_ext->right - c_ext->left;

    /* we need the pre-snap position */
    c_ext->left = droppoint_orig.x;
    c_ext->top = droppoint_orig.y;
    c_ext->right = c_ext->left + child_width;
    c_ext->bottom = c_ext->top + child_height;

    /* check if the top of the child is inside the parent, but the bottom is not */
    if (c_ext->top < p_ext->bottom && c_ext->bottom > p_ext->bottom)
    {
      /* check if child is smaller than parent height wise */
      if (child_height < parent_height)
        new_height = child_height;
      /* check if parent is bigger than 1 in height */
      else if (parent_height  > 1)
        new_height = round_up(parent_height) - 1;
    }
    else
    {
      new_height = child_height;
    }
    /* check if the left of the child is inside the partent, but the right is not */
    if (c_ext->left < p_ext->right && c_ext->right > p_ext->right)
    {
      /* check if child is smaller than parent width wise */
      if (child_width < parent_width)
        new_width = child_width;
      /* check if parent is bigger than 1 in width */
      else if (parent_width > 1)
        new_width = round_up(parent_width) - 1;
    }
    else
    {
      new_width = child_width;
    }

    g_free(p_ext);
    g_free(c_ext);

    /* if we can't fit in both directions, produce an error */
    if (!new_height && !new_width)
    {
      message_error(_("The object you dropped cannot fit into its parent. \nEither expand the parent object, or drop the object elsewhere."));
      obj->parent->children = g_list_remove(obj->parent->children, obj);
      obj->ops->destroy (obj);
      return NULL;
    }
    /* if we can't fit height wise, make height same as of the parent */
    else if (!new_height)
      new_height = parent_height;
    /* if we can't fit width wise, make the width same as of the parent */
    else if (!new_width)
      new_width = parent_width;

    new_pos.x = droppoint.x + new_width;
    new_pos.y = droppoint.y + new_height;
    obj->ops->move_handle(obj, handle2, &new_pos,
                                   HANDLE_MOVE_USER,0);

  }


  diagram_add_object(ddisp->diagram, obj);
  diagram_remove_all_selected(ddisp->diagram, TRUE); /* unselect all */
  diagram_select(ddisp->diagram, obj);
  obj->ops->selectf(obj, &droppoint, ddisp->renderer);

  /* Connect first handle if possible: */
  if ((handle1 != NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1);
  }
  object_add_updates(obj, ddisp->diagram);
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);

  list = g_list_prepend(NULL, obj);
  undo_insert_objects(ddisp->diagram, list, 1);
  diagram_update_extents(ddisp->diagram);
  diagram_modified(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
  if (prefs.reset_tools_after_create)
    tool_reset();
  return obj;
}
