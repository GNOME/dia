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
#ifndef LAYER_DIALOG_H
#define LAYER_DIALOG_H

#include <gtk/gtk.h>
#include "diagram.h"

struct LayerDialog {
  GtkWidget *dialog;
  GtkWidget *diagram_omenu;

  GtkWidget *layer_list;

  Diagram *diagram;

  GtkWidget *buttons[4];
};

void create_layer_dialog(void);
void layer_dialog_update_diagram_list(void);
void layer_dialog_show(void);
void layer_dialog_set_diagram(Diagram *dia);


/* DiaLayerWidget: */
#define DIA_LAYER_WIDGET(obj)          \
  GTK_CHECK_CAST (obj, dia_layer_widget_get_type (), DiaLayerWidget)
#define DIA_LAYER_WIDGET_CLASS(klass)  \
  GTK_CHECK_CLASS_CAST (klass, dia_layer_widget_get_type (), DiaLayerWidgetClass)
#define IS_DIA_LAYER_WIDGET(obj)       \
  GTK_CHECK_TYPE (obj, dia_layer_widget_get_type ())

typedef struct _DiaLayerWidget      DiaLayerWidget;
typedef struct _DiaLayerWidgetClass  DiaLayerWidgetClass;
typedef struct _EditLayerDialog EditLayerDialog;

struct _DiaLayerWidget
{
  GtkListItem list_item;

  Diagram *dia;
  Layer *layer;
  
  GtkWidget *visible;
  GtkWidget *selectable;
  GtkWidget *label;

  EditLayerDialog *edit_dialog;
};

struct _EditLayerDialog {
  GtkWidget *dialog;
  GtkWidget *name_entry;
  DiaLayerWidget *layer_widget;
};


struct _DiaLayerWidgetClass
{
  GtkListItemClass parent_class;
};

GtkType    dia_layer_widget_get_type(void);
GtkWidget* dia_layer_widget_new(Diagram *dia, Layer *layer);
void dia_layer_set_layer(DiaLayerWidget *widget, Diagram *dia, Layer *layer);
void dia_layer_update_from_layer(DiaLayerWidget *widget);

#endif /* LAYER_DIALOG_H */

