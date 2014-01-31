/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * diacellrendererproperty.h
 * Copyright (C) 2008 Hans Breuer <hans@breuer.org>
 *
 * gimpcellrendererviewable.h
 * Copyright (C) 2003 Michael Natterer <mitch@gimp.org>
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

#ifndef __DIA_CELL_RENDERER_PROPERTY_H__
#define __DIA_CELL_RENDERER_PROPERTY_H__


#include <gtk/gtk.h>
#include "diatypes.h"

#define DIA_TYPE_CELL_RENDERER_PROPERTY            (dia_cell_renderer_property_get_type ())
#define DIA_CELL_RENDERER_PROPERTY(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), DIA_TYPE_CELL_RENDERER_PROPERTY, DiaCellRendererProperty))
#define DIA_CELL_RENDERER_PROPERTY_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), DIA_TYPE_CELL_RENDERER_PROPERTY, DiaCellRendererPropertyClass))
#define DIA_IS_CELL_RENDERER_PROPERTY(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_CELL_RENDERER_PROPERTY))
#define DIA_IS_CELL_RENDERER_PROPERTY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), DIA_TYPE_CELL_RENDERER_PROPERTY))
#define DIA_CELL_RENDERER_PROPERTY_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), DIA_TYPE_CELL_RENDERER_PROPERTY, DiaCellRendererPropertyClass))


typedef struct _DiaCellRendererPropertyClass DiaCellRendererPropertyClass;

struct _DiaCellRendererProperty
{
  GtkCellRenderer   parent_instance;
  
  /*< private >*/
  DiaRenderer      *renderer;
};

struct _DiaCellRendererPropertyClass
{
  GtkCellRendererClass  parent_class;

  void (* clicked) (DiaCellRendererProperty *cell,
                    const gchar              *path,
                    GdkModifierType           state);
};


GType             dia_cell_renderer_property_get_type (void) G_GNUC_CONST;

GtkCellRenderer * dia_cell_renderer_property_new      (void);

void   dia_cell_renderer_property_clicked (DiaCellRendererProperty *cell,
                                            const gchar              *path,
                                            GdkModifierType           state);

#endif /* __DIA_CELL_RENDERER_PROPERTY_H__ */
