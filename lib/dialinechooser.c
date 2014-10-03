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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "intl.h"
#include "widgets.h"
#include "dialinechooser.h"

static const char *button_menu_key = "dia-button-menu";
static const char *menuitem_enum_key = "dia-menuitem-value";


/* --------------- DiaLinePreview -------------------------------- */

static void dia_line_preview_set(DiaLinePreview *line, LineStyle lstyle);

static void dia_line_preview_class_init (DiaLinePreviewClass  *klass);
static void dia_line_preview_init       (DiaLinePreview       *arrow);
static gint dia_line_preview_expose     (GtkWidget      *widget,
					 GdkEventExpose *event);

GType
dia_line_preview_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (DiaLinePreviewClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_line_preview_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,
      sizeof (DiaLinePreview),
      0,
      (GInstanceInitFunc) dia_line_preview_init
    };

    type = g_type_register_static (GTK_TYPE_MISC, "DiaLinePreview", &info, 0);
  }

  return type;
}

static void
dia_line_preview_class_init (DiaLinePreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS(class);
  widget_class->expose_event = dia_line_preview_expose;
}

static void
dia_line_preview_init (DiaLinePreview *line)
{
#if GTK_CHECK_VERSION(2,18,0)
  gtk_widget_set_has_window (GTK_WIDGET (line), FALSE);
#else
  GTK_WIDGET_SET_FLAGS (line, GTK_NO_WINDOW);
#endif

  GTK_WIDGET (line)->requisition.width = 30 + GTK_MISC (line)->xpad * 2;
  GTK_WIDGET (line)->requisition.height = 15 + GTK_MISC (line)->ypad * 2;

  line->lstyle = LINESTYLE_SOLID;
}

GtkWidget *
dia_line_preview_new (LineStyle lstyle)
{
  DiaLinePreview *line = g_object_new(DIA_TYPE_LINE_PREVIEW, NULL);

  line->lstyle = lstyle;
  return GTK_WIDGET(line);
}

static void
dia_line_preview_set(DiaLinePreview *line, LineStyle lstyle)
{
  if (line->lstyle != lstyle) {
    line->lstyle = lstyle;
#if GTK_CHECK_VERSION(2,18,0)
    if (gtk_widget_is_drawable(GTK_WIDGET(line)))
#else
    if (GTK_WIDGET_DRAWABLE(line))
#endif
      gtk_widget_queue_draw(GTK_WIDGET(line));
  }
}

static gint
dia_line_preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
  DiaLinePreview *line = DIA_LINE_PREVIEW(widget);
  GtkMisc *misc = GTK_MISC(widget);
  gint width, height;
  gint x, y;
  GdkWindow *win;
  GdkGC *gc;
  GdkGCValues gcvalues;
  gint8 dash_list[6];
  int line_width = 2;
  GtkStyle *style;

#if GTK_CHECK_VERSION(2,18,0)
  if (gtk_widget_is_drawable(widget)) {
#else
  if (GTK_WIDGET_DRAWABLE(widget)) {
#endif
    width = widget->allocation.width - misc->xpad * 2;
    height = widget->allocation.height - misc->ypad * 2;
    x = (widget->allocation.x + misc->xpad);
    y = (widget->allocation.y + misc->ypad);

    win = gtk_widget_get_window (widget);
    style = gtk_widget_get_style (widget);
    gc = style->fg_gc[widget->state];

    /* increase line width */
    gdk_gc_get_values(gc, &gcvalues);
    switch (line->lstyle) {
    case LINESTYLE_DEFAULT:
    case LINESTYLE_SOLID:
      gdk_gc_set_line_attributes(gc, line_width, GDK_LINE_SOLID,
				 gcvalues.cap_style, gcvalues.join_style);
      break;
    case LINESTYLE_DASHED:
      gdk_gc_set_line_attributes(gc, line_width, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 10;
      gdk_gc_set_dashes(gc, 0, dash_list, 2);
      break;
    case LINESTYLE_DASH_DOT:
      gdk_gc_set_line_attributes(gc, line_width, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 4;
      dash_list[2] = 2;
      dash_list[3] = 4;
      gdk_gc_set_dashes(gc, 0, dash_list, 4);
      break;
    case LINESTYLE_DASH_DOT_DOT:
      gdk_gc_set_line_attributes(gc, line_width, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 10;
      dash_list[1] = 2;
      dash_list[2] = 2;
      dash_list[3] = 2;
      dash_list[4] = 2;
      dash_list[5] = 2;
      gdk_gc_set_dashes(gc, 0, dash_list, 6);
      break;
    case LINESTYLE_DOTTED:
      gdk_gc_set_line_attributes(gc, line_width, GDK_LINE_ON_OFF_DASH,
				 gcvalues.cap_style, gcvalues.join_style);
      dash_list[0] = 2;
      dash_list[1] = 2;
      gdk_gc_set_dashes(gc, 0, dash_list, 2);
      break;
    }
    gdk_draw_line(win, gc, x, y+height/2, x+width, y+height/2);
    gdk_gc_set_line_attributes(gc, gcvalues.line_width, gcvalues.line_style,
			       gcvalues.cap_style, gcvalues.join_style);
  }
  return TRUE;
}


/* ------- Code for DiaLineChooser ---------------------- */

static void dia_line_chooser_class_init (DiaLineChooserClass  *klass);
static void dia_line_chooser_init       (DiaLineChooser       *arrow);

GType
dia_line_chooser_get_type (void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (DiaLineChooserClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_line_chooser_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,
      sizeof (DiaLineChooser),
      0,
      (GInstanceInitFunc) dia_line_chooser_init
    };

    type = g_type_register_static (GTK_TYPE_BUTTON, "DiaLineChooser", &info, 0);
  }

  return type;
}

static gint
dia_line_chooser_event(GtkWidget *widget, GdkEvent *event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    GtkMenu *menu = g_object_get_data(G_OBJECT(widget), button_menu_key);
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		   event->button.button, event->button.time);
    return TRUE;
  }
  return FALSE;
}

static void
dia_line_chooser_class_init (DiaLineChooserClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS(class);
  widget_class->event = dia_line_chooser_event;
}

static void
dia_line_chooser_dialog_response (GtkWidget *dialog,
                                  gint response_id,
                                  DiaLineChooser *lchooser)
{
  LineStyle new_style;
  real new_dash;

  if (response_id == GTK_RESPONSE_OK) {
    dia_line_style_selector_get_linestyle(lchooser->selector,
					  &new_style, &new_dash);
    if (new_style != lchooser->lstyle || new_dash != lchooser->dash_length) {
      lchooser->lstyle = new_style;
      lchooser->dash_length = new_dash;
      dia_line_preview_set(lchooser->preview, new_style);
      if (lchooser->callback)
        (* lchooser->callback)(new_style, new_dash, lchooser->user_data);
    }
  } else {
    dia_line_style_selector_set_linestyle(lchooser->selector,
                                          lchooser->lstyle,
                                          lchooser->dash_length);
  }
  gtk_widget_hide(lchooser->dialog);
}

static void
dia_line_chooser_change_line_style(GtkMenuItem *mi, DiaLineChooser *lchooser)
{
  LineStyle lstyle = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mi),
						       menuitem_enum_key));

  dia_line_chooser_set_line_style(lchooser, lstyle, lchooser->dash_length);
}

void
dia_line_chooser_set_line_style(DiaLineChooser *lchooser, 
				LineStyle lstyle,
				real dashlength)
{
  if (lstyle != lchooser->lstyle) {
    dia_line_preview_set(lchooser->preview, lstyle);
    lchooser->lstyle = lstyle;
    dia_line_style_selector_set_linestyle(lchooser->selector, lchooser->lstyle,
					  lchooser->dash_length);
  }
  lchooser->dash_length = dashlength;
  if (lchooser->callback)
    (* lchooser->callback)(lchooser->lstyle, lchooser->dash_length,
			   lchooser->user_data);
}

static void
dia_line_chooser_init (DiaLineChooser *lchooser)
{
  GtkWidget *wid;
  GtkWidget *menu, *mi, *ln;
  gint i;

  lchooser->lstyle = LINESTYLE_SOLID;
  lchooser->dash_length = DEFAULT_LINESTYLE_DASHLEN;

  wid = dia_line_preview_new(LINESTYLE_SOLID);
  gtk_container_add(GTK_CONTAINER(lchooser), wid);
  gtk_widget_show(wid);
  lchooser->preview = DIA_LINE_PREVIEW(wid);

  lchooser->dialog = gtk_dialog_new_with_buttons(_("Line Style Properties"),
                                                 NULL,
                                                 GTK_DIALOG_NO_SEPARATOR,
                                                 GTK_STOCK_CANCEL,
                                                 GTK_RESPONSE_CANCEL,
                                                 GTK_STOCK_OK,
                                                 GTK_RESPONSE_OK,
                                                 NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(lchooser->dialog),
                                  GTK_RESPONSE_OK);
  g_signal_connect(G_OBJECT(lchooser->dialog), "response",
                   G_CALLBACK(dia_line_chooser_dialog_response), lchooser);

  wid = dia_line_style_selector_new();
  gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area (GTK_DIALOG(lchooser->dialog))), wid,
		     TRUE, TRUE, 0);
  gtk_widget_show(wid);
  lchooser->selector = DIALINESTYLESELECTOR(wid);

  menu = gtk_menu_new();
  g_object_ref_sink(GTK_OBJECT(menu));
  g_object_set_data_full(G_OBJECT(lchooser), button_menu_key, menu,
			 (GDestroyNotify)g_object_unref);
  for (i = 0; i <= LINESTYLE_DOTTED; i++) {
    mi = gtk_menu_item_new();
    g_object_set_data(G_OBJECT(mi), menuitem_enum_key, GINT_TO_POINTER(i));
    ln = dia_line_preview_new(i);
    gtk_container_add(GTK_CONTAINER(mi), ln);
    gtk_widget_show(ln);
    g_signal_connect(G_OBJECT(mi), "activate",
		     G_CALLBACK(dia_line_chooser_change_line_style), lchooser);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    gtk_widget_show (mi);
  }
  mi = gtk_menu_item_new_with_label(_("Details\342\200\246"));
  g_signal_connect_swapped(G_OBJECT(mi), "activate",
			   G_CALLBACK(gtk_widget_show), lchooser->dialog);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  gtk_widget_show (mi);
}

GtkWidget *
dia_line_chooser_new(DiaChangeLineCallback callback,
		     gpointer user_data)
{
  DiaLineChooser *chooser = g_object_new(DIA_TYPE_LINE_CHOOSER, NULL);

  chooser->callback = callback;
  chooser->user_data = user_data;

  return GTK_WIDGET(chooser);
}

