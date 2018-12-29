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
#include "dia-arrow-chooser.h"
#include "renderer/diacairo.h"
#include "dia_dirs.h"

/* --------------- DiaArrowPreview -------------------------------- */
static void dia_arrow_preview_set(DiaArrowPreview *arrow, 
                                  ArrowType atype, gboolean left);
static gint dia_arrow_preview_draw       (GtkWidget             *widget,
                                          cairo_t               *ctx);

G_DEFINE_TYPE (DiaArrowPreview, dia_arrow_preview, GTK_TYPE_WIDGET)

/** Initialize class information for the arrow preview class.
 * @param class The class object to initialize/
 */
static void
dia_arrow_preview_class_init(DiaArrowPreviewClass *class)
{
  GtkWidgetClass *widget_class;

  widget_class = GTK_WIDGET_CLASS (class);
  widget_class->draw = dia_arrow_preview_draw;
}

/** Initialize an arrow preview widget.
 * @param arrow The widget to initialize.
 */
static void
dia_arrow_preview_init (DiaArrowPreview *self)
{
  gtk_widget_set_has_window (GTK_WIDGET (self), FALSE);
  gtk_widget_set_size_request (GTK_WIDGET (self), -1, 24);

  self->atype = ARROW_NONE;
  self->left = TRUE;
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
static gboolean
dia_arrow_preview_draw (GtkWidget *widget, cairo_t *ctx)
{
  Point from, to;
  Point move_arrow, move_line, arrow_head;
  DiaCairoRenderer *renderer;
  DiaArrowPreview *arrow = DIA_ARROW_PREVIEW(widget);
  Arrow arrow_type;
  gint width, height;
  GtkAllocation alloc;
  int linewidth = 2;
  DiaRendererClass *renderer_ops;
  GtkStyleContext *context = gtk_widget_get_style_context (widget);

  gtk_widget_get_allocation (widget, &alloc);

  width = alloc.width;
  height = alloc.height;

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

  renderer = g_object_new (DIA_TYPE_CAIRO_RENDERER, NULL);
  renderer->with_alpha = TRUE;
  renderer->surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);

  renderer_ops = DIA_RENDERER_GET_CLASS (renderer);
  renderer_ops->begin_render(DIA_RENDERER (renderer), NULL);
  renderer_ops->set_linewidth(DIA_RENDERER (renderer), linewidth);
  {
    GdkRGBA fg;
    GdkRGBA bg = { 0, 0, 0, 0 };
    /* the text colors are the best approximation to what we had */
    gtk_style_context_get_color (context, gtk_widget_get_state_flags (widget), &fg);


    renderer_ops->draw_line(DIA_RENDERER (renderer), &from, &to, &fg);
    arrow_draw (DIA_RENDERER (renderer), arrow_type.type, 
                &arrow_head, &from, 
                arrow_type.length, 
                arrow_type.width,
                linewidth, &fg, &bg);
  }
  renderer_ops->end_render(DIA_RENDERER (renderer));

  cairo_set_source_surface (ctx, renderer->surface, 0, 0);
  cairo_paint (ctx);

  g_object_unref(renderer);

  return FALSE;
}

typedef struct _DiaArrowChooserPopoverPrivate DiaArrowChooserPopoverPrivate;

struct _DiaArrowChooserPopoverPrivate {
  GtkWidget *list;
  GtkWidget *size;
  GtkWidget *size_box;

  Arrow arrow;
  gboolean left;
};

G_DEFINE_TYPE_WITH_CODE (DiaArrowChooserPopover, dia_arrow_chooser_popover, GTK_TYPE_POPOVER,
                         G_ADD_PRIVATE (DiaArrowChooserPopover))

enum {
  DAC_POP_VALUE_CHANGED,
  DAC_POP_LAST_SIGNAL
};

static guint dac_pop_signals[DAC_POP_LAST_SIGNAL] = { 0 };

enum {
  DAC_POP_PROP_LEFT = 1,
  DAC_POP_N_PROPS
};
static GParamSpec* dac_pop_properties[DAC_POP_N_PROPS];

GtkWidget *
dia_arrow_chooser_popover_new (gboolean left)
{
  return g_object_new (DIA_TYPE_ARROW_CHOOSER_POPOVER,
                       "left", left,
                       NULL);
}

static void
dia_arrow_chooser_popover_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
  DiaArrowChooserPopover *self = DIA_ARROW_CHOOSER_POPOVER (object);
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);
  switch (property_id) {
    case DAC_POP_PROP_LEFT:
      priv->left = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
dia_arrow_chooser_popover_class_init (DiaArrowChooserPopoverClass *klass)
{
  GFile *template_file;
  GBytes *template;
  GError *err = NULL;
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->set_property = dia_arrow_chooser_popover_set_property;

  dac_pop_properties[DAC_POP_PROP_LEFT] =
    g_param_spec_boolean ("left",
                          "Is left",
                          "Does the arrow point left",
                          TRUE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE);

  g_object_class_install_properties (object_class,
                                     DAC_POP_N_PROPS,
                                     dac_pop_properties);

  dac_pop_signals[DAC_POP_VALUE_CHANGED] = g_signal_new ("value-changed",
                                                         G_TYPE_FROM_CLASS (klass),
                                                         G_SIGNAL_RUN_FIRST,
                                                         0, NULL, NULL, NULL,
                                                         G_TYPE_NONE, 0);

  /* TODO: Use GResource */
  template_file = g_file_new_for_path (build_ui_filename ("ui/dia-arrow-chooser-popover.ui"));
  template = g_file_load_bytes (template_file, NULL, NULL, &err);

  if (err)
    g_critical ("Failed to load template: %s", err->message);

  gtk_widget_class_set_template (widget_class, template);
  gtk_widget_class_bind_template_child_private (widget_class, DiaArrowChooserPopover, list);
  gtk_widget_class_bind_template_child_private (widget_class, DiaArrowChooserPopover, size);
  gtk_widget_class_bind_template_child_private (widget_class, DiaArrowChooserPopover, size_box);

  g_object_unref (template_file);
}

static void
row_selected (GtkListBox             *box,
              GtkListBoxRow          *row,
              DiaArrowChooserPopover *self)
{
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);
  ArrowType atype = arrow_type_from_index (gtk_list_box_row_get_index (row));

  if (priv->arrow.type != atype) {
    priv->arrow.type = atype;
    dia_arrow_chooser_popover_set_arrow (self, &priv->arrow);
  }

  gtk_widget_set_sensitive (GTK_WIDGET (priv->size_box),
                            g_ascii_strcasecmp (arrow_get_name_from_type (atype), "None") != 0);
}

static void
size_changed (DiaSizeSelector        *selector,
              DiaArrowChooserPopover *self)
{
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);

  dia_size_selector_get_size (DIA_SIZE_SELECTOR (priv->size), &priv->arrow.width, &priv->arrow.length);

  dia_arrow_chooser_popover_set_arrow (self, &priv->arrow);
}

static void
dia_arrow_chooser_popover_init (DiaArrowChooserPopover *self)
{
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);

  priv->arrow.type = ARROW_NONE;
  priv->arrow.length = DEFAULT_ARROW_LENGTH;
  priv->arrow.width = DEFAULT_ARROW_WIDTH;

  gtk_widget_init_template (GTK_WIDGET (self));

  /* although from ARROW_NONE to MAX_ARROW_TYPE-1 this is sorted by *index* to keep the order consistent with earlier releases */
  for (int i = ARROW_NONE; i < MAX_ARROW_TYPE; ++i) {
    ArrowType arrow_type = arrow_type_from_index (i);
    GtkWidget *ar = dia_arrow_preview_new (arrow_type, priv->left);
    /* TODO: Don't gettext here */
    gtk_widget_set_tooltip_text (ar, gettext (arrow_get_name_from_type (arrow_type)));
    gtk_widget_show (ar);
    gtk_list_box_insert (GTK_LIST_BOX (priv->list), ar, i);
  }

  g_signal_connect (G_OBJECT (priv->list), "row-selected",
                    G_CALLBACK (row_selected), self);

  g_signal_connect (G_OBJECT (priv->size), "value-changed",
                    G_CALLBACK (size_changed), self);
}

Arrow
dia_arrow_chooser_popover_get_arrow (DiaArrowChooserPopover *self)
{
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);

  return priv->arrow;
}

void
dia_arrow_chooser_popover_set_arrow (DiaArrowChooserPopover *self,
                                     Arrow                  *arrow)
{
  DiaArrowChooserPopoverPrivate *priv = dia_arrow_chooser_popover_get_instance_private (self);

  priv->arrow.type = arrow->type;
  priv->arrow.width = arrow->width;
  priv->arrow.length = arrow->length;

  dia_size_selector_set_size (DIA_SIZE_SELECTOR (priv->size), arrow->width, arrow->length);
  gtk_list_box_select_row (GTK_LIST_BOX (priv->list),
                           gtk_list_box_get_row_at_index (GTK_LIST_BOX (priv->list),
                                                          arrow_index_from_type (arrow->type)));

  g_signal_emit (G_OBJECT (self),
                 dac_pop_signals[DAC_POP_VALUE_CHANGED], 0);
}

/* ------- Code for DiaArrowChooser ----------------------- */

typedef struct _DiaArrowChooserPrivate DiaArrowChooserPrivate;

struct _DiaArrowChooserPrivate {
  gboolean left;

  GtkWidget *preview;
  GtkWidget *popover;
};

G_DEFINE_TYPE_WITH_CODE (DiaArrowChooser, dia_arrow_chooser, GTK_TYPE_MENU_BUTTON,
                         G_ADD_PRIVATE (DiaArrowChooser))

enum {
    DAC_VALUE_CHANGED,
    DAC_LAST_SIGNAL
};

static guint dac_signals[DAC_LAST_SIGNAL] = { 0 };

enum {
  DAC_PROP_LEFT = 1,
  DAC_N_PROPS
};
static GParamSpec* dac_properties[DAC_N_PROPS];

static void
dia_arrow_chooser_set_property (GObject      *object,
                                guint         property_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  DiaArrowChooser *self = DIA_ARROW_CHOOSER (object);
  DiaArrowChooserPrivate *priv = dia_arrow_chooser_get_instance_private (self);
  switch (property_id) {
    case DAC_PROP_LEFT:
      priv->left = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

/** Initialize class information for the arrow choose.
 * @param class Class structure to initialize private fields of.
 */
static void
dia_arrow_chooser_class_init (DiaArrowChooserClass *class)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (class);

  object_class->set_property = dia_arrow_chooser_set_property;

  dac_properties[DAC_PROP_LEFT] =
    g_param_spec_boolean ("left",
                          "Is left",
                          "Does the arrow point left",
                          TRUE,
                          G_PARAM_CONSTRUCT_ONLY | G_PARAM_WRITABLE);

  g_object_class_install_properties (object_class,
                                     DAC_N_PROPS,
                                     dac_properties);

  dac_signals[DAC_VALUE_CHANGED] = g_signal_new ("value-changed",
                                                 G_TYPE_FROM_CLASS (class),
                                                 G_SIGNAL_RUN_FIRST,
                                                 0, NULL, NULL, NULL,
                                                 G_TYPE_NONE, 0);

  /* Just in case it's not registered when using GtkBuilder */
  g_type_ensure (dia_size_selector_get_type ());
}

static void
value_changed (DiaArrowChooserPopover *popover,
               DiaArrowChooser        *chooser)
{
  DiaArrowChooserPrivate *priv = dia_arrow_chooser_get_instance_private (chooser);
  Arrow arrow = dia_arrow_chooser_popover_get_arrow (popover);

  dia_arrow_preview_set (DIA_ARROW_PREVIEW (priv->preview), arrow.type, priv->left);

  g_signal_emit (G_OBJECT (chooser), dac_signals[DAC_VALUE_CHANGED], 0);
}

/** Initialize an arrow choose object.
 * @param arrow Newly allocated arrow choose object.
 */
static void
dia_arrow_chooser_init (DiaArrowChooser *self)
{
  DiaArrowChooserPrivate *priv = dia_arrow_chooser_get_instance_private (self);
  GtkWidget *wid;

  wid = dia_arrow_preview_new (ARROW_NONE, priv->left);
  gtk_container_add (GTK_CONTAINER (self), wid);
  gtk_widget_show (wid);
  priv->preview = wid;

  priv->popover = dia_arrow_chooser_popover_new (priv->left);
  g_signal_connect (G_OBJECT (priv->popover), "value-changed",
                    G_CALLBACK (value_changed), self);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (self), priv->popover);
}

/** Create a new arrow chooser object.
 * @param left If TRUE, this chooser will point its arrowheads to the left.
 * @return A new DiaArrowChooser widget.
 */
GtkWidget *
dia_arrow_chooser_new (gboolean left)
{
  return g_object_new (DIA_TYPE_ARROW_CHOOSER,
                       "left", left,
                       NULL);
}

/** Set the type of arrow shown by the arrow chooser.  If the arrow type
 * changes, the callback function will be called.
 * @param chooser The chooser to update.
 * @param arrow The arrow type and dimensions the chooser will dispaly.
 * Should it be called as well when the dimensions change?
 */
void
dia_arrow_chooser_set_arrow (DiaArrowChooser *self, Arrow *arrow)
{
  DiaArrowChooserPrivate *priv = dia_arrow_chooser_get_instance_private (self);

  dia_arrow_chooser_popover_set_arrow (DIA_ARROW_CHOOSER_POPOVER (priv->popover),
                                       arrow);
}

/** Get the currently selected arrow type from an arrow chooser.
 * @param arrow An arrow chooser to query.
 * @return The arrow type that is currently selected in the chooser.
 */
ArrowType
dia_arrow_chooser_get_arrow_type (DiaArrowChooser *self)
{
  return dia_arrow_chooser_get_arrow(self).type;
}

Arrow
dia_arrow_chooser_get_arrow (DiaArrowChooser *self)
{
  DiaArrowChooserPrivate *priv = dia_arrow_chooser_get_instance_private (self);

  return dia_arrow_chooser_popover_get_arrow (DIA_ARROW_CHOOSER_POPOVER (priv->popover));
}
