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

#ifndef __DIA_CANVAS_H__
#define __DIA_CANVAS_H__


#include <gdk/gdk.h>
#include <gtk/gtkcontainer.h>


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */


#define DIA_TYPE_CANVAS                  (dia_canvas_get_type ())
#define DIA_CANVAS(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_CANVAS, DiaCanvas))
#define DIA_CANVAS_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_CANVAS, DiaCanvasClass))
#define DIA_IS_CANVAS(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_CANVAS))
#define DIA_IS_CANVAS_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_CANVAS))
#define DIA_CANVAS_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_CANVAS, DiaCanvasClass))


typedef struct _DiaCanvas        DiaCanvas;
typedef struct _DiaCanvasClass   DiaCanvasClass;
typedef struct _DiaCanvasChild   DiaCanvasChild;

struct _DiaCanvas
{
  GtkContainer container;

  GtkRequisition wantedsize;
  GList *children;
};

struct _DiaCanvasClass
{
  GtkContainerClass parent_class;
};

struct _DiaCanvasChild
{
  GtkWidget *widget;
  gint x;
  gint y;
};


GType      dia_canvas_get_type          (void) G_GNUC_CONST;
GtkWidget* dia_canvas_new               (void);
void       dia_canvas_put               (DiaCanvas     *canvas,
                                        GtkWidget      *widget,
                                        gint            x,
                                        gint            y);
void       dia_canvas_move              (DiaCanvas     *canvas,
                                        GtkWidget      *widget,
                                        gint            x,
                                        gint            y);
void       dia_canvas_set_size          (DiaCanvas      *canvas,
					gint           width,
					gint           height);

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __DIA_CANVAS_H__ */
