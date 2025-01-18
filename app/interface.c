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

#define G_LOG_DOMAIN "Dia"

#include "config.h"

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <stdio.h>
#include <string.h>

#include "dia-canvas.h"
#include "diagram.h"
#include "object.h"
#include "layer-editor/layer_dialog.h"
#include "interface.h"
#include "display.h"
#include "disp_callbacks.h"
#include "menus.h"
#include "toolbox.h"
#include "commands.h"
#include "dia_dirs.h"
#include "navigation.h"
#include "persistence.h"
#include "widgets.h"
#include "message.h"
#include "dia-ruler.h"
#include "diainteractiverenderer.h"
#include "dia-version-info.h"

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "dia-guide-tool.h"

#include <glib/gprintf.h>

static gboolean _ddisplay_hruler_button_press (GtkWidget *widget,
        GdkEventButton *bevent,
        DDisplay *ddisp);

static gboolean _ddisplay_vruler_button_press (GtkWidget *widget,
        GdkEventButton *bevent,
        DDisplay *ddisp);

static gboolean _ddisplay_ruler_button_press (GtkWidget *widget,
        GdkEventButton *event,
        DDisplay *ddisp,
        GtkOrientation orientation);

static gboolean _ddisplay_ruler_button_release (GtkWidget *widget,
        GdkEventButton *bevent,
        DDisplay *ddisp);

static gboolean _ddisplay_hruler_motion_notify (GtkWidget *widget,
        GdkEventMotion *event,
        DDisplay *ddisp);

static gboolean _ddisplay_vruler_motion_notify (GtkWidget *widget,
        GdkEventMotion *event,
        DDisplay *ddisp);


void
dia_dnd_file_drag_data_received (GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 int               x,
                                 int               y,
                                 GtkSelectionData *data,
                                 guint             info,
                                 guint             time,
                                 DDisplay         *ddisp)
{
  switch (gdk_drag_context_get_selected_action (context)) {
    case GDK_ACTION_DEFAULT:
    case GDK_ACTION_COPY:
    case GDK_ACTION_MOVE:
    case GDK_ACTION_LINK:
    case GDK_ACTION_ASK:
    case GDK_ACTION_PRIVATE:
    default:
      {
        Diagram *diagram = NULL;
        gchar *sPath = NULL, *pFrom, *pTo;

        pFrom = strstr ((gchar *) gtk_selection_data_get_data (data), "file:");
        while (pFrom) {
          GError *error = NULL;

          pTo = pFrom;
          while (*pTo != 0 && *pTo != 0xd && *pTo != 0xa) pTo ++;
          sPath = g_strndup (pFrom, pTo - pFrom);

          /* format changed with Gtk+2.0, use conversion */
          pFrom = g_filename_from_uri (sPath, NULL, &error);
          if (!ddisp) {
            diagram = diagram_load (pFrom, NULL);
          } else {
            diagram = ddisp->diagram;
            if (!diagram_load_into (diagram, pFrom, NULL)) {
              /* the import filter is supposed to show the error message */
              gtk_drag_finish (context, TRUE, FALSE, time);
              break;
            }
          }

          g_clear_pointer (&pFrom, g_free);
          g_clear_pointer (&sPath, g_free);

          if (diagram != NULL) {
            diagram_update_extents(diagram);
            layer_dialog_set_diagram(diagram);

            if (diagram->displays == NULL) {
              new_display (diagram);
            }
          }

          pFrom = strstr (pTo, "file:");
        } /* while */
        gtk_drag_finish (context, TRUE, FALSE, time);
      }
      break;
  }
}


static GtkWidget *toolbox_shell = NULL;

static struct {
  GtkWindow    *main_window;
  GtkToolbar   *toolbar;
  GtkNotebook  *diagram_notebook;
  GtkStatusbar *statusbar;
  GtkWidget    *layer_view;
} ui;


/**
 * is_integrated_ui:
 *
 * Used to determine if the current user interface is the integrated interface
 * or the distributed interface.  This cannot presently be determined by the
 * preferences setting because changing that setting at run time does not
 * change the interface.
 *
 * Returns: %TRUE if the integrated interface is present, else %FALSE.
 */
int
is_integrated_ui (void)
{
  return ui.main_window == NULL ? FALSE : TRUE;
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


static void
interface_toggle_snap_to_guides (GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp = (DDisplay *) data;
  ddisplay_set_snap_to_guides (ddisp,
                               gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget)));
  ddisplay_add_update_all (ddisp);
  ddisplay_flush (ddisp);
}


static int
origin_button_press (GtkWidget *widget, GdkEventButton *event, gpointer data)
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
view_zoom_set (double factor)
{
  DDisplay *ddisp;
  double scale;

  ddisp = ddisplay_active();
  g_return_if_fail (ddisp != NULL);

  scale = ((double) factor) / 1000.0 * DDISPLAY_NORMAL_ZOOM;

  ddisplay_zoom_middle (ddisp, scale / ddisp->zoom_factor);
}


static void
zoom_activate_callback (GtkWidget *item, gpointer user_data)
{
  DDisplay *ddisp = (DDisplay *) user_data;
  const gchar *zoom_text =
      gtk_entry_get_text (GTK_ENTRY (g_object_get_data (G_OBJECT (ddisp->zoom_status), "user_data")));
  double zoom_amount, magnify;
  char *zoomamount = g_object_get_data (G_OBJECT (item), "zoomamount");
  if (zoomamount != NULL) {
    zoom_text = zoomamount;
  }

  zoom_amount = parse_zoom (zoom_text);

  if (zoom_amount > 0) {
    /* Set limits to avoid crashes, see bug #483384 */
    if (zoom_amount < .1) {
      zoom_amount = .1;
    } else if (zoom_amount > 1e4) {
      zoom_amount = 1e4;
    }

    // Translators: Current zoom level
    zoomamount = g_strdup_printf (_("%f%%"), zoom_amount);
    gtk_entry_set_text (GTK_ENTRY (g_object_get_data (G_OBJECT (ddisp->zoom_status), "user_data")), zoomamount);
    g_clear_pointer (&zoomamount, g_free);
    magnify = (zoom_amount*DDISPLAY_NORMAL_ZOOM/100.0)/ddisp->zoom_factor;
    if (fabs (magnify - 1.0) > 0.000001) {
      ddisplay_zoom_middle (ddisp, magnify);
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

  combo = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  entry = gtk_entry_new();
  g_signal_connect (G_OBJECT (entry), "activate",
                    G_CALLBACK (zoom_activate_callback),
                    ddisp);
  gtk_box_pack_start (GTK_BOX (combo), entry, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (combo), "user_data", entry);
  gtk_entry_set_width_chars (GTK_ENTRY (entry), 8);
  gtk_widget_show (entry);

  button = gtk_button_new ();
  gtk_widget_set_can_focus (GTK_WIDGET (button), FALSE);
  arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_container_add (GTK_CONTAINER (button), arrow);
  gtk_box_pack_start (GTK_BOX (combo), button, TRUE, TRUE, 0);
  g_object_set_data (G_OBJECT (combo), "user_data", entry);
  gtk_widget_show_all (button);

  menu = gtk_menu_new ();
  zoom_add_zoom_amount (menu, "800%", ddisp);
  zoom_add_zoom_amount (menu, "400%", ddisp);
  zoom_add_zoom_amount (menu, "200%", ddisp);
  zoom_add_zoom_amount (menu, "100%", ddisp);
  zoom_add_zoom_amount (menu, "50%", ddisp);
  zoom_add_zoom_amount (menu, "25%", ddisp);
  zoom_add_zoom_amount (menu, "12%", ddisp);

  gtk_widget_show_all (menu);

  g_signal_connect (G_OBJECT (button), "button_press_event",
                    G_CALLBACK (zoom_popup_menu), menu);

  return combo;
}


/**
 * close_notebook_page_callback:
 * @button: The notebook close button.
 * @user_data: Container widget (e.g. Box).
 *
 * Since: dawn-of-time
 */
void
close_notebook_page_callback (GtkButton *button,
                              gpointer   user_data)
{
  GtkBox *page = user_data;
  DDisplay *ddisp = g_object_get_data (G_OBJECT (page), "DDisplay");

  /* When the page widget is destroyed it removes itself from the notebook */
  ddisplay_close (ddisp);
}


static void
notebook_switch_page (GtkNotebook *notebook,
                      GtkWidget   *page,
                      guint        page_num,
                      gpointer     user_data)
{
  DDisplay *ddisp = g_object_get_data (G_OBJECT (page), "DDisplay");

  g_return_if_fail (ddisp != NULL);

  display_set_active (ddisp);

  gtk_widget_grab_focus (ddisp->canvas);
}

/* Shared helper functions for both UI cases
 */
static void
_ddisplay_setup_rulers (DDisplay *ddisp, GtkWidget *shell, GtkWidget *grid)
{
  ddisp->hrule = dia_ruler_new (GTK_ORIENTATION_HORIZONTAL, shell, ddisp);
  ddisp->vrule = dia_ruler_new (GTK_ORIENTATION_VERTICAL, shell, ddisp);

  /* Callbacks for adding guides - horizontal ruler. */
  g_signal_connect (ddisp->hrule, "button-press-event",
                    G_CALLBACK (_ddisplay_hruler_button_press),
                    ddisp);

  g_signal_connect (ddisp->hrule, "button_release_event",
                    G_CALLBACK (_ddisplay_ruler_button_release),
                    ddisp);

  g_signal_connect (ddisp->hrule, "motion_notify_event",
                    G_CALLBACK (_ddisplay_hruler_motion_notify),
                    ddisp);

  /* Callbacks for adding guides - vertical ruler. */
  g_signal_connect (ddisp->vrule, "button-press-event",
                    G_CALLBACK (_ddisplay_vruler_button_press),
                    ddisp);

  g_signal_connect (ddisp->vrule, "button_release_event",
                    G_CALLBACK (_ddisplay_ruler_button_release),
                    ddisp);

  g_signal_connect (ddisp->vrule, "motion_notify_event",
                    G_CALLBACK (_ddisplay_vruler_motion_notify),
                    ddisp);

  /* harder to change position in the grid, but we did not do it for years ;) */
  gtk_grid_attach (GTK_GRID (grid), ddisp->hrule, 1, 0, 1, 1);
  gtk_widget_set_hexpand (ddisp->hrule, TRUE);
  gtk_grid_attach (GTK_GRID (grid), ddisp->vrule, 0, 1, 1, 1);
  gtk_widget_set_vexpand (ddisp->vrule, TRUE);
}

static void
_ddisplay_setup_events (DDisplay *ddisp, GtkWidget *shell)
{
  gtk_widget_set_events (shell,
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_FOCUS_CHANGE_MASK);

  g_signal_connect (G_OBJECT (shell),
                    "focus_out_event",
                    G_CALLBACK (ddisplay_focus_out_event),
                    ddisp);
  g_signal_connect (G_OBJECT (shell),
                    "focus_in_event",
                    G_CALLBACK (ddisplay_focus_in_event),
                    ddisp);
  g_signal_connect (G_OBJECT (shell),
                    "realize",
                    G_CALLBACK (ddisplay_realize),
                    ddisp);
  g_signal_connect (G_OBJECT (shell),
                    "unrealize",
                    G_CALLBACK (ddisplay_unrealize),
                    ddisp);
}

static void
_ddisplay_setup_scrollbars (DDisplay *ddisp, GtkWidget *grid, int width, int height)
{
  /*  The adjustment datums  */
  ddisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, (width-1)/4, width-1));
  ddisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, (height-1)/4, height-1));

  ddisp->hsb = gtk_scrollbar_new (GTK_ORIENTATION_HORIZONTAL, ddisp->hsbdata);
  gtk_widget_set_can_focus (ddisp->hsb, FALSE);
  ddisp->vsb = gtk_scrollbar_new (GTK_ORIENTATION_VERTICAL, ddisp->vsbdata);
  gtk_widget_set_can_focus (ddisp->vsb, FALSE);

  /*  set up the scrollbar observers  */
  g_signal_connect (G_OBJECT (ddisp->hsbdata), "value_changed",
                    G_CALLBACK (ddisplay_hsb_update), ddisp);
  g_signal_connect (G_OBJECT (ddisp->vsbdata), "value_changed",
                    G_CALLBACK (ddisplay_vsb_update), ddisp);

  /* harder to change position in the grid, but we did not do it for years ;) */
  gtk_grid_attach (GTK_GRID (grid), ddisp->hsb, 0, 2, 2, 1);
  gtk_widget_set_hexpand(ddisp->hsb, TRUE);
  gtk_grid_attach (GTK_GRID (grid), ddisp->vsb, 2, 0, 1, 2);
  gtk_widget_set_vexpand(ddisp->vsb, TRUE);

  gtk_widget_show (ddisp->hsb);
  gtk_widget_show (ddisp->vsb);
}
static void
_ddisplay_setup_navigation (DDisplay *ddisp, GtkWidget *grid, gboolean top_left)
{
  GtkWidget *navigation_button;

  /*  Popup button between scrollbars for navigation window  */
  navigation_button = navigation_popup_new(ddisp);
  gtk_widget_set_tooltip_text(navigation_button,
                       _("Pops up the Navigation window."));
  gtk_widget_show(navigation_button);

  /* harder to change position in the grid, but we did not do it for years ;) */
  if (top_left)
    gtk_grid_attach (GTK_GRID (grid), navigation_button, 0, 0, 1, 1);
  else
    gtk_grid_attach (GTK_GRID (grid), navigation_button, 2, 2, 1, 1);
  if (!ddisp->origin)
    ddisp->origin = g_object_ref (navigation_button);
}


/**
 * use_integrated_ui_for_display_shell:
 * @ddisp: The diagram #DDisplay object that a window is created for
 * @title: the title
 *
 * Since: dawn-of-time
 */
static void
use_integrated_ui_for_display_shell (DDisplay *ddisp, char *title)
{
  GtkWidget *grid;
  GtkWidget *label;                /* Text label for the notebook page */
  GtkWidget *tab_label_container;  /* Container to hold text label & close button */
  int width, height;               /* Width/Heigth of the diagram */
  GtkWidget *image;
  GtkWidget *close_button;         /* Close button for the notebook page */
  int notebook_page_index;

  ddisp->is_standalone_window = FALSE;

  ddisp->shell = GTK_WIDGET (ui.main_window);
  ddisp->modified_status = GTK_WIDGET (ui.statusbar);

  tab_label_container = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3);
  label = gtk_label_new( title );
  gtk_box_pack_start( GTK_BOX(tab_label_container), label, FALSE, FALSE, 0 );
  gtk_widget_show (label);
  /* Create a new tab page */
  ddisp->container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  /* <from GEdit> */
  /* don't allow focus on the close button */
  close_button = gtk_button_new ();
  gtk_widget_set_tooltip_text (close_button, _("Close"));
  gtk_button_set_relief (GTK_BUTTON (close_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (close_button), FALSE);

  /* make it as small as possible */
  // Gtk3 disabled -- Hub
  // rcstyle = gtk_rc_style_new ();
  // rcstyle->xthickness = rcstyle->ythickness = 0;
  // gtk_widget_modify_style (close_button, rcstyle);
  // g_clear_object (&rcstyle);

  image = gtk_image_new_from_icon_name ("window-close-symbolic", GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER(close_button), image);
  g_signal_connect (G_OBJECT (close_button), "clicked",
                    G_CALLBACK (close_notebook_page_callback), ddisp->container);
  /* </from GEdit> */

  gtk_box_pack_start( GTK_BOX(tab_label_container), close_button, FALSE, FALSE, 0 );
  gtk_widget_show (close_button);
  gtk_widget_show (image);

  /* Set events for new tab page */
  _ddisplay_setup_events (ddisp, ddisp->container);

  g_object_set_data (G_OBJECT (ddisp->container), "DDisplay",  ddisp);
  g_object_set_data (G_OBJECT (ddisp->container), "tab-label", label);
  g_object_set_data (G_OBJECT (ddisp->container), "window",    ui.main_window);

  /*  the grid containing all widgets  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 2);

  gtk_box_pack_start( GTK_BOX(ddisp->container), grid, TRUE, TRUE, 0 );

  /*  scrollbars, rulers, canvas, menu popup button  */
  ddisp->origin = NULL;
  _ddisplay_setup_rulers (ddisp, ddisp->container, grid);

  /* Get the width/height of the Notebook child area */
  /* TODO: Fix width/height hardcoded values */
  width = 100;
  height = 100;
  _ddisplay_setup_scrollbars (ddisp, grid, width, height);
  _ddisplay_setup_navigation (ddisp, grid, TRUE);

  ddisp->canvas = dia_canvas_new (ddisp);

  /*  place all remaining widgets (no 'origin' anymore, since navigation is top-left */
  gtk_grid_attach (GTK_GRID (grid), ddisp->canvas, 1, 1, 1, 1);
  gtk_widget_set_hexpand (ddisp->canvas, TRUE);
  gtk_widget_set_vexpand (ddisp->canvas, TRUE);

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
  gtk_widget_show (grid);
  display_rulers_show (ddisp);
  gtk_widget_show (ddisp->canvas);

  /* Show new page */
  notebook_page_index = gtk_notebook_append_page (GTK_NOTEBOOK (ui.diagram_notebook),
                                                  ddisp->container,
                                                  tab_label_container);
  gtk_notebook_set_current_page (ui.diagram_notebook, notebook_page_index);

  integrated_ui_toolbar_grid_snap_synchronize_to_display (ddisp);
  integrated_ui_toolbar_object_snap_synchronize_to_display (ddisp);
  integrated_ui_toolbar_guides_snap_synchronize_to_display (ddisp);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (ddisp->canvas);
}


/**
 * create_display_shell:
 * @ddisp: The diagram display object that a window is created for
 * @width: Diagram width
 * @height: Diagram Height
 * @title: Window title
 * @use_mbar: Flag to indicate whether to add a menubar to the window
 *
 * Since: dawn-of-time
 */
void
create_display_shell (DDisplay *ddisp,
                      int       width,
                      int       height,
                      char     *title,
                      int       use_mbar)
{
  GtkWidget *grid, *widget;
  GtkWidget *status_hbox;
  GtkWidget *root_vbox = NULL;
  GtkWidget *zoom_hbox, *zoom_label;
  int s_width, s_height;
  GtkAllocation alloc;

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
  gtk_window_set_default_size(GTK_WINDOW (ddisp->shell), width, height);

  g_object_set_data (G_OBJECT (ddisp->shell), "user_data", (gpointer) ddisp);

  _ddisplay_setup_events (ddisp, ddisp->shell);
  /* following two not shared with integrated UI */
  g_signal_connect (G_OBJECT (ddisp->shell), "delete_event",
		    G_CALLBACK (ddisplay_delete), ddisp);
  g_signal_connect (G_OBJECT (ddisp->shell), "destroy",
		    G_CALLBACK (ddisplay_destroy), ddisp);

  /*  the grid containing all widgets  */
  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 2);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 2);
  gtk_container_set_border_width (GTK_CONTAINER (grid), 2);
  if (use_mbar)
  {
      root_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
      gtk_container_add (GTK_CONTAINER (ddisp->shell), root_vbox);
      gtk_box_pack_end (GTK_BOX (root_vbox), grid, TRUE, TRUE, 0);
  }
  else
  {
      gtk_container_add (GTK_CONTAINER (ddisp->shell), grid);
  }


  /*  scrollbars, rulers, canvas, menu popup button  */
  if (!use_mbar) {
      ddisp->origin = gtk_button_new ();
      gtk_widget_set_can_focus (ddisp->origin, FALSE);
      widget = gtk_arrow_new (GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
      gtk_container_add (GTK_CONTAINER (ddisp->origin), widget);
      gtk_widget_set_tooltip_text (widget, _("Diagram menu."));
      gtk_widget_show (widget);
      g_signal_connect (G_OBJECT (ddisp->origin), "button_press_event",
                        G_CALLBACK (origin_button_press), ddisp);
  }
  else {
      ddisp->origin = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (ddisp->origin), GTK_SHADOW_OUT);
  }

  _ddisplay_setup_rulers (ddisp, ddisp->shell, grid);
  _ddisplay_setup_scrollbars (ddisp, grid, width, height);
  _ddisplay_setup_navigation (ddisp, grid, FALSE);

  ddisp->canvas = dia_canvas_new (ddisp);

  /*  pack all remaining widgets  */
  gtk_grid_attach (GTK_GRID (grid), ddisp->origin, 0, 0, 1, 1);
  gtk_grid_attach (GTK_GRID (grid), ddisp->canvas, 1, 1, 1, 1);
  gtk_widget_set_vexpand(ddisp->canvas, TRUE);
  gtk_widget_set_hexpand(ddisp->canvas, TRUE);

  /* TODO rob use per window accel */
  ddisp->accel_group = menus_get_display_accels ();
  gtk_window_add_accel_group(GTK_WINDOW(ddisp->shell), ddisp->accel_group);

  if (use_mbar) {
    ddisp->menu_bar = menus_create_display_menubar (&ddisp->ui_manager, &ddisp->actions);
    g_assert (ddisp->menu_bar);
    gtk_box_pack_start (GTK_BOX (root_vbox), ddisp->menu_bar, FALSE, TRUE, 0);
  }

  /* the statusbars */
  status_hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);

  /* Zoom status pseudo-optionmenu */
  ddisp->zoom_status = create_zoom_widget(ddisp);
  zoom_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  zoom_label = gtk_label_new(_("Zoom"));
  gtk_box_pack_start (GTK_BOX(zoom_hbox), zoom_label,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX(zoom_hbox), ddisp->zoom_status,
		      FALSE, FALSE, 0);

  gtk_box_pack_start (GTK_BOX (status_hbox), zoom_hbox, FALSE, FALSE, 0);

  /* Grid on/off button */
  ddisp->grid_status = dia_toggle_button_new_with_icon_names ("dia-grid-on",
                                                              "dia-grid-off");

  g_signal_connect(G_OBJECT(ddisp->grid_status), "toggled",
		   G_CALLBACK (grid_toggle_snap), ddisp);
  gtk_widget_set_tooltip_text(ddisp->grid_status,
		       _("Toggles snap-to-grid for this window."));
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->grid_status,
		      FALSE, FALSE, 0);


  ddisp->mainpoint_status = dia_toggle_button_new_with_icon_names ("dia-mainpoints-on",
                                                                   "dia-mainpoints-off");

  g_signal_connect(G_OBJECT(ddisp->mainpoint_status), "toggled",
		   G_CALLBACK (interface_toggle_mainpoint_magnetism), ddisp);
  gtk_widget_set_tooltip_text(ddisp->mainpoint_status,
		       _("Toggles object snapping for this window."));
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->mainpoint_status,
		      FALSE, FALSE, 0);

  ddisp->guide_snap_status = dia_toggle_button_new_with_icon_names ("dia-guides-snap-on",
                                                                    "dia-guides-snap-off");

  g_signal_connect(G_OBJECT(ddisp->guide_snap_status), "toggled",
           G_CALLBACK (interface_toggle_snap_to_guides), ddisp);
  gtk_widget_set_tooltip_text(ddisp->guide_snap_status,
               _("Toggles snap-to-guides for this window."));
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->guide_snap_status,
              FALSE, FALSE, 0);


  /* Statusbar */
  ddisp->modified_status = gtk_statusbar_new ();

  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->modified_status,
		      TRUE, TRUE, 0);

  gtk_grid_attach (GTK_GRID (grid), status_hbox, 0, 3, 3, 1);

  display_rulers_show (ddisp);
  gtk_widget_show (ddisp->zoom_status);
  gtk_widget_show (zoom_hbox);
  gtk_widget_show (zoom_label);
  gtk_widget_show (ddisp->grid_status);
  gtk_widget_show (ddisp->mainpoint_status);
  gtk_widget_show (ddisp->guide_snap_status);
  gtk_widget_show (ddisp->modified_status);
  gtk_widget_show (status_hbox);
  gtk_widget_show (grid);
  if (use_mbar)
  {
      gtk_widget_show (ddisp->menu_bar);
      gtk_widget_show (root_vbox);
  }
  gtk_widget_show (ddisp->shell);

  gtk_widget_get_allocation (ddisp->hrule, &alloc);

  /* before showing up, checking canvas's REAL size */
  if (use_mbar && alloc.width > width) {
    /* The menubar is not shrinkable, so the shell will have at least
     * the menubar's width. If the diagram's requested width is smaller,
     * the canvas will be enlarged to fit the place. In this case, we
     * need to adjust the horizontal scrollbar according to the size
     * that will be allocated, which the same as the hrule got.
     */

    width = alloc.width;

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
 * ddisplay_update_rulers:
 * @ddisp: The #DDisplay to hide the rulers on.
 * @extents: the diagram extents
 * @visible: the visible area
 *
 * Adapt the rulers to current field of view
 *
 * Since: dawn-of-time
 */
void
ddisplay_update_rulers (DDisplay           *ddisp,
                        const DiaRectangle *extents,
                        const DiaRectangle *visible)
{
  dia_ruler_set_range (DIA_RULER (ddisp->hrule),
                       visible->left,
                       visible->right,
                       0.0f /* position*/,
                       MAX (extents->right, visible->right)/* max_size*/);
  dia_ruler_set_range (DIA_RULER (ddisp->vrule),
                       visible->top,
                       visible->bottom,
                       0.0f /*        position*/,
                       MAX (extents->bottom, visible->bottom)/* max_size*/);
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


/**
 * create_integrated_ui:
 *
 * Create integrated user interface
 *
 * Since: dawn-of-time
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
  gchar *version;

  GtkWidget *layer_view;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_ref (window);

  version = g_strdup_printf ("Dia v%s", dia_version_string ());
  gtk_window_set_title (GTK_WINDOW (window), version);
  g_clear_pointer (&version, g_free);

  /* hint to window manager on X so the wm can put the window in the same
     as it was when the application shut down                             */
  gtk_window_set_role (GTK_WINDOW (window), DIA_MAIN_WINDOW);

  gtk_window_set_default_size (GTK_WINDOW (window), 146, 349);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (toolbox_delete),
		      window);

  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (toolbox_destroy),
		      window);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
  gtk_widget_show (main_vbox);

  /* Application Statusbar */
  statusbar = gtk_statusbar_new ();
  gtk_box_pack_end (GTK_BOX (main_vbox), statusbar, FALSE, TRUE, 0);
  /* HBox for everything below the menubar and toolbars */
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_end (GTK_BOX (main_vbox), hbox, TRUE, TRUE, 0);
  gtk_widget_show (hbox);

  /* Layer View  */
  layer_view = create_layer_view_widget ();
  gtk_box_pack_end (GTK_BOX (hbox), layer_view, FALSE, FALSE, 0);

  /* Diagram Notebook */
  notebook = gtk_notebook_new ();
  gtk_box_pack_end (GTK_BOX (hbox), notebook, TRUE, TRUE, 0);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);

  g_signal_connect (G_OBJECT (notebook),
                    "switch-page",
                    G_CALLBACK (notebook_switch_page),
                    NULL);

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
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);

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
 * create_toolbox:
 *
 * Create toolbox component for distributed user interface
 *
 * Since: dawn-of-time
 */
void
create_toolbox (void)
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *wrapbox;
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_object_ref(window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia");
  gtk_window_set_role (GTK_WINDOW (window), "toolbox_window");
  gtk_window_set_default_size(GTK_WINDOW(window), 146, 349);
  g_signal_connect (G_OBJECT (window), "delete_event",
		    G_CALLBACK (toolbox_delete), window);
  g_signal_connect (G_OBJECT (window), "destroy",
		    G_CALLBACK (toolbox_destroy), window);

  main_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
  gtk_container_add (GTK_CONTAINER (window), main_vbox);

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
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
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
  if (ui.toolbar) {
    return gtk_widget_get_visible (GTK_WIDGET (ui.toolbar));
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
  if (ui.layer_view) {
    return gtk_widget_get_visible (GTK_WIDGET (ui.layer_view))? TRUE : FALSE;
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


static gboolean
_ddisplay_hruler_button_press (GtkWidget *widget,
                               GdkEventButton *event,
                               DDisplay *ddisp)
{
  return _ddisplay_ruler_button_press (widget, event, ddisp,
                                       GTK_ORIENTATION_HORIZONTAL);
}


static gboolean
_ddisplay_vruler_button_press (GtkWidget *widget,
                               GdkEventButton *event,
                               DDisplay *ddisp)
{
  return _ddisplay_ruler_button_press (widget, event, ddisp,
                                       GTK_ORIENTATION_VERTICAL);
}


static gboolean
_ddisplay_ruler_button_press (GtkWidget *widget,
                              GdkEventButton *event,
                              DDisplay *ddisp,
                              GtkOrientation orientation)
{
  /* Start adding a new guide if the left button was pressed. */
  if (event->type == GDK_BUTTON_PRESS && event->button == 1) {
      _guide_tool_start_new (ddisp, orientation);
  }

  return FALSE;
}


static gboolean
_ddisplay_ruler_button_release (GtkWidget *widget,
                                 GdkEventButton *event,
                                 DDisplay *ddisp)
{
  /* Hack to get this triggered. */
  if(active_tool->type == GUIDE_TOOL)
  {
    if (active_tool->button_release_func)
    {
      (*active_tool->button_release_func) (active_tool, event, ddisp);
    }
  }

  return FALSE;
}


static gboolean
_ddisplay_hruler_motion_notify (GtkWidget *widget,
    GdkEventMotion *event,
    DDisplay *ddisp)
{
  /* Hack to get this triggered. */
  if(active_tool->type == GUIDE_TOOL) {
    if (active_tool->motion_func) {

      /* Minus ruler height. */
      GtkRequisition ruler_requisition;
      gtk_widget_get_preferred_size (widget, NULL, &ruler_requisition);
      guide_tool_set_ruler_height(active_tool, ruler_requisition.height);

      /* Do the move. */
      (*active_tool->motion_func) (active_tool, event, ddisp);
    }
  }

  return FALSE;
}


static gboolean
_ddisplay_vruler_motion_notify (GtkWidget *widget,
    GdkEventMotion *event,
    DDisplay *ddisp)
{
  /* Hack to get this triggered. */
  if(active_tool->type == GUIDE_TOOL) {
    if (active_tool->motion_func) {

      /* Minus ruler width. */
      GtkRequisition ruler_requisition;
      gtk_widget_get_preferred_size (widget, NULL, &ruler_requisition);
      guide_tool_set_ruler_height(active_tool, ruler_requisition.width);

      /* Do the move. */
      (*active_tool->motion_func) (active_tool, event, ddisp);
    }
  }

  return FALSE;
}


double
parse_zoom (const char *zoom)
{
  static GRegex *extract_zoom = NULL;
  GMatchInfo *match_info;
  char *num;
  double res = -1;

  if (g_once_init_enter (&extract_zoom)) {
    GError *error = NULL;
    GRegex *regex = g_regex_new ("%?(\\d*)%?", G_REGEX_OPTIMIZE, 0, &error);

    if (error) {
      g_critical ("Failed to prepare regex: %s", error->message);

      g_clear_error (&error);
    }

    g_once_init_leave (&extract_zoom, regex);
  }

  g_regex_match (extract_zoom, zoom, 0, &match_info);

  if (!g_match_info_matches (match_info)) {
    return -1;
  }

  num = g_match_info_fetch (match_info, 1);

  res = g_ascii_strtod (num, NULL);

  g_clear_pointer (&num, g_free);
  g_match_info_free (match_info);

  return res * 10;
}
