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

/* Parts of this file are derived from the file layers_dialog.c in the Gimp:
 *
 * The GIMP -- an image manipulation program
 * Copyright (C) 1995 Spencer Kimball and Peter Mattis
 */

#include <string.h>

#include "layer_dialog.h"

#include "pixmaps/eye.xbm"
#include "pixmaps/new.xpm"
#include "pixmaps/lower.xpm"
#include "pixmaps/raise.xpm"
#include "pixmaps/delete.xpm"

static struct LayerDialog *layer_dialog = NULL;

typedef struct _ButtonData ButtonData;

struct _ButtonData {
  gchar **xpm_data;
  gpointer callback;
  char *tooltip;
};

static void layer_dialog_new_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_raise_callback(GtkWidget *widget, gpointer gdata);
static void  layer_dialog_lower_callback(GtkWidget *widget, gpointer gdata);
static void  layer_dialog_delete_callback(GtkWidget *widget, gpointer gdata);

static ButtonData buttons[] = {
  { new_xpm, layer_dialog_new_callback, "New Layer" },
  { raise_xpm, layer_dialog_raise_callback, "Raise Layer" },
  { lower_xpm, layer_dialog_lower_callback, "Lower Layer" },
  { delete_xpm, layer_dialog_delete_callback, "Delete Layer" },
};

enum {
  BUTTON_NEW = 0,
  BUTTON_RAISE,
  BUTTON_LOWER,
  BUTTON_DELETE
};
 
static int num_buttons = sizeof(buttons)/sizeof(ButtonData);

static GtkWidget *
create_button_box(GtkWidget *parent)
{
  GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *box;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GtkStyle *style;
  int i;
  
  GtkTooltips *tool_tips = NULL; /* ARG */
  
  gtk_widget_realize(parent);
  style = gtk_widget_get_style(parent);

  button_box = gtk_hbox_new (TRUE, 1);

  for (i=0;i<num_buttons;i++) {
    pixmap = gdk_pixmap_create_from_xpm_d (parent->window,
					   &mask,
					   &style->bg[GTK_STATE_NORMAL],
					   buttons[i].xpm_data);
      
    pixmapwid =  gtk_pixmap_new (pixmap, mask);
    gtk_widget_show(pixmapwid);

    button = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (button), pixmapwid);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       (GtkSignalFunc) buttons[i].callback,
			       GTK_OBJECT (parent));
          
    if (tool_tips != NULL)
      gtk_tooltips_set_tip (tool_tips, button, buttons[i].tooltip, NULL);

    gtk_box_pack_start (GTK_BOX(button_box), button, TRUE, TRUE, 0);

    layer_dialog->buttons[i] = button;
    
    gtk_widget_show (button);
  }

  return button_box;
}

void
create_layer_dialog(void)
{
  GtkWidget *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *omenu;
  GtkWidget *menu;
  GtkWidget *list;
  GtkWidget *separator;
  GtkWidget *scrolled_win;
  GtkWidget *button_box;

  layer_dialog = g_new(struct LayerDialog, 1);

  layer_dialog->diagram = NULL;
  
  layer_dialog->dialog = dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), "Layers");
  gtk_window_set_wmclass (GTK_WINDOW (dialog),
			  "layer_window", "Dia");
  gtk_window_set_policy (GTK_WINDOW (dialog), TRUE, TRUE, TRUE);

  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

  
  vbox = GTK_DIALOG(dialog)->vbox;

  hbox = gtk_hbox_new(FALSE, 1);
  
  label = gtk_label_new("Diagrams:");
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  layer_dialog->diagram_omenu = omenu = gtk_option_menu_new();
  gtk_box_pack_start(GTK_BOX(hbox), omenu, TRUE, TRUE, 2);
  gtk_widget_show (omenu);

  menu = gtk_menu_new();
  gtk_option_menu_set_menu(GTK_OPTION_MENU(omenu), menu);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);

  separator = gtk_hseparator_new();
  gtk_box_pack_start(GTK_BOX(vbox), separator, FALSE, FALSE, 2);
  gtk_widget_show (separator);

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
				  GTK_POLICY_AUTOMATIC, 
				  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (vbox), scrolled_win, TRUE, TRUE, 2);

  layer_dialog->layer_list = list = gtk_list_new();

  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_SINGLE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (scrolled_win);
  gtk_widget_show (list);

  button_box = create_button_box(dialog);
  
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  layer_dialog_update_diagram_list();
}

static void
dia_layer_select_callback(GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *lw;
  lw = DIA_LAYER_WIDGET(widget);

  diagram_remove_all_selected(lw->dia);
  data_set_active_layer(lw->dia->data, lw->layer);
  diagram_add_update_all(lw->dia);
  diagram_flush(lw->dia);
}

static void
dia_layer_deselect_callback(GtkWidget *widget, gpointer data)
{
}


static void
layer_dialog_new_callback(GtkWidget *widget, gpointer gdata)
{
  Layer *layer;
  Diagram *dia;

  dia = layer_dialog->diagram;

  if (dia != NULL) {
    layer = new_layer(g_strdup("New layer"));
    data_add_layer(dia->data, layer);
    diagram_add_update_all(dia);
    diagram_flush(dia);

    layer_dialog_set_diagram(dia);
    /* TODO: Select the newly created layer here. */
    
  }
}

static void
layer_dialog_delete_callback(GtkWidget *widget, gpointer gdata)
{
  Layer *layer;
  Diagram *dia;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len>1)) {
    data_delete_active_layer(dia->data);
    diagram_add_update_all(dia);
    diagram_flush(dia);

    layer_dialog_set_diagram(dia);
    /* TODO: This selects the first layer as active, we really
       should select the first item in the list here. */
  }
}

static void
layer_dialog_raise_callback(GtkWidget *widget, gpointer gdata)
{
}

static void
layer_dialog_lower_callback(GtkWidget *widget, gpointer gdata)
{
}


static void
layer_dialog_select_diagram_callback(GtkWidget *widget, gpointer gdata)
{
  Diagram *dia = (Diagram *) gdata;

  layer_dialog_set_diagram(dia);
}
     
void
layer_dialog_update_diagram_list(void)
{
  GtkWidget *new_menu;
  GtkWidget *menu_item;
  GList *dia_list;
  Diagram *dia;
  char *filename;
  int i;
  int current_nr;
    
  new_menu = gtk_menu_new();

  current_nr = -1;
  
  i = 0;
  dia_list = open_diagrams;
  while (dia_list != NULL) {
    dia = (Diagram *) dia_list->data;

    if (dia == layer_dialog->diagram) {
      current_nr = i;
    }
    
    filename = strrchr(dia->filename, '/');
    if (filename==NULL) {
      filename = dia->filename;
    } else {
      filename++;
    }

    menu_item = gtk_menu_item_new_with_label(filename);

    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			(GtkSignalFunc) layer_dialog_select_diagram_callback,
			(gpointer) dia);

    gtk_menu_append( GTK_MENU(new_menu), menu_item);
    gtk_widget_show (menu_item);

    dia_list = g_list_next(dia_list);
    i++;
  }

  if (open_diagrams==NULL) {
    menu_item = gtk_menu_item_new_with_label ("none");
    gtk_signal_connect (GTK_OBJECT (menu_item), "activate",
			(GtkSignalFunc) layer_dialog_select_diagram_callback,
			(gpointer) NULL);
    gtk_menu_append( GTK_MENU(new_menu), menu_item);
    gtk_widget_show (menu_item);
  }
  
  gtk_option_menu_remove_menu(GTK_OPTION_MENU(layer_dialog->diagram_omenu));

  gtk_option_menu_set_menu(GTK_OPTION_MENU(layer_dialog->diagram_omenu),
			   new_menu);

  gtk_option_menu_set_history(GTK_OPTION_MENU(layer_dialog->diagram_omenu),
			      current_nr);
  gtk_menu_set_active(GTK_MENU(new_menu), current_nr);

  if (current_nr == -1) {
    dia = NULL;
    if (open_diagrams!=NULL) {
      dia = (Diagram *) open_diagrams->data;
    }
    layer_dialog_set_diagram(dia);
  }
}

void
layer_dialog_show()
{
  gtk_widget_show(layer_dialog->dialog);
}

void
layer_dialog_set_diagram(Diagram *dia)
{
  DiagramData *data;
  GtkWidget *layer_widget;
  Layer *layer;
  int i;

  gtk_list_clear_items(GTK_LIST(layer_dialog->layer_list), 0, -1);
  layer_dialog->diagram = dia;

  if (dia != NULL) {
    data = dia->data;
    
    for (i=0;i<data->layers->len;i++) {
      layer = (Layer *) g_ptr_array_index(data->layers, i);
      layer_widget = dia_layer_widget_new(dia, layer);
      gtk_widget_show(layer_widget);
      gtk_container_add(GTK_CONTAINER(layer_dialog->layer_list), layer_widget);
    }
  }
}



/******* DiaLayerWidget: *****/

static void
dia_layer_widget_class_init(DiaLayerWidgetClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = (GtkObjectClass*) class;
}

static void 
dia_layer_widget_visible_toggled_callback(GtkWidget *widget, gpointer gdata)
{
  DiaLayerWidget *lw;
  lw = DIA_LAYER_WIDGET(gdata);

  lw->layer->visible = GTK_TOGGLE_BUTTON(lw->visible)->active;
  diagram_add_update_all(lw->dia);
  diagram_flush(lw->dia);
}



static void
dia_layer_widget_init(DiaLayerWidget *lw)
{
  GtkWidget *hbox;
  GtkWidget *visible;
  GtkWidget *label;

  hbox = gtk_hbox_new(FALSE, 0);

  lw->dia = NULL;
  lw->layer = NULL;
  
  lw->visible = visible = gtk_toggle_button_new_with_label("Visible");
  gtk_box_pack_start(GTK_BOX(hbox), visible, FALSE, TRUE, 0);
  gtk_widget_show(visible);
  gtk_signal_connect (GTK_OBJECT (visible), "toggled",
		      (GtkSignalFunc) dia_layer_widget_visible_toggled_callback,
		      (gpointer) lw);

  
  lw->label = label = gtk_label_new("layer_default_label");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);
  
  gtk_widget_show(hbox);

  gtk_container_add(GTK_CONTAINER(lw), hbox);

  gtk_signal_connect (GTK_OBJECT (lw), "select",
		      (GtkSignalFunc) dia_layer_select_callback,
		      (gpointer) NULL);
  gtk_signal_connect (GTK_OBJECT (lw), "deselect",
		      (GtkSignalFunc) dia_layer_deselect_callback, 
		      (gpointer) NULL);
}


guint
dia_layer_widget_get_type(void)
{
  static guint dlw_type = 0;
  GList *list;
  char *fontname;
  int i;

  if (!dlw_type) {
    GtkTypeInfo dlw_info = {
      "DiaLayerWidget",
      sizeof (DiaLayerWidget),
      sizeof (DiaLayerWidgetClass),
      (GtkClassInitFunc) dia_layer_widget_class_init,
      (GtkObjectInitFunc) dia_layer_widget_init,
      (GtkArgSetFunc) NULL,
      (GtkArgGetFunc) NULL
    };
    
    dlw_type = gtk_type_unique (gtk_list_item_get_type (), &dlw_info);
  }
  
  return dlw_type;
}

GtkWidget *
dia_layer_widget_new(Diagram *dia, Layer *layer)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET ( gtk_type_new (dia_layer_widget_get_type ()));
  dia_layer_set_layer(DIA_LAYER_WIDGET(widget), dia, layer);
  
  return widget;
}

void
dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, Layer *layer)
{
  widget->dia = dia;
  widget->layer = layer;

  dia_layer_update_from_layer(widget);
}

void
dia_layer_update_from_layer(DiaLayerWidget *widget)
{
  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(widget->visible),
			      widget->layer->visible);
  gtk_label_set(GTK_LABEL(widget->label), widget->layer->name);
}
