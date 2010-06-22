/* GTK - The GIMP Toolkit
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/*
 * Modified by the GTK+ Team and others 1997-2000.  See the AUTHORS
 * file for a list of people on the GTK+ Team.  See the ChangeLog
 * files for a list of changes.  These files are distributed with
 * GTK+ at ftp://ftp.gtk.org/pub/gtk/. 
 */

#include "diacanvas.h"
#include "intl.h"
#include <gtk/gtkversion.h>

enum {
  CHILD_PROP_0,
  CHILD_PROP_X,
  CHILD_PROP_Y
};

static void dia_canvas_class_init    (DiaCanvasClass    *klass);
static void dia_canvas_init          (DiaCanvas         *canvas);
static void dia_canvas_realize       (GtkWidget        *widget);
static void dia_canvas_size_request  (GtkWidget        *widget,
				     GtkRequisition   *requisition);
static void dia_canvas_size_allocate (GtkWidget        *widget,
				     GtkAllocation    *allocation);
static void dia_canvas_add           (GtkContainer     *container,
				     GtkWidget        *widget);
static void dia_canvas_remove        (GtkContainer     *container,
				     GtkWidget        *widget);
static void dia_canvas_forall        (GtkContainer     *container,
				     gboolean 	       include_internals,
				     GtkCallback       callback,
				     gpointer          callback_data);
static GType dia_canvas_child_type   (GtkContainer     *container);

static void dia_canvas_set_child_property (GtkContainer *container,
                                          GtkWidget    *child,
                                          guint         property_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);
static void dia_canvas_get_child_property (GtkContainer *container,
                                          GtkWidget    *child,
                                          guint         property_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void dia_canvas_send_configure (DiaCanvas     *darea);

static GtkContainerClass *parent_class = NULL;


GType
dia_canvas_get_type (void)
{
  static GType canvas_type = 0;

  if (!canvas_type)
    {
      static const GTypeInfo canvas_info =
      {
	sizeof (DiaCanvasClass),
	NULL,		/* base_init */
	NULL,		/* base_finalize */
	(GClassInitFunc) dia_canvas_class_init,
	NULL,		/* class_finalize */
	NULL,		/* class_data */
	sizeof (DiaCanvas),
	0,		/* n_preallocs */
	(GInstanceInitFunc) dia_canvas_init,
      };

      canvas_type = g_type_register_static (GTK_TYPE_CONTAINER, "DiaCanvas",
					   &canvas_info, 0);
    }

  return canvas_type;
}

static void
dia_canvas_class_init (DiaCanvasClass *class)
{
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  widget_class = (GtkWidgetClass*) class;
  container_class = (GtkContainerClass*) class;

  parent_class = g_type_class_peek_parent (class);

  widget_class->realize = dia_canvas_realize;
  widget_class->size_request = dia_canvas_size_request;
  widget_class->size_allocate = dia_canvas_size_allocate;

  container_class->add = dia_canvas_add;
  container_class->remove = dia_canvas_remove;
  container_class->forall = dia_canvas_forall;
  container_class->child_type = dia_canvas_child_type;

  container_class->set_child_property = dia_canvas_set_child_property;
  container_class->get_child_property = dia_canvas_get_child_property;

  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_X,
					      g_param_spec_int ("x",
                                                                _("X position"),
                                                                _("X position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                G_PARAM_READWRITE));

  gtk_container_class_install_child_property (container_class,
					      CHILD_PROP_Y,
					      g_param_spec_int ("y",
                                                                _("Y position"),
                                                                _("Y position of child widget"),
                                                                G_MININT,
                                                                G_MAXINT,
                                                                0,
                                                                G_PARAM_READWRITE));
}

static GType
dia_canvas_child_type (GtkContainer     *container)
{
  return GTK_TYPE_WIDGET;
}

static void
dia_canvas_init (DiaCanvas *canvas)
{
  canvas->children = NULL;
}

GtkWidget*
dia_canvas_new (void)
{
  return g_object_new (DIA_TYPE_CANVAS, NULL);
}

void
dia_canvas_set_size (DiaCanvas *canvas,
		     gint      width,
		     gint      height)
{
  g_return_if_fail (DIA_IS_CANVAS (canvas));

  canvas->wantedsize.width = width;
  canvas->wantedsize.height = height;

  gtk_widget_queue_resize (GTK_WIDGET (canvas));
}

static void
dia_canvas_send_configure (DiaCanvas *darea)
{
  GtkWidget *widget;
  GdkEvent *event = gdk_event_new (GDK_CONFIGURE);

  widget = GTK_WIDGET (darea);

  event->configure.window = g_object_ref (widget->window);
  event->configure.send_event = TRUE;
  event->configure.x = widget->allocation.x;
  event->configure.y = widget->allocation.y;
  event->configure.width = widget->allocation.width;
  event->configure.height = widget->allocation.height;
  
  gtk_widget_event (widget, event);
  gdk_event_free (event);
}

static DiaCanvasChild*
get_child (DiaCanvas  *canvas,
           GtkWidget *widget)
{
  GList *children;
  
  children = canvas->children;
  while (children)
    {
      DiaCanvasChild *child;
      
      child = children->data;
      children = children->next;

      if (child->widget == widget)
        return child;
    }

  return NULL;
}

void
dia_canvas_put (DiaCanvas       *canvas,
               GtkWidget      *widget,
               gint            x,
               gint            y)
{
  DiaCanvasChild *child_info;

  g_return_if_fail (DIA_IS_CANVAS (canvas));
  g_return_if_fail (GTK_IS_WIDGET (canvas));

  child_info = g_new (DiaCanvasChild, 1);
  child_info->widget = widget;
  child_info->x = x;
  child_info->y = y;

  gtk_widget_set_parent (widget, GTK_WIDGET (canvas));

  canvas->children = g_list_append (canvas->children, child_info);
}

static void
dia_canvas_move_internal (DiaCanvas       *canvas,
                         GtkWidget      *widget,
                         gboolean        change_x,
                         gint            x,
                         gboolean        change_y,
                         gint            y)
{
  DiaCanvasChild *child;
  
  g_return_if_fail (DIA_IS_CANVAS (canvas));
  g_return_if_fail (GTK_IS_WIDGET (widget));
  g_return_if_fail (widget->parent == GTK_WIDGET (canvas));  
  
  child = get_child (canvas, widget);

  g_assert (child);

  gtk_widget_freeze_child_notify (widget);
  
  if (change_x)
    {
      child->x = x;
      gtk_widget_child_notify (widget, "x");
    }

  if (change_y)
    {
      child->y = y;
      gtk_widget_child_notify (widget, "y");
    }

  gtk_widget_thaw_child_notify (widget);
  
#if GTK_CHECK_VERSION(2,20,0)
  if (gtk_widget_get_visible (widget) && gtk_widget_get_visible (canvas))
#else
  if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (canvas))
#endif
    gtk_widget_queue_resize (GTK_WIDGET (canvas));
}

void
dia_canvas_move (DiaCanvas       *canvas,
                GtkWidget      *widget,
                gint            x,
                gint            y)
{
  dia_canvas_move_internal (canvas, widget, TRUE, x, TRUE, y);
}

static void
dia_canvas_set_child_property (GtkContainer    *container,
                              GtkWidget       *child,
                              guint            property_id,
                              const GValue    *value,
                              GParamSpec      *pspec)
{
  switch (property_id)
    {
    case CHILD_PROP_X:
      dia_canvas_move_internal (DIA_CANVAS (container),
                               child,
                               TRUE, g_value_get_int (value),
                               FALSE, 0);
      break;
    case CHILD_PROP_Y:
      dia_canvas_move_internal (DIA_CANVAS (container),
                               child,
                               FALSE, 0,
                               TRUE, g_value_get_int (value));
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
dia_canvas_get_child_property (GtkContainer *container,
                              GtkWidget    *child,
                              guint         property_id,
                              GValue       *value,
                              GParamSpec   *pspec)
{
  DiaCanvasChild *canvas_child;

  canvas_child = get_child (DIA_CANVAS (container), child);
  
  switch (property_id)
    {
    case CHILD_PROP_X:
      g_value_set_int (value, canvas_child->x);
      break;
    case CHILD_PROP_Y:
      g_value_set_int (value, canvas_child->y);
      break;
    default:
      GTK_CONTAINER_WARN_INVALID_CHILD_PROPERTY_ID (container, property_id, pspec);
      break;
    }
}

static void
dia_canvas_realize (GtkWidget *widget)
{
  GdkWindowAttr attributes;
  gint attributes_mask;

  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  attributes.window_type = GDK_WINDOW_CHILD;
  attributes.x = widget->allocation.x;
  attributes.y = widget->allocation.y;
  attributes.width = widget->allocation.width;
  attributes.height = widget->allocation.height;
  attributes.wclass = GDK_INPUT_OUTPUT;
  attributes.visual = gtk_widget_get_visual (widget);
  attributes.colormap = gtk_widget_get_colormap (widget);
  attributes.event_mask = gtk_widget_get_events (widget);
  attributes.event_mask |= GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK;
      
  attributes_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;
      
  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget), &attributes, 
				   attributes_mask);
  gdk_window_set_user_data (widget->window, widget);
      
  widget->style = gtk_style_attach (widget->style, widget->window);
  gtk_style_set_background (widget->style, widget->window, GTK_STATE_NORMAL);

  dia_canvas_send_configure (DIA_CANVAS (widget));
}

static void
dia_canvas_size_request (GtkWidget      *widget,
			GtkRequisition *requisition)
{
  DiaCanvas *canvas;  
  DiaCanvasChild *child;
  GList *children;
  GtkRequisition child_requisition;

  canvas = DIA_CANVAS (widget);

  requisition->width = canvas->wantedsize.width;
  requisition->height = canvas->wantedsize.height;


  children = canvas->children;
  while (children)
    {
      child = children->data;
      children = children->next;

#if GTK_CHECK_VERSION(2,20,0)
      if (gtk_widget_get_visible (child->widget))
#else
      if (GTK_WIDGET_VISIBLE (child->widget))
#endif
	{
          gtk_widget_size_request (child->widget, &child_requisition);

          requisition->height = MAX (requisition->height,
                                     child->y +
                                     child_requisition.height);
          requisition->width = MAX (requisition->width,
                                    child->x +
                                    child_requisition.width);
	}
    }
  /*
  requisition->height += GTK_CONTAINER (canvas)->border_width * 2;
  requisition->width += GTK_CONTAINER (canvas)->border_width * 2;
  */
}

static void
dia_canvas_size_allocate (GtkWidget     *widget,
			 GtkAllocation *allocation)
{
  DiaCanvas *canvas;
  DiaCanvasChild *child;
  GtkAllocation child_allocation;
  GtkRequisition child_requisition;
  GList *children;
  guint16 border_width;

  canvas = DIA_CANVAS (widget);

  widget->allocation = *allocation;

#if GTK_CHECK_VERSION(2,20,0)
  if (gtk_widget_get_realized (widget)) {
#else
  if (GTK_WIDGET_REALIZED (widget)) {
#endif
    gdk_window_move_resize (widget->window,
			    allocation->x, 
			    allocation->y,
			    allocation->width, 
			    allocation->height);
    dia_canvas_send_configure (DIA_CANVAS (widget));
  }
  border_width = GTK_CONTAINER (canvas)->border_width;
  
  children = canvas->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      
#if GTK_CHECK_VERSION(2,20,0)
      if (gtk_widget_get_visible (child->widget))
#else
      if (GTK_WIDGET_VISIBLE (child->widget))
#endif
	{
	  gtk_widget_get_child_requisition (child->widget, &child_requisition);
	  child_allocation.x = child->x + border_width;
	  child_allocation.y = child->y + border_width;

#if GTK_CHECK_VERSION(2,20,0)
	  if (gtk_widget_get_has_window (widget))
#else
	  if (GTK_WIDGET_NO_WINDOW (widget))
#endif
	    {
	      child_allocation.x += widget->allocation.x;
	      child_allocation.y += widget->allocation.y;
	    }
	  
	  child_allocation.width = child_requisition.width;
	  child_allocation.height = child_requisition.height;
	  gtk_widget_size_allocate (child->widget, &child_allocation);
	}
    }
}

static void
dia_canvas_add (GtkContainer *container,
	       GtkWidget    *widget)
{
  dia_canvas_put (DIA_CANVAS (container), widget, 0, 0);
}

static void
dia_canvas_remove (GtkContainer *container,
		  GtkWidget    *widget)
{
  DiaCanvas *canvas;
  DiaCanvasChild *child;
  GList *children;

  canvas = DIA_CANVAS (container);

  children = canvas->children;
  while (children)
    {
      child = children->data;

      if (child->widget == widget)
	{
#if GTK_CHECK_VERSION(2,20,0)
	  gboolean was_visible = gtk_widget_get_visible (widget);
#else
	  gboolean was_visible = GTK_WIDGET_VISIBLE (widget);
#endif
	  
	  gtk_widget_unparent (widget);

	  canvas->children = g_list_remove_link (canvas->children, children);
	  g_list_free (children);
	  g_free (child);

#if GTK_CHECK_VERSION(2,20,0)
	  if (was_visible && gtk_widget_get_visible (container))
#else
	  if (was_visible && GTK_WIDGET_VISIBLE (container))
#endif
	    gtk_widget_queue_resize (GTK_WIDGET (container));

	  break;
	}

      children = children->next;
    }
}

static void
dia_canvas_forall (GtkContainer *container,
		  gboolean	include_internals,
		  GtkCallback   callback,
		  gpointer      callback_data)
{
  DiaCanvas *canvas;
  DiaCanvasChild *child;
  GList *children;

  g_return_if_fail (callback != NULL);

  canvas = DIA_CANVAS (container);

  children = canvas->children;
  while (children)
    {
      child = children->data;
      children = children->next;

      (* callback) (child->widget, callback_data);
    }
}

