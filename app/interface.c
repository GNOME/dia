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
/* $Header$ */

#include "config.h"

#include "interface.h"

#include "pixmaps.h"

#include "preferences.h"

#include "commands.h"

#include "dia_dirs.h"

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
    N_("Create Text"),
    N_("Text"),
    { CREATE_OBJECT_TOOL, "Standard - Text", NULL }
  },
  { NULL,
    N_("Create Box"),
    N_("Box"),
    { CREATE_OBJECT_TOOL, "Standard - Box", NULL }
  },
  { NULL,
    N_("Create Ellipse"),
    N_("Ellipse"),
    { CREATE_OBJECT_TOOL, "Standard - Ellipse", NULL }
  },
  { NULL,
    N_("Create Polygon"),
    N_("Polygon"),
    { CREATE_OBJECT_TOOL, "Standard - Polygon", NULL }
  },
  { NULL,
    N_("Create Beziergon"),
    N_("Beziergon"),
    { CREATE_OBJECT_TOOL, "Standard - Beziergon", NULL }
  },
  { NULL,
    N_("Create Line"),
    N_("Line"),
    { CREATE_OBJECT_TOOL, "Standard - Line", NULL }
  },
  { NULL,
    N_("Create Arc"),
    N_("Arc"),
    { CREATE_OBJECT_TOOL, "Standard - Arc", NULL }
  },
  { NULL,
    N_("Create Zigzagline"),
    N_("Zigzagline"),
    { CREATE_OBJECT_TOOL, "Standard - ZigZagLine", NULL }
  },
  { NULL,
    N_("Create Polyline"),
    N_("Polyline"),
    { CREATE_OBJECT_TOOL, "Standard - PolyLine", NULL }
  },
  { NULL,
    N_("Create Bezierline"),
    N_("Bezierline"),
    { CREATE_OBJECT_TOOL, "Standard - BezierLine", NULL }
  },
  { NULL,
    N_("Create Image"),
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
static GtkTooltips *tool_tips;
static GSList *tool_group = NULL;

GtkWidget *modify_tool_button;

/*  If we create *all* menus at startup, we can read a .menurc file and
 *  get persistent menu shortcuts.  Yummy!
 */
void
create_menus() {
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

  ddisplay_popup_menu(ddisp, event);
  return TRUE;
}

void
create_display_shell(DDisplay *ddisp,
		     int width, int height,
		     char *title, int top_level_window)
{
  GtkWidget *table, *widget;
  GtkWidget *status_hbox;
  int s_width, s_height;

  s_width = gdk_screen_width ();
  s_height = gdk_screen_height ();
  if (width > s_width)
    width = s_width;
  if (height > s_height)
    width = s_width;

  /*  The adjustment datums  */
  ddisp->hsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, width, 1, 1, width-1));
  ddisp->vsbdata = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, height, 1, 1, height-1));

  /*  The toplevel shell */
  if (top_level_window) {
    ddisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
    gtk_window_set_wmclass (GTK_WINDOW (ddisp->shell), "diagram_window",
			    "Dia");
    gtk_window_set_policy (GTK_WINDOW (ddisp->shell), TRUE, TRUE, TRUE);
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

  /* Clipboard handling signals */
  gtk_signal_connect (GTK_OBJECT(ddisp->shell), "selection_get",
		      GTK_SIGNAL_FUNC (get_selection_handler), NULL);
  gtk_signal_connect (GTK_OBJECT(ddisp->shell), "selection_received",
		      GTK_SIGNAL_FUNC (received_selection_handler), NULL);

  /*  the table containing all widgets  */
  table = gtk_table_new (4, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (ddisp->shell), table);

  /*  scrollbars, rulers, canvas, menu popup button  */
  ddisp->origin = gtk_button_new();
  GTK_WIDGET_UNSET_FLAGS(ddisp->origin, GTK_CAN_FOCUS);
  widget = gtk_arrow_new(GTK_ARROW_RIGHT, GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(ddisp->origin), widget);
  gtk_widget_show(widget);
  gtk_signal_connect(GTK_OBJECT(ddisp->origin), "button_press_event",
		     GTK_SIGNAL_FUNC(origin_button_press), ddisp);

  ddisp->hrule = gtk_hruler_new ();
  gtk_signal_connect_object (GTK_OBJECT (ddisp->shell), "motion_notify_event",
                             (GtkSignalFunc) GTK_WIDGET_CLASS (GTK_OBJECT (ddisp->hrule)->klass)->motion_notify_event,
                             GTK_OBJECT (ddisp->hrule));

  ddisp->vrule = gtk_vruler_new ();
  gtk_signal_connect_object (GTK_OBJECT (ddisp->shell), "motion_notify_event",
                             (GtkSignalFunc) GTK_WIDGET_CLASS (GTK_OBJECT (ddisp->vrule)->klass)->motion_notify_event,
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
  gtk_drawing_area_size (GTK_DRAWING_AREA (ddisp->canvas), width, height);
  gtk_widget_set_events (ddisp->canvas, CANVAS_EVENT_MASK);
  GTK_WIDGET_SET_FLAGS (ddisp->canvas, GTK_CAN_FOCUS);
  gtk_signal_connect (GTK_OBJECT (ddisp->canvas), "event",
		      (GtkSignalFunc) ddisplay_canvas_events,
		      ddisp);
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

  /*  the popup menu  */
#ifdef GNOME
  ddisp->popup = gnome_display_menus_create ();
  ddisp->accel_group = gnome_popup_menu_get_accel_group(GTK_MENU(ddisp->popup));
#else
  menus_get_image_menu (&ddisp->popup, &ddisp->accel_group);
#endif

  if (top_level_window) {
    gchar *accelfilename = dia_config_filename("menus/display");

    /*  the accelerator table/group for the popup */
    gtk_window_add_accel_group (GTK_WINDOW(ddisp->shell),
				ddisp->accel_group);
    
    if (accelfilename) {
      gtk_item_factory_parse_rc(accelfilename);
      g_free(accelfilename);
    }
  }


  /* the statusbars */
  status_hbox = gtk_hbox_new (FALSE, 2);

  ddisp->zoom_status = gtk_statusbar_new ();
  ddisp->modified_status = gtk_statusbar_new ();
  
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->zoom_status,
		      FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (status_hbox), ddisp->modified_status,
		      TRUE, TRUE, 
		      0);

  gtk_table_attach (GTK_TABLE (table), status_hbox, 0, 3, 3, 4,
                    GTK_FILL, GTK_FILL, 0, 0);


  gtk_widget_show (ddisp->hsb);
  gtk_widget_show (ddisp->vsb);
  gtk_widget_show (ddisp->origin);
  gtk_widget_show (ddisp->hrule);
  gtk_widget_show (ddisp->vrule);
  gtk_widget_show (ddisp->canvas);
  gtk_widget_show (ddisp->zoom_status);
  gtk_widget_show (ddisp->modified_status);
  gtk_widget_show (status_hbox);
  gtk_widget_show (table);
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
    tool_select (tooldata->type, tooldata->extra_data, tooldata->user_data);
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
    tool_options_dialog_show (tooldata->type, tooldata->extra_data, tooldata->user_data);
    return TRUE;
  }

  return FALSE;
}

void 
tool_select_callback(GtkWidget *widget, gpointer data) {
  ToolButtonData *tooldata = (ToolButtonData *)data;

  if (tooldata == NULL) {
    g_warning("NULL tooldata in tool_select_callback");
    return;
  }

  if (tooldata->type != -1) {
    tool_select (tooldata->type, tooldata->extra_data, tooldata->user_data);
    
  }
}

static void
create_tools(GtkWidget *parent)
{
#ifndef USE_WRAPBOX
  GtkWidget *table;
#endif
  GtkWidget *button;
  GtkWidget *pixmapwidget;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;

  char **pixmap_data;
  int i;

#ifndef USE_WRAPBOX
  table = gtk_table_new (ROWS, COLUMNS, FALSE);
  gtk_box_pack_start (GTK_BOX (parent), table, FALSE, TRUE, 0);
#endif

  for (i = 0; i < NUM_TOOLS; i++) {
    tool_widgets[i] = button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

#ifdef USE_WRAPBOX
    gtk_wrap_box_pack(GTK_WRAP_BOX(parent), button,
# if 1 /* HB: IHMO even 13 buttons look better when stretched to parent */
		      TRUE, TRUE, FALSE, TRUE);
# else
		      FALSE, TRUE, FALSE, TRUE);
# endif
#else
    gtk_table_attach (GTK_TABLE (table), button,
		      (i % COLUMNS), (i % COLUMNS) + 1,
		      (i / COLUMNS), (i / COLUMNS) + 1,
		      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		      GTK_FILL,
		      0, 0);
#endif

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
    
    gtk_misc_set_padding(GTK_MISC(pixmapwidget), 4, 4);
    
    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
			GTK_SIGNAL_FUNC (tool_select_update),
			&tool_data[i].callback_data);
    
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			GTK_SIGNAL_FUNC (tool_button_press),
			&tool_data[i].callback_data);

    tool_data[i].callback_data.widget = button;

    gtk_tooltips_set_tip (tool_tips, button,
			  gettext(tool_data[i].tool_desc), NULL);
    
    gtk_widget_show (pixmapwidget);
    gtk_widget_show (button);
  }
#ifndef USE_WRAPBOX
  gtk_widget_show(table);
#endif
}

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

  rows = (g_slist_length(sheet->objects) - 1) / COLUMNS + 1;
  if (rows<1)
    rows = 1;
  
#ifdef USE_WRAPBOX
  table = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(table), COLUMNS * 1.0 / rows);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(table), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(table), GTK_JUSTIFY_LEFT);
#else
  table = gtk_table_new (rows, COLUMNS, TRUE);
#endif

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
#ifdef USE_WRAPBOX
    gtk_wrap_box_pack(GTK_WRAP_BOX(table), button,
		      FALSE, TRUE, FALSE, TRUE);
    if (sheet_obj->line_break)
      gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(table),button,TRUE);
#else
    gtk_table_attach (GTK_TABLE (table), button,
		      (i % COLUMNS), (i % COLUMNS) + 1,
		      (i / COLUMNS), (i / COLUMNS) + 1,
		      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		      GTK_FILL,
		      0, 0);
#endif

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

static void
create_sheets(GtkWidget *parent)
{
  GtkWidget *notebook;
  GtkWidget *separator;
  GSList *list;
  Sheet *sheet;
  GtkWidget *child;
  GtkWidget *label;
  GtkWidget *menu_label;
  
  separator = gtk_hseparator_new ();
#ifdef USE_WRAPBOX
  /* add a bit of padding around the label */
  label = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(label), separator, TRUE, TRUE, 3);
  gtk_widget_show(label);

  gtk_wrap_box_pack (GTK_WRAP_BOX(parent), label, TRUE,TRUE, FALSE,FALSE);
  gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(parent), label, TRUE);
#else
  gtk_box_pack_start (GTK_BOX (parent), separator, FALSE, TRUE, 3);
#endif
  gtk_widget_show(separator);

  notebook = gtk_notebook_new ();
  /*
  gtk_signal_connect (GTK_OBJECT (notebook), "switch_page",
		      GTK_SIGNAL_FUNC (page_switch), NULL);
  */
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_popup_enable (GTK_NOTEBOOK (notebook));
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 1);
#ifdef USE_WRAPBOX
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), notebook, TRUE, TRUE, TRUE, TRUE);
  gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(parent), notebook, TRUE);
#else
  gtk_box_pack_start (GTK_BOX (parent), notebook, TRUE, TRUE, 0);
#endif
  
  list = get_sheets_list();
  while (list != NULL) {
    sheet = (Sheet *) list->data;

    label = gtk_label_new(gettext(sheet->name));
    menu_label = gtk_label_new(gettext(sheet->name));
    gtk_misc_set_alignment(GTK_MISC(menu_label), 0.0, 0.5);
    
    child = create_sheet_page(notebook, sheet);
    
    gtk_widget_show(label);
    gtk_widget_show(menu_label);
    gtk_widget_show_all(child);

    gtk_notebook_append_page_menu (GTK_NOTEBOOK (notebook),
				   child, label, menu_label);
    
    list = g_slist_next(list);
  }
  
  gtk_widget_show(notebook);
}

static void
create_color_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GtkWidget *line_area;
  GdkPixmap *default_pixmap;
  GdkPixmap *swap_pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  GtkWidget *hbox;

  gtk_widget_ensure_style(parent);
  style = gtk_widget_get_style(parent);

  default_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &mask, 
		&style->bg[GTK_STATE_NORMAL], default_xpm);
  swap_pixmap =
    gdk_pixmap_colormap_create_from_xpm_d(NULL,
		gtk_widget_get_colormap(parent), &mask, 
		&style->bg[GTK_STATE_NORMAL], swap_xpm);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
#ifdef USE_WRAPBOX
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), frame, TRUE, TRUE, FALSE, FALSE);
  gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(parent), frame, TRUE);
#else
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
#endif
  gtk_widget_realize (frame);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  
  /* Color area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  col_area = color_area_create (54, 42, default_pixmap, swap_pixmap);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);


  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

  gtk_tooltips_set_tip (tool_tips, col_area, _("Foreground & background colors.  The small black "
                        "and white squares reset colors.  The small arrows swap colors.  Double "
                        "click to change colors."),
                        NULL);

  gtk_widget_show (alignment);
  
  /* Linewidth area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  line_area = linewidth_area_create ();
  gtk_container_add (GTK_CONTAINER (alignment), line_area);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

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
#ifndef USE_WRAPBOX
  GtkWidget *hbox;
#endif
  GtkWidget *chooser;

#ifndef USE_WRAPBOX
  hbox = gtk_hbox_new(FALSE, 0);
  gtk_box_pack_start (GTK_BOX (parent), hbox, FALSE, FALSE, 0);
  gtk_widget_show(hbox);
#endif

  chooser = dia_arrow_chooser_new(TRUE, change_start_arrow_style, NULL);
#ifdef USE_WRAPBOX
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE);
  gtk_wrap_box_set_child_forced_break(GTK_WRAP_BOX(parent), chooser, TRUE);
#else
  gtk_box_pack_start(GTK_BOX(hbox), chooser, FALSE, TRUE, 0);
#endif
  gtk_widget_show(chooser);

  chooser = dia_line_chooser_new(change_line_style, NULL);
#ifdef USE_WRAPBOX
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, TRUE, TRUE, FALSE, TRUE);
#else
  gtk_box_pack_start(GTK_BOX(hbox), chooser, TRUE, TRUE, 0);
#endif
  gtk_widget_show(chooser);

  chooser = dia_arrow_chooser_new(FALSE, change_end_arrow_style, NULL);
#ifdef USE_WRAPBOX
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE);
#else
  gtk_box_pack_start(GTK_BOX(hbox), chooser, FALSE, TRUE, 0);
#endif
  gtk_widget_show(chooser);
}

static void
toolbox_delete (GtkWidget *widget, gpointer data)
{
  if (!app_is_embedded())
    app_exit();
}

static void
toolbox_destroy (GtkWidget *widget, gpointer data)
{
  app_exit();
}

void
create_toolbox ()
{
  GtkWidget *window;
  GtkWidget *main_vbox;
  GtkWidget *vbox;
  gchar *accelfilename;
#ifndef GNOME
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;
#endif

#ifdef GNOME
  window = gnome_app_new ("Dia", _("Diagram Editor"));
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref (window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia v" VERSION);
#endif
  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox_window",
			  "Dia");
  gtk_window_set_policy(GTK_WINDOW(window), TRUE, TRUE, FALSE);

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (toolbox_destroy),
		      NULL);

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
#ifdef GNOME
  gnome_app_set_contents(GNOME_APP(window), main_vbox);
#else
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
#endif
  gtk_widget_show (main_vbox);

#ifdef GNOME
  gnome_toolbox_menus_create(window);
#else
  menus_get_toolbox_menubar(&menubar, &accel_group);
  gtk_accel_group_attach (accel_group, GTK_OBJECT (window));
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);
#endif
  
  accelfilename = dia_config_filename("menus/toolbox");
  
  if (accelfilename) {
    gtk_item_factory_parse_rc(accelfilename);
    g_free(accelfilename);
  }
  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();

  /*  gtk_tooltips_set_colors (tool_tips,
			   &colors[11],
			   &main_vbox->style->fg[GTK_STATE_NORMAL]);*/

#ifdef USE_WRAPBOX
  vbox = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(vbox), 143.0 / 283.0);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(vbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(vbox), GTK_JUSTIFY_LEFT);
#else  
  vbox = gtk_vbox_new (FALSE, 1);
#endif
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_widget_show (vbox);

  create_tools (vbox);
  create_sheets (vbox);
  create_color_area (vbox);
  create_lineprops_area (vbox);
  
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
