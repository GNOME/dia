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
#include "layer-editor/dia-layer-editor-dialog.h"

G_BEGIN_DECLS

#define DIA_TYPE_GUIDE_DIALOG dia_guide_dialog_get_type ()
G_DECLARE_DERIVABLE_TYPE (DiaGuideDialog, dia_guide_dialog, DIA, GUIDE_DIALOG, GtkDialog)

struct _DiaGuideDialogClass {
  /*< private >*/
  GtkDialogClass parent;
};

GtkWidget *dia_guide_dialog_new (GtkWindow *parent,
                                 Diagram   *dia);

G_END_DECLS
