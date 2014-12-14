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

#include <config.h>

#ifdef HAVE_GNOME
#undef GTK_DISABLE_DEPRECATED /* gnome */
#include <gnome.h>
#else
#include <gtk/gtk.h>
#endif
/* this file should be the only place to include it */
#ifdef HAVE_MAC_INTEGRATION
#include <gtkosxapplication.h>
#endif

#include <stdio.h>
#include <string.h>

#include "diagram.h"
#include "object.h"
#include "layer_dialog.h"
#include "interface.h"
#include "display.h"
#include "disp_callbacks.h"
#include "menus.h"
#include "toolbox.h"
#include "commands.h"
#include "dia_dirs.h"
#include "intl.h"
#include "navigation.h"
#include "persistence.h"
#include "widgets.h"
#include "message.h"
#include "ruler.h"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "dia-app-icons.h"

static void
dia_dnd_file_drag_data_received (GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             info,
                                 guint             time,
				 DDisplay         *ddisp)
{
#if GTK_CHECK_VERSION(2,22,0)
  switch (gdk_drag_context_get_selected_action(context))
#else
  switch (context->action)
#endif
    {
    case GDK_ACTION_DEFAULT:
    case GDK_ACTION_COPY:
    case GDK_ACTION_MOVE:
    case GDK_ACTION_LINK:
    case GDK_ACTION_ASK:
    default:
      {
        Diagram *diagram = NULL;
        gchar *sPath = NULL, *pFrom, *pTo; 

        pFrom = strstr((gchar *) gtk_selection_data_get_data(data), "file:");
        while (pFrom) {
          GError *error = NULL;

          pTo = pFrom;
          while (*pTo != 0 && *pTo != 0xd && *pTo != 0xa) pTo ++;
          sPath = g_strndup(pFrom, pTo - pFrom);

          /* format changed with Gtk+2.0, use conversion */
          pFrom = g_filename_from_uri (sPath, NULL, &error);
	  if (!ddisp)
            diagram = diagram_load (pFrom, NULL);
	  else {
	    diagram = ddisp->diagram;
	    if (!diagram_load_into (diagram, pFrom, NULL)) {
	      /* the import filter is supposed to show the error message */
              gtk_drag_finish (context, TRUE, FALSE, time);
	      break;
	    }
	  }

          g_free (pFrom);
          g_free(sPath);

          if (diagram != NULL) {
            diagram_update_extents(diagram);
            layer_dialog_set_diagram(diagram);
            
	    if (diagram->displays == NULL) {
	      new_display(diagram);
	    }
          }

          pFrom = strstr(pTo, "file:");
        } /* while */
        gtk_drag_finish (context, TRUE, FALSE, time);
      }
      break;
    }
  return;
}

static GtkWidget *toolbox_shell = NULL;

static struct 
{
    GtkWindow    * main_window;
    GtkToolbar   * toolbar;
    GtkNotebook  * diagram_notebook;
    GtkStatusbar * statusbar;
    GtkWidget    * layer_view;
} ui;

/** 
 * Used to determine if the current user interface is the integrated interface or 
 * the distributed interface.  This cannot presently be determined by the preferences
 * setting because changing that setting at run time does not change the interface.
 * @return Non-zero if the integrated interface is present, else zero.
 */
int is_integrated_ui (void)
{
  return ui.main_window == NULL? 0 : 1;
}

static void
grid_toggle_snap(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;
  ddisplay_set_snap_to_grid(ddisp, 
			    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

static void
interface_toggle_mainpoint_magnetism(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;
  ddisplay_set_snap_to_objects(ddisp,
				   gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  ddisplay_add_update_all(ddisp);
  ddisplay_flush(ddisp);
}

static gint
origin_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;

  display_set_active(ddisp);
  ddisplay_popup_menu(ddisp, event);

  /* stop the signal emission so the button doesn't grab the
   * pointer from us */
  g_signal_stop_emission_by_name (G_OBJECT(widget), "button_press_event");

  return FALSE;
}

void
view_zoom_set (float factor)
{
  DDisplay *ddisp;
  real scale;

  ddisp = ddisplay_active();
  if (!ddisp) return;

  scale = ((real) factor)/1000.0 * DDISPLAY_NORMAL_ZOOM;

  ddisplay_zoom_middle(ddisp, scale / ddisp->zoom_factor);
}

static void
zoom_activate_callback(GtkWidget *item, gpointer user_data) 
{
  DDisplay *ddisp = (DDisplay *)user_data;
  const gchar *zoom_text =
      gtk_entry_get_text(GTK_ENTRY(g_object_get_data(G_OBJECT(ddisp->zoom_status), "user_data")));
  float zoom_amount, magnify;
  gchar *zoomamount = g_object_get_data(G_OBJECT(item), "zoomamount");
  if (zoomamount != NULL) {
    zoom_text = zoomamount;
  }

  if (sscanf(zoom_text, "%f", &zoom_amount) == 1) {
    /* Set limits to avoid crashes, see bug #483384 */
    if (zoom_amount < .1) {
      zoom_amount = .1;
    } else if (zoom_amount > 1e4) {
      zoom_amount = 1e4;
    }
    zoomamount = g_strdup_printf("%f%%\n", zoom_amount);
    gtk_entry_set_text(GTK_ENTRY(g_object_get_data(G_OBJECT(ddisp->zoom_status), "user_data")), zoomamount);
    g_free(zoomamount);
    magnify = (zoom_amount*DDISPLAY_NORMAL_ZOOM/100.0)/ddisp->zoom_factor;
    if (fabs(magnify - 1.0) > 0.000001) {
      ddisplay_zoom_middle(ddisp, magnify);
    }
  }
}

static void
zoom_add_zoom_amount(GtkWidget *menu, gchar *text, DDisplay *ddisp) 
{
  GtkWidget *menuitem = gtk_menu_item_new_with_label(text);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  g_signal_connect(G_OBJECT(menuitem), 
		     "activate", G_CALLBACK(zoom_activate_callback),
		     ddisp);
  g_object_set_data(G_OBJECT(menuitem), "zoomamount", text);
}

static void
zoom_popup_menu(GtkWidget *button, GdkEventButton *event, gpointer user_data) {
  /* display_set_active(ddisp); */
  /* ddisplay_popup_menu(ddisp, event); */

  GtkMenu *menu = GTK_MENU(user_data);

  gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 
		 event->button, event->time);
  /* stop the signal emission so the button doesn't grab the
   * pointer from us */
  g_signal_stop_emission_by_name (G_OBJECT(button), "button_press_event");
}

static GtkWidget*
create_zoom_widget(DDisplay *ddisp) { 
  GtkWidget *combo;
  GtkWidget *entry;
  GtkWidget *menu;
  GtkWidget *button;
  GtkWidget *arrow;

  combo = gtk_hbox_new(FALSE, 0);
  entry = gtk_entry_new();
  g_signal_connect (G_OBJECT (entry), "activate",
		    G_CALLBACK(zoom_activate_callback),
		      ddisp);
  gtk_box_pack_start(GTK_BOX(combo), entry, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT(combo), "user_data", entry);
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 8);
  gtk_widget_show(entry);

  button = gtk_button_new();
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus (GTK_WIDGET (button), FALSE);
#else
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
#endif
  arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  gtk_box_pack_start(GTK_BOX(combo), button, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT(combo), "user_data", entry);
  gtk_widget_show_all(button);

  menu = gtk_menu_new();
  zoom_add_zoom_amount(menu, "800%", ddisp);
  zoom_add_zoom_amount(menu, "400%", ddisp);
  zoom_add_zoom_amount(menu, "200%", ddisp);
  zoom_add_zoom_amount(menu, "100%", ddisp);
  zoom_add_zoom_amount(menu, "50%", ddisp);
  zoom_add_zoom_amount(menu, "25%", ddisp);
  zoom_add_zoom_amount(menu, "12%", ddisp);

  gtk_widget_show_all(menu);

  g_signal_connect (G_OBJECT (button), "button_press_event",
		    G_CALLBACK(zoom_popup_menu),
		      menu);

  return combo;
}

static gboolean
display_drop_callback(GtkWidget *widget, GdkDragContext *context,
		      gint x, gint y, guint time)
{
  if (gtk_drag_get_source_widget(context) != NULL) {
    /* we only accept drops from the same instance of the application,
     * as the drag data is a pointer in our address space */
    return TRUE;
  }
  gtk_drag_finish (context, FALSE, FALSE, time);
  return FALSE;
}

static void
display_data_received_callback (GtkWidget *widget, 
				GdkDragContext *context,
				gint x, 
				gint y, 
				GtkSelectionData *data,
				guint info, 
				guint time, 
				DDisplay *ddisp)
{
  if (gtk_selection_data_get_format(data) == 8 &&
      gtk_selection_data_get_length(data) == sizeof(ToolButtonData *) &&
      gtk_drag_get_source_widget(context) != NULL) {
    ToolButtonData *tooldata = *(ToolButtonData **)gtk_selection_data_get_data(data);
    /* g_message("Tool drop %s at (%d, %d)", (gchar *)tooldata->extra_data, x, y);*/
    ddisplay_drop_object(ddisp, x, y,
			 object_get_type((gchar *)tooldata->extra_data),
			 tooldata->user_data);

    gtk_drag_finish (context, TRUE, FALSE, time);
  } else {
    dia_dnd_file_drag_data_received (widget, context, x, y, data, info, time, ddisp);
  }
  /* ensure the right window has the focus for text editing */
  gtk_window_present(GTK_WINDOW(ddisp->shell));
}

/**
 * @param button The notebook close button.
 * @param user_data Container widget (e.g. VBox).
 */
void
close_notebook_page_callback (GtkButton *button,
                              gpointer   user_data)
{
  GtkBox      *page     = user_data;
  DDisplay    *ddisp    = g_object_get_data (G_OBJECT (page), "DDisplay");

  /* When the page widget is destroyed it removes itself from the notebook */
  ddisplay_close (ddisp);
}

/*!
 * Called when the widget's window "size, position or stacking"
 * changes. Needs GDK_STRUCTURE_MASK set.
 */
static gboolean
canvas_configure_event (GtkWidget         *widget,
			GdkEventConfigure *cevent,
			DDisplay          *ddisp)
{
  gboolean new_size = FALSE;
  int width, height;

  g_return_val_if_fail (widget == ddisp->canvas, FALSE);


  if (ddisp->renderer) {
    width = dia_renderer_get_width_pixels (ddisp->renderer);
    height = dia_renderer_get_height_pixels (ddisp->renderer);
  } else {
    /* We can continue even without a renderer here because
     * ddisplay_resize_canvas () does the setup for us.
     */
    width = height = 0;
  }

  /* Only do this when size is really changing */
  if (width != cevent->width || height != cevent->height) {
    g_print ("Canvas size change...\n");
    ddisplay_resize_canvas (ddisp, cevent->width, cevent->height);
    ddisplay_update_scrollbars(ddisp);
    /* on resize stop further propagation - does not help */
    new_size = TRUE;
  }

  /* If the UI is not integrated, resizing should set the resized
   * window as active.  With integrated UI, there is only one window.
   */
  if (is_integrated_ui () == 0)
    display_set_active(ddisp);

  /* continue propagation with FALSE */
  return new_size;
}

/* Got when an area previously obscured need to be redrawn.
 * Needs GDK_EXPOSURE_MASK.
 * Gone with gtk+-3.0 or better replaced by "draw".
 */
static gboolean
canvas_expose_event (GtkWidget      *widget,
		     GdkEventExpose *event,
		     DDisplay       *ddisp)
{
  ddisplay_add_display_area (ddisp,
			     event->area.x, event->area.y,
			     event->area.x + event->area.width,
			     event->area.y + event->area.height);
  ddisplay_flush(ddisp);
  return FALSE;
}

static GtkWidget *
create_canvas (DDisplay *ddisp)
{
  GtkWidget *canvas = gtk_drawing_area_new();

  /* Dia's canvas does it's double buffering alone so switch off GTK's */
  gtk_widget_set_double_buffered (canvas, FALSE);

  gtk_widget_set_events (canvas,
			 GDK_EXPOSURE_MASK | 
			 GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK |
			 GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK | 
			 GDK_STRUCTURE_MASK | GDK_ENTER_NOTIFY_MASK |
			 GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  g_signal_connect (G_OBJECT (canvas), "configure-event",
		    G_CALLBACK (canvas_configure_event), ddisp);
  g_signal_connect (G_OBJECT (canvas), "expose-event",
		    G_CALLBACK (canvas_expose_event), ddisp);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus (canvas, TRUE);
#else
  GTK_WIDGET_SET_FLAGS (canvas, GTK_CAN_FOCUS);
#endif
  g_signal_connect (G_OBJECT (canvas), "event",
                    G_CALLBACK(ddisplay_canvas_events),
                    ddisp);

  canvas_setup_drag_dest (canvas);
  g_signal_connect (G_OBJECT (canvas), "drag_drop",
		    G_CALLBACK(display_drop_callback), NULL);
  g_signal_connect (G_OBJECT (canvas), "drag_data_received",
		    G_CALLBACK(display_data_received_callback), ddisp);
  g_object_set_data (G_OBJECT (canvas), "user_data", (gpointer) ddisp);

  return canvas;
}

/* Shared helper functions for both UI cases
 */
static void
_ddisplay_setup_rulers (DDisplay *ddisp, GtkWidget *shell, GtkWidget *table)
{
  ddisp->hrule = dia_ruler_new (GTK_ORIENTATION_HORIZONTAL, shell, ddisp);
  ddisp->vrule = dia_ruler_new (GTK_ORIENTATION_VERTICAL, shell, ddisp);

  /* harder to change position in the table, but we did not do it for years ;) */
  gtk_table_attach (GTK_TABLE (table), ddisp->hrule, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->vrule, 0, 1, 1, 2,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
}
static void
_ddisplay_setup_events (DDisplay *ddisp, GtkWidget *shell)
{
  gtk_widget_set_events (shell,
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_FOCUS_CHANGE_MASK);

  g_signal_connect (G_OBJECT (shell), "focus_out_event",
		    G_CALLBACK (ddisplay_focus_out_event), ddisp);
  g_signal_connect (G_OBJECT (shell), "focus_in_event",
		    G_CALLBACK (ddisplay_focus_in_event), ddisp);
  g_signal_connect (G_OBJECT (shell), "realize",
		    G_CALLBACK (ddisplay_realize), ddisp);
  g_signal_connect (G_OBJECT (shell), "unrealize",
		    G_CALLBACK (ddisplay_unrealize), ddisp);
}
static void
_ddisplay_setup_scrollbars (DDisplay *ddisp, GtkWidget *table, int width, int height)
{
  /*  The adjustment datums  */
  ddisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, (width-1)/4, width-1));
  ddisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, (height-1)/4, height-1));

  ddisp->hsb = gtk_hscrollbar_new (ddisp->hsbdata);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus (ddisp->hsb, FALSE);
#else
  GTK_WIDGET_UNSET_FLAGS (ddisp->hsb, GTK_CAN_FOCUS);
#endif
  ddisp->vsb = gtk_vscrollbar_new (ddisp->vsbdata);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_focus (ddisp->vsb, FALSE);
#else
  GTK_WIDGET_UNSET_FLAGS (ddisp->vsb, GTK_CAN_FOCUS);
#endif

  /*  set up the scrollbar observers  */
  g_signal_connect (G_OBJECT (ddisp->hsbdata), "value_changed",
		    G_CALLBACK(ddisplay_hsb_update), ddisp);
  g_signal_connect (G_OBJECT (ddisp->vsbdata), "value_changed",
		    G_CALLBACK(ddisplay_vsb_update), ddisp);

  /* harder to change position in the table, but we did not do it for years ;) */
  gtk_table_attach (GTK_TABLE (table), ddisp->hsb, 0, 2, 2, 3,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->vsb, 2, 3, 0, 2,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  gtk_widget_show (ddisp->hsb);
  gtk_widget_show (ddisp->vsb);
}
static void
_ddisplay_setup_navigation (DDisplay *ddisp, GtkWidget *table, gboolean top_left)
{
  GtkWidget *navigation_button;

  /*  Popup button between scrollbars for navigation window  */
  navigation_button = navigation_popup_new(ddisp);
  gtk_widget_set_tooltip_text(navigation_button,
                       _("Pops up the Navigation window."));
  gtk_widget_show(navigation_button);

  /* harder to change position in the table, but we did not do it for years ;) */
  if (top_left)
    gtk_table_attach (GTK_TABLE (table), navigation_button, 0, 1, 0, 1,
                      GTK_FILL, GTK_FILL, 0, 0);
  else
    gtk_table_attach (GTK_TABLE (table), navigation_button, 2, 3, 2, 3,
                      GTK_FILL, GTK_FILL, 0, 0);
  if (!ddisp->origin)
    ddisp->origin = g_object_ref (navigation_button);
}
/**
 * @param ddisp The diagram display object that a window is created for
 * @param title
 */
static void
use_integrated_ui_for_display_shell(DDisplay *ddisp, char *title)
{
  GtkWidget *table;
  GtkWidget *label;                /* Text label for the notebook page */
  GtkWidget *tab_label_container;  /* Container to hold text label & close button */
  int width, height;               /* Width/Heigth of the diagram */
  GtkWidget *image;
  GtkWidget *close_button;         /* Close button for the notebook page */
  GtkRcStyle *rcstyle;
  gint       notebook_page_index;

  ddisp->is_standalone_window = FALSE;

  ddisp->shell = GTK_WIDGET (ui.main_window);
  ddisp->modified_status = GTK_WIDGET (ui.statusbar);
 
  tab_label_container = gtk_hbox_new(FALSE,3);
  label = gtk_label_new( title );
  gtk_box_pack_start( GTK_BOX(tab_label_container), label, FALSE, FALSE, 0 );
  gtk_widget_show (label);
  /* Create a new tab page */
  ddisp->container = gtk_vbox_new(FALSE, 0);

  /* <from GEdit> */
  /* don't allow focus on the close button */
  close_button = gtk_button_new();
  gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

  /* make it as small as possible */
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (close_button, rcstyle);
  g_object_unref (rcstyle),

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
                                    GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER(close_button), image);
  g_signal_connect (G_OBJECT (close_button), "clicked", 
                    G_CALLBACK (close_notebook_page_callback), ddisp->container);
  /* </from GEdit> */

  gtk_box_pack_start( GTK_BOX(tab_label_container), close_button, FALSE, FALSE, 0 );
  gtk_widget_show (close_button);
  gtk_widget_show (image);

  /* Set events for new tab page */
  _ddisplay_setup_events (ddisp, ddisp->container);

  notebook_page_index = gtk_notebook_append_page (GTK_NOTEBOOK(ui.diagram_notebook),
                                                  ddisp->container,
                                                  tab_label_container);

  g_object_set_data (G_OBJECT (ddisp->container), "DDisplay",  ddisp);
  g_object_set_data (G_OBJECT (ddisp->container), "tab-label", label);
  g_object_set_data (G_OBJECT (ddisp->container), "window",    ui.main_window);

  /*  the table containing all widgets  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);

  gtk_box_pack_start( GTK_BOX(ddisp->container), table, TRUE, TRUE, 0 );

  /*  scrollbars, rulers, canvas, menu popup button  */
  ddisp->origin = NULL;
  _ddisplay_setup_rulers (ddisp, ddisp->container, table);

  /* Get the width/height of the Notebook child area */
  /* TODO: Fix width/height hardcoded values */
  width = 100;
  height = 100;
  _ddisplay_setup_scrollbars (ddisp, table, width, height);
  _ddisplay_setup_navigation (ddisp, table, TRUE);

  ddisp->canvas = create_canvas (ddisp);

  /*  place all remaining widgets (no 'origin' anymore, since navigation is top-left */
  gtk_table_attach (GTK_TABLE (table), ddisp->canvas, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  ddisp->common_toolbar = ui.toolbar;
  /* Stand-alone window menubar */
  ddisp->menu_bar = NULL;
  /* Stand-alone window Zoom status/menu */
  ddisp->zoom_status = NULL;
  /* Stand-alone window Grid on/off button */
  ddisp->grid_status = NULL;
  /* Stand-alone window Object Snapping button */
  ddisp->mainpoint_status = NULL;

  gtk_widget_show (ddisp->container);
  gtk_widget_show (table);
  display_rulers_show (ddisp);
  gtk_widget_show (ddisp->canvas);

  /* Show new page */
  gtk_notebook_set_current_page (ui.diagram_notebook, notebook_page_index);

  integrated_ui_toolbar_grid_snap_synchronize_to_display (ddisp);
  integrated_ui_toolbar_object_snap_synchronize_to_display (ddisp);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (ddisp->canvas);
}

/**
 * @param ddisp The diagram display object that a window is created for
 * @param width Diagram widgth
 * @param height Diagram Height
 * @param title Window title
 * @param use_mbar Flag to indicate whether to add a menubar to the window
 */
void
create_display_shell(DDisplay *ddisp,
		     int width, int height,
		     char *title, int use_mbar)
{
  GtkWidget *table, *widget;
  GtkWidget *status_hbox;
  GtkWidget *root_vbox = NULL;
  GtkWidget *zoom_hbox, *zoom_label;
  int s_width, s_height;

  if (app_is_interactive() && is_integrated_ui())
  {
    use_integrated_ui_for_display_shell(ddisp, title);
    return;
  }
 
  ddisp->is_standalone_window = TRUE;
  ddisp->container            = NULL;

  s_width = gdk_screen_width ();
  s_height = gdk_screen_height ();
  if (width > s_width)
    width = s_width;
  if (height > s_height)
    height = s_height;

  /*  The toplevel shell */
  ddisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
  gtk_window_set_role (GTK_WINDOW (ddisp->shell), "diagram_window");
  gtk_window_set_icon_name (GTK_WINDOW (ddisp->shell), "dia");
  gtk_window_set_default_size(GTK_WINDOW (ddisp->shell), width, height);
  /* set_icon_name needs registered theme icons, not always available: provide fallback */
  if (!gtk_window_get_icon (GTK_WINDOW (ddisp->shell))) {
    static GdkPixbuf *pixbuf = NULL;

    if (!pixbuf)
      pixbuf = gdk_pixbuf_new_from_inline(-1, dia_diagram_icon, FALSE, NULL);
    if (pixbuf)
      gtk_window_set_icon (GTK_WINDOW (ddisp->shell), pixbuf);
  }

  g_object_set_data (G_OBJECT (ddisp->shell), "user_data", (gpointer) ddisp);

  _ddisplay_setup_events (ddisp, ddisp->shell);
  /* following two not shared with integrated UI */
  g_signal_connect (G_OBJECT (ddisp->shell), "delete_event",
		    G_CALLBACK (ddisplay_delete), ddisp);
  g_signal_connect (G_OBJECT (ddisp->shell), "destroy",
		    G_CALLBACK (ddisplay_destroy), ddisp);

  /*  the table containing all widgets  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  if (use_mbar) 
  {
      root_vbox = gtk_vbox_new (FALSE, 1);
      gtk_container_add (GTK_CONTAINER (ddisp->shell), root_vbox);
      gtk_box_pack_end (GTK_BOX (root_vbox), table, TRUE, TRUE, 0);
  }
  else
  {
      gtk_container_add (GTK_CONTAINER (ddisp->shell), table);
  }
  

  /*  scrollbars, rulers, canvas, menu popup button  */
  if (!use_mbar) {
      ddisp->origin = gtk_button_new();
#if GTK_CHECK_VERSION(2,18,0)
      gtk_widget_set_can_focus (ddisp->origin, FALSE);
#else
      GTK_WIDGET_UNSET_FLAGS(ddisp->origin, GTK_CAN_FOCUS);
#endif
      widget = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
      gtk_container_add(GTK_CONTAINER(ddisp->origin), widget);
      gtk_widget_set_tooltip_text(widget, _("Diagram menu."));
      gtk_widget_show(widget);
      g_signal_connect(G_OBJECT(ddisp->origin), "button_press_event",
		     G_CALLBACK(origin_button_press), ddisp);
  }
  else {
      ddisp->origin = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (ddisp->origin), GTK_SHADOW_OUT);
  }
  
  _ddisplay_setup_rulers (ddisp, ddisp->shell, table);
  _ddisplay_setup_scrollbars (ddisp, table, width, height);
  _ddisplay_setup_navigation (ddisp, table, FALSE);

  ddisp->canvas = create_canvas (ddisp);

  /*  pack all remaining widgets  */
  gtk_table_attach (GTK_TABLE (table), ddisp->origin, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->canvas, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  /* TODO rob use per window accel */
  ddisp->accel_group = menus_get_display_accels ();
  gtk_window_add_accel_group(GTK_WINDOW(ddisp->shell), ddisp->accel_group);

  if (use_mbar) {
    ddisp->menu_bar = menus_create_display_menubar (&ddisp->ui_manager, &ddisp->actions);
    g_assert (ddisp->menu_bar);
    gtk_box_pack_start (GTK_BOX (root_vbox), ddisp->menu_bar, FALSE, TRUE, 0);
  }

  /* the statusbars */
  status_hbox = gtk_hbox_new (FALSE, 2);

  /* Zoom status pseudo-optionmenu */
  ddisp->zoom_status = create_zoom_widget(ddisp);
  zoom_hbox = gtk_hbox_new(FALSE, 0);
  zoom_label = gtk_label_new(_("Zoom"));
  gtk_box_pack_start (GTK_BOX(zoom_hbox), zoom_label,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(zoom_hbox), ddisp->zoom_status,
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (status_hbox), zoom_hbox, FALSE, FALSE, 0);

  /* Grid on/off button */
  ddisp->grid_status = dia_toggle_button_new_with_icons(dia_on_grid_icon,
							dia_off_grid_icon);
  
  g_signal_connect(G_OBJECT(ddisp->grid_status), "toggled",
		   G_CALLBACK (grid_toggle_snap), ddisp);
  gtk_widget_set_tooltip_text(ddisp->grid_status,
		       _("Toggles snap-to-grid for this window."));
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->grid_status,
		      FALSE, FALSE, 0);


  ddisp->mainpoint_status = dia_toggle_button_new_with_icons(dia_mainpoints_on_icon,
							dia_mainpoints_off_icon);
  
  g_signal_connect(G_OBJECT(ddisp->mainpoint_status), "toggled",
		   G_CALLBACK (interface_toggle_mainpoint_magnetism), ddisp);
  gtk_widget_set_tooltip_text(ddisp->mainpoint_status,
		       _("Toggles object snapping for this window."));
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->mainpoint_status,
		      FALSE, FALSE, 0);


  /* Statusbar */
  ddisp->modified_status = gtk_statusbar_new ();

  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->modified_status,
		      TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), status_hbox, 0, 3, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);

  display_rulers_show (ddisp);
  gtk_widget_show (ddisp->zoom_status);
  gtk_widget_show (zoom_hbox);
  gtk_widget_show (zoom_label);
  gtk_widget_show (ddisp->grid_status);
  gtk_widget_show (ddisp->mainpoint_status);
  gtk_widget_show (ddisp->modified_status);
  gtk_widget_show (status_hbox);
  gtk_widget_show (table);
  if (use_mbar) 
  {
      gtk_widget_show (ddisp->menu_bar);
      gtk_widget_show (root_vbox);
  }
  gtk_widget_show (ddisp->shell);

  /* before showing up, checking canvas's REAL size */
  if (use_mbar && ddisp->hrule->allocation.width > width) 
  {
    /* The menubar is not shrinkable, so the shell will have at least
     * the menubar's width. If the diagram's requested width is smaller,
     * the canvas will be enlarged to fit the place. In this case, we
     * need to adjust the horizontal scrollbar according to the size
     * that will be allocated, which the same as the hrule got.
     */

    width = ddisp->hrule->allocation.width;

    gtk_adjustment_set_upper (ddisp->hsbdata, width);
    gtk_adjustment_set_page_increment (ddisp->hsbdata, (width - 1) / 4);
    gtk_adjustment_set_page_size (ddisp->hsbdata, width - 1);

    gtk_adjustment_changed (GTK_ADJUSTMENT(ddisp->hsbdata));
  }
  gtk_widget_show (ddisp->canvas);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (ddisp->canvas);
}

/**
 * Adapt the rulers to current field of view
 *
 * @param ddisp The display to hide the rulers on.
 */
void 
ddisplay_update_rulers (DDisplay        *ddisp,
                        const Rectangle *extents,
		        const Rectangle *visible)
{
  dia_ruler_set_range  (ddisp->hrule,
			visible->left,
			visible->right,
			0.0f /* position*/,
			MAX(extents->right, visible->right)/* max_size*/);
  dia_ruler_set_range  (ddisp->vrule,
			visible->top,
			visible->bottom,
			0.0f /*        position*/,
			MAX(extents->bottom, visible->bottom)/* max_size*/);
}

static void
toolbox_destroy (GtkWidget *widget, gpointer data)
{
  app_exit();
}

static gboolean
toolbox_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gulong handlerid;
  /** Stop toolbox_destroy from being called */
  handlerid = g_signal_handler_find(widget, G_SIGNAL_MATCH_FUNC,
				    0, 0, NULL, toolbox_destroy, NULL);
  if (handlerid != 0)
    g_signal_handler_disconnect (G_OBJECT (widget), handlerid);

  /** If the app didn't exit, don't close the window */
  return (!app_exit());
}

static void
app_set_icon (GtkWindow *window)
{
  gtk_window_set_icon_name (window, "dia");
  if (!gtk_window_get_icon (window)) {
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_inline (-1, dia_app_icon, FALSE, NULL);
    if (pixbuf) {
      gtk_window_set_icon (window, pixbuf);
      g_object_unref (pixbuf);
    }
  }
}

#ifdef HAVE_MAC_INTEGRATION
static gboolean
_osx_app_exit (GtkosxApplication *app,
	       gpointer           user_data)
{
  return !app_exit ();
}
static void
_create_mac_integration (GtkWidget *menubar)
{
  GtkosxApplication *theOsxApp = g_object_new(GTKOSX_TYPE_APPLICATION, NULL);

  /* from control-x to command-x in one call? Does _not_ work as advertized */
  gtkosx_application_set_use_quartz_accelerators (theOsxApp, TRUE);

  if (menubar) {
    /* hijack the menubar */
    gtkosx_application_set_menu_bar(theOsxApp, GTK_MENU_SHELL(menubar));
    /* move some items to the dia menu - apparently must be _after_ hijack */
    {
      GtkWidget *item;

      item = menus_get_widget (INTEGRATED_MENU "/Help/HelpAbout");
      if (GTK_IS_MENU_ITEM (item))
        gtkosx_application_insert_app_menu_item (theOsxApp, item, 0);
      gtkosx_application_insert_app_menu_item (theOsxApp, gtk_separator_menu_item_new (), 1);
      item = menus_get_widget (INTEGRATED_MENU "/File/FilePrefs");
      if (GTK_IS_MENU_ITEM (item))
        gtkosx_application_insert_app_menu_item (theOsxApp, item, 2);
      item = menus_get_widget (INTEGRATED_MENU "/File/FilePlugins");
      if (GTK_IS_MENU_ITEM (item))
        gtkosx_application_insert_app_menu_item (theOsxApp, item, 3);
#if 0 /* not sure if we should move these, too */
      item = menus_get_widget (INTEGRATED_MENU "/File/FileTree");
      if (GTK_IS_MENU_ITEM (item))
        gtkosx_application_insert_app_menu_item (theOsxApp, item, 4);
      item = menus_get_widget (INTEGRATED_MENU "/File/FileSheets");
      if (GTK_IS_MENU_ITEM (item))
        gtkosx_application_insert_app_menu_item (theOsxApp, item, 5);
#endif
      /* remove Quit from File menu */
      item = menus_get_widget (INTEGRATED_MENU "/File/FileQuit");
      if (GTK_IS_MENU_ITEM (item))
        gtk_widget_hide (item);
    }
    gtk_widget_hide (menubar); /* not working, it's shown elsewhere */
    /* setup the dock icon */
    gtkosx_application_set_dock_icon_pixbuf (theOsxApp,
	gdk_pixbuf_new_from_inline (-1, dia_app_icon, FALSE, NULL));
  }
  /* Don't quit without asking to save files first */
  g_signal_connect (theOsxApp, "NSApplicationBlockTermination",
		    G_CALLBACK (_osx_app_exit), NULL);
  /* without this all the above wont have any effect */
  gtkosx_application_ready(theOsxApp);
}
#endif

/**
 * Create integrated user interface
 */
void 
create_integrated_ui (void)
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *hbox;
  GtkWidget *wrapbox;
  GtkWidget *menubar;
  GtkWidget *toolbar;
  GtkWidget *notebook;
  GtkWidget *statusbar;
  GtkAccelGroup *accel_group;

  GtkWidget *layer_view;
	
#ifdef HAVE_GNOME
  window = gnome_app_new ("Dia", _("Diagram Editor"));
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_ref (window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia v" VERSION);
#endif

  /* hint to window manager on X so the wm can put the window in the same  
     as it was when the application shut down                             */      
  gtk_window_set_role (GTK_WINDOW (window), DIA_MAIN_WINDOW);
  
  gtk_window_set_default_size (GTK_WINDOW (window), 146, 349);
 
  app_set_icon (GTK_WINDOW (window));

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (toolbox_delete),
		      window);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (toolbox_destroy),
		      window);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
#ifdef HAVE_GNOME
  gnome_app_set_contents (GNOME_APP(window), main_vbox);
#else
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
#endif
  gtk_widget_show (main_vbox);

  /* Applicatioon Statusbar */
  statusbar = gtk_statusbar_new ();
  gtk_box_pack_end (GTK_BOX (main_vbox), statusbar, FALSE, TRUE, 0);
  /* HBox for everything below the menubar and toolbars */
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);
  
  /* Layer View  */
  layer_view = create_layer_view_widget ();
  gtk_box_pack_end (GTK_BOX (hbox), layer_view, FALSE, FALSE, 0);
	 
  /* Diagram Notebook */
  notebook = gtk_notebook_new ();
  gtk_box_pack_end (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE); 
  gtk_widget_show (notebook);

  /* Toolbox widget */
  wrapbox = toolbox_create();
  gtk_box_pack_start (GTK_BOX (hbox), wrapbox, FALSE, TRUE, 0);

  g_signal_connect (G_OBJECT (wrapbox), "drag_data_received",
		    G_CALLBACK (dia_dnd_file_drag_data_received),
                    NULL); /* userdata == NULL here intentionally */
  /* setup the notebook to receive drops as well */
  toolbox_setup_drag_dest (notebook);
  g_signal_connect (G_OBJECT (notebook), "drag_data_received",
		    G_CALLBACK (dia_dnd_file_drag_data_received),
                    NULL); /* userdata == NULL here intentionally */

  /* menus -- initialised afterwards, because initing the display menus
   * uses the tool buttons*/
  menus_get_integrated_ui_menubar(&menubar, &toolbar, &accel_group);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_widget_show (menubar);
#ifdef HAVE_GNOME
  gnome_app_set_menus (GNOME_APP (window), GTK_MENU_BAR (menubar));
#else
#  ifdef HAVE_MAC_INTEGRATION
  _create_mac_integration (menubar);
#  else
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
#  endif
#endif

  /* Toolbar */
  /* TODO: maybe delete set_style(toolbar,ICONS) */
  gtk_toolbar_set_style (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, TRUE, 0);
  gtk_widget_show (toolbar);

  persistence_register_window(GTK_WINDOW(window));

  ui.main_window      = GTK_WINDOW    (window);
  ui.toolbar          = GTK_TOOLBAR   (toolbar);
  ui.diagram_notebook = GTK_NOTEBOOK  (notebook);
  ui.statusbar        = GTK_STATUSBAR (statusbar);
  ui.layer_view       =                layer_view;

  /* NOTE: These functions use ui.xxx assignments above and so must come after
   *       the user interface components are set.                              */
  integrated_ui_toolbar_show (TRUE);
  integrated_ui_statusbar_show (TRUE);

  /* For access outside here: */
  g_object_set_data (G_OBJECT (ui.main_window), DIA_MAIN_NOTEBOOK, notebook);

  /* TODO: Figure out what to do about toolbox_shell for integrated UI */
  toolbox_shell = window;
}

/**
 * Create toolbox component for distributed user interface 
 */
void
create_toolbox ()
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *wrapbox;
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;

#ifdef HAVE_GNOME
  window = gnome_app_new ("Dia", _("Diagram Editor"));
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_ref(window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia v" VERSION);
#endif
  gtk_window_set_role (GTK_WINDOW (window), "toolbox_window");
  gtk_window_set_default_size(GTK_WINDOW(window), 146, 349);

  app_set_icon (GTK_WINDOW (window));

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (toolbox_delete), window);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (toolbox_destroy), window);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
#ifdef HAVE_GNOME
  gnome_app_set_contents(GNOME_APP(window), main_vbox);
#else
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
#endif

  wrapbox = toolbox_create();
  gtk_box_pack_end (GTK_BOX (main_vbox), wrapbox, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (wrapbox), "drag_data_received",
		    G_CALLBACK (dia_dnd_file_drag_data_received),
                    NULL); /* userdata == NULL here intentionally */
  gtk_widget_show (main_vbox);

  /* menus -- initialised afterwards, because initing the display menus
   * uses the tool buttons*/
  menus_get_toolbox_menubar(&menubar, &accel_group);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
  gtk_widget_show (menubar);
#ifdef HAVE_GNOME
  gnome_app_set_menus(GNOME_APP(window), GTK_MENU_BAR(menubar));
#else
#  ifdef HAVE_MAC_INTEGRATION
  _create_mac_integration (menubar);
#  else
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
#  endif
#endif
  persistence_register_window(GTK_WINDOW(window));

  toolbox_shell = window;
}

void
toolbox_show(void)
{
  gtk_widget_show(toolbox_shell);
}

void
toolbox_hide(void)
{
  gtk_widget_hide(toolbox_shell);
}

GtkWidget *
interface_get_toolbox_shell(void)
{
  if (is_integrated_ui ())
    return GTK_WIDGET(ui.main_window);

  return toolbox_shell;
}

/* Indicate if the integrated UI toolbar is showing.
 * @return TRUE if showing, FALSE if not showing or doesn't exist 
 */ 
gboolean 
integrated_ui_toolbar_is_showing (void)
{
  if (ui.toolbar)
  {
#if GTK_CHECK_VERSION(2,20,0)
    return gtk_widget_get_visible (GTK_WIDGET (ui.toolbar))? TRUE : FALSE;
#else
    return GTK_WIDGET_VISIBLE (ui.toolbar)? TRUE : FALSE;
#endif
  }
  return FALSE;
}

/* show integrated UI main toolbar and set pulldown menu action. */
void 
integrated_ui_toolbar_show (gboolean show)
{
  if (ui.toolbar) {
    GtkAction *action = NULL;
    if (show)
      gtk_widget_show (GTK_WIDGET (ui.toolbar));
    else
      gtk_widget_hide (GTK_WIDGET (ui.toolbar));
    action = menus_get_action (VIEW_MAIN_TOOLBAR_ACTION);
    if (action)
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show);
  }
}

/* Indicate if the integrated UI Layer View is showing.
 * @return TRUE if showing, FALSE if not showing or doesn't exist 
 */ 
gboolean 
integrated_ui_layer_view_is_showing (void)
{
  if (ui.layer_view)
  {
#if GTK_CHECK_VERSION(2,20,0)
    return gtk_widget_get_visible (GTK_WIDGET (ui.layer_view))? TRUE : FALSE;
#else
    return GTK_WIDGET_VISIBLE (ui.layer_view)? TRUE : FALSE;
#endif
  }
  return FALSE;
}

void 
integrated_ui_layer_view_show (gboolean show)
{
  if (ui.layer_view) {
    GtkAction *action;
    if (show)
      gtk_widget_show (ui.layer_view);
    else
      gtk_widget_hide (ui.layer_view);
    action = menus_get_action (VIEW_LAYERS_ACTION);
    if (action) 
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show);
  }
}

/* show() integrated UI main statusbar and set pulldown menu action. */
void 
integrated_ui_statusbar_show (gboolean show)
{
  if (ui.statusbar)
  {
    GtkAction *action;
    if (show)
      gtk_widget_show (GTK_WIDGET (ui.statusbar));
    else
      gtk_widget_hide (GTK_WIDGET (ui.statusbar));
    action = menus_get_action (VIEW_MAIN_STATUSBAR_ACTION);
    if (action)
      gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), show);
  }
}
