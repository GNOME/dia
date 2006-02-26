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
 */

/** \file diaarrowchooser.c  A widget to choose arrowhead.  This only select arrowhead, not width  and height. 
 * \ingroup diawidgets
 */

#include <gtk/gtk.h>

#include "intl.h"
#include "widgets.h"
#include "diaarrowchooser.h"
#include "render_pixmap.h"

static const char *button_menu_key = "dia-button-menu";
static const char *menuitem_enum_key = "dia-menuitem-value";

/* --------------- DiaArrowPreview -------------------------------- */
static void dia_arrow_preview_set(DiaArrowPreview *arrow, 
                                  ArrowType atype, gboolean left);

static void dia_arrow_preview_class_init (DiaArrowPreviewClass  *klass);
static void dia_arrow_preview_init       (DiaArrowPreview       *arrow);
static gint dia_arrow_preview_expose     (GtkWidget      *widget,
					  GdkEventExpose *event);

/** Get the class information for the arrow preview widget.
 * @return A type object (statically allocated) for the arrow preview object.
 */
GType
dia_arrow_preview_get_type(void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (DiaArrowPreviewClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_arrow_preview_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,
      sizeof (DiaArrowPreview),
      0,
      (GInstanceInitFunc) dia_arrow_preview_init
    };

    type = g_type_register_static(GTK_TYPE_MISC, "DiaArrowPreview", &info, 0);
  }

  return type;
}

/** Initialize class information for the arrow preview class.
 * @param class The class object to initialize/
 */
static void
dia_arrow_preview_class_init(DiaArrowPreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->expose_event = dia_arrow_preview_expose;
}

/** Initialize an arrow preview widget.
 * @param arrow The widget to initialize.
 */
static void
dia_arrow_preview_init(DiaArrowPreview *arrow)
{
  GTK_WIDGET_SET_FLAGS (arrow, GTK_NO_WINDOW);

  GTK_WIDGET (arrow)->requisition.width = 40 + GTK_MISC (arrow)->xpad * 2;
  GTK_WIDGET (arrow)->requisition.height = 20 + GTK_MISC (arrow)->ypad * 2;

  arrow->atype = ARROW_NONE;
  arrow->left = TRUE;
}

/** Create a new arrow preview widget.
 * @param atype The type of arrow to start out selected with.
 * @param left If TRUE, this preview will point to the left.
 * @return A new widget.
 */
GtkWidget *
dia_arrow_preview_new(ArrowType atype, gboolean left)
{
  DiaArrowPreview *arrow = g_object_new(DIA_TYPE_ARROW_PREVIEW, NULL);

  arrow->atype = atype;
  arrow->left = left;
  return GTK_WIDGET(arrow);
}

/** Set the values shown by an arrow preview widget.
 * @param arrow Preview widget to change.
 * @param atype New arrow type to use.
 * @param left If TRUE, the preview should point to the left.
 */
static void
dia_arrow_preview_set(DiaArrowPreview *arrow, ArrowType atype, gboolean left)
{
  if (arrow->atype != atype || arrow->left != left) {
    arrow->atype = atype;
    arrow->left = left;
    if (GTK_WIDGET_DRAWABLE(arrow))
      gtk_widget_queue_draw(GTK_WIDGET(arrow));
  }
}

/** Expose handle for the arrow preview widget.
 * @param widget The widget to display.
 * @param event The event that caused the call.
 * @return TRUE always.
 * The expose handler gets called when the Arrow needs to be drawn.
 */
static gint
dia_arrow_preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE(widget)) {
    Point from, to;
    Point move_arrow, move_line, arrow_head;
    DiaRenderer *renderer;
    DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
    Arrow arrow_type;
    GtkMisc *misc = GTK_MISC(widget);
    gint width, height;
    gint x, y;
    GdkWindow *win;
    int linewidth = 2;
    DiaRendererClass *renderer_ops;

    width = widget->allocation.width - misc->xpad * 2;
    height = widget->allocation.height - misc->ypad * 2;
    x = (widget->allocation.x + misc->xpad);
    y = (widget->allocation.y + misc->ypad);

    win = widget->window;

    to.y = from.y = height/2;
    if (arrow->left) {
      from.x = width-linewidth;
      to.x = 0;
    } else {
      from.x = 0;
      to.x = width-linewidth;
    }

    /* here we must do some acrobaticts and construct Arrow type
     * variable
     */
    arrow_type.type = arrow->atype;
    arrow_type.length = .75*((real)height-linewidth); 
    arrow_type.width = .75*((real)height-linewidth);
    
    /* and here we calculate new arrow start and end of line points */
    calculate_arrow_point(&arrow_type, &from, &to,
                          &move_arrow, &move_line,
			  linewidth);
    arrow_head = to;
    point_add(&arrow_head, &move_arrow);
    point_add(&to, &move_line);

    renderer = new_pixmap_renderer(win, width, height);
    renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    renderer_pixmap_set_pixmap(renderer, win, x, y, width, height);
    renderer_ops->begin_render(renderer);
    renderer_ops->set_linewidth(renderer, linewidth);
    renderer_ops->draw_line(renderer, &from, &to, &color_black);
    arrow_draw (renderer, arrow_type.type, 
                &arrow_head, &from, 
		arrow_type.length, 
		arrow_type.width,
                linewidth, &color_black, &color_white);
    renderer_ops->end_render(renderer);
    g_object_unref(renderer);
  }

  return TRUE;
}


/* ------- Code for DiaArrowChooser ----------------------- */

static void dia_arrow_chooser_class_init (DiaArrowChooserClass  *klass);
static void dia_arrow_chooser_init       (DiaArrowChooser       *arrow);

/** Get the class info for the arrow chooser.
 * @return GType structure filled in for arrow chooser (statically allocated).
 */
GType
dia_arrow_chooser_get_type(void)
{
  static GType type = 0;

  if (!type) {
    static const GTypeInfo info = {
      sizeof (DiaArrowChooserClass),
      (GBaseInitFunc) NULL,
      (GBaseFinalizeFunc) NULL,
      (GClassInitFunc) dia_arrow_chooser_class_init,
      (GClassFinalizeFunc) NULL,
      NULL,
      sizeof (DiaArrowChooser),
      0,
      (GInstanceInitFunc) dia_arrow_chooser_init
    };

    type = g_type_register_static(GTK_TYPE_BUTTON, "DiaArrowChooser", &info, 0);
  }

  return type;
}

/** Generic event handle for the arrow choose.
 * This just handles popping up the arrowhead menu when the button is clicked.
 * @param widget The arrow chooser widget.
 * @param event An event affecting the arrow chooser.
 * @return TRUE if we handled the event, FALSE otherwise.
 */
static gint
dia_arrow_chooser_event(GtkWidget *widget, GdkEvent *event)
{
  if (event->type == GDK_BUTTON_PRESS && event->button.button == 1) {
    GtkMenu *menu = gtk_object_get_data(GTK_OBJECT(widget), button_menu_key);
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL,
		   event->button.button, event->button.time);
    return TRUE;
  }

  return FALSE;
}

/** Initialize class information for the arrow choose.
 * @param class Class structure to initialize private fields of.
 */
static void
dia_arrow_chooser_class_init(DiaArrowChooserClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS(class);
  widget_class->event = dia_arrow_chooser_event;
}

/** Initialize an arrow choose object.
 * @param arrow Newly allocated arrow choose object.
 */
static void
dia_arrow_chooser_init(DiaArrowChooser *arrow)
{
  GtkWidget *wid;

  arrow->left = FALSE;
  arrow->arrow.type = ARROW_NONE;
  arrow->arrow.length = DEFAULT_ARROW_LENGTH;
  arrow->arrow.width = DEFAULT_ARROW_WIDTH;

  wid = dia_arrow_preview_new(ARROW_NONE, arrow->left);
  gtk_container_add(GTK_CONTAINER(arrow), wid);
  gtk_widget_show(wid);
  arrow->preview = DIA_ARROW_PREVIEW(wid);

  arrow->dialog = NULL;
}

/** Handle the "ressponse" event for the arrow chooser dialog.
 * @param dialog The dialog that got a response.
 * @param response_id The ID of the response (e.g. GTK_RESPONSE_OK)
 * @param chooser The arrowchooser widget (userdata)
 */
static void
dia_arrow_chooser_dialog_response(GtkWidget *dialog,
				  gint response_id,
				  DiaArrowChooser *chooser)
{
  if (response_id == GTK_RESPONSE_OK) {
    Arrow new_arrow = dia_arrow_selector_get_arrow(chooser->selector);

    if (new_arrow.type   != chooser->arrow.type   ||
        new_arrow.length != chooser->arrow.length ||
        new_arrow.width  != chooser->arrow.width) {
      chooser->arrow = new_arrow;
      dia_arrow_preview_set(chooser->preview, new_arrow.type, chooser->left);
      if (chooser->callback)
        (* chooser->callback)(chooser->arrow, chooser->user_data);
    }
  } else {
    dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);
  }
  gtk_widget_hide(chooser->dialog);
}

/** Create a new arrow chooser dialog.
 * @param chooser The widget to attach a dialog to.  The dialog will be placed
 * in chooser->dialog.
 */
static void
dia_arrow_chooser_dialog_new(DiaArrowChooser *chooser)
{
  GtkWidget *wid;

  chooser->dialog = gtk_dialog_new_with_buttons(_("Arrow Properties"),
                                                NULL,
                                                GTK_DIALOG_NO_SEPARATOR,
                                                GTK_STOCK_CANCEL,
                                                GTK_RESPONSE_CANCEL,
                                                GTK_STOCK_OK,
                                                GTK_RESPONSE_OK,
                                                NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(chooser->dialog),
                                  GTK_RESPONSE_OK);
  g_signal_connect(G_OBJECT(chooser->dialog), "response",
                   G_CALLBACK(dia_arrow_chooser_dialog_response), chooser);
  g_signal_connect(G_OBJECT(chooser->dialog), "destroy",
                   G_CALLBACK(gtk_widget_destroyed), &chooser->dialog);

  wid = dia_arrow_selector_new();
  gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(chooser->dialog)->vbox), wid,
                     TRUE, TRUE, 0);
  gtk_widget_show(wid);
  chooser->selector = DIA_ARROW_SELECTOR(wid);
}

/** Display an arrow chooser dialog, creating one if necessary.
 * @param widget Ignored
 * @param chooser An arrowchooser widget to display in a dialog.  This may
 * get the dialog field set as a sideeffect.
 */
static void
dia_arrow_chooser_dialog_show(GtkWidget *widget, DiaArrowChooser *chooser)
{
  if (chooser->dialog) {
    gtk_window_present(GTK_WINDOW(chooser->dialog));
    return;
  }

  dia_arrow_chooser_dialog_new(chooser);
  dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);
  gtk_widget_show(chooser->dialog);
}

/** Set a new arrow type for an arrow chooser, as selected from a menu.
 * @param mi The menu item currently selected in the arrow chooser menu.
 * @param chooser The arrow chooser to update.
 */
static void
dia_arrow_chooser_change_arrow_type(GtkMenuItem *mi, DiaArrowChooser *chooser)
{
  ArrowType atype = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mi),
						      menuitem_enum_key));
  Arrow arrow;
  arrow.width = chooser->arrow.width;
  arrow.length = chooser->arrow.length;
  arrow.type = atype;
  dia_arrow_chooser_set_arrow(chooser, &arrow);
}

/** Create a new arrow chooser object.
 * @param left If TRUE, this chooser will point its arrowheads to the left.
 * @param callback void (*callback)(Arrow *arrow, gpointer user_data) which
 *                 will be called when the arrow type or dimensions change.
 * @param user_data Any user data.  This will be stored in chooser->user_data.
 * @param tool_tips An object to set arrow names with.
 * @return A new DiaArrowChooser widget.
 */
GtkWidget *
dia_arrow_chooser_new(gboolean left, DiaChangeArrowCallback callback,
		      gpointer user_data, GtkTooltips *tool_tips)
{
  DiaArrowChooser *chooser = g_object_new(DIA_TYPE_ARROW_CHOOSER, NULL);
  GtkWidget *menu, *mi, *ar;
  gint i;

  chooser->left = left;
  dia_arrow_preview_set(chooser->preview, chooser->preview->atype, left);
  chooser->callback = callback;
  chooser->user_data = user_data;

  menu = gtk_menu_new();
  g_object_ref(G_OBJECT(menu));
  gtk_object_sink(GTK_OBJECT(menu));
  g_object_set_data_full(G_OBJECT(chooser), button_menu_key, menu,
			 (GtkDestroyNotify)gtk_widget_unref);
  for (i = 0; arrow_types[i].name != NULL; i++) {
    mi = gtk_menu_item_new();
    g_object_set_data(G_OBJECT(mi), menuitem_enum_key,
		      GINT_TO_POINTER(arrow_types[i].enum_value));
    if (tool_tips) {
      gtk_tooltips_set_tip(tool_tips, mi, gettext(arrow_types[i].name), NULL);
    }
    ar = dia_arrow_preview_new(arrow_types[i].enum_value, left);

    gtk_container_add(GTK_CONTAINER(mi), ar);
    gtk_widget_show(ar);
    g_signal_connect(G_OBJECT(mi), "activate",
		     G_CALLBACK(dia_arrow_chooser_change_arrow_type), chooser);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    gtk_widget_show(mi);
  }
  mi = gtk_menu_item_new_with_label(_("Details..."));
  g_signal_connect(G_OBJECT(mi), "activate",
		   G_CALLBACK(dia_arrow_chooser_dialog_show), chooser);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  gtk_widget_show(mi);

  return GTK_WIDGET(chooser);
}

/** Set the type of arrow shown by the arrow chooser.  If the arrow type
 * changes, the callback function will be called.
 * @param chooser The chooser to update.
 * @param arrow The arrow type and dimensions the chooser will dispaly.
 * Should it be called as well when the dimensions change?
 */
void
dia_arrow_chooser_set_arrow(DiaArrowChooser *chooser, Arrow *arrow)
{
  if (chooser->arrow.type != arrow->type) {
    dia_arrow_preview_set(chooser->preview, arrow->type, chooser->left);
    chooser->arrow.type = arrow->type;
    if (chooser->dialog != NULL)
      dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);
    if (chooser->callback)
      (* chooser->callback)(chooser->arrow, chooser->user_data);
  }
  chooser->arrow.width = arrow->width;
  chooser->arrow.length = arrow->length;
}

/** Get the currently selected arrow type from an arrow chooser.
 * @param arrow An arrow chooser to query.
 * @return The arrow type that is currently selected in the chooser.
 */
ArrowType
dia_arrow_chooser_get_arrow_type(DiaArrowChooser *arrow)
{
  return arrow->arrow.type;
}

