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

#include <config.h>

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
#include "dia-layer.h"

#include "dia-application.h" /* dia_diagram_change */

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
  DiaLayer *layer;

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

  GtkWidget *editor;
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

enum {
  COL_FILENAME,
  COL_DIAGRAM
};

struct LayerDialog {
  GtkWidget *dialog;

  GtkWidget *layer_editor;

  Diagram *diagram;
};

static struct LayerDialog *layer_dialog = NULL;

typedef struct _ButtonData ButtonData;

struct _ButtonData {
  gchar *icon_name;
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
  DiaLayer *layer;
  int index;
  int applied;
};

struct LayerVisibilityChange {
  Change change;

  GList *original_visibility;
  DiaLayer *layer;
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
undo_layer(Diagram *dia, DiaLayer *layer, enum LayerChangeType, int index);
static struct LayerVisibilityChange *
undo_layer_visibility(Diagram *dia, DiaLayer *layer, gboolean exclusive);
static void
layer_visibility_change_apply(struct LayerVisibilityChange *change,
			      Diagram *dia);

static GtkWidget* dia_layer_widget_new(Diagram *dia, DiaLayer *layer, DiaLayerEditor *editor);
static void dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, DiaLayer *layer);
static void dia_layer_update_from_layer(DiaLayerWidget *widget);


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

  /* if layer_dialog were renamed to layer_view_data this would make
   * more sense.
   */
  layer_dialog = g_new0 (struct LayerDialog, 1);

  layer_dialog->diagram = NULL;

  layer_dialog->dialog = vbox = gtk_vbox_new (FALSE, 1);

  hbox = gtk_hbox_new (FALSE, 1);

  label = gtk_label_new (_ ("Layers"));
  gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 2);
  gtk_widget_show (label);

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

  gtk_container_add (GTK_CONTAINER (hide_button), image);
  g_signal_connect (G_OBJECT (hide_button), "clicked",
                    G_CALLBACK (layer_view_hide_button_clicked), NULL);

  gtk_box_pack_end (GTK_BOX (hbox), hide_button, FALSE, FALSE, 2);
  gtk_widget_show_all (hbox);

  gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 2);

  layer_dialog->layer_editor = dia_layer_editor_new ();
  gtk_widget_show (layer_dialog->layer_editor);
  gtk_box_pack_start (GTK_BOX (vbox), layer_dialog->layer_editor, TRUE, TRUE, 0);

  return vbox;
}


void
layer_dialog_create (void)
{
  layer_dialog = g_new0(struct LayerDialog, 1);

  layer_dialog->diagram = NULL;

  layer_dialog->dialog = dia_layer_editor_dialog_new ();
  gtk_widget_show (layer_dialog->dialog);
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
layer_dialog_set_diagram (Diagram *dia)
{
  if (layer_dialog == NULL || layer_dialog->dialog == NULL) {
    layer_dialog_create (); /* May have been destroyed */
  }

  g_assert (layer_dialog != NULL); /* must be valid now */

  if (DIA_IS_LAYER_EDITOR_DIALOG (layer_dialog->dialog)) {
    dia_layer_editor_dialog_set_diagram (DIA_LAYER_EDITOR_DIALOG (layer_dialog->dialog),
                                         dia);
  } else {
    dia_layer_editor_set_diagram (DIA_LAYER_EDITOR (layer_dialog->layer_editor),
                                  dia);
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

enum {
  EXCLUSIVE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0, };


static void
dia_layer_widget_class_init(DiaLayerWidgetClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  widget_class->unrealize = dia_layer_widget_unrealize;

  signals[EXCLUSIVE] =
    g_signal_new ("exclusive",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL,
                  NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
dia_layer_widget_set_connectable(DiaLayerWidget *widget, gboolean on)
{
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget->connectable), on);
}


static gboolean shifted = FALSE;

static gboolean
dia_layer_widget_button_event (GtkWidget      *widget,
                               GdkEventButton *event,
                               gpointer        userdata)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET(userdata);

  shifted = event->state & GDK_SHIFT_MASK;

  internal_call = FALSE;
  /* Redraw the label? */
  gtk_widget_queue_draw(GTK_WIDGET(lw));
  return FALSE;
}

static void
dia_layer_widget_connectable_toggled (GtkToggleButton *widget,
                                      gpointer         userdata)
{
  DiaLayerWidget *lw = DIA_LAYER_WIDGET (userdata);

  if (!lw->layer)
    return;

  if (shifted) {
    shifted = FALSE;
    internal_call = TRUE;
    g_signal_emit (lw, signals[EXCLUSIVE], 0);
    internal_call = FALSE;
  } else {
    dia_layer_set_connectable (lw->layer,
                               gtk_toggle_button_get_active (widget));
  }
  if (lw->layer == lw->dia->data->active_layer) {
    lw->connect_off = !gtk_toggle_button_get_active (widget);
    if (lw->connect_off) lw->connect_on = FALSE;
  } else {
    lw->connect_on = gtk_toggle_button_get_active (widget);
    if (lw->connect_on) lw->connect_off = FALSE;
  }

  gtk_widget_queue_draw (GTK_WIDGET (lw));

  if (!internal_call) {
    diagram_add_update_all (lw->dia);
    diagram_flush (lw->dia);
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

  lw->visible = visible = dia_toggle_button_new_with_icon_names ("dia-visible",
                                                                 "dia-visible-empty");

  lw->editor = NULL;

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
    dia_toggle_button_new_with_icon_names ("dia-connectable",
                                           "dia-connectable-empty");

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
dia_layer_widget_new (Diagram *dia, DiaLayer *layer, DiaLayerEditor *editor)
{
  GtkWidget *widget;

  widget = GTK_WIDGET (gtk_type_new (dia_layer_widget_get_type ()));
  dia_layer_set_layer (DIA_LAYER_WIDGET (widget), dia, layer);

  /* These may get toggled when the button is set without the widget being
   * selected first.
   * The connect_on state gets also used to restore with just a deselect
   * of the active layer.
   */
  DIA_LAYER_WIDGET (widget)->connect_on = dia_layer_is_connectable (layer);
  DIA_LAYER_WIDGET (widget)->connect_off = FALSE;
  DIA_LAYER_WIDGET (widget)->editor = GTK_WIDGET (editor);

  return widget;
}

/** Layer has either been selected or created */
static void
dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, DiaLayer *layer)
{
  widget->dia = dia;
  widget->layer = layer;

  dia_layer_update_from_layer(widget);
}


static void
dia_layer_update_from_layer (DiaLayerWidget *widget)
{
  internal_call = TRUE;
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->visible),
                                dia_layer_is_visible (widget->layer));

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget->connectable),
                                dia_layer_is_connectable (widget->layer));
  internal_call = FALSE;

  gtk_label_set_text (GTK_LABEL (widget->label),
                      dia_layer_get_name (widget->layer));
}


/*
 *  The edit layer attributes dialog
 */
/* called from the layer widget for rename */
static void
edit_layer_ok_callback (GtkWidget *w, gpointer client_data)
{
  EditLayerDialog *dialog = (EditLayerDialog *) client_data;
  DiaLayer *layer;

  g_return_if_fail (dialog->layer_widget != NULL);

  layer = dialog->layer_widget->layer;

  g_object_set (layer,
                "name", gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)),
                NULL);
  /* reflect name change on listeners */
  dia_application_diagram_change (dia_application_get_default (),
                                  dialog->layer_widget->dia,
                                  DIAGRAM_CHANGE_LAYER,
                                  layer);

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
  Diagram *dia = ddisplay_active_diagram ();
  DiaLayer *layer;
  int pos = data_layer_get_index (dia->data, dia->data->active_layer) + 1;

  layer = dia_layer_new (gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)), dia->data);
  data_add_layer_at (dia->data, layer, pos);
  data_set_active_layer (dia->data, layer);

  diagram_add_update_all (dia);
  diagram_flush (dia);

  undo_layer (dia, layer, TYPE_ADD_LAYER, pos);
  undo_set_transactionpoint (dia->undo);

  /* ugly way of updating the layer widget */
  if (layer_dialog && layer_dialog->diagram == dia) {
    layer_dialog_set_diagram (dia);
  }

  gtk_widget_destroy (dialog->dialog);
  g_free (dialog);
}

static void
edit_layer_rename_ok_callback (GtkWidget *w, gpointer client_data)
{
  EditLayerDialog *dialog = (EditLayerDialog *) client_data;
  Diagram *dia = ddisplay_active_diagram();
  DiaLayer *layer = dia->data->active_layer;

  g_object_set (layer,
                "name", gtk_entry_get_text (GTK_ENTRY (dialog->name_entry)),
                NULL);

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
layer_dialog_edit_layer (DiaLayerWidget *layer_widget, Diagram *dia, DiaLayer *layer)
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
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), dia_layer_get_name (layer_widget->layer));
  else if (layer)
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), dia_layer_get_name (layer));
  else if (dia) {
    gchar *name = g_strdup_printf (_("New layer %d"), dia->data->layers->len);
    gtk_entry_set_text (GTK_ENTRY (dialog->name_entry), name);
    g_free (name);
  }

  gtk_widget_show (dialog->name_entry);
  gtk_widget_show (hbox);

  button = gtk_button_new_from_stock (GTK_STOCK_OK);
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
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
  gtk_widget_set_can_default (GTK_WIDGET (button), TRUE);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_action_area (GTK_DIALOG (dialog->dialog))),
                      button, TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (button), "clicked",
                    G_CALLBACK (edit_layer_cancel_callback),
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
layer_change_free (struct LayerChange *change)
{
  switch (change->type) {
    case TYPE_DELETE_LAYER:
      if (change->applied) {
        g_clear_object (&change->layer);
      }
      break;
    case TYPE_ADD_LAYER:
      if (!change->applied) {
        g_clear_object (&change->layer);
      }
      break;
    case TYPE_RAISE_LAYER:
      break;
    case TYPE_LOWER_LAYER:
      break;
  }
}

static Change *
undo_layer(Diagram *dia, DiaLayer *layer, enum LayerChangeType type, int index)
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
  DiaLayer *layer = change->layer;
  int visible = FALSE;
  int i;

  if (change->is_exclusive) {
    /*  First determine if _any_ other layer widgets are set to visible.
     *  If there is, exclusive switching turns all off.  */
    for (i=0;i<dia->data->layers->len;i++) {
      DiaLayer *temp_layer = g_ptr_array_index(dia->data->layers, i);
      if (temp_layer != layer) {
        visible |= dia_layer_is_visible (temp_layer);
      }
    }

    /*  Now, toggle the visibility for all layers except the specified one  */
    layers = dia->data->layers;
    for (i = 0; i < layers->len; i++) {
      DiaLayer *temp_layer = (DiaLayer *) g_ptr_array_index(layers, i);
      if (temp_layer == layer) {
        dia_layer_set_visible (temp_layer, TRUE);
      } else {
        dia_layer_set_visible (temp_layer, !visible);
      }
    }
  } else {
    dia_layer_set_visible (layer, !dia_layer_is_visible (layer));
  }
  diagram_add_update_all (dia);

  if (layer_dialog->diagram == dia) {
    layer_dialog_set_diagram (dia);
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
    DiaLayer *layer = DIA_LAYER (g_ptr_array_index (layers, i));
    dia_layer_set_visible (layer, GPOINTER_TO_INT (vis->data));
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
undo_layer_visibility(Diagram *dia, DiaLayer *layer, gboolean exclusive)
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
    DiaLayer *temp_layer = DIA_LAYER (g_ptr_array_index (layers, i));
    visibilities = g_list_append (visibilities, GINT_TO_POINTER (dia_layer_is_visible (temp_layer)));
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
diagram_edit_layer(Diagram *dia, DiaLayer *layer)
{
  g_return_if_fail(dia != NULL);

  layer_dialog_edit_layer (NULL, layer ? NULL : dia, layer);
}

typedef struct _DiaLayerEditorPrivate DiaLayerEditorPrivate;
struct _DiaLayerEditorPrivate {
  GtkWidget *list;
  Diagram   *diagram;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerEditor, dia_layer_editor, GTK_TYPE_VBOX)

enum {
  PROP_0,
  PROP_DIAGRAM,
  LAST_PROP
};

static GParamSpec *pspecs[LAST_PROP] = { NULL, };


static void
dia_layer_editor_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      dia_layer_editor_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_get_property (GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);

  switch (property_id) {
    case PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_editor_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_finalize (GObject *object)
{
  DiaLayerEditor *self = DIA_LAYER_EDITOR (object);
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);

  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_layer_editor_parent_class)->finalize (object);
}


static void
dia_layer_editor_class_init (DiaLayerEditorClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->set_property = dia_layer_editor_set_property;
  object_class->get_property = dia_layer_editor_get_property;
  object_class->finalize = dia_layer_editor_finalize;

  /**
   * DiaLayerEditor:diagram:
   *
   * Since: 0.98
   */
  pspecs[PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, pspecs);
}


static void rename_layer (GtkWidget *widget, DiaLayerEditor *self);

static gint
list_event (GtkWidget      *widget,
            GdkEvent       *event,
            DiaLayerEditor *self)
{
  GdkEventKey *kevent;
  GtkWidget *event_widget;
  DiaLayerWidget *layer_widget;

  event_widget = gtk_get_event_widget (event);

  if (GTK_IS_LIST_ITEM (event_widget)) {
    layer_widget = DIA_LAYER_WIDGET (event_widget);

    switch (event->type) {
      case GDK_2BUTTON_PRESS:
        rename_layer (GTK_WIDGET (layer_widget), self);
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


static void
exclusive_connectable (DiaLayerWidget *layer_row,
                       DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  GList *list;
  DiaLayerWidget *lw;
  DiaLayer *layer;
  int connectable = FALSE;
  int i;

  /*  First determine if _any_ other layer widgets are set to connectable  */
  for (i = 0; i < data_layer_count (DIA_DIAGRAM_DATA (priv->diagram)); i++) {
    layer = data_layer_get_nth (DIA_DIAGRAM_DATA (priv->diagram), i);

    if (layer_row->layer != layer) {
      connectable |= dia_layer_is_connectable (layer);
    }
  }

  /*  Now, toggle the connectability for all layers except the specified one  */
  list = GTK_LIST (priv->list)->children;
  while (list) {
    lw = DIA_LAYER_WIDGET (list->data);
    if (lw != layer_row) {
      dia_layer_widget_set_connectable (lw, !connectable);
    } else {
      dia_layer_widget_set_connectable (lw, TRUE);
    }
    gtk_widget_queue_draw (GTK_WIDGET (lw));

    list = g_list_next (list);
  }
}


static void
new_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  GtkWidget *layer_widget;
  int pos;
  static int next_layer_num = 1;

  dia = layer_dialog->diagram;

  if (dia != NULL) {
    gchar* new_layer_name = g_strdup_printf (_("New layer %d"),
                                             next_layer_num++);
    layer = dia_layer_new (new_layer_name, dia->data);

    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;
    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    data_add_layer_at (dia->data, layer, dia->data->layers->len - pos);

    diagram_add_update_all (dia);
    diagram_flush (dia);

    layer_widget = dia_layer_widget_new (dia, layer, self);
    g_signal_connect (layer_widget,
                      "exclusive",
                      G_CALLBACK (exclusive_connectable),
                      self);
    gtk_widget_show (layer_widget);

    list = g_list_prepend (list, layer_widget);

    gtk_list_insert_items (GTK_LIST (priv->list), list, pos);

    gtk_list_select_item (GTK_LIST (priv->list), pos);

    undo_layer (dia, layer, TYPE_ADD_LAYER, dia->data->layers->len - pos);
    undo_set_transactionpoint (dia->undo);

    g_free (new_layer_name);
  }
}


static void
rename_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  GtkWidget *selected;
  Diagram *dia;
  DiaLayer *layer;

  dia = layer_dialog->diagram;

  if (dia == NULL) {
    return;
  }

  selected = GTK_LIST (priv->list)->selection->data;
  layer = dia->data->active_layer;
  layer_dialog_edit_layer (DIA_LAYER_WIDGET (selected), dia, layer);
}


static void
delete_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  Diagram *dia;
  GtkWidget *selected;
  DiaLayer *layer;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    layer = dia->data->active_layer;

    data_remove_layer (dia->data, layer);
    diagram_add_update_all (dia);
    diagram_flush (dia);

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);
    gtk_container_remove (GTK_CONTAINER (priv->list), selected);

    undo_layer (dia, layer, TYPE_DELETE_LAYER,
                dia->data->layers->len - pos);
    undo_set_transactionpoint (dia->undo);

    if (--pos < 0) {
      pos = 0;
    }

    gtk_list_select_item (GTK_LIST (priv->list), pos);
  }
}


static void
raise_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    if (pos > 0) {
      layer = DIA_LAYER_WIDGET (selected)->layer;
      data_raise_layer (dia->data, layer);

      list = g_list_prepend (list, selected);

      g_object_ref (selected);

      gtk_list_remove_items (GTK_LIST (priv->list),
                             list);

      gtk_list_insert_items (GTK_LIST (priv->list),
                             list, pos - 1);

      g_object_unref (selected);

      gtk_list_select_item (GTK_LIST (priv->list), pos-1);

      diagram_add_update_all (dia);
      diagram_flush (dia);

      undo_layer (dia, layer, TYPE_RAISE_LAYER, 0);
      undo_set_transactionpoint (dia->undo);
    }
  }
}


static void
lower_layer (GtkWidget *widget, DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);
  DiaLayer *layer;
  Diagram *dia;
  GtkWidget *selected;
  GList *list = NULL;
  int pos;

  dia = layer_dialog->diagram;

  if ((dia != NULL) && (dia->data->layers->len > 1)) {
    assert (GTK_LIST (priv->list)->selection != NULL);
    selected = GTK_LIST (priv->list)->selection->data;

    pos = gtk_list_child_position (GTK_LIST (priv->list), selected);

    if (pos < dia->data->layers->len-1) {
      layer = DIA_LAYER_WIDGET (selected)->layer;
      data_lower_layer (dia->data, layer);

      list = g_list_prepend (list, selected);

      g_object_ref (selected);

      gtk_list_remove_items (GTK_LIST (priv->list),
                             list);

      gtk_list_insert_items (GTK_LIST (priv->list),
                             list, pos + 1);

      g_object_unref (selected);

      gtk_list_select_item (GTK_LIST (priv->list), pos+1);

      diagram_add_update_all (dia);
      diagram_flush (dia);

      undo_layer (dia, layer, TYPE_LOWER_LAYER, 0);
      undo_set_transactionpoint (dia->undo);
    }
  }
}


static ButtonData editor_buttons[] = {
  { "list-add", new_layer, N_("New Layer") },
  { "list-remove", delete_layer, N_("Delete Layer") },
  { "document-properties", rename_layer, N_("Rename Layer") },
  { "go-up", raise_layer, N_("Raise Layer") },
  { "go-down", lower_layer, N_("Lower Layer") },
};


static void
dia_layer_editor_init (DiaLayerEditor *self)
{
  GtkWidget *scrolled_win;
  GtkWidget *button_box;
  GtkWidget *button;
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);

  gtk_container_set_border_width (GTK_CONTAINER (self), 6);
  g_object_set (self, "width-request", 250, NULL);

  priv->diagram = NULL;

  scrolled_win = gtk_scrolled_window_new (NULL, NULL);
  gtk_widget_show (scrolled_win);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_win),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_win),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);
  gtk_box_pack_start (GTK_BOX (self), scrolled_win, TRUE, TRUE, 2);

  priv->list = gtk_list_new ();
  gtk_widget_show (priv->list);
  gtk_list_set_selection_mode (GTK_LIST (priv->list), GTK_SELECTION_BROWSE);
  gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_win), priv->list);
  gtk_container_set_focus_vadjustment (GTK_CONTAINER (priv->list),
                                       gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (scrolled_win)));

  g_signal_connect (G_OBJECT (priv->list),
                    "event",
                    G_CALLBACK (list_event),
                    self);

  button_box = gtk_hbox_new (FALSE, 0);

  for (int i = 0; i < G_N_ELEMENTS (editor_buttons); i++) {
    GtkWidget * image;

    button = gtk_button_new ();

    image = gtk_image_new_from_icon_name (editor_buttons[i].icon_name,
                                          GTK_ICON_SIZE_BUTTON);

    gtk_button_set_image (GTK_BUTTON (button), image);

    g_signal_connect (G_OBJECT (button),
                      "clicked",
                      G_CALLBACK (editor_buttons[i].callback),
                      self);

    gtk_widget_set_tooltip_text (button, gettext (editor_buttons[i].tooltip));

    gtk_box_pack_start (GTK_BOX (button_box), button, TRUE, TRUE, 0);

    gtk_widget_show (button);
  }

  gtk_box_pack_start (GTK_BOX (self), button_box, FALSE, FALSE, 2);
  gtk_widget_show (button_box);
}


GtkWidget *
dia_layer_editor_new (void)
{
  return g_object_new (DIA_TYPE_LAYER_EDITOR,
                       NULL);
}


void
dia_layer_editor_set_diagram (DiaLayerEditor *self,
                              Diagram        *dia)
{
  DiaLayerEditorPrivate *priv;
  GtkWidget *layer_widget;
  DiaLayer *layer;
  DiaLayer *active_layer = NULL;
  int sel_pos;
  int i,j;

  g_return_if_fail (DIA_IS_LAYER_EDITOR (self));

  priv = dia_layer_editor_get_instance_private (self);

  g_clear_object (&priv->diagram);

  if (dia == NULL) {
    g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);

    return;
  }

  priv->diagram = g_object_ref (dia);

  active_layer = DIA_DIAGRAM_DATA (dia)->active_layer;

  gtk_container_foreach (GTK_CONTAINER (priv->list),
                         _layer_widget_clear_layer, NULL);
  gtk_list_clear_items (GTK_LIST (priv->list), 0, -1);

  sel_pos = 0;
  for (i = data_layer_count (DIA_DIAGRAM_DATA (dia)) - 1, j = 0; i >= 0; i--, j++) {
    layer = data_layer_get_nth (DIA_DIAGRAM_DATA (dia), i);

    layer_widget = dia_layer_widget_new (dia, layer, self);
    gtk_widget_show (layer_widget);

    gtk_container_add (GTK_CONTAINER (priv->list),
                       layer_widget);

    if (layer == active_layer) {
      sel_pos = j;
    }
  }

  gtk_list_select_item (GTK_LIST (priv->list), sel_pos);

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[PROP_DIAGRAM]);
}


Diagram *
dia_layer_editor_get_diagram (DiaLayerEditor *self)
{
  DiaLayerEditorPrivate *priv = dia_layer_editor_get_instance_private (self);

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR (self), NULL);

  priv = dia_layer_editor_get_instance_private (self);

  return priv->diagram;
}





typedef struct _DiaLayerEditorDialogPrivate DiaLayerEditorDialogPrivate;
struct _DiaLayerEditorDialogPrivate {
  Diagram      *diagram;

  GtkListStore *store;

  GtkWidget    *combo;
  GtkWidget    *editor;
};

G_DEFINE_TYPE_WITH_PRIVATE (DiaLayerEditorDialog, dia_layer_editor_dialog, GTK_TYPE_DIALOG)

enum {
  DLG_PROP_0,
  DLG_PROP_DIAGRAM,
  LAST_DLG_PROP
};

static GParamSpec *dlg_pspecs[LAST_DLG_PROP] = { NULL, };


static void
dia_layer_editor_dialog_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);

  switch (property_id) {
    case DLG_PROP_DIAGRAM:
      dia_layer_editor_dialog_set_diagram (self, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_dialog_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);


  switch (property_id) {
    case DLG_PROP_DIAGRAM:
      g_value_set_object (value, dia_layer_editor_dialog_get_diagram (self));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
dia_layer_editor_dialog_finalize (GObject *object)
{
  DiaLayerEditorDialog *self = DIA_LAYER_EDITOR_DIALOG (object);
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  g_clear_object (&priv->diagram);

  G_OBJECT_CLASS (dia_layer_editor_dialog_parent_class)->finalize (object);
}


static gboolean
dia_layer_editor_dialog_delete_event (GtkWidget *widget, GdkEventAny *event)
{
  gtk_widget_hide (widget);

  /* We're caching, so don't destroy */
  return TRUE;
}


static void
dia_layer_editor_dialog_class_init (DiaLayerEditorDialogClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_layer_editor_dialog_set_property;
  object_class->get_property = dia_layer_editor_dialog_get_property;
  object_class->finalize = dia_layer_editor_dialog_finalize;

  widget_class->delete_event = dia_layer_editor_dialog_delete_event;

  /**
   * DiaLayerEditorDialog:diagram:
   *
   * Since: 0.98
   */
  dlg_pspecs[DLG_PROP_DIAGRAM] =
    g_param_spec_object ("diagram",
                         "Diagram",
                         "The current diagram",
                         DIA_TYPE_DIAGRAM,
                         G_PARAM_STATIC_STRINGS | G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_DLG_PROP, dlg_pspecs);
}


static void
diagrams_changed (GListModel *diagrams,
                  guint       position,
                  guint       removed,
                  guint       added,
                  gpointer    self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  Diagram *dia;
  int i;
  int current_nr;
  GtkTreeIter iter;
  char *basename = NULL;

  g_return_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self));

  current_nr = -1;

  gtk_list_store_clear (priv->store);

  if (g_list_model_get_n_items (diagrams) < 1) {
    gtk_widget_set_sensitive (self, FALSE);

    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   NULL);

    return;
  }

  gtk_widget_set_sensitive (self, TRUE);

  i = 0;
  while ((dia = DIA_DIAGRAM (g_list_model_get_item (diagrams, i)))) {
    basename = g_path_get_basename (dia->filename);

    gtk_list_store_append (priv->store, &iter);
    gtk_list_store_set (priv->store,
                        &iter,
                        COL_FILENAME,
                        basename,
                        COL_DIAGRAM,
                        dia,
                        -1);

    if (dia == priv->diagram) {
      current_nr = i;
      gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                     &iter);
    }

    i++;

    g_free (basename);
    g_clear_object (&dia);
  }

  if (current_nr == -1) {
    dia = NULL;
    if (g_list_model_get_n_items (diagrams) > 0) {
      dia = g_list_model_get_item (diagrams, i);
    }

    layer_dialog_set_diagram (dia);

    g_clear_object (&dia);
  }
}


static void
diagram_changed (GtkComboBox *widget,
                 gpointer     self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  GtkTreeIter iter;
  Diagram *diagram = NULL;

  if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (priv->combo),
                                     &iter)) {
    gtk_tree_model_get (GTK_TREE_MODEL (priv->store),
                        &iter,
                        COL_DIAGRAM,
                        &diagram,
                        -1);
  }

  layer_dialog_set_diagram (diagram);

  g_clear_object (&diagram);
}


static void
dia_layer_editor_dialog_init (DiaLayerEditorDialog *self)
{
  GtkWidget *vbox;
  GtkWidget *hbox;
  GtkCellRenderer *renderer;
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  priv->diagram = NULL;

  gtk_window_set_title (GTK_WINDOW (self), _("Layers"));
  gtk_window_set_role (GTK_WINDOW (self), "layer_window");
  gtk_window_set_resizable (GTK_WINDOW (self), TRUE);

  g_signal_connect (G_OBJECT (self), "destroy",
                    G_CALLBACK (gtk_widget_destroyed),
                    self);

  vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));

  hbox = gtk_hbox_new (FALSE, 1);

  priv->store = gtk_list_store_new (2,
                                    G_TYPE_STRING,
                                    DIA_TYPE_DIAGRAM);
  priv->combo = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->store));

  g_signal_connect (dia_application_get_diagrams (dia_application_get_default ()),
                    "items-changed",
                    G_CALLBACK (diagrams_changed),
                    self);

  g_signal_connect (priv->combo,
                    "changed",
                    G_CALLBACK (diagram_changed),
                    self);

  renderer = gtk_cell_renderer_text_new ();
  gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combo),
                              renderer,
                              TRUE);
  gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combo),
                                  renderer,
                                  "text", COL_FILENAME,
                                  NULL);


  gtk_box_pack_start (GTK_BOX (hbox), priv->combo, TRUE, TRUE, 2);
  gtk_widget_show (priv->combo);

  gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 2);
  gtk_widget_show (hbox);

  priv->editor = dia_layer_editor_new ();
  g_object_bind_property (self, "diagram",
                          priv->editor, "diagram",
                          G_BINDING_DEFAULT);
  gtk_box_pack_start (GTK_BOX (vbox), priv->editor, TRUE, TRUE, 2);
  gtk_widget_show (priv->editor);

  gtk_dialog_add_button (GTK_DIALOG (self), _("_Close"), GTK_RESPONSE_CLOSE);

  gtk_container_set_border_width (GTK_CONTAINER (gtk_dialog_get_action_area (GTK_DIALOG (self))),
                                  2);

  persistence_register_window (GTK_WINDOW (self));
}


GtkWidget *
dia_layer_editor_dialog_new (void)
{
  return g_object_new (DIA_TYPE_LAYER_EDITOR_DIALOG, NULL);
}


gboolean
find_diagram (GtkTreeModel *model,
              GtkTreePath  *path,
              GtkTreeIter  *iter,
              gpointer      self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);
  Diagram *diagram = NULL;

  gtk_tree_model_get (model, iter, COL_DIAGRAM, &diagram, -1);

  if (diagram == priv->diagram) {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   iter);

    g_clear_object (&diagram);

    return TRUE;
  }

  g_clear_object (&diagram);

  return FALSE;
}


void
dia_layer_editor_dialog_set_diagram (DiaLayerEditorDialog *self,
                                     Diagram              *dia)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  g_return_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self));

  priv = dia_layer_editor_dialog_get_instance_private (self);

  g_clear_object (&priv->diagram);
  if (dia) {
    priv->diagram = g_object_ref (dia);

    g_signal_handlers_block_by_func (priv->combo, diagram_changed, self);
    gtk_tree_model_foreach (GTK_TREE_MODEL (priv->store),
                            find_diagram,
                            self);
    g_signal_handlers_unblock_by_func (priv->combo, diagram_changed, self);
  } else {
    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (priv->combo),
                                   NULL);
  }

  g_object_notify_by_pspec (G_OBJECT (self), pspecs[DLG_PROP_DIAGRAM]);
}


Diagram *
dia_layer_editor_dialog_get_diagram (DiaLayerEditorDialog *self)
{
  DiaLayerEditorDialogPrivate *priv = dia_layer_editor_dialog_get_instance_private (self);

  g_return_val_if_fail (DIA_IS_LAYER_EDITOR_DIALOG (self), NULL);

  priv = dia_layer_editor_dialog_get_instance_private (self);

  return priv->diagram;
}
