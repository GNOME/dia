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

#include <gtk/gtk.h>

#include "intl.h"
#include "widgets.h"
#include "diaarrowchooser.h"
#include "render_pixmap.h"

static const char *button_menu_key = "dia-button-menu";
static const char *menuitem_enum_key = "dia-menuitem-value";

static gint close_and_hide(GtkWidget *wid, GdkEventAny *event) 
{
  gtk_widget_hide(wid);
  return TRUE;
}

/* --------------- DiaArrowPreview -------------------------------- */
static void dia_arrow_preview_set(DiaArrowPreview *arrow, 
                                  ArrowType atype, gboolean left);

static void dia_arrow_preview_class_init (DiaArrowPreviewClass  *klass);
static void dia_arrow_preview_init       (DiaArrowPreview       *arrow);
static gint dia_arrow_preview_expose     (GtkWidget      *widget,
					  GdkEventExpose *event);

GtkType
dia_arrow_preview_get_type (void)
{
  static GtkType arrow_type = 0;

  if (!arrow_type) {
      static const GtkTypeInfo arrow_info = {
        "DiaArrowPreview",
        sizeof (DiaArrowPreview),
        sizeof (DiaArrowPreviewClass),
        (GtkClassInitFunc) dia_arrow_preview_class_init,
        (GtkObjectInitFunc) dia_arrow_preview_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      arrow_type = gtk_type_unique (GTK_TYPE_MISC, &arrow_info);
    }
  return arrow_type;
}

static void
dia_arrow_preview_class_init (DiaArrowPreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->expose_event = dia_arrow_preview_expose;
}

static void
dia_arrow_preview_init (DiaArrowPreview *arrow)
{
  GTK_WIDGET_SET_FLAGS (arrow, GTK_NO_WINDOW);

  GTK_WIDGET (arrow)->requisition.width = 30 + GTK_MISC (arrow)->xpad * 2;
  GTK_WIDGET (arrow)->requisition.height = 20 + GTK_MISC (arrow)->ypad * 2;

  arrow->atype = ARROW_NONE;
  arrow->left = TRUE;
}

GtkWidget *
dia_arrow_preview_new (ArrowType atype, gboolean left)
{
  DiaArrowPreview *arrow = gtk_type_new (dia_arrow_preview_get_type());

  arrow->atype = atype;
  arrow->left = left;
  return GTK_WIDGET(arrow);
}

static void
dia_arrow_preview_set(DiaArrowPreview *arrow, ArrowType atype, gboolean left)
{
  if (arrow->atype != atype || arrow->left != left) {
    arrow->atype = atype;
    arrow->left = left;
    if (GTK_WIDGET_DRAWABLE(arrow))
      gtk_widget_queue_clear(GTK_WIDGET(arrow));
  }
}

static gint
dia_arrow_preview_expose(GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE(widget)) {
    Point from, to;
    DiaRenderer *renderer;
    DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
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
    renderer = new_pixmap_renderer(win, width, height);
    renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    renderer_pixmap_set_pixmap(renderer, win, x, y, width, height);
    renderer_ops->begin_render(renderer);
    renderer_ops->set_linewidth(renderer, linewidth);
    renderer_ops->draw_line(renderer, &to, &from, &color_black);
    arrow_draw (renderer, arrow->atype, 
                &to, &from, 
		.75*((real)height-linewidth), 
		.75*((real)height-linewidth),
                linewidth, &color_black, &color_white);
    renderer_ops->end_render(renderer);
    g_object_unref(renderer);
  }

  return TRUE;
}


/* ------- Code for DiaArrowChooser ----------------------- */




static void dia_arrow_chooser_class_init (DiaArrowChooserClass  *klass);
static void dia_arrow_chooser_init       (DiaArrowChooser       *arrow);
static gint dia_arrow_chooser_event      (GtkWidget *widget,
					  GdkEvent *event);
static void dia_arrow_chooser_dialog_ok  (DiaArrowChooser *arrow);
static void dia_arrow_chooser_dialog_cancel (DiaArrowChooser *arrow);
static void dia_arrow_chooser_dialog_destroy (DiaArrowChooser *arrow);
static void dia_arrow_chooser_change_arrow_type (GtkMenuItem *mi,
						 DiaArrowChooser *arrow);
static void dia_arrow_chooser_dialog_new(GtkWidget *widget, gpointer userdata);
static void dia_arrow_chooser_dialog_show(GtkWidget *widget, gpointer userdata);

GtkType
dia_arrow_chooser_get_type (void)
{
  static GtkType arrow_type = 0;

  if (!arrow_type) {
      static const GtkTypeInfo arrow_info = {
        "DiaArrowChooser",
        sizeof (DiaArrowChooser),
        sizeof (DiaArrowChooserClass),
        (GtkClassInitFunc) dia_arrow_chooser_class_init,
        (GtkObjectInitFunc) dia_arrow_chooser_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };
      arrow_type = gtk_type_unique (GTK_TYPE_BUTTON, &arrow_info);
    }
  return arrow_type;
}

static void
dia_arrow_chooser_class_init (DiaArrowChooserClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = (GtkWidgetClass *)class;
  widget_class->event = dia_arrow_chooser_event;
}

static void
dia_arrow_chooser_init (DiaArrowChooser *arrow)
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

/* Creating the dialog separately so we can handle destroy */
static void
dia_arrow_chooser_dialog_new(GtkWidget *widget, gpointer userdata)
{
  DiaArrowChooser *chooser = DIA_ARROW_CHOOSER(userdata);
  if (chooser->dialog == NULL) {
    GtkWidget *wid;
    chooser->dialog = wid = gtk_dialog_new();
    gtk_window_set_title(GTK_WINDOW(wid), _("Arrow Properties"));
    g_signal_connect(GTK_OBJECT(wid), "delete_event",
		       G_CALLBACK(close_and_hide), NULL);
    g_signal_connect(GTK_OBJECT(wid), "destroy",
		       G_CALLBACK(gtk_widget_destroyed),
		       &chooser->dialog);

    wid = dia_arrow_selector_new();
    gtk_container_set_border_width(GTK_CONTAINER(wid), 5);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(chooser->dialog)->vbox), wid,
		       TRUE, TRUE, 0);
    gtk_widget_show(wid);
    chooser->selector = DIAARROWSELECTOR(wid);
    dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);

    wid = gtk_button_new_with_label(_("OK"));
    GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(chooser->dialog)->action_area),wid);
    gtk_widget_grab_default(wid);
    g_signal_connect_swapped(GTK_OBJECT(wid), "clicked",
			      G_CALLBACK(dia_arrow_chooser_dialog_ok),
			      GTK_OBJECT(chooser));
    gtk_widget_show(wid);

    wid = gtk_button_new_with_label(_("Cancel"));
    GTK_WIDGET_SET_FLAGS(wid, GTK_CAN_DEFAULT);
    gtk_container_add(GTK_CONTAINER(GTK_DIALOG(chooser->dialog)->action_area),wid);
    g_signal_connect_swapped(GTK_OBJECT(wid), "clicked",
			      G_CALLBACK(dia_arrow_chooser_dialog_cancel),
			      GTK_OBJECT(chooser));
    gtk_widget_show(wid);
  }
}

static void
dia_arrow_chooser_dialog_show(GtkWidget *widget, gpointer userdata)
{
     DiaArrowChooser *chooser = DIA_ARROW_CHOOSER(userdata);
     dia_arrow_chooser_dialog_new(widget, chooser);
     dia_arrow_selector_set_arrow(chooser->selector, chooser->arrow);
     gtk_widget_show(chooser->dialog);
}

GtkWidget *
dia_arrow_chooser_new(gboolean left, DiaChangeArrowCallback callback,
		      gpointer user_data, GtkTooltips *tool_tips)
{
  DiaArrowChooser *chooser = gtk_type_new(dia_arrow_chooser_get_type());
  GtkWidget *menu, *mi, *ar;
  gint i;

  chooser->left = left;
  dia_arrow_preview_set(chooser->preview, chooser->preview->atype, left);
  chooser->callback = callback;
  chooser->user_data = user_data;

  dia_arrow_chooser_dialog_new(NULL, chooser);

  menu = gtk_menu_new();
  gtk_object_set_data_full(GTK_OBJECT(chooser), button_menu_key, menu,
			   (GtkDestroyNotify)gtk_widget_unref);
  for (i = 0; arrow_types[i].name != NULL; i++) {
    mi = gtk_menu_item_new();
    gtk_object_set_data(GTK_OBJECT(mi), menuitem_enum_key,
			GINT_TO_POINTER(arrow_types[i].enum_value));
    if (tool_tips) {
      gtk_tooltips_set_tip(tool_tips, mi, arrow_types[i].name, NULL);
    }
    ar = dia_arrow_preview_new(arrow_types[i].enum_value, left);

    gtk_container_add(GTK_CONTAINER(mi), ar);
    gtk_widget_show(ar);
    g_signal_connect(GTK_OBJECT(mi), "activate",
		       G_CALLBACK(dia_arrow_chooser_change_arrow_type),
		       chooser);
    gtk_container_add(GTK_CONTAINER(menu), mi);
    gtk_widget_show(mi);
  }
  mi = gtk_menu_item_new_with_label(_("Details..."));
  g_signal_connect(GTK_OBJECT(mi), "activate",
		     G_CALLBACK(dia_arrow_chooser_dialog_show),
		     GTK_OBJECT(chooser));
  gtk_container_add(GTK_CONTAINER(menu), mi);
  gtk_widget_show(mi);

  return GTK_WIDGET(chooser);
}

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

static void
dia_arrow_chooser_dialog_ok  (DiaArrowChooser *arrow)
{
  Arrow new_arrow = dia_arrow_selector_get_arrow(arrow->selector);

  if (new_arrow.type   != arrow->arrow.type ||
      new_arrow.length != arrow->arrow.length ||
      new_arrow.width  != arrow->arrow.width) {
    arrow->arrow = new_arrow;
    dia_arrow_preview_set(arrow->preview, new_arrow.type, arrow->left);
    if (arrow->callback)
      (* arrow->callback)(arrow->arrow, arrow->user_data);
  }
  gtk_widget_hide(arrow->dialog);
}

static void
dia_arrow_chooser_dialog_cancel (DiaArrowChooser *arrow)
{
  if (arrow->dialog != NULL)
    dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
  gtk_widget_hide(arrow->dialog);
}

static void
dia_arrow_chooser_dialog_destroy (DiaArrowChooser *chooser)
{
  /*  dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);*/
  chooser->dialog = NULL;
  chooser->selector = NULL;
}

static void
dia_arrow_chooser_change_arrow_type(GtkMenuItem *mi, DiaArrowChooser *arrow)
{
  ArrowType atype = GPOINTER_TO_INT(gtk_object_get_data(GTK_OBJECT(mi),
							menuitem_enum_key));
  dia_arrow_chooser_set_arrow_type(arrow, atype);
}

/** Set the type of arrow shown by the arrow chooser.
 */
void
dia_arrow_chooser_set_arrow_type(DiaArrowChooser *arrow, ArrowType atype) {
  if (arrow->arrow.type != atype) {
    dia_arrow_preview_set(arrow->preview, atype, arrow->left);
    arrow->arrow.type = atype;
    if (arrow->dialog != NULL)
      dia_arrow_selector_set_arrow(arrow->selector, arrow->arrow);
    if (arrow->callback)
      (* arrow->callback)(arrow->arrow, arrow->user_data);
  }
}

ArrowType dia_arrow_chooser_get_arrow_type(DiaArrowChooser *arrow) {
  return arrow->arrow.type;
}
