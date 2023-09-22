/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * dia-props.h - a dialog for the diagram properties
 * Copyright (C) 2000 James Henstridge
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

#include "diagram.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DIA_TYPE_DIAGRAM_PROPERTIES_DIALOG dia_diagram_properties_dialog_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaDiagramPropertiesDialog, dia_diagram_properties_dialog, DIA, DIAGRAM_PROPERTIES_DIALOG, GtkDialog)

struct _DiaDiagramPropertiesDialogClass {
  /*< private >*/
  GtkDialogClass parent;
};


void                        dia_diagram_properties_dialog_set_diagram (DiaDiagramPropertiesDialog *self,
                                                                       Diagram                    *diagram);
Diagram                    *dia_diagram_properties_dialog_get_diagram (DiaDiagramPropertiesDialog *self);
DiaDiagramPropertiesDialog *dia_diagram_properties_dialog_get_default (void);

void diagram_properties_show(Diagram *dia);


G_END_DECLS
