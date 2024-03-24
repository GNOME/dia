/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 * © 2023 Hubert Figuière <hub@figuiere.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "display.h"
#include "diagram.h"
#include "tool.h"
#include "interface.h"
#include "focus.h"
#include "object_ops.h"
#include "connectionpoint_ops.h"
#include "menus.h"
#include "message.h"
#include "magnify.h"
#include "diamenu.h"
#include "preferences.h"
#include "scroll_tool.h"
#include "commands.h"
#include "textedit.h"
#include "lib/parent.h"
#include "dia_dirs.h"
#include "object.h"
#include "disp_callbacks.h"
#include "create.h"
#include "dia-object-change-list.h"

typedef struct {
	GdkEvent *event; /* Button down event which may be holding */
	DDisplay *ddisp; /* DDisplay where event occurred */
	guint     tag;   /* Tag for timeout */
} HoldTimeoutData;

static HoldTimeoutData hold_data = {NULL, NULL, 0};



static void
object_menu_item_proxy (GtkWidget *widget, gpointer data)
{
  DiaMenuItem *dia_menu_item;
  DiaObjectChange *obj_change;
  DiaObject *obj;
  DDisplay *ddisp = ddisplay_active();
  Point last_clicked_pos;

  if (!ddisp) return;

  last_clicked_pos = ddisplay_get_clicked_position(ddisp);
  obj = (DiaObject *)ddisp->diagram->data->selected->data;
  dia_menu_item = (DiaMenuItem *) data;


  object_add_updates(obj, ddisp->diagram);
  obj_change = (dia_menu_item->callback)(obj, &last_clicked_pos,
					 dia_menu_item->callback_data);
  object_add_updates(obj, ddisp->diagram);
  diagram_update_connections_object(ddisp->diagram, obj, TRUE);

  if (obj_change != NULL) {
    dia_object_change_change_new (ddisp->diagram, obj, obj_change);
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
  g_clear_object (&dia_menu->app_data);
  dia_menu->app_data_free = NULL;
}


/*
  This add a Properties... menu item to the GtkMenu passed, at the
  end and set the callback to raise de properties dialog

  pass TRUE in separator if you want to insert a separator before the property
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

  menu_item = gtk_menu_item_new_with_label(_("Properties…"));
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(dialogs_properties_callback), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);
}


static void
_follow_link_callback (GtkAction *action, gpointer data)
{
  DiaObject *obj;
  DDisplay *ddisp = ddisplay_active();
  char *url;

  if (!ddisp) return;

  obj = (DiaObject *) ddisp->diagram->data->selected->data;

  url = dia_object_get_meta (obj, "url");

  if (!url) return;

  if (strstr (url, "://") == NULL) {
    if (!g_path_is_absolute(url)) {
      gchar *p = NULL;

      if (ddisp->diagram->filename) {
        p = dia_absolutize_filename (ddisp->diagram->filename, url);
      }

      if (p) {
        g_clear_pointer (&url, g_free);
        url = p;
      }
    }
    dia_file_open (url, NULL);
  } else {
    activate_url (GTK_WIDGET (ddisp->shell), url, NULL);
  }

  g_clear_pointer (&url, g_free);
}


static void
add_follow_link_menu_item (GtkMenu *menu)
{
  GtkWidget *menu_item = gtk_menu_item_new_with_label(_("Follow link…"));
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(_follow_link_callback), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);
}


static void
_convert_to_path_callback (GtkAction *action, gpointer data)
{
  DDisplay *ddisp = ddisplay_active ();
  GList *selected, *list;
  DiaObjectChange *change_list = NULL;

  if (!ddisp) return;

  /* copy the list before modifying it */
  list = selected = diagram_get_sorted_selected (ddisp->diagram);
  while (list) {
    DiaObject *obj = DIA_OBJECT (list->data);

    if (obj) { /* paranoid */
      DiaObject *path = create_standard_path_from_object (obj);

      if (path) { /* not so paranoid */
        DiaObjectChange *change = object_substitute (obj, path);

        if (!change_list) {
          change_list = dia_object_change_list_new ();
        }

        if (change) {
          dia_object_change_list_add (DIA_OBJECT_CHANGE_LIST (change_list),
                                      change);
        }
      }
    }
    list = g_list_next(list);
  }
  g_list_free (selected);

  if (change_list) {
    dia_object_change_change_new (ddisp->diagram, NULL, change_list);

    diagram_modified(ddisp->diagram);
    diagram_update_extents(ddisp->diagram);

    undo_set_transactionpoint(ddisp->diagram->undo);
    diagram_flush(ddisp->diagram);
  }
}
static void
add_convert_to_path_menu_item (GtkMenu *menu)
{
  GtkWidget *menu_item = gtk_menu_item_new_with_label(_("Convert to Path"));
  g_signal_connect(G_OBJECT(menu_item), "activate", G_CALLBACK(_convert_to_path_callback), NULL);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);
}

static void
_combine_to_path_callback (GtkAction *action, gpointer data)
{
  DDisplay *ddisp = ddisplay_active();
  GList *cut_list;
  DiaObject *obj;
  Diagram *dia;

  if (!ddisp) return;
  dia = ddisp->diagram;

  diagram_selected_break_external(ddisp->diagram);
  cut_list = parent_list_affected(diagram_get_sorted_selected(ddisp->diagram));
  obj = create_standard_path_from_list (cut_list, GPOINTER_TO_INT (data));
  if (obj) {
    /* remove the objects just combined */
    DiaChange *change = dia_delete_objects_change_new_with_children (dia, cut_list);
    dia_change_apply (change, DIA_DIAGRAM_DATA (dia));
    /* add the new object with undo */
    dia_insert_objects_change_new (dia, g_list_prepend (NULL, obj), 1);
    diagram_add_object (dia, obj);
    diagram_select(dia, obj);
    undo_set_transactionpoint (ddisp->diagram->undo);
    object_add_updates(obj, dia);
  } else {
    /* path combination result is empty, this is just a delete */
    DiaChange *change = dia_delete_objects_change_new_with_children (ddisp->diagram, cut_list);
    dia_change_apply (change, DIA_DIAGRAM_DATA (ddisp->diagram));
    undo_set_transactionpoint (ddisp->diagram->undo);
  }
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(dia);
  g_list_free (cut_list);
}
static void
add_combine_to_path_menu_items (GtkMenu *menu)
{
  struct {
    const gchar    *name;
    PathCombineMode mode;
  } _ops[] = {
    { N_("Union"), PATH_UNION },
    { N_("Difference"), PATH_DIFFERENCE },
    { N_("Intersection"), PATH_INTERSECTION },
    { N_("Exclusion"), PATH_EXCLUSION },
  };
  int i;
  for (i = 0; i < G_N_ELEMENTS (_ops); ++i) {
    GtkWidget *menu_item = gtk_menu_item_new_with_label(_(_ops[i].name));
    g_signal_connect(G_OBJECT(menu_item), "activate",
		     G_CALLBACK(_combine_to_path_callback), GINT_TO_POINTER (_ops[i].mode));
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
  }
}

static void
create_object_menu(DiaMenu *dia_menu, gboolean root_menu, gboolean combining)
{
  int i;
  GtkWidget *menu;
  GtkWidget *menu_item;
  gboolean native_convert_to_path = FALSE;

  menu = gtk_menu_new();

  if ( dia_menu->title ) {
    menu_item = gtk_menu_item_new_with_label(gettext(dia_menu->title));
    gtk_widget_set_sensitive(menu_item, FALSE);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
    gtk_widget_show(menu_item);
  }

  /* separator below the menu title */
  menu_item = gtk_menu_item_new();
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), menu_item);
  gtk_widget_show(menu_item);

  for (i=0;i<dia_menu->num_items;i++) {
    DiaMenuItem *item = &dia_menu->items[i];

    /* HACK: rely on object menu naming convention to avoid duplicated menu entry/functionality */
    if (item->text && strcmp (item->text, "Convert to Path") == 0)
      native_convert_to_path = TRUE;

    if (item->active & DIAMENU_TOGGLE) {
      if (item->text)
        menu_item = gtk_check_menu_item_new_with_label(gettext(item->text));
      else
        menu_item = gtk_check_menu_item_new();
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(menu_item),
                                     item->active & DIAMENU_TOGGLE_ON);
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
      g_signal_connect (G_OBJECT (menu_item), "activate",
                        G_CALLBACK (object_menu_item_proxy), &dia_menu->items[i]);
    } else {
      if ( item->callback_data ) {
            /* This menu item is a submenu if it has no callback, but does
             * Have callback_data. In this case the callback_data is a
             * DiaMenu pointer for the submenu. */
        if ( ((DiaMenu*)item->callback_data)->app_data == NULL ) {
              /* Create the popup menu items for the submenu. */
          create_object_menu((DiaMenu*)(item->callback_data), FALSE, FALSE) ;
          gtk_menu_item_set_submenu( GTK_MENU_ITEM (menu_item),
                                     GTK_WIDGET(((DiaMenu*)(item->callback_data))->app_data));
        }
      }
    }
  }

  if (root_menu) {
    /* Finally add a Properties... menu item for objects*/
    add_properties_menu_item(GTK_MENU (menu), i > 0);
    add_follow_link_menu_item(GTK_MENU (menu));
    if (!native_convert_to_path)
      add_convert_to_path_menu_item(GTK_MENU (menu));
    if (combining)
      add_combine_to_path_menu_items (GTK_MENU (menu));
  }

  dia_menu->app_data = menu;
  dia_menu->app_data_free = dia_menu_free;
}

static DiaMenuItem empty_menu_items[] = { {0, } };
static DiaMenu empty_menu = {
  NULL,
  0, /* NOT: sizeof(empty_menu_items)/sizeof(DiaMenuItem), */
  empty_menu_items,
  NULL
};

static void
popup_object_menu(DDisplay *ddisp, GdkEvent *event)
{
  Diagram *diagram;
  DiaObject *obj;
  GtkMenu *menu = NULL;
  DiaMenu *dia_menu = NULL;
  GList *selected_list;
  int i;
  int num_items;
  Point last_clicked_pos;

  diagram = ddisp->diagram;
  if (g_list_length (diagram->data->selected) < 1)
    return;

  selected_list = diagram->data->selected;

  /* Have to have exactly one selected object */
  if (selected_list == NULL) {
    message_error("Selected list is %s while selected_count is %d\n",
		  (selected_list?"long":"empty"), g_list_length (diagram->data->selected));
    return;
  }

  last_clicked_pos = ddisplay_get_clicked_position(ddisp);
  obj = (DiaObject *)g_list_first(selected_list)->data;

  /* Possibly react differently at a handle? */

  /* Get its menu, and remember the # of object-generated items */
  if (    g_list_length (diagram->data->selected) > 1
      ||  obj->ops->get_object_menu == NULL
      || (obj->ops->get_object_menu)(obj, &last_clicked_pos) == NULL) {
    dia_menu = &empty_menu;
    if (dia_menu->title &&
	(0 != strcmp(dia_menu->title,obj->type->name))) {
      dia_menu->app_data_free(dia_menu);
    }
    if (g_list_length (diagram->data->selected) > 1)
      dia_menu->title = _("Selection");
    else
      dia_menu->title = obj->type->name;
    num_items = 0;
  } else {
    dia_menu = (obj->ops->get_object_menu)(obj, &last_clicked_pos);
    num_items = dia_menu ? dia_menu->num_items : 0;
    if (!dia_menu) /* just to be sure */
      dia_menu = &empty_menu;
  }

  if (dia_menu->app_data == NULL) {
    create_object_menu(dia_menu, TRUE, g_list_length (diagram->data->selected) > 1);
    /* append the Input Methods menu, if there is canvas editable text */
    if (obj && focus_get_first_on_object(obj) != NULL) {
      GtkWidget *menuitem = gtk_menu_item_new_with_mnemonic (_("Input _Methods"));
      GtkWidget *submenu = gtk_menu_new ();

      gtk_widget_show (menuitem);
      gtk_menu_item_set_submenu (GTK_MENU_ITEM (menuitem), submenu);
      gtk_menu_shell_append (GTK_MENU_SHELL (dia_menu->app_data), menuitem);

      gtk_im_multicontext_append_menuitems (
        GTK_IM_MULTICONTEXT(ddisp->im_context),
        GTK_MENU_SHELL(submenu));
    }
  }
  /* Update active/nonactive menuitems */
  for (i=0;i<num_items;i++) {
    DiaMenuItem *item = &dia_menu->items[i];
    gtk_widget_set_sensitive(GTK_WIDGET(item->app_data),
			     item->active & DIAMENU_ACTIVE);
    if (item->active & DIAMENU_TOGGLE) {
      g_signal_handlers_block_by_func (G_OBJECT (GTK_CHECK_MENU_ITEM (item->app_data)),
				      G_CALLBACK (object_menu_item_proxy), item);
      gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM(item->app_data),
				      item->active & DIAMENU_TOGGLE_ON);
      g_signal_handlers_unblock_by_func (G_OBJECT (GTK_CHECK_MENU_ITEM(item->app_data)),
					G_CALLBACK (object_menu_item_proxy), item);
    }
  }

  menu = GTK_MENU(dia_menu->app_data);
  /* add the properties menu item to raise the properties from the contextual menu */

  if (event->type == GDK_BUTTON_PRESS)
    gtk_menu_popup_at_pointer(menu, (GdkEvent*)event);
  else if (event->type == GDK_KEY_PRESS)
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 0, ((GdkEventKey *)event)->time);
  else /* warn about unexpected usage of this function */
    g_warning ("Unhandled GdkEvent type=%d", event->type);
}


int
ddisplay_focus_in_event (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  DDisplay *ddisp;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  ddisp = (DDisplay *)data;

  g_assert (event->in == TRUE);

  gtk_im_context_focus_in(GTK_IM_CONTEXT(ddisp->im_context));

  return FALSE;
}


int
ddisplay_focus_out_event (GtkWidget *widget, GdkEventFocus *event, gpointer data)
{
  DDisplay *ddisp;
  int return_val;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (data != NULL, FALSE);

  return_val = FALSE;

  ddisp = (DDisplay *)data;

  g_assert (event->in == FALSE);

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
                                   gtk_widget_get_window(widget));
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
                                     gtk_widget_get_window(widget));
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
handle_key_event (DDisplay   *ddisp,
                  Focus      *focus,
                  guint       keystate,
                  guint       keysym,
                  const char *str,
                  int         strlen)
{
  DiaObject *obj = focus_get_object (focus);
  Point p = obj->position;
  DiaObjectChange *obj_change = NULL;
  gboolean modified;

  object_add_updates (obj, ddisp->diagram);

  modified = (focus->key_event) (focus,
                                 keystate,
                                 keysym,
                                 str,
                                 strlen,
                                 &obj_change);

  /* Make sure object updates its data and its connected: */
  p = obj->position;
  (obj->ops->move)(obj,&p);
  diagram_update_connections_object(ddisp->diagram,obj,TRUE);

  object_add_updates(obj, ddisp->diagram);

  if (modified) {
    if (obj_change != NULL) {
      dia_object_change_change_new (ddisp->diagram, obj, obj_change);
      undo_set_transactionpoint(ddisp->diagram->undo);
    }
    diagram_update_extents(ddisp->diagram);
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

  Focus *focus = get_active_focus((DiagramData *) ddisp->diagram);

  ddisplay_im_context_preedit_reset(ddisp, focus);

  if (focus != NULL)
    handle_key_event(ddisp, focus, 0, 0, str, g_utf8_strlen(str,-1));
}


void
ddisplay_im_context_preedit_changed (GtkIMContext *context,
                                     DDisplay     *ddisp)
{
  int cursor_pos;
  Focus *focus = get_active_focus((DiagramData *) ddisp->diagram);

  ddisplay_im_context_preedit_reset(ddisp, focus);

  gtk_im_context_get_preedit_string(context, &ddisp->preedit_string,
                                    &ddisp->preedit_attrs, &cursor_pos);
  if (ddisp->preedit_string != NULL) {
    if (focus != NULL) {
      handle_key_event(ddisp, focus, 0, 0, ddisp->preedit_string,
                       g_utf8_strlen(ddisp->preedit_string,-1));
    } else {
      ddisplay_im_context_preedit_reset(ddisp, focus);
    }
  }
}


static void
_scroll_page (DDisplay *ddisp, Direction dir)
{
  Point delta = {0, 0};

  switch (dir) {
    case DIR_LEFT:
      delta.x = ddisp->diagram->data->paper.width * ddisp->diagram->data->paper.scaling;
      break;
    case DIR_RIGHT:
      delta.x = -ddisp->diagram->data->paper.width * ddisp->diagram->data->paper.scaling;
      break;
    case DIR_UP:
      delta.y = -ddisp->diagram->data->paper.height * ddisp->diagram->data->paper.scaling;
      break;
    case DIR_DOWN:
      delta.y = ddisp->diagram->data->paper.height * ddisp->diagram->data->paper.scaling;
      break;
    default:
      g_return_if_reached ();
  }
  ddisplay_scroll (ddisp, &delta);
  ddisplay_flush (ddisp);
}


static void
_scroll_step (DDisplay *ddisp, guint keyval)
{
  switch (keyval) {
  case GDK_KEY_Up :
    ddisplay_scroll_up(ddisp);
    ddisplay_flush(ddisp);
    break;
  case GDK_KEY_Down:
    ddisplay_scroll_down(ddisp);
    ddisplay_flush(ddisp);
    break;
  case GDK_KEY_Left:
    ddisplay_scroll_left(ddisp);
    ddisplay_flush(ddisp);
    break;
  case GDK_KEY_Right:
    ddisplay_scroll_right(ddisp);
    ddisplay_flush(ddisp);
    break;
  default :
    g_assert_not_reached ();
  }
}


/*
 * Cleanup/Remove Timeout Handler for Button Press and Hold
 */
static void
hold_remove_handler (void)
{
  if (hold_data.tag != 0) {
    g_source_remove (hold_data.tag);
    hold_data.tag = 0;
    gdk_event_free (hold_data.event);
  }
}


/*
 * Timeout Handler for Button Press and Hold
 * If this function is called, then the button must still be down,
 * indicating that the user has pressed and held the button, but not moved
 * the pointer (mouse).
 * Dynamic data is cleaned up in ddisplay_canvas_events()
 */
static gboolean
hold_timeout_handler(gpointer data)
{
  if (active_tool->button_hold_func)
    (*active_tool->button_hold_func) (active_tool, (GdkEventButton *)(hold_data.event), hold_data.ddisp);
  hold_remove_handler();
  return FALSE;
}


/**
 * ddisplay_canvas_events:
 * @canvas: the canvas
 * @event: the #GdkEvent received
 * @ddisp: the #DDisplay
 *
 * Main input handler for a diagram canvas.
 *
 * Since: dawn-of-time
 */
int
ddisplay_canvas_events (GtkWidget *canvas,
                        GdkEvent  *event,
                        DDisplay  *ddisp)
{
  GdkEventMotion *mevent;
  GdkEventButton *bevent;
  GdkEventKey *kevent;
  int tx, ty;
  GdkModifierType tmask;
  guint state = 0;
  Focus *focus;
  DiaObject *obj;
  DiaRectangle *visible;
  Point middle;
  int return_val;
  int key_handled;
  int im_context_used;
  gdouble x, y;
  Point delta = { 0.0, 0.0 };
  GdkScrollDirection direction;
  static gboolean moving = FALSE;

  return_val = FALSE;

  if (!gtk_widget_get_window (canvas)) {
    return FALSE;
  }

  switch (event->type) {
    case GDK_SCROLL:

      tmask = 0;
      gdk_event_get_state(event, &tmask);
      gdk_event_get_coords(event, &x, &y);

      if (gdk_event_get_scroll_direction (event, &direction)) {
        switch (direction) {
        case GDK_SCROLL_UP:
          if (tmask & GDK_SHIFT_MASK) {
            ddisplay_scroll_left(ddisp);
          } else if (tmask & GDK_CONTROL_MASK) {
            ddisplay_untransform_coords(ddisp, (int)x, (int)y, &middle.x, &middle.y);
            /* zooming with the wheel in small steps 1^(1/8) */
            ddisplay_zoom_centered(ddisp, &middle, 1.090508);
          } else {
            ddisplay_scroll_up (ddisp);
          }
          break;
        case GDK_SCROLL_DOWN:
          if (tmask & GDK_SHIFT_MASK) {
            ddisplay_scroll_right (ddisp);
          } else if (tmask & GDK_CONTROL_MASK) {
            ddisplay_untransform_coords (ddisp,
                                         (int) x,
                                         (int) y,
                                         &middle.x,
                                         &middle.y);
            /* zooming with the wheel in small steps 1/(1^(1/8)) */
            ddisplay_zoom_centered (ddisp, &middle, 0.917004);
          } else {
            ddisplay_scroll_down (ddisp);
          }
          break;
        case GDK_SCROLL_LEFT:
          ddisplay_scroll_left (ddisp);
          break;
        case GDK_SCROLL_RIGHT:
          ddisplay_scroll_right (ddisp);
          break;
        case GDK_SCROLL_SMOOTH:
          /* shouldn't happen with gdk_event_get_scroll_direction() */
          break;
        default:
          break;
        }
      } else if (gdk_event_get_scroll_deltas (event, &delta.x, &delta.y)) {
        if (tmask & GDK_CONTROL_MASK) {
          ddisplay_untransform_coords (ddisp, (int)x, (int)y, &middle.x, &middle.y);
          /* zooming with the wheel in small steps 1^(1/8) */
          if (delta.y > 0) {
            ddisplay_zoom_centered (ddisp, &middle, 0.917004);
          } else {
            ddisplay_zoom_centered (ddisp, &middle, 1.090508);
          }
        } else {
          ddisplay_scroll (ddisp, &delta);
        }
      }

      ddisplay_flush (ddisp);
      break;

    case GDK_FOCUS_CHANGE: {
        GdkEventFocus *focus_evt = (GdkEventFocus*) event;
        hold_remove_handler ();
        if (focus_evt->in) {
          display_set_active (ddisp);
          ddisplay_do_update_menu_sensitivity (ddisp);
        }
        break;
      }
    case GDK_2BUTTON_PRESS:
      display_set_active (ddisp);
      hold_remove_handler ();
      bevent = (GdkEventButton *) event;

      switch (bevent->button) {
        case 1:
          if (transient_tool) {
            break;
          }
          if (active_tool->double_click_func) {
            (*active_tool->double_click_func) (active_tool, bevent, ddisp);
          }
          break;
        case 2:
        case 3:
        default:
          break;
      }
      break;

    case GDK_BUTTON_PRESS:
      display_set_active (ddisp);
      bevent = (GdkEventButton *) event;

      ddisplay_set_clicked_point (ddisp, bevent->x, bevent->y);

      switch (bevent->button) {
        case 1:
          if (transient_tool) {
            break;
          }
          /* get the focus again, may be lost by zoom combo */
          moving = TRUE;
          gtk_widget_grab_focus (canvas);
          if (active_tool->button_press_func) {
            (*active_tool->button_press_func) (active_tool, bevent, ddisp);
          }

          /* Detect user holding down the button.
          * Set timeout for 1sec. If timeout is called, user must still
          * be holding. If user releases button, remove timeout. */
          hold_data.event = gdk_event_copy (event); /* need to free later */
          hold_data.ddisp = ddisp;
          hold_data.tag = g_timeout_add (1000, hold_timeout_handler, NULL);
          break;
        case 2:
          if (ddisp->menu_bar == NULL && !is_integrated_ui ()) {
            popup_object_menu (ddisp, event);
          } else if (!transient_tool) {
            tool_reset ();
            gtk_widget_grab_focus (canvas);
            transient_tool = create_scroll_tool ();
            (*transient_tool->button_press_func) (transient_tool, bevent, ddisp);
          }
          break;

        case 3:
          if (transient_tool) {
            break;
          }
          if (ddisp->menu_bar == NULL) {
            if (bevent->state & GDK_CONTROL_MASK || is_integrated_ui ()) {
                  /* for two button mouse users ... */
              popup_object_menu (ddisp, event);
              break;
            }
            ddisplay_popup_menu (ddisp, bevent);
            break;
          } else {
            popup_object_menu (ddisp, event);
            break;
          }
        default:
          break;
      }
      break;

    case GDK_BUTTON_RELEASE:
      display_set_active (ddisp);
      bevent = (GdkEventButton *) event;

      switch (bevent->button) {
        case 1:
          if (moving) {
            moving = FALSE;
          }
          if (active_tool->button_release_func) {
            (*active_tool->button_release_func) (active_tool,
                                                 bevent, ddisp);
          }
          /* Button Press and Hold - remove handler then deallocate memory */
          hold_remove_handler ();
          break;
        case 2:
          if (transient_tool) {
            (*transient_tool->button_release_func) (transient_tool,
                                                    bevent,
                                                    ddisp);

            tool_free (transient_tool);
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
      {
        GdkWindow *window = gtk_widget_get_window(canvas);
        GdkDisplay *display = gdk_window_get_display(window);
        GdkSeat *seat = gdk_display_get_default_seat (display);
        GdkDevice *device = gdk_seat_get_pointer (seat);

        gdk_window_get_device_position (window, device, &tx, &ty, &tmask);
      }
      hold_remove_handler();

      mevent = (GdkEventMotion *) event;

      if (mevent->is_hint) {
        mevent->x = tx;
        mevent->y = ty;
        mevent->state = tmask;
        mevent->is_hint = FALSE;
      }
      if (transient_tool && (*transient_tool->motion_func)) {
        (*transient_tool->motion_func) (transient_tool, mevent, ddisp);
      } else if (active_tool->motion_func) {
        (*active_tool->motion_func) (active_tool, mevent, ddisp);
      }
      break;

    case GDK_KEY_PRESS:
      if (moving) {
        /*Disable Keyboard accels whilst dragging an object*/
        break;
      }
      display_set_active (ddisp);
      kevent = (GdkEventKey *) event;
      state = kevent->state;
      key_handled = FALSE;
      im_context_used = FALSE;
      #if 0
      printf("Key input %d in state %d\n", kevent->keyval, textedit_mode(ddisp));
      #endif
      focus = get_active_focus ((DiagramData *) ddisp->diagram);
      if (focus != NULL) {
        /* Keys goes to the active focus. */
        obj = focus_get_object (focus);
        if (diagram_is_selected (ddisp->diagram, obj)) {

          if (!gtk_im_context_filter_keypress (
              GTK_IM_CONTEXT (ddisp->im_context), kevent)) {

            switch (kevent->keyval) {
              case GDK_KEY_Tab:
                focus = textedit_move_focus (ddisp, focus,
                                             (state & GDK_SHIFT_MASK) == 0);
                obj = focus_get_object (focus);
                break;
              case GDK_KEY_Escape:
                textedit_deactivate_focus ();
                tool_reset ();
                ddisplay_do_update_menu_sensitivity (ddisp);
                break;
              default:
                /*! key event not swallowed by the input method ? */
                handle_key_event (ddisp, focus,
                                  kevent->state, kevent->keyval,
                                  kevent->string, kevent->length);

                diagram_flush(ddisp->diagram);
            }
          }
          return_val = key_handled = im_context_used = TRUE;
        }
      }

      #if 0
      /* modifier requirement added 2004-07-17, IMO reenabling unmodified keys here
       * shouldn't break im_context handling. How to test?    --hb
       */
      if (!key_handled && (state & (GDK_CONTROL_MASK | GDK_MOD1_MASK))) {
      #else
      if (!key_handled) {
      #endif
        /* IM doesn't need receive keys, take care of it ourselves. */
        return_val = TRUE;

        switch (kevent->keyval) {
          case GDK_KEY_Home:
          case GDK_KEY_KP_Home:
            /* match upper left corner of the diagram with it's view */
            ddisplay_set_origo (ddisp,
                                ddisp->diagram->data->extents.left,
                                ddisp->diagram->data->extents.top);
            ddisplay_update_scrollbars (ddisp);
            ddisplay_add_update_all (ddisp);
            break;
          case GDK_KEY_End:
          case GDK_KEY_KP_End:
            /* match lower right corner of the diagram with it's view */
            visible = &ddisp->visible;
            ddisplay_set_origo (ddisp,
                                ddisp->diagram->data->extents.right - (visible->right - visible->left),
                                ddisp->diagram->data->extents.bottom - (visible->bottom - visible->top));
            ddisplay_update_scrollbars (ddisp);
            ddisplay_add_update_all (ddisp);
            break;
          case GDK_KEY_Page_Up:
          case GDK_KEY_KP_Page_Up:
            _scroll_page (ddisp, !(state & GDK_CONTROL_MASK) ? DIR_UP : DIR_LEFT);
            break;
          case GDK_KEY_Page_Down :
          case GDK_KEY_KP_Page_Down:
            _scroll_page (ddisp, !(state & GDK_CONTROL_MASK) ? DIR_DOWN : DIR_RIGHT);
            break;
          case GDK_KEY_Up:
          case GDK_KEY_Down:
          case GDK_KEY_Left:
          case GDK_KEY_Right:
            if (g_list_length (ddisp->diagram->data->selected) > 0) {
              Diagram *dia = ddisp->diagram;
              GList *objects = dia->data->selected;
              Direction dir = GDK_KEY_Up == kevent->keyval ? DIR_UP :
                  GDK_KEY_Down == kevent->keyval ? DIR_DOWN :
                  GDK_KEY_Right == kevent->keyval ? DIR_RIGHT : DIR_LEFT;
              object_add_updates_list (objects, dia);
              object_list_nudge (objects, dia, dir,
                                  /* step one pixel or more with <ctrl> */
                                  ddisplay_untransform_length (ddisp, (state & GDK_SHIFT_MASK) ? 10 : 1));
              diagram_update_connections_selection (dia);
              object_add_updates_list (objects, dia);
              diagram_modified (dia);
              diagram_flush (dia);

              undo_set_transactionpoint (dia->undo);
            } else {
              _scroll_step (ddisp, kevent->keyval);
            }
            break;
          case GDK_KEY_KP_Add:
          case GDK_KEY_plus:
          case GDK_KEY_equal:
            ddisplay_zoom_middle (ddisp, M_SQRT2);
            break;
          case GDK_KEY_KP_Subtract:
          case GDK_KEY_minus:
            ddisplay_zoom_middle (ddisp, M_SQRT1_2);
            break;
          case GDK_KEY_Shift_L:
          case GDK_KEY_Shift_R:
            if (active_tool->type == MAGNIFY_TOOL)
              set_zoom_out (active_tool);
            break;
          case GDK_KEY_Escape:
            view_unfullscreen ();
            break;
          case GDK_KEY_F2:
          case GDK_KEY_Return:
            gtk_action_activate (menus_get_action ("ToolsTextedit"));
            break;
          case GDK_KEY_Menu:
            popup_object_menu (ddisp, event);
            break;
          default:
            if (kevent->string && kevent->keyval == ' ') {
              tool_select_former();
            } else if ((kevent->state & (GDK_MOD1_MASK|GDK_CONTROL_MASK)) == 0 &&
              kevent->length != 0) {
              /* Find first editable? */
            }
        }
      }

      if (!im_context_used) {
        gtk_im_context_reset (GTK_IM_CONTEXT (ddisp->im_context));
      }

      break;

    case GDK_KEY_RELEASE:
      kevent = (GdkEventKey *) event;
      if (gtk_im_context_filter_keypress (GTK_IM_CONTEXT (ddisp->im_context),
                                          kevent)) {
        return_val = TRUE;
      } else {
        switch (kevent->keyval) {
          case GDK_KEY_Shift_L:
          case GDK_KEY_Shift_R:
            if (active_tool->type == MAGNIFY_TOOL) {
              set_zoom_in (active_tool);
            }
            break;
          default:
            break;
        }
      }
      break;

    case GDK_NOTHING:
    case GDK_DELETE:
    case GDK_DESTROY:
    case GDK_EXPOSE:
    case GDK_3BUTTON_PRESS:
    case GDK_ENTER_NOTIFY:
    case GDK_LEAVE_NOTIFY:
    case GDK_CONFIGURE:
    case GDK_MAP:
    case GDK_UNMAP:
    case GDK_PROPERTY_NOTIFY:
    case GDK_SELECTION_CLEAR:
    case GDK_SELECTION_REQUEST:
    case GDK_SELECTION_NOTIFY:
    case GDK_PROXIMITY_IN:
    case GDK_PROXIMITY_OUT:
    case GDK_DRAG_ENTER:
    case GDK_DRAG_LEAVE:
    case GDK_DRAG_MOTION:
    case GDK_DRAG_STATUS:
    case GDK_DROP_START:
    case GDK_DROP_FINISHED:
    case GDK_CLIENT_EVENT:
    case GDK_VISIBILITY_NOTIFY:
    case GDK_WINDOW_STATE:
    case GDK_SETTING:
    case GDK_OWNER_CHANGE:
    case GDK_GRAB_BROKEN:
    case GDK_DAMAGE:
    case GDK_EVENT_LAST:
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_UPDATE:
    case GDK_TOUCH_END:
    case GDK_TOUCH_CANCEL:
    case GDK_TOUCHPAD_SWIPE:
    case GDK_TOUCHPAD_PINCH:
    case GDK_PAD_BUTTON_PRESS:
    case GDK_PAD_BUTTON_RELEASE:
    case GDK_PAD_RING:
    case GDK_PAD_STRIP:
    case GDK_PAD_GROUP_MODE:
    default:
      break;
  }

  return return_val;
}


int
ddisplay_hsb_update (GtkAdjustment *adjustment,
                     DDisplay      *ddisp)
{
  ddisplay_set_origo (ddisp, gtk_adjustment_get_value (adjustment), ddisp->origo.y);
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);
  return FALSE;
}


int
ddisplay_vsb_update (GtkAdjustment *adjustment,
                     DDisplay      *ddisp)
{
  ddisplay_set_origo (ddisp, ddisp->origo.x, gtk_adjustment_get_value (adjustment));
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);
  return FALSE;
}


int
ddisplay_delete (GtkWidget *widget, GdkEvent  *event, gpointer data)
{
  DDisplay *ddisp;

  ddisp = (DDisplay *)data;

  ddisplay_close (ddisp);
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
ddisplay_drop_object (DDisplay      *ddisp,
                      int            x,
                      int            y,
                      DiaObjectType *otype,
                      gpointer       user_data)
{
  Point droppoint;
  Point droppoint_orig;
  Handle *handle1, *handle2;
  DiaObject *obj, *p_obj;
  GList *list;
  double click_distance;
  gboolean avoid_reset;

  ddisplay_untransform_coords(ddisp, x, y, &droppoint.x, &droppoint.y);

  /* save it before snap_to_grid modifies it */
  droppoint_orig = droppoint;

  snap_to_grid(ddisp, &droppoint.x, &droppoint.y);

  obj = dia_object_default_create (otype, &droppoint,
                                   user_data,
                                   &handle1, &handle2);

  if (!obj)
    return NULL;

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
    DiaRectangle p_ext, c_ext;
    double parent_height, child_height, parent_width, child_width;
    double vadjust = 0.0, hadjust = 0.0;
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
      message_error (_("The object you dropped cannot fit into its parent. \nEither expand the parent object, or drop the object elsewhere."));
      obj->parent->children = g_list_remove(obj->parent->children, obj);
      obj->ops->destroy (obj);
      g_clear_pointer (&obj, g_free);
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
  /* if we entered textedit don't reset */
  avoid_reset = textedit_activate_object(ddisp, obj, NULL);

  /* Connect first handle if possible: */
  if ((handle1 != NULL) &&
      (handle1->connect_type != HANDLE_NONCONNECTABLE)) {
    object_connect_display(ddisp, obj, handle1, FALSE);
  }
  object_add_updates(obj, ddisp->diagram);
  ddisplay_do_update_menu_sensitivity(ddisp);
  diagram_flush(ddisp->diagram);

  list = g_list_prepend(NULL, obj);
  dia_insert_objects_change_new (ddisp->diagram, list, 1);
  diagram_update_extents(ddisp->diagram);

  undo_set_transactionpoint(ddisp->diagram->undo);
  diagram_modified(ddisp->diagram);
  if (!avoid_reset && prefs.reset_tools_after_create)
    tool_reset();
  else /* cant use gtk_action_activate (menus_get_action ("ToolsTextedit")); - it's disabled */
    tool_select (TEXTEDIT_TOOL, NULL, NULL, NULL, 0);

  return obj;
}
