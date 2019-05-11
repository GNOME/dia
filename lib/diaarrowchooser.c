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
#include <config.h>

#include <gtk/gtk.h>
#include "intl.h"
#include "widgets.h"
#include "diaarrowchooser.h"
#include "renderer/diacairo.h"

static const char *button_menu_key = "dia-button-menu";
static const char *menuitem_enum_key = "dia-menuitem-value";

static const gchar*
_dia_translate (const gchar* term, gpointer data)
{
  const gchar* trans = term;

  if (term && *term) {
    /* first try our own ... */
    trans = dgettext (GETTEXT_PACKAGE, term);
    /* ... than gtk */
    if (term == trans)
      trans = dgettext ("gtk20", term);
  }
  return trans;
}

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
  gtk_widget_set_has_window (GTK_WIDGET (arrow), FALSE);

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
    if (gtk_widget_is_drawable(GTK_WIDGET(arrow)))
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
  if (gtk_widget_is_drawable(widget)) {
    Point from, to;
    Point move_arrow, move_line, arrow_head;
    DiaCairoRenderer *renderer;
    DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
    Arrow arrow_type;
    GtkMisc *misc = GTK_MISC(widget);
    gint width, height;
    gint x, y;
    GdkWindow *win;
    int linewidth = 2;
    DiaRendererClass *renderer_ops;
    cairo_surface_t *surface;
    cairo_t *ctx;

    width = widget->allocation.width - misc->xpad * 2;
    height = widget->allocation.height - misc->ypad * 2;
    x = (widget->allocation.x + misc->xpad);
    y = (widget->allocation.y + misc->ypad);

    win = gtk_widget_get_window (widget);

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

    surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

    renderer = g_object_new (dia_cairo_renderer_get_type (), NULL);
    renderer->with_alpha = TRUE;
    renderer->surface = cairo_surface_reference (surface);

    renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
    renderer_ops->begin_render(DIA_RENDERER (renderer), NULL);
    renderer_ops->set_linewidth(DIA_RENDERER (renderer), linewidth);
    {
      Color color_bg, color_fg;
      GtkStyle *style = gtk_widget_get_style (widget);
      /* the text colors are the best approximation to what we had */
      GdkColor bg = style->base[gtk_widget_get_state(widget)];
      GdkColor fg = style->text[gtk_widget_get_state(widget)];

      GDK_COLOR_TO_DIA(bg, color_bg);
      GDK_COLOR_TO_DIA(fg, color_fg);
      renderer_ops->draw_line(DIA_RENDERER (renderer), &from, &to, &color_fg);
      arrow_draw (DIA_RENDERER (renderer), arrow_type.type,
                  &arrow_head, &from,
		  arrow_type.length,
		  arrow_type.width,
                  linewidth, &color_fg, &color_bg);
    }
    renderer_ops->end_render(DIA_RENDERER (renderer));
    g_object_unref(renderer);

    ctx = gdk_cairo_create (win);
    cairo_set_source_surface (ctx, surface, x, y);
    cairo_paint (ctx);
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
    GtkMenu *menu = g_object_get_data(G_OBJECT(widget), button_menu_key);
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
  gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(chooser->dialog))), wid,
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
 * @return A new DiaArrowChooser widget.
 */
GtkWidget *
dia_arrow_chooser_new(gboolean left, DiaChangeArrowCallback callback,
		      gpointer user_data)
{
  DiaArrowChooser *chooser = g_object_new(DIA_TYPE_ARROW_CHOOSER, NULL);
  GtkWidget *menu, *mi, *ar;
  gint i;

  chooser->left = left;
  dia_arrow_preview_set(chooser->preview, chooser->preview->atype, left);
  chooser->callback = callback;
  chooser->user_data = user_data;

  menu = gtk_menu_new();
  g_object_ref_sink(menu);
  g_object_set_data_full(G_OBJECT(chooser), button_menu_key, menu,
			 (GDestroyNotify)g_object_unref);
  /* although from ARROW_NONE to MAX_ARROW_TYPE-1 this is sorted by *index* to keep the order consistent with earlier releases */
  for (i = ARROW_NONE; i < MAX_ARROW_TYPE; ++i) {
    ArrowType arrow_type = arrow_type_from_index(i);
    mi = gtk_menu_item_new();
    g_object_set_data(G_OBJECT(mi), menuitem_enum_key,
		      GINT_TO_POINTER(arrow_type));
    gtk_widget_set_tooltip_text(mi, _dia_translate(arrow_get_name_from_type(arrow_type), NULL));
    ar = dia_arrow_preview_new(arrow_type, left);

    gtk_container_add(GTK_CONTAINER(mi), ar);
    gtk_widget_show(ar);
    g_signal_connect(G_OBJECT(mi), "activate",
		     G_CALLBACK(dia_arrow_chooser_change_arrow_type), chooser);
    gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
    gtk_widget_show(mi);
  }
  mi = gtk_menu_item_new_with_label(_dia_translate("Details\342\200\246", NULL));
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

