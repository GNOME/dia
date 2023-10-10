/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diarrowchooser.c -- Copyright (C) 1999 James Henstridge.
 *                     Copyright (C) 2004 Hubert Figuiere
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
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "config.h"

#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "dia-arrow-selector.h"
#include "dia-arrow-chooser.h"
#include "dia-arrow-preview.h"

/**
 * DiaArrowChooser:
 *
 * A widget to choose arrowhead. This only select arrowhead, not width and height.
 */
struct _DiaArrowChooser {
  GtkButton button;
  DiaArrowPreview *preview;
  Arrow arrow;
  gboolean left;

  DiaChangeArrowCallback callback;
  gpointer user_data;

  GtkWidget *menu;
  GtkWidget *dialog;
  DiaArrowSelector *selector;
};

G_DEFINE_TYPE (DiaArrowChooser, dia_arrow_chooser, GTK_TYPE_BUTTON)


static void
dia_arrow_chooser_dispose (GObject *object)
{
  DiaArrowChooser *chooser = DIA_ARROW_CHOOSER (object);

  g_clear_object (&chooser->menu);

  G_OBJECT_CLASS (dia_arrow_chooser_parent_class)->dispose (object);
}


/**
 * dia_arrow_chooser_event:
 * @widget: The arrow chooser widget.
 * @event: An event affecting the arrow chooser.
 *
 * Generic event handle for the arrow choose.
 *
 * This just handles popping up the arrowhead menu when the button is clicked.
 *
 * Returns: %TRUE if we handled the event, %FALSE otherwise.
 *
 * Since: 0.98
 */
static int
dia_arrow_chooser_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  if (event->button == 1) {
    gtk_menu_popup_at_widget (GTK_MENU (DIA_ARROW_CHOOSER (widget)->menu),
                              widget,
                              GDK_GRAVITY_EAST,
                              GDK_GRAVITY_WEST,
                              (GdkEvent*)event);

    return TRUE;
  }

  return FALSE;
}


static void
dia_arrow_chooser_class_init (DiaArrowChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dia_arrow_chooser_dispose;

  widget_class->button_press_event = dia_arrow_chooser_button_press_event;
}


static void
dia_arrow_chooser_init (DiaArrowChooser *arrow)
{
  GtkWidget *wid;

  arrow->left = FALSE;
  arrow->arrow.type = ARROW_NONE;
  arrow->arrow.length = DEFAULT_ARROW_LENGTH;
  arrow->arrow.width = DEFAULT_ARROW_WIDTH;

  wid = dia_arrow_preview_new (ARROW_NONE, arrow->left);
  gtk_container_add (GTK_CONTAINER (arrow), wid);
  gtk_widget_show (wid);
  arrow->preview = DIA_ARROW_PREVIEW (wid);

  arrow->dialog = NULL;
}


/**
 * dia_arrow_chooser_dialog_response:
 * @dialog: The dialog that got a response.
 * @response_id: The ID of the response (e.g. %GTK_RESPONSE_OK)
 * @chooser: The arrowchooser widget (userdata)
 *
 * Handle the "response" event for the arrow chooser dialog.
 */
static void
dia_arrow_chooser_dialog_response (GtkWidget       *dialog,
                                   int              response_id,
                                   DiaArrowChooser *chooser)
{
  if (response_id == GTK_RESPONSE_OK) {
    Arrow new_arrow = dia_arrow_selector_get_arrow (chooser->selector);

    if (new_arrow.type   != chooser->arrow.type   ||
        new_arrow.length != chooser->arrow.length ||
        new_arrow.width  != chooser->arrow.width) {
      chooser->arrow = new_arrow;
      dia_arrow_preview_set_arrow (chooser->preview, new_arrow.type, chooser->left);
      if (chooser->callback) {
        (* chooser->callback) (chooser->arrow, chooser->user_data);
      }
    }
  } else {
    dia_arrow_selector_set_arrow (chooser->selector, chooser->arrow);
  }

  gtk_widget_hide (chooser->dialog);
}


/**
 * dia_arrow_chooser_dialog_new:
 * @chooser: The widget to attach a dialog to. The dialog will be placed
 * in chooser->dialog.
 *
 * Create a new arrow chooser dialog.
 *
 * Since: 0.98
 */
static void
dia_arrow_chooser_dialog_new (DiaArrowChooser *chooser)
{
  GtkWidget *wid;

  chooser->dialog = gtk_dialog_new_with_buttons (_("Arrow Properties"),
                                                 NULL,
                                                 0,
                                                 _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                 _("_OK"), GTK_RESPONSE_OK,
                                                 NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (chooser->dialog),
                                   GTK_RESPONSE_OK);
  g_signal_connect (G_OBJECT (chooser->dialog), "response",
                    G_CALLBACK (dia_arrow_chooser_dialog_response), chooser);
  g_signal_connect (G_OBJECT (chooser->dialog), "destroy",
                    G_CALLBACK (gtk_widget_destroyed), &chooser->dialog);

  wid = dia_arrow_selector_new ();
  gtk_container_set_border_width (GTK_CONTAINER (wid), 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (chooser->dialog))),
                      wid,
                      TRUE,
                      TRUE,
                      0);
  gtk_widget_show (wid);
  chooser->selector = DIA_ARROW_SELECTOR (wid);
}


/**
 * dia_arrow_chooser_dialog_show:
 * @widget: Ignored
 * @chooser: An arrowchooser widget to display in a dialog. This may get the
 *           dialog field set as a sideeffect.
 *
 * Display an arrow chooser dialog, creating one if necessary.
 *
 * Since: dawn-of-time
 */
static void
dia_arrow_chooser_dialog_show (GtkWidget *widget, DiaArrowChooser *chooser)
{
  if (chooser->dialog) {
    gtk_window_present (GTK_WINDOW (chooser->dialog));
    return;
  }

  dia_arrow_chooser_dialog_new (chooser);
  dia_arrow_selector_set_arrow (chooser->selector, chooser->arrow);
  gtk_widget_show (chooser->dialog);
}


/**
 * dia_arrow_chooser_change_arrow_type:
 * @mi: The menu item currently selected in the arrow chooser menu.
 * @chooser: The arrow chooser to update.
 *
 * Set a new arrow type for an arrow chooser, as selected from a menu.
 *
 * Since: dawn-of-time
 */
static void
dia_arrow_chooser_change_arrow_type (GtkMenuItem     *mi,
                                     DiaArrowChooser *chooser)
{
  ArrowType atype = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), "arrow-type"));
  Arrow arrow;
  arrow.width = chooser->arrow.width;
  arrow.length = chooser->arrow.length;
  arrow.type = atype;
  dia_arrow_chooser_set_arrow (chooser, &arrow);
}


/**
 * dia_arrow_chooser_new:
 * @left: If %TRUE, this chooser will point its arrowheads to the left.
 * @callback: void (*callback)(Arrow *arrow, gpointer user_data) which
 *                 will be called when the arrow type or dimensions change.
 * @user_data: Any user data.  This will be stored in chooser->user_data.
 *
 * Create a new arrow chooser object.
 *
 * Returns: A new DiaArrowChooser widget.
 *
 * Since: dawn-of-time
 */
GtkWidget *
dia_arrow_chooser_new (gboolean               left,
                       DiaChangeArrowCallback callback,
                       gpointer               user_data)
{
  DiaArrowChooser *chooser = g_object_new (DIA_TYPE_ARROW_CHOOSER, NULL);
  GtkWidget *mi, *ar;
  int i;

  chooser->left = left;
  dia_arrow_preview_set_arrow (chooser->preview,
                               dia_arrow_preview_get_arrow (chooser->preview),
                               left);
  chooser->callback = callback;
  chooser->user_data = user_data;

  chooser->menu = g_object_ref_sink (gtk_menu_new ());

  /* although from ARROW_NONE to MAX_ARROW_TYPE-1 this is sorted by *index* to keep the order consistent with earlier releases */
  for (i = ARROW_NONE; i < MAX_ARROW_TYPE; ++i) {
    ArrowType arrow_type = arrow_type_from_index (i);
    mi = gtk_menu_item_new ();
    g_object_set_data (G_OBJECT (mi),
                       "arrow-type",
                       GINT_TO_POINTER (arrow_type));
    gtk_widget_set_tooltip_text (mi,
                                 gettext (arrow_get_name_from_type (arrow_type)));
    ar = dia_arrow_preview_new (arrow_type, left);

    gtk_container_add (GTK_CONTAINER (mi), ar);
    gtk_widget_show (ar);
    g_signal_connect (G_OBJECT (mi),
                      "activate",
                      G_CALLBACK (dia_arrow_chooser_change_arrow_type),
                      chooser);
    gtk_menu_shell_append (GTK_MENU_SHELL (chooser->menu), mi);
    gtk_widget_show (mi);
  }

  mi = gtk_menu_item_new_with_label (_("Details…"));
  g_signal_connect (G_OBJECT (mi),
                    "activate",
                    G_CALLBACK (dia_arrow_chooser_dialog_show),
                    chooser);
  gtk_menu_shell_append (GTK_MENU_SHELL (chooser->menu), mi);
  gtk_widget_show (mi);

  return GTK_WIDGET (chooser);
}


/**
 * dia_arrow_chooser_set_arrow:
 * @chooser: The chooser to update.
 * @arrow: The arrow type and dimensions the chooser will display.
 * Should it be called as well when the dimensions change?
 *
 * Set the type of arrow shown by the arrow chooser. If the arrow type
 * changes, the callback function will be called.
 *
 * Since: dawn-of-time
 */
void
dia_arrow_chooser_set_arrow (DiaArrowChooser *chooser, Arrow *arrow)
{
  if (chooser->arrow.type != arrow->type) {
    dia_arrow_preview_set_arrow (chooser->preview, arrow->type, chooser->left);
    chooser->arrow.type = arrow->type;
    if (chooser->dialog != NULL) {
      dia_arrow_selector_set_arrow (chooser->selector, chooser->arrow);
    }
    if (chooser->callback) {
      (* chooser->callback) (chooser->arrow, chooser->user_data);
    }
  }
  chooser->arrow.width = arrow->width;
  chooser->arrow.length = arrow->length;
}


/**
 * dia_arrow_chooser_get_arrow_type:
 * @arrow: An arrow chooser to query.
 *
 * Get the currently selected arrow type from an arrow chooser.
 *
 * Returns: The arrow type that is currently selected in the chooser.
 *
 * Since: dawn-of-time
 */
ArrowType
dia_arrow_chooser_get_arrow_type (DiaArrowChooser *arrow)
{
  return arrow->arrow.type;
}
