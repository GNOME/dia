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
#include "intl.h"

#include "interface.h"
#include "menus.h"
#include "disp_callbacks.h"
#include "tool.h"
#include "sheet.h"
#include "app_procs.h"
#include "color_area.h"
#include "linewidth_area.h"

#include "pixmaps.h"

typedef struct _ToolButton ToolButton;

typedef struct _ToolButtonData ToolButtonData;

struct _ToolButtonData
{
  ToolType type;
  gpointer extra_data;
  gpointer user_data; /* Used by create_object_tool */
};

struct _ToolButton
{
  gchar **icon_data;
  char  *tool_desc;
  ToolButtonData callback_data;
};

static ToolButton tool_data[] =
{
  { (char **) arrow_xpm,
    N_("Modify object(s)"),
    { MODIFY_TOOL, NULL, NULL}
  },
  { (char **) magnify_xpm,
    N_("Magnify"),
    { MAGNIFY_TOOL, NULL, NULL}
  },
  { (char **) scroll_xpm,
    N_("Scroll around the diagram"),
    { SCROLL_TOOL, NULL, NULL}
  },
  { NULL,
    N_("Create Text"),
    { CREATE_OBJECT_TOOL, "Standard - Text", NULL }
  },
  { NULL,
    N_("Create Box"),
    { CREATE_OBJECT_TOOL, "Standard - Box", NULL }
  },
  { NULL,
    N_("Create Ellipse"),
    { CREATE_OBJECT_TOOL, "Standard - Ellipse", NULL }
  },
  { NULL,
    N_("Create Line"),
    { CREATE_OBJECT_TOOL, "Standard - Line", NULL }
  },
  { NULL,
    N_("Create Arc"),
    { CREATE_OBJECT_TOOL, "Standard - Arc", NULL }
  },
  { NULL,
    N_("Create Zigzagline"),
    { CREATE_OBJECT_TOOL, "Standard - ZigZagLine", NULL }
  },
  { NULL,
    "Create Polyline",
    { CREATE_OBJECT_TOOL, "Standard - PolyLine", NULL }
  }
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (ToolButton))
#define COLUMNS   4
#define ROWS      3

static GtkWidget *toolbox_shell = NULL;
static GtkWidget *tool_widgets[NUM_TOOLS];
static GtkTooltips *tool_tips;
static GSList *tool_group = NULL;

GtkWidget *modify_tool_button;

/*  The popup shell is a pointer to the display shell that posted the latest
 *  popup menu.  When this is null, and a command is invoked, then the
 *  assumption is that the command was a result of a keyboard accelerator
 */
GtkWidget *popup_shell = NULL;

void
create_display_shell(DDisplay *ddisp,
		     int width, int height,
		     char *title)
{
  GtkWidget *table;
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
  ddisp->shell = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref  (ddisp->shell);
  gtk_window_set_title (GTK_WINDOW (ddisp->shell), title);
  gtk_window_set_wmclass (GTK_WINDOW (ddisp->shell), "diagram_window",
			  "Dia");
  gtk_window_set_policy (GTK_WINDOW (ddisp->shell), TRUE, TRUE, TRUE);
  gtk_object_set_user_data (GTK_OBJECT (ddisp->shell), (gpointer) ddisp);
  gtk_widget_set_events (ddisp->shell, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);

  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "delete_event",
                      GTK_SIGNAL_FUNC (ddisplay_delete),
                      ddisp);
  gtk_signal_connect (GTK_OBJECT (ddisp->shell), "destroy",
                      GTK_SIGNAL_FUNC (ddisplay_destroy),
                      ddisp);
  
  /*  the table containing all widgets  */
  table = gtk_table_new (3, 3, FALSE);
  gtk_table_set_col_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_col_spacing (GTK_TABLE (table), 1, 2);
  gtk_table_set_row_spacing (GTK_TABLE (table), 0, 1);
  gtk_table_set_row_spacing (GTK_TABLE (table), 1, 2);
  gtk_container_set_border_width (GTK_CONTAINER (table), 2);
  gtk_container_add (GTK_CONTAINER (ddisp->shell), table);

  /*  scrollbars, rulers, canvas, menu popup button  */
  ddisp->origin = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (ddisp->origin), GTK_SHADOW_OUT);

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
  menus_get_image_menu (&ddisp->popup, &ddisp->accel_group);

  /*  the accelerator table/group for the popup */
  gtk_window_add_accel_group (GTK_WINDOW(ddisp->shell), ddisp->accel_group);

  gtk_widget_show (ddisp->hsb);
  gtk_widget_show (ddisp->vsb);
  gtk_widget_show (ddisp->origin);
  gtk_widget_show (ddisp->hrule);
  gtk_widget_show (ddisp->vrule);
  gtk_widget_show (ddisp->canvas);
  gtk_widget_show (table);
  gtk_widget_show (ddisp->shell);

  /*  set the focus to the canvas area  */
  gtk_widget_grab_focus (ddisp->canvas);
}

static void
tool_select_update (GtkWidget *w,
		     gpointer   data)
{
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if ((tooldata->type != -1) && GTK_TOGGLE_BUTTON (w)->active)
    tool_select (tooldata->type, tooldata->extra_data, tooldata->user_data);
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

static void
create_tools(GtkWidget *parent)
{
  GtkWidget *table;
  GtkWidget *button;
  GtkWidget *pixmapwidget;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  char **pixmap_data;
  int i;
  
  table = gtk_table_new (ROWS, COLUMNS, FALSE);
  gtk_box_pack_start (GTK_BOX (parent), table, FALSE, TRUE, 0);
  gtk_widget_realize (table);

  for (i = 0; i < NUM_TOOLS; i++) {
    tool_widgets[i] = button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

    gtk_table_attach (GTK_TABLE (table), button,
		      (i % COLUMNS), (i % COLUMNS) + 1,
		      (i / COLUMNS), (i / COLUMNS) + 1,
		      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		      GTK_FILL,
		      0, 0);

    if (tool_data[i].callback_data.type == MODIFY_TOOL) {
      modify_tool_button = GTK_WIDGET(button);
    }
    
    if (tool_data[i].icon_data==NULL) {
      ObjectType *type;
      type =
	object_get_type((char *)tool_data[i].callback_data.extra_data);
      pixmap_data = type->pixmap;
    } else {
      pixmap_data = tool_data[i].icon_data;
    }
    
    style = gtk_widget_get_style(button);
    pixmap = gdk_pixmap_create_from_xpm_d(parent->window, &mask, 
					  &style->bg[GTK_STATE_NORMAL], 
					  pixmap_data);
    
    pixmapwidget = gtk_pixmap_new(pixmap, mask);
    
    gtk_misc_set_padding(GTK_MISC(pixmapwidget), 4, 4);
    
    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    
    gtk_signal_connect (GTK_OBJECT (button), "toggled",
			GTK_SIGNAL_FUNC (tool_select_update),
			&tool_data[i].callback_data);
    
    gtk_signal_connect (GTK_OBJECT (button), "button_press_event",
			GTK_SIGNAL_FUNC (tool_button_press),
			&tool_data[i].callback_data);
    
    gtk_tooltips_set_tip (tool_tips, button,
			  gettext(tool_data[i].tool_desc), NULL);
    
    gtk_widget_show (pixmapwidget);
    gtk_widget_show (button);
  }
  gtk_widget_show(table);
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
  
  table = gtk_table_new (rows, COLUMNS, TRUE);
  
  style = gtk_widget_get_style(parent);

  i = 0;
  list = sheet->objects;
  while (list != NULL) {
    sheet_obj = (SheetObject *) list->data;
    
    
    if (sheet_obj->pixmap != NULL) {
      pixmap = gdk_pixmap_create_from_xpm_d(parent->window, &mask, 
					    &style->bg[GTK_STATE_NORMAL], 
					    sheet_obj->pixmap);
    } else {
      ObjectType *type;
      type = object_get_type(sheet_obj->object_type);
      pixmap = gdk_pixmap_create_from_xpm_d(parent->window, &mask, 
					    &style->bg[GTK_STATE_NORMAL], 
					    type->pixmap);
    }

    pixmapwidget = gtk_pixmap_new(pixmap, mask);

    
    button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_group (GTK_RADIO_BUTTON (button));
    
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);


    gtk_container_add (GTK_CONTAINER (button), pixmapwidget);
    gtk_table_attach (GTK_TABLE (table), button,
		      (i % COLUMNS), (i % COLUMNS) + 1,
		      (i / COLUMNS), (i / COLUMNS) + 1,
		      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
		      GTK_FILL,
		      0, 0);


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
			  sheet_obj->description, NULL);

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

  
  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (parent), separator, FALSE, TRUE, 3);
  gtk_widget_show(separator);

  notebook = gtk_notebook_new ();
  /*
  gtk_signal_connect (GTK_OBJECT (notebook), "switch_page",
		      GTK_SIGNAL_FUNC (page_switch), NULL);
  */
  gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_TOP);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), TRUE);
  gtk_notebook_set_scrollable (GTK_NOTEBOOK (notebook), TRUE);
  gtk_container_set_border_width (GTK_CONTAINER (notebook), 1);
  gtk_box_pack_start (GTK_BOX (parent), notebook, TRUE, TRUE, 0);
  
  list = get_sheets_list();
  while (list != NULL) {
    sheet = (Sheet *) list->data;

    label = gtk_label_new( sheet->name );
    
    child = create_sheet_page(notebook, sheet);
    
    gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				   child, label);
    gtk_widget_show(label);
    gtk_widget_show_all(child);
    
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

  style = gtk_widget_get_style(parent);

  default_pixmap =
    gdk_pixmap_create_from_xpm_d(parent->window, &mask, 
				 &style->bg[GTK_STATE_NORMAL], 
				 default_xpm);
  swap_pixmap =
    gdk_pixmap_create_from_xpm_d(parent->window, &mask, 
				 &style->bg[GTK_STATE_NORMAL], 
				 swap_xpm);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_box_pack_start (GTK_BOX (parent), frame, FALSE, FALSE, 0);
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


void
toolbox_delete (GtkWidget *widget, gpointer data)
{
  app_exit();
}

void
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
  GtkWidget *menubar;
  GtkAccelGroup *accel_group;

#ifdef GNOME
  window = gnome_app_new ("Dia", _("Diagram Editor"));
#else
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_ref (window);
  gtk_window_set_title (GTK_WINDOW (window), "Dia v" VERSION);
  gtk_window_set_wmclass (GTK_WINDOW (window), "toolbox_window",
			  "Dia");

  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (toolbox_delete),
		      NULL);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (toolbox_destroy),
		      NULL);
#endif

  main_vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (main_vbox), 1);
#ifdef GNOME
  gnome_app_set_contents(GNOME_APP(window), main_vbox);
#else
  gtk_container_add (GTK_CONTAINER (window), main_vbox);
#endif
  gtk_widget_show (main_vbox);

#ifdef GNOME
  gnome_menus_create(window);
#else
  menus_get_toolbox_menubar(&menubar, &accel_group);
  gtk_accel_group_attach (accel_group, GTK_OBJECT (window));
  gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, TRUE, 0);
  gtk_widget_show (menubar);
#endif
  
  /*  tooltips  */
  tool_tips = gtk_tooltips_new ();

  /*  gtk_tooltips_set_colors (tool_tips,
			   &colors[11],
			   &main_vbox->style->fg[GTK_STATE_NORMAL]);*/
  
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (main_vbox), vbox, TRUE, TRUE, 0);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
  gtk_widget_show (vbox);

  create_tools (vbox);
  create_sheets (vbox);
  create_color_area (vbox);
  
  gtk_widget_show (window);
  toolbox_shell = window;
}

