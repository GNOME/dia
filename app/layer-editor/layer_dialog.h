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

enum LayerChangeType {
  TYPE_DELETE_LAYER,
  TYPE_ADD_LAYER,
  TYPE_RAISE_LAYER,
  TYPE_LOWER_LAYER,
};

#define DIA_TYPE_LAYER_CHANGE dia_layer_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaLayerChange, dia_layer_change, DIA, LAYER_CHANGE, DiaChange)

DiaChange *dia_layer_change_new            (Diagram              *dia,
                                            DiaLayer             *layer,
                                            enum LayerChangeType  type,
                                            int                   index);


#define DIA_TYPE_LAYER_VISIBILITY_CHANGE dia_layer_visibility_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaLayerVisibilityChange, dia_layer_visibility_change, DIA, LAYER_VISIBILITY_CHANGE, DiaChange)

DiaChange *dia_layer_visibility_change_new (Diagram  *dia,
                                            DiaLayer *layer,
                                            gboolean  exclusive);


void layer_dialog_create      (void);
void layer_dialog_show        (void);
void layer_dialog_set_diagram (Diagram *dia);

/* Integrated UI component */
GtkWidget * create_layer_view_widget (void);

void diagram_edit_layer (Diagram *dia, DiaLayer *layer);

G_END_DECLS
