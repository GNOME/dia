/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacellrendererproperty.c - a cell renderer for Dia's StdProps system, to be used in GtkTreeView
 * Copyright (C) 2008 Hans Breuer
 *
 * based on :
 * diacellrendererproperty.c
 * Copyright (C) 2003 Michael Natterer <mitch@dia.org>
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

#include <config.h>

#include <gtk/gtk.h>

#include "diacellrendererproperty.h"

#include "diarenderer.h"
#include "diamarshal.h"
#include "message.h"

enum
{
  CLICKED,
  LAST_SIGNAL
};

enum
{
  PROP_0,
  PROP_RENDERER,
  PROP_PROPERTY
};

static void dia_cell_renderer_property_finalize     (GObject         *object);
static void dia_cell_renderer_property_get_property (GObject         *object,
                                                     guint            param_id,
                                                     GValue          *value,
                                                     GParamSpec      *pspec);
static void dia_cell_renderer_property_set_property (GObject         *object,
                                                     guint            param_id,
                                                     const GValue    *value,
                                                     GParamSpec      *pspec);
static void dia_cell_renderer_property_get_size     (GtkCellRenderer *cell,
                                                     GtkWidget       *widget,
                                                     GdkRectangle    *rectangle,
                                                     gint            *x_offset,
                                                     gint            *y_offset,
                                                     gint            *width,
                                                     gint            *height);
static void dia_cell_renderer_property_render       (GtkCellRenderer *cell,
                                                     GdkWindow       *window,
                                                     GtkWidget       *widget,
                                                     GdkRectangle    *background_area,
                                                     GdkRectangle    *cell_area,
                                                     GdkRectangle    *expose_area,
                                                     GtkCellRendererState flags);
static gboolean dia_cell_renderer_property_activate (GtkCellRenderer *cell,
                                                     GdkEvent        *event,
                                                     GtkWidget       *widget,
                                                     const gchar     *path,
                                                     GdkRectangle    *background_area,
                                                     GdkRectangle    *cell_area,
                                                     GtkCellRendererState flags);


G_DEFINE_TYPE (DiaCellRendererProperty, dia_cell_renderer_property, GTK_TYPE_CELL_RENDERER)

#define parent_class dia_cell_renderer_property_parent_class

static guint property_cell_signals[LAST_SIGNAL] = { 0 };


static void
dia_cell_renderer_property_class_init (DiaCellRendererPropertyClass *klass)
{
  GObjectClass         *object_class = G_OBJECT_CLASS (klass);
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS (klass);

  property_cell_signals[CLICKED] =
    g_signal_new ("clicked",
                  G_OBJECT_CLASS_TYPE (object_class),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (DiaCellRendererPropertyClass, clicked),
                  NULL, NULL,
                  dia_marshal_VOID__STRING_FLAGS,
                  G_TYPE_NONE, 2,
                  G_TYPE_STRING,
                  GDK_TYPE_MODIFIER_TYPE);

  object_class->finalize     = dia_cell_renderer_property_finalize;
  object_class->get_property = dia_cell_renderer_property_get_property;
  object_class->set_property = dia_cell_renderer_property_set_property;

  cell_class->get_size       = dia_cell_renderer_property_get_size;
  cell_class->render         = dia_cell_renderer_property_render;
  cell_class->activate       = dia_cell_renderer_property_activate;

  klass->clicked             = NULL;

  g_object_class_install_property (object_class, PROP_RENDERER,
                                   g_param_spec_object ("renderer",
                                                        NULL, NULL,
                                                        DIA_TYPE_RENDERER,
                                                        G_PARAM_READWRITE));
  g_object_class_install_property (object_class, PROP_PROPERTY,
                                   g_param_spec_object ("property",
                                                        NULL, NULL,
                                                        DIA_TYPE_RENDERER,
                                                        G_PARAM_READWRITE));
}

static void
dia_cell_renderer_property_init (DiaCellRendererProperty *cellproperty)
{
  GTK_CELL_RENDERER (cellproperty)->mode = GTK_CELL_RENDERER_MODE_ACTIVATABLE;
}

static void
dia_cell_renderer_property_finalize (GObject *object)
{
  DiaCellRendererProperty *cell = DIA_CELL_RENDERER_PROPERTY (object);

  if (cell->renderer)
    {
      g_object_unref (cell->renderer);
      cell->renderer = NULL;
    }

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
dia_cell_renderer_property_get_property (GObject    *object,
                                          guint       param_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  DiaCellRendererProperty *cell = DIA_CELL_RENDERER_PROPERTY (object);

  switch (param_id)
    {
    case PROP_RENDERER:
      g_value_set_object (value, cell->renderer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
dia_cell_renderer_property_set_property (GObject      *object,
                                          guint         param_id,
                                          const GValue *value,
                                          GParamSpec   *pspec)
{
  DiaCellRendererProperty *cell = DIA_CELL_RENDERER_PROPERTY (object);

  switch (param_id)
    {
    case PROP_RENDERER:
      {
        DiaRenderer *renderer;

        renderer = (DiaRenderer *) g_value_dup_object (value);
        if (cell->renderer)
          g_object_unref (cell->renderer);
        cell->renderer = renderer;
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
      break;
    }
}

static void
dia_cell_renderer_property_get_size (GtkCellRenderer *cell,
                                      GtkWidget       *widget,
                                      GdkRectangle    *cell_area,
                                      gint            *x_offset,
                                      gint            *y_offset,
                                      gint            *width,
                                      gint            *height)
{
  G_GNUC_UNUSED DiaCellRendererProperty *cellproperty;
  gint                      view_width  = 0;
  gint                      view_height = 0;
  gint                      calc_width;
  gint                      calc_height;

#if 0
  cellproperty = DIA_CELL_RENDERER_PROPERTY (cell);

  if (cellproperty->renderer)
    {
      view_width  = (cellproperty->renderer->width  +
                     2 * cellproperty->renderer->border_width);
      view_height = (cellproperty->renderer->height +
                     2 * cellproperty->renderer->border_width);
    }
#else
  /* not sure how to translate, most props could have a fixed size for drawing */
  view_width = 120;
  view_height = 30;
#endif

  calc_width  = (gint) cell->xpad * 2 + view_width;
  calc_height = (gint) cell->ypad * 2 + view_height;

  if (x_offset) *x_offset = 0;
  if (y_offset) *y_offset = 0;

  if (cell_area && view_width > 0 && view_height > 0)
    {
      if (x_offset)
        {
          *x_offset = (((gtk_widget_get_direction (widget) == GTK_TEXT_DIR_RTL) ?
                        1.0 - cell->xalign : cell->xalign) *
                       (cell_area->width - calc_width - 2 * cell->xpad));
          *x_offset = (MAX (*x_offset, 0) + cell->xpad);
        }
      if (y_offset)
        {
          *y_offset = (cell->yalign *
                       (cell_area->height - calc_height - 2 * cell->ypad));
          *y_offset = (MAX (*y_offset, 0) + cell->ypad);
        }
    }

  if (width)  *width  = calc_width;
  if (height) *height = calc_height;
}

static void
dia_cell_renderer_property_render (GtkCellRenderer      *cell,
                                    GdkWindow            *window,
                                    GtkWidget            *widget,
                                    GdkRectangle         *background_area,
                                    GdkRectangle         *cell_area,
                                    GdkRectangle         *expose_area,
                                    GtkCellRendererState  flags)
{
  DiaCellRendererProperty *cellproperty;

  cellproperty = DIA_CELL_RENDERER_PROPERTY (cell);

  if (cellproperty->renderer)
    {
      if (! (flags & GTK_CELL_RENDERER_SELECTED))
        {
#if 0
          dia_view_renderer_remove_idle (cellproperty->renderer);
#endif
        }
#if 0
      dia_view_renderer_draw (cellproperty->renderer, window, widget,
                               cell_area, expose_area);
#endif
    }
}

static gboolean
dia_cell_renderer_property_activate (GtkCellRenderer      *cell,
                                      GdkEvent             *event,
                                      GtkWidget            *widget,
                                      const gchar          *path,
                                      GdkRectangle         *background_area,
                                      GdkRectangle         *cell_area,
                                      GtkCellRendererState  flags)
{
  DiaCellRendererProperty *cellproperty;

  cellproperty = DIA_CELL_RENDERER_PROPERTY (cell);

  if (cellproperty->renderer)
    {
      GdkModifierType state = 0;

      if (event && ((GdkEventAny *) event)->type == GDK_BUTTON_PRESS)
        state = ((GdkEventButton *) event)->state;

      if (! event ||
          (((GdkEventAny *) event)->type == GDK_BUTTON_PRESS &&
           ((GdkEventButton *) event)->button == 1))
        {
          dia_cell_renderer_property_clicked (cellproperty, path, state);

          return TRUE;
        }
    }

  return FALSE;
}

GtkCellRenderer *
dia_cell_renderer_property_new (void)
{
  return g_object_new (DIA_TYPE_CELL_RENDERER_PROPERTY, NULL);
}

void
dia_cell_renderer_property_clicked (DiaCellRendererProperty *cell,
                                     const gchar              *path,
                                     GdkModifierType           state)
{
  GdkEvent *event;

  g_return_if_fail (DIA_IS_CELL_RENDERER_PROPERTY (cell));
  g_return_if_fail (path != NULL);

  g_signal_emit (cell, property_cell_signals[CLICKED], 0, path, state);

  event = gtk_get_current_event ();

  if (event)
    {
      GdkEventButton *bevent = (GdkEventButton *) event;

      if (bevent->type == GDK_BUTTON_PRESS &&
          (bevent->button == 1 || bevent->button == 2))
        {
#if 0
          dia_view_popup_show (gtk_get_event_widget (event),
                                bevent,
                                cell->renderer->context,
                                cell->renderer->property,
                                cell->renderer->width,
                                cell->renderer->height,
                                cell->renderer->dot_for_dot);
#else
          /* either edit the property itself _or_ if it is a list open another property list */
          message_warning ("Clicked!");
#endif
        }

      gdk_event_free (event);
    }
}
