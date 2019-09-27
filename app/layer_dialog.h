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
#include "diagram.h"

G_BEGIN_DECLS

#define DIA_TYPE_LAYER_EDITOR dia_layer_editor_get_type ()

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkVBox, g_object_unref)

G_DECLARE_DERIVABLE_TYPE (DiaLayerEditor, dia_layer_editor, DIA, LAYER_EDITOR, GtkVBox)

struct _DiaLayerEditorClass {
  GtkVBoxClass parent;
};

GtkWidget *dia_layer_editor_new         (void);
void       dia_layer_editor_set_diagram (DiaLayerEditor *self,
                                         Diagram        *dia);
Diagram   *dia_layer_editor_get_diagram (DiaLayerEditor *self);



#define DIA_TYPE_LAYER_EDITOR_DIALOG dia_layer_editor_dialog_get_type ()

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkDialog, g_object_unref)

G_DECLARE_DERIVABLE_TYPE (DiaLayerEditorDialog, dia_layer_editor_dialog, DIA, LAYER_EDITOR_DIALOG, GtkDialog)

struct _DiaLayerEditorDialogClass {
  GtkDialogClass parent;
};

GtkWidget *dia_layer_editor_dialog_new         (void);
void       dia_layer_editor_dialog_set_diagram (DiaLayerEditorDialog *self,
                                                Diagram              *dia);
Diagram   *dia_layer_editor_dialog_get_diagram (DiaLayerEditorDialog *self);



#define DIA_TYPE_LAYER_PROPERTIES dia_layer_properties_get_type ()

G_DECLARE_DERIVABLE_TYPE (DiaLayerProperties, dia_layer_properties, DIA, LAYER_PROPERTIES, GtkDialog)

struct _DiaLayerPropertiesClass {
  GtkDialogClass parent;
};

GtkWidget *dia_layer_properties_new         (void);
void       dia_layer_properties_set_layer   (DiaLayerProperties *self,
                                             DiaLayer           *layer);
DiaLayer  *dia_layer_properties_get_layer   (DiaLayerProperties *self);
void       dia_layer_properties_set_diagram (DiaLayerProperties *self,
                                             Diagram            *dia);
Diagram   *dia_layer_properties_get_diagram (DiaLayerProperties *self);



#define DIA_TYPE_LAYER_WIDGET dia_layer_widget_get_type ()

G_DEFINE_AUTOPTR_CLEANUP_FUNC (GtkListItem, g_object_unref)

G_DECLARE_DERIVABLE_TYPE (DiaLayerWidget, dia_layer_widget, DIA, LAYER_WIDGET, GtkListItem)

struct _DiaLayerWidgetClass {
  GtkListItemClass parent;
};

GtkWidget      *dia_layer_widget_new             (DiaLayer       *layer,
                                                  DiaLayerEditor *editor);
void            dia_layer_widget_set_layer       (DiaLayerWidget *self,
                                                  DiaLayer       *layer);
DiaLayer       *dia_layer_widget_get_layer       (DiaLayerWidget *self);
void            dia_layer_widget_set_editor      (DiaLayerWidget *self,
                                                  DiaLayerEditor *editor);
DiaLayerEditor *dia_layer_widget_get_editor      (DiaLayerWidget *self);
void            dia_layer_widget_set_connectable (DiaLayerWidget *self,
                                                  gboolean        on);
gboolean        dia_layer_widget_get_connectable (DiaLayerWidget *self);


void layer_dialog_create      (void);
void layer_dialog_show        (void);
void layer_dialog_set_diagram (Diagram *dia);

/* Integrated UI component */
GtkWidget * create_layer_view_widget (void);

void diagram_edit_layer (Diagram *dia, DiaLayer *layer);

G_END_DECLS
