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

#include <assert.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "config.h"
#include "intl.h"

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

enum LayerChangeType {
  TYPE_DELETE_LAYER,
  TYPE_ADD_LAYER,
  TYPE_RAISE_LAYER,
  TYPE_LOWER_LAYER
};
static Change *
undo_layer(Diagram *dia, Layer *layer, enum LayerChangeType, int index);

static void layer_dialog_new_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_raise_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_lower_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_delete_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_edit_layer(DiaLayerWidget *layer_widget);

static ButtonData buttons[] = {
  { new_xpm, layer_dialog_new_callback, N_("New Layer") },
  { raise_xpm, layer_dialog_raise_callback, N_("Raise Layer") },
  { lower_xpm, layer_dialog_lower_callback, N_("Lower Layer") },
  { delete_xpm, layer_dialog_delete_callback, N_("Delete Layer") },
};

enum {
  BUTTON_NEW = 0,
  BUTTON_RAISE,
  BUTTON_LOWER,
  BUTTON_DELETE
};

#define BUTTON_EVENT_MASK  GDK_EXPOSURE_MASK | GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | \
                           GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK

static int num_buttons = sizeof(buttons)/sizeof(ButtonData);

#define NORMAL 0
#define SELECTED 1
#define INSENSITIVE 2

static GdkPixmap *eye_pixmap[3] = {NULL, NULL, NULL};

static GtkWidget *
create_button_box(GtkWidget *parent)
{
  GtkWidget *button;
  GtkWidget *button_box;
  GtkWidget *pixmapwid;
  GdkPixmap *pixmap;
  GdkBitmap *mask;
  GdkColormap *cmap;
  GtkStyle *style;
  int i;
  
  GtkTooltips *tool_tips = NULL; /* ARG */
  
  gtk_widget_ensure_style(parent);
  style = gtk_widget_get_style(parent);
  cmap = gtk_widget_get_colormap(parent);

  button_box = gtk_hbox_new (TRUE, 1);

  for (i=0;i<num_buttons;i++) {
    pixmap = gdk_pixmap_colormap_create_from_xpm_d (NULL, cmap,
		&mask, &style->bg[GTK_STATE_NORMAL], buttons[i].xpm_data);
      
    pixmapwid =  gtk_pixmap_new (pixmap, mask);
    gtk_widget_show(pixmapwid);

    button = gtk_button_new ();
    gtk_container_add (GTK_CONTAINER (button), pixmapwid);
    gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			       (GtkSignalFunc) buttons[i].callback,
			       GTK_OBJECT (parent));
          
    if (tool_tips != NULL)
      gtk_tooltips_set_tip (tool_tips, button, gettext(buttons[i].tooltip), NULL);

    gtk_box_pack_start (GTK_BOX(button_box), button, TRUE, TRUE, 0);

    layer_dialog->buttons[i] = button;
    
    gtk_widget_show (button);
  }

  return button_box;
}

static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventKey *kevent;
  GdkEventButton *bevent;
  GtkWidget *event_widget;
  DiaLayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget)) {
    layer_widget = DIA_LAYER_WIDGET(event_widget);

    switch (event->type) {
    case GDK_BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      break;

    case GDK_2BUTTON_PRESS:
      bevent = (GdkEventButton *) event;
      layer_dialog_edit_layer(layer_widget);
      return TRUE;

    case GDK_KEY_PRESS:
      kevent = (GdkEventKey *) event;
      switch (kevent->keyval) {
      case GDK_Up:
	/* printf ("up arrow\n"); */
	break;
      case GDK_Down:
	/* printf ("down arrow\n"); */
	break;
      default:
	return FALSE;
      }
      return TRUE;

    default:
      break;
    }
  }

  return FALSE;
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
  GtkWidget *button;

  layer_dialog = g_new(struct LayerDialog, 1);

  layer_dialog->diagram = NULL;
  
  layer_dialog->dialog = dialog = gtk_dialog_new ();
  gtk_window_set_title (GTK_WINDOW (dialog), _("Layers"));
  gtk_window_set_wmclass (GTK_WINDOW (dialog),
			  "layer_window", "Dia");
  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, TRUE, TRUE);

  gtk_signal_connect (GTK_OBJECT (dialog), "delete_event",
                      GTK_SIGNAL_FUNC(gtk_widget_hide), NULL);

  
  vbox = GTK_DIALOG(dialog)->vbox;

  hbox = gtk_hbox_new(FALSE, 1);
  
  label = gtk_label_new(_("Diagrams:"));
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

  gtk_list_set_selection_mode (GTK_LIST (list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (list),
				       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));
  gtk_widget_show (scrolled_win);
  gtk_widget_show (list);

  gtk_signal_connect (GTK_OBJECT (list), "event",
		      (GtkSignalFunc) layer_list_events,
		      NULL);

  button_box = create_button_box(dialog);
  
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->action_area),
				 2);

  button = gtk_button_new_with_label (_("Close"));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect_object(GTK_OBJECT (button), "clicked",
			    GTK_SIGNAL_FUNC(gtk_widget_hide),
			    GTK_OBJECT(dialog));

  gtk_widget_show (button);

  
  layer_dialog_update_diagram_list();
}

static void
dia_layer_select_callback(GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *lw;
  lw = DIA_LAYER_WIDGET(widget);

  diagram_remove_all_selected(lw->dia, TRUE);
  diagram_update_extents(lw->dia);
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
  GtkWidget *selected;
  GList *list = NULL;
  GtkWidget *layer_widget;
  int pos;

  dia = layer_dialog->diagram;

  if (dia != NULL) {
    layer = new_layer(g_strdup(_("New layer")));

    assert(GTK_LIST(layer_dialog->layer_list)->selection != NULL);
    selected = GTK_LIST(layer_dialog->layer_list)->selection->data;
    pos = gtk_list_child_position(GTK_LIST(layer_dialog->layer_list), selected);

    data_add_layer_at(dia->data, layer, dia->data->layers->len - pos);
    
    diagram_add_update_all(dia);
    diagram_flush(dia);

    layer_widget = dia_layer_widget_new(dia, layer);
    gtk_widget_show(layer_widget);

    list = g_list_prepend(list, layer_widget);
    
    gtk_list_insert_items(GTK_LIST(layer_dialog->layer_list), list, pos);

    gtk_list_select_item(GTK_LIST(layer_dialog->layer_list), pos);

    undo_layer(dia, layer, TYPE_ADD_LAYER, dia->data->layers->len - pos);
    undo_set_transactionpoint(dia->undo);
  }
}

static void
layer_dialog_delete_callback(GtkWidget *widget, gpointer gdata)
{
  Diagram *dia;
  GtkWidget *selected;
  Layer *layer;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len>1)) {
    assert(GTK_LIST(layer_dialog->layer_list)->selection != NULL);
    selected = GTK_LIST(layer_dialog->layer_list)->selection->data;

    layer = dia->data->active_layer;

    data_delete_layer(dia->data, layer);
    diagram_add_update_all(dia);
    diagram_flush(dia);
    
    pos = gtk_list_child_position(GTK_LIST(layer_dialog->layer_list), selected);
    gtk_container_remove(GTK_CONTAINER(layer_dialog->layer_list), selected);

    undo_layer(dia, layer, TYPE_DELETE_LAYER,
	       dia->data->layers->len - pos);
    undo_set_transactionpoint(dia->undo);

    if (--pos<0)
      pos = 0;

    gtk_list_select_item(GTK_LIST(layer_dialog->layer_list), pos);
  }
}

static void
layer_dialog_raise_callback(GtkWidget *widget, gpointer gdata)
{
  Layer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len>1)) {
    assert(GTK_LIST(layer_dialog->layer_list)->selection != NULL);
    selected = GTK_LIST(layer_dialog->layer_list)->selection->data;

    pos = gtk_list_child_position(GTK_LIST(layer_dialog->layer_list), selected);

    if (pos > 0) {
      layer = DIA_LAYER_WIDGET(selected)->layer;
      data_raise_layer(dia->data, layer);
      
      list = g_list_prepend(list, selected);

      gtk_widget_ref(selected);
      
      gtk_list_remove_items(GTK_LIST(layer_dialog->layer_list),
			    list);
      
      gtk_list_insert_items(GTK_LIST(layer_dialog->layer_list),
			    list, pos - 1);

      gtk_widget_unref(selected);

      gtk_list_select_item(GTK_LIST(layer_dialog->layer_list), pos-1);
      
      diagram_add_update_all(dia);
      diagram_flush(dia);
      
      undo_layer(dia, layer, TYPE_RAISE_LAYER, 0);
      undo_set_transactionpoint(dia->undo);
    }

  }
}

static void
layer_dialog_lower_callback(GtkWidget *widget, gpointer gdata)
{
  Layer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len>1)) {
    assert(GTK_LIST(layer_dialog->layer_list)->selection != NULL);
    selected = GTK_LIST(layer_dialog->layer_list)->selection->data;

    pos = gtk_list_child_position(GTK_LIST(layer_dialog->layer_list), selected);

    if (pos < dia->data->layers->len-1) {
      layer = DIA_LAYER_WIDGET(selected)->layer;
      data_lower_layer(dia->data, layer);
      
      list = g_list_prepend(list, selected);

      gtk_widget_ref(selected);
      
      gtk_list_remove_items(GTK_LIST(layer_dialog->layer_list),
			    list);
      
      gtk_list_insert_items(GTK_LIST(layer_dialog->layer_list),
			    list, pos + 1);

      gtk_widget_unref(selected);

      gtk_list_select_item(GTK_LIST(layer_dialog->layer_list), pos+1);
      
      diagram_add_update_all(dia);
      diagram_flush(dia);

      undo_layer(dia, layer, TYPE_LOWER_LAYER, 0);
      undo_set_transactionpoint(dia->undo);
    }

  }
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
    
    filename = strrchr(dia->filename, G_DIR_SEPARATOR);
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
    menu_item = gtk_menu_item_new_with_label (_("none"));
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
  Layer *active_layer = NULL;
  int sel_pos;
  int i,j;

  
  if (dia!=NULL)
    active_layer = dia->data->active_layer;
  
  gtk_list_clear_items(GTK_LIST(layer_dialog->layer_list), 0, -1);
  layer_dialog->diagram = dia;
  if (dia != NULL) {
    i = g_list_index(open_diagrams, dia);
    if (i >= 0)
      gtk_option_menu_set_history(GTK_OPTION_MENU(layer_dialog->diagram_omenu),
				  i);
  }

  if (dia != NULL) {
    data = dia->data;

    sel_pos = 0;
    for (i=data->layers->len-1,j=0;i>=0;i--,j++) {
      layer = (Layer *) g_ptr_array_index(data->layers, i);
      layer_widget = dia_layer_widget_new(dia, layer);
      gtk_widget_show(layer_widget);
      gtk_container_add(GTK_CONTAINER(layer_dialog->layer_list), layer_widget);
      if (layer==active_layer)
	sel_pos = j;
    }
    gtk_list_select_item(GTK_LIST(layer_dialog->layer_list), sel_pos);
  }
}



/******* DiaLayerWidget: *****/

static void
dia_layer_widget_unrealize(GtkWidget *widget)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(widget);

  if (lw->edit_dialog != NULL) {
    gtk_widget_destroy(lw->edit_dialog->dialog);
    g_free(lw->edit_dialog);
    lw->edit_dialog = NULL;
  }

  (* GTK_WIDGET_CLASS (gtk_type_class(gtk_list_item_get_type ()))->unrealize) (widget);
}

static void
dia_layer_widget_class_init(DiaLayerWidgetClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  widget_class->unrealize = dia_layer_widget_unrealize;
}

static void
dia_layer_widget_visible_redraw(DiaLayerWidget *layer_widget)
{
  GdkPixmap *pixmap;
  GdkColor *color;
  GtkStateType state;

  state = GTK_WIDGET(layer_widget)->state;

  if (GTK_WIDGET_IS_SENSITIVE (layer_widget))
    {
      if (state == GTK_STATE_SELECTED)
	color = &layer_widget->visible->style->bg[GTK_STATE_SELECTED];
      else
	color = &layer_widget->visible->style->white;
    }
  else
    color = &layer_widget->visible->style->bg[GTK_STATE_INSENSITIVE];

  gdk_window_set_background (layer_widget->visible->window, color);

  if (layer_widget->layer->visible) {
    if (!eye_pixmap[NORMAL]) {
      eye_pixmap[NORMAL] =
	gdk_pixmap_create_from_data (layer_widget->visible->window,
				     (gchar*) eye_bits, eye_width, eye_height, -1,
				     &layer_widget->visible->style->fg[GTK_STATE_NORMAL],
				     &layer_widget->visible->style->white);
      eye_pixmap[SELECTED] =
	gdk_pixmap_create_from_data (layer_widget->visible->window,
				     (gchar*) eye_bits, eye_width, eye_height, -1,
				     &layer_widget->visible->style->fg[GTK_STATE_SELECTED],
				     &layer_widget->visible->style->bg[GTK_STATE_SELECTED]);
      eye_pixmap[INSENSITIVE] =
	gdk_pixmap_create_from_data (layer_widget->visible->window,
				     (gchar*) eye_bits, eye_width, eye_height, -1,
				     &layer_widget->visible->style->fg[GTK_STATE_INSENSITIVE],
				     &layer_widget->visible->style->bg[GTK_STATE_INSENSITIVE]);
    }

    if (GTK_WIDGET_IS_SENSITIVE (GTK_WIDGET(layer_widget))) {
      if (state == GTK_STATE_SELECTED)
	pixmap = eye_pixmap[SELECTED];
      else
	pixmap = eye_pixmap[NORMAL];
    } else {
      pixmap = eye_pixmap[INSENSITIVE];
    }

    gdk_draw_pixmap (layer_widget->visible->window,
		     layer_widget->visible->style->black_gc,
		     pixmap, 0, 0, 0, 0, eye_width, eye_height);
  } else {
    gdk_window_clear (layer_widget->visible->window);
  }
}

static void
dia_layer_widget_exclusive_visible(DiaLayerWidget *layer_widget)
{
  GList *list;
  DiaLayerWidget *lw;
  Layer *layer;
  int visible = FALSE;
  int i;

  /*  First determine if _any_ other layer widgets are set to visible  */
  for (i=0;i<layer_widget->dia->data->layers->len;i++) {
    layer = g_ptr_array_index(layer_widget->dia->data->layers, i);
    if (layer_widget->layer != layer) {
	visible |= layer->visible;
    }
  }

  /*  Now, toggle the visibility for all layers except the specified one  */
  for (i=0;i<layer_widget->dia->data->layers->len;i++) {
    layer = g_ptr_array_index(layer_widget->dia->data->layers, i);
    if (layer_widget->layer != layer) {
	visible |= layer->visible;
    }
  }

  list = GTK_LIST(layer_dialog->layer_list)->children;
  while (list) {
    lw = DIA_LAYER_WIDGET(list->data);
    if (lw != layer_widget)
      lw->layer->visible = !visible;
    else
      lw->layer->visible = TRUE;
    
    dia_layer_widget_visible_redraw (lw);
    
    list = g_list_next(list);
  }
}

static gint
dia_layer_widget_button_events(GtkWidget *widget,
			       GdkEvent  *event,
			       gpointer data)
{
  static GtkWidget *click_widget = NULL;
  static int button_down = 0;
  static int old_state;
  static int exclusive;
  GtkWidget *event_widget;
  gint return_val;
  GdkEventButton *bevent;
  DiaLayerWidget *lw;
  
  lw = DIA_LAYER_WIDGET(data);


  return_val = FALSE;
  switch (event->type) {
  case GDK_EXPOSE:
    if (widget == lw->visible)
      dia_layer_widget_visible_redraw (lw);
    break;
    
  case GDK_BUTTON_PRESS:
    return_val = TRUE;
    
    bevent = (GdkEventButton *) event;
    
    button_down = 1;
    click_widget = widget;
    gtk_grab_add (click_widget);
    
    if (widget == lw->visible) {
      old_state = lw->layer->visible;
      
      /*  If this was a shift-click, make all/none visible  */
      if (event->button.state & GDK_SHIFT_MASK) {
	exclusive = TRUE;
	dia_layer_widget_exclusive_visible (lw);
      } else {
	exclusive = FALSE;
	lw->layer->visible = !lw->layer->visible;
	dia_layer_widget_visible_redraw (lw);
      }
    }
    break;
    
  case GDK_BUTTON_RELEASE:
    return_val = TRUE;
    
    button_down = 0;
    gtk_grab_remove (click_widget);
    
    if (widget == lw->visible) {
      if ((exclusive) || (old_state != lw->layer->visible)) {
	diagram_add_update_all(lw->dia);
	diagram_flush(lw->dia);
      }
    } 
    break;
    
  case GDK_ENTER_NOTIFY:
  case GDK_LEAVE_NOTIFY:
    event_widget = gtk_get_event_widget (event);
    
    if (button_down && (event_widget == click_widget)) {
      if (widget == lw->visible) {
	if (exclusive) {
	  dia_layer_widget_exclusive_visible (lw);
	} else {
	  lw->layer->visible = !lw->layer->visible;
	  dia_layer_widget_visible_redraw (lw);
	}
      } 
    }
    break;
    
  default:
    break;
  }
  
  return return_val;
}

static void
dia_layer_widget_init(DiaLayerWidget *lw)
{
  GtkWidget *hbox;
  GtkWidget *visible;
  GtkWidget *label;
  GtkWidget *alignment;

  hbox = gtk_hbox_new(FALSE, 0);

  lw->dia = NULL;
  lw->layer = NULL;
  lw->edit_dialog = NULL;
  
  alignment = gtk_alignment_new (0.5, 0.5, 0.0, 0.0);
  lw->visible = visible = gtk_drawing_area_new ();
  gtk_drawing_area_size (GTK_DRAWING_AREA (visible), eye_width, eye_height);
  gtk_widget_set_events (visible, BUTTON_EVENT_MASK);
  gtk_signal_connect (GTK_OBJECT (visible), "event",
                      (GtkSignalFunc) dia_layer_widget_button_events,
                      lw);
  gtk_object_set_user_data (GTK_OBJECT (visible), lw);
  gtk_widget_show(visible);
  gtk_container_add (GTK_CONTAINER (alignment), visible);
  gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, TRUE, 2);
  gtk_widget_show(alignment);
  
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
  gtk_label_set_text(GTK_LABEL(widget->label), widget->layer->name);
}


/*
 *  The edit layer attributes dialog
 */

static void
edit_layer_ok_callback (GtkWidget *w,
			gpointer   client_data)
{
  EditLayerDialog *dialog;
  Layer *layer;
  
  dialog = (EditLayerDialog *) client_data;
  layer = dialog->layer_widget->layer;

  g_free (layer->name);
  layer->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)));
  
  diagram_add_update_all(dialog->layer_widget->dia);
  diagram_flush(dialog->layer_widget->dia);

  dia_layer_update_from_layer(dialog->layer_widget);

  dialog->layer_widget->edit_dialog = NULL;
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
edit_layer_cancel_callback (GtkWidget *w,
			    gpointer   client_data)
{
  EditLayerDialog *dialog;

  dialog = (EditLayerDialog *) client_data;

  dialog->layer_widget->edit_dialog = NULL;
  dialog = (EditLayerDialog *) client_data;
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static gint
edit_layer_delete_callback (GtkWidget *w,
			    GdkEvent *e,
			    gpointer client_data)
{
  edit_layer_cancel_callback (w, client_data);

  return TRUE;
}

static void
layer_dialog_edit_layer(DiaLayerWidget *layer_widget)
{
  EditLayerDialog *dialog;
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *button;

  /*  the new dialog structure  */
  dialog = (EditLayerDialog *) g_malloc (sizeof (EditLayerDialog));
  dialog->layer_widget = layer_widget;

  /*  the dialog  */
  dialog->dialog = gtk_dialog_new ();
  gtk_window_set_wmclass (GTK_WINDOW (dialog->dialog), "edit_layer_attrributes", "Dia");
  gtk_window_set_title (GTK_WINDOW (dialog->dialog), _("Edit Layer Attributes"));
  gtk_window_set_position (GTK_WINDOW (dialog->dialog), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal */
  gtk_signal_connect (GTK_OBJECT (dialog->dialog), "delete_event",
		      GTK_SIGNAL_FUNC (edit_layer_delete_callback),
		      dialog);

  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->vbox), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  dialog->name_entry = gtk_entry_new ();
  gtk_box_pack_start (GTK_BOX (hbox), dialog->name_entry, TRUE, TRUE, 0);
  gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), layer_widget->layer->name);
  gtk_widget_show (dialog->name_entry);
  gtk_widget_show (hbox);

  button = gtk_button_new_with_label (_("OK"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(edit_layer_ok_callback),
		      dialog);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label (_("Cancel"));
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog->dialog)->action_area), 
		      button, TRUE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC(edit_layer_cancel_callback),
		      dialog);
  gtk_widget_show (button);
  
  gtk_widget_show (vbox);
  gtk_widget_show (dialog->dialog);

  layer_widget->edit_dialog = dialog;
}


/******** layer changes: */

struct LayerChange {
  Change change;

  enum LayerChangeType type;
  Layer *layer;
  int index;
  int applied;
};

static void
layer_change_apply(struct LayerChange *change, Diagram *dia)
{
  change->applied = 1;

  switch (change->type) {
  case TYPE_DELETE_LAYER:
    data_delete_layer(dia->data, change->layer);
    break;
  case TYPE_ADD_LAYER:
    data_add_layer_at(dia->data, change->layer, change->index);
    break;
  case TYPE_RAISE_LAYER:
    data_raise_layer(dia->data, change->layer);
    break;
  case TYPE_LOWER_LAYER:
    data_lower_layer(dia->data, change->layer);
    break;
  }

  diagram_add_update_all(dia);

  if (layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }
}

static void
layer_change_revert(struct LayerChange *change, Diagram *dia)
{
  switch (change->type) {
  case TYPE_DELETE_LAYER:
    data_add_layer_at(dia->data, change->layer, change->index);
    break;
  case TYPE_ADD_LAYER:
    data_delete_layer(dia->data, change->layer);
    break;
  case TYPE_RAISE_LAYER:
    data_lower_layer(dia->data, change->layer);
    break;
  case TYPE_LOWER_LAYER:
    data_raise_layer(dia->data, change->layer);
    break;
  }

  diagram_add_update_all(dia);

  if (layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }

  change->applied = 0;
}

static void
layer_change_free(struct LayerChange *change)
{
  switch (change->type) {
  case TYPE_DELETE_LAYER:
    if (change->applied) {
      layer_destroy(change->layer);
    }
    break;
  case TYPE_ADD_LAYER:
    if (!change->applied) {
      layer_destroy(change->layer);
    }
    break;
  case TYPE_RAISE_LAYER:
    break;
  case TYPE_LOWER_LAYER:
    break;
  }
}

static Change *
undo_layer(Diagram *dia, Layer *layer, enum LayerChangeType type, int index)
{
  struct LayerChange *change;

  change = g_new(struct LayerChange, 1);
  
  change->change.apply = (UndoApplyFunc) layer_change_apply;
  change->change.revert = (UndoRevertFunc) layer_change_revert;
  change->change.free = (UndoFreeFunc) layer_change_free;

  change->type = type;
  change->layer = layer;
  change->index = index;
  change->applied = 1;

  undo_push_change(dia->undo, (Change *) change);
  return (Change *)change;
}
