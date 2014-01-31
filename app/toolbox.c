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

#include <gtk/gtk.h>
#include "gtkwrapbox.h"
#include "gtkhwrapbox.h"

#include "diaarrowchooser.h"
#include "dialinechooser.h"
#include "diadynamicmenu.h"
#include "attributes.h"
#include "sheet.h"
#include "color_area.h"
#include "intl.h"
#include "message.h"
#include "object.h"
#include "widgets.h"

#include "linewidth_area.h"
#include "preferences.h"
#include "persistence.h"

#include "toolbox.h"

#include "pixmaps/missing.xpm"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "dia-app-icons.h"

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

static GtkWidget *sheet_option_menu;
static GtkWidget *sheet_wbox;

GtkWidget *modify_tool_button;
gchar *interface_current_sheet_name;
static GSList *tool_group = NULL;

ToolButton tool_data[] =
{
  { (const gchar **) dia_modify_tool_icon,
    N_("Modify object(s)\nUse <Space> to toggle between this and other tools"),
    NULL,
    "ToolsModify",
    { MODIFY_TOOL, NULL, NULL}
  },
  { (const gchar **) dia_textedit_tool_icon,
    N_("Text edit(s)\nUse <Esc> to leave this tool"),
    "F2",
    "ToolsTextedit",
    { TEXTEDIT_TOOL, NULL, NULL}
  },
  { (const gchar **) dia_zoom_tool_icon,
    N_("Magnify"),
    "M",
    "ToolsMagnify",
    { MAGNIFY_TOOL, NULL, NULL}
  },
  { (const gchar **) dia_scroll_tool_icon,
    N_("Scroll around the diagram"),
    "S",
    "ToolsScroll",
    { SCROLL_TOOL, NULL, NULL}
  },
  { NULL,
    N_("Text"),
    "T",
    "ToolsText",
    { CREATE_OBJECT_TOOL, "Standard - Text", NULL }
  },
  { NULL,
    N_("Box"),
    "R",
    "ToolsBox",
    { CREATE_OBJECT_TOOL, "Standard - Box", NULL }
  },
  { NULL,
    N_("Ellipse"),
    "E",
    "ToolsEllipse",
    { CREATE_OBJECT_TOOL, "Standard - Ellipse", NULL }
  },
  { NULL,
    N_("Polygon"),
    "P",
    "ToolsPolygon",
    { CREATE_OBJECT_TOOL, "Standard - Polygon", NULL }
  },
  { NULL,
    N_("Beziergon"),
    "B",
    "ToolsBeziergon",
    { CREATE_OBJECT_TOOL, "Standard - Beziergon", NULL }
  },
  { NULL,
    N_("Line"),
    "L",
    "ToolsLine",
    { CREATE_OBJECT_TOOL, "Standard - Line", NULL }
  },
  { NULL,
    N_("Arc"),
    "A",
    "ToolsArc",
    { CREATE_OBJECT_TOOL, "Standard - Arc", NULL }
  },
  { NULL,
    N_("Zigzagline"),
    "Z",
    "ToolsZigzagline",
    { CREATE_OBJECT_TOOL, "Standard - ZigZagLine", NULL }
  },
  { NULL,
    N_("Polyline"),
    NULL,
    "ToolsPolyline",
    { CREATE_OBJECT_TOOL, "Standard - PolyLine", NULL }
  },
  { NULL,
    N_("Bezierline"),
    "C",
    "ToolsBezierline",
    { CREATE_OBJECT_TOOL, "Standard - BezierLine", NULL }
  },
  { NULL,
    N_("Image"),
    "I",
    "ToolsImage",
    { CREATE_OBJECT_TOOL, "Standard - Image", NULL }
#ifdef HAVE_CAIRO
  },
  { NULL,
    N_("Outline"),
    NULL,
    "ToolsOutline",
    { CREATE_OBJECT_TOOL, "Standard - Outline", NULL }
#endif
  }
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (ToolButton))
const int num_tools = NUM_TOOLS;

#define COLUMNS   4
#define ROWS      3

static GtkWidget *tool_widgets[NUM_TOOLS];

static Sheet *
get_sheet_by_name(const gchar *name)
{
  GSList *tmp;
  for (tmp = get_sheets_list(); tmp != NULL; tmp = tmp->next) {
    Sheet *sheet = tmp->data;
    /* There is something fishy with comparing both forms: the english and the localized one.
     * But we should be on the safe side here, especially when bug #328570 gets tackled.
     */
    if (0 == g_ascii_strcasecmp(name, sheet->name) || 0 == g_ascii_strcasecmp(name, gettext(sheet->name)))
      return sheet;
  }
  return NULL;
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
                              tooldata->user_data, w, event->state&1);
    return TRUE;
  }

  return FALSE;
}

static const GtkTargetEntry display_target_table[] = {
  { "application/x-dia-object", 0, 0 },
  { "text/uri-list", 0, DIA_DND_TYPE_URI_LIST },
  { "text/plain", 0, DIA_DND_TYPE_TEXT_PLAIN }
};
static int display_n_targets = sizeof(display_target_table)/sizeof(display_target_table[0]);

static void
tool_drag_data_get (GtkWidget *widget, GdkDragContext *context,
		    GtkSelectionData *selection_data, guint info,
		    guint32 time, ToolButtonData *tooldata)
{
  if (info == 0) {
    gtk_selection_data_set(selection_data, 
			   gtk_selection_data_get_target(selection_data),
			   8, (guchar *)&tooldata, sizeof(ToolButtonData *));
  }
}

static void
tool_setup_drag_source(GtkWidget *button, ToolButtonData *tooldata,
		       GdkPixbuf *pixbuf)
{
  g_return_if_fail(tooldata->type == CREATE_OBJECT_TOOL);

  gtk_drag_source_set(button, GDK_BUTTON1_MASK,
		      display_target_table, display_n_targets,
		      GDK_ACTION_DEFAULT|GDK_ACTION_COPY);
  g_signal_connect(G_OBJECT(button), "drag_data_get",
		   G_CALLBACK(tool_drag_data_get), tooldata);
  if (pixbuf)
    gtk_drag_source_set_icon_pixbuf (button, pixbuf);
}

static void
fill_sheet_wbox(Sheet *sheet)
{
  int rows;
  GSList *tmp;
  GtkWidget *first_button = NULL;

  gtk_container_foreach(GTK_CONTAINER(sheet_wbox),
			(GtkCallback)gtk_widget_destroy, NULL);
  tool_group = gtk_radio_button_get_group(GTK_RADIO_BUTTON(tool_widgets[0]));

  /* Remember sheet 'name' for 'Sheets and Objects' dialog */
  interface_current_sheet_name = sheet->name;

  /* set the aspect ratio on the wbox */
  rows = ceil(g_slist_length(sheet->objects) / (double)COLUMNS);
  if (rows<1) rows = 1;
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(sheet_wbox),
				COLUMNS * 1.0 / rows);
  for (tmp = sheet->objects; tmp != NULL; tmp = tmp->next) {
    SheetObject *sheet_obj = tmp->data;
    GdkPixbuf *pixbuf = NULL;
    GtkWidget *image;
    GtkWidget *button;
    ToolButtonData *data;

    if (sheet_obj->pixmap != NULL) {
      pixbuf = gdk_pixbuf_new_from_xpm_data (sheet_obj->pixmap);
    } else if (sheet_obj->pixmap_file != NULL) {
      GError* gerror = NULL;
      
      pixbuf = gdk_pixbuf_new_from_file(sheet_obj->pixmap_file, &gerror);
      if (pixbuf != NULL) {
          int width = gdk_pixbuf_get_width (pixbuf);
          int height = gdk_pixbuf_get_height (pixbuf);
          if (width > 22 && prefs.fixed_icon_size) {
	    GdkPixbuf *cropped;
	    g_warning ("Shape icon '%s' size wrong, cropped.", sheet_obj->pixmap_file);
	    cropped = gdk_pixbuf_new_subpixbuf (pixbuf, 
	                                       (width - 22) / 2, height > 22 ? (height - 22) / 2 : 0, 
					       22, height > 22 ? 22 : height);
	    g_object_unref (pixbuf);
	    pixbuf = cropped;
	  }
      } else {
          pixbuf = gdk_pixbuf_new_from_xpm_data (missing);

          message_warning("failed to load icon for file\n %s\n cause=%s",
                          sheet_obj->pixmap_file,gerror?gerror->message:"[NULL]");
      }
    } else {
      DiaObjectType *type;
      type = object_get_type(sheet_obj->object_type);
      pixbuf = gdk_pixbuf_new_from_xpm_data (type->pixmap);
    }
    if (pixbuf) {
      image = gtk_image_new_from_pixbuf (pixbuf);
    } else {
      image = gtk_image_new ();
    }

    button = gtk_radio_button_new (tool_group);
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    tool_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));

    gtk_container_add (GTK_CONTAINER (button), image);
    gtk_widget_show(image);

    gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(sheet_wbox), button,
		      FALSE, TRUE, FALSE, TRUE, sheet_obj->line_break);
    gtk_widget_show(button);

    data = g_new(ToolButtonData, 1);
    data->type = CREATE_OBJECT_TOOL;
    data->extra_data = sheet_obj->object_type;
    data->user_data = sheet_obj->user_data;
    g_object_set_data_full(G_OBJECT(button), "Dia::ToolButtonData",
			   data, (GDestroyNotify)g_free);
    if (first_button == NULL) first_button = button;
    
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (tool_select_update), data);
    g_signal_connect (G_OBJECT (button), "button_press_event",
		      G_CALLBACK (tool_button_press), data);

    tool_setup_drag_source(button, data, pixbuf);
    g_object_unref(pixbuf);

    gtk_widget_set_tooltip_text (button, gettext(sheet_obj->description));
  }
  /* If the selection is in the old sheet, steal it */
  if (active_tool != NULL &&
      active_tool->type == CREATE_OBJECT_TOOL &&
      first_button != NULL)
    g_signal_emit_by_name(G_OBJECT(first_button), "toggled",
			  GTK_BUTTON(first_button), NULL);
}

static void
sheet_option_menu_changed(DiaDynamicMenu *menu, gpointer user_data)
{
  char *string = dia_dynamic_menu_get_entry(menu);
  Sheet *sheet = get_sheet_by_name(string);
  if (sheet == NULL) {
    message_warning(_("No sheet named %s"), string);
  } else {
    persistence_set_string("last-sheet-selected", string);
    fill_sheet_wbox(sheet);
  }
  g_free(string);
}

static int
cmp_names (const void *a, const void *b)
{
  return g_utf8_collate(gettext( (gchar *)a ), gettext( (gchar *)b ));
}

static GList *
get_sheet_names()
{
  GSList *tmp;
  GList *names = NULL;
  for (tmp = get_sheets_list(); tmp != NULL; tmp = tmp->next) {
    Sheet *sheet = tmp->data;
    names = g_list_append(names, sheet->name);
  }
  /* Already sorted in lib/ but here we sort by the localized (display-)name */
  return g_list_sort (names, cmp_names);
}

static void
create_sheet_dropdown_menu(GtkWidget *parent)
{
  GList *sheet_names = get_sheet_names();

  if (sheet_option_menu != NULL) {
    gtk_container_remove(GTK_CONTAINER(parent), sheet_option_menu);
    sheet_option_menu = NULL;
  }

  sheet_option_menu =
    dia_dynamic_menu_new_stringlistbased(_("Other sheets"), sheet_names, 
					 NULL, "sheets");
  g_signal_connect(DIA_DYNAMIC_MENU(sheet_option_menu), "value-changed",
		   G_CALLBACK(sheet_option_menu_changed), sheet_option_menu);
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(sheet_option_menu),
				     "Assorted");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(sheet_option_menu),
				     "Flowchart");
  dia_dynamic_menu_add_default_entry(DIA_DYNAMIC_MENU(sheet_option_menu),
				     "UML");
  /*    gtk_widget_set_size_request(sheet_option_menu, 20, -1);*/
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), sheet_option_menu,
			    TRUE, TRUE, FALSE, FALSE, TRUE);    
  /* 15 was a magic number that goes beyond the standard objects and the divider. */
  gtk_wrap_box_reorder_child(GTK_WRAP_BOX(parent),
			     sheet_option_menu, NUM_TOOLS+1);
  gtk_widget_show(sheet_option_menu);
}

void
fill_sheet_menu(void)
{
  gchar *selection = dia_dynamic_menu_get_entry(DIA_DYNAMIC_MENU(sheet_option_menu));
  create_sheet_dropdown_menu(gtk_widget_get_parent(sheet_option_menu));
  dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(sheet_option_menu), selection);
  g_free(selection);
}

static void
create_sheets(GtkWidget *parent)
{
  GtkWidget *separator;
  GtkWidget *label;
  GtkWidget *swin;
  gchar *sheetname;
  Sheet *sheet;
  
  separator = gtk_hseparator_new ();
  /* add a bit of padding around the separator */
  label = gtk_vbox_new(FALSE, 0);
  gtk_box_pack_start(GTK_BOX(label), separator, TRUE, TRUE, 3);
  gtk_widget_show(label);

  gtk_wrap_box_pack_wrapped (GTK_WRAP_BOX(parent), label, TRUE,TRUE, FALSE,FALSE, TRUE);
  gtk_widget_show(separator);

  create_sheet_dropdown_menu(parent);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				 GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), swin, TRUE, TRUE, TRUE, TRUE, TRUE);
  gtk_widget_show(swin);

  sheet_wbox = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(sheet_wbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(sheet_wbox), GTK_JUSTIFY_LEFT);
  gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin), sheet_wbox);
  gtk_widget_show(sheet_wbox);

  sheetname = persistence_register_string("last-sheet-selected", _("Flowchart"));
  sheet = get_sheet_by_name(sheetname);
  if (sheet == NULL) {
    /* Couldn't find it */
  } else {
    fill_sheet_wbox(sheet);
    dia_dynamic_menu_select_entry(DIA_DYNAMIC_MENU(sheet_option_menu),
				  sheetname);
  }
  g_free(sheetname);
}

static void
create_color_area (GtkWidget *parent)
{
  GtkWidget *frame;
  GtkWidget *alignment;
  GtkWidget *col_area;
  GtkWidget *line_area;
  GtkStyle *style;
  GtkWidget *hbox;

  gtk_widget_ensure_style(parent);
  style = gtk_widget_get_style(parent);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), frame, TRUE, TRUE, FALSE, FALSE, TRUE);

  hbox = gtk_hbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 0);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  
  /* Color area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  col_area = color_area_create (54, 42, parent, style);
  gtk_container_add (GTK_CONTAINER (alignment), col_area);


  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);

  gtk_widget_set_tooltip_text (col_area, 
			_("Foreground & background colors for new objects.  "
			  "The small black and white squares reset colors.  "
			  "The small arrows swap colors.  Double-click to "
			  "change colors."));

  gtk_widget_show (alignment);
  
  /* Linewidth area: */
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  gtk_container_set_border_width (GTK_CONTAINER (alignment), 3);
  
  line_area = linewidth_area_create ();
  gtk_container_add (GTK_CONTAINER (alignment), line_area);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, TRUE, TRUE, 0);
  gtk_widget_set_tooltip_text(line_area, _("Line widths.  Click on a line to set the default line width for new objects.  Double-click to set the line width more precisely."));
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
  Arrow arrow;
  real dash_length;
  LineStyle style;
  gchar *arrow_name;

  chooser = dia_arrow_chooser_new(TRUE, change_start_arrow_style, NULL);
  gtk_wrap_box_pack_wrapped(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE, TRUE);
  arrow.width = persistence_register_real("start-arrow-width", DEFAULT_ARROW_WIDTH);
  arrow.length = persistence_register_real("start-arrow-length", DEFAULT_ARROW_LENGTH);
  arrow_name = persistence_register_string("start-arrow-type", "None");
  arrow.type = arrow_type_from_name(arrow_name);
  g_free(arrow_name);
  dia_arrow_chooser_set_arrow(DIA_ARROW_CHOOSER(chooser), &arrow);
  attributes_set_default_start_arrow(arrow);
  gtk_widget_set_tooltip_text(chooser, _("Arrow style at the beginning of new lines.  Click to pick an arrow, or set arrow parameters with Details\342\200\246"));
  gtk_widget_show(chooser);

  chooser = dia_line_chooser_new(change_line_style, NULL);
  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, TRUE, TRUE, FALSE, TRUE);
  gtk_widget_set_tooltip_text (chooser, _("Line style for new lines.  Click to pick a line style, or set line style parameters with Details\342\200\246"));
  style = persistence_register_integer("line-style", LINESTYLE_SOLID);
  dash_length = persistence_register_real("dash-length", DEFAULT_LINESTYLE_DASHLEN);
  dia_line_chooser_set_line_style(DIA_LINE_CHOOSER(chooser), style, dash_length);
  gtk_widget_show(chooser);

  chooser = dia_arrow_chooser_new(FALSE, change_end_arrow_style, NULL);
  arrow.width = persistence_register_real("end-arrow-width", DEFAULT_ARROW_WIDTH);
  arrow.length = persistence_register_real("end-arrow-length", DEFAULT_ARROW_LENGTH);
  arrow_name = persistence_register_string("end-arrow-type", "Filled Concave");
  arrow.type = arrow_type_from_name(arrow_name);
  g_free(arrow_name);
  dia_arrow_chooser_set_arrow(DIA_ARROW_CHOOSER(chooser), &arrow);
  attributes_set_default_end_arrow(arrow);

  gtk_wrap_box_pack(GTK_WRAP_BOX(parent), chooser, FALSE, TRUE, FALSE, TRUE);
  gtk_widget_set_tooltip_text(chooser, _("Arrow style at the end of new lines.  Click to pick an arrow, or set arrow parameters with Details\342\200\246"));
  gtk_widget_show(chooser);
}

void
tool_select_update (GtkWidget *w,
		     gpointer   data)
{
  ToolButtonData *tooldata = (ToolButtonData *) data;

  if (tooldata == NULL) {
    g_warning("NULL tooldata in tool_select_update");
    return;
  }

  if (tooldata->type != -1) {
    gint x, y;
    GdkModifierType mask;
    /*  get the modifiers  */
    gdk_window_get_pointer (gtk_widget_get_parent_window(w), &x, &y, &mask);
    tool_select (tooldata->type, tooldata->extra_data, tooldata->user_data,
                 w, mask&1);
  }
}

GdkPixbuf *
tool_get_pixbuf (ToolButton *tb)
{
  GdkPixbuf *pixbuf;
  const gchar **icon_data;

  if (tb->icon_data==NULL) {
    DiaObjectType *type;
    
    type = object_get_type((char *)tb->callback_data.extra_data);
    if (type == NULL)
      icon_data = tool_data[0].icon_data;
    else
      icon_data = type->pixmap;
  } else {
    icon_data = tb->icon_data;
  }
  
  if (strncmp((char*)icon_data, "GdkP", 4) == 0) {
    pixbuf = gdk_pixbuf_new_from_inline(-1, (guint8*)icon_data, TRUE, NULL);
  } else {
    const char **pixmap_data = icon_data;
    pixbuf = gdk_pixbuf_new_from_xpm_data (pixmap_data);
  }
  return pixbuf;
}

/*
 * Don't look too deep into this function. It is doing bad things
 * with casts to conform to the historically interface. We know
 * the difference between char* and char** - most of the time ;)
 */
static GtkWidget *
create_widget_from_xpm_or_gdkp(const char **icon_data, GtkWidget *button, GdkPixbuf **pb_out) 
{
  GtkWidget *pixmapwidget;

  if (strncmp((char*)icon_data, "GdkP", 4) == 0) {
    GdkPixbuf *p;
    *pb_out = p = gdk_pixbuf_new_from_inline(-1, (guint8*)icon_data, TRUE, NULL);
    pixmapwidget = gtk_image_new_from_pixbuf(p);
  } else {
    const char **pixmap_data = icon_data;
    *pb_out = gdk_pixbuf_new_from_xpm_data (pixmap_data);
    pixmapwidget = gtk_image_new_from_pixbuf (*pb_out);
  }
  return pixmapwidget;
}

static void
create_tools(GtkWidget *parent)
{
  GtkWidget *button;
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *image;
  /* GtkStyle *style; */
  const char **pixmap_data;
  int i;

  for (i = 0; i < NUM_TOOLS; i++) {
    tool_widgets[i] = button = gtk_radio_button_new (tool_group);
    gtk_container_set_border_width (GTK_CONTAINER (button), 0);
    gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_HALF);
    tool_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

    gtk_wrap_box_pack(GTK_WRAP_BOX(parent), button,
		      TRUE, TRUE, FALSE, TRUE);

    if (tool_data[i].callback_data.type == MODIFY_TOOL) {
      modify_tool_button = GTK_WIDGET(button);
    }
    
    if (tool_data[i].icon_data==NULL) {
      DiaObjectType *type;
      type =
	object_get_type((char *)tool_data[i].callback_data.extra_data);
      if (type == NULL)
	pixmap_data = tool_data[0].icon_data;
      else
	pixmap_data = type->pixmap;
      image = create_widget_from_xpm_or_gdkp(pixmap_data, button, &pixbuf);
    } else {
      image = create_widget_from_xpm_or_gdkp(tool_data[i].icon_data, button, &pixbuf);
    }
    
    /* GTKBUG:? padding changes */
    gtk_misc_set_padding(GTK_MISC(image), 2, 2);
    
    gtk_container_add (GTK_CONTAINER (button), image);
    
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK (tool_select_update),
			&tool_data[i].callback_data);
    
    g_signal_connect (G_OBJECT (button), "button_press_event",
		      G_CALLBACK (tool_button_press),
			&tool_data[i].callback_data);

    if (tool_data[i].callback_data.type == CREATE_OBJECT_TOOL)
      tool_setup_drag_source(button, &tool_data[i].callback_data, pixbuf);

    if (pixbuf)
      g_object_unref(pixbuf);

    tool_data[i].callback_data.widget = button;

    if (tool_data[i].tool_accelerator) {
	guint key;
	GdkModifierType mods;
	gchar *alabel, *atip;

	gtk_accelerator_parse (tool_data[i].tool_accelerator, &key, &mods);

	alabel = gtk_accelerator_get_label(key, mods);
	atip = g_strconcat(gettext(tool_data[i].tool_desc), " (", alabel, ")", NULL);
	gtk_widget_set_tooltip_text (button, atip);
	g_free (atip);
	g_free (alabel);

    } else {
	gtk_widget_set_tooltip_text (button,
				gettext(tool_data[i].tool_desc));
    }
    
    gtk_widget_show (image);
    gtk_widget_show (button);
  }
}

GtkWidget *
toolbox_create(void)
{
  GtkWidget *wrapbox;

  wrapbox = gtk_hwrap_box_new(FALSE);
  gtk_wrap_box_set_aspect_ratio(GTK_WRAP_BOX(wrapbox), 144.0 / 318.0);
  gtk_wrap_box_set_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_TOP);
  gtk_wrap_box_set_line_justify(GTK_WRAP_BOX(wrapbox), GTK_JUSTIFY_LEFT);


  /* pack the rest of the stuff */
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

  return wrapbox;
}

void
toolbox_setup_drag_dest (GtkWidget *widget)
{
  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL,
		    toolbox_target_table, toolbox_n_targets, GDK_ACTION_COPY);
}

void
canvas_setup_drag_dest (GtkWidget *canvas)
{
  gtk_drag_dest_set(canvas, GTK_DEST_DEFAULT_ALL,
		    display_target_table, display_n_targets, GDK_ACTION_COPY);
}

