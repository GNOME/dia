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

#include <stdio.h>
#include <string.h>

#include "diagram.h"
#include "layer_dialog.h"
#include "interface.h"
#include "display.h"
#include "pixmaps.h"
#include "preferences.h"
#include "commands.h"
#include "dia_dirs.h"
#include "diagram_tree_window.h"
#include "intl.h"
#include <gdk-pixbuf/gdk-pixbuf.h>

static const GtkTargetEntry create_object_targets[] = {
  { "application/x-dia-object", 0, 0 },
};

ToolButton tool_data[] =
{
  { (char **) arrow_xpm,
    N_("Modify object(s)"),
    N_("Modify"),
    { MODIFY_TOOL, NULL, NULL}
  },
  { (char **) magnify_xpm,
    N_("Magnify"),
    N_("Magnify"),
    { MAGNIFY_TOOL, NULL, NULL}
  },
  { (char **) scroll_xpm,
    N_("Scroll around the diagram"),
    N_("Scroll"),
    { SCROLL_TOOL, NULL, NULL}
  },
  { NULL,
    N_("Text"),
    N_("Text"),
    { CREATE_OBJECT_TOOL, "Standard - Text", NULL }
  },
  { NULL,
    N_("Box"),
    N_("Box"),
    { CREATE_OBJECT_TOOL, "Standard - Box", NULL }
  },
  { NULL,
    N_("Ellipse"),
    N_("Ellipse"),
    { CREATE_OBJECT_TOOL, "Standard - Ellipse", NULL }
  },
  { NULL,
    N_("Polygon"),
    N_("Polygon"),
    { CREATE_OBJECT_TOOL, "Standard - Polygon", NULL }
  },
  { NULL,
    N_("Beziergon"),
    N_("Beziergon"),
    { CREATE_OBJECT_TOOL, "Standard - Beziergon", NULL }
  },
  { NULL,
    N_("Line"),
    N_("Line"),
    { CREATE_OBJECT_TOOL, "Standard - Line", NULL }
  },
  { NULL,
    N_("Arc"),
    N_("Arc"),
    { CREATE_OBJECT_TOOL, "Standard - Arc", NULL }
  },
  { NULL,
    N_("Zigzagline"),
    N_("Zigzagline"),
    { CREATE_OBJECT_TOOL, "Standard - ZigZagLine", NULL }
  },
  { NULL,
    N_("Polyline"),
    N_("Polyline"),
    { CREATE_OBJECT_TOOL, "Standard - PolyLine", NULL }
  },
  { NULL,
    N_("Bezierline"),
    N_("Bezierline"),
    { CREATE_OBJECT_TOOL, "Standard - BezierLine", NULL }
  },
  { NULL,
    N_("Image"),
    N_("Image"),
    { CREATE_OBJECT_TOOL, "Standard - Image", NULL }
  }
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (ToolButton))
const int num_tools = NUM_TOOLS;

#define COLUMNS   4
#define ROWS      3

static GtkWidget *toolbox_shell = NULL;
static GtkWidget *tool_widgets[NUM_TOOLS];
/*static*/ GtkTooltips *tool_tips;
static GSList *tool_group = NULL;

GtkWidget *modify_tool_button;

static void
grid_toggle_snap(GtkWidget *widget, gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;
  ddisplay_set_snap_to_grid(ddisp, 
			    gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
}

/*  The popup shell is a pointer to the display shell that posted the latest
 *  popup menu.  When this is null, and a command is invoked, then the
 *  assumption is that the command was a result of a keyboard accelerator
 */
GtkWidget *popup_shell = NULL;

static gint
origin_button_press(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  DDisplay *ddisp = (DDisplay *)data;

  display_set_active(ddisp);
  ddisplay_popup_menu(ddisp, event);

  /* stop the signal emission so the button doesn't grab the
   * pointer from us */
  gtk_signal_emit_stop_by_name(GTK_OBJECT(widget), "button_press_event");

  return FALSE;
}

static void
zoom_activate_callback(GtkWidget *item, gpointer user_data) {
  DDisplay *ddisp = (DDisplay *)user_data;
  const gchar *zoom_text =
      gtk_entry_get_text(GTK_ENTRY(gtk_object_get_user_data(GTK_OBJECT(ddisp->zoom_status))));
  float zoom_amount, magnify;
  gchar *zoomamount = g_object_get_data(G_OBJECT(item), "zoomamount");
  if (zoomamount != NULL) {
    zoom_text = zoomamount;
  }

  if (sscanf(zoom_text, "%f", &zoom_amount) == 1) {
    Point middle;
    Rectangle *visible;

    magnify = (zoom_amount*DDISPLAY_NORMAL_ZOOM/100.0)/ddisp->zoom_factor;
    if (fabs(magnify - 1.0) > 0.000001) {
      visible = &ddisp->visible;
      middle.x = visible->left*0.5 + visible->right*0.5;
      middle.y = visible->top*0.5 + visible->bottom*0.5;
      ddisplay_zoom(ddisp, &middle, magnify);
    }
  }
}

static void
zoom_add_zoom_amount(GtkWidget *menu, gchar *text, DDisplay *ddisp) {
  GtkWidget *menuitem = gtk_menu_item_new_with_label(text);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
  gtk_signal_connect(GTK_OBJECT(menuitem), 
		     "activate", (GtkSignalFunc)zoom_activate_callback,
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
  gtk_signal_emit_stop_by_name(GTK_OBJECT(button), "button_press_event");
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
  gtk_signal_connect (GTK_OBJECT (entry), "activate",
		      (GtkSignalFunc) zoom_activate_callback,
		      ddisp);
  gtk_box_pack_start_defaults(GTK_BOX(combo), entry);
  gtk_object_set_user_data(GTK_OBJECT(combo), entry);
  gtk_entry_set_width_chars(GTK_ENTRY(entry), 8);
  gtk_widget_show(entry);

  button = gtk_button_new();
  GTK_WIDGET_UNSET_FLAGS(button, GTK_CAN_FOCUS);
  arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(button), arrow);
  gtk_box_pack_start_defaults(GTK_BOX(combo), button);
  gtk_object_set_user_data(GTK_OBJECT(combo), entry);
  gtk_widget_show_all(button);

  menu = gtk_menu_new();
  zoom_add_zoom_amount(menu, "400%", ddisp);
  zoom_add_zoom_amount(menu, "283%", ddisp);
  zoom_add_zoom_amount(menu, "200%", ddisp);
  zoom_add_zoom_amount(menu, "141%", ddisp);
  zoom_add_zoom_amount(menu, "100%", ddisp);
  zoom_add_zoom_amount(menu, "85%", ddisp);
  zoom_add_zoom_amount(menu, "70.7%", ddisp);
  zoom_add_zoom_amount(menu, "50%", ddisp);
  zoom_add_zoom_amount(menu, "35.4%", ddisp);
  zoom_add_zoom_amount(menu, "25%", ddisp);

  gtk_widget_show_all(menu);

  gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
		      (GtkSignalFunc) zoom_popup_menu,
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

inline int round_up (double x)
{
  if (x - (int) x > 0.001)
    return (int) x + 1;
  else
    return (int) x ;
}

static void
display_data_received_callback (GtkWidget *widget, GdkDragContext *context,
				gint x, gint y, GtkSelectionData *data,
				guint info, guint time, DDisplay *ddisp)
{
  if (data->format == 8 && data->length == sizeof(ToolButtonData *) &&
      gtk_drag_get_source_widget(context) != NULL) {
    ToolButtonData *tooldata = *(ToolButtonData **)data->data;

    /* g_message("Tool drop %s at (%d, %d)", (gchar *)tooldata->extra_data, x, y);*/
    ddisplay_drop_object(ddisp, x, y,
			 object_get_type((gchar *)tooldata->extra_data),
			 tooldata->user_data);

    gtk_drag_finish (context, TRUE, FALSE, time);
  } else
    gtk_drag_finish (context, FALSE, FALSE, time);
}

void
create_display_shell(DDisplay *ddisp,
		     int width, int height,
		     char *title, int use_mbar, int top_level_window)
{
  GtkWidget *table, *widget;
  GtkWidget *status_hbox;
  GtkWidget *root_vbox = NULL;
  GtkWidget *zoom_hbox, *zoom_label;
  int s_width, s_height;

  s_width = gdk_screen_width ();
  s_height = gdk_screen_height ();
  if (width > s_width)
    width = s_width;
  if (height > s_height)
    width = s_width;

  /*  The adjustment datums  */
  ddisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, (width-1)/4, width-1));
  ddisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, (height-1)/4, height-1));

  /*  The toplevel shell */
  if (top_level_window) {
    ddisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
    gtk_window_set_role (GTK_WINDOW (ddisp->shell), "diagram_window");
  } else {
    ddisp->shell = gtk_event_box_new ();
  }
  gtk_object_set_user_data (GTK_OBJECT (ddisp->shell), (gpointer) ddisp);

  gtk_widget_set_events (ddisp->shell,
			 GDK_POINTER_MOTION_MASK |
			 GDK_POINTER_MOTION_HINT_MASK |
			 GDK_FOCUS_CHANGE_MASK);
                      /* GDK_ALL_EVENTS_MASK */

  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "delete_event",
                      GTK_SIGNAL_FUNC (ddisplay_delete),
                      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "destroy",
                      GTK_SIGNAL_FUNC (ddisplay_destroy),
                      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "focus_out_event",
		      GTK_SIGNAL_FUNC (ddisplay_focus_out_event),
		      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "focus_in_event",
		      GTK_SIGNAL_FUNC (ddisplay_focus_in_event),
		      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "realize",
                      GTK_SIGNAL_FUNC (ddisplay_realize),
                      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "unrealize",
                      GTK_SIGNAL_FUNC (ddisplay_unrealize),
		      ddisp);
/*FIXME?:
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "size_allocate",
		      GTK_SIGNAL_FUNC (ddisplay_size_allocate),
		      ddisp);
*/

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
      GTK_WIDGET_UNSET_FLAGS(ddisp->origin, GTK_CAN_FOCUS);
      widget = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
      gtk_container_add(GTK_CONTAINER(ddisp->origin), widget);
      gtk_tooltips_set_tip(tool_tips, widget, _("Diagram menu."), NULL);
      gtk_widget_show(widget);
      gtk_signal_connect(GTK_OBJECT(ddisp->origin), "button_press_event",
		     GTK_SIGNAL_FUNC(origin_button_press), ddisp);
  }
  else {
      ddisp->origin = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME (ddisp->origin), GTK_SHADOW_OUT);
  }
  

  ddisp->hrule = gtk_hruler_new ();
  gtk_signal_connect_object (GTK_OBJECT (ddisp->shell), "motion_notify_event",
                             (GtkSignalFunc) GTK_WIDGET_GET_CLASS (ddisp->hrule)->motion_notify_event,
                             GTK_OBJECT (ddisp->hrule));

  ddisp->vrule = gtk_vruler_new ();
  gtk_signal_connect_object (GTK_OBJECT (ddisp->shell), "motion_notify_event",
                             (GtkSignalFunc) GTK_WIDGET_GET_CLASS (ddisp->vrule)->motion_notify_event,
                             GTK_OBJECT (ddisp->vrule));

  ddisp->hsb = gtk_hscrollbar_new (ddisp->hsbdata);
  GTK_WIDGET_UNSET_FLAGS (ddisp->hsb, GTK_CAN_FOCUS);
  ddisp->vsb = gtk_vscrollbar_new (ddisp->vsbdata);
  GTK_WIDGET_UNSET_FLAGS (ddisp->vsb, GTK_CAN_FOCUS);


  /*  set up the scrollbar observers  */
  gtk_signal_connect (GTK_OBJECT (ddisp->hsbdata), "value_changed",
		      (GtkSignalFunc) ddisplay_hsb_update,
		      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->vsbdata), "value_changed",
		      (GtkSignalFunc) ddisplay_vsb_update,
		      ddisp);

  ddisp->canvas = gtk_drawing_area_new ();
  /* Dia's canvas does it' double buffering alone so switch off GTK's */
  gtk_widget_set_double_buffered (ddisp->canvas, FALSE);
  gtk_drawing_area_size (GTK_DRAWING_AREA (ddisp->canvas), width, height);
  gtk_widget_set_events (ddisp->canvas, CANVAS_EVENT_MASK);
  GTK_WIDGET_SET_FLAGS (ddisp->canvas, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (ddisp->canvas), "event",
		      (GtkSignalFunc) ddisplay_canvas_events,
		      ddisp);

  gtk_drag_dest_set(ddisp->canvas, GTK_DEST_DEFAULT_ALL,
		    create_object_targets, 1, GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (ddisp->canvas), "drag_drop",
		      GTK_SIGNAL_FUNC(display_drop_callback), NULL);
  gtk_signal_connect (GTK_OBJECT (ddisp->canvas), "drag_data_received",
		      GTK_SIGNAL_FUNC(display_data_received_callback), ddisp);

  gtk_object_set_user_data (GTK_OBJECT (ddisp->canvas), (gpointer) ddisp);
  /*  pack all the widgets  */
  gtk_table_attach (GTK_TABLE (table), ddisp->origin, 0, 1, 0, 1,
                    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->hrule, 1, 2, 0, 1,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->vrule, 0, 1, 1, 2,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->canvas, 1, 2, 1, 2,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->hsb, 0, 2, 2, 3,
                    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), ddisp->vsb, 2, 3, 0, 2,
                    GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);

  menus_get_image_menu (NULL, &ddisp->accel_group);
  if (top_level_window)
    gtk_window_add_accel_group(GTK_WINDOW(ddisp->shell), ddisp->accel_group);
  if (use_mbar) 
  {
    GString *path;
    char *display = "<DisplayMBar>";

    menus_get_image_menubar(&ddisp->menu_bar, &ddisp->mbar_item_factory);
    gtk_box_pack_start (GTK_BOX (root_vbox), ddisp->menu_bar, FALSE, TRUE, 0);

    path = g_string_new (display);
    g_string_append (path,"/View/Show Rulers");
    ddisp->rulers       = menus_get_item_from_path(path->str, ddisp->mbar_item_factory);
    g_string_append (g_string_assign(path, display),"/View/Show Grid");
    ddisp->visible_grid = menus_get_item_from_path(path->str, ddisp->mbar_item_factory);
    g_string_append (g_string_assign(path, display),"/View/Snap To Grid");
    ddisp->snap_to_grid = menus_get_item_from_path(path->str, ddisp->mbar_item_factory);
    g_string_append (g_string_assign(path, display),"/View/Show Connection Points");
    ddisp->show_cx_pts_mitem  = menus_get_item_from_path(path->str, ddisp->mbar_item_factory);

#ifdef HAVE_LIBART
    g_string_append (g_string_assign(path, display),"/View/AntiAliased");
    ddisp->antialiased  = menus_get_item_from_path(path->str, ddisp->mbar_item_factory);
#endif

    menus_initialize_updatable_items (&ddisp->updatable_menu_items, ddisp->mbar_item_factory, display);
    g_string_free (path,FALSE);
  }

  /* the statusbars */
  status_hbox = gtk_hbox_new (FALSE, 2);

  /*
    gtk_signal_connect(GTK_OBJECT(ddisp->origin), "button_press_event",
    GTK_SIGNAL_FUNC(origin_button_press), ddisp);
  */

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
  ddisp->grid_status = gtk_toggle_button_new();
  
  g_signal_connect(G_OBJECT(ddisp->grid_status), "toggled",
		   grid_toggle_snap, ddisp);
  gtk_tooltips_set_tip(tool_tips, ddisp->grid_status,
		       _("Toggles snap-to-grid for this window."), NULL);
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->grid_status,
		      FALSE, FALSE, 0);

  /* Statusbar */
  ddisp->modified_status = gtk_statusbar_new ();

  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->modified_status,
		      TRUE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), status_hbox, 0, 3, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (ddisp->hsb);
  gtk_widget_show (ddisp->vsb);
  gtk_widget_show (ddisp->origin);
  gtk_widget_show (ddisp->hrule);
  gtk_widget_show (ddisp->vrule);
  gtk_widget_show (ddisp->canvas);
  gtk_widget_show (ddisp->zoom_status);
  gtk_widget_show (zoom_hbox);
  gtk_widget_show (zoom_label);
  gtk_widget_show (ddisp->grid_status);
  gtk_widget_show (ddisp->modified_status);
  gtk_widget_show (status_hbox);
  gtk_widget_show (table);
  if (use_mbar) 
  {
      gtk_widget_show (ddisp->menu_bar);
      gtk_widget_show (root_vbox);
  }
  gtk_widget_show (ddisp->shell);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (ddisp->canvas);
}

void
tool_select_update (GtkWidget *w,
		     gpointer   data)
{
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if (tooldata == NULL) {
    g_warning(_("NULL tooldata in tool_select_update"));
    return;
  }

  if (tooldata->type != -1) {
    tool_select (tooldata->type, tooldata->extra_data, tooldata->user_data,
                 w);
  }
}

static gint
tool_button_press (GtkWidget      *w,
		    GdkEventButton *event,
		    gpointer        data)
{
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if ((event->type == GDK_2BUTTON_PRESS) &&
      (event->button == 1)) {
    tool_options_dialog_show (tooldata->type, tooldata->extra_data, 
                              tooldata->user_data, w);
    return TRUE;
  }

  return FALSE;
}

static void
tool_drag_data_get (GtkWidget *widget, GdkDragContext *context,
		    GtkSelectionData *selection_data, guint info,
		    guint32 time, ToolButtonData *tooldata)
{
  if (info == 0) {
    gtk_selection_data_set(selection_data, selection_data->target,
			   8, (guchar *)&tooldata, sizeof(ToolButtonData *));
  }
}

static void
tool_setup_drag_source(GtkWidget *button, ToolButtonData *tooldata,
		       GdkPixmap *pixmap, GdkBitmap *mask)
{
  g_return_if_fail(tooldata->type == CREATE_OBJECT_TOOL);

  gtk_drag_source_set(button, GDK_BUTTON1_MASK,
		      create_object_targets, 1,
		      GDK_ACTION_DEFAULT|GDK_ACTION_COPY);
  gtk_signal_connect(GTK_OBJECT(button), "drag_data_get",
		     GTK_SIGNAL_FUNC(tool_drag_data_get), tooldata);
  if (pixmap)
    gtk_drag_source_set_icon(button, gtk_widget_get_colormap(button),
			     pixmap, mask);
}

void 
tool_select_callback(GtkWidget *widget, gpointer data) {
  ToolButtonData *tooldata = (ToolButtonData *)data;

  if (tooldata == NULL) {
    g_warning("NULL tooldata in tool_select_callback");
    return;
  }

  if (tooldata->type != -1) {
    tool_select (tooldata->type, tooldata->extra_data, 
                 tooldata->user_data,widget);    
  }
}

static void
create_tools(GtkWidget *parent)
{
  GtkWidget *button;
  GtkWidget *pixmapwidget;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  char **pixmap_data;
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    tool_widgets[i] = button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

    gtk_wrap_box_pack(GTK_WRAP_BOX(parent), button,
		      TRUE, TRUE, FALSE, TRUE);

    if (tool_data[i].callback_data.type == MODIFY_TOOL) {
      modify_tool_button = GTK_WIDGET(button);
    }
    
    if (tool_data[i].icon_data==NULL) {
      ObjectType *type;
      type =
	object_get_type((char *)tool_data[i].callback_data.extra_data);
      if (type == NULL)
	pixmap_data = tool_data[0].icon_data;
      else
	pixmap_data = type->pixmap;
    } else {
      pixmap_data = tool_data[i].icon_data;
    }
    
    style = gtk_widget_get_style(button);
    pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(button), &mask,
		&style->bg[GTK_STATE_NORMAL], pixmap_data);
    
    pixmapwidget = gtk_pixmap_new(pixmap, mask);
    
    /* GTKBUG:? padding changes */
    gtk_misc_set_padding(GTK_MISC(pixmapwidget), 2, 2);
    
    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
			GTK_SIGNAL_FUNC (tool_select_update),
			&tool_data[i].callback_data);
    
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			GTK_SIGNAL_FUNC (tool_button_press),
			&tool_data[i].callback_data);

    if (tool_data[i].callback_data.type == CREATE_OBJECT_TOOL)
      tool_setup_drag_source(button, &tool_data[i].callback_data,
			     pixmap, mask);

    gdk_pixmap_unref(pixmap);
    if (mask) gdk_bitmap_unref(mask);

    tool_data[i].callback_data.widget = button;

    gtk_tooltips_set_tip (tool_tips, button,
			  gettext(tool_data[i].tool_desc), NULL);
    
    gtk_widget_show (pixmapwidget);
    gtk_widget_show (button);
  }
}

static GtkWidget *sheet_option_menu;
static GtkWidget *sheet_menu;
static GtkWidget *sheet_wbox;

#if 0
static GtkWidget *
create_sheet_page(GtkWidget *parent, Sheet *sheet)
{
  GSList *list;
  SheetObject *sheet_obj;
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *pixmapwidget;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  ToolButtonData *data;
  GtkStyle *style;
  GtkWidget *scrolled_window;
  int i;
  int rows;

  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy( GTK_SCROLLED_WINDOW(scrolled_window),
				  GTK_POLICY_AUTOMATIC,
				  GTK_POLICY_AUTOMATIC);

  rows = ceil(g_slist_length(sheet->objects) / (double)COLUMNS);
  if (rows<1)
    rows = 1;
  
  table = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(table), COLUMNS * 1.0 / rows);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(table), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(table), GTK_JUSTIFY_LEFT);

  style = gtk_widget_get_style(parent);

  i = 0;
  list = sheet->objects;
  while (list != NULL) {
    sheet_obj = (SheetObject *) list->data;
    
    
    if (sheet_obj->pixmap != NULL) {
      pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(parent), &mask, 
			&style->bg[GTK_STATE_NORMAL], sheet_obj->pixmap);
    } else if (sheet_obj->pixmap_file != NULL) {
      pixmap = gdk_pixmap_colormap_create_from_xpm(NULL,
			gtk_widget_get_colormap(parent), &mask,
			&style->bg[GTK_STATE_NORMAL], sheet_obj->pixmap_file);
    } else {
      ObjectType *type;
      type = object_get_type(sheet_obj->object_type);
      pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(parent), &mask, 
			&style->bg[GTK_STATE_NORMAL], type->pixmap);
    }

    pixmapwidget = gtk_pixmap_new(pixmap, mask);

    
    button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);


    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(table), button,
		      FALSE, TRUE, FALSE, sheet_obj->line_break);

    /* This is a Memory leak, these can't be freed.
       Doesn't matter much anyway... */

    data = g_new(ToolButtonData, 1);
    data->type = CREATE_OBJECT_TOOL;
    data->extra_data = sheet_obj->object_type;
    data->user_data = sheet_obj->user_data;
    
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
			GTK_SIGNAL_FUNC (tool_select_update),
			data);
    
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			GTK_SIGNAL_FUNC (tool_button_press),
			data);

    gtk_tooltips_set_tip (tool_tips, button,
			  gettext(sheet_obj->description), NULL);

    list = g_slist_next(list);
    i++;
  }

  gtk_widget_show(table);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), table);
  
  return scrolled_window;
}
#endif

gchar *interface_current_sheet_name;

static void
fill_sheet_wbox(GtkWidget *menu_item, Sheet *sheet)
{
  int rows;
  GtkStyle *style;
  GSList *tmp;
  GtkWidget *first_button = NULL;

  gtk_container_foreach(GTK_CONTAINER(sheet_wbox),
			(GtkCallback)gtk_widget_destroy, NULL);
  tool_group = gtk_radio_button_group(GTK_RADIO_BUTTON(tool_widgets[0]));

  /* Remember sheet 'name' for 'Sheets and Objects' dialog */
  interface_current_sheet_name = sheet->name;

  /* set the aspect ratio on the wbox */
  rows = ceil(g_slist_length(sheet->objects) / (double)COLUMNS);
  if (rows<1) rows = 1;
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(sheet_wbox),
				COLUMNS * 1.0 / rows);
  style = gtk_widget_get_style(sheet_wbox);
  for (tmp = sheet->objects; tmp != NULL; tmp = tmp->next) {
    SheetObject *sheet_obj = tmp->data;
    GdkPixmap *pixmap = NULL;
    GdkBitmap *mask = NULL;
    GtkWidget *pixmapwidget;
    GtkWidget *button;
    ToolButtonData *data;

    if (sheet_obj->pixmap != NULL) {
      pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(sheet_wbox), &mask, 
			&style->bg[GTK_STATE_NORMAL], sheet_obj->pixmap);
    } else if (sheet_obj->pixmap_file != NULL) {
      GdkPixbuf *pixbuf;
      GError* gerror = NULL;
      
      pixbuf = gdk_pixbuf_new_from_file(sheet_obj->pixmap_file, &gerror);
      if (pixbuf != NULL) {
          gdk_pixbuf_render_pixmap_and_mask(pixbuf, &pixmap, &mask, 1.0);
          gdk_pixbuf_unref(pixbuf);
      } else {
          
          g_warning("failed to load pixbuf for file %s; cause=%s",
                    sheet_obj->pixmap_file,gerror?gerror->message:"[NULL]");
      }
    } else {
      ObjectType *type;
      type = object_get_type(sheet_obj->object_type);
      pixmap = gdk_pixmap_colormap_create_from_xpm_d(NULL,
			gtk_widget_get_colormap(sheet_wbox), &mask, 
			&style->bg[GTK_STATE_NORMAL], type->pixmap);
    }
    if (pixmap) {
      pixmapwidget = gtk_pixmap_new(pixmap, mask);
      gdk_pixmap_unref(pixmap);
      if (mask) gdk_bitmap_unref(mask);
    } else {
      pixmapwidget = gtk_type_new(gtk_pixmap_get_type());
    }

    button = gtk_radio_button_new (tool_group);
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));

    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    gtk_widget_show(pixmapwidget);

    gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(sheet_wbox), button,
		      FALSE, TRUE, FALSE, TRUE, sheet_obj->line_break);
    gtk_widget_show(button);

    data = g_new(ToolButtonData, 1);
    data->type = CREATE_OBJECT_TOOL;
    data->extra_data = sheet_obj->object_type;
    data->user_data = sheet_obj->user_data;
    gtk_object_set_data_full(GTK_OBJECT(button), "Dia::ToolButtonData",
			     data, (GdkDestroyNotify)g_free);
    if (first_button == NULL) first_button = button;
    
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
			GTK_SIGNAL_FUNC (tool_select_update), data);
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			GTK_SIGNAL_FUNC (tool_button_press), data);

    tool_setup_drag_source(button, data, pixmap, mask);

    gtk_tooltips_set_tip (tool_tips, button,
			  gettext(sheet_obj->description), NULL);
  }
  /* If the selection is in the old sheet, steal it */
  if (active_tool != NULL &&
      active_tool->type == CREATE_OBJECT_TOOL &&
      first_button != NULL)
    gtk_signal_emit_by_name(GTK_OBJECT(first_button), "toggled",
			    GTK_BUTTON(first_button), NULL);
}

void
fill_sheet_menu(void)
{
  GSList *tmp;

  gtk_container_foreach(GTK_CONTAINER(sheet_menu),
			(GtkCallback)gtk_widget_destroy, NULL);
  gtk_widget_ref(sheet_menu);
  gtk_option_menu_remove_menu(GTK_OPTION_MENU(sheet_option_menu));
  gtk_container_add(GTK_CONTAINER(sheet_menu), gtk_tearoff_menu_item_new());
  for (tmp = get_sheets_list(); tmp != NULL; tmp = tmp->next) {
    Sheet *sheet = tmp->data;
    GtkWidget *menuitem;

    if (sheet->objects == NULL)
      continue;

    menuitem = gtk_menu_item_new_with_label(gettext(sheet->name));
    gtk_signal_connect(GTK_OBJECT(menuitem), "activate",
		       GTK_SIGNAL_FUNC(fill_sheet_wbox), sheet);
    gtk_container_add(GTK_CONTAINER(sheet_menu), menuitem);
  }
  gtk_option_menu_set_menu(GTK_OPTION_MENU(sheet_option_menu), sheet_menu);
  gtk_widget_unref(sheet_menu);
  gtk_menu_item_activate(
	GTK_MENU_ITEM(GTK_OPTION_MENU(sheet_option_menu)->menu_item));
  gtk_widget_show_all(sheet_menu);
}

void
create_sheets(GtkWidget *parent)
{
  GtkWidget *separator;
  GtkWidget *label;
  GtkWidget *swin;
  
  separator = gtk_hseparator_new ();
  /* add a bit of padding around the separator */
  label = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(label), separator, TRUE, TRUE, 3);
  gtk_widget_show(label);

  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX(parent), label, TRUE,TRUE, FALSE,FALSE, TRUE);
  gtk_widget_show(separator);

  sheet_option_menu = gtk_option_menu_new();
  gtk_widget_set_usize(sheet_option_menu, 20, -1);
  sheet_menu = gtk_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(sheet_option_menu), sheet_menu);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), sheet_option_menu,
		    TRUE, TRUE, FALSE, FALSE, TRUE);
  gtk_widget_show(sheet_option_menu);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				 GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), swin, TRUE, TRUE, TRUE, TRUE, TRUE);
  gtk_widget_show(swin);

  sheet_wbox = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(sheet_wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(sheet_wbox), GTK_JUSTIFY_LEFT);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), sheet_wbox);
  gtk_widget_show(sheet_wbox);

  fill_sheet_menu();
}

static void
create_color_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GtkWidget *line_area;
  GdkPixmap *default_pixmap;
  GdkBitmap *default_mask;
  GdkPixmap *swap_pixmap;
  GdkBitmap *swap_mask;
  GtkStyle *style;
  GtkWidget *hbox;

  gtk_widget_ensure_style(parent);
  style = gtk_widget_get_style(parent);

  default_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &default_mask, 
		&style->bg[GTK_STATE_NORMAL], default_xpm);
  swap_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &swap_mask, 
		&style->bg[GTK_STATE_NORMAL], swap_xpm);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), frame, TRUE, TRUE, FALSE, FALSE, TRUE);
  gtk_widget_realize (frame);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  
  /* Color area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  col_area = color_area_create (54, 42, 
                                default_pixmap, default_mask, 
                                swap_pixmap, swap_mask);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);


  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

  gtk_tooltips_set_tip (tool_tips, col_area, 
			_("Foreground & background colors for new objects.  "
			  "The small black and white squares reset colors.  "
			  "The small arrows swap colors.  Double click to "
			  "change colors."),
                        NULL);

  gtk_widget_show (alignment);
  
  /* Linewidth area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  line_area = linewidth_area_create ();
  gtk_container_add (GTK_CONTAINER (alignment), line_area);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  gtk_tooltips_set_tip(tool_tips, line_area, _("Line widths.  Click on a line to set the default line width for new objects.  Double-click to set the line width more precisely."), NULL);
  gtk_widget_show (alignment);

  gtk_widget_show (col_area);
  gtk_widget_show (line_area);
  gtk_widget_show (hbox);
  gtk_widget_show (frame);
}

static void
change_start_arrow_style(Arrow arrow, gpointer user_data)
{
  attributes_set_default_start_arrow(arrow);
}
static void
change_end_arrow_style(Arrow arrow, gpointer user_data)
{
  attributes_set_default_end_arrow(arrow);
}
static void
change_line_style(LineStyle lstyle, real dash_length, gpointer user_data)
{
  attributes_set_default_line_style(lstyle, dash_length);
}

static void
create_lineprops_area(GtkWidget *parent)
{
  GtkWidget *chooser;

  chooser = dia_arrow_chooser_new(TRUE, change_start_arrow_style, NULL);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE, TRUE);
  gtk_tooltips_set_tip(tool_tips, chooser, _("Arrow style at the beginning of new lines.  Click to pick an arrow, or set arrow parameters with Details..."), NULL);
  gtk_widget_show(chooser);

  chooser = dia_line_chooser_new(change_line_style, NULL);
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, TRUE, TRUE, FALSE, TRUE);
  gtk_tooltips_set_tip(tool_tips, chooser, _("Line style for new lines.  Click to pick a line style, or set line style parameters with Details..."), NULL);
  gtk_widget_show(chooser);

  chooser = dia_arrow_chooser_new(FALSE, change_end_arrow_style, NULL);
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE);
  gtk_tooltips_set_tip(tool_tips, chooser, _("Arrow style at the end of new lines.  Click to pick an arrow, or set arrow parameters with Details..."), NULL);
  gtk_widget_show(chooser);
}

static void
toolbox_destroy (GtkWidget *widget, gpointer data)
{
  app_exit();
}

static void
toolbox_delete (GtkWidget *widget, GdkEvent *event, gpointer data)
{
  if (!app_is_embedded()) {
    gulong handlerid;
    /** Stop toolbox_destroy from being called */
    handlerid = g_signal_handler_find(widget, G_SIGNAL_MATCH_FUNC,
				      0, 0, NULL, toolbox_destroy, NULL);
    if (handlerid != 0)
      g_signal_handler_disconnect (GTK_OBJECT (widget), handlerid);
    app_exit();
  }
}


/* HB: file dnd stuff lent by The Gimp, not fully understood but working ...
 */
enum
{
  DIA_DND_TYPE_URI_LIST,
  DIA_DND_TYPE_TEXT_PLAIN,
};

static GtkTargetEntry toolbox_target_table[] =
{
  { "text/uri-list", 0, DIA_DND_TYPE_URI_LIST },
  { "text/plain", 0, DIA_DND_TYPE_TEXT_PLAIN }
};

static guint toolbox_n_targets = (sizeof (toolbox_target_table) /
                                 sizeof (toolbox_target_table[0]));

static void
dia_dnd_file_drag_data_received (GtkWidget        *widget,
                                 GdkDragContext   *context,
                                 gint              x,
                                 gint              y,
                                 GtkSelectionData *data,
                                 guint             info,
                                 guint             time)
{
  switch (context->action)
    {
    case GDK_ACTION_DEFAULT:
    case GDK_ACTION_COPY:
    case GDK_ACTION_MOVE:
    case GDK_ACTION_LINK:
    case GDK_ACTION_ASK:
    default:
      {
        Diagram *diagram = NULL;
        DDisplay *ddisp;
        gchar *sPath = NULL, *pFrom, *pTo; 

        pFrom = strstr((gchar *) data->data, "file:");
        while (pFrom) {
          GError *error = NULL;

          pTo = pFrom;
          while (*pTo != 0 && *pTo != 0xd && *pTo != 0xa) pTo ++;
          sPath = g_strndup(pFrom, pTo - pFrom);

          /* format changed with Gtk+2.0, use conversion */
          pFrom = g_filename_from_uri (sPath, NULL, &error);
          diagram = diagram_load (pFrom, NULL);
          g_free (pFrom);
          g_free(sPath);

          if (diagram != NULL) {
            diagram_update_extents(diagram);
            layer_dialog_set_diagram(diagram);
            
            ddisp = new_display(diagram);
          }

          pFrom = strstr(pTo, "file:");
        } /* while */
        gtk_drag_finish (context, TRUE, FALSE, time);
      }
      break;
    }
  return;
}

void
create_toolbox ()
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *wrapbox;
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;

#ifdef GNOME
  window = gnome_app_new ("Dia", _("Diagram Editor"));
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref (window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia v" VERSION);
#endif
  gtk_window_set_role (GTK_WINDOW (window), "toolbox_window");
  gtk_window_set_default_size(GTK_WINDOW(window), 146, 349);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      window);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (toolbox_destroy),
		      window);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
#ifdef GNOME
  gnome_app_set_contents(GNOME_APP(window), main_vbox);
#else
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
#endif
  gtk_widget_show (main_vbox);

  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();

  /*  gtk_tooltips_set_colors (tool_tips,
			   &colors[11],
			   &main_vbox->style->fg[GTK_STATE_NORMAL]);*/

  wrapbox = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(wrapbox), 144.0 / 318.0);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_LEFT);


  /* pack the rest of the stuff */
  gtk_box_pack_end (GTK_BOX (main_vbox), wrapbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (wrapbox), 0);
  gtk_widget_show (wrapbox);

  create_tools (wrapbox);
  create_sheets (wrapbox);
  create_color_area (wrapbox);
  create_lineprops_area (wrapbox);

  /* Setup toolbox area as file drop destination */
  gtk_drag_dest_set (wrapbox,
		     GTK_DEST_DEFAULT_ALL,
		     toolbox_target_table, toolbox_n_targets,
		     GDK_ACTION_COPY);
  gtk_signal_connect (GTK_OBJECT (wrapbox), "drag_data_received",
                      GTK_SIGNAL_FUNC (dia_dnd_file_drag_data_received),
                      wrapbox);

  /* menus -- initialised afterwards, because initing the display menus
   * uses the tool buttons*/
  menus_get_toolbox_menubar(&menubar, &accel_group);
  gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
#ifdef GNOME
  gnome_app_set_menus(GNOME_APP(window), GTK_MENU_BAR(menubar));
#else
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);
#endif

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

void
create_tree_window(void)
{
  GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM
    (menus_get_item_from_path("<Toolbox>/File/Diagram tree", NULL));
  create_diagram_tree_window(&prefs.dia_tree, GTK_WIDGET(item));
}

GtkWidget *
interface_get_toolbox_shell(void)
{
  return toolbox_shell;
}

