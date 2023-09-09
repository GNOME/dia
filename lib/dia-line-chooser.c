/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dialinechooser.c -- Copyright (C) 1999 James Henstridge.
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

#include "dia-line-style-selector.h"
#include "dia-line-chooser.h"
#include "dia-line-preview.h"

struct _DiaLineChooser {
  GtkButton button;
  DiaLinePreview *preview;
  DiaLineStyle lstyle;
  double dash_length;

  GtkMenu *menu;

  DiaChangeLineCallback callback;
  gpointer user_data;

  GtkWidget *dialog;
  DiaLineStyleSelector *selector;
};

G_DEFINE_TYPE (DiaLineChooser, dia_line_chooser, GTK_TYPE_BUTTON)


static void
dia_line_chooser_dispose (GObject *object)
{
  DiaLineChooser *self = DIA_LINE_CHOOSER (object);

  g_clear_object (&self->menu);

  G_OBJECT_CLASS (dia_line_chooser_parent_class)->dispose (object);
}


static int
dia_line_chooser_button_press_event (GtkWidget *widget, GdkEventButton *event)
{
  DiaLineChooser *self = DIA_LINE_CHOOSER (widget);

  if (event->button == 1) {
    gtk_menu_popup_at_widget (self->menu,
                              widget,
                              GDK_GRAVITY_EAST,
                              GDK_GRAVITY_WEST,
                              (GdkEvent*)event);

    return TRUE;
  }

  return FALSE;
}


static void
dia_line_chooser_class_init (DiaLineChooserClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->dispose = dia_line_chooser_dispose;

  widget_class->button_press_event = dia_line_chooser_button_press_event;
}


static void
dia_line_chooser_dialog_response (GtkWidget      *dialog,
                                  int             response_id,
                                  DiaLineChooser *lchooser)
{
  DiaLineStyle new_style;
  double new_dash;

  if (response_id == GTK_RESPONSE_OK) {
    dia_line_style_selector_get_linestyle (lchooser->selector,
                                           &new_style,
                                           &new_dash);

    if (new_style != lchooser->lstyle || new_dash != lchooser->dash_length) {
      lchooser->lstyle = new_style;
      lchooser->dash_length = new_dash;
      dia_line_preview_set_style (lchooser->preview, new_style);

      if (lchooser->callback)
        (* lchooser->callback) (new_style, new_dash, lchooser->user_data);
    }
  } else {
    dia_line_style_selector_set_linestyle (lchooser->selector,
                                           lchooser->lstyle,
                                           lchooser->dash_length);
  }

  gtk_widget_hide (lchooser->dialog);
}


static void
dia_line_chooser_change_line_style (GtkMenuItem *mi, DiaLineChooser *lchooser)
{
  DiaLineStyle lstyle = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (mi), "line-style"));

  dia_line_chooser_set_line_style (lchooser, lstyle, lchooser->dash_length);
}


void
dia_line_chooser_set_line_style (DiaLineChooser *lchooser,
                                 DiaLineStyle    lstyle,
                                 double          dashlength)
{
  if (lstyle != lchooser->lstyle) {
    dia_line_preview_set_style (lchooser->preview, lstyle);
    lchooser->lstyle = lstyle;
    dia_line_style_selector_set_linestyle (lchooser->selector,
                                           lchooser->lstyle,
                                           lchooser->dash_length);
  }

  lchooser->dash_length = dashlength;

  if (lchooser->callback)
    (* lchooser->callback) (lchooser->lstyle,
                            lchooser->dash_length,
                            lchooser->user_data);
}


static void
dia_line_chooser_init (DiaLineChooser *lchooser)
{
  GtkWidget *wid;
  GtkWidget *mi, *ln;
  int i;

  lchooser->lstyle = DIA_LINE_STYLE_SOLID;
  lchooser->dash_length = DEFAULT_LINESTYLE_DASHLEN;

  wid = dia_line_preview_new (DIA_LINE_STYLE_SOLID);
  gtk_container_add (GTK_CONTAINER (lchooser), wid);
  gtk_widget_show (wid);
  lchooser->preview = DIA_LINE_PREVIEW (wid);

  lchooser->dialog = gtk_dialog_new_with_buttons (_("Line Style Properties"),
                                                  NULL,
                                                  0,
                                                  _("_Cancel"), GTK_RESPONSE_CANCEL,
                                                  _("_OK"), GTK_RESPONSE_OK,
                                                  NULL);
  gtk_dialog_set_default_response (GTK_DIALOG (lchooser->dialog),
                                   GTK_RESPONSE_OK);
  g_signal_connect (G_OBJECT (lchooser->dialog),
                    "response", G_CALLBACK (dia_line_chooser_dialog_response),
                    lchooser);

  wid = dia_line_style_selector_new ();
  gtk_container_set_border_width (GTK_CONTAINER (wid), 5);
  gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG(lchooser->dialog))),
                      wid,
                      TRUE,
                      TRUE,
                      0);
  gtk_widget_show (wid);
  lchooser->selector = DIA_LINE_STYLE_SELECTOR (wid);

  lchooser->menu = GTK_MENU (g_object_ref_sink (gtk_menu_new ()));
  for (i = 0; i <= DIA_LINE_STYLE_DOTTED; i++) {
    mi = gtk_menu_item_new ();
    g_object_set_data (G_OBJECT (mi),
                      "line-style",
                      GINT_TO_POINTER (i));
    ln = dia_line_preview_new (i);
    gtk_container_add (GTK_CONTAINER (mi), ln);
    gtk_widget_show (ln);
    g_signal_connect (G_OBJECT (mi),
                      "activate", G_CALLBACK (dia_line_chooser_change_line_style),
                      lchooser);
    gtk_menu_shell_append (GTK_MENU_SHELL (lchooser->menu), mi);
    gtk_widget_show (mi);
  }
  mi = gtk_menu_item_new_with_label (_("Details…"));
  g_signal_connect_swapped (G_OBJECT (mi),
                            "activate", G_CALLBACK (gtk_widget_show),
                            lchooser->dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (lchooser->menu), mi);
  gtk_widget_show (mi);
}


GtkWidget *
dia_line_chooser_new (DiaChangeLineCallback callback,
                      gpointer              user_data)
{
  DiaLineChooser *chooser = g_object_new (DIA_TYPE_LINE_CHOOSER, NULL);

  chooser->callback = callback;
  chooser->user_data = user_data;

  return GTK_WIDGET (chooser);
}
