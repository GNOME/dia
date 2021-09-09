/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
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

#pragma once

#include <gtk/gtk.h>

#include "dia-layer.h"
#include "dia-layer-editor.h"

G_BEGIN_DECLS


#define DIA_TYPE_LAYER_WIDGET dia_layer_widget_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaLayerWidget, dia_layer_widget, DIA, LAYER_WIDGET, GtkBin)

struct _DiaLayerWidgetClass {
  GtkBinClass parent_class;
};

GtkWidget      *dia_layer_widget_new             (DiaLayer       *layer);
void            dia_layer_widget_set_layer       (DiaLayerWidget *self,
                                                  DiaLayer       *layer);
DiaLayer       *dia_layer_widget_get_layer       (DiaLayerWidget *self);
void            dia_layer_widget_set_editor      (DiaLayerWidget *self,
                                                  DiaLayerEditor *editor);
DiaLayerEditor *dia_layer_widget_get_editor      (DiaLayerWidget *self);
void            dia_layer_widget_set_connectable (DiaLayerWidget *self,
                                                  gboolean        on);
gboolean        dia_layer_widget_get_connectable (DiaLayerWidget *self);
void            dia_layer_widget_select          (DiaLayerWidget *self);
void            dia_layer_widget_deselect        (DiaLayerWidget *self);

G_END_DECLS
