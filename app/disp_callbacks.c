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
#include "charconv.h"

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
  gtk_menu_ensure_uline_accel_group (GTK_MENU (menu)) ;

  if ( dia_menu->title ) {
    menu_item = gtk_menu_item_new_with_label(gettext(dia_menu->title));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_append(GTK_MENU(menu), menu_item);
    gtk_widget_show(menu_item);
  }

  menu_item = gtk_menu_item_new();
  gtk_menu_append(GTK_MENU(menu), menu_item);
  gtk_widget_show(menu_item);

  for (i=0;i<dia_menu->num_items;i++) {
    gchar *label = dia_menu->items[i].text;

    if (label)
      menu_item = gtk_menu_item_new_with_label(gettext(label));
    else
      menu_item = gtk_menu_item_new();
    gtk_menu_append(GTK_MENU(menu), menu_item);
    gtk_widget_show(menu_item);
    dia_menu->items[i].app_data = menu_item;
    if ( dia_menu->items[i].callback ) {
    /* only connect signal handler if there is actually a callback */
      gtk_signal_connect(GTK_OBJECT(menu_item), "activate",
			 object_menu_proxy, &dia_menu->items[i]);
    } else { 
      if ( dia_menu->items[i].callback_data ) { 
        /* This menu item is a submenu if it has no callback, but does
	 * Have callback_data. In this case the callback_data is a
	 * DiaMenu pointer for the submenu. */
        if ( ((DiaMenu*)dia_menu->items[i].callback_data)->app_data == NULL ) {
	  /* Create the popup menu items for the submenu. */
          create_object_menu( (DiaMenu*)(dia_menu->items[i].callback_data) ) ;
          gtk_menu_item_set_submenu( GTK_MENU_ITEM (menu_item), 
      GTK_WIDGET(((DiaMenu*)(dia_menu->items[i].callback_data))->app_data));
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
      gtk_widget_set_sensitive((GtkWidget *)dia_menu->items[i].app_data,
			       dia_menu->items[i].active);
    }

    menu = GTK_MENU(dia_menu->app_data);
  } else {
    if (no_menu == NULL) {
      no_menu = gtk_menu_new();

      menu_item = gtk_menu_item_new_with_label(_("No object menu"));
      gtk_menu_append(GTK_MENU(no_menu), menu_item);
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
  gtk_widget_draw_focus(widget);

#ifdef USE_XIM
  if (gdk_im_ready () && ddisp->ic)
    gdk_im_begin(ddisp->ic, widget->window);
#endif

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
  gtk_widget_draw_focus (widget);

#ifdef USE_XIM
  gdk_im_end ();
#endif

  return return_val;
}

void
ddisplay_realize(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;

  g_return_if_fail(widget != NULL);
  g_return_if_fail(data != NULL);

  ddisp = (DDisplay *)data;

#ifdef USE_XIM
  if (gdk_im_ready() && (ddisp->ic_attr = gdk_ic_attr_new()) != NULL) {
    gint width, height;
    GdkColormap *colormap;
    GdkICAttr *attr = ddisp->ic_attr;
    GdkICAttributesType attrmask = GDK_IC_ALL_REQ;
    GdkIMStyle style;
    GdkIMStyle supported_style =
      GDK_IM_PREEDIT_NONE |
      GDK_IM_PREEDIT_NOTHING |
      GDK_IM_PREEDIT_POSITION |
      GDK_IM_STATUS_NONE |
      GDK_IM_STATUS_NOTHING;

    if (widget->style && widget->style->font->type != GDK_FONT_FONTSET)
      supported_style &= ~GDK_IM_PREEDIT_POSITION;

    attr->style = style = gdk_im_decide_style(supported_style);
    attr->client_window = widget->window;

    if ((colormap = gtk_widget_get_colormap(widget)) !=
	gtk_widget_get_default_colormap()) {
      attrmask |= GDK_IC_PREEDIT_COLORMAP;
      attr->preedit_colormap = colormap;
    }

    attrmask |= GDK_IC_PREEDIT_FOREGROUND;
    attrmask |= GDK_IC_PREEDIT_BACKGROUND;
    attr->preedit_foreground = widget->style->fg[GTK_STATE_NORMAL];
    attr->preedit_background = widget->style->base[GTK_STATE_NORMAL];


    switch (style & GDK_IM_PREEDIT_MASK) {
    case GDK_IM_PREEDIT_POSITION:
      if (ddisp->canvas->style &&
	  ddisp->canvas->style->font->type != GDK_FONT_FONTSET) {
	g_warning("over-the-spot style requires fontset");
	break;
      }
      attrmask |= GDK_IC_PREEDIT_POSITION_REQ;
      gdk_window_get_size(widget->window, &width, &height);
      attr->spot_location.x = 0;
      attr->spot_location.y = 14;
      attr->preedit_area.x = 0;
      attr->preedit_area.y = 0;
      attr->preedit_area.width = width;
      attr->preedit_area.height = height;
      attr->preedit_fontset = widget->style->font;
    }
    ddisp->ic = gdk_ic_new(attr, attrmask);
    if (ddisp->ic == NULL)
      g_warning("could not create input context");
    else {
      gtk_widget_add_events(ddisp->canvas, gdk_ic_get_events(ddisp->ic));

      if (GTK_WIDGET_HAS_FOCUS(widget))
	gdk_im_begin(ddisp->ic, widget->window);
    }
  }
#endif
}

void
ddisplay_unrealize (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp;
  
  g_return_if_fail (widget != NULL);
  g_return_if_fail (data != NULL);

  ddisp = (DDisplay *) data;

#ifdef USE_XIM
  if (gdk_im_ready ()) {
    if (ddisp->ic)
      gdk_ic_destroy (ddisp->ic);
    ddisp->ic = NULL;
    if (ddisp->ic_attr)
      gdk_ic_attr_destroy (ddisp->ic_attr);
    ddisp->ic_attr = NULL;
  }
#endif
 
}

#ifdef USE_XIM
static void
set_input_dialog(DDisplay *ddisp, int x, int y)
{
  if (gdk_im_ready () && ddisp->ic && (gdk_ic_get_style(ddisp->ic) & GDK_IM_PREEDIT_POSITION)) {
    ddisp->ic_attr->spot_location.x = x;
    ddisp->ic_attr->spot_location.y = y; 
    gdk_ic_set_attr(ddisp->ic, ddisp->ic_attr, GDK_IC_SPOT_LOCATION);
  }
}
#endif

void
ddisplay_popup_menu(DDisplay *ddisp, GdkEventButton *event)
{
  GtkWidget *menu;

  popup_shell = ddisp->shell;
  menus_get_image_menu(&menu, NULL);
  gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
		 event->button, event->time);
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
  int modified = FALSE;
  int x, y;

  return_val = FALSE;
 
  if (!canvas->window) 
    return FALSE;

  /*  get the pointer position  */
  gdk_window_get_pointer (canvas->window, &tx, &ty, &tmask);

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

    case GDK_CONFIGURE:
      if (ddisp->renderer != NULL) {
	width = ddisp->renderer->pixel_width;
	height = ddisp->renderer->pixel_height;
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

    case GDK_FOCUS_CHANGE:
      ddisplay_do_update_menu_sensitivity(ddisp);
      break;
      
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
        case 4: /* for wheel mouse; button 4 and 5 */
          ddisplay_scroll_up(ddisp);
          ddisplay_flush(ddisp);
          break;
        case 5:
          ddisplay_scroll_down(ddisp);
          ddisplay_flush(ddisp);
          break;
        case 6: /* for two-wheel mouse; button 6 and 7 */
          ddisplay_scroll_left(ddisp);
          ddisplay_flush(ddisp);
          break;
        case 7:
          ddisplay_scroll_right(ddisp);
          ddisplay_flush(ddisp);
          break;
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
      if (*active_tool->motion_func)
	(*active_tool->motion_func) (active_tool, mevent, ddisp);
      break;

    case GDK_KEY_PRESS:
      display_set_active(ddisp);
      kevent = (GdkEventKey *) event;
      state = kevent->state;
      key_handled = FALSE;

      focus = active_focus();
      if ((focus != NULL) &&
	  !(state & (GDK_CONTROL_MASK | GDK_MOD1_MASK)) ) {
	/* Keys goes to the active focus. */
	obj = focus->obj;
	if (diagram_is_selected(ddisp->diagram, obj)) {
	  ObjectChange *obj_change = NULL;
	  utfchar *utf;
	  int len = 0;
	
	  object_add_updates(obj, ddisp->diagram);
#ifdef USE_XIM
	  ddisplay_transform_coords(ddisp, obj->position.x, obj->position.y,
				    &x, &y);
	  set_input_dialog(ddisp, x, y);
#endif
#ifdef UNICODE_WORK_IN_PROGRESS
	  utf = charconv_utf8_from_gtk_event_key (kevent->keyval, kevent->string);
#else
	  /* 'utf' is definetly not utf */
#  ifdef GTK_TALKS_UTF8_WE_DONT
	  utf = charconv_utf8_to_local8 (kevent->string);
#  else
	  utf = g_strdup (kevent->string);
#  endif
#endif
	  if (utf != NULL) len = uni_strlen (utf, strlen (utf));
	  modified = (focus->key_event)(focus, kevent->keyval,
					utf, len, &obj_change);
	  if (utf != NULL) g_free (utf);
	  { /* Make sure object updates its data and its connected: */
	    Point p = obj->position;
	    (obj->ops->move)(obj,&p);  
            diagram_update_connections_object(ddisp->diagram,obj,TRUE);
          }
	  
	  object_add_updates(obj, ddisp->diagram);

	  if (modified && (obj_change != NULL)) {
	    undo_object_change(ddisp->diagram, obj, obj_change);
	    undo_set_transactionpoint(ddisp->diagram->undo);
	  }

	  diagram_flush(ddisp->diagram);

	  return_val = key_handled = TRUE;
	}
      }
      if (!key_handled) {
	/* No focus to receive keys, take care of it ourselves. */
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
      switch(kevent->keyval) {
	case GDK_Shift_L:
	case GDK_Shift_R:
	  if (active_tool->type == MAGNIFY_TOOL)
	    set_zoom_in(active_tool);
	  break;
	default:
	  break;
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

void
ddisplay_drop_object(DDisplay *ddisp, gint x, gint y, ObjectType *otype,
		     gpointer user_data)
{
  Point droppoint;
  Handle *handle1, *handle2;
  Object *obj;
  GList *list;

  ddisplay_untransform_coords(ddisp, x, y, &droppoint.x, &droppoint.y);

  snap_to_grid(ddisp, &droppoint.x, &droppoint.y);

  obj = otype->ops->create(&droppoint, user_data, &handle1, &handle2);
  diagram_add_object(ddisp->diagram, obj);
  diagram_remove_all_selected(ddisp->diagram, TRUE); /* unselect all */
  diagram_select(ddisp->diagram, obj);
  obj->ops->selectf(obj, &droppoint, (Renderer *)ddisp->renderer);

  /* Connect first handle if possible: */
  if ((handle1 != NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1);
  }
  object_add_updates(obj, ddisp->diagram);
  diagram_flush(ddisp->diagram);

  list = g_list_prepend(NULL, obj);
  undo_insert_objects(ddisp->diagram, list, 1);
  diagram_update_extents(ddisp->diagram);
  diagram_modified(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
}
