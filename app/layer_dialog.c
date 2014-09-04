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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <assert.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>
#undef GTK_DISABLE_DEPRECATED /* GtkListItem, gtk_list_new, ... */
#include <gtk/gtk.h>

#include "intl.h"

#include "layer_dialog.h"
#include "persistence.h"
#include "widgets.h"
#include "interface.h"

#include "dia-application.h" /* dia_diagram_change */

#include "dia-app-icons.h"

/* DiaLayerWidget: */
#define DIA_LAYER_WIDGET(obj)          \
  G_TYPE_CHECK_INSTANCE_CAST (obj, dia_layer_widget_get_type (), DiaLayerWidget)
#define DIA_LAYER_WIDGET_CLASS(klass)  \
  G_TYPE_CHECK_CLASS_CAST (klass, dia_layer_widget_get_type (), DiaLayerWidgetClass)
#define IS_DIA_LAYER_WIDGET(obj)       \
  G_TYPE_CHECK_INSTANCE_TYPE (obj, dia_layer_widget_get_type ())

typedef struct _DiaLayerWidgetClass  DiaLayerWidgetClass;
typedef struct _EditLayerDialog EditLayerDialog;

struct _DiaLayerWidget
{
  GtkListItem list_item;

  Diagram *dia;
  Layer *layer;
  
  GtkWidget *visible;
  GtkWidget *connectable;
  GtkWidget *label;

  EditLayerDialog *edit_dialog;

  /** If true, the user has set this layers connectivity to on
   * while it was not selected.
   */
  gboolean connect_on; 
  /** If true, the user has set this layers connectivity to off
   * while it was selected.
   */
  gboolean connect_off;
};

struct _EditLayerDialog {
  GtkWidget *dialog;
  GtkWidget *name_entry;
  DiaLayerWidget *layer_widget;
};


struct _DiaLayerWidgetClass
{
  GtkListItemClass parent_class;
};

GType dia_layer_widget_get_type(void);

struct LayerDialog {
  GtkWidget *dialog;
  GtkWidget *diagram_omenu;

  GtkWidget *layer_list;

  Diagram *diagram;
};

static struct LayerDialog *layer_dialog = NULL;

typedef struct _ButtonData ButtonData;

struct _ButtonData {
  gchar *stock_name;
  gpointer callback;
  char *tooltip;
};

enum LayerChangeType {
  TYPE_DELETE_LAYER,
  TYPE_ADD_LAYER,
  TYPE_RAISE_LAYER,
  TYPE_LOWER_LAYER,
};

struct LayerChange {
  Change change;

  enum LayerChangeType type;
  Layer *layer;
  int index;
  int applied;
};

struct LayerVisibilityChange {
  Change change;

  GList *original_visibility;
  Layer *layer;
  gboolean is_exclusive;
  int applied;
};

/** If TRUE, we're in the middle of a internal call to 
 * dia_layer_widget_*_toggled and should not make undo, update diagram etc.
 *
 * If these calls were not done by simulating button presses, we could avoid
 * this hack.
 */
static gboolean internal_call = FALSE;

static Change *
undo_layer(Diagram *dia, Layer *layer, enum LayerChangeType, int index);
static struct LayerVisibilityChange *
undo_layer_visibility(Diagram *dia, Layer *layer, gboolean exclusive);
static void
layer_visibility_change_apply(struct LayerVisibilityChange *change, 
			      Diagram *dia);

static GtkWidget* dia_layer_widget_new(Diagram *dia, Layer *layer);
static void dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, Layer *layer);
static void dia_layer_update_from_layer(DiaLayerWidget *widget);

static void layer_dialog_new_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_rename_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_raise_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_lower_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_delete_callback(GtkWidget *widget, gpointer gdata);
static void layer_dialog_edit_layer(DiaLayerWidget *layer_widget, Diagram *dia, Layer *layer);

static ButtonData buttons[] = {
  { GTK_STOCK_ADD, layer_dialog_new_callback, N_("New Layer") },
  { GTK_STOCK_EDIT, layer_dialog_rename_callback, N_("Rename Layer") },
  { GTK_STOCK_GO_UP, layer_dialog_raise_callback, N_("Raise Layer") },
  { GTK_STOCK_GO_DOWN, layer_dialog_lower_callback, N_("Lower Layer") },
  { GTK_STOCK_DELETE, layer_dialog_delete_callback, N_("Delete Layer") },
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

static GtkWidget *
create_button_box(GtkWidget *parent, gboolean show_labels)
{
  GtkWidget *button;
  GtkWidget *button_box;
  int i;
  
  button_box = gtk_hbox_new (TRUE, 1);

  for (i=0;i<num_buttons;i++) {
    if (show_labels == TRUE)
    {
    button = gtk_button_new_from_stock(buttons[i].stock_name);
    }
    else
    {
      GtkWidget * image;
 
      button = gtk_button_new ();
      
      image = gtk_image_new_from_stock (buttons[i].stock_name,
                                        GTK_ICON_SIZE_BUTTON);

      gtk_button_set_image (GTK_BUTTON (button), image);
    }

    g_signal_connect_swapped (G_OBJECT (button), "clicked",
			      G_CALLBACK(buttons[i].callback),
			      G_OBJECT (parent));
          
    gtk_widget_set_tooltip_text (button, gettext(buttons[i].tooltip));

    gtk_box_pack_start (GTK_BOX(button_box), button, TRUE, TRUE, 0);

    gtk_widget_show (button);
  }

  return button_box;
}

static gint
layer_list_events (GtkWidget *widget,
		   GdkEvent  *event)
{
  GdkEventKey *kevent;
  GtkWidget *event_widget;
  DiaLayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget)) {
    layer_widget = DIA_LAYER_WIDGET(event_widget);

    switch (event->type) {
    case GDK_2BUTTON_PRESS:
      layer_dialog_edit_layer(layer_widget, NULL, NULL);
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

static gboolean
layer_dialog_delete(GtkWidget *widget, gpointer data)
{
  gtk_widget_hide(widget);
  /* We're caching, so don't destroy */
  return TRUE;
}

static void
layer_view_hide_button_clicked (void * not_used)
{
  integrated_ui_layer_view_show (FALSE);
}

GtkWidget * create_layer_view_widget (void)
{
  GtkWidget  *vbox;
  GtkWidget  *hbox;
  GtkWidget  *label;
  GtkWidget  *hide_button;
  GtkRcStyle *rcstyle;    /* For hide_button */   
  GtkWidget  *image;      /* For hide_button */
  GtkWidget  *list;
  GtkWidget  *separator;
  GtkWidget  *scrolled_win;
  GtkWidget  *button_box;
  
  /* if layer_dialog were renamed to layer_view_data this would make
   * more sense.
   */
  layer_dialog = g_new (struct LayerDialog, 1);

  layer_dialog->diagram = NULL;

  layer_dialog->dialog = vbox = gtk_vbox_new (FALSE, 1);
    
  hbox = gtk_hbox_new (FALSE, 1);
  
  label = gtk_label_new (_ ("Layers:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);
  
  layer_dialog->diagram_omenu = NULL;

  /* Hide Button */
  hide_button = gtk_button_new ();
  gtk_button_set_relief (GTK_BUTTON (hide_button), GTK_RELIEF_NONE);
  gtk_button_set_focus_on_click (GTK_BUTTON (hide_button), FALSE);

  /* make it as small as possible */
  rcstyle = gtk_rc_style_new ();
  rcstyle->xthickness = rcstyle->ythickness = 0;
  gtk_widget_modify_style (hide_button, rcstyle);
  g_object_unref (rcstyle);

  image = gtk_image_new_from_stock (GTK_STOCK_CLOSE,
                                    GTK_ICON_SIZE_MENU);

  gtk_container_add (GTK_CONTAINER(hide_button), image);
  g_signal_connect (G_OBJECT (hide_button), "clicked", 
                    G_CALLBACK (layer_view_hide_button_clicked), NULL);    
    
  gtk_box_pack_start (GTK_BOX (hbox), hide_button, FALSE, FALSE, 2);
    
  gtk_box_pack_start (GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show_all (hbox);

  button_box = create_button_box(vbox, FALSE);
  
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);
    
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

  g_signal_connect (G_OBJECT (list), "event",
		    G_CALLBACK (layer_list_events), NULL);
    
  return vbox;
}

void
layer_dialog_create(void)
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
  gtk_window_set_role (GTK_WINDOW (dialog), "layer_window");
  gtk_window_set_resizable (GTK_WINDOW (dialog), TRUE);

  g_signal_connect (G_OBJECT (dialog), "delete_event",
                    G_CALLBACK(layer_dialog_delete), NULL);
  g_signal_connect (G_OBJECT (dialog), "destroy",
                    G_CALLBACK(gtk_widget_destroyed), 
		    &(layer_dialog->dialog));

  vbox = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

  hbox = gtk_hbox_new(FALSE, 1);
  
  label = gtk_label_new(_("Diagram:"));
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

  g_signal_connect (G_OBJECT (list), "event",
		    G_CALLBACK (layer_list_events), NULL);

  button_box = create_button_box(dialog, TRUE);
  
  gtk_box_pack_start (GTK_BOX (vbox), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);

  gtk_container_set_border_width(GTK_CONTAINER(
				 gtk_dialog_get_action_area (GTK_DIALOG(dialog))),
				 2);

  button = gtk_button_new_from_stock (GTK_STOCK_CLOSE);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG(dialog))), 
		      button, TRUE, TRUE, 0);
  g_signal_connect_swapped(G_OBJECT (button), "clicked",
			   G_CALLBACK(gtk_widget_hide),
			   G_OBJECT(dialog));

  gtk_widget_show (button);

  persistence_register_window(GTK_WINDOW(dialog));

  layer_dialog_update_diagram_list();
}

static void
dia_layer_select_callback(GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *lw;
  lw = DIA_LAYER_WIDGET(widget);

  /* Don't deselect if we're selected the active layer.  This can happen
   * if the window has been defocused. */
  if (lw->dia->data->active_layer != lw->layer) {
    diagram_remove_all_selected(lw->dia, TRUE);
  }
  diagram_update_extents(lw->dia);
  data_set_active_layer(lw->dia->data, lw->layer);
  diagram_add_update_all(lw->dia);
  diagram_flush(lw->dia);

  internal_call = TRUE;
  if (lw->connect_off) { /* If the user wants this off, it becomes so */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lw->connectable), FALSE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lw->connectable), TRUE);
  }
  internal_call = FALSE;
}

static void
dia_layer_deselect_callback(GtkWidget *widget, gpointer data)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(widget);

  /** If layer dialog or diagram is missing, we are so dead. */
  if (layer_dialog == NULL || layer_dialog->diagram == NULL) return;

  internal_call = TRUE;
  /** Set to on if the user has requested so. */
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(lw->connectable), 
			       lw->connect_on);
  internal_call = FALSE;
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
  static int next_layer_num = 1;

  dia = layer_dialog->diagram;

  if (dia != NULL) {
    gchar* new_layer_name = g_strdup_printf(_("New layer %d"),
					    next_layer_num++);
    layer = new_layer(new_layer_name, dia->data);

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
layer_dialog_rename_callback(GtkWidget *widget, gpointer gdata)
{
  GtkWidget *selected;
  Diagram *dia;
  Layer *layer;
  dia = layer_dialog->diagram;
  selected = GTK_LIST(layer_dialog->layer_list)->selection->data;
  layer = dia->data->active_layer;
  layer_dialog_edit_layer (DIA_LAYER_WIDGET (selected), dia, layer);
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

    data_remove_layer(dia->data, layer);
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

      g_object_ref(selected);
      
      gtk_list_remove_items(GTK_LIST(layer_dialog->layer_list),
			    list);
      
      gtk_list_insert_items(GTK_LIST(layer_dialog->layer_list),
			    list, pos - 1);

      g_object_unref(selected);

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

      g_object_ref(selected);
      
      gtk_list_remove_items(GTK_LIST(layer_dialog->layer_list),
			    list);
      
      gtk_list_insert_items(GTK_LIST(layer_dialog->layer_list),
			    list, pos + 1);

      g_object_unref(selected);

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

  if (layer_dialog == NULL || layer_dialog->dialog == NULL) {
    if (!dia_open_diagrams())
      return; /* shortcut; maybe session end w/o this dialog */
    else
      layer_dialog_create();
  }
  g_assert(layer_dialog != NULL); /* must be valid now */
  /* oh this options: here integrated UI ;( */
  if (!layer_dialog->diagram_omenu)
    return;
        
  new_menu = gtk_menu_new();

  current_nr = -1;
  
  i = 0;
  dia_list = dia_open_diagrams();
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

    g_signal_connect (G_OBJECT (menu_item), "activate",
		      G_CALLBACK (layer_dialog_select_diagram_callback), dia);

    gtk_menu_append( GTK_MENU(new_menu), menu_item);
    gtk_widget_show (menu_item);

    dia_list = g_list_next(dia_list);
    i++;
  }

  if (dia_open_diagrams()==NULL) {
    menu_item = gtk_menu_item_new_with_label (_("none"));
    g_signal_connect (G_OBJECT (menu_item), "activate",
		      G_CALLBACK (layer_dialog_select_diagram_callback), NULL);
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
    if (dia_open_diagrams()!=NULL) {
      dia = (Diagram *) dia_open_diagrams()->data;
    }
    layer_dialog_set_diagram(dia);
  }
}

void
layer_dialog_show()
{
  if (is_integrated_ui () == FALSE)
  {   
  if (layer_dialog == NULL || layer_dialog->dialog == NULL)
    layer_dialog_create();
  g_assert(layer_dialog != NULL); /* must be valid now */
  gtk_window_present(GTK_WINDOW(layer_dialog->dialog));
  }
}

/*
 * Used to avoid writing to possibly already deleted layer in
 * dia_layer_widget_connectable_toggled(). Must be called before
 * e.g. gtk_list_clear_items() cause that will emit the toggled
 * signal to last focus widget. See bug #329096
 */
static void
_layer_widget_clear_layer (GtkWidget *widget, gpointer user_data)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(widget);
  lw->layer = NULL;
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

  if (layer_dialog == NULL || layer_dialog->dialog == NULL) 
    layer_dialog_create(); /* May have been destroyed */
  g_assert(layer_dialog != NULL); /* must be valid now */

  gtk_container_foreach (GTK_CONTAINER(layer_dialog->layer_list),
                         _layer_widget_clear_layer, NULL);
  gtk_list_clear_items(GTK_LIST(layer_dialog->layer_list), 0, -1);
  layer_dialog->diagram = dia;
  if (dia != NULL) {
    i = g_list_index(dia_open_diagrams(), dia);
    if (i >= 0 && layer_dialog->diagram_omenu != NULL)
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

/* The connectability buttons don't quite behave the way they should.
 * The shift-click behavior messes up the active layer.
 * To fix this, we need to rework the code so that the setting of
 * connect_on and connect_off is not tied to the button toggling,
 * but determined by what caused it (creation, user selection,
 * shift-selection).
 */

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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->unrealize = dia_layer_widget_unrealize;
}

static void
dia_layer_widget_set_connectable(DiaLayerWidget *widget, gboolean on)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget->connectable), on);
}

static void
dia_layer_widget_exclusive_connectable(DiaLayerWidget *layer_widget)
{
  GList *list;
  DiaLayerWidget *lw;
  Layer *layer;
  int connectable = FALSE;
  int i;

  /*  First determine if _any_ other layer widgets are set to connectable  */
  for (i=0;i<layer_widget->dia->data->layers->len;i++) {
    layer = g_ptr_array_index(layer_widget->dia->data->layers, i);
    if (layer_widget->layer != layer) {
	connectable |= layer->connectable;
    }
  }

  /*  Now, toggle the connectability for all layers except the specified one  */
  list = GTK_LIST(layer_dialog->layer_list)->children;
  while (list) {
    lw = DIA_LAYER_WIDGET(list->data);
    if (lw != layer_widget) 
      dia_layer_widget_set_connectable(lw, !connectable);
    else {
      dia_layer_widget_set_connectable(lw, TRUE);
    }
    gtk_widget_queue_draw(GTK_WIDGET(lw));
    
    list = g_list_next(list);
  }
}

static gboolean shifted = FALSE;

static gboolean
dia_layer_widget_button_event(GtkWidget *widget,
			      GdkEventButton *event,
			      gpointer userdata)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(userdata);

  shifted = event->state & GDK_SHIFT_MASK;
  internal_call = FALSE;
  /* Redraw the label? */
  gtk_widget_queue_draw(GTK_WIDGET(lw));
  return FALSE;
}

static void
dia_layer_widget_connectable_toggled(GtkToggleButton *widget,
				     gpointer userdata)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(userdata);
  if (!lw->layer)
    return;
  if (shifted) {
    shifted = FALSE;
    internal_call = TRUE;
    dia_layer_widget_exclusive_connectable(lw);
    internal_call = FALSE;
  } else {
    lw->layer->connectable = gtk_toggle_button_get_active(widget);
  }
  if (lw->layer == lw->dia->data->active_layer) {
    lw->connect_off = !gtk_toggle_button_get_active(widget);
    if (lw->connect_off) lw->connect_on = FALSE;
  } else {
    lw->connect_on = gtk_toggle_button_get_active(widget);
    if (lw->connect_on) lw->connect_off = FALSE;
  }
  gtk_widget_queue_draw(GTK_WIDGET(lw));
  if (!internal_call) {
    diagram_add_update_all(lw->dia);
    diagram_flush(lw->dia);
  }
}

static void
dia_layer_widget_visible_clicked(GtkToggleButton *widget,
				 gpointer userdata)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(userdata);
  struct LayerVisibilityChange *change;

  /* Have to use this internal_call hack 'cause there's no way to switch
   * a toggle button without causing the 'clicked' event:(
   */
  if (!internal_call) {
    Diagram *dia = lw->dia;
    change = undo_layer_visibility(dia, lw->layer, shifted);
    /** This apply kills 'lw', thus we have to hold onto 'lw->dia' */
    layer_visibility_change_apply(change, dia);
    undo_set_transactionpoint(dia->undo);   
  }
}

static void
dia_layer_widget_init(DiaLayerWidget *lw)
{
  GtkWidget *hbox;
  GtkWidget *visible;
  GtkWidget *connectable;
  GtkWidget *label;

  hbox = gtk_hbox_new(FALSE, 0);

  lw->dia = NULL;
  lw->layer = NULL;
  lw->edit_dialog = NULL;

  lw->connect_on = FALSE;
  lw->connect_off = FALSE;
 
  lw->visible = visible = 
    dia_toggle_button_new_with_icons(dia_visible_icon, dia_visible_empty_icon);

  g_signal_connect(G_OBJECT(visible), "button-release-event",
		   G_CALLBACK(dia_layer_widget_button_event), lw);
  g_signal_connect(G_OBJECT(visible), "button-press-event",
		   G_CALLBACK(dia_layer_widget_button_event), lw);
  g_signal_connect(G_OBJECT(visible), "clicked",
		   G_CALLBACK(dia_layer_widget_visible_clicked), lw);
  gtk_box_pack_start (GTK_BOX (hbox), visible, FALSE, TRUE, 2);
  gtk_widget_show(visible);

  /*gtk_image_new_from_stock(GTK_STOCK_CONNECT, 
			    GTK_ICON_SIZE_BUTTON), */
  lw->connectable = connectable = 
    dia_toggle_button_new_with_icons(dia_connectable_icon,
				     dia_connectable_empty_icon);

  g_signal_connect(G_OBJECT(connectable), "button-release-event",
		   G_CALLBACK(dia_layer_widget_button_event), lw);
  g_signal_connect(G_OBJECT(connectable), "button-press-event",
		   G_CALLBACK(dia_layer_widget_button_event), lw);
  g_signal_connect(G_OBJECT(connectable), "clicked",
		   G_CALLBACK(dia_layer_widget_connectable_toggled), lw);

  gtk_box_pack_start (GTK_BOX (hbox), connectable, FALSE, TRUE, 2);
  gtk_widget_show(connectable);
  
  lw->label = label = gtk_label_new("layer_default_label");
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, TRUE, 0);
  gtk_widget_show(label);
  
  gtk_widget_show(hbox);

  gtk_container_add(GTK_CONTAINER(lw), hbox);

  g_signal_connect (G_OBJECT (lw), "select",
		    G_CALLBACK (dia_layer_select_callback), NULL);
  g_signal_connect (G_OBJECT (lw), "deselect",
		    G_CALLBACK (dia_layer_deselect_callback), NULL);
}

GType
dia_layer_widget_get_type(void)
{
  static GType dlw_type = 0;

  if (!dlw_type) {
    static const GTypeInfo dlw_info = {
      sizeof (DiaLayerWidgetClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_layer_widget_class_init,
      NULL,           /* class_finalize */
      NULL,           /* class_data */
      sizeof(DiaLayerWidget),
      0,              /* n_preallocs */
      (GInstanceInitFunc)dia_layer_widget_init,
    };
    
    dlw_type = g_type_register_static (gtk_list_item_get_type (), 
				       "DiaLayerWidget", 
				       &dlw_info, 0);
  }
  
  return dlw_type;
}

static GtkWidget *
dia_layer_widget_new(Diagram *dia, Layer *layer)
{
  GtkWidget *widget;
  
  widget = GTK_WIDGET ( gtk_type_new (dia_layer_widget_get_type ()));
  dia_layer_set_layer(DIA_LAYER_WIDGET(widget), dia, layer);

  /* These may get toggled when the button is set without the widget being
   * selected first.
   * The connect_on state gets also used to restore with just a deselect
   * of the active layer.
   */
  DIA_LAYER_WIDGET(widget)->connect_on = layer->connectable;
  DIA_LAYER_WIDGET(widget)->connect_off = FALSE;

  return widget;
}

/** Layer has either been selected or created */
static void
dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, Layer *layer)
{
  widget->dia = dia;
  widget->layer = layer;

  dia_layer_update_from_layer(widget);
}

static void
dia_layer_update_from_layer (DiaLayerWidget *widget)
{
  internal_call = TRUE;
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget->visible),
			       widget->layer->visible);
  
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget->connectable),
			       widget->layer->connectable);
  internal_call = FALSE;

  gtk_label_set_text (GTK_LABEL (widget->label), widget->layer->name);
}


/*
 *  The edit layer attributes dialog
 */
/* called from the layer widget for rename */
static void
edit_layer_ok_callback (GtkWidget *w, gpointer client_data)
{
  EditLayerDialog *dialog = (EditLayerDialog *) client_data;
  Layer *layer;
  
  g_return_if_fail (dialog->layer_widget != NULL);
  
  layer = dialog->layer_widget->layer;

  g_free (layer->name);
  layer->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)));
  /* reflect name change on listeners */
  dia_diagram_change (dialog->layer_widget->dia, DIAGRAM_CHANGE_LAYER, layer);
  
  diagram_add_update_all (dialog->layer_widget->dia);
  diagram_flush (dialog->layer_widget->dia);

  dia_layer_update_from_layer (dialog->layer_widget);

  dialog->layer_widget->edit_dialog = NULL;
  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
edit_layer_add_ok_callback (GtkWidget *w, gpointer client_data)
{
  EditLayerDialog *dialog = (EditLayerDialog *) client_data;
  Diagram *dia = ddisplay_active_diagram();
  Layer *layer;
  int pos = data_layer_get_index (dia->data, dia->data->active_layer) + 1;
  
  layer = new_layer(g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry))), dia->data);
  data_add_layer_at(dia->data, layer, pos);
  data_set_active_layer(dia->data, layer);

  diagram_add_update_all(dia);
  diagram_flush(dia);
  
  undo_layer(dia, layer, TYPE_ADD_LAYER, pos);
  undo_set_transactionpoint(dia->undo);

  /* ugly way of updating the layer widget */
  if (layer_dialog && layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }

  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
edit_layer_rename_ok_callback (GtkWidget *w, gpointer client_data)
{
  EditLayerDialog *dialog = (EditLayerDialog *) client_data;
  Diagram *dia = ddisplay_active_diagram();
  Layer *layer = dia->data->active_layer;
  
  g_free (layer->name);
  layer->name = g_strdup (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)));

  diagram_add_update_all(dia);
  diagram_flush(dia);
  /* FIXME: undo handling */

  /* ugly way of updating the layer widget */
  if (layer_dialog && layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }


  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}
static void
edit_layer_cancel_callback (GtkWidget *w,
			    gpointer   client_data)
{
  EditLayerDialog *dialog;

  dialog = (EditLayerDialog *) client_data;

  if (dialog->layer_widget)
    dialog->layer_widget->edit_dialog = NULL;
  if (dialog->dialog != NULL)
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
layer_dialog_edit_layer (DiaLayerWidget *layer_widget, Diagram *dia, Layer *layer)
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
  gtk_window_set_role (GTK_WINDOW (dialog->dialog), "edit_layer_attrributes");
  gtk_window_set_title (GTK_WINDOW (dialog->dialog), 
      (layer_widget || layer) ? _("Edit Layer") : _("Add Layer"));
  gtk_window_set_position (GTK_WINDOW (dialog->dialog), GTK_WIN_POS_MOUSE);

  /*  handle the wm close signal */
  g_signal_connect (G_OBJECT (dialog->dialog), "delete_event",
		    G_CALLBACK (edit_layer_delete_callback),
		    dialog);
  g_signal_connect (G_OBJECT (dialog->dialog), "destroy",
		    G_CALLBACK (gtk_widget_destroy),
		    &dialog->dialog);
  
  /*  the main vbox  */
  vbox = gtk_vbox_new (FALSE, 1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 2);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog->dialog))), vbox, TRUE, TRUE, 0);

  /*  the name entry hbox, label and entry  */
  hbox = gtk_hbox_new (FALSE, 1);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
  label = gtk_label_new (_("Layer name:"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
  gtk_widget_show (label);
  dialog->name_entry = gtk_entry_new ();
  gtk_entry_set_activates_default(GTK_ENTRY (dialog->name_entry), TRUE);
  gtk_box_pack_start (GTK_BOX (hbox), dialog->name_entry, TRUE, TRUE, 0);
  if (layer_widget)
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), layer_widget->layer->name);
  else if (layer)
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), layer->name);
  else if (dia) {
    gchar *name = g_strdup_printf (_("New layer %d"), dia->data->layers->len);
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), name);
    g_free (name);
  }

  gtk_widget_show (dialog->name_entry);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
#else
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
#endif
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog->dialog))), 
		      button, TRUE, TRUE, 0);
  if (layer_widget)
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK(edit_layer_ok_callback), dialog);
  else if (layer)
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK(edit_layer_rename_ok_callback), dialog);
  else if (dia)
    g_signal_connect (G_OBJECT (button), "clicked",
		      G_CALLBACK(edit_layer_add_ok_callback), dialog);

  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_from_stock (GTK_STOCK_CANCEL);
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
#else
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
#endif
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog->dialog))), 
		      button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK(edit_layer_cancel_callback),
		    dialog);
  gtk_widget_show (button);

  gtk_widget_show (vbox);
  gtk_widget_show (dialog->dialog);

  if (layer_widget)
    layer_widget->edit_dialog = dialog;
}


/******** layer changes: */

static void
layer_change_apply(struct LayerChange *change, Diagram *dia)
{
  change->applied = 1;

  switch (change->type) {
  case TYPE_DELETE_LAYER:
    data_remove_layer(dia->data, change->layer);
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
    data_remove_layer(dia->data, change->layer);
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

  change = g_new0(struct LayerChange, 1);
  
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

static void
layer_visibility_change_apply(struct LayerVisibilityChange *change, 
			      Diagram *dia)
{
  GPtrArray *layers;
  Layer *layer = change->layer;
  int visible = FALSE;
  int i;

  if (change->is_exclusive) {
    /*  First determine if _any_ other layer widgets are set to visible.
     *  If there is, exclusive switching turns all off.  */
    for (i=0;i<dia->data->layers->len;i++) {
      Layer *temp_layer = g_ptr_array_index(dia->data->layers, i);
      if (temp_layer != layer) {
	visible |= temp_layer->visible;
      }
    }

    /*  Now, toggle the visibility for all layers except the specified one  */
    layers = dia->data->layers;
    for (i = 0; i < layers->len; i++) {
      Layer *temp_layer = (Layer *) g_ptr_array_index(layers, i);
      if (temp_layer == layer) {
	temp_layer->visible = TRUE;
      } else {
	temp_layer->visible = !visible;
      }
    }
  } else {
    layer->visible = !layer->visible;
  }
  diagram_add_update_all(dia);

  if (layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }
}

/** Revert to the visibility before this change was applied.
 */
static void
layer_visibility_change_revert(struct LayerVisibilityChange *change,
			       Diagram *dia)
{
  GList *vis = change->original_visibility;
  GPtrArray *layers = dia->data->layers;
  int i;

  for (i = 0; vis != NULL && i < layers->len; vis = g_list_next(vis), i++) {
    Layer *layer = (Layer*) g_ptr_array_index(layers, i);
    layer->visible = (gboolean)vis->data;
  }

  if (vis != NULL || i < layers->len) {
    printf("Internal error: visibility undo has %d visibilities, but %d layers\n",
	   g_list_length(change->original_visibility), layers->len);
  }

  diagram_add_update_all(dia);

  if (layer_dialog->diagram == dia) {
    layer_dialog_set_diagram(dia);
  }
}

static void
layer_visibility_change_free(struct LayerVisibilityChange *change)
{
  g_list_free(change->original_visibility);
}

static struct LayerVisibilityChange *
undo_layer_visibility(Diagram *dia, Layer *layer, gboolean exclusive)
{
  struct LayerVisibilityChange *change;
  GList *visibilities = NULL;
  int i;
  GPtrArray *layers = dia->data->layers;

  change = g_new0(struct LayerVisibilityChange, 1);
  
  change->change.apply = (UndoApplyFunc) layer_visibility_change_apply;
  change->change.revert = (UndoRevertFunc) layer_visibility_change_revert;
  change->change.free = (UndoFreeFunc) layer_visibility_change_free;

  for (i = 0; i < layers->len; i++) {
    Layer *temp_layer = (Layer *) g_ptr_array_index(layers, i);
    visibilities = g_list_append(visibilities, (gpointer)temp_layer->visible);
  }

  change->original_visibility = visibilities;
  change->layer = layer;
  change->is_exclusive = exclusive;

  undo_push_change(dia->undo, (Change *) change);
  return change;
}

/*!
 * \brief edit a layers name, possibly also creating the layer
 */
void 
diagram_edit_layer(Diagram *dia, Layer *layer)
{
  g_return_if_fail(dia != NULL);
  
  layer_dialog_edit_layer (NULL, layer ? NULL : dia, layer);
}
