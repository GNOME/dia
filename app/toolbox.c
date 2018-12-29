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

#include "attributes.h"
#include "sheet.h"
#include "dia-colour-area.h"
#include "dia-line-width-area.h"
#include "widgets/dia-sheet-chooser.h"
#include "widgets/dia-arrow-chooser.h"
#include "intl.h"
#include "message.h"
#include "object.h"
#include "widgets.h"

#include "preferences.h"
#include "persistence.h"

#include "toolbox.h"

#include "pixmaps/missing.xpm"

#include <gdk-pixbuf/gdk-pixbuf.h>
#include "dia-app-icons.h"

G_DEFINE_TYPE (DiaToolbox, dia_toolbox, GTK_TYPE_BOX)

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
  },
  { NULL,
    N_("Outline"),
    NULL,
    "ToolsOutline",
    { CREATE_OBJECT_TOOL, "Standard - Outline", NULL }
  }
};

#define NUM_TOOLS (sizeof (tool_data) / sizeof (ToolButton))
const int num_tools = NUM_TOOLS;

#define COLUMNS   4
#define ROWS      3

static GtkWidget *tool_widgets[NUM_TOOLS];

static DiaSheet *
get_sheet_by_name(const gchar *name)
{
  GSList *tmp;
  for (tmp = get_sheets_list(); tmp != NULL; tmp = tmp->next) {
    DiaSheet *sheet = tmp->data;
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
fill_sheet_wbox (DiaToolbox *self, DiaSheet *sheet)
{
  int i = 0;
  GSList *tmp;
  GtkWidget *first_button = NULL;
  gtk_container_foreach (GTK_CONTAINER (self->items),
                         (GtkCallback) gtk_widget_destroy, NULL);
  tool_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (tool_widgets[0]));

  /* Remember sheet 'name' for 'Sheets and Objects' dialog */
  interface_current_sheet_name = sheet->name;

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
    gtk_widget_show (image);

    if (sheet_obj->line_break) {
      i += COLUMNS - (i % COLUMNS);
    }
    gtk_grid_attach (GTK_GRID (self->items), button, i % COLUMNS, i / COLUMNS, 1, 1);
    gtk_widget_show (button);

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

    i++;
  }
  /* If the selection is in the old sheet, steal it */
  if (active_tool != NULL &&
      active_tool->type == CREATE_OBJECT_TOOL &&
      first_button != NULL)
    g_signal_emit_by_name(G_OBJECT(first_button), "toggled",
			  GTK_BUTTON(first_button), NULL);
}

static void
sheet_selected (DiaSheetChooser *chooser,
                DiaSheet        *sheet,
                DiaToolbox      *self)
{
  persistence_set_string ("last-sheet-selected", DIA_SHEET (sheet)->name);
  fill_sheet_wbox (self, sheet);
}

static void
create_sheet_dropdown_menu (DiaToolbox *self)
{
  GSList *sheets = get_sheets_list();
  GSList *l;
  GtkWidget *button;
  DiaSheet *tmp;

  self->sheets = g_list_store_new (DIA_TYPE_SHEET);

  /* TODO: Handle this & translations better */
  tmp = get_sheet_by_name ("Assorted");
  g_object_set_data (G_OBJECT (tmp), "dia-list-top", GINT_TO_POINTER (TRUE));
  g_list_store_append (self->sheets, tmp);
  tmp = get_sheet_by_name ("Flowchart");
  g_object_set_data (G_OBJECT (tmp), "dia-list-top", GINT_TO_POINTER (TRUE));
  g_list_store_append (self->sheets, tmp);
  tmp = get_sheet_by_name ("UML");
  g_object_set_data (G_OBJECT (tmp), "dia-list-top", GINT_TO_POINTER (TRUE));
  g_list_store_append (self->sheets, tmp);

  for (l = sheets; l != NULL; l = l->next) {
    if (g_strcmp0 (DIA_SHEET (l->data)->name, "Assorted") == 0 ||
        g_strcmp0 (DIA_SHEET (l->data)->name, "Flowchart") == 0 ||
        g_strcmp0 (DIA_SHEET (l->data)->name, "UML") == 0) {
      continue;
    }
    g_list_store_append (self->sheets, DIA_SHEET (l->data));
  }

  button = dia_sheet_chooser_new ();
  g_signal_connect (G_OBJECT (button), "sheet-selected",
                    G_CALLBACK (sheet_selected), self);
  dia_sheet_chooser_set_model (DIA_SHEET_CHOOSER (button), G_LIST_MODEL (self->sheets));
  gtk_box_pack_start (GTK_BOX (self), button, FALSE, FALSE, 0);
  gtk_widget_show (button);
}

static void
create_sheets (DiaToolbox *self)
{
  GtkWidget *swin;
  gchar *sheetname;
  DiaSheet *sheet;
  
  create_sheet_dropdown_menu (self);

  swin = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (swin),
                                  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_propagate_natural_height (GTK_SCROLLED_WINDOW (swin),
                                                    TRUE);
  gtk_box_pack_start (GTK_BOX (self), swin, FALSE, FALSE, 0);
  gtk_widget_show (swin);

  self->items = g_object_new (GTK_TYPE_GRID,
                              "column-homogeneous", TRUE,
                              "column-spacing", 4,
                              "row-spacing", 4,
                              NULL);
  gtk_container_add (GTK_CONTAINER (swin), self->items);
  gtk_widget_show (self->items);

  sheetname = persistence_register_string ("last-sheet-selected", _("Flowchart"));
  sheet = get_sheet_by_name (sheetname);
  if (sheet != NULL) {
    fill_sheet_wbox (self, sheet);
  }
  g_free (sheetname);
}

static void
create_color_area (DiaToolbox *self)
{
  GtkWidget *col_area;
  GtkWidget *line_area;
  GtkWidget *hbox;

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_end (GTK_BOX (self), hbox, FALSE, FALSE, 0);
  
  /* Color area: */
  col_area = dia_colour_area_new (54, 42);
  gtk_widget_set_halign (col_area, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (col_area, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (hbox), col_area, TRUE, TRUE, 0);
  
  /* Linewidth area: */
  line_area = dia_line_width_area_new ();
  gtk_widget_set_halign (col_area, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (col_area, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (hbox), line_area, TRUE, TRUE, 0);

  gtk_widget_show (col_area);
  gtk_widget_show (line_area);
  gtk_widget_show (hbox);
}

static void
change_start_arrow_style (DiaArrowChooser *chooser, gpointer user_data)
{
  attributes_set_default_start_arrow (dia_arrow_chooser_get_arrow (chooser));
}

static void
change_end_arrow_style (DiaArrowChooser *chooser, gpointer user_data)
{
  attributes_set_default_end_arrow (dia_arrow_chooser_get_arrow (chooser));
}

static void
change_line_style(DiaLineStyleSelector *selector, gpointer user_data)
{
  LineStyle lstyle;
  real dash_length;
  dia_line_chooser_get_line_style (selector, &lstyle, &dash_length);
  attributes_set_default_line_style (lstyle, dash_length);
}

static void
create_lineprops_area (DiaToolbox *self)
{
  GtkWidget *box;
  GtkWidget *chooser;
  Arrow arrow;
  real dash_length;
  LineStyle style;
  gchar *arrow_name;

  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_set_homogeneous (GTK_BOX (box), TRUE);
  gtk_style_context_add_class (gtk_widget_get_style_context (box),
                               GTK_STYLE_CLASS_LINKED);
  gtk_box_pack_end (GTK_BOX (self), box, FALSE, FALSE, 0);
  gtk_widget_show (box);

  chooser = dia_arrow_chooser_new (TRUE);
  g_signal_connect (G_OBJECT (chooser), "value-changed",
                    G_CALLBACK (change_start_arrow_style), NULL);
  gtk_container_add (GTK_CONTAINER (box), chooser);
  arrow.width = persistence_register_real ("start-arrow-width", DEFAULT_ARROW_WIDTH);
  arrow.length = persistence_register_real ("start-arrow-length", DEFAULT_ARROW_LENGTH);
  arrow_name = persistence_register_string ("start-arrow-type", "None");
  arrow.type = arrow_type_from_name (arrow_name);
  g_free (arrow_name);
  dia_arrow_chooser_set_arrow (DIA_ARROW_CHOOSER (chooser), &arrow);
  attributes_set_default_start_arrow (arrow);
  gtk_widget_set_tooltip_text (chooser, _("Arrow style at the beginning of new lines.  Click to pick an arrow, or set arrow parameters with Details\342\200\246"));
  gtk_widget_show (chooser);

  chooser = dia_line_chooser_new ();
  g_signal_connect (G_OBJECT (chooser), "value-changed",
                    G_CALLBACK (change_line_style), NULL);
  gtk_container_add (GTK_CONTAINER (box), chooser);
  gtk_widget_set_tooltip_text (chooser, _("Line style for new lines.  Click to pick a line style, or set line style parameters with Details\342\200\246"));
  style = persistence_register_integer ("line-style", LINESTYLE_SOLID);
  dash_length = persistence_register_real ("dash-length", DEFAULT_LINESTYLE_DASHLEN);
  dia_line_chooser_set_line_style (DIA_LINE_CHOOSER (chooser), style, dash_length);
  gtk_widget_show (chooser);

  chooser = dia_arrow_chooser_new (FALSE);
  g_signal_connect (G_OBJECT (chooser), "value-changed",
                    G_CALLBACK (change_end_arrow_style), NULL);
  gtk_container_add (GTK_CONTAINER (box), chooser);
  arrow.width = persistence_register_real ("end-arrow-width", DEFAULT_ARROW_WIDTH);
  arrow.length = persistence_register_real ("end-arrow-length", DEFAULT_ARROW_LENGTH);
  arrow_name = persistence_register_string ("end-arrow-type", "Filled Concave");
  arrow.type = arrow_type_from_name (arrow_name);
  g_free (arrow_name);
  dia_arrow_chooser_set_arrow (DIA_ARROW_CHOOSER (chooser), &arrow);
  attributes_set_default_end_arrow (arrow);
  gtk_widget_set_tooltip_text (chooser, _("Arrow style at the end of new lines.  Click to pick an arrow, or set arrow parameters with Details\342\200\246"));
  gtk_widget_show (chooser);
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
create_tools (DiaToolbox *self)
{
  GtkWidget *button;
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *image;
  /* GtkStyle *style; */
  const char **pixmap_data;
  int i;

  self->tools = gtk_grid_new ();
  gtk_widget_show (self->tools);
  gtk_box_pack_start (GTK_BOX (self), self->tools, FALSE, FALSE, 0);

  for (i = 0; i < NUM_TOOLS; i++) {
    tool_widgets[i] = button = gtk_radio_button_new (tool_group);
    gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
    tool_group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (button));
    gtk_toggle_button_set_mode (GTK_TOGGLE_BUTTON (button), FALSE);

/*

  0     1     2     3 
  0, 0  0, 1  0, 2  0, 3
  4     5     6     7 
  1, 0  1, 1  1, 2  1, 3
  8     9     10    11
  2, 0  2, 1  2, 2  2, 3

*/

    gtk_grid_attach (GTK_GRID (self->tools), button, i % COLUMNS, i / COLUMNS, 1, 1);

    if (tool_data[i].callback_data.type == MODIFY_TOOL) {
      modify_tool_button = GTK_WIDGET(button);
    }
    
    if (tool_data[i].icon_data==NULL) {
      DiaObjectType *type;
      type = object_get_type((char *)tool_data[i].callback_data.extra_data);
      if (type == NULL)
        pixmap_data = tool_data[0].icon_data;
      else
        pixmap_data = type->pixmap;
      image = create_widget_from_xpm_or_gdkp(pixmap_data, button, &pixbuf);
    } else {
      image = create_widget_from_xpm_or_gdkp(tool_data[i].icon_data, button, &pixbuf);
    }
    
    g_object_set (G_OBJECT (image),
                  "margin", 2,
                  NULL);
    
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
      gtk_widget_set_tooltip_text (button, gettext(tool_data[i].tool_desc));
    }
    
    gtk_widget_show (image);
    gtk_widget_show (button);
  }
}

static void
dia_toolbox_class_init (DiaToolboxClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
}

static void
dia_toolbox_init (DiaToolbox *self)
{
  g_object_set (G_OBJECT (self),
                "orientation", GTK_ORIENTATION_VERTICAL,
                "spacing", 8,
                "margin", 8,
                NULL);
  gtk_widget_show (GTK_WIDGET (self));

  create_tools (self);
  create_sheets (self);
  create_lineprops_area (self);
  create_color_area (self);

  /* Setup toolbox area as file drop destination */
  gtk_drag_dest_set (GTK_WIDGET (self),
                     GTK_DEST_DEFAULT_ALL,
                     toolbox_target_table, toolbox_n_targets,
                     GDK_ACTION_COPY);
}

GtkWidget *
dia_toolbox_new ()
{
  return g_object_new (DIA_TYPE_TOOLBOX, NULL);
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

